# Adafruit ADA5752 I2C Quad Rotary Encoder + 4 x Neopixel LEDs

The ADA5752 is an I2C module with four rotary encoders and four RGB LEDs. It has an onboard microcontroller (ATtiny8X7) to do most of the work. This removes all the complexity of interrupt handling for the encoder signals and the nanosecond timing for the RGB LEDs communications. It does not have the rotary encoders fitted, so you need to buy those separately and solder them on. The mounting holes were too small for the encoders that I bought, so I had to enlarge the holes by drilling them out.

<img src="https://github.com/mumanchu/mumanchu/blob/main/assets/ada5752/ada5752.jpg" alt="Picture of ADA5752 board" width="600">

This board is part of Adafruit's "Seesaw" framework which comprises a set of intelligent boards with I2C interfaces and a comprehensive Adafruit Seesaw library for programming them. It has Stemma QT connectors (for chaining I2C devices), but you don't need to use those because it also has a 2.54mm pin header. All the Seesaw boards will run on 3.3V or 5V, which is nice.

> [!WARNING]
> The 4 x RGB LEDs will need up to 240mA at full brightness. Make sure your microcontroller board can deliver this. If not, use a separate power supply for the ADA5752.

I wanted to use this board _without_ the Seesaw library, so this small-footprint AdafruitADA5752 library `Adafruit5752.h` was developed. It handles the rotary encoders and RGB LEDs in the same `AdafruitADA5752` class.

Poll the encoder with `hasInterrupt()` from `loop()` to check for events, then read the positions and button states. It does not use and actual GPIO interrupt because you cannot call the I2C methods from an interrupt handler. The interrupt state is not cleared until the position and button states are read. The red LED on the board turns on when the INT pin is active (low). See the example sketch.

This code was developed and tested on an STM32. It may need modifications for other MCUs, further tests will be done later.

## The RGB LEDs are located UNDERNEATH the Rotary Encoders!

The board is very nice, BUT for some reason they mounted the RGB LEDs **underneath** the rotary encoders, so they cannot be seen! See the pictures of the board above, the LEDs are covered when the encoders are fitted. This would only work if the encoders had transparent shafts and transparent buttons. I have not been able to find any encoders with transparent shafts. So I removed the onboard RGB LEDs and made a piggyback board for them, so they are mounted below the encoders. 

<img src="https://github.com/mumanchu/mumanchu/blob/main/assets/ada5752/ada5752piggyback2.jpg" alt="Picture of ADA5752 piggyback board" width="600">

The piggyback board needs VCC and GND, plus the DIN data signal from the ADA5752 board. With the LEDs removed, the DIN signal can be taked from the DIN pin of LED 0. You can see this on the schematic in Useful Links.

<img src="https://github.com/mumanchu/mumanchu/blob/main/assets/ada5752/ada5752dinpin.jpg" alt="Picture of ADA5752 DIN pin" width="600">

> [!WARNING]
> It is almost impossible to unsolder the existing RGB LEDs without damaging them. So you will need four new WS2813B LEDs. An LED strip can also be used, maybe with more than 4 LEDs if you modify the Neopixel buffer length in `AdafruitADA5752::beginNeopixel()` (but I have not tried this). If using more than 4 LEDs you'll probably need a separate power supply.
  
## setEncoderPosition() FW Bug!

The Seesaw firmware on my quad encoder board has a firmware bug. The I2C message to set the encoder position does not work. This bug was fixed on 2024.02.27, but my board's FW version is from 2017.06.27. The firmware release date can be read from the module, see the code in `AdafruitADA5752.begin()`. This library contains a workaround which corrects this bug, see `setEncoderPosition()` and `getEncoderPosition()`.

https://github.com/adafruit/Adafruit_seesawPeripheral/issues/25


## Class Reference

For full details, read the commented source code in `Adafruit5752.h`. In the unlikely event that you need to access the library from more than one `.cpp` file, split it into two files, one with the class definition (.h) and one with the code (.cpp).

```cpp
class AdafruitADA5752
{
public:
	// Convert 3 separate 8-bit RGB values to a single 24-bit RGB value
	#define RGB(r, g, b) (ulong)((ulong)(byte)(r) << 16) + ((ulong)(byte)(g) << 8) + (byte)(b))

	bool begin(TwoWire* twoWire, byte i2cAddress, byte interruptPin = 255);
	bool hasInterrupt();
	bool setNeopixelColor(byte encoder, ulong rgb);
	bool showNeopixels();
	bool encoderButtonPressed(bool* pressed, byte encoder = 0);
	bool getEncoderPosition(long* position, byte encoder = 0);
	bool setEncoderPosition(long newPosition, byte encoder = 0);
	bool getEncoderDelta(long* delta, byte encoder = 0);
	bool enableEncoderInterrupt(byte encoder = 0);
	bool disableEncoderInterrupt(byte encoder = 0);
};
```

## Useful Links

**Quad Rotary Encoder Wiki** \
https://learn.adafruit.com/adafruit-i2c-quad-rotary-encoder-breakout

**Adafruit Seesaw Reference** \
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/reading-and-writing-data

**ADA5752 Schematic**
https://learn.adafruit.com/assets/122222

<br/>

## Revision History

| Date       | Version  | Details |
|:---------- |:---------|:----------- |
| 2026.07.10 | 0.0.0	| Preliminary |

<br/>

## Joke of the Week

Matt's Tip #31: The road to hell is paved with good inventions.

