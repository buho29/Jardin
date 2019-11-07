#include "AnalogButtons.h"
#include <Arduino.h>

AnalogButton::AnalogButton(uint8_t id, uint16_t value, 
	uint16_t holdDuration, uint16_t holdInterval) {
	
	this->id = id;
	this->value = value;

	this->duration = holdDuration;
	this->interval = holdInterval;
}

void InputButtons::setup(uint8_t pin, uint8_t mode, uint16_t debounce, uint8_t margin)
{
	this->pin = pin;
	this->debounce = debounce;
	this->counter = 0;
	this->margin = margin;
	pinMode(pin, mode);
}

void InputButtons::addButton(AnalogButton &button) {
	if (buttonsCount < ANALOGBUTTONS_MAX_SIZE) {
    	buttons[buttonsCount++] = button;
  	}
}

void InputButtons::addEvent(EventClick * callback)
{
	if (listenerCount < ANALOGBUTTONS_MAX_EVENT) {
		listeners[listenerCount++] = callback;
	}
}

void InputButtons::dispatchEvent(AnalogButton * button)
{
	for (size_t i = 0; i < listenerCount; i++)
	{
		listeners[i]->onClick(button->id, button->isHeldDown);
	}
}

void InputButtons::check() {
	// In case this function gets called very frequently avoid sampling the analog pin too often
	if (millis() - time > ANALOGBUTTONS_SAMPLING_INTERVAL) {
		time = millis();
		uint16_t reading = analogRead(pin);
		for (uint8_t i = 0; i < buttonsCount; i++) {
			if ((int16_t)reading >= (int16_t)buttons[i].value - margin && reading <= buttons[i].value + margin) {
				
				if (lastButtonPressed != &buttons[i]) {

					buttons[i].isHeldDown = false;

					if (debounceButton != &buttons[i]) {
                    	counter = 0;
                    	debounceButton = &buttons[i];
                  	}
					if (++counter >= debounce) {
						// button properly debounced
						lastButtonPressed = &buttons[i];
						previousMillis = millis();
					}
				}
				else {
					if (!buttons[i].isHeldDown && ((millis() - previousMillis) > buttons[i].duration)) {
						// button has been hold down long enough
						buttons[i].isHeldDown = true;
						dispatchEvent(&buttons[i]);
						previousMillis = millis();
					}
					else if (buttons[i].isHeldDown && ((millis() - previousMillis) > buttons[i].interval)) {
						// button was already held, it's time to fire again
						dispatchEvent(&buttons[i]);
						previousMillis = millis();
					}
				}
				// The first matching button is the only one that gets triggered
				return;
			}
		}
		// If execution reaches this point then no button has been pressed during this check
		if (lastButtonPressed != 0x00 && !lastButtonPressed->isHeldDown) {
			dispatchEvent(lastButtonPressed);
		}
		debounceButton = lastButtonPressed = 0x00;
	}
}

InputButtons inputButtons;
