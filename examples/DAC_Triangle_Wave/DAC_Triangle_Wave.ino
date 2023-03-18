#include "Pwm16.h"

Pwm16 pwm;

void setup() {
  pwm.init(Pwm16::MODE_4CH);
}

uint16_t cnt = 0;
int8_t step = 1;
void loop() {
  pwm.analogWrite16(Pwm16::CH_1B, cnt);

  if (cnt == 0) {
    step = 1;
  } else if (cnt == 0xffff) {
    step = -1;
  }
  cnt += step;
  delayMicroseconds(10);
}