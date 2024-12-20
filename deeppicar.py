#!/usr/bin/python
import os
import time
import atexit
import cv2
import math
import numpy as np
import sys
import params
import argparse

from PIL import Image, ImageDraw

##########################################################
# import deeppicar's sensor/actuator modules
##########################################################
camera   = __import__(params.camera)
actuator = __import__(params.actuator)
inputdev = __import__(params.inputdev)

##########################################################
# global variable initialization
##########################################################
use_thread = True
view_video = False
enable_record = False
cfg_cam_res = (160, 120)
cfg_cam_fps = 30
enable_ondevice_dnn = False
frame_id = 0
period = 0.05 # sec (=50ms)

##########################################################
# local functions
##########################################################
def deg2rad(deg):
    return deg * math.pi / 180.0
def rad2deg(rad):
    return int(180.0 * rad / math.pi)

def g_tick():
    t = time.time()
    count = 0
    while True:
        count += 1
        yield max(t + count*period - time.time(),0)

def turn_off():
    actuator.stop()
    camera.stop()
    if frame_id > 0:
        keyfile.close()
        vidfile.release()

# Scale the image to the model input size
def get_image_resize(img):
    orig_h, orig_w, _ = img.shape
    scaled_img = cv2.resize(img, (params.img_width, params.img_height))
    return scaled_img

# Crop the image to the model input size
def get_image_crop(img):
    orig_h, orig_w, _ = img.shape
    startx = int((orig_w - params.img_width) * 0.5); # crop from both sides
    starty = int((orig_h - params.img_height) * 0.75); # crop from bottom
    return img[starty:starty+params.img_height, startx:startx+params.img_width,:]
 
def preprocess(img):
    if args.pre == "resize":
        img = get_image_resize(img)
    elif args.pre == "crop":
        img = get_image_crop(img)
    # print("cropped img info: ", img.shape, img.dtype, img.min(), img.max(), img[0][0], img[0][1], img[0][2])

    # Convert to grayscale and read channel dimension
    if params.img_channels == 1:
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        img = np.reshape(img, (params.img_height, params.img_width, 1))
    if args.int8 == True:
        img = img - 128
        img = np.expand_dims(img, axis=0).astype(np.int8)
    else:
        img = img / 255.
        img = np.expand_dims(img, axis=0).astype(np.float32)        
    return img

def overlay_image(l_img, s_img, x_offset, y_offset):
    assert y_offset + s_img.shape[0] <= l_img.shape[0]
    assert x_offset + s_img.shape[1] <= l_img.shape[1]

    l_img = l_img.copy()
    for c in range(0, 3):
        l_img[y_offset:y_offset+s_img.shape[0],
              x_offset:x_offset+s_img.shape[1], c] = (
                  s_img[:,:,c] * (s_img[:,:,3]/255.0) +
                  l_img[y_offset:y_offset+s_img.shape[0],
                        x_offset:x_offset+s_img.shape[1], c] *
                  (1.0 - s_img[:,:,3]/255.0))
    return l_img

# def get_action(angle):
#     degree = rad2deg(angle)
#     if degree <= -args.turnthresh:
#         return "left"
#     elif degree < args.turnthresh and degree > -args.turnthresh:
#         return "center"
#     elif degree >= args.turnthresh:
#         return "right"
#     else:
#         return "unknown"

# def put_action(angle):
#     degree = rad2deg(angle)
#     if degree <= -args.turnthresh:
#         actuator.left()
#     elif degree < args.turnthresh and degree > -args.turnthresh:
#         actuator.center()
#     elif degree >= args.turnthresh:
#         actuator.right()
#     else:
#         actuator.stop()

def print_stats(execution_times):
    # Calculate statistics
    avg = np.mean(execution_times)
    min = np.min(execution_times)
    max = np.max(execution_times)
    p99 = np.percentile(execution_times, 99)
    p90 = np.percentile(execution_times, 90)
    p50 = np.percentile(execution_times, 50)

    print(f"Average Execution Time: {avg:.6f} seconds")
    print(f"Minimum Execution Time: {min:.6f} seconds")
    print(f"Maximum Execution Time: {max:.6f} seconds")
    print(f"99th Percentile Execution Time: {p99:.6f} seconds")
    print(f"90th Percentile Execution Time: {p90:.6f} seconds")
    print(f"50th Percentile Execution Time: {p50:.6f} seconds")

def measure_execution_time(func, num_trials):
    """
    Measure the execution time of a function.

    Args:
        func (function): Function to measure.
        num_trials (int): Number of trials.

    Returns:
        list: List of execution times.
    """
    execution_times = []
    for _ in range(num_trials):
        start_time = time.time()
        func()  # Call the function to measure its execution time
        end_time = time.time()
        execution_time = end_time - start_time
        execution_times.append(execution_time)
    # Calculate statistics
    print_stats(execution_times)

##########################################################
# program begins
##########################################################

