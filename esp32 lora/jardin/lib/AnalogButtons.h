#ifndef __ANALOGBUTTONS_H
#define __ANALOGBUTTONS_H

/*
 AnalogButtons

 In order to reduce the number of pins used by some projects, sketches can use
 this library to wire multiple buttons to one single analog pin.
 You can register a call-back function which gets called when a button is
 pressed or held down for the defined number of seconds.
 Includes a software key de-bouncing simple algorithm which can be tweaked and
 is based on the max sampling frequency of 50Hz (one sample every 120ms)
 
 Minimum hold duration (time that must elapse before a button is considered
 being held) and hold interval (time that must elapse between each activation
 of the hold function) can both be configured.

 By default max number of buttons per pin is limited to 8 for memory
 consumption reasons, but it can be controlled defining the
 ANALOGBUTTONS_MAX_SIZE macro _before_ including this library.

 This work is largely inspired by the AnalogButtons library available in the
 Arduino Playground library collection.

 */
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define ANALOGBUTTONS_MAX_SIZE 8
#define ANALOGBUTTONS_MAX_EVENT 10
#define ANALOGBUTTONS_SAMPLING_INTERVAL 20

//clase virtual para registrase a eventos en InputButtons
class EventClick
{
public:
	virtual void onClick(uint8_t id, bool isHeldDown) = 0;
};

// clase q representa un pulsador
class AnalogButton {
	friend class InputButtons;
public:
	AnalogButton() {};
	AnalogButton(uint8_t id, uint16_t value, 
		uint16_t holdDuration = 500, uint16_t holdInterval = 250);

private:
	uint8_t id;
	uint16_t value;
	uint32_t duration;
	uint16_t interval;
	bool isHeldDown;
};
//clase a la q nos registramos 
class InputButtons {
private:
	uint32_t previousMillis;
	uint16_t debounce;
	uint32_t time;
	uint8_t counter;
	uint8_t margin;

	// AnalogPin
	uint8_t pin;

	uint8_t buttonsCount = 0;
	AnalogButton buttons[ANALOGBUTTONS_MAX_SIZE];

	uint8_t listenerCount = 0;
	EventClick * listeners[10];

	void dispatchEvent(AnalogButton * button);

	// last button pressed
	AnalogButton* lastButtonPressed;

	AnalogButton* debounceButton;

public:
	InputButtons(){}

	void setup(uint8_t pin, uint8_t mode = INPUT, uint16_t debounce = 3, uint8_t margin = 50);

	void addButton(AnalogButton &button);

	void addEvent(EventClick * callback);
	
	void check();
};

extern InputButtons inputButtons;
#endif

