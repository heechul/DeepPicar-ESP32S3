import requests
import time
from params import URL

def init(throttle_pct=0, steering_deg=0):
    set_throttle(throttle_pct)
    set_steering(steering_deg)

def set_throttle(throttle_pct):
    if throttle_pct >= -100 and throttle_pct <= 100 :
        requests.get(URL + "/control?var=throttle_pct&val={}".format(throttle_pct))

def set_steering(steering_deg):
    if steering_deg >= -90 and steering_deg <= 90 :
        requests.get(URL + "/control?var=steering_deg&val={}".format(steering_deg))

# local control (smart controller)
def auto():
    requests.get(URL + "/control?var=auto&val={}".format(0))

def manual():
    requests.get(URL + "/control?var=manual&val={}".format(0))

def stop():
    set_steering(0)
    set_throttle(0)

# exit    
def turn_off():
    stop()


# test functions
def left():
    set_steering(-30)

def right():
    set_steering(30)

def center():
    set_steering(0)

def ffw():
    set_throttle(10)

def rew():
    set_throttle(-20)
 
if __name__ == "__main__":
    manual()
    print("manual")

    while (True):
        time.sleep(1)
        left()
        print("left")
        time.sleep(1)    
        right()
        print("right")
        time.sleep(1)    
        center()
        print("center")
        time.sleep(1)
        ffw()
        print("ffw")
        time.sleep(1)
        stop()
        print("stop")
        time.sleep(1)    
        rew()
        print("rew")
        time.sleep(1)    