parser = argparse.ArgumentParser(description='DeepPicar main')
parser.add_argument("-d", "--dnn", help="Enable DNN", action="store_true", default=False)
parser.add_argument("-p", "--prob_dnn", help="probability of using DNN", type=float, default=1.0)
parser.add_argument("-t", "--throttle", help="throttle percent. [0-100]%", type=int, default=0)
parser.add_argument("--turnthresh", help="throttle percent. [0-30]degree", type=int, default=10)
parser.add_argument("-n", "--ncpu", help="number of cores to use.", type=int, default=2)
parser.add_argument("-f", "--hz", help="control frequnecy", type=int)
parser.add_argument("--fpvvideo", help="Take FPV video of DNN driving", action="store_true")
parser.add_argument("--use_tensorflow", help="use the full tensorflow instead of tflite", action="store_true")
parser.add_argument("--pre", help="preprocessing [resize|crop]", type=str, default="resize")
parser.add_argument("--int8", help="use int8 quantized model", action="store_true", default=True)
args = parser.parse_args()

if args.dnn:
    print ("DNN is on")
if args.int8:
    print ("use int8 quantized model")
if args.throttle:
    print ("throttle = %d pct" % (args.throttle))
if args.turnthresh:
    print ("turn angle threshold = %d degree\n" % (args.turnthresh))
if args.hz:
    period = 1.0/args.hz
    print("new period: ", period)
if args.fpvvideo:
    print("FPV video of DNN driving is on")


print("period (sec):", period)
print("preprocessing:", args.pre)
print("use_int8:", args.int8)
print("prob_dnn:", args.prob_dnn)

##########################################################
# import deeppicar's DNN model
##########################################################
print ("Loading model: " + params.model_file)

print("use_tensorflow:", args.use_tensorflow)
if args.use_tensorflow:
    from tensorflow import keras
    model = keras.models.load_model(params.model_file+'.h5')
else:
    try:
        # Import TFLite interpreter from tflite_runtime package if it's available.
        from tflite_runtime.interpreter import Interpreter
        interpreter = Interpreter(params.model_file+'.tflite', num_threads=args.ncpu)
    except ImportError:
        # Import TFLMicro interpreter
        try:
            from tflite_micro_runtime.interpreter import Interpreter 
            interpreter = Interpreter(params.model_file+'.tflite')
        except:
            # If all failed, fallback to use the TFLite interpreter from the full TF package.
            import tensorflow as tf
            interpreter = tf.lite.Interpreter(model_path=params.model_file+'.tflite', num_threads=args.ncpu)

    interpreter.allocate_tensors()
    input_index = interpreter.get_input_details()[0]["index"]
    output_index = interpreter.get_output_details()[0]["index"]
    # print(interpreter.get_output_details()[0])
    input_type = interpreter.get_input_details()[0]['dtype']
    print('input: ', input_type)
    output_type = interpreter.get_output_details()[0]['dtype']
    print('output: ', output_type)
    quantization = interpreter.get_output_details()[0]['quantization']
    (scale, zerop) = quantization
    print('  zerop: ', zerop, 'scale: ', scale)

# initlaize deeppicar modules
actuator.init(args.throttle)
camera.init(res=cfg_cam_res, fps=cfg_cam_fps, threading=use_thread)
atexit.register(turn_off)

g = g_tick()
start_ts = time.time()

frame_arr = []
angle_arr = []
dnn_times = []
action_times = []
throttle_pct = args.throttle
steering_deg = 0
prev_throttle_pct = -1
prev_steering_deg = -1
dnn_steering_deg = 0

stext = ""

