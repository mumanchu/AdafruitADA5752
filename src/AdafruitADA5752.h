#pragma once

/////////////////////////////////////////////////////////////////////
// Adafruit ADA5752 I2C Quad Rotary Encoder QT Module
// with 4 x Neopixel RGB LEDs
// Copyright (C) mumanchu, muman.ch, 2026.07.09
// All Rights Reversed
/*
Instead of using the full Adafruit Seesaw* package, this code 
supports only the ADA5752 module with 4 x rotary encoders and
4 x RGB led. The code has been optimized for only this module.

https://www.adafruit.com/product/5752
https://learn.adafruit.com/adafruit-i2c-quad-rotary-encoder-breakout/overview

Poll the encoder with hasInterrupt(), then read the position and
button states. It does not use actual interrupts because you cannot 
call I2C methods from an interrupt handler. The interrupt state is 
not cleared until the position and button states are read. The red 
onboard LED turns on when the INT pin is active (low).

Where possible, the methods return 'false' on a communications
or other error, so the program won't continue to run blindly if 
there's a hardware or software problem. The Adafruit Seesaw* 
methods don't do that. Check the source code below for details.

* "Seesaw" is an Adafruit C++ software library for Adafruit modules 
that contain an onboard microcontroller with I2C communications. 
The onboard microcontroller acts as a gateway between the Seesaw 
I2C messages and the peripherals connected to the microcontroller.

ADAFRUIT SEESAW REFERENCES
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/reading-and-writing-data
https://github.com/adafruit/Adafruit_Seesaw/tree/master

ADAFRUIT SEESAW ROTARY ENCODER
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/encoder
Base register	0x11
Register	Name				Size		R/W
0x10		Interrupt Enable	1 byte		W
0x20		Interrupt Disable	1 byte		W
0x30		Position			4 bytes		R/W
0x40		Delta				4 bytes		R
Lower 4 bits are the encoder number

ADAFRUIT SEESAW NEOPIXEL
https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/neopixel
Base register	0x0E
Register	Name				Size
0x01		PIN					8 bits		Pin number PORTA 
0x02		SPEED				8 bits		0=400kHz, 1=800kHz (default) 
0x03		BUF_LENGTH			16 bits		No. of bytes for pixel array
0x04		BUF					32 bytes	1st 2 bytes are start address + data bytes 
0x05		SHOW				none		Causes output to update

ADA5752 SCHEMATIC
https://learn.adafruit.com/assets/122222

I2C ADDRESS
Set by A0 A1 A2 jumpers
0x49..0x50, default = 0x49
https://learn.adafruit.com/adafruit-i2c-quad-rotary-encoder-breakout/pinouts
*/

class AdafruitADA5752
{
	TwoWire* wire;
	byte i2cAdds;
	byte intPin;
	bool lastEncoderButtonState[4];
	long encoderStartPosition[4];			// to fix Adafruit FW bug
	static const byte encoderButtonPin[4];

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

protected:
	bool beginNeopixel();
	bool adaPinMode(byte pin, byte mode);
	bool adaSetGPIOInterrupt(byte encoder, bool enabled);
	bool adaDigitalRead(byte pin, bool* value);

	bool read(byte adds, byte reg, byte* buf, byte len, uint delayMicros = 0);
	bool read32(byte base, byte reg, ulong* data, uint delayMicros = 0);
	bool write(byte adds, byte reg, byte* buf, byte len);
	ulong swap32(const byte* buf);
};

// Pin numbers for encoder buttons
// https://learn.adafruit.com/adafruit-i2c-quad-rotary-encoder-breakout/pinouts
const byte AdafruitADA5752::encoderButtonPin[4] = { 12, 14, 17, 9 };


// i2cAddress   = 0x49 .. 0x50
// interruptPin = the INT output pin from the module,
//                poll this with hasInterrupt()
bool AdafruitADA5752::begin(TwoWire* twoWire, byte i2cAddress, 
	byte interruptPin /*=255*/)
{
	wire = twoWire;
	i2cAdds = i2cAddress;
	intPin = interruptPin;

	// the interrupt pin does not generate an interrupt,
	// it is polled by calling hasInterrupt()
	if (interruptPin != 255)
		pinMode(interruptPin, INPUT_PULLUP);

	ulong version;
	if (!read32(0x00, 0x02, &version))
		return false;
	if (version == 0 || version == 0xffffffff) {
		LOGERROR("ADA5752 not responding")
		return false;
	}

	// read the firmare build date
	// the Seesaw setEncoderPosition() bug was fixed on 2024.02.27
	// https://github.com/adafruit/Adafruit_seesawPeripheral/issues/25
	// if you FW version is older than that then you have this bug
	// the Seesaw FW on my module is from 2017.06.27
	// see setEncoderPosition() below for my workaround
	uint year = version & 0x7F;
	uint month = (version >> 7) & 0x0F;
	uint day = (version >> 11) & 0x1F;

	if ((version >> 16) != 5752) {
		LOGERROR("not an ADA5752")
		return false;
	}

	// configure neopixel
	if (!beginNeopixel())
		return false;

	// configure each encoder
	for (int enc = 0; enc < 4; ++enc) {
		if (!adaPinMode(encoderButtonPin[enc], INPUT_PULLUP))
			return false;
		if (interruptPin != 255) {
			if (!adaSetGPIOInterrupt(enc, true))
				return false;
			if (!enableEncoderInterrupt(enc))
				return false;
		}
		else {
			if (!adaSetGPIOInterrupt(enc, false))
				return false;
			if (!disableEncoderInterrupt(enc))
				return false;
		}
		lastEncoderButtonState[enc] = false;

		// start at position 0
		if (!setEncoderPosition(0, enc))
			return false;
	}
	return true;
}

