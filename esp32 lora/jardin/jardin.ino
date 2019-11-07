/*
 Name:		riego_automatico.ino
 Created:	05/08/2018 0:25:12
 Author:	pp
*/

#include "app.h"

void setup() {

	Serial.begin(115200);
	delay(10);
	controler.begin(); 
	model.begin();
}

void loop() {

	uint32_t c = millis();

	model.update();
	controler.update();
	
	if (millis() - c > 80)
		Serial.printf("loop %d ms\n", millis() - c);
}