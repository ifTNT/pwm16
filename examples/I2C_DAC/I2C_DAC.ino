#include <Pwm16.h>
#include <Wire.h>

Pwm16 pwm;

void setup() {
  Serial.begin(115200);

  Wire.begin(0x08);
  Wire.onReceive(rxCallback);

  pwm.init(Pwm16::MODE_4CH);
  pwm.analogWrite16(Pwm16::CH_1B, 0);

  Serial.println("Waiting for command from I2C...");
}

void loop() { ; }

/*
 * Write the duty-cycle while receiving command from the I2C bus.
 */
void rxCallback(int num_bytes) {
  uint16_t duty = 0;
  Wire.readBytes((uint8_t*)&duty, 2);
  pwm.analogWrite16(Pwm16::CH_0A, duty);
  pwm.analogWrite16(Pwm16::CH_1B, duty);
  Serial.println(duty, HEX);
}