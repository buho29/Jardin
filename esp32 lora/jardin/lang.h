// lang.h

#ifndef _LANG_h
	#define _LANG_h

#include "arduino.h"

//#include <pgmspace.h>

#define MAX_STRING_LEN 80
#define PROGMEM
#define pgm_read_word(addr) (*(const unsigned char **)(addr))

static char buffer[MAX_STRING_LEN + 1];

enum Str {
	errorSensor = 0,
	conWifi = 1,
	conWifi1 = 2,
	errorWifi = 3,
	updatedTime = 4,
	errorTime = 5,
	updatedFore = 6,
	errorFore = 7,
	wateringStr = 8,
	notWatering = 9,
	formatFore = 10,
	waterStr = 11,
	cancelStr = 12,
	tapStr = 13,
	temperature = 14,
	pressure =15,
	humidity=16,
	pauseStr=17,
	resumeStr=18,
	dis24hInterStr=19,
	weaterInterStr=20,
	sensorInterStr=21,
	zonesStr=22
};

const char _errorSensor[] PROGMEM		= "Error con|sensor bme280";
const char _conWifi[] PROGMEM			= "Conectando a|";
const char _conWifi1[] PROGMEM			= "Conectado a|";
const char _errorWifi[] PROGMEM			= "Error Wifi";
const char _updatedTime[] PROGMEM		= "Hora actualizada|";
const char _errorTime[] PROGMEM			= "Error actualizando|la hora";
const char _updatedFore[] PROGMEM		= "Meteo actualizada";
const char _errorFore[] PROGMEM			= "Error descargando|Meteo";
const char _wateringStr[] PROGMEM		= "Hoy se riega";
const char _notWatering[] PROGMEM		= "Hoy no se riega";
const char _formatFore[] PROGMEM		= "Viento %.1fkm/h- Nubosidad %d%% - Precipitacion %d%%/%.1fmm/%dh - %s ";
const char _waterStr[] PROGMEM			= "Regar";
const char _cancelStr[] PROGMEM			= "Cancelar";
const char _tapStr[] PROGMEM			= "Grifo";
const char _temperatureStr[] PROGMEM	= "Temperarura";
const char _pressureStr[] PROGMEM		= "Presion";
const char _humidityStr[] PROGMEM		= "Humedad";
const char _pauseStr[] PROGMEM			= "Pausar";
const char _resumeStr[] PROGMEM			= "Reanudar";
const char _dis24hInterStr[] PROGMEM	= "Desactivar 24h";
const char _weaterInterStr[] PROGMEM	= "Predicion Meteo";
const char _sensorInterStr[] PROGMEM	= "Predicion Sensor";
const char _zonesStr[] PROGMEM			= "Zonas";

const char* const string_table[] PROGMEM =
{
	_errorSensor,_conWifi,_conWifi1,_errorWifi,_updatedTime,_errorTime,_updatedFore,
	_errorFore,_wateringStr,_notWatering,_formatFore,_waterStr,_cancelStr,_tapStr,
	_temperatureStr,_pressureStr,_humidityStr,_pauseStr,_resumeStr,
	_dis24hInterStr,_weaterInterStr,_sensorInterStr,_zonesStr
};


// date month

const char monthStr0[] PROGMEM = "";
const char monthStr1[] PROGMEM = "Enero";
const char monthStr2[] PROGMEM = "Febrero";
const char monthStr3[] PROGMEM = "Marzo";
const char monthStr4[] PROGMEM = "Abril";
const char monthStr5[] PROGMEM = "Mayo";
const char monthStr6[] PROGMEM = "Junio";
const char monthStr7[] PROGMEM = "Julio";
const char monthStr8[] PROGMEM = "Agosto";
const char monthStr9[] PROGMEM = "Septiembre";
const char monthStr10[] PROGMEM = "Octubre";
const char monthStr11[] PROGMEM = "Noviembre";
const char monthStr12[] PROGMEM = "Deciembre";

const PROGMEM char * const PROGMEM monthNames_P[] =
{
	monthStr0,monthStr1,monthStr2,monthStr3,monthStr4,monthStr5,monthStr6,
	monthStr7,monthStr8,monthStr9,monthStr10,monthStr11,monthStr12
};

//date day

const char dayStr0[] PROGMEM = "Err";
const char dayStr1[] PROGMEM = "Domingo";
const char dayStr2[] PROGMEM = "Lunes";
const char dayStr3[] PROGMEM = "Martes";
const char dayStr4[] PROGMEM = "Miercoles";
const char dayStr5[] PROGMEM = "Jueves";
const char dayStr6[] PROGMEM = "Viernes";
const char dayStr7[] PROGMEM = "Sabado";

const PROGMEM char * const PROGMEM dayNames_P[] =
{
   dayStr0,dayStr1,dayStr2,dayStr3,dayStr4,dayStr5,dayStr6,dayStr7
};

extern char * getStr(Str id);
extern char * getMonthStr(uint8_t month);
extern char * getDayStr(uint8_t day);

#endif

