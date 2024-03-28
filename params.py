#!/usr/bin/env python
##########################################################
# camera module selection
#   "camera-esp32web" "camera-null"
##########################################################
URL="http://192.168.4.1"
camera="camera-esp32web"
# camera="camera-null"

##########################################################
# actuator selection
#   "actuator-null" "actuator-esp32web"
##########################################################
actuator="actuator-esp32web"
# actuator="actuator-null"
##########################################################
# intputdev selection
#   "input-kbd", "input-joystick", "input-web"
##########################################################
inputdev="input-kbd"

##########################################################
# model input config 
#   160x120x3 or 80x60x3 or 40x30x3
##########################################################
img_width = 160
img_height = 120
img_channels = 3
temporal_context = 1
##########################################################
# model selection
#   "model_large"   <-- nvidia dave-2 model
##########################################################
model_name = "opt-k2"
model_file = "models/{}-{}x{}x{}-T{}".format(model_name, img_width, img_height, img_channels, temporal_context)

##########################################################
# recording config 
##########################################################
rec_vid_file="out-video.avi"
rec_csv_file="out-key.csv"
