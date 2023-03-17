# Arduino 16bits DAC/PWM

Provide two extra 16-bits PWM channels to Arduino UNO/Nano.

## Introduction

The main focus of this project is to create extra channels of 16-bit PWM in the MCU using the available 4 channels of 8-bit PWM on Atmega328p. This is important because high-resolution PWM is necessary for creating a software DAC. While the Atmega328p chip can provide 16 bits of PWM, it only has two channels, so using an external IC such as PCA9685 or MCP4725 becomes necessary when more channels are required, which comes at an additional cost. However, by combining the available 4 channels of 8-bit PWM, we can create two extra channels of 16-bit PWM without the need for an external IC. This can save costs and simplify the design process.

### Related Work

The [dual PWM circuits](http://www.openmusiclabs.com/learning/digital/pwm-dac/dual-pwm-circuits/index.html) is a commonly used method for combining multiple low-resolution DACs to create a high-resolution one. It uses a voltage divider circuit to attenuate the low half voltage and add them with the high half voltage to create higher precision. However, it has a precision problem. Specifically, the 0.25 LSB of error from the high half of the DAC will contribute 64 LSBs of error in the output, which is not acceptable for high-precision applications.

On the other hand, [Arduino-True-DAC](https://github.com/anton-a-tkachev/Arduino-True-DAC) is a amazing work that implements a true 12-bit DAC on Arduino using the R-2R resistor ladder. But it consumes up to 12 digital pins, which can be challenging if multiple channels are required.

### Tradeoff and Limitations

To create extra channels of 16-bit PWM using the available 4 channels of 8-bit PWM on Atmega328p, this project requires the use of Timer0 and Timer2 in the microcontroller. However, since Arduino uses Timer0 for built-in timing functions such as delay() and millis(), including this library in your project can cause malfunctions in these APIs. Fortunately, this issue can be resolved by overriding the default ISR of Timer0, which will allow you to use both the built-in timing functions and the additional PWM channels.

The frequency of the PWM signal is limited by the frequency of the oscillator used in the microcontroller. For example, with a 16MHz oscillator, the maximum PWM frequency that can be achieved is $16MHz / 2^{16} \approx 244.14Hz$ Hz. This limitation is important to consider when designing a system that requires high-frequency PWM signals, as a higher-frequency oscillator may be needed to achieve the desired frequency.

## Usage

### Installation

TBD

### Schematic

To combine the two outputs of the internal PWM channels, this project requires an external digital circuit. Unlike the analog circuit used in the dual PWM setting, combining PWM signals in the digital domain is relatively easier and more reliable. The circuit requires a positive-edge-triggered D flip-flop and two OR gates, as shown below. If you are interested in how these components work, the next chapter will provide more information.

### APIs

TBD

### Example

TBD

## Technical Detail

### How Does It Work?

TBD

### How Precise is It?

TBD

## Contribution

This project was created by Yong-Hsiang Hu (@ifTNT) and is licensed under the MIT license. If you have any thoughts or feedback on this project, you are welcome to leave a comment or create a pull request. Collaboration and community involvement can help improve and refine the project, making it even more useful for others who may be interested in using it for their own projects. So, if you have any ideas, suggestions, or contributions, don't hesitate to share them with the project creator and community.
