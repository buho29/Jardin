// 
// 
// 

#include "model.h"

void Model::begin()
{
	using namespace std::placeholders;

	status = Status::starting;
	dispachEvent(EventType::status);

	//iniamos los pins
	for (uint8_t i = 0; i < countPins; i++)
	{
		pinMode(pins[i], OUTPUT);
		digitalWrite(pins[i], OFF);

		Serial.printf("pin %d\n", pins[i]);
	}

	readEprom();

	//strcpy(config.password, "EGYDRNA6H4Q");
	//strcpy(config.ssid, "Movistar_1664");

	//strcpy(config.password, "gzfq4137");
	//strcpy(config.ssid, "AndroidAP");

	//sensor 
	if (bme.begin(0x76)) {
		bme.setSampling(Adafruit_BME280::MODE_FORCED,
		Adafruit_BME280::SAMPLING_X1, // temperature
		Adafruit_BME280::SAMPLING_X1, // pressure
		Adafruit_BME280::SAMPLING_X1, // humidity
		Adafruit_BME280::FILTER_OFF   );

		updateSensor();
		initLogSensor();

		// cada hora guardamos los valores de los sensores
		for (uint8_t h = 0; h < MODEL_SIZE_LOG; h++)
		{
			Task * t =tasker.setInterval(
				std::bind(&Model::saveLoger, this, _1),h,0);
		}
		
	}
	else {
		Serial.println("bme280 Error");
		strcpy(msgError, getStr(Str::errorSensor));
		dispachEvent(EventType::error);
	}

	//una vez al dia actualizaremos los datos
	tasker.setInterval(
		std::bind(&Model::onUpdateData, this, _1), 18,30);
	
	//activamos softAP
	enableSoftAP();

	//intentamos conectar al wifi y descargar la hora y la meteo
	connectWifi();
	
	//calculamos zonas segun las alarmas
	bool emptyZones = true;
	for (int16_t zone = -1; hasZone(++zone); )
	{
		updateZone(zone);
		emptyZones = false;
	}

	//insertamos datos en el primer run
	if (emptyZones) {
		loadDefaultConfig();
		writeEprom();
		Serial.println("loaded default alarm");
	}
	
	//imprime las zonas y sus alarmas
	//printZones();

	//iniciamos las tareas
	updateTasks();

	status = Status::started;
	dispachEvent(EventType::status);

	readModes();
}

void Model::printZones()
{

	//imprimir todos los datos
	for (uint8_t z = 0; z < MODEL_MAX_ZONES; z++)
	{
		Serial.printf("\nname :%s\n", config.zoneNames[z]);

		ZoneData * zd = getZone(z);
		char buff[9], result[18];
		Tasker::tickTimeToChar(buff, zd->start);
		sprintf(result, "%s %d min\n", buff, (int)(zd->duration / 60));

		Serial.printf("id: %d start: %s\n", z, result);

		printAlarms(z);
	}
}

void Model::printTask(Task *t)
{
	char buff[9];
	Tasker::tickTimeToChar(buff, t->start);
	uint16_t d = Tasker::getDuration(t->start, t->stop);

	Serial.printf("--- task %s %d min\n", buff, (int)(d / 60));

}

void Model::printAlarms(uint8_t z)
{
	char buff[9];
	for (uint8_t i = 0; i < MODEL_MAX_ALARMS; i++)
	{
		Alarm * a = &config.alarms[z][i];

		Tasker::tickTimeToChar(buff, a->init);
		uint16_t d = Tasker::getDuration(a->init, a->end);

		Serial.printf("--- Alarm %d %d %s %d min\n", i, a->pin, buff, (int)(d / 60));
	}
}

void Model::loadDefaultConfig()
{
	Serial.println("loadDefault");
	//borramos todas las zonas y alarmas
	for (uint8_t zone = 0; zone < MODEL_MAX_ZONES; zone++)
	{
		strcpy(config.zoneNames[zone], "");
		for (uint8_t i = 0; i < MODEL_MAX_ALARMS; i++)
		{
			config.alarms[zone][i].setNull();
		}
	}

	uint8_t _zone1 = addZone("Cesped D");
	uint8_t _zone2 = addZone("Cesped N");
	uint8_t _zone3 = addZone("Huerta");

	uint8_t duration = 60;

	/*	rele 1 27	rele 2 14	rele 3 21	rele 4 17
	*/

	addAlarm(_zone1, 27, duration, 7, 0 ,0);
	addAlarm(_zone1, 14, duration, 7, 0 ,duration);
	addAlarm(_zone1, 21, duration, 7, 0 ,duration*2);

	addAlarm(_zone3, 17, duration, 7, 0 ,duration*3);

	addAlarm(_zone2, 27, duration, 21, 0 ,0);
	addAlarm(_zone2, 14, duration, 21, 0 ,duration);
	addAlarm(_zone2, 21, duration, 21, 0 ,duration*2);

	// iniciar desactivando modos de riego
	for (uint8_t i = 0; i < 20; i++)
	{
		config.modes[i] = false;
	}

}

void Model::updateTasks()
{
	for (int16_t zone = -1; hasZone(++zone); )
	{
		updateTasks(zone);
	}
}

//llamado por tasker x tiempo (alarmas)
void Model::onWater(Task * t)
{
	// si se abre los grifos 
	if (isManualWater) {
		Serial.println("Esta ocupado");
		// nos salimos
		return;
	}
	
	//reseteamos uno que fue cancelado 
	// y salimos
	if (!t->enabled){
		t->enabled = true;
		return;
	}
	//buscamos la correspondencia con la alarma
	bool found;
	for (int16_t zone = -1; hasZone(++zone); )
	{
		for (int16_t i = -1; hasAlarm(zone, ++i); )
		{
			Task * task = tasks[zone][i];
			//bingo
			if (task == t ) 
			{
				currentZone = zone;
				currentAlarm = i;
				found = true;
				goto endloop;
			}
		}
	}
endloop:

	if (found) 
	{
		// si no se puede regar cancelamos
		if (t->runing && !isManualZoneWater && !canWatering()) {
			stopWaterZone();
			return;
		}

		Alarm * a = getAlarm(currentZone,currentAlarm);

		lastTimeZoneChanged = now();
		openTap(a->pin, t->runing);

		// si es la ultima alarma de la zona
		if (!t->runing && !hasAlarm(currentZone, currentAlarm + 1)) {
			// si fue activado el riego de la zona manualmente
			if (isManualZoneWater) {
				isManualZoneWater = false;
			}

			// reseteamos los tasks de la zona
			reloadTasks(currentZone);

			currentZone = -1;
			currentAlarm = -1;
			elapsedPausedTask = 0;

			lastTimeZoneChanged = now();
			status = Status::standby;
			dispachEvent(EventType::status);
		}

	}else
	Serial.printf("Meeec! onWater task not found:%d isWatering:%d zone:%d iindex:%d\n", found,t->runing,currentZone,currentAlarm);
}

bool Model::waterZone(uint8_t zone)
{
	if (hasZone(zone) && isWatering()) return false;

	Serial.printf("waterZone %d\n",zone);

	uint32_t current = Tasker::getTickNow();

	isManualZoneWater = true;

	for (int16_t i = -1; hasAlarm(zone, ++i);)
	{
		Alarm * alarm = getAlarm(zone, i);
		Task * t = tasks[zone][i];

		t->enabled = true;//por si fue cancelado
		// encolamos alarmas una tras otra
		t->start = current;
		
		t->stop = (current + alarm->end - alarm->init) % TASK_TICKS_24H;

		current = t->stop;
	}
	return true;
}

// detiene el riego de la zona en curso
bool Model::stopWaterZone()
{
	Serial.println("stopWaterZone");

	if (currentZone < 0) return false;

	//cancelamos las siguientes tareas
	for (int16_t i = -1; hasAlarm(currentZone, ++i);)
	{
		Task * t = tasks[currentZone][i];
		t->enabled = false;
		t->runing = false;

		// cuando task a sido manipulado
		// lo reseteamos
		if (isManualZoneWater || isPaused()) {
			Alarm * alarm = getAlarm(currentZone, i);
			t->start = alarm->init;
			t->stop = alarm->end;
		}
	}

	isManualZoneWater = false;

	// cancelamos el q esta activo 
	Alarm * a = getAlarm(currentZone, currentAlarm);
	bool r = openTap(a->pin, false); // apaga el pin

	currentZone = -1;
	currentAlarm = -1;
	elapsedPausedTask = 0;
	pausedTime = 0;


	lastTimeZoneChanged = now();

	status = Status::standby;
	dispachEvent(EventType::status);
	return r;
}
// pausa el riego de la zona en curso
bool Model::pauseWaterZone()
{
	Serial.printf("\n--pauseWaterZone %d\n",currentZone);

	if (currentZone < 0) return false;

	Task * t = tasks[currentZone][currentAlarm];
	Alarm * a = getAlarm(currentZone,currentAlarm);

	bool r = false;
	bool iswate = isWatering();

	if (iswate && !isPaused()) {//pausamos


		// tiempo trancurrido de la tarea actual
		elapsedPausedTask = Tasker::getDuration(t->start, Tasker::getTickNow());
		pausedTime = Tasker::getTickNow();

		lastTimeZoneChanged = now();

		uint16_t da = Tasker::getDuration(a->init, a->end);
		uint16_t dt = Tasker::getDuration(t->start, t->stop);

		// cuando la duracion de la alarma es mas grande 
		// que la duracion de la tarea (porq fue pausado)
		if (da > dt) {
			//sumamos esa dife
			elapsedPausedTask += (da - dt);
		}

		//detenemos todas las alarmas de la zona
		for (int16_t i = -1; hasAlarm(currentZone, ++i);)
		{
			t = tasks[currentZone][i];
			Alarm * alarm = getAlarm(currentZone, i);
			t->enabled = false;
			t->runing = false;
			//t->start = alarm->init;
			//t->stop = alarm->end;
		}

		// cerramos el q esta activo 
		r = openTap(a->pin, false); // apaga el pin
		Serial.println("--pauseWaterZone pausado");
	}
	else if(!iswate){//reanudamos

		uint32_t current = Tasker::getTickNow();
		uint16_t elapsed = Tasker::getDuration( pausedTime, current);


		Task * t = tasks[currentZone][currentAlarm];
		Alarm * a = getAlarm(currentZone, currentAlarm);

		// 
		/*t->enabled = true;
		t->start = current;
		t->stop = (current + a->end - a->init - elapsedPausedTask) % TASK_TICKS_24H;*/

		char buff[9];
		Tasker::tickTimeToChar(buff, current);
		Serial.printf("current %s \n", buff);
		
		Tasker::tickTimeToChar(buff, pausedTime);
		Serial.printf("pausedTime %s \n", buff);


		for (int16_t i = currentAlarm-1; hasAlarm(currentZone, ++i);)
		{
			t = tasks[currentZone][i];

			t->enabled = true;//por si fue cancelado
			t->runing = false;
			//sumamos la pausa
			Serial.printf("alarm id %d elapsed %d\n", i,elapsed);
			printTask(t);
			t->start = (t->start + elapsed) % TASK_TICKS_24H;
			t->stop = (t->stop + elapsed) % TASK_TICKS_24H;

			printTask(t);
		}

		elapsedPausedTask = 0;
		pausedTime = 0;


		lastTimeZoneChanged = now();

		Serial.println("--pauseWaterZone reanudado");
		r = true;
	}

	Serial.println("--pauseWaterZone final");
	return r;
}

