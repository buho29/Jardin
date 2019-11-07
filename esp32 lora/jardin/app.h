// app.h

#ifndef _MODEL_h
#define _MODEL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>  

#include <TimeLib.h>
#include <ArduinoJson.h>
#include <Adafruit_BME280.h>

#include "lib/Task.h"
#include "lib/AnalogButtons.h"
#include "lib/Components.h"
#include "lib/Flag.h"

#include "lang.h"

#define ON LOW
#define OFF HIGH

#define MODEL_MAX_LISTENERS 50
#define MODEL_SIZE_LOG 24
#define MODEL_MAX_ZONES 10
#define MODEL_MAX_CHAR_NAME_ZONE 20
#define MODEL_MAX_ALARMS 20
#define MODEL_GTM 2*60*60

#define SENSOR_INTERVAL 60000
#define DELAY_MSG_DISPLAY 2000

#define NULLPIN 255

//pins
#define ANALOG_PIN_BUTTONS 36

enum EventType {
	all,
	error,
	conectedWifi,
	loadedForescast,
	loadedTime,
	sensor,
	sensorLog,
	status,
	zoneChanged,
	modesChanged
};

//clase virtual para registrase a eventos en model
class EventChange
{
public:
	virtual void onChange(EventType ev) = 0;
};

enum Status {
	starting,started,standby,watering
};

enum Buttons {
	btUp, btCenter, btDown, btLeft, btRigth
};

struct SensorData
{
	float temperature;
	float pressure;
	uint8_t humidity;
	uint32_t time;
};

struct ZoneData
{
	uint32_t start;
	uint16_t duration;
	uint8_t alarmSize;
};

//data meteo
struct WeatherData {
	struct Forecast {
		int8_t icon;
		char phrase[100];
		float win[2];// "Speed","Direction"
		uint8_t hoursOfPrecipitation;
		uint8_t precipitationProbability;
		uint8_t cloudCover;
		float totalLiquid;
	} forecast[2];//day/nigth

	time_t time;
	int8_t temp[2]; // min max
	time_t sun[2]; // inicio final

	bool isDay();
	Forecast * getCurrent();
	int8_t minTemp();
	int8_t maxTemp();

	uint8_t hoursOfPrecipitation();
	uint8_t precipitationProbability();
	uint8_t cloudCover();
	float totalLiquid();
	float speedWin();
};

/*	data Alarm
zonas : [
	[alarma1, alarma2, alarma3 ],// zona 1
	[alarma4, alarma5 ] // zona 2
]
*/
struct Alarm {
	uint8_t pin;
	time_t init;
	time_t end;
	void copy(Alarm * alarm);
	void setNull();
};
// dato que se guardara en la eeprom
struct Config {
	time_t time;
	char ssid[32] = "Movistar_1664";
	char password[64] = "EGYDRNA6H4Q";
	char cityID[10] = "1451030";// tintores
	char cityName[20] = "Tintores";
	Alarm alarms[MODEL_MAX_ZONES][MODEL_MAX_ALARMS];
	char zoneNames[MODEL_MAX_ZONES][MODEL_MAX_CHAR_NAME_ZONE+1];
	SensorData sensorLog[MODEL_SIZE_LOG];
	bool modes[20];
};

/*			Modes			*/
/*			Expression			*/

//interface Interpreter
class Expression
{
protected:
	bool enabled = false;
public:
	// evalua entre 0 a 100 si se riega
	// evaluate() < 50 se riega
	virtual int8_t evaluate() = 0;
	// si true se salta la evaluacion y no se riega
	virtual bool skip() = 0;
	virtual void setEnabled(bool value);
	virtual bool getEnabled();
	virtual const char * getName() = 0;
};

//desactiva el riego 24h  
class Disable24Expression :public Expression
{
private:
	Task * task;
public:
	virtual int8_t evaluate();
	virtual bool skip();
	virtual void setEnabled(bool value);
	void onTimeOut(Task * current);
	virtual const char * getName();
};

//predicion meteo
class WeaterExpression :public Expression
{
public:
	virtual int8_t evaluate();
	virtual bool skip();
	virtual const char * getName();
};

