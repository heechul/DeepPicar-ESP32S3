# DeepPicar-ESP32S3

## Driving
Launch 'deeppicar.py' to control DeepPicar. 

For throttle control, use '-t' option. 
For frequnecy control, use '-f' option. 

For example, the following will set the throttle at 50% and the control frequency at 40Hz

    $ python deeppicar.py -t 50 -f 40

At runtime, you can give a command to the car by pressing a key. The supported commands are as follows. 

    'j' - left turn
    'l' - right turn 
    'k' - center
    'a' - acceleration 
    'z' - reverse
    's' - stop 
    'd' - enable/disable DNN
    'r' - enable/disable data recording
    'q' - quit

To collecta data, press 'r' and then drive the car for a while. after recording 1000 frames, the recording session will automatically stop. The video and key input data are stored in out-video.avi and out-key.csv files, respectively. 

## Training

Use RunAll-v2.ipynb to train a model. For example, change the folowing three variables to change 
the model size. 

img_width = 80
img_height = 60
img_channels = 3

## Models

| Model	  |  input res.   | #params      |  flops       |  accuracy  |
|---------|---------------|--------------|--------------|------------|
|Opt-K2(L)|	              |              |              |            |
|Opt-K2(M)|               |              |              |            |
|Opt-K2(S)|               |              |              |            |

## Misc
TFLite model viewer
https://netron.app

## Performance Analysis
TBD

## Design considerations
TBD

### int8 vs. float input image format
TBD