// Poll for a rotary encoder event, encoder turned or push button pressed
bool AdafruitADA5752::hasInterrupt()
{
	if (intPin == 255) {
		LOGERROR("INT pin not enabled");
		return false;
	}
	// INT is active low
	return digitalRead(intPin) == 0;
}

// Write the led's color to the chip's neopixel buffer
// call showNeopixels() to transfer the buffer to the LEDs
// https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/neopixel
bool AdafruitADA5752::setNeopixelColor(byte encoder, ulong rgb)
{
	byte buf[5];
	// buffer address, MS byte first
	uint adds = encoder * 3;
	buf[0] = (byte)(adds >> 8);
	buf[1] = (byte)adds;;
	// GRB data
	byte* p = (byte*)&rgb;
	buf[2] = p[1];		// G
	buf[3] = p[2];		// R
	buf[4] = p[0];		// B
	return write(0x0E, 0x04, buf, 5);
}

// Transfer the chip's neopixel buffer to the LEDs
bool AdafruitADA5752::showNeopixels()
{
	return write(0x0E, 0x05, NULL, 0);
}

// This returns false on error, it does NOT return the button state!
// *pressed = true if the encoder button has been pressed
// *pressed = true only once, then the pushbutton must be released 
// and pressed before it will be true again.
bool AdafruitADA5752::encoderButtonPressed(bool* pressed, byte encoder)
{
	*pressed = false;
	if (encoder > 3)
		return false;
	bool currentState;
	if (!adaDigitalRead(encoderButtonPin[encoder], &currentState))
		return false;
	// state is active low
	currentState = !currentState;
	// pressed
	if (currentState && !lastEncoderButtonState[encoder])
		*pressed = true;
	lastEncoderButtonState[encoder] = currentState;
	return true;
}

bool AdafruitADA5752::getEncoderPosition(long* position, byte encoder)
{
	ulong data;
	if (!read32(0x11, 0x30 + encoder, &data)) {
		*position = 0;
		return false;
	}
	// Seesaw FW bug, subtract initial position
	*position = (long)data - encoderStartPosition[encoder];
	return true;
}

// THIS DOES NOT WORK on the ADA5752, it's a documented Adafruit bug
// The Seesaw setEncoderPosition() bug was fixed on 2024.02.27,
// https://github.com/adafruit/Adafruit_seesawPeripheral/issues/25
// https://forums.adafruit.com/viewtopic.php?t=204380
// the Seesaw FW on my module is from 2017.06.27
bool AdafruitADA5752::setEncoderPosition(long newPosition, byte encoder)
{
	//this does not work on the quad encoder board
	//long posr = swap32((byte*)&pos);
	//return write(0x11, 0x30 + encoder, (byte*)&posr, 4);

	// workaround, save the initial position to subtract in getEncoderPosition()
	encoderStartPosition[encoder] = 0;
	long currentPosition;
	if (!getEncoderPosition(&currentPosition, encoder))
		return false;
	encoderStartPosition[encoder] = currentPosition - newPosition;
	return true;
}

// Returns the change since the last read and resets it to 0
bool AdafruitADA5752::getEncoderDelta(long* delta, byte encoder)
{
	ulong data;
	if (!read32(0x11, 0x40 + encoder, &data)) {
		*delta = 0;
		return false;
	}
	*delta = (long)data;
	return true;
}

bool AdafruitADA5752::enableEncoderInterrupt(byte encoder)
{
	byte b = 0x01;
	return write(0x11, 0x10 + encoder, &b, 1);
}

bool AdafruitADA5752::disableEncoderInterrupt(byte encoder)
{
	byte b = 0x01;
	return write(0x11, 0x20 + encoder, &b, 1);
}


// Internal Methods

