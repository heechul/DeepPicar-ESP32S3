#define ACTUATOR_SERVOS

#ifdef ACTUATOR_SERVOS
#include <Arduino.h>

int lr_pwm_Pin = 3; // left/right throttle
int fr_pwm_Pin = 4; // forward/reverse throttle

// Setting PWM properties

int throttlePwmChannel = 0;
int steeringPwmChannel = 1;

void setup_control() {
  // sets the pins as outputs:
  pinMode(lr_pwm_Pin, OUTPUT);
  pinMode(fr_pwm_Pin, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(throttlePwmChannel, 50, 14);
  ledcAttachPin(fr_pwm_Pin, throttlePwmChannel);
  ledcWrite(throttlePwmChannel, 0);

  ledcSetup(steeringPwmChannel, 50, 14);
  ledcAttachPin(lr_pwm_Pin, steeringPwmChannel);
  ledcWrite(steeringPwmChannel, 0);
}

void set_throttle(int throttle_pct)
{
  int duty_us = map(throttle_pct, -100, 100, 900, 1700);
  int duty = map(duty_us, 900, 1700, 0, 16383);
  printf("%s:duty_us: %d, duty: %d\n", __FUNCTION__, duty_us, duty);
  ledcWrite(throttlePwmChannel, duty);
}

void set_steering(int steering_deg)
{
  int duty_us = map(steering_deg, -90, 90, 544, 2400);
  int duty = map(duty_us, 544, 2400, 0, 16383);
  printf("%s:duty_us: %d, duty: %d\n", __FUNCTION__, duty_us, duty);
  ledcWrite(steeringPwmChannel, duty);
}

// steering
void left() 
{
  set_steering(-30);
}
void right()
{
  set_steering(30);
}
void center() {
  set_steering(0);
}

// throttle
int throttle_pct = 0;

void forward() 
{
  throttle_pct = 10;
  set_throttle(throttle_pct);
}
void backward() 
{
  throttle_pct = -10;
  set_throttle(throttle_pct);
}
void nomove()
{
  set_throttle(0);
}
void throttleup() 
{
  if (throttle_pct < 100)
  {
    throttle_pct += 10;
  }
  set_throttle(throttle_pct);
}
void throttledown() 
{
  if (throttle_pct > -100)
  {
    throttle_pct -= 10;
  }
  set_throttle(throttle_pct);
}

#endif // ACTUATOR_8835