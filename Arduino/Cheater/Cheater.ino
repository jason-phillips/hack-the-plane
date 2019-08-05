#include <PowerFunctions.h>

PowerFunctions pf0(10, 0);
PowerFunctions pf1(10, 1);
PowerFunctions pf2(10, 2);
PowerFunctions pf3(10, 3);

void setup() {
  Serial.begin(9600);
  Serial.println(F("READY"));
}

void allForward(void) {
  pf0.combo_pwm(PWM_FWD7,PWM_FWD7);
  pf1.combo_pwm(PWM_FWD7,PWM_FWD7);
  pf2.combo_pwm(PWM_FWD7,PWM_FWD7);
  pf3.combo_pwm(PWM_FWD7,PWM_FWD7);
}

void allStop(void) {
  pf0.combo_pwm(PWM_BRK,PWM_BRK);
  pf1.combo_pwm(PWM_BRK,PWM_BRK);
  pf2.combo_pwm(PWM_BRK,PWM_BRK);
  pf3.combo_pwm(PWM_BRK,PWM_BRK);
  pf0.combo_pwm(PWM_FLT,PWM_FLT);
  pf1.combo_pwm(PWM_FLT,PWM_FLT);
  pf2.combo_pwm(PWM_FLT,PWM_FLT);
  pf3.combo_pwm(PWM_FLT,PWM_FLT);
}

void loop() {
  allForward();
  delay(10000);
  allStop();
}