void Model::reloadTasks(uint8_t zone)
{
	Serial.println("reloadTasks");
	for (int16_t i = -1; hasAlarm(zone, ++i); )
	{
		Alarm * alarm = getAlarm(zone, i);
		Task * t = tasks[zone][i];

		t->start = alarm->init;
		t->stop = alarm->end;
		t->runing = false;
		t->enabled = true;
	}
}

void Model::onUpdateData(Task * t)
{
	Serial.printf("----------actualiza !\n");
	if (connectedWifi) {
		load();
	}
}

bool Model::openTap(uint8_t pin, bool val)
{
	int16_t id = getIndexPin(pin);

	// no existe nos salimos
	if (id < 0) return false;

	currentPin = pin;

	if (val) 
		digitalWrite(pin, ON);// Enciende el pin 
	else digitalWrite(pin, OFF);// apaga el pin
		
	isPinOn[id] = val;
	
	Serial.printf("opentap pin %d id:%d isOn %d water %d\n", pin, id, isPinOn[id], isWatering());
	status = Status::watering;
	dispachEvent(EventType::status);

	if (!isWatering()) {
		isManualWater = false;
		lastTimeZoneChanged = now();
		status = Status::standby;
		dispachEvent(EventType::status);
	}
	return true;
}

bool Model::switchTap(uint8_t pin)
{
	uint8_t id = getIndexPin(pin);

	Serial.printf("switchTap pin:%d id:%d ison:%d\n", pin,id, isPinOn[id]);
	
	isManualWater = true;
	return openTap(pin, !isPinOn[id]);

}

void Model::update()
{
	//static uint32_t lastFreeHeap = 0;

	uint32_t c = millis();
	tasker.check();

	static uint32_t delaySensors = 0;
	if (millis() - delaySensors > SENSOR_INTERVAL) {
		delaySensors = millis();
		updateSensor();
	}

	if (millis() - c > 50)
		Serial.printf("model.update %d ms\n", millis() - c);

	//c = millis();
	server.handleClient();
	//if (millis() - c > 1)
		//Serial.printf("server.handleClient %d ms\n", millis() - c);

	static uint32_t delayedRSSI = 0;
	if (WiFi.status() == WL_CONNECTED && millis() - delayedRSSI > 2000) {
		delayedRSSI = millis();
		rssi = WiFi.RSSI();
		if(esp_get_free_heap_size() < 180000)
			Serial.printf("\nram left %d\n",esp_get_free_heap_size());
	}
}

void Model::updateSensor()
{
	bme.takeForcedMeasurement();

	if (isnan(bme.readTemperature()) || isnan(bme.readPressure()) || 
		isnan(bme.readHumidity()) || bme.readHumidity() == 255) 
	{
		Serial.println("sensor isnan");
		delay(100);
		updateSensor();
	}

	currentSensor.temperature = bme.readTemperature();
	currentSensor.pressure = bme.readPressure() / 100.0F;
	currentSensor.humidity = bme.readHumidity();
	currentSensor.time = now();
	dispachEvent(EventType::sensor);
}
// iniciamos el logger con valores por defecto
void Model::initLogSensor()
{
	for (uint8_t i = 0; i < MODEL_SIZE_LOG; i++)
	{
		SensorData * s = getLog(i);
		// si esta vacio lo llenamos con el actual
		if (s->time == 0) {
			s->temperature = currentSensor.temperature;
			s->pressure = currentSensor.pressure;
			s->humidity = currentSensor.humidity;
			s->time = currentSensor.time;
		}

		//no deberia pasar
		if (isnan(s->temperature) || isnan(s->pressure) || 
			isnan(s->humidity) || s->humidity == 255) 
		{
			if (i > 0) {
				SensorData * sLast = getLog(i-1);
				s->temperature = sLast->temperature;
				s->pressure = sLast->pressure;
				s->humidity = sLast->humidity;
				s->time = sLast->time;
			}
			else {
				s->temperature = currentSensor.temperature;
				s->pressure = currentSensor.pressure;
				s->humidity = currentSensor.humidity;
				s->time = currentSensor.time;
			}
			Serial.println("mec sensor NaN");
		}
		Serial.printf("%f\t%f\t%d\t%d\n", s->temperature,  s->pressure, s->humidity,s->time);

	}
	dispachEvent(EventType::sensorLog);
}

void Model::resetLogSensor()
{
	for (uint8_t i = 0; i < MODEL_SIZE_LOG; i++)
	{
		SensorData * s = getLog(i);
		s->pressure = 0;
		s->temperature = 0;
		s->pressure = 0;
		s->humidity = 0;
		s->time = 0;
	}
}

void Model::saveLoger(Task * t)
{	
	SensorData * s, * s1;
	for (uint8_t i = 0; i < MODEL_SIZE_LOG - 1; i++)
	{
		s = getLog(i);
		s1 = getLog(i + 1);
		s->temperature = s1->temperature;
		s->pressure = s1->pressure;
		s->humidity = s1->humidity;
		s->time = s1->time;
	}
	//cogemos el ultimo
	s = getLog(MODEL_SIZE_LOG-1);
	s->temperature = currentSensor.temperature;
	s->pressure = currentSensor.pressure;
	s->humidity = currentSensor.humidity;
	s->time = currentSensor.time;

	writeEprom();

	dispachEvent(EventType::sensorLog);

	char buff[9];
	char buff1[9];
	Tasker::tickTimeToChar(buff, t->start);
	Tasker::tickTimeToChar(buff1, Tasker::getTickNow());

	Serial.printf(" saveLogger %d %s %s temp %.1f pression %.1f hum %d%%\n",t->id,buff1,buff, s->temperature, s->pressure, s->humidity);
}

void Model::load()
{

	loadLocalTime();
	loadForecast();
}

void Model::connectWifi()
{

	strcpy(msgStatus, getStr(Str::conWifi));
	strcat(msgStatus, config.ssid);

	dispachEvent(EventType::status);

	WiFi.begin(config.ssid, config.password);

	uint16_t timeout = 20000;
	uint32_t current_time = millis();
	uint32_t delay_time = millis();

	while (millis() - current_time < timeout) {

		//delay(500);

		bool delayed = millis() - delay_time > 500;

		if (delayed) {
			delay_time = millis();
		}

		if (delayed && WiFi.status() == WL_CONNECTED) {

			connectedWifi = true;

			Serial.println("WiFi connected");
			//if (MDNS.begin("jardin")) {
			//	Serial.println("MDNS responder started");
			//}
			strcpy(msgStatus, getStr(Str::conWifi1));
			strcat(msgStatus, config.ssid);

			IPAddress ip =  WiFi.localIP();
			
			Serial.printf("%s",msgStatus);
			Serial.print("IP address: ");
			Serial.println(WiFi.localIP());

			dispachEvent(EventType::conectedWifi);
			
			 // descarga el tiempo y meteo
			load();
			return;
		}
	}

	Serial.println("WiFi Error");
	strcpy(msgError, getStr(Str::errorWifi));
	dispachEvent(EventType::error);


	//volvemos intentarlo en 60sg
	retryLater();
}

void Model::enableSoftAP()
{
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP("Jardin_esp32", "123456789");
	Serial.print("softAPIP :");
	Serial.println(WiFi.softAPIP());


	startWebServer();
}

double Model::getDistanceRSSI() {
	/*
	* RSSI = TxPower - 10 * n * lg(d)
	* n = 2 (in free space)
	*
	* d = 10 ^ ((TxPower - RSSI) / (10 * n))
	*/
	double txPower = -40;
	return pow(10, ((double)txPower - rssi) / (10 * 2));
}

uint16_t Model::getElapsedAlarm()
{

	uint16_t elapsed = 0;

	if (isPaused()) {
		elapsed = elapsedPausedTask;
	}
	else {

		Task * t = tasks[currentZone][currentAlarm];
		Alarm * a = getAlarm(currentZone, currentAlarm);

		if (a == nullptr) {
			Serial.printf("getElapsedAlarm() alarm is null zone: %d alarm %d\n", currentZone, currentAlarm);
			return 0;
		}
		uint16_t da = Tasker::getDuration(a->init, a->end);
		uint16_t dt = Tasker::getDuration(t->start, t->stop);

		// tiempo trancurrido de la tarea actual
		elapsed = Tasker::getDuration(t->start, Tasker::getTickNow());

		// cuando la duracion de la alarma es mas grande 
		// que la duracion de la tarea (porq fue pausado)
		if (da > dt) {
			//sumamos esa dife
			elapsed += (da - dt);
		}
	}
	return elapsed;
}

