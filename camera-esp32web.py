import cv2
from threading import Thread,Lock
import time
import requests
from params import URL

use_thread = False
need_flip = False
cap = None
frame = None

# public API
# init(), read_frame(), stop()

def init(res=(160, 120), fps=30, threading=True):
    print ("Initilize camera.")
    global cap, use_thread, frame, cam_thr
    cam_res = 1
    cap = cv2.VideoCapture(URL + ":81/stream")
    if not cap.isOpened():
        print ("Cannot open camera.")
        return    
    # 0 - 96x96, 1 - 160x120, 2 - 176x144, 3 - 320x240, 4 - 352x288, 
    # 5 - 640x480, 6 - 800x600, 7 - 1024x768, 8 - 1280x1024, 9 - 1600x1200
    if res == (96, 96): cam_res = 0
    elif res == (160, 120): cam_res = 1
    elif res == (176, 144): cam_res = 2
    elif res == (320, 240): cam_res = 3
    else:
        print ("Camera resolution not supported.")
        return
    requests.get(URL + "/control?var=framesize&val={}".format(cam_res)) 
    
    # start the camera thread
    if threading:
        use_thread = True
        cam_thr = Thread(target=__update, args=())
        cam_thr.start()
        print ("start camera thread")
        time.sleep(1.0)
    else:
        print ("No camera threading.")
    if need_flip == True:
        print ("camera is Flipped")
    print ("camera init completed.")

def __update():
    global frame
    while use_thread:
        ret, tmp_frame = cap.read() # blocking read
        if need_flip == True:
            frame = cv2.flip(tmp_frame, -1)
        else:
            frame = tmp_frame
    print ("Camera thread finished...")
    cap.release()        

def read_frame():
    global frame
    if not use_thread:
       ret, frame = cap.read() # blocking read
    return frame

def stop():
    global use_thread
    print ("Close the camera.")
    use_thread = False

if __name__ == "__main__":
    init()
    while True:
        frame = read_frame()
        cv2.imshow('frame', frame)
        ch = cv2.waitKey(1) & 0xFF
        if ch == ord('q'):
            stop()
            break
    
