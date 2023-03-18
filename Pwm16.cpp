#include "Pwm16.h"

void Pwm16::init(enum Mode mode) {
  m_mode = mode;

  noInterrupts();

  initTimer1();

  switch (mode) {
    case MODE_4CH:
      initTimer0(false);
      initTimer2(true);
      break;
    case MODE_4CH_REVERSE:
      initTimer0(true);
      initTimer2(false);

      break;
    default:
      break;
  }

  /*
   * To synchronize all timers, we must first halt them and then clear the
   * prescaler and counter of each timer. By doing so, we ensure that all timers
   * start counting from the same starting point, achieving synchronization.
   *
   * It's worth noting that Timer2 uses a different prescaler unit compared to
   * the other timers, but experiments have shown that they can still
   * synchronize without any problem.
   *
   */
  GTCCR = _BV(TSM) | _BV(PSRSYNC) | _BV(PSRASY);
  TCNT0 = 0;
  TCNT1 = 0;
  TCNT2 = 0;
  GTCCR = 0;  // Let the timers free run simultaneously...
  interrupts();
}

void Pwm16::analogWrite16(enum Channel ch, uint16_t duty) {
  if (m_mode == MODE_2CH && ch != CH_0A && ch != CH_0B) return;

  pinMode(m_channel_pin[(int)ch][0], OUTPUT);
  pinMode(m_channel_pin[(int)ch][1], OUTPUT);

  uint8_t timer0_ocr, timer2_ocr;

  calcOCR(&timer0_ocr, &timer2_ocr, duty);

  /*
   * Swap the highHalf and the lowHalf.
   */
  if (m_mode == MODE_4CH_REVERSE) {
    timer0_ocr ^= timer2_ocr;
    timer2_ocr ^= timer0_ocr;
    timer0_ocr ^= timer2_ocr;
  }

  switch (ch) {
    case CH_0A:
      break;
    case CH_0B:
      break;
    case CH_1A:
      OCR0A = timer0_ocr;
      OCR2A = timer2_ocr;
      break;
    case CH_1B:
      OCR0B = timer0_ocr;
      OCR2B = timer2_ocr;
      break;
  }
}

void Pwm16::initTimer0(bool isHighHalf) {
  /*
   * In order to trigger external flip-flop properly, we need to invert the
   * output of the LowHalf.
   * Additionally, the Timer0 is setted to the Fast PWM mode with TOP = 0xff.
   */
  const uint8_t compare_output =
      _BV(COM0A1) | _BV(COM0B1) | (isHighHalf ? 0 : _BV(COM0A0) | _BV(COM0B0));
  const uint8_t mode = _BV(WGM01) | _BV(WGM00);
  TCCR0A = compare_output | mode;

  /*
   * Set the prescaler to 256 for the HighHalf, and 1 for the LowHalf.
   */
  TCCR0B = isHighHalf ? _BV(CS02) : _BV(CS00);

  /*
   * Set the duty cycle to 0 to prevent output voltage.
   */
  OCR0A = 0;
  OCR0B = 0;
}
void Pwm16::initTimer1() {}

void Pwm16::initTimer2(bool isHighHalf) {
  /*
   * In order to trigger external flip-flop properly, we need to invert the
   * output of the LowHalf.
   * Additionally, the Timer2 is setted to the Fast PWM mode with TOP = 0xff.
   */
  const uint8_t compare_output =
      _BV(COM2A1) | _BV(COM2B1) | (isHighHalf ? 0 : _BV(COM2A0) | _BV(COM2B0));
  const uint8_t mode = _BV(WGM21) | _BV(WGM20);
  TCCR2A = compare_output | mode;

  /*
   * Set the prescaler to 256 for the HighHalf, and 1 for the LowHalf.
   */
  TCCR2B = isHighHalf ? _BV(CS22) | _BV(CS21) : _BV(CS20);

  /*
   * Set the duty cycle to 0 to prevent output voltage.
   */
  OCR2A = 0;
  OCR2B = 0;
}
inline void Pwm16::calcOCR(uint8_t* lowHalf, uint8_t* highHalf, uint16_t val) {
  /*
   * In the ATmega328p, a duty cycle of 255 corresponds to a voltage of 5V.
   * Therefore, in order to prevent any potential issues, we should limit the
   * LowHalf value to a range of 0 to 254. This ensures that the duty cycle
   * stays within a safe range and prevents any voltage spikes or other related
   * problems.
   */
  *highHalf = (val >> 8) & 0xff;
  *lowHalf = (val & 0xff) - !!(val & 0xff);
}