uint16_t Model::getAlarmDuration(uint8_t zone, uint8_t index)
{
	Alarm * a = getAlarm(zone, index);
	return Tasker::getDuration(a->init, a->end);
}

uint8_t Model::getSymbol(uint8_t icon)
{
	// Dia
	if (icon < 5) return FontGlyph::sun;
	if (icon >= 5 && icon < 7) return FontGlyph::sun_cloud;
	if (icon >= 7 && icon < 9) return FontGlyph::cloud;
	if (icon == 11) return FontGlyph::mist;
	if (icon >= 12 && icon < 15) return FontGlyph::rain;
	if (icon >= 15 && icon < 18) return FontGlyph::thunder;
	if (icon == 18) return FontGlyph::rain;
	if (icon >= 19 && icon < 30) return FontGlyph::snow;

	// Noche
	if (icon >= 33 && icon < 35) return FontGlyph::moon;
	if (icon >= 35 && icon < 36) return FontGlyph::sun_cloud;
	if (icon == 37) return FontGlyph::mist;
	if (icon == 38) return FontGlyph::cloud;
	if (icon >= 39 && icon < 41)return FontGlyph::rain;
	if (icon >= 41 && icon < 43) return FontGlyph::thunder;
	if (icon >= 43 && icon < 45) return FontGlyph::snow;
	return FontGlyph::sun;
}

ZoneData * Model::getZone(uint8_t zone)
{
	return &zones[zone];
}

void Model::loadLocalTime()
{
	//https://github.com/G6EJD/ESP32-Time-Services-and-SETENV-variable/blob/master/README.md
	//https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
	configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
	//init and get the time
	//configTime(3600, 3600, "pool.ntp.org");

	struct tm timeinfo;

	if (getLocalTime(&timeinfo)) {

		loadedTime = true;


		Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

		setTime(timeinfo.tm_hour,// hours
			timeinfo.tm_min,// minutes
			timeinfo.tm_sec,// seconds 
			timeinfo.tm_mday,//day of the month
			timeinfo.tm_mon + 1, // month
			timeinfo.tm_year + 1900);// year    
	
		char buff[9];
		Tasker::tickTimeToChar(buff, Tasker::getTickNow());

		strcpy(msgStatus, getStr(Str::updatedTime));
		strcat(msgStatus, buff);
		//actualizar alarmas
		updateTasks();
		dispachEvent(EventType::loadedTime);
		return;
	}
	// a fallado
	//intentamos recuperar la hora de la eeprom
	if (config.time > 0) 
	{
		setTime(config.time);
		updateTasks();
		dispachEvent(EventType::loadedTime);
	}

	Serial.println("Failed to obtain time");
	strcpy(msgError, getStr(Str::errorTime));

	loadedTime = false;
	dispachEvent(EventType::error);
	retryLater();

}

void Model::loadForecast()
{
	const int httpPort = 80;
	if (client.connect(host, httpPort)) {
		
		String peta = String("url =") + host + url1 + config.cityID + url2;

		// This will send the request to the server
		client.print(String("GET ") + 
			url1 + config.cityID + url2 +
			" HTTP/1.1\r\n" + "Host: " + host + "\r\n" +
			"Connection: close\r\n\r\n");

		unsigned long timeout = millis();

		while (client.available() == 0) {
			if (millis() - timeout > 5000) {
				Serial.println(">>> Client Timeout !");
				client.stop();
				break;
			}
		}

		// HTTP headers end with an empty line
		if (client.available() && client.find("\r\n\r\n")
			&& parseForescast()) {

			loadedForescast = true;
			strcpy(msgStatus, getStr(Str::updatedFore));
			dispachEvent(EventType::loadedForescast);

			updateSunTask();
			
			return;
		}
	}

	strcpy(msgError, getStr(Str::errorFore));

	loadedForescast = false;
	dispachEvent(EventType::error);

	retryLater();
}

bool Model::parseForescast()
{
	// Allocate the JSON document
	// Use arduinojson.org/v6/assistant to compute the capacity.
	const size_t capacity = 2 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(6) + 9 * JSON_OBJECT_SIZE(2) + 24 * JSON_OBJECT_SIZE(3) + 6 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(9) + JSON_OBJECT_SIZE(15) + 2 * JSON_OBJECT_SIZE(20) + 5759;

	Serial.printf("\nram left %d\n", esp_get_free_heap_size());
	Serial.printf("capacity json %d \n", capacity);//capacity json 8911 
	DynamicJsonDocument doc(10000);

	Serial.printf("\nram left %d\n", esp_get_free_heap_size());

	//DynamicJsonDocument doc(2000);
	//StaticJsonDocument<10000> doc;

	// JSON input string.
	//
	// It's better to use a char[] as shown here.
	// If you use a const char* or a String, ArduinoJson will
	// have to make a copy of the input in the JsonBuffer.

	String json = client.readStringUntil('\r');

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, json);

	// Test if parsing succeeds.
	if (error) {
		Serial.print("deserializeJson() failed: ");
		Serial.println(error.c_str());

		//DeserializationError  IncompleteInput
		return false;
	}

	// Get the modes object in the document
	JsonObject root = doc.as<JsonObject>();

	// probamos el json q es correcto
	if (!root.containsKey("DailyForecasts")) {
		Serial.println("error estructura json");
		return false;
	}

	JsonObject DailyForecasts = root["DailyForecasts"][0];

	weather.time = DailyForecasts["EpochDate"]; // 1532149200
	weather.temp[0] = DailyForecasts["Temperature"]["Minimum"]["Value"];
	weather.temp[1] = DailyForecasts["Temperature"]["Maximum"]["Value"];

	Serial.printf("Temperatura min %d° max %d°", weather.temp[0], weather.temp[1]);

	JsonObject sun = DailyForecasts["Sun"];
	int32_t Sun_EpochRise = (int32_t)sun["EpochRise"] + MODEL_GTM; // 1536213600
	int32_t Sun_EpochSet = (int32_t)sun["EpochSet"] + MODEL_GTM; // 1536260160
	
	weather.sun[0] = Sun_EpochRise;
	weather.sun[1] = Sun_EpochSet;


		float hoursOfSun = DailyForecasts["HoursOfSun"]; // 5.9

	//dia
	JsonObject Forecasts_Day = DailyForecasts["Day"];
	WeatherData::Forecast * day = &weather.forecast[0];

	strcpy(day->phrase, Forecasts_Day["LongPhrase"]);// "Intervalos de nubes y sol"

	//Serial.printf("json len:%d sizeof:%d ",json.length(),sizeof(DailyForecasts0_Day["ShortPhrase"]));

	day->icon = Forecasts_Day["Icon"]; // 4
	

	day->precipitationProbability = Forecasts_Day["PrecipitationProbability"]; // 25

	day->win[0] = Forecasts_Day["Wind"]["Speed"]["Value"]; // 9.3// "km/h"
	day->win[1] = Forecasts_Day["Wind"]["Direction"]["Degrees"]; // 332

	day->totalLiquid = Forecasts_Day["TotalLiquid"]["Value"];// 0// "mm"

	day->hoursOfPrecipitation = Forecasts_Day["HoursOfPrecipitation"]; // 0
	day->cloudCover = Forecasts_Day["CloudCover"]; // 67

	// noche
	JsonObject Forecasts_Night = DailyForecasts["Night"];
	WeatherData::Forecast * night = &weather.forecast[1];

	strcpy(night->phrase, Forecasts_Night["LongPhrase"]);// "Intervalos de nubes y sol"
	night->icon = Forecasts_Night["Icon"]; // 36
	night->precipitationProbability = Forecasts_Night["PrecipitationProbability"]; // 25

	//Serial.printf("icon dia %d icon noche %d", day->icon,night->icon);

	night->win[0] = Forecasts_Night["Wind"]["Speed"]["Value"]; // 9.3// "km/h"
	night->win[1] = Forecasts_Night["Wind"]["Direction"]["Degrees"]; // 332

	night->totalLiquid = Forecasts_Night["TotalLiquid"]["Value"];// 0// "mm"

	night->hoursOfPrecipitation = Forecasts_Night["HoursOfPrecipitation"]; // 0
	night->cloudCover = Forecasts_Night["CloudCover"]; // 67

	return true;

}

void Model::readEprom()
{
	EEPROM.begin(sizeof(config));
	EEPROM.get(0, config);
	EEPROM.commit();
}

void Model::writeEprom()
{

	if (loadedTime) {
		config.time = now();
	}

	EEPROM.begin(sizeof(config));
	EEPROM.put(0, config);
	EEPROM.commit();
}

void Model::retryLater()
{
	using namespace std::placeholders;
	tasker.setTimeout(std::bind(&Model::onTimeOut, this, _1),retryTime);
}

void Model::onTimeOut(Task * current)
{
	if (!connectedWifi) {
		connectWifi();
		return;
	}
	if (!loadedTime)
		loadLocalTime();
	if (!loadedForescast)
		loadForecast();
}

void Model::updateSunTask()
{

	uint16_t s;
	if (isDay())
		s = model.weather.sun[1] - now();
	else
		s = model.weather.sun[0] - now();

	char buff[9], buff1[9];
	Tasker::tickTimeToChar(buff, model.weather.sun[0]);
	Tasker::tickTimeToChar(buff1, model.weather.sun[1]);

	Serial.printf("sol se levanta a %s se acuesta a %s\n", buff, buff1);
	Serial.printf("proxima actualizacion sol a %s\n", isDay()? buff1:buff);

	if (sunTask != nullptr) tasker.remove(sunTask);

	using namespace std::placeholders;
	sunTask = tasker.setTimeout(std::bind(&Model::onSunChanged, this, _1), s + 1);
}

