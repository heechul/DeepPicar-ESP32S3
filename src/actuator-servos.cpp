#define ACTUATOR_SERVOS

#ifdef ACTUATOR_SERVOS
#include <Arduino.h>

int lr_pwm_Pin = 3; // left/right throttle
int fr_pwm_Pin = 4; // forward/reverse throttle

// Setting PWM properties

int throttlePwmChannel = 0;
int steeringPwmChannel = 1;

void servors_test();

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

  servors_test();
}

void set_throttle(int throttle_pct)
{
  int duty_us = map(throttle_pct, -100, 100, 900, 1700);
  int duty = duty_us * 16383 / 20000;
  printf("%s:duty_us: %d, duty: %d\n", __FUNCTION__, duty_us, duty);
  ledcWrite(throttlePwmChannel, duty);
}

void set_steering(int steering_deg)
{
  int duty_us = map(steering_deg, -90, 90, 544, 2400);
  int duty = duty_us * 16383 / 20000;
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

void servors_test()
{
  // receive input from serial monitor to control ESC
  // 
  // PWM values for throttle set by the serial monitor
  //  900 - 1300  reverse
  // 1300 - stop  neutral, arming the servor
  // 1300 - 1700  forward

  while(1)  {
    if (Serial.available() > 0) {
      char c = Serial.read();
      switch (c) {
        case 'a':
          forward();
          break;
        case 's':
          nomove();
          break;
        case 'z':
          backward();
          break;
        case 'i':
          throttleup();
          break;
        case ',':
          throttledown();
          break;
        case 'j':
          left();
          break;
        case 'k':
          center();
          break;
        case 'l':
          right();
          break;
        default:
          break;
      }
    }
  }
}
#endif // ACTUATOR_8835