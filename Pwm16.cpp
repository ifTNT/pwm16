#include "Pwm16.h"

void Pwm16::init(enum Mode mode) {
  m_mode = mode;

  noInterrupts();

  m_setupTimer0(false, false);
  m_setupTimer1(false, false);
  m_setupTimer2(false, false);

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

void Pwm16::analogWrite16(const enum Channel ch, const uint16_t code) {
  if (m_mode == MODE_2CH && ch != CH_0A && ch != CH_0B) return;

  pinMode(m_channel_pin[(int)ch][0], OUTPUT);
  pinMode(m_channel_pin[(int)ch][1], OUTPUT);

  if (code == 0xffff) {
    // Output high to the specified channel
    m_digitalWrite(ch, HIGH_HALF, HIGH);
    m_digitalWrite(ch, LOW_HALF, HIGH);
    return;
  } else if (code == 0x0000) {
    // Output low to the specified channel
    m_digitalWrite(ch, HIGH_HALF, LOW);
    m_digitalWrite(ch, LOW_HALF, LOW);
    m_clearFlipFlop(ch);
    return;
  }

  uint8_t high_duty, low_duty;
  m_calcDuty(&high_duty, &low_duty, code);

  switch (ch) {
    case CH_0A:
    case CH_0B:
      m_pwmCh0(ch, high_duty, low_duty);
      break;
    case CH_1A:
    case CH_1B:
      m_pwmCh1(ch, high_duty, low_duty);
      break;
  }
}

/**
 * Setup duty cycle of channel 0
*/
void Pwm16::m_pwmCh0(const enum Channel ch, const uint8_t high_duty,
                const uint8_t low_duty) {
  m_setupTimer1(ch == CH_0A, ch == CH_0B);
  switch (ch) {
    case CH_0A:
      OCR1A = (uint16_t)high_duty << 8 | low_duty;
      break;
    case CH_0B:
      OCR1B = (uint16_t)high_duty << 8 | low_duty;
      break;
    default:;
  }
}

/**
 * Setup the PWM duty cycle of both HighHalf and LowHalf of channel 1
*/
void Pwm16::m_pwmCh1(const enum Channel ch, const uint8_t high_duty,
                     const uint8_t low_duty) {
  // [TODO] The output of duty cycle below 256 is currently unavailable, so we
  // bypass the situation here.
  if (high_duty == 0x00) {
    m_digitalWrite(ch, HIGH_HALF, LOW);
    m_digitalWrite(ch, LOW_HALF, LOW);
    m_clearFlipFlop(ch);
    return;
  }

  m_setupTimer0(ch == CH_1A, ch == CH_1B);
  m_setupTimer2(ch == CH_1A, ch == CH_1B);
  // Calculate the OCR
  uint8_t t0_ocr, t2_ocr;
  if (m_isTimerHalf(TIMER_2, HIGH_HALF)) {
    t0_ocr = m_dutyToOcr(low_duty);
    t2_ocr = m_dutyToOcr(high_duty);
  } else {
    t0_ocr = m_dutyToOcr(low_duty);
    t2_ocr = m_dutyToOcr(high_duty);
  }

  if (low_duty == 0x00) {
    // Clear the flip flop
    m_clearFlipFlop(ch);
  }

  if (high_duty == 0x00) {
    // Output low to HighHalf
    m_digitalWrite(ch, HIGH_HALF, LOW);
    m_clearFlipFlop(ch);
  }

  switch (ch) {
    case CH_1A:
      OCR0A = t0_ocr;
      OCR2A = t2_ocr;
      break;
    case CH_1B:
      OCR0B = t0_ocr;
      OCR2B = t2_ocr;
      break;
    default:;
  }
}

/**
 * Setup required register of Timer0 for PWM.
 *
 * \param chAEnable Whether channel A should be enable. False means not to
 * modify current setting.
 *
 * \param chBEnable Whether channel B should be enable. False means not to
 * modify current setting.
 */
void Pwm16::m_setupTimer0(bool chAEnable, bool chBEnable) {
  /*
   * In order to trigger external flip-flop properly, we need to invert the
   * output of the LowHalf.
   * Additionally, the Timer0 is setted to the Fast PWM mode with TOP = 0xff.
   */
  uint8_t compare_output = TCCR0A & 0xf0;
  const uint8_t is_low_half_mask = m_isTimerHalf(TIMER_0, HIGH_HALF) - 1;
  if (chAEnable) {
    compare_output &= ~_BV(COM0A1) & ~_BV(COM0A0);
    compare_output |= _BV(COM0A1) | (is_low_half_mask & _BV(COM0A0));
  }
  if (chBEnable) {
    compare_output &= ~_BV(COM0B1) & ~_BV(COM0B0);
    compare_output |= _BV(COM0B1) | (is_low_half_mask & _BV(COM0B0));
  }
  const uint8_t mode = _BV(WGM01) | _BV(WGM00);
  TCCR0A = compare_output | mode;

  /*
   * Set the prescaler to 256 for the HighHalf, and 1 for the LowHalf.
   */
  TCCR0B = m_isTimerHalf(TIMER_0, HIGH_HALF) ? _BV(CS02) : _BV(CS00);
}

/**
 * Setup required register of Timer1 for PWM.
 *
 * \param chAEnable Whether channel A should be enable. False means not to
 * modify current setting.
 *
 * \param chBEnable Whether channel B should be enable. False means not to
 * modify current setting.
 */
void Pwm16::m_setupTimer1(bool chAEnable, bool chBEnable) {
  /*
   * We have configured Timer1 to operate in Mode 14, which is a Fast PWM mode.
   * In this mode, the TOP value is set to the value of the ICR1 register.
   * Additionally, we have set the compare output to the non-inverting mode.
   * This means that the output signal will be set at the BOTTOM and cleared
   * when a compare match occurs.
   *
   * To achieve the highest output frequency possible, we have set the prescaler
   * to 1. This will cause the timer to increment at the maximum rate, resulting
   * in the fastest output frequency that is achievable with this configuration.
   *
   * Overall, this setup is optimized for fast, accurate PWM output. By using
   * Mode 14 and setting the prescaler to 1, we have achieved the fastest output
   * possible with this timer configuration.
   */
  uint8_t compare_output = TCCR1A & 0xf0;
  if (chAEnable) {
    compare_output &= ~_BV(COM1A1) & ~_BV(COM1A0);
    compare_output |= _BV(COM1A1);
  }
  if (chBEnable) {
    compare_output &= ~_BV(COM1B1) & ~_BV(COM1B0);
    compare_output |= _BV(COM1B1);
  }
  TCCR1A = compare_output | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1 = 0xffff;
}

/**
 * Setup required register of Timer2 for PWM.
 *
 * \param chAEnable Whether channel A should be enable. False means not to
 * modify current setting.
 *
 * \param chBEnable Whether channel B should be enable. False means not to
 * modify current setting.
 */
void Pwm16::m_setupTimer2(bool chAEnable, bool chBEnable) {
  /*
   * In order to trigger external flip-flop properly, we need to invert the
   * output of the LowHalf.
   * Additionally, the Timer2 is setted to the Fast PWM mode with TOP = 0xff.
   */
  uint8_t compare_output = 0;
  const uint8_t is_low_half_mask = m_isTimerHalf(TIMER_2, HIGH_HALF) - 1;
  if (chAEnable) {
    compare_output &= ~_BV(COM2A1) & ~_BV(COM2A0);
    compare_output |= _BV(COM2A1) | (is_low_half_mask & _BV(COM2A0));
  }
  if (chBEnable) {
    compare_output &= ~_BV(COM2B1) & ~_BV(COM2B0);
    compare_output |= _BV(COM2B1) | (is_low_half_mask & _BV(COM2B0));
  }
  const uint8_t mode = _BV(WGM21) | _BV(WGM20);
  TCCR2A = compare_output | mode;

  /*
   * Set the prescaler to 256 for the HighHalf, and 1 for the LowHalf.
   */
  TCCR2B =
      m_isTimerHalf(TIMER_2, HIGH_HALF) ? _BV(CS22) | _BV(CS21) : _BV(CS20);
}

/**
 * Modified digitalWrite() to avoid overriding Timer setting.
*/
void Pwm16::m_digitalWrite(enum Channel ch, enum Half half, uint8_t val) {
  // Retrieve the pin number from specified channel and HighHalf/LowHalf
  const uint8_t* pin_pair = m_channel_pin[(int)ch];
  const uint8_t pin = pin_pair[m_isTimerHalf(TIMER_2, half)];

  // Following code if from Arduino
  const uint8_t bit = digitalPinToBitMask(pin);
  const uint8_t port = digitalPinToPort(pin);
  volatile uint8_t* out;

  if (port == NOT_A_PIN) return;

  m_disconnectPwm(ch, half);

  out = portOutputRegister(port);

  const uint8_t oldSREG = SREG;
  cli();

  if (val == LOW) {
    *out &= ~bit;
  } else {
    *out |= bit;
  }

  SREG = oldSREG;
}

/**
 * Disconnect half-channel from internal PWM generator
*/
void Pwm16::m_disconnectPwm(enum Channel ch, enum Half half) {
  switch (ch) {
    case CH_0A:
      TCCR1A &= ~_BV(COM1A0) & ~_BV(COM1A1);
      break;
    case CH_0B:
      TCCR1A &= ~_BV(COM1B0) & ~_BV(COM1B1);
      break;
    case CH_1A:
      if (m_isTimerHalf(TIMER_0, half)) {
        TCCR0A &= ~_BV(COM0A0) & ~_BV(COM0A1);
      } else {
        TCCR2A &= ~_BV(COM2A0) & ~_BV(COM2A1);
      }
      break;
    case CH_1B:
      if (m_isTimerHalf(TIMER_0, half)) {
        TCCR0A &= ~_BV(COM0B0) & ~_BV(COM0B1);
      } else {
        TCCR2A &= ~_BV(COM2B0) & ~_BV(COM2B1);
      }
      break;
  }
}

/**
 * Clear the LowHalf flip-flop output state.
*/
void Pwm16::m_clearFlipFlop(enum Channel ch) {
  if (ch == CH_1A || ch == CH_1B) {
    const uint8_t tmp_tccr0a = TCCR0A;
    const uint8_t tmp_tccr2a = TCCR2A;
    m_digitalWrite(ch, HIGH_HALF, LOW);
    m_digitalWrite(ch, LOW_HALF, LOW);
    m_digitalWrite(ch, LOW_HALF, HIGH);
    // Restore the state of HighHalf
    if (m_isTimerHalf(TIMER_0, HIGH_HALF)) {
      TCCR0A = tmp_tccr0a;
    } else {
      TCCR2A = tmp_tccr2a;
    }
  }
}

/**
 * This function is designed to convert a duty cycle to a value of Timer OCR
 * (Output Compare Register) using the attached table. It's important to note
 * that the conversion result of a duty cycle of 0 is invalid and should not be
 * used.
 * +-------------------+-------+-------+-------+---+------+-------+
 * | PWM Duty(/0xff)   | 0x00  | 0x01  | 0x02  |   | ...  | 0xff  |
 * +-------------------+-------+-------+-------+---+------+-------+
 * | Timer OCR         | Inv.  | 0x00  | 0x01  |   | ...  | 0xfe  |
 * +-------------------+-------+-------+-------+---+------+-------+
 */
inline uint8_t Pwm16::m_dutyToOcr(uint8_t duty) { return duty - 1; }

/**
 * This function is designed to calculate the duty cycle of the HighHalf and the
 * LowHalf from given code. The relationship between the code and the duty cycle
 * is shown in the following table:
 *
 * +--------+---------+-------------+-------------+
 * |  Code  | Desired |  HighHalf   |   LowHalf   |
 * |        |  Duty   | Duty(/0xff) | Duty(/0xff) |
 * +--------+---------+-------------+-------------+
 * | 0x0000 | Low     | Low(0x00)   | Clear(0x00) |
 * | 0x0001 | 0x0002* | Low(0x00)   | 0x02        |
 * | .      |         |             |             |
 * | 0x00fe | 0x00ff* | Low(0x00)   | 0xff        |
 * | 0x00ff | 0x0100* | 0x01        | Clear(0x00) |
 * | 0x0100 | 0x0101  | 0x01        | 0x01        |
 * | .      |         |             |             |
 * | 0xfefe | 0xfeff  | 0xfe        | 0xff        |
 * | 0xfeff | 0xff00  | 0xff        | Clear(0x00) |
 * | 0xff01 | 0xff01  | 0xff        | 0x01        |
 * | .      |         |             |             |
 * | 0xfffe | 0xffff  | 0xff        | 0xff        |
 * | 0xffff | High    | Invalid     | Invalid     |
 * +--------+---------+-------------+-------------+
 * (*): Can't generate the desired duty cycle currently.
 *
 * It's important to note that the result of a code of 0xffff is invalid and
 * should be avoided. Additionally, the duty cycle of 0x00 in the LowHalf
 * implies clear the flip-flip, this mechanism should be implemented.
 */
inline void Pwm16::m_calcDuty(uint8_t* highHalf, uint8_t* lowHalf,
                              uint16_t code) {
  code += !!code;
  *highHalf = (code >> 8) & 0xff;
  *lowHalf = code & 0xff;
}

/**
 * Return whether timer is that half.
 *
 * Karnaugh map:
 * t0 = Whether timer is timer0
 * r = Whether mode is reversed
 * h = Whether is high half
 * +------+-------+------+-----+------+
 * | .    | !r,!h | !r,h | r,h | r,!h |
 * +------+-------+------+-----+------+
 * | t0   | 1     | 0    | 1   | 0    |
 * +------+-------+------+-----+------+
 * | !t0  | 0     | 1    | 0   | 1    |
 * +------+-------+------+-----+------+
 *
 * Boolean algebra:
 * f = t0!r!h + !t0!rh + t0rh + !t0r!h
 *   = t0(!r!h + rh) + !t0(!rh + r!h)
 *   = t0!(r ⊕ h) + !t0(r ⊕ h)
 *   = t0 ⊕ r ⊕ h
 */
bool Pwm16::m_isTimerHalf(enum Timer t, enum Half half) {
  if (t == TIMER_1 || m_mode == MODE_2CH) return false;

  const bool t0 = t == TIMER_0;
  const bool r = m_mode == MODE_4CH_REVERSE;
  const bool h = half == HIGH_HALF;

  return t0 ^ r ^ h;
}