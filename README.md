# Arduino 16bits DAC/PWM

Provide two extra 16-bits PWM channels to Arduino UNO/Nano.

## Introduction

The main focus of this project is to create extra channels of 16-bit PWM in the MCU using the available 4 channels of 8-bit PWM on ATmega328P. This is important because high-resolution PWM is necessary for creating a software DAC. While the ATmega328P chip can provide 16 bits of PWM, it only has two channels, so using an external IC such as PCA9685 or MCP4725 becomes necessary when more channels are required, which comes at an additional cost. However, by combining the available 4 channels of 8-bit PWM, we can create two extra channels of 16-bit PWM without the need for an external IC. This can save costs and simplify the design process.

### Related Work

The [dual PWM circuits](http://www.openmusiclabs.com/learning/digital/pwm-dac/dual-pwm-circuits/index.html) is a commonly used method for combining multiple low-resolution DACs to create a high-resolution one. It uses a voltage divider circuit to attenuate the low half voltage and add them with the high half voltage to create higher precision. However, it has a precision problem. Specifically, the 0.25 LSB of error from the high half of the DAC will contribute 64 LSBs of error in the output, which is not acceptable for high-precision applications.

On the other hand, [Arduino-True-DAC](https://github.com/anton-a-tkachev/Arduino-True-DAC) is a amazing work that implements a true 12-bit DAC on Arduino using the R-2R resistor ladder. But it consumes up to 12 digital pins, which can be challenging if multiple channels are required.

### Tradeoff and Limitations

To create extra channels of 16-bit PWM using the available 4 channels of 8-bit PWM on ATmega328P, this project requires the use of Timer0 and Timer2 in the microcontroller. However, since Arduino uses Timer0 for built-in timing functions such as delay() and millis(), including this library in your project can cause malfunctions in these APIs. Fortunately, this issue can be resolved by overriding the default ISR of Timer0, which will allow you to use both the built-in timing functions and the additional PWM channels.

The frequency of the PWM signal is limited by the frequency of the oscillator used in the microcontroller. For example, with a 16MHz oscillator, the maximum PWM frequency that can be achieved is $16MHz / 2^{16} \approx 244.14Hz$ Hz. This limitation is important to consider when designing a system that requires high-frequency PWM signals, as a higher-frequency oscillator may be needed to achieve the desired frequency.

## Usage

### Installation

You can download the released .zip file from github. Then import it into your Arduino. Fore instruction, please refer to the [official tutorial](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries).

### External Circuit

If you require less than two channels of PWM signals, there's no need for an external circuit. However, if you need more channels, an external digital circuit will be necessary to combine the two outputs of the internal PWM channels. Unlike the analog circuit used in the dual PWM setting, combining PWM signals in the digital domain is relatively easier and more reliable. The circuit requires a positive-edge-triggered D flip-flop and two OR gates, as shown below. If you are interested in how these components work, the next chapter will provide more information.

![Schematic](https://raw.githubusercontent.com/ifTNT/arduino-16bit-dac-pwm/main/image/schmatic.svg)
### APIs

- `void init(enum Mode mode)`: This function will initialize all resources needed by this library. And it should be placed inside the `setup()` function. The library is designed to operate in three different modes, each of which is described in detail below.
  
  |Mode|Description|
  |-|-|
  |Pwm16::MODE_2CH|Provide two 16bits PWM channels. This mode won't affect the built-in timing function.|
  |Pwm16::MODE_4CH|This mode of operation will enable the library to supply four channels of 16-bits PWM signals. However, it's important to note that this mode will utilize all the timer resources on the ATmega328P microcontroller, which may lead to unexpected behavior of the built-in timing functions. Therefore, it's essential to exercise caution while using this mode of operation.|
  |Pwm16::MODE_4CH_REVERSE|This mode also provide four channels of 16-bits PWM signals. However, in this mode, the HighHalf and LowHalf signals are swapped. For further details and information, please refer to the next section.|

- `void analogWrite16(enum Channel ch, uint16_t duty)`: This function serves as an alternative to the Arduino's `analogWrite()` function. It enables the library to output a specified duty cycle to a specified channel with a resolution of 16 bits. The available channels will depend on the chosen operation mode, as illustrated in the table below.

  |Channel|Arduino Pin|Enabled in MODE_2CH|Enabled in MODE_4CH|
  |-|-|-|-|
  |Pwm16::CH_0A|9|✔|✔|
  |Pwm16::CH_0B|10|✔|✔|
  |Pwm16::CH_1A|6 & 11||✔|
  |Pwm16::CH_1B|3 & 5||✔|

  Furthermore, you may have noticed that the last two channels require two pins each, referred to as HighHalf and LowHalf in previous sections. The order of these signals can be swapped by changing the operation mode. The relationship between the channels and their corresponding signals is listed in the table below.

  |Mode|HighHalf Signal Pin|LowHalf Signal Pin|
  |-|-|-|
  |Pwm16::MODE_4CH|3 (CH_1B) &<br> 11 (CH_1A)|5 (CH_1B) &<br> 6 (CH_1A)|
  |Pwm16::MODE_4CH_REVERSE|5 (CH_1B) &<br> 6 (CH_1A)|3 (CH_1B) &<br> 11 (CH_1A)|

### Example

TBD

## Technical Detail

### How Does It Work?

The ATmega328P microcontroller has three timers, namely Timer0, Timer1, and Timer2, each of which has the ability to provide two channels of PWM output. However, except for the 16-bit Timer1, the remaining timers have a resolution of only 8 bits. Additionally, each timer has the ability to set its own prescaler, which can be used to adjust the period of the clock cycle.

By using this feature, we can set one timer to run 256 times faster than another. We refer to the faster timer as the "LowHalf" and the slower timer as the "HighHalf". With this configuration, the 256 cycles of the LowHalf timer can fit exactly into one cycle of the HighHalf timer, providing an additional 8 bits of duty-cycle resolution to the PWM output. This technique allows us to achieve higher-resolution PWM signals using the available resources of the microcontroller.

The next challenge is how to accurately combine these two PWM signals. Learning from the issues with the dual PWM circuit discussed earlier, we decide not to use any analog circuit to accomplish this. Since the PWM signal is synchronized at the rising edge, the next step is to extend the falling edge of the HighHalf signal to the falling edge, of the immediate successor LowHalf pulse. Note that we invert the output of LowHalf in order to trigger the flip-flop properly.

To achieve this, we need to delay the output of the HighHalf signal by one clock cycle, which is the duty-cycle of the LowHalf signal. This can be done using the D flip-flop, which will store the previous value of the HighHalf signal and delay it by one clock cycle. We can then use an OR gates to combine the delayed HighHalf signal with the LowHalf signal. This approach ensures that the two PWM signals are combined accurately, without introducing any significant errors or distortions.

The uncovered OR gate is used to balance the latency difference between the signal from the flip-flop and the original HighHalf signal. Since the flip-flip has propagation delay and transition delay, which will lead extra error in the final duty-cycle. By balancing the latency difference, the error introduced by the flip-flop can be minimized, resulting in a more precise PWM output.

The animation displayed depicts two PWM outputs being observed under an oscilloscope. The yellow line represents the HighHalf PWM signal while the green line corresponds to the inverted LowHalf signal. It is evident from the animation that the rising edge of both signals is synchronized. Furthermore, if the duty cycle is counted to 254, the LowHalf signal will continue to carry to the HighHalf signal. This is important because, in the ATmega328P system, a duty cycle of 255 corresponds to 5V output. Therefore, in order to prevent the direct output of 5V, it is necessary to avoid setting the duty cycle to 255 in the LowHalf signal.

![Arduino 8bits PWM input](https://github.com/ifTNT/arduino-16bit-dac-pwm/raw/main/image/arduino-8bits-pwm-input.gif)

Once the HighHalf signal passes through the flip-flop, there will be a delay until the next rising edge of the LowHalf signal. This delay can be seen clearly in the accompanying image. The yellow line in the image represents the original HighHalf signal, while the green line corresponds to the delayed HighHalf signal. As can be observed, the delayed signal is shifted to the right of the original signal, indicating the presence of a phase shift between the two.

![Phase shift](https://github.com/ifTNT/arduino-16bit-dac-pwm/raw/main/image/phase-shift.gif)

### How Precise is It?

TBD

## Contribution

This project was created by Yong-Hsiang Hu (@ifTNT) and is licensed under the MIT license. If you have any thoughts or feedback on this project, you are welcome to leave a comment or create a pull request. Collaboration and community involvement can help improve and refine the project, making it even more useful for others who may be interested in using it for their own projects. So, if you have any ideas, suggestions, or contributions, don't hesitate to share them with the project creator and community.
