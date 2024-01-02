#define ACTUATOR_8835

#ifdef ACTUATOR_8835
#include <Arduino.h>

int lr_dir_Pin = 1; // left/right direction
int fr_dir_Pin = 2; // forward/reverse direction
int lr_pwm_Pin = 3; // left/right throttle
int fr_pwm_Pin = 4; // forward/reverse throttle

// Setting PWM properties

int dutyCycle = 225;
int pwmChannel = 0;

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
  int tmp = throttle_pct * 255 / 100;
  if (tmp < 256 && tmp >= 0) dutyCycle = tmp;
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