//predicion sensores
class SensorExpression :public Expression
{
public:
	virtual int8_t evaluate();
	virtual bool skip();
	virtual const char * getName();
};

//deactiva el riego un dia de la semana
class dayExpression :public Expression
{
public:
	uint8_t day = 0;
	virtual int8_t evaluate();
	virtual bool skip();
	virtual const char * getName();
};

//lista de interpreters que se comporta como uno solo
class ListExpression :public Expression
{
protected:
	Expression * expressionList[20];
public:
	ListExpression();
	virtual int8_t evaluate();
	virtual bool skip();
	virtual const char * getName();
	bool add(Expression * interpreter);
	Expression * get(uint8_t index);
	bool has(uint8_t index);
	uint8_t count = 0;
};
// root
class WateringModes : public ListExpression
{
public:
	WateringModes();
private:
	Disable24Expression disable24;
	WeaterExpression weater;
	SensorExpression sensor;
	dayExpression week[7];
};

/*			Model			*/

class Model
{
private:
	WiFiClient client;
	WebServer server;

	//Meteo service
	// http://dataservice.accuweather.com/forecasts/v1/daily/1day/1451030?apikey=q329xaaTojo0koLv6A3uFgh3dQLgp6em%20&language=es-ES&details=true&metric=true
	// http://dataservice.accuweather.com/forecasts/v1/daily/1day/1451030?apikey=SoOCQzMkcUaK83HlGPMz3rxaxlUsEr1a%20&language=es-ES&details=true&metric=true
	const char* host = "dataservice.accuweather.com";
	///url
	const char * url1 = "/forecasts/v1/daily/1day/";
	//const char * url2 = "?apikey=q329xaaTojo0koLv6A3uFgh3dQLgp6em%20&language=es-ES&details=true&metric=true";
	const char * url2 = "?apikey=SoOCQzMkcUaK83HlGPMz3rxaxlUsEr1a%20&language=es-ES&details=true&metric=true";

	//sensor
	Adafruit_BME280 bme; // I2C

	//internet
	void load();

	void connectWifi();
	void enableSoftAP();
	void loadForecast();
	bool parseForescast();
	//event
	void dispachEvent(EventType ev);
	EventChange * listeners[MODEL_MAX_LISTENERS];
	EventType types[MODEL_MAX_LISTENERS];
	uint8_t listenerCount = 0;
	// data
	Config config;
	void readEprom();
	void writeEprom();
	// 
	void loadDefaultConfig();
	//
	ZoneData zones[MODEL_MAX_ZONES];

	void loadLocalTime();
	// cuando guardamos el log
	void saveLoger(Task * t);
	// el tiempo para volver a intentarlo
	const uint8_t retryTime = 60;
	// en caso de error 
	void retryLater();
	// para setTimeout
	void onTimeOut(Task * t);
	//para actualizar cuando es de noche/dia
	void onSunChanged(Task * t);
	Task * sunTask = nullptr;
	void updateSunTask();

	void updateSensor();
	void initLogSensor();
	void resetLogSensor();
	void onWater(Task * t);

	void reloadTasks(uint8_t zone);
	Task * tasks[MODEL_MAX_ZONES][MODEL_MAX_ALARMS];

	void onUpdateData(Task * t);


	//grifos y pins
	bool openTap(uint8_t pin,bool val);
	uint8_t pins[4] = {27,14,21,17};
	bool isPinOn[4] = {false,false,false,false};

	//pause variable
	int16_t elapsedPausedTask = 0;
	int32_t pausedTime = 0;

	void setZoneName(uint8_t zone,char * name);

	void updateZone(uint8_t zone);

	void updateTasks();
	void updateTasks(uint8_t zone);

	//
	void readModes();
	void writeModes();

