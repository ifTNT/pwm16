#ifndef PWM16_H
#define PWM16_H

#include "Arduino.h"

#if !defined(__AVR_ATmega328P__) && !defined(__AVR_ATmega168__)
#error "Currently, only the board based on ATmega328/168 is supported."
#endif

class Pwm16 {
 public:
  enum Mode { MODE_2CH, MODE_4CH, MODE_4CH_REVERSE };
  enum Channel { CH_0A, CH_0B, CH_1A, CH_1B };
  void init(enum Mode mode);
  void analogWrite16(enum Channel ch, uint16_t duty);

 private:
  enum Mode m_mode;
  const uint8_t m_channel_pin[4][2] = {{9, 9}, {10, 10}, {6, 11}, {3, 5}};
  void initTimer0(bool isHighHalf);
  void initTimer1();
  void initTimer2(bool isHighHalf);
  inline void calcOCR(uint8_t* lowHalf, uint8_t* highHalf, uint16_t val);
};

#endif  // PWN16_H