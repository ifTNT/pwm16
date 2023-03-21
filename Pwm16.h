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

  /**
   * Initialize internal Timer register.
   *
   * \param mode The operation mode of this module. Valid values are:
   * Pwm16::MODE_2CH, Pwm16::MODE_4CH, Pwm16::MODE_4CH_REVERSE
   *
   * \warning Set the mode to MODE_4CH and MODE_4CH_REVERSE will influence the
   * built-in timing function like delay() and millis().
   */
  void init(enum Mode mode);

  /**
   * Alternative version of analogWrite with 16-bits resolution
   *
   * \param ch Output channel
   *
   * \param code The code of duty cycle ranging from 0x0000 to 0xffff. When the
   * duty cycle code is set to 0x0000, the output signal is digital low.
   * Conversely, when the duty cycle code is set to 0xffff, the output signal is
   * digital high.
   */
  void analogWrite16(const enum Channel ch, const uint16_t code);

 private:
  enum Mode m_mode;
  enum Timer { TIMER_0, TIMER_1, TIMER_2 };
  enum Half { HIGH_HALF, LOW_HALF };

  /*
   * Corresponding pin to each channel.
   * The format of CH_1 is {Timer0 pin, Timer2 pin}
   */
  const uint8_t m_channel_pin[4][2] = {{9, 9}, {10, 10}, {6, 11}, {5, 3}};

  void m_setupTimer0(bool chAEnable, bool chBEnable);
  void m_setupTimer1(bool chAEnable, bool chBEnable);
  void m_setupTimer2(bool chAEnable, bool chBEnable);
  void m_pwmCh0(const enum Channel ch, const uint8_t high_duty,
                const uint8_t low_duty);
  void m_pwmCh1(const enum Channel ch, const uint8_t high_duty,
                const uint8_t low_duty);

  /*
   * Digital signal helper.
   */
  void m_digitalWrite(enum Channel ch, enum Half half, uint8_t val);
  void m_disconnectPwm(enum Channel ch, enum Half half);
  void m_clearFlipFlop(enum Channel ch);

  /*
   * Mathmatical building blocks.
   */
  inline bool m_isTimerHalf(enum Timer t, enum Half half);
  inline uint8_t m_dutyToOcr(uint8_t duty);
  inline void m_calcDuty(uint8_t* lowHalf, uint8_t* highHalf, uint16_t val);
};

#endif  // PWN16_H