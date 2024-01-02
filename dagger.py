import cv2
import time
import numpy as np
import math
from PIL import Image, ImageDraw
import glob
from pandas import read_csv
import argparse

use_int8 = False # use int8 or float32
use_thread = False # use thread or not

imgs = []
vals = []

def rad2deg(rad):
    return 180.0 * rad / math.pi
def deg2rad(deg):
    return math.pi * deg / 180.0

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

def get_action(angle):
    degree = rad2deg(angle)
    if degree <= -args.turnthresh:
        return "left"
    elif degree < args.turnthresh and degree > -args.turnthresh:
        return "center"
    elif degree >= args.turnthresh:
        return "right"
    else:
        return "unknown"


if __name__ == "__main__":
    # first argument is the video file path

    parser = argparse.ArgumentParser(description='Dagger main')
    parser.add_argument("--turnthresh", help="throttle percent. [0-30]degree", type=int, default=10)
    parser.add_argument("-v", "--video", help="video file path", type=str, default="./dataset/out-video.avi")
    args = parser.parse_args()

    print (args)

    vid_file_path = args.video
    csv_file_path = vid_file_path.replace('video', 'key').replace('avi', 'csv')

    print(vid_file_path, csv_file_path)
    vid = cv2.VideoCapture(vid_file_path)
    df = read_csv(csv_file_path)

    for val in df["wheel"].values:
        ret,img = vid.read()
        imgs.append(img)
        vals.append(val)
    print(len(imgs), len(vals))

    # Convert lists to numpy arrays and ensure they are of equal length    
    imgs = np.asarray(imgs)  # input images
    vals = np.asarray(vals)  # steering angles
    assert len(imgs) == len(vals)
    print("Loaded {} smaples".format(len(imgs)))

    index = 0
    count = 0

    while True:
        frame = imgs[index]    
        stext = "frame{:03d} {}".format(index, get_action(vals[index]))
        print(stext)

        # overlay text
        textColor = (255,0,0)
        bgColor = (0,0,0)
        newImage = Image.new('RGBA', (100, 15), bgColor)
        drawer = ImageDraw.Draw(newImage)
        drawer.text((0, 0), stext, fill=textColor)
        newImage = cv2.cvtColor(np.array(newImage), cv2.COLOR_BGR2RGBA)
        frame = overlay_image(frame, newImage, x_offset = 0, y_offset = 0)

        # show image
        cv2.imshow('frame', frame)

        ch = cv2.waitKey(0) & 0xFF # wait for keypress
        if ch == ord('q'):
            break
        elif ch == ord('i'):
            index += 1
            index = index % len(imgs)
        elif ch == ord(','):
            index -= 1
            index = index % len(imgs)
        elif ch == ord('j'):
            # left
            if vals[index] != deg2rad(-30):
                vals[index] = deg2rad(-30)
                count += 1
        elif ch == ord('l'):
            # right
            if vals[index] != deg2rad(30):
                vals[index] = deg2rad(30)
                count += 1
        elif ch == ord('k'):
            # straight
            if vals[index] != deg2rad(0):
                vals[index] = deg2rad(0)
                count += 1
        elif ch == ord('s'):
            # save to csv
            # df.to_csv(csv_file_path.replace('.csv', '.orig.csv'), index=False)
            df["wheel"] = vals
            df.to_csv(csv_file_path, index=False)
            print("Changed {} samples".format(count))