temporal_context_buffer = []
# enter main loop
while True:
    if use_thread:
        time.sleep(next(g))
    frame = camera.read_frame()
    if frame is None:
        print("frame is None")
        break
    ts = time.time()

    # receive input (must be non blocking)
    ch = inputdev.read_single_event()
    
    # process input
    if ch == ord('j'): # left 
        steering_deg = -30
    elif ch == ord('k'): # center 
        steering_deg = 0
    elif ch == ord('l'): # right
        steering_deg = 30
    elif ch == ord('u'):
        steering_deg += -10
    elif ch == ord('o'):
        steering_deg += 10
    elif ch == ord('a'): # accel
        throttle_pct += 5
        start_ts = ts
    elif ch == ord('z'): # reverse
        throttle_pct += -5
    elif ch == ord('s'): # stop
        throttle_pct = 0 
        actuator.manual()
        print ("stop")
        print ("duration: %.2f" % (ts - start_ts))
        enable_record = False # stop recording as well 
        args.dnn = False # manual mode
        enable_ondevice_dnn = False
    elif ch == ord('n'):    
        actuator.auto()
        enable_ondevice_dnn = True
        print ("auto: Ondevice DNN enabled")
    elif ch == ord('b'):
        actuator.manual()
        enable_ondevice_dnn = False
        print ("manual")
    elif ch == ord('m'):
        n_trials=100
        print("actuator latency measumenets: {} trials".format(n_trials))
        measure_execution_time(actuator.left, n_trials)
    elif ch == ord('r'):
        enable_record = not enable_record
        print ("record mode: ", enable_record)
    elif ch == ord('t'):
        print ("toggle video mode")
        view_video = not view_video
    elif ch == ord('d'):
        args.dnn = not args.dnn
        print ("dnn mode:", args.dnn)
    elif ch == ord('q'):
        break

    if args.dnn == True:
        # AI enabled mode
        # 1. machine input
        img = preprocess(frame)
        
        # add temporal context to the input image
        temporal_context_buffer.append(img)
        if len(temporal_context_buffer) < params.temporal_context:
            for i in range(params.temporal_context - len(temporal_context_buffer)):
                temporal_context_buffer.append(img)
        elif len(temporal_context_buffer) > params.temporal_context:
            temporal_context_buffer.pop(0)   

        img = np.concatenate(temporal_context_buffer, axis=3) # (batch, height, weight, channel). 
        # print (img.shape)
        # 2. machine output
        if args.use_tensorflow:
            dnn_angle = model.predict(img)[0]
        else: # tflite
            interpreter.set_tensor(input_index, img)
            interpreter.invoke()
            if output_type == np.float32:
                dnn_angle = interpreter.get_tensor(output_index)[0][0]
            elif output_type == np.int8:
                q = interpreter.get_tensor(output_index)[0][0]
                dnn_angle = scale * (q - zerop)
                # print('dequantized output:', q, angle, rad2deg(angle))
            else:
                print("unknown output type")
                exit(1)
        # dnn latency measurement
        dnn_times.append(time.time() - ts)

        dnn_steering_deg = rad2deg(dnn_angle)
        dnn_throttle_pct = throttle_pct

        # 3. actuator output. 
        #    50% of time choose dnn_angle, while chooing angle for the rest 
        if np.random.rand() < args.prob_dnn:
            actuator.set_steering(dnn_steering_deg)
            actuator.set_throttle(dnn_throttle_pct)
        else:
            actuator.set_steering(steering_deg)
            actuator.set_throttle(throttle_pct)
    
        # latency measurement
        action_times.append(time.time() - ts)
    elif enable_ondevice_dnn == True:
        # AI enabled mode
        pass    # do nothing. 
    else:
        # manual mode
        if prev_steering_deg != steering_deg: 
            actuator.set_steering(steering_deg)
        if prev_throttle_pct != throttle_pct:
            actuator.set_throttle(throttle_pct)

    dur = time.time() - ts
    if dur > period:
        print("%.3f: took %d ms - deadline miss."
              % (ts - start_ts, int(dur * 1000)))
    # else:
    #     print("%.3f: took %d ms" % (ts - start_ts, int(dur * 1000)))
    
    if view_video == True:
        if args.dnn == True:
            if abs(steering_deg - dnn_steering_deg) < 5:
                textColor = (0, 255, 0)
            else:
                textColor = (255,0,0)
            bgColor = (0,0,0)
            newImage = Image.new('RGBA', (140, 20), bgColor)
            drawer = ImageDraw.Draw(newImage)
            drawer.text((0, 0), stext, fill=textColor)
            newImage = cv2.cvtColor(np.array(newImage), cv2.COLOR_BGR2RGBA)
            frame = overlay_image(frame, newImage, x_offset = 0, y_offset = 0)
        cv2.imshow('frame', frame)
        cv2.waitKey(1) & 0xFF

    if enable_record == True:
        assert(not frame is None)
        if frame_id == 0:
            # create files for data recording
            keyfile = open(params.rec_csv_file, 'w+')
            keyfile.write("ts,frame,wheel,ai\n") # ts (ms)
            try:
                fourcc = cv2.cv.CV_FOURCC(*'XVID')
            except AttributeError as e:
                print ("fourcc = cv2.cv.CV_FOURCC(*'XVID') failed")
                fourcc = cv2.VideoWriter_fourcc(*'XVID')
                print ("fourcc = cv2.VideoWriter_fourcc(*'XVID')")
            vidfile = cv2.VideoWriter(params.rec_vid_file, fourcc,
                                    cfg_cam_fps, cfg_cam_res)

        # increase frame_id
        frame_id += 1

        # write input (angle)
        str = "{},{},{}\n".format(int(ts*1000), frame_id, deg2rad(steering_deg))
        keyfile.write(str)

        # write video stream
        vidfile.write(frame)
        #img_name = "cal_images/opencv_frame_{}.png".format(frame_id)
        #cv2.imwrite(img_name, frame)
        if frame_id >= 1000:
            print ("recorded 1000 frames")
            break
        if frame_id % 10 == 0: 
            print ("%.3f %d %.3f %d(ms)" %(ts, frame_id, steering_deg, int((time.time() - ts)*1000)))
    
    # update previous steering angle and throttle
    stext = "EXP: %2d - AI: %2d, Throttle=%2d" % (steering_deg, dnn_steering_deg, throttle_pct)
    if prev_steering_deg != steering_deg or prev_throttle_pct != throttle_pct:
        prev_steering_deg = steering_deg
        prev_throttle_pct = throttle_pct
        print (stext)

print ("Finish..")
if len(dnn_times) > 0:
    print ("DNN latency measurements: {} trials".format(len(dnn_times)))
    print_stats(dnn_times)
if len(action_times) > 0:
    print ("Actuator latency measurements: {} trials".format(len(action_times)))
    print_stats(action_times)
turn_off()