	//servidor
	void startWebServer();
	void handleConfig();
	void handleNotFound();
	void handleRoot();
	void handleSensorLog();
	void handleSensor();
	void handleTaps();
	void handleStatus();
	void handleZones();
	void handleAlarms();
	void handleEditZone();
	void handleEditAlarm();
	void handleWeather();
	void handleModes();
 public:
	 Model() {};
	void begin();
	void printZones();
	void printTask(Task * t);
	void printAlarms(uint8_t zone);
	void update();
	void addEvent(EventType type, EventChange * callBack);

	bool connectedWifi = false;
	bool loadedTime = false;

	Status status;
	char msgError[100] = "";
	char msgStatus[100] = "";

	int16_t currentZone = -1,currentAlarm = -1;
	uint8_t currentPin;
	int32_t lastTimeZoneChanged = 0;
	// grifos
	uint8_t getPin(uint8_t index);
	bool getIsTapOn(uint8_t pin);
	const uint8_t countPins = 4;
	int16_t getIndexPin(uint8_t pin);

	// esta abierto algun grifo ?
	bool isWatering();
	// esta pausado ?
	bool isPaused();

	// estamos regando manualmente una zona ?
	bool isManualZoneWater = false;
	// estamos regando manualmente ?
	bool isManualWater = false;

	//data Zone
	char * getZoneName(uint8_t zone);
	bool hasZone(uint8_t zone);
	ZoneData * getZone(uint8_t zone);

	//zone
	bool editZone(uint8_t zone, char * name);
	int8_t addZone(char * name);
	bool removeZone(uint8_t zone);

	//alarmas
	int8_t addAlarm(uint8_t zone, uint8_t pin, uint32_t time,uint16_t duration );
	int8_t addAlarm(uint8_t zone, uint8_t pin, uint16_t duration, uint8_t h, uint8_t m, uint8_t s = 0);

	bool editAlarm(uint8_t zone, uint8_t index, uint8_t pin, uint32_t time,uint16_t duration );
	bool removeAlarm(uint8_t zone, uint8_t index);
	Alarm * getAlarm(uint8_t zone, uint8_t index);
	bool hasAlarm(uint8_t zone, uint8_t index);

	// devuelve la proxima zona q se va regar
	uint8_t getNextZone();

	//riego zona manual
	bool waterZone(uint8_t zone);
	//parar el riego de la zona actual
	bool stopWaterZone();
	bool pauseWaterZone();

	bool switchTap(uint8_t pin);

	SensorData * getLog(uint8_t index);
	SensorData currentSensor;

	WeatherData weather;
	bool loadedForescast = false;
	bool isDay();
	char * getCityName();

	bool canWatering();
	bool switchMode(uint8_t index);
	WateringModes modes;
	int32_t lastTimeModesChanged = 0;

	//info wifi
	int8_t rssi;
	double getDistanceRSSI();

	uint16_t getElapsedAlarm();
	uint16_t getAlarmDuration(uint8_t zone, uint8_t index);
	uint8_t getSymbol(uint8_t icon);
};

extern Model model;

/*			Vistas (screens)			*/

class ZoneView : public View
{
public:
	ZoneView() :View() {};
	virtual void begin();
	virtual bool acceptClickButton(uint8_t id);
	void setZone(uint8_t zoneID, ZoneData * zone);
	void updateZone();
	virtual void measure();
	virtual char * nameClass();
private:

	Color c1 = Color(0), c2;
	Rectangle rect = Rectangle(Dimension{ 0,0,128,14 }, true);
	Text txt1 = Text(Dimension{ 0,-1,128,-1 }, Align::ACenter, Font::medium);
	Text txt2 = Text(Dimension{ 0,14,128,-1 }, Align::ACenter, Font::large);
	Text txt3 = Text(Dimension{ 0,30,128,-1 }, "120 min 2/3", Align::ACenter, Font::small);
	Button bt1 = Button(Dimension{ 0,46,62,-1 });
	Button bt2 = Button(Dimension{ 66,46,60,-1 });
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&rect, &c1, &txt1, &c2,// titulo
		&txt2,&txt3,//14:00:00 120min 2/3
		&bt1, &bt2// pausa / regar
	};
	uint8_t zoneID;
	ZoneData * zone;
};

class WaterView : public View, public EventChange
{
public:
	WaterView() :View() {};
	virtual void begin();

