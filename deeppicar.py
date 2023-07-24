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
use_dnn = False
use_thread = True
view_video = False
fpv_video = False
enable_record = False
cfg_cam_res = (160, 120)
cfg_cam_fps = 30

frame_id = 0
angle = 0.0
period = 0.05 # sec (=50ms)

##########################################################
# local functions
##########################################################
def deg2rad(deg):
    return deg * math.pi / 180.0
def rad2deg(rad):
    return 180.0 * rad / math.pi

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

# scaled crop. return img_height x img_width image
def get_image(img):
    orig_h, orig_w, _ = img.shape
    scaled_h = int(orig_h * params.img_width / orig_w)
    scaled_w = params.img_width
    scaled_img = cv2.resize(img, (scaled_w, scaled_h))
    # print(scaled_img.shape)
    # crop bottom center pixels of the model input size
    startx = int((scaled_w - params.img_width) * 0.5);
    starty = int((scaled_h - params.img_height) * 1.0);
    return scaled_img[starty:starty+params.img_height, startx:startx+params.img_width,:]
 
def preprocess(img):
    img = get_image(img)
    # Convert to grayscale and readd channel dimension
    if params.img_channels == 1:
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        img = np.reshape(img, (params.img_height, params.img_width, params.img_channels))
    img = img / 255.
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
parser.add_argument("-d", "--dnn", help="Enable DNN", action="store_true")
parser.add_argument("-t", "--throttle", help="throttle percent. [0-100]%", type=int, default=100)
parser.add_argument("--turnthresh", help="throttle percent. [0-30]degree", type=int, default=10)
parser.add_argument("-n", "--ncpu", help="number of cores to use.", type=int, default=2)
parser.add_argument("-f", "--hz", help="control frequnecy", type=int)
parser.add_argument("--fpvvideo", help="Take FPV video of DNN driving", action="store_true")
parser.add_argument("--use_tensorflow", help="use the full tensorflow instead of tflite", action="store_true")
parser.add_argument("--pre", help="preprocessing [resize|crop]", type=str, default="crop")
args = parser.parse_args()

if args.dnn:
    print ("DNN is on")
    use_dnn = True
if args.throttle:
    print ("throttle = %d pct" % (args.throttle))
if args.turnthresh:
    args.turnthresh = args.turnthresh
    print ("turn angle threshold = %d degree\n" % (args.turnthresh))
if args.hz:
    period = 1.0/args.hz
    print("new period: ", period)
if args.fpvvideo:
    fpv_video = True
    print("FPV video of DNN driving is on")

print("period (sec):", period)
print("preprocessing:", args.pre)

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


# initlaize deeppicar modules
actuator.init(args.throttle)
camera.init(res=cfg_cam_res, fps=cfg_cam_fps, threading=use_thread)
atexit.register(turn_off)

g = g_tick()
start_ts = time.time()

frame_arr = []
angle_arr = []
actuator_times = []

# enter main loop
while True:
    if use_thread:
        time.sleep(next(g))
    frame = camera.read_frame()
    ts = time.time()

    if view_video == True:
        cv2.imshow('frame', frame)
        cv2.waitKey(1) & 0xFF

    # receive input (must be non blocking)
    ch = inputdev.read_single_event()
    
    if ch == ord('j'): # left 
        angle = deg2rad(-30)
        actuator.left()
        actuator_times.append(time.time() - ts)
        print ("left")
    elif ch == ord('k'): # center 
        angle = deg2rad(0)
        actuator.center()
        actuator_times.append(time.time() - ts)
        print ("center")
    elif ch == ord('l'): # right
        angle = deg2rad(30)
        actuator.right()
        actuator_times.append(time.time() - ts)
        print ("right")
    elif ch == ord('a'):
        actuator.ffw()
        actuator_times.append(time.time() - ts)
        print ("accel")
    elif ch == ord('s'):
        actuator.stop()
        actuator_times.append(time.time() - ts)
        print ("stop")
    elif ch == ord('z'):
        actuator.rew()
        actuator_times.append(time.time() - ts)
        print ("reverse")
    elif ch == ord('n'):
        actuator.auto()
        print ("auto")
    elif ch == ord('b'):
        actuator.manual()
        print ("manual")
    elif ch == ord('m'):
        n_trials=1000
        print("actuator latency measumenets: {} trials".format(n_trials))
        measure_execution_time(actuator.left, n_trials)
    elif ch == ord('r'):
        print ("toggle record mode")
        enable_record = not enable_record
    elif ch == ord('t'):
        print ("toggle video mode")
        view_video = not view_video
    elif ch == ord('d'):
        print ("toggle DNN mode")
        use_dnn = not use_dnn
    elif ch == ord('q'):
        break

    if use_dnn == True:
        # 1. machine input
        img = preprocess(frame)
        img = np.expand_dims(img, axis=0).astype(np.float32)
        if args.use_tensorflow:
            angle = model.predict(img)[0]
        else:
            interpreter.set_tensor(input_index, img)
            interpreter.invoke()
            angle = interpreter.get_tensor(output_index)[0][0]

        degree = rad2deg(angle)
        if degree <= -args.turnthresh:
            actuator.left()
            print ("left (%d) by CPU" % (degree))
        elif degree < args.turnthresh and degree > -args.turnthresh:
            actuator.center()
            print ("center (%d) by CPU" % (degree))
        elif degree >= args.turnthresh:
            actuator.right()
            print ("right (%d) by CPU" % (degree))

    dur = time.time() - ts
    if dur > period:
        print("%.3f: took %d ms - deadline miss."
              % (ts - start_ts, int(dur * 1000)))
    # else:
    #     print("%.3f: took %d ms" % (ts - start_ts, int(dur * 1000)))
    
    if enable_record == True and frame_id == 0:
        # create files for data recording
        keyfile = open(params.rec_csv_file, 'w+')
        keyfile.write("ts,frame,wheel\n") # ts (ms)
        try:
            fourcc = cv2.cv.CV_FOURCC(*'XVID')
        except AttributeError as e:
            fourcc = cv2.VideoWriter_fourcc(*'XVID')
        vidfile = cv2.VideoWriter(params.rec_vid_file, fourcc,
                                cfg_cam_fps, cfg_cam_res)
    if enable_record == True and frame is not None:
        # increase frame_id
        frame_id += 1

        # write input (angle)
        str = "{},{},{}\n".format(int(ts*1000), frame_id, angle)
        keyfile.write(str)

        if use_dnn and fpv_video:
            textColor = (255,255,255)
            bgColor = (0,0,0)
            newImage = Image.new('RGBA', (100, 20), bgColor)
            drawer = ImageDraw.Draw(newImage)
            drawer.text((0, 0), "Frame #{}".format(frame_id), fill=textColor)
            drawer.text((0, 10), "Angle:{}".format(angle), fill=textColor)
            newImage = cv2.cvtColor(np.array(newImage), cv2.COLOR_BGR2RGBA)
            frame = overlay_image(frame,
                                     newImage,
                                     x_offset = 0, y_offset = 0)
        # write video stream
        vidfile.write(frame)
        #img_name = "cal_images/opencv_frame_{}.png".format(frame_id)
        #cv2.imwrite(img_name, frame)
        if frame_id >= 1000:
            print ("recorded 1000 frames")
            break
        print ("%.3f %d %.3f %d(ms)" %
           (ts, frame_id, angle, int((time.time() - ts)*1000)))

print ("Finish..")
print ("Actuator latency measurements: {} trials".format(len(actuator_times)))
print_stats(actuator_times)
turn_off()