void Model::onSunChanged(Task * t)
{
	char buff[9];
	Tasker::tickTimeToChar(buff, Tasker::getTickNow());
	Serial.printf("onSunChanged, %s \n", buff);

	updateSunTask();

	dispachEvent(EventType::loadedForescast);
}

SensorData * Model::getLog(uint8_t index)
{
	if(index <MODEL_SIZE_LOG)
		return &config.sensorLog[index];
	return nullptr;
}

bool Model::isDay()
{
	if (loadedForescast) {
		return model.weather.isDay();
	}
	return true;
}

char * Model::getCityName()
{
	return config.cityName;
}

uint8_t Model::getPin(uint8_t index)
{
	return pins[index];
}

bool Model::getIsTapOn(uint8_t pin)
{
	uint8_t index = getIndexPin(pin);
	return isPinOn[index];
}

int16_t Model::getIndexPin(uint8_t pin)
{
	for (uint8_t i = 0; i < countPins; i++)
	{
		if (pins[i] == pin)
			return i;
	}
	return -1;
}

bool Model::isWatering()
{
	// comprobamos si los otros pin estan abiertos
	for (uint8_t i = 0; i < countPins; i++)
	{
		if (isPinOn[i])
			return true;
	}
	return false;
}

bool Model::isPaused()
{
	return pausedTime > 0;
}

char * Model::getZoneName(uint8_t zone)
{
	if (zone < MODEL_MAX_ZONES) {
		char * nameZone = config.zoneNames[zone];
		if (strcmp(nameZone, "") != 0)
			return nameZone;
	}
	return nullptr;
}

bool Model::hasZone(uint8_t zone)
{
	return getZoneName(zone) != nullptr;
}

void Model::setZoneName(uint8_t zone, char * name)
{
	strncpy(config.zoneNames[zone], name,MODEL_MAX_CHAR_NAME_ZONE);
}

int8_t Model::addZone(char * name)
{
	//buscamos si existe
	int16_t index = -1;
	while (hasZone(++index)) {
		char * n = getZoneName(index);
		// si ya existe el nombre 
		if(strcmp(name, n) == 0) 
			//nos salimos
			return -1;
	}

	if (index > -1 && index < MODEL_MAX_ZONES) 
		setZoneName(index, name);

	return index;
}

bool Model::editZone(uint8_t zone, char * name)
{
	//buscamos si existe
	int16_t index = -1;
	while (hasZone(++index)) {
		char * n = getZoneName(index);
		// si ya existe el nombre 
		if (zone != index && strcmp(name, n) == 0)
			//nos salimos
			return false;
	}

	if (hasZone(zone))
	{
		setZoneName(zone, name);
		return true;
	}

	return false;
}

bool Model::removeZone(uint8_t zone)
{
	uint16_t size = -1;
	while (hasZone(++size)){}

	for (uint8_t z = zone; z < size - 1; z++)
	{

		char * n = getZoneName(z + 1);
		setZoneName(z, n);

		for (int16_t i = 0; i < MODEL_MAX_ALARMS;i++)
		{

			//borramos la tarea en tasker
			Task * t = tasks[z][i];
			if(z == zone && t != nullptr)
				tasker.remove(t);

			Alarm * alarm = &config.alarms[z][i];
			Alarm * alarmNext = &config.alarms[z+1][i];

			//desplazamos la alarma 
			alarm->copy(alarmNext);
			//desplazamos las tareas
			tasks[z][i] = tasks[z + 1][i];
		}
		//actualizamos los datos de zona
		updateZone(z);
	}

	uint8_t last = size - 1;
	//borramos ultimo registro
	setZoneName(last, "");

	for (int16_t i = 0; i < MODEL_MAX_ALARMS;i++)
	{
		Alarm * alarm = &config.alarms[last][i];
		alarm->setNull();
		tasks[last][i] = nullptr;
	}

	updateZone(last);

	printZones();

	return true;
}

bool Model::removeAlarm(uint8_t zone, uint8_t index)
{
	Serial.printf("remove alarm zone:%d index:%d\n", zone, index);
	if (hasZone(zone) && hasAlarm(zone, index)) {

		//buscamos el ultimo index
		int8_t size = -1;
		while (hasAlarm(zone, ++size)) {}

		Serial.printf("size %d\n", size);

		//borramos la tarea en tasker
		Task * t = tasks[zone][index];
		tasker.remove(t);

		//desplazamos los de delante para rellenar el hueco
		for (uint8_t i = index; i < size - 1; i++)
		{
			Alarm * alarm = getAlarm(zone, i);
			Alarm * alarmNext = getAlarm(zone, i + 1);

			alarm->copy(alarmNext);

			tasks[zone][i] = tasks[zone][i + 1];

			Serial.printf("alarm %d init:%d ",i, alarm->init);
			if(tasks[zone][i]!=nullptr)
				Serial.printf(" task init:%d\n",i, tasks[zone][i]->start);
			else Serial.println(" task is null");

		}

		//borramos el ultimo
		getAlarm(zone, size - 1)->setNull();
		tasks[zone][size - 1] = nullptr;


		// actualizamos la zona
		updateZone(zone);
		// actualizamos las tareas desde las alarmas
		updateTasks(zone);

		printAlarms(zone);
		return true;
	}
	return false;
}

void Model::updateZone(uint8_t zone)
{
	//refrescamos datos de la zona

	Alarm * alarm;
	uint32_t dura = 0;
	uint32_t start = UINT32_MAX;
	int16_t index = -1;


	while(hasAlarm(zone, ++index))
	{
		alarm = getAlarm(zone, index);
		dura += Tasker::getDuration(alarm->init, alarm->end);
		start = MIN(start, alarm->init);
	}

	if (start == UINT32_MAX) start = 0;

	zones[zone].duration = dura;
	zones[zone].start = start;

	zones[zone].alarmSize = index;
}

void Model::updateTasks(uint8_t zone)
{
	using namespace std::placeholders;

	for (int16_t i = -1; hasAlarm(zone, ++i); )
	{
		Alarm * alarm = getAlarm(zone, i);
		Task * t = tasks[zone][i];

		if (t == nullptr)
		{
			//Serial.printf("initAlarm crea task zone = %d iindex = %d init%d end%d\n", zone, iindex, alarm->init, alarm->end);

			t = tasker.add(
				alarm->init, alarm->end,
				std::bind(&Model::onWater, this, _1)
			);

			tasks[zone][i] = t;
		}
		else {
			//Serial.printf("initAlarm recupera task zone = %d iindex = %d init%d end%d\n", zone, iindex, alarm->init, alarm->end);
			t->start = alarm->init;
			t->stop = alarm->end;
		}
	}
}

void Model::readModes()
{
	int16_t i = -1;
	while (model.modes.has(++i))
	{
		Expression * exp = model.modes.get(i);
		//si nunca fue iniciado
		//config.modes[i] = false;
		exp->setEnabled(config.modes[i]);
	}
	lastTimeModesChanged = now();
	dispachEvent(EventType::modesChanged);
}

void Model::writeModes()
{
	int16_t i = -1;
	while (model.modes.has(++i))
	{
		Expression * exp = model.modes.get(i);
		config.modes[i] = exp->getEnabled();
	}
	writeEprom();
	lastTimeModesChanged = now();
	dispachEvent(EventType::modesChanged);
}

int8_t Model::addAlarm(uint8_t zone, uint8_t pin ,uint32_t time, uint16_t duration)
{

	if (hasZone(zone) && getIndexPin(pin) > -1)
	{
		//buscamos el ultimo index
		int8_t size = -1;
		while (hasAlarm(zone, ++size)) {}

		Alarm * a = &config.alarms[zone][size];
		a->pin = pin;
		a->init = time;
		a->end = time + duration;

		updateZone(zone);
		updateTasks(zone);

		return size;
	}
	
	return -1;
}

int8_t Model::addAlarm(uint8_t zone, uint8_t pin, uint16_t duration, uint8_t h, uint8_t m, uint8_t s)
{
	int32_t alarmTime = Tasker::getTickTime(h, m, s);
	return addAlarm(zone,pin,alarmTime,duration);
}

bool Model::editAlarm(uint8_t zone, uint8_t index, uint8_t pin, uint32_t time, uint16_t duration)
{
	Serial.printf(
		"edita alarm zone %d | pin %d | pin %d | time %d | duration %d\n",
		zone, index, pin, time, duration);

	if (hasZone(zone) && hasAlarm(zone, index))
	{
		Alarm * a = getAlarm(zone, index);

		a->pin = pin;
		a->init = time;
		a->end = time + duration;

		Task * t = tasks[zone][index];
		t->enabled = true;
		t->runing = false;

		updateZone(zone);
		updateTasks(zone);
		
		return true;
	}
	
	return false;
}

Alarm * Model::getAlarm(uint8_t zone, uint8_t index)
{
	if (index < MODEL_MAX_ALARMS) {
		Alarm * a = &config.alarms[zone][index];
		if (a->pin != NULLPIN)
			return a;
	}
	return nullptr;
}

bool Model::hasAlarm(uint8_t zone, uint8_t index)
{
	return getAlarm(zone, index) != nullptr;
}

bool Model::canWatering()
{
	int8_t evaluate = modes.evaluate();
	Serial.printf("skip %d evaluate %d result %d\n",modes.skip(),evaluate, (!modes.skip() && modes.evaluate() < 50));
	// 
	return !modes.skip() && (evaluate < 50 || evaluate < 0);
}

bool Model::switchMode(uint8_t index)
{
	if (index < modes.count) {
		Expression * exp = modes.get(index);
		bool enabled = exp->getEnabled();
		exp->setEnabled(!enabled);

		writeModes();

		return true;
	}
	
	return false;
}