// The ADA5752 has four RGB LEDs
bool AdafruitADA5752::beginNeopixel()
{
	// set neopixel pin 18
	byte b = 18;
	if (!write(0x0E, 0x01, &b, 1))
		return false;
	// set buffer length to 12 bytes (4 x 24-bit GRB)
	byte buf[2] = { 0, 3 * 4 };
	if (!write(0x0E, 0x03, buf, 2))
		return false;
	// (speed is already 800KHz by default)
	return true;
}

//https://learn.adafruit.com/adafruit-seesaw-atsamd09-breakout/gpio
//https://github.com/adafruit/Adafruit_Seesaw/blob/985b41efae3d9a8cba12a7b4d9ff0d226f9e0759/Adafruit_seesaw.cpp#L206

bool AdafruitADA5752::adaPinMode(byte pin, byte mode)
{
	ulong pinMask = 1L << pin;
	ulong pinMaskr = swap32((byte*)&pinMask);
	byte* p = (byte*)&pinMaskr;

	switch (mode) {
	// we only need this one, for now
	case INPUT_PULLUP:
		// 0x03=DIRCLR, 0x0B=PULLENSET, 0x05=SET 
		return write(0x01, 0x03, p, 4) &&
			write(0x01, 0x0B, p, 4) &&
			write(0x01, 0x05, p, 4);
	/*these are not used
	case OUTPUT:
		// 0x02=DIRSET
		return write(0x01, 0x02, p, 4);
	case INPUT:
		// 0x03=DIRCLR
		return write(0x01, 0x03, p, 4);
	case INPUT_PULLDOWN:
		// 0x03=DIRCLR, 0x0B=PULLENSET, 0x06=CLR 
		return write(0x01, 0x03, p, 4) &&
			write(0x01, 0x0B, p, 4) &&
			write(0x01, 0x06, p, 4);
	*/
	default:
		LOGERROR("mode not supported");
	}
	return false;
}

bool AdafruitADA5752::adaSetGPIOInterrupt(byte encoder, bool enabled)
{
	if (encoder > 3)
		return false;
	ulong pinMask = 1ul << encoderButtonPin[encoder];
	ulong pinMaskr = swap32((byte*)&pinMask);
	// 0x08=INTENSET, 0x09=INTENCLR
	return write(0x01, enabled ? 0x08 : 0x09, (byte*)&pinMaskr, 4);
}

bool AdafruitADA5752::adaDigitalRead(byte pin, bool* value)
{
	ulong data;
	if (!read32(0x01, 0x04, &data)) {
		*value = false;
		return false;
	}
	*value = (data & (1ul << pin)) ? true : false;
	return true;
}

// Read bytes
bool AdafruitADA5752::read(byte base, byte reg, byte* buf, byte length, 
	uint delayMicros/*=0*/)
{
	memset(buf, 0, length);		// returns 0s if it fails

	wire->beginTransmission(i2cAdds);
	wire->write(&base, 1);
	wire->write(&reg, 1);
	if (wire->endTransmission() != 0) {
		LOGERROR("read endTransmission() failed");
		return false;
	}
	// the delay is only needed for some read such as reading ADC data
	if (delayMicros != 0)
		delayMicroseconds(delayMicros);
	wire->requestFrom(i2cAdds, (size_t)length);
	if (wire->readBytes(buf, length) != length) {
		LOGERROR("readBytes() failed");
		return false;
	}
	return true;
}

// Read a 32-bit value, the value is converted from big to little startian
bool AdafruitADA5752::read32(byte base, byte reg, ulong* data, 
	uint delayMicros/*=0*/)
{
	byte buf[4];
	if (!read(base, reg, buf, 4, delayMicros)) {
		*data = 0;
		return false;
	}
	*data = swap32(buf);
	return true;
}

// Write bytes
bool AdafruitADA5752::write(byte adds, byte reg, byte* buf, byte len)
{
	wire->beginTransmission(i2cAdds);
	wire->write(&adds, 1);
	wire->write(&reg, 1);
	if (buf != NULL && len != 0)
		wire->write(buf, len);
	if (wire->endTransmission() != 0) {
		LOGERROR("write endTransmission() failed");
		return false;
	}
	return true;
}

// Reverse the 4 bytes of a 32-bit value
// (little to big-endian conversion, or is it big to little-startian?)
// (or Motorola to Intel format, if you're under 65)
ulong AdafruitADA5752::swap32(const byte* buf)
{
	byte buf2[4];
	buf2[3] = buf[0];
	buf2[2] = buf[1];
	buf2[1] = buf[2];
	buf2[0] = buf[3];
	return *(ulong*)buf2;

	//(the optimiser seems to prefer the code above)
	//return ((ulong)buf[0] << 24) + ((ulong)buf[1] << 16) + ((ulong)buf[2] << 8) + buf[3];
}

