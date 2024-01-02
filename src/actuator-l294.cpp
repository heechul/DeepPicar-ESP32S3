// #define ACTUATOR_L294

#ifdef ACTUATOR_L294
#include <Arduino.h>

int l_Pin = 1; // left
int r_Pin = 2; // right
int f_Pin = 3; // foward
int b_Pin = 4; // backward
int t_Pin = 5; // throttle

// Setting PWM properties

int dutyCycle = 225;
int pwmChannel = 0;

void setup_control() {
  // sets the pins as outputs:
  pinMode(f_Pin, OUTPUT);
  pinMode(b_Pin, OUTPUT);
  pinMode(l_Pin, OUTPUT);
  pinMode(r_Pin, OUTPUT);
  pinMode(t_Pin, OUTPUT);

  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, 30000, 8);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(t_Pin, pwmChannel);
  ledcWrite(pwmChannel, 0);
}


void run() {
  ledcWrite(pwmChannel, dutyCycle);  
}

void forward() {
  digitalWrite(f_Pin, HIGH);
  digitalWrite(b_Pin, LOW);
  run();
}
void backward() {
  digitalWrite(f_Pin, LOW);
  digitalWrite(b_Pin, HIGH);
  run();
}
void left() {
  digitalWrite(l_Pin, HIGH);
  digitalWrite(r_Pin, LOW);
}
void right() {
  digitalWrite(l_Pin, LOW);
  digitalWrite(r_Pin, HIGH);
}
void center() {
  digitalWrite(l_Pin, LOW);
  digitalWrite(r_Pin, LOW);
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

#endif // ACTUATOR_L294