uint8_t Model::getNextZone()
{
	uint8_t _zone;
	uint32_t _now = Tasker::getTickNow();
	uint32_t current;
	uint32_t min = UINT32_MAX;

	for (int16_t zone = -1; hasZone(++zone);)
	{

		Serial.printf("next zone %s %d\n", getZoneName(zone),zone);
		for (int16_t i = -1;  hasAlarm(zone, ++i);)
		{
			Alarm * alarm = getAlarm(zone, i);

			current = Tasker::howTimeLeft(alarm->init);

		//Serial.printf("min %d current %d\n", min,current);
			if (current < min) {
				min = current;
				_zone = zone;
			}
		}
	}

	return _zone;
}

/*			Web server			*/

void Model::startWebServer()
{

	//webserver
	server.on("/", std::bind(&Model::handleRoot, this));
	server.on("/sensorLog", std::bind(&Model::handleSensorLog, this));
	server.on("/sensor", std::bind(&Model::handleSensor, this));
	server.on("/taps", std::bind(&Model::handleTaps, this));
	server.on("/status", std::bind(&Model::handleStatus, this));
	server.on("/alarms", std::bind(&Model::handleAlarms, this));
	server.on("/zones", std::bind(&Model::handleZones, this));
	server.on("/weather", std::bind(&Model::handleWeather, this));
	server.on("/editZone", std::bind(&Model::handleEditZone, this));
	server.on("/editAlarm", std::bind(&Model::handleEditAlarm, this));
	server.on("/config", std::bind(&Model::handleConfig, this));
	server.on("/modes", std::bind(&Model::handleModes, this));

	server.onNotFound(std::bind(&Model::handleNotFound, this));

	server.begin();
	Serial.println("HTTP server started");
}

void Model::handleSensorLog()
{
	DynamicJsonDocument doc(2000);
	//StaticJsonDocument<3000> doc;

	// Make our document represent an object
	JsonArray root = doc.to<JsonArray>();

	SensorData * s;

	for (uint8_t i = 0; i < MODEL_SIZE_LOG; i++)
	{
		s = getLog(i);

		JsonObject sensor = root.createNestedObject();
		sensor["temp"] = s->temperature;
		sensor["press"] = s->pressure;
		sensor["hum"] = s->humidity;
		sensor["time"] = s->time;
	}
	//serializeJson(modes, Serial);

	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	Serial.println(str.length());
	server.send(200, "application/json", str);
}

void Model::handleSensor()
{
	StaticJsonDocument<500> doc;

	// Make our document represent an object
	JsonObject root = doc.to<JsonObject>();

	root["temp"] = currentSensor.temperature;
	root["press"] = currentSensor.pressure;
	root["hum"] = currentSensor.humidity;
	root["time"] = currentSensor.time;

	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	server.send(200, "application/json", str);

}

void Model::handleTaps()
{

	if (server.hasArg("pin") && server.hasArg("cmd")) {
		uint8_t pin = (uint8_t)server.arg("pin").toInt();
		bool open = server.arg("cmd").toInt() != 0;

		isManualWater = true;

		int r = (int)openTap(pin, open);
		Serial.printf("open (%d,%d)\n", pin, open);
		//server.send(200, "text/plain", String(r));
	}

	DynamicJsonDocument doc(300);

	// Make our document represent an object
	JsonArray root = doc.to<JsonArray>();

	for (uint8_t i = 0; i < countPins; i++)
	{
		JsonObject oPin = root.createNestedObject();
		oPin["pin"] = pins[i];
		oPin["open"] = (uint8_t)isPinOn[i];
	}

	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	server.send(200, "application/json", str);
}

void Model::handleStatus()
{
	Flags f;

	for (uint8_t i = 0; i < countPins; i++)
	{
		if (isPinOn[i]) f.add(pow(2, i));
	}

	// riego(0-1) | status (stamby/manual/zone) | time (de cuando se cambiaron los datos de zonas/alarmas
	String str = String("") + f.get() + "|";
/*
	String modeWater = "0"; ;
	if (isManualWater) modeWater = "1";// abriendo grifos manualmente
	else if (currentZone > -1 ) 
	{	// regando zona
		if(!isPaused())
			modeWater = "2"; 
		else modeWater = "3";
	}

	str += modeWater + "|";*/
	str += String("") + lastTimeZoneChanged + "|";
	str += String("") + lastTimeModesChanged;

	server.send(200, "text/plain", str);
}

void Model::handleZones()
{
	int8_t zoneID;
	int8_t cmd;
	bool success = false;

	if (server.hasArg("id") && server.hasArg("cmd")) {
		zoneID = server.arg("id").toInt();
		cmd = server.arg("cmd").toInt();

		switch (cmd)
		{
		case 0://parar
			success = stopWaterZone();
			break;
		case 1://iniciar
			success = waterZone(zoneID);
			break;
		case 2://pausar
			success = pauseWaterZone();
			break;
		}

		Serial.printf("callZone cmd %d success %d\n", cmd, success);

	}

	DynamicJsonDocument doc(2000);
	JsonArray root = doc.to<JsonArray>();

	for (int16_t zone = -1; hasZone(++zone); )
	{
		ZoneData * z = getZone(zone);

		JsonObject o = root.createNestedObject();
		o["id"] = zone;
		o["name"] = getZoneName(zone);
		o["duration"] = z->duration;
		o["start"] = z->start;
		
		if (zone == currentZone || (success && cmd == 1 && zone == zoneID)) {
			o["running"] = 1;

			if (isPaused()) 
				o["paused"] = 1;
			else 
				o["paused"] = 0;

			uint16_t elapsed = getElapsedAlarm();

			for (uint8_t i = 0; i < currentAlarm; i++)
			{
				Alarm * alarm = getAlarm(zone, i);
				elapsed += Tasker::getDuration(alarm->init, alarm->end);
			}

			o["elapsed"] = elapsed;
			Serial.printf("----elapsed/%d paused/%d\n", elapsed, isPaused());
		}
		else {
			o["running"] = 0;
			o["elapsed"] = 0;
			o["paused"] = 0;
		}

	}

	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	server.send(200, "application/json", str);
}

void Model::handleWeather()
{

	WeatherData::Forecast * forecast = model.weather.getCurrent();

	DynamicJsonDocument doc(500);
	// Make our document represent an object
	JsonObject root = doc.to<JsonObject>();
	root["cityName"] = config.cityName;
	root["minTemp"] = model.weather.minTemp();
	root["maxTemp"] = model.weather.maxTemp();
	root["phrase"] = forecast->phrase;
	root["cloudCover"] = forecast->cloudCover;
	root["precipitationProbability"] = forecast->precipitationProbability;
	root["totalLiquid"] = forecast->totalLiquid;
	root["hoursOfPrecipitation"] = forecast->hoursOfPrecipitation;
	root["idIcon"] = getSymbol(forecast->icon);

	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	server.send(200, "application/json", str);

}

void Model::handleModes()
{
	if (server.hasArg("mode")) {
		uint8_t index = (uint8_t)server.arg("mode").toInt();
		switchMode(index);
	}

	DynamicJsonDocument doc(1000);


	// Make our document represent an object
	JsonObject root = doc.to<JsonObject>();
	
	if (model.canWatering())
		root["prediction"] = getStr(Str::wateringStr);
	else
		root["prediction"] = getStr(Str::notWatering);
	
	JsonArray jmodes = root.createNestedArray("modes");

	//print
	int16_t i = -1;
	while (modes.has(++i))
	{
		JsonObject obj = jmodes.createNestedObject();
		Expression * exp = modes.get(i);
		obj["name"] = String (exp->getName());
		obj["enabled"] = exp->getEnabled();
		Serial.printf("%s enabled: %d evaluate: %d skip: %d\n",
			exp->getName(), exp->getEnabled(),
			exp->evaluate(), exp->skip()
		);
	}
	String str;
	// Write JSON document
	serializeJsonPretty(root, str);
	server.send(200, "application/json", str);
}

void Model::handleAlarms()
{

	DynamicJsonDocument doc(500);
	JsonArray root = doc.to<JsonArray>();

	if (server.hasArg("zone")) {
		uint8_t zone = (uint8_t)server.arg("zone").toInt();

		for (int16_t i = -1; hasAlarm(zone, ++i); )
		{
			Alarm * alarm = getAlarm(zone, i);
			JsonObject jAlarm = root.createNestedObject();
			jAlarm["pin"] = alarm->pin;
			jAlarm["time"] = alarm->init;
			jAlarm["duration"] = alarm->end - alarm->init;
		}

		String str;
		// Write JSON document
		serializeJsonPretty(root, str);
		server.send(200, "application/json", str);

	}
	else {
		server.send(200, "text/plain", "-1");
	}
}

void Model::handleEditZone()
{

//nuevo		localhost/editZone?cmd=0&name=Huerta
//edit		localhost/editZone?cmd=1&id=1&name=Lechugas
//borrar	localhost/editZone?cmd=2&id=2
	if (server.hasArg("cmd")) {
		uint8_t cmd = (uint8_t)server.arg("cmd").toInt();

		bool success = false;

		if (server.hasArg("name") && server.arg("name").length() > 0)
		{
			char name[MODEL_MAX_CHAR_NAME_ZONE];
			server.arg("name").toCharArray(name, MODEL_MAX_CHAR_NAME_ZONE);

			if (cmd == 0) {
				success = addZone(name) > -1;
			}
			else if (server.hasArg("id") && cmd == 1) {
				uint8_t zone = (uint8_t)server.arg("id").toInt();
				success = editZone(zone, name);
			}

		}
		else if (server.hasArg("id") && cmd == 2) {
			uint8_t zone = (uint8_t)server.arg("id").toInt();
			success = removeZone(zone);
		}

		if (success) {
			writeEprom();
			lastTimeZoneChanged = now();
			dispachEvent(EventType::zoneChanged);
		}

		server.send(200, "text/plain", String((int)success));
	}
	else {
		server.send(200, "text/plain", "-1");
	}
}

