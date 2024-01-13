#ifndef CONTROL_H_
#define CONTROL_H_

// setup control
void setup_control();

// steering
void left();
void right();
void center();

// throttle
void run();
void forward();
void backward();
void nomove();
void set_throttle(int throttle_pct);
void throttleup();
void throttledown();

#endif /* CONTROL_H_ */