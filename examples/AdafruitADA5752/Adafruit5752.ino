/////////////////////////////////////////////////////////////////////
// Example Sketch
// Adafruit ADA5752 I2C Quad Rotary Encoder QT Module
// with 4 x Neopixel RGB LEDs
// Copyright (C) muman.ch + mumanchu, 2027.07.10
// https://github.com/mumanchu/AdafruitADA5752

#include <Wire.h>

#if 1
#define LOGERROR(s) { Serial.println(s); Serial.flush(); }
#else
#define LOGERROR(s)
#endif

#include "AdafruitADA5752.h"
AdafruitADA5752 ada5752;


void setup()
{
	// use different pins for RX/TX
	//Serial.setTx(PC_10);
	//Serial.setRx(PC_11);

	Serial.begin(115200);
	delay(3000);
	Serial.println("\n\rStarted...\n\r");
	Serial.flush();

	pinMode(LED_BUILTIN, OUTPUT);

	Wire.begin();
	Wire.setClock(800000);
	Wire.setTimeout(100);

	if (!ada5752.begin(&Wire, 0x49, 8)) {
		LOGERROR("ada5752.begin() failed");
		while (1)
			yield();
	}
}

void loop()
{
	// scheduler
	ulong t = millis();

	// poll the encoder every 50ms
	static ulong t1 = 0;
	if (t - t1 >= 50) {
		t1 = t;

		// toggle the led so we know it's running
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

		// if there's been an encoder event
		if (ada5752.hasInterrupt()) {
			static long lastPos[4] = { 0, 0, 0, 0 };

			for (int enc = 0; enc < 4; ++enc) {
				bool pressed;
				ada5752.encoderButtonPressed(&pressed, enc);
				long pos;
				ada5752.getEncoderPosition(&pos, enc);
				if (pos != lastPos[enc] || pressed) {
					char buf[32];
					sprintf(buf, "%d %6ld %s", enc, pos, pressed ? "pressed" : "");
					Serial.println(buf);
				}
				lastPos[enc] = pos;
			}
		}
	}

	// change the neopixel colors every 250ms, R->G->B
	static ulong t2 = 0;
	if (t - t2 >= 250) {
		t2 = t;
		static ulong rgb = 0x000f0000;

		for (int enc = 0; enc < 4; ++enc) {
			ada5752.setNeopixelColor(enc, rgb);
			rgb >>= 8;
			if (rgb == 0)
				rgb = 0x000f0000;
		}
		ada5752.showNeopixels();
	}
}
