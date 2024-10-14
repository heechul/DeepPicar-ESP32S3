// #define CONFIG_ACTUATOR_8835

#ifdef CONFIG_ACTUATOR_8835
#include <Arduino.h>

static int lr_dir_Pin = 1; // left/right direction
static int fr_dir_Pin = 2; // forward/reverse direction
static int lr_pwm_Pin = 3; // left/right throttle
static int fr_pwm_Pin = 4; // forward/reverse throttle

// Setting PWM properties

static int dutyCycle = 225;
static int pwmChannel = 0;

void setup_control() {
  // sets the pins as outputs:
  pinMode(lr_dir_Pin, OUTPUT);
  pinMode(fr_dir_Pin, OUTPUT);
  pinMode(lr_pwm_Pin, OUTPUT);
  pinMode(fr_pwm_Pin, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, 30000, 8);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(fr_pwm_Pin, pwmChannel);
  ledcWrite(pwmChannel, 0);
}


void run() {
  ledcWrite(pwmChannel, dutyCycle);  
}

void forward() {
  digitalWrite(fr_dir_Pin, HIGH);
  run();
}
void backward() {
  digitalWrite(fr_dir_Pin, LOW);
  run();
}
void left() {
  digitalWrite(lr_dir_Pin, HIGH);
  digitalWrite(lr_pwm_Pin, HIGH);
}
void right() {
  digitalWrite(lr_dir_Pin, LOW);
  digitalWrite(lr_pwm_Pin, HIGH);
}
void center() {
  digitalWrite(lr_pwm_Pin, LOW);
}

void set_throttle(int throttle_pct)
{
  int tmp = abs(throttle_pct) * 255 / 100;
  if (tmp < 256 && tmp >= 0) dutyCycle = tmp;
 
  if (throttle_pct < 0) {
    backward();
  } else if (throttle_pct >= 0) {
    forward();
  }
}

void set_steering(int steering_deg)
{
  if (steering_deg < -10) {
    left();
  } else if (steering_deg > 10) {
    right();
  } else {
    center();
  }
}

void throttleup() {
  if (dutyCycle < 255) {
    dutyCycle += 5;
    run();
  }
}
void throttledown() {
  if (dutyCycle > 200) {
    dutyCycle -= 5;
    run();
  }
}
void nomove() {
  ledcWrite(pwmChannel, 0);  
}

#endif // ACTUATOR_8835