void Model::handleEditAlarm()
{
//nuevo		localhost/editAlarm?cmd=0&idZone=2&pin=17&time=64800&duration=25
//edit		localhost/editAlarm?cmd=1&idZone=2&pin=17&time=64800&duration=25&id=0
//borrar	localhost/editAlarm?cmd=2&idZone=2&id=0
	if (server.hasArg("cmd") && server.hasArg("idZone")) {

		uint8_t zone = (uint8_t)server.arg("idZone").toInt();
		uint8_t cmd = (uint8_t)server.arg("cmd").toInt();

		bool success = false;

		if (server.hasArg("pin") &&
			server.hasArg("time") &&
			server.hasArg("duration"))
		{
			uint8_t pin = (uint8_t)server.arg("pin").toInt();
			uint32_t time = server.arg("time").toInt();
			uint16_t duration = (uint16_t)server.arg("duration").toInt();

			if (cmd == 0) {
				success = addAlarm(zone, pin, time, duration) > -1;
			}
			else if (cmd == 1 && server.hasArg("id"))
			{
				uint8_t index = (uint8_t)server.arg("id").toInt();
				success = editAlarm(zone, index, pin, time, duration);
			}
		}
		else if (cmd == 2 && server.hasArg("id"))
		{
			uint8_t index = (uint8_t)server.arg("id").toInt();
			success = removeAlarm(zone, index);
		}

		if (success) {
			writeEprom();
			lastTimeZoneChanged = now();
			dispachEvent(EventType::zoneChanged);
		}

		server.send(200, "text/plain", String((int)success));
	}
	else {
		server.send(200, "text/plain", "-1");
	}
}

void Model::handleConfig()
{
	//edita wifi			localhost/config?cmd=0&ssid=vomistar_69&pass=teta
	//edita meteo			localhost/config?cmd=0&cityID=1451030&cityName=Tintores 
	//reboot				localhost/config?cmd=1
	//reiniciar alarmas		localhost/config?cmd=2
	//reiniciar sensorlog	localhost/config?cmd=3
	//imprime config	localhost/config
	if (server.hasArg("cmd")) {

		uint8_t cmd = (uint8_t)server.arg("cmd").toInt();

		Serial.printf("cmd %d\n", cmd);

		bool success = false;

		if (server.hasArg("ssid") &&
			server.hasArg("pass"))
		{
			char ssid[32], pass[64];
			
			server.arg("ssid").toCharArray(ssid,32);
			server.arg("pass").toCharArray(pass,64);

			strcpy(config.ssid, ssid);
			strcpy(config.password, pass);

			WiFi.disconnect(true);

			enableSoftAP();
			connectWifi();
			
			success = true;
		}
		else if (server.hasArg("cityID") &&
			server.hasArg("cityName"))
		{
			char cityID[10], cityName[20];

			server.arg("cityID").toCharArray(cityID,10);
			server.arg("cityName").toCharArray(cityName,20);

			strcpy(config.cityID, cityID);
			strcpy(config.cityName, cityName);

			loadForecast();

			success = true;
		}
		else if(cmd == 1)
		{
			loadDefaultConfig();
			success = true;
		}
		else if (cmd == 2) {
			server.send(200, "text/plain", "1");
			ESP.restart();
			return;
		}
		else if (cmd == 3) {
			success = true;
			resetLogSensor();
			initLogSensor();
		}

		if (success) {
			writeEprom();
			lastTimeZoneChanged = now();
			dispachEvent(EventType::zoneChanged);
		}
		
		server.send(200, "text/plain", String((int)success));

	}
	else {

		StaticJsonDocument<500> doc;

		// Make our document represent an object
		JsonObject root = doc.to<JsonObject>();

		root["RSSI"] = rssi;
		root["distanceRSSI"] = model.getDistanceRSSI();
		root["ssid"] = config.ssid;
		root["cityID"] = config.cityID;
		root["cityName"] = config.cityName;

		String str;
		// Write JSON document
		serializeJsonPretty(root, str);
		server.send(200, "application/json", str);
	}
}

void Model::handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}

void Model::handleRoot()
{
	char temp[400];
	char buff[9];
	Tasker::tickTimeToChar(buff, currentSensor.time);
	const char * html =
		"<html>\n\
	<head>\n\
		<meta http-equiv='refresh' content='5'/>\n\
		<title>Jardin</title>\n\
		<style>\n\
			body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n\
		</style>\n\
	</head>\n\
	<body>\n\
		<h1>Jardin</h1>\n\
		<p>temp: %0.00f</p>\n\
		<p>pressure: %0.00f</p>\n\
		<p>hum: %d %%</p>\n\
		<p>time: %s</p>\n\
	</body>\n\
</html>";

	snprintf(temp, 400, html,
		currentSensor.temperature, currentSensor.pressure, currentSensor.humidity, buff);
	server.send(200, "text/html", temp);

}

// observer
void Model::addEvent(EventType type, EventChange * callBack)
{
	if (listenerCount < MODEL_MAX_LISTENERS) {
		listeners[listenerCount] = callBack;
		types[listenerCount++] = type;
	}
}
void Model::dispachEvent(EventType ev)
{
	for (uint8_t i = 0; i < listenerCount; i++)
	{
		EventChange * e = listeners[i];
		EventType t = types[i];

		if (t == ev || t == EventType::all)
		{
			e->onChange(ev);
		}
	}
}


ViewControler controler;
Model model;

/*			WaterView			*/

void WaterView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	txt1.setText(getStr(Str::zonesStr));

	model.addEvent(EventType::all, this);
}

char * WaterView::nameClass()
{
	return "WaterView";
}

void WaterView::onChange(EventType e)
{
	if (
		(e == EventType::status && model.status == Status::started) ||
		e == EventType::loadedTime || e == EventType::zoneChanged)
	{
		int16_t zoneID = -1;
		while (model.hasZone(++zoneID))
		{
			//Serial.printf("zone %s iindex%d\n", model.getZoneName(iindex),iindex);
			ZoneData * zone = model.getZone(zoneID);

			zones[zoneID].setZone(zoneID, zone);

			if (!existView(&zones[zoneID]))
				addView(&zones[zoneID]);
			
		}
		// Todo borrar excesos vistas cuando se borro uno
		for (uint8_t i = zoneID; i < viewCount; i++)
		{
			removeView(i);
		}

	}
	else if (e == EventType::status && 
		(model.status == Status::watering &&
			!model.isManualWater && model.currentZone > -1))
	{
		Serial.printf("zona:%s alarma:%d %s\n",
			model.getZoneName(model.currentZone), model.currentAlarm,
			model.isWatering() ? "regando" : "cerrando"
		);
		//todo
		ZoneView * zone = &zones[model.currentZone];
		zone->updateZone();
		setCurrent(zone);
		controler.setCurrent(this);

	}
	else if (e == EventType::status && 
		(model.status == Status::standby)) {
		for (uint8_t i = 0; i < viewCount; i++)
		{

			ZoneView * zone = (ZoneView*) getView(i);
			//zone->updateZone();
		}
	}
	else if (e == EventType::modesChanged) {
		// hoy se puede regar ?
		if (model.canWatering())
		{
			txt.setText(getStr(Str::wateringStr));
		}
		else {
			txt.setText(getStr(Str::notWatering));
		}
	}
}

bool WaterView::acceptClickButton(uint8_t id)
{
	if (currentView != nullptr &&
		currentView->acceptClickButton(id))
		return true;

	Buttons b = (Buttons)id;

	if (b == btCenter && currentView == nullptr)
	{
		setCurrent(&zones[0]);
		return true;
	}
	else if (b == btUp && currentView != nullptr)
	{
		next();
		return true;
	}
	else if (b == btDown && currentView != nullptr)
	{
		previous();
		return true;
	}
	return false;
}

/*			ZonesView			*/

void ZoneView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}
	bt2.selected = true;
	bt2.setText(getStr(Str::waterStr));
	bt1.setText(getStr(Str::pauseStr));
}

bool ZoneView::acceptClickButton(uint8_t id)
{
	Buttons b = (Buttons)id;

	if (b == btRigth)
	{
		bt1.selected = !bt1.selected;
		bt2.selected = !bt2.selected;
		return true;
	}
	else if (b == btCenter)
	{
		// regar-cancelar
		if (bt2.selected) {
			//todo
			Serial.printf("\nregando %d model.zone%d zone%d\n", model.isWatering(), model.currentZone, zoneID);
			if (zoneID == model.currentZone) {
				model.stopWaterZone();
			}
			else {
				model.waterZone(zoneID);
			}
		}
		// pausa
		else
		{
			model.pauseWaterZone();
		}
		return true;
	}
	else if (b == btLeft)
	{
		parentView->home();
		return true;
	}
	return false;
}

void ZoneView::setZone(uint8_t zoneID, ZoneData * zone)
{
	this->zoneID = zoneID;
	this->zone = zone;
	txt1.setText(model.getZoneName(zoneID));
	updateZone();
}

void ZoneView::updateZone()
{

	if (model.currentZone == zoneID) 
	{	
		if(model.isWatering() || model.isPaused())
			bt2.setText(getStr(Str::cancelStr));
		else bt2.setText(getStr(Str::waterStr));

		if(model.isPaused())
			bt1.setText(getStr(Str::resumeStr));
		else bt1.setText(getStr(Str::pauseStr));

	}
	else 
	{
		bt2.setText(getStr(Str::waterStr));
		bt1.setText(getStr(Str::pauseStr));
	}

	char buff[9], result[18];

	Tasker::tickTimeToChar(buff, zone->start);
	sprintf(result, "%s", buff);
	txt2.setText(result);

	sprintf(result, " %d min %d/%d",
		(int)(zone->duration / 60),
		0,
		zone->alarmSize
	);
	txt3.setText(result);
}