	virtual char * nameClass();
	virtual void onChange(EventType e);
	virtual bool acceptClickButton(uint8_t id);

private:

	Text txt = Text(Dimension{ 0,32,128,-1 }, Align::ACenter, Font::medium);

	Color c1 = Color(0), c2;
	Rectangle rect = Rectangle(Dimension{ 0,0,128,14 }, true);
	Text txt1 = Text(Dimension{ 0,-2,128,-1 }, "Zonas", Align::ACenter, Font::medium);
	
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&rect, &c1, &txt1, &c2,&txt
	};
	ZoneView zones[MODEL_MAX_ZONES];
};

class TimeView : public View
{
public:
	TimeView() :View() {};
	virtual void begin();
	virtual void measure();

	virtual char * nameClass();
private:

	Color c1 = Color(0), c2;
	Rectangle rect = Rectangle(Dimension{ 0,0,128,14 }, true);
	Text txt1 = Text(Dimension{ 0,-2,128,-1 }, "Lunes", Align::ACenter, Font::medium);
	Text txt2 = Text(Dimension{ 0,18,128,-1 }, "19:00:00", Align::ACenter, Font::veryLarge);
	Text txtS = Text(Dimension{ 0,50,128,-1 }, Align::ACenter);
	Ui * uis[CONTAINER_MAX_SIZE] = {
		 &rect, &c1, &txt1, &c2, &txt2, &txtS
	};
};

class ServerView : public View, public EventChange
{
public:
	ServerView() :View() {};
	virtual void begin();
	virtual void measure();

	virtual char * nameClass();
	virtual void onChange(EventType e);
	virtual bool acceptClickButton(uint8_t id);
private:

	Color c1 = Color(0), c2;
	Rectangle rect = Rectangle(Dimension{ 0,0,128,14 }, true);
	Text txt1 = Text(Dimension{ 0,-2,128,-1 }, "Server", Align::ACenter, Font::medium);
	Text txt2 = Text(Dimension{ 0,18,128,-1 }, "1.1.1.1", Align::ACenter, Font::medium);
	Text txt3 = Text(Dimension{ 0,18*2,128,-1 }, "1.1.1.1", Align::ACenter, Font::medium);
	Text txtS = Text(Dimension{ 0,50,128,-1 }, Align::ACenter, Font::medium);
	Ui * uis[CONTAINER_MAX_SIZE] = {
		 &rect, &c1, &txt1, &c2, &txt2,&txt3, &txtS
	};
};

class MessageView : public View
{
public:
	MessageView() :View() {};
	void print(char * msg);
	virtual void begin();
	virtual void draw();

	virtual char * nameClass();
private:
	Text txt1 = Text(Dimension{ 0,0,128,-1 }, Align::ACenter);
	Text txt2 = Text(Dimension{ 0,16,128,-1 }, Align::ACenter);
	Text txt3 = Text(Dimension{ 0,16 * 2,128,-1 }, Align::ACenter);
	Text * uis[CONTAINER_MAX_SIZE] = {
		 &txt1, &txt2, &txt3
	};
};

class SensorLog : public View, public EventChange
{
public:
	SensorLog() :View() {};
	SensorLog(uint8_t column);
	virtual void begin();
	virtual char * nameClass();
	virtual void onChange(EventType e);
private:
	Text txt1 = Text(Dimension{ 0,0,-1,-1 }, "+8");
	Text txt2 = Text(Dimension{ 0,32 - 8,-1,-1 }, "0");
	Text txt3 = Text(Dimension{ 0,64 - 12,-1,-1 }, "-8");
	Text txt4 = Text(Dimension{ 30,0,128 - 30,-1 }, "Title", Align::ACenter, Font::verySmall);
	GraphXY graf = GraphXY(Dimension{ 30,0,128 - 30,64 });
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&txt2, &txt1, &txt3, &txt4, &graf
	};
	float data[24];
	float maxValue, minValue, averageValue;
	uint8_t sensorType = 0;
	float getValue(SensorData * sensor);
	char * getTitle();
};