void ZoneView::measure()
{
	static uint32_t currentTime = 0;

	View::measure();

	if (model.currentZone == zoneID && millis() - currentTime > 1000) {
		currentTime = 0;

		char buff[9], result[18];

		uint16_t ad = model.getAlarmDuration(model.currentZone, model.currentAlarm);

		Tasker::tickTimeToChar(buff, ad - model.getElapsedAlarm());

		sprintf(result, "%s", buff);
		txt2.setText(result);

		sprintf(result, " %d min %d/%d",
			(int)(zone->duration / 60),
			model.currentAlarm+1,
			zone->alarmSize
		);
		txt3.setText(result);
	}
}

char * ZoneView::nameClass()
{
	return "ZoneView";
}

/*			TapsView			*/

void TapsView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	model.addEvent(EventType::all, this);
}

char * TapsView::nameClass()
{
	return "TapsView";
}

void TapsView::onChange(EventType e)
{

	if (
		(e == EventType::status && model.status == Status::started) ||
		e == EventType::loadedTime)
	{
		char * tap = getStr(Str::tapStr);
		char buffer[10];

		list.reset();

		for (int16_t i = 0; i < model.countPins; i++)
		{
			sprintf(buffer, "%s %d", tap, model.getPin(i));
			list.addItem(buffer);
		}
	}
	else if (e == EventType::status && model.status == Status::watering) {
		uint8_t index = model.getIndexPin(model.currentPin);
		//Serial.printf("manual regando id%d pin%d isOn%d\n", index, model.currentPin,model.getIsTapOn(model.currentPin));
		list.selectItem(index, model.getIsTapOn(model.currentPin));
	}
}

bool TapsView::acceptClickButton(uint8_t id)
{
	Buttons b = (Buttons)id;

	if (b == btUp)
	{
		list.up();
		return true;
	}
	else if (b == btCenter)
	{
		uint8_t index = list.selectedItem();
		Serial.printf("accept pin:%d id:%d isOn : %d\n", model.getPin(id), id, model.getIsTapOn(model.getPin(id)));
		model.switchTap(model.getPin(index));
		return true;
	}
	else if (b == btDown)
	{
		list.down();
		return true;
	}
	return false;
}

/*			ModesView			*/

void ModesView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	model.addEvent(EventType::all, this);
}

char * ModesView::nameClass()
{
	return "ModesView";
}

void ModesView::onChange(EventType e)
{
	if (e == EventType::status && model.status == Status::started)
	{
		char buffer[20];

		list.reset();
		
		int16_t i = -1;
		while (model.modes.has(++i))
		{
			Expression * exp = model.modes.get(i);
			sprintf(buffer, "%s", exp->getName());
			list.addItem(buffer);
		}
	}
	else if (e == EventType::modesChanged) {
		
		Serial.println();
		int16_t i = -1;
		while (model.modes.has(++i))
		{
			Expression * exp = model.modes.get(i);
			list.selectItem(i,exp->getEnabled());
			/*Serial.printf("%s enabled: %d evaluate: %d skip: %d\n",
				exp->getName(),exp->getEnabled(),
				exp->evaluate(),exp->skip()
			);*/
		}
		//Serial.printf("Modes changed %d\n",model.canWatering());
	}
}

bool ModesView::acceptClickButton(uint8_t id)
{
	Buttons b = (Buttons)id;

	if (b == btUp)
	{
		list.up();
		return true;
	}
	else if (b == btCenter)
	{
		uint8_t index = list.selectedItem();
		model.switchMode(index);
		return true;
	}
	else if (b == btDown)
	{
		list.down();
		return true;
	}
	return false;
}

/*			TimeView			*/

void TimeView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}
}

void TimeView::measure()
{
	View::measure();

	char buff[TEXT_MAX_SIZE];
	Tasker::tickTimeToChar(buff, Tasker::getTickNow());
	txt2.setText(buff);

	txt1.setText(getDayStr(weekday()));

	sprintf(buff, "%d %s %d",
		day(),
		getMonthStr(month()),
		year()
	);

	txtS.setText(buff);
}

char * TimeView::nameClass()
{
	return "TimeView";
}

/*			ServerView			*/

void ServerView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}
}

void ServerView::measure()
{
	View::measure();

	String softAPIP = String("AP ") + WiFi.softAPIP().toString();
	txt3.setText(softAPIP.c_str());

	if (WiFi.status() == WL_CONNECTED) {

		String localIP = String("Lan ") + WiFi.localIP().toString();
		txt2.setText(localIP.c_str());

		char buff[TEXT_MAX_SIZE];

		sprintf(buff, "%ddBm %.2fm",
			model.rssi,
			model.getDistanceRSSI()
		);

		txtS.setText(buff);
	}
	else
	{
		txt2.setText("0.0.0.0");
		txtS.setText(getStr(Str::errorWifi));
	}
}

char * ServerView::nameClass()
{
	return "ServerView";
}

void ServerView::onChange(EventType e)
{

}

bool ServerView::acceptClickButton(uint8_t id)
{
	return false;
}

/*			AlertView			*/

void MessageView::print(char * msg)
{
	//borramos lo anterior
	for (uint8_t i = 0; i < 3; i++)
	{
		uis[i]->setText("");
	}

	//cortamos en lineas
	char * split = strtok(msg, "|");
	byte i = 0;
	while (split != NULL)
	{
		uis[i++]->setText(split);
		split = strtok(NULL, "|");
	}

	draw();
}

void MessageView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}
}

void MessageView::draw()
{
	u8g2.firstPage();
	do {

		for (size_t i = 0; i < childCount; i++)
		{
			Ui *ui = childs[i];
			ui->draw();
		}
	} while (u8g2.nextPage());
}

char * MessageView::nameClass()
{
	return "MessageView";
}

/*			SensorView			*/

void SensorView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	addView(&sensorLogTemp);
	addView(&sensorLogPress);
	addView(&sensorLogHum);

	model.addEvent(EventType::sensor, this);
}

char * SensorView::nameClass()
{
	return "SensorView";
}

bool SensorView::acceptClickButton(uint8_t id)
{
	if (currentView != nullptr &&
		currentView->acceptClickButton(id))
		return true;

	Buttons b = (Buttons)id;
	if (b == btLeft && currentView != nullptr)
	{
		home();
		return true;
	}else if (b == btCenter && currentView == nullptr)
	{
		setCurrent(&sensorLogTemp);
		return true;
	}
	if (b == btUp && currentView != nullptr) {
		next();
		return true;
	}
	else if (b == btDown && currentView != nullptr) {
		previous();
		return true;
	}else if(b == btRigth && currentView != nullptr)
		return true;

	return false;
}

void SensorView::onChange(EventType e)
{
	float temp = model.currentSensor.temperature;
	char buff[] = "99.99°c";
	sprintf(buff, "%.2f°c", temp);
	temperature.setText(buff);

	float press = model.currentSensor.pressure;
	sprintf(buff, "%.1fhpa", press);
	pressure.setText(buff);

	uint8_t hum = model.currentSensor.humidity;
	sprintf(buff, "%d%%", hum);
	humidity.setText(buff);
}

/*			SensorLog			*/

SensorLog::SensorLog(uint8_t column)
{
	sensorType = column;
}

void SensorLog::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	model.addEvent(EventType::sensorLog, this);
}

char * SensorLog::nameClass()
{
	return "SensorLog";
}

void SensorLog::onChange(EventType e)
{
	averageValue = 0;
	maxValue = -FLT_MAX;
	minValue = FLT_MAX;

	for (uint8_t i = 0; i < MODEL_SIZE_LOG; i++)
	{
		SensorData * tmp = model.getLog(i);
		float value = getValue(tmp);
		data[i] = value;
		maxValue = MAX(value, maxValue);
		minValue = MIN(value, minValue);
		averageValue += value;
	}
	averageValue /= MODEL_SIZE_LOG;

	graf.setData(data, MODEL_SIZE_LOG, maxValue, minValue);

	char buff[5] = "1000";
	snprintf(buff, 5, "%d", (uint16_t)maxValue);
	txt1.setText(buff);

	snprintf(buff, 5, "%d", (uint16_t)averageValue);
	txt2.setText(buff);

	snprintf(buff, 5, "%d", (uint16_t)minValue);
	txt3.setText(buff);

	txt4.setText(getTitle());
}

float SensorLog::getValue(SensorData * sensor)
{
	switch (sensorType)
	{
	case 0: return sensor->temperature;
	case 1: return sensor->pressure;
	case 2: return sensor->humidity;
	}
	return 0.0f;
}

char * SensorLog::getTitle()
{
	switch (sensorType)
	{
	case 0: return getStr(Str::temperature);
	case 1: return getStr(Str::pressure);
	case 2: return getStr(Str::humidity);
	}
	return "";
}

/*			ForecastView		*/

char * ForecastView::nameClass()
{
	return "ForecastView";
}

void ForecastView::begin()
{
	for (uint8_t i = 0; i < CONTAINER_MAX_SIZE; i++)
	{
		Ui * ui = uis[i];
		if (ui == nullptr) break;
		addChild(ui);
	}

	txt1.setText(model.getCityName());

	model.addEvent(EventType::loadedForescast, this);
}

void ForecastView::onChange(EventType e)
{
	int8_t minTemp = model.weather.minTemp();
	int8_t maxTemp = model.weather.maxTemp();

	char buff[TEXTSCROLL_MAX_SIZE] = "28/13°c";
	sprintf(buff, "%d/%d°c", maxTemp, minTemp);
	temperature.setText(buff);

	WeatherData::Forecast * forecast = model.weather.getCurrent();

	uint8_t idIcon = forecast->icon;
	icon.fontGlyph = model.getSymbol(idIcon);

	Serial.printf("hace sol %s icon:%d speedwin %.1f\n", index ? "false" : "true", idIcon, model.weather.speedWin());

	char * phrase = forecast->phrase;
	uint8_t cloudCover = forecast->cloudCover;
	uint8_t precipitationProbability = forecast->precipitationProbability;
	float totalLiquid = forecast->totalLiquid;
	uint8_t hoursOfPrecipitation = forecast->hoursOfPrecipitation;
	float speedWin = model.weather.speedWin();

	sprintf(buff, getStr(Str::formatFore),
		speedWin, cloudCover, precipitationProbability, totalLiquid, hoursOfPrecipitation, phrase);

	ts.setText(buff);
}