class SensorView : public View, public EventChange
{
public:
	SensorView() :View() {};
	virtual void begin();
	virtual char * nameClass();
	virtual bool acceptClickButton(uint8_t id);
	virtual void onChange(EventType e);
private:
	Glyph icon = Glyph(Dimension{ 14,32,-1 });
	//temperatura
	RectangleR rec1 = RectangleR(Dimension{ 64,0,64,32 }, 5);
	Text txt1 = Text(Dimension{ 64,2,64,-1 }, "Temperatura", ACenter, Font::verySmall);
	Text temperature = Text(Dimension{ 64,14,64,-1 }, ACenter);
	//presion
	RectangleR rec2 = RectangleR(Dimension{ 64,36,64,28 }, 5);
	Text txt3 = Text(Dimension{ 64,36,64,-1 }, "Presion", ACenter, Font::verySmall);
	Text pressure = Text(Dimension{ 64,46,64,-1 }, ACenter);
	//humedad
	RectangleR rec3 = RectangleR(Dimension{ 0,36,58,28 }, 5);
	Text txt5 = Text(Dimension{ 0,36,64,-1 }, "Humedad", ACenter, Font::verySmall);
	Text humidity = Text(Dimension{ 0,46,64,-1 }, ACenter);

	Ui * uis[CONTAINER_MAX_SIZE] = {
		&icon,
		&rec1 ,&txt1,&temperature,
		&rec2 ,&txt3,&pressure,
		&rec3 ,&txt5,&humidity
	};
	SensorLog sensorLogTemp = SensorLog(0);
	SensorLog sensorLogPress = SensorLog(1);
	SensorLog sensorLogHum = SensorLog(2);

};

class ForecastView : public View, public EventChange
{
public:
	ForecastView() :View() {};
	virtual char * nameClass();
	virtual void begin();
	virtual void onChange(EventType e);
private:
	Color c1 = Color(0), c2;
	Rectangle rect = Rectangle(Dimension{ 0,0,128,14 }, true);
	Glyph icon = Glyph(Dimension{ 4,50,-1,-1 });
	Text txt1 = Text(Dimension{ 0,-1,128,-1 }, "pueblo", Align::ACenter, Font::medium);
	Text txt2 = Text(Dimension{ 40,14,85,-1 }, "Max   Min", Align::ACenter, Font::verySmall);
	Text temperature = Text(Dimension{ 50,22,85,-1 }, "13/19°c", Align::ACenter, Font::veryLarge);
	TextScroll ts = TextScroll(Dimension{ 0,62,128,-1 });
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&rect, &c1, &txt1, &c2,
		&icon, &txt2, &temperature,
		&ts
	};
};

class TapsView : public View, public EventChange
{
public:
	TapsView() :View() {};
	virtual void begin();

	virtual char * nameClass();
	virtual void onChange(EventType e);
	virtual bool acceptClickButton(uint8_t id);
private:
	List list = List(Dimension{ 0,0,128,64 });
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&list
	};
};

class ModesView : public View, public EventChange
{
public:
	ModesView() :View() {};
	virtual void begin();

	virtual char * nameClass();
	virtual void onChange(EventType e);
	virtual bool acceptClickButton(uint8_t id);
private:
	List list = List(Dimension{ 0,0,128,64 });
	Ui * uis[CONTAINER_MAX_SIZE] = {
		&list
	};
};

/*			controler			*/

class ViewControler :public EventClick, public EventChange
{
public:
	ViewControler() {}
	void begin();
	void update();
	void onChange(EventType e);
	void setCurrent(View * view);
private:
	ManagerView managerView;
	MessageView messageView;
	ForecastView forecastView;
	SensorView sensorView;
	TimeView timeView;
	ServerView serverView;
	WaterView waterView;
	TapsView tapsView;
	ModesView modesView;
	void onClick(uint8_t id, bool isHeldDown);
	uint32_t lastTimeInput;
	uint32_t delayScroll = 60000;//1min
	uint32_t delayDisplayOff = 300000;//5min
};

extern ViewControler controler;

#endif