/*			controler			*/

void ViewControler::begin()
{

	inputButtons.setup(ANALOG_PIN_BUTTONS, INPUT);

	AnalogButton b = AnalogButton(btUp, 3278); // id,value
	inputButtons.addButton(b);

	b = AnalogButton(btCenter, 2745);
	inputButtons.addButton(b);

	b = AnalogButton(btDown, 2372);
	inputButtons.addButton(b);

	b = AnalogButton(btLeft, 2095);
	inputButtons.addButton(b);

	b = AnalogButton(btRigth, 1870);
	inputButtons.addButton(b);

	inputButtons.addEvent(this);

	model.addEvent(EventType::all, this);
	messageView.begin();

	managerView.add(&forecastView);
	managerView.add(&sensorView);
	managerView.add(&timeView);
	managerView.add(&waterView);
	managerView.add(&modesView);
	managerView.add(&tapsView);
	managerView.add(&serverView);
	managerView.begin();
	managerView.setCurrent(&timeView);
}

void ViewControler::update()
{
	uint32_t c = millis();

	inputButtons.check();

	//if (millis() - c > 60)
	//	Serial.printf("controler.update check %d ms\n", millis() - c);

	//c = millis();
	managerView.draw();

	if (millis() - c > 60)
		Serial.printf("controler.update draw %d ms\n", millis() - c);

	//apagamos la pantalla cuando no hay actividad en 5 min
	if(
		millis() - lastTimeInput > delayDisplayOff 
		&& managerView.isEnabledDisplay())
	{
		Serial.println(delayDisplayOff);
		managerView.enableDisplay(false);
	}
	// iniciamos el pase de paginas cuando no hay actividad en 1 min 
	else if (
		millis() - lastTimeInput > delayScroll
		&& !managerView.isStarted()
		&& managerView.isEnabledDisplay()) 
	{
		managerView.start();
		Serial.println("start");
	}

}

void ViewControler::onChange(EventType e)
{
	if (model.status == starting &&
		(e == EventType::conectedWifi ||
			e == EventType::loadedTime ||
			e == EventType::loadedForescast ||
			e == EventType::status))
	{
		messageView.print(model.msgStatus);
		Serial.println(F("delay 2sg"));
		delay(DELAY_MSG_DISPLAY);
	}
	else if (model.status == starting && e == EventType::error)
	{
		messageView.print(model.msgError);
		Serial.println(F(" errordelay 2sg"));
		delay(DELAY_MSG_DISPLAY);
	}
	else if (model.status == started && e == EventType::status)
	{
		managerView.start();
	}
}

void ViewControler::setCurrent(View * view)
{
	managerView.stop();
	managerView.enableDisplay(true);
	lastTimeInput = millis();
	managerView.setCurrent(view);
}

void ViewControler::onClick(uint8_t id, bool isHeldDown)
{
	managerView.stop();
	lastTimeInput = millis();

	if (!managerView.isEnabledDisplay()) {
		managerView.enableDisplay(true);
		return;
	}

	if (!managerView.acceptClickButton(id)) {

		switch ((Buttons)id)
		{
		case btLeft:
			managerView.next();
			break;
		case btRigth:
			managerView.previous();
			break;
		}
		//Serial.println("bt acepted false");
	}
	else {
		//Serial.println("bt acepted true");
	}

	//Serial.printf("button %s clicked Controler, isHeldDown %s\n", buff, isHeldDown ? "true" : "false");
}

void Alarm::copy(Alarm * alarm)
{
	init = alarm->init;
	end = alarm->end;
	pin = alarm->pin;
}

void Alarm::setNull()
{
	pin = NULLPIN;
	init = 0;
	end = 0;
}

/*			Weather			*/

bool WeatherData::isDay()
{
	return now() > sun[0] && now() < sun[1];
}

WeatherData::Forecast * WeatherData::getCurrent()
{
	// dia = 0 noche = 1
	uint8_t index = !isDay();
	return &forecast[index];
}

int8_t WeatherData::minTemp()
{
	return temp[0];
}

int8_t WeatherData::maxTemp()
{
	return temp[1];
}

uint8_t WeatherData::hoursOfPrecipitation()
{
	return forecast[0].hoursOfPrecipitation + forecast[1].hoursOfPrecipitation;
}

float WeatherData::totalLiquid()
{
	return forecast[0].totalLiquid + forecast[1].totalLiquid;
}

float WeatherData::speedWin()
{
	// dia = 0 noche = 1
	uint8_t index = !isDay();
	return forecast[index].win[0];
}

uint8_t WeatherData::precipitationProbability()
{
	return (uint8_t) ((forecast[0].precipitationProbability + forecast[1].precipitationProbability)/ 2);
}

uint8_t WeatherData::cloudCover()
{
	return (uint8_t) ((forecast[0].cloudCover + forecast[1].cloudCover) / 2);
}

/*			Expression			*/

int8_t WeaterExpression::evaluate()
{
/*{
  "minTemp": 5,
  "maxTemp": 18,
  "cloudCover": 95,
  "precipitationProbability": 75,
  "totalLiquid": 4.5,
  "hoursOfPrecipitation": 3,
}*/
	if (model.loadedForescast) {
		WeatherData::Forecast *fore = model.weather.getCurrent();
		int16_t result = fore->cloudCover;
		result += fore->precipitationProbability;
		result += fore->hoursOfPrecipitation * 100;
		result = MIN(result / 3, 100);
		Serial.printf("weater evaluate %d cloud%d precipitation %d%% hoursOfPrecipitation %dh\n", result,
			fore->cloudCover, fore->precipitationProbability, fore->hoursOfPrecipitation);
		return (int8_t)result;
	}
	return -1;
}

bool WeaterExpression::skip()
{
	// cancelamos cuando el viento es mas de 8km/h
	return model.weather.speedWin() > 8;
}

const char * WeaterExpression::getName()
{
	return getStr(Str::weaterInterStr);
}

int8_t SensorExpression::evaluate()
{
	float averageTmp = 0,averageHum = 0;


	for (uint8_t i = 0; i < MODEL_SIZE_LOG; i++)
	{
		SensorData * sensor = model.getLog(i);
		averageTmp += sensor->temperature;
		averageHum += sensor->humidity;
	}

	averageTmp /= MODEL_SIZE_LOG;
	averageHum /= MODEL_SIZE_LOG;

	float result = map(averageTmp,0,30,100,0);
	result += map(averageHum,30,80,0,100);

	result /= 2;
	//Serial.printf("tmp %.0f hum %.0f \n", averageTmp,averageHum);
	Serial.printf("sensor evaluate %.0f tmp %d hum %d \n",result, 
		map(averageTmp, 0, 30, 100, 0), map(averageHum, 30, 80, 0, 100));
	//Serial.printf("tmp1 %d tmp2 %d \n",map(10,0,30,100,0), map(15,0,30,100,0));
	//Serial.printf("tmp3 %d tmp4 %d \n",map(20,0,30,100,0), map(25,0,30,100, 0));

	return  MIN((int8_t)(result), 100);
}


bool SensorExpression::skip()
{
	// cancelamos cuando la temp es menos 5°
	return model.currentSensor.temperature < 5;
}

const char * SensorExpression::getName()
{
	return getStr(Str::sensorInterStr);
}


ListExpression::ListExpression()
{
	enabled = true;
}

int8_t ListExpression::evaluate()
{
	uint16_t result = 0;
	uint8_t c = 0;
	for (uint8_t i = 0; i < count; i++)
	{
		Expression * exp = expressionList[i];
		if (exp->getEnabled() && exp->evaluate() > -1) {
			result += exp->evaluate();
			c++;
		}
	}
	if (c == 0) return -1;
	return (int8_t) (result / c);
}

bool ListExpression::skip()
{
	for (uint8_t i = 0; i < count; i++)
	{
		Expression * exp = expressionList[i];
		if(exp->getEnabled() && exp->skip())
			return true;
	}
	return false;
}

bool ListExpression::add(Expression * exp)
{
	if (count < 20) {
		expressionList[count++] = exp;
		return true;
	}
	return false;
}

Expression * ListExpression::get(uint8_t index)
{
	if (index < count) {
		return expressionList[index];
	}
	return nullptr;
}

bool ListExpression::has(uint8_t index)
{
	return get(index) != nullptr;
}

const char * ListExpression::getName()
{
	return "";
}

void Expression::setEnabled(bool value)
{
	enabled = value;
}

bool Expression::getEnabled()
{
	return enabled;
}

int8_t Disable24Expression::evaluate()
{
	return -1;
}

bool Disable24Expression::skip()
{
	return true;
}

void Disable24Expression::setEnabled(bool value)
{
	if (value == enabled) return;

	Expression::setEnabled(value);
	if (value) {
		if (task != nullptr)
			tasker.remove(task);

		using namespace std::placeholders;
		task = tasker.setTimeout(std::bind(&Disable24Expression::onTimeOut, this, _1), 24 * 60 * 60);
	}
	else {
		if (task != nullptr)
			tasker.remove(task);
		task = nullptr;
	}
}

void Disable24Expression::onTimeOut(Task * current)
{
	setEnabled(false);
}

const char * Disable24Expression::getName()
{
	return getStr(Str::dis24hInterStr);
}

WateringModes::WateringModes()
{
	add(&disable24);
	add(&weater);
	add(&sensor);
	for (uint8_t i = 0; i < 7; i++)
	{
		dayExpression * day = &week[i];
		day->day = i + 1;
		add(day);
	}
}

int8_t dayExpression::evaluate()
{
	return -1;
}

bool dayExpression::skip()
{
	return weekday() == day;
}

const char * dayExpression::getName()
{
	return getDayStr(day);
}
