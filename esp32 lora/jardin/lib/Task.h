// tools.h

#ifndef _TASK_h
#define _TASK_h


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//#define __AVR__

#include <TimeLib.h>

#ifndef __AVR__
#include <functional>
#endif // __AVR__

class Task;

#ifndef __AVR__
typedef std::function<void(Task *)> p_callbackTask;
#else
typedef void(*p_callbackTask)(Task * task);
#endif // __AVR__

#define TASK_MAX_SIZE 100
#define TASK_INTERVAL 100
#define TASK_TICKS_24H 86400 //% 24h*60min*60s

class Task
{
	friend class Tasker;
public:
	Task() {};
	void addEvent(p_callbackTask callback);
	uint8_t id = 0;
	uint32_t start = 0;
	uint32_t stop = 0;
	bool runing = false;
	bool enabled = true; 
private:
	p_callbackTask callback;
	void setup(uint32_t dateStart, uint32_t dateStop,
		p_callbackTask callback);
	int8_t mode = 0;// 1 setTimout | 0 ignora | -1 reusar | 2 setInterval
};


class Tasker
{

 public:
	 Tasker() {};

	Task *  setTimeout(p_callbackTask callback, uint16_t s);
	Task *  setInterval(p_callbackTask callback, uint16_t s);
	Task *  setInterval(p_callbackTask callback, uint8_t h, uint8_t m, uint8_t s = 0);

	Task * add(uint32_t dateStart, uint32_t dateStop,
		p_callbackTask callback);

	//marca task para su reutilizacion
	void remove(Task * task);

	// Devuelve un puntero de la primera ocurencia de id
	Task* get(uint8_t id);
	
	// a meter en un loop
	void check();

	//devuelve los tick de el dia (24h)
	static uint32_t getTickTime(uint16_t h, uint16_t m, uint16_t s);
	//devuelve los tick de el tiempo actual (24h)
	static uint32_t getTickNow();
	//convierte en string los ticks
	static void tickTimeToChar(char * result, uint32_t date);

	static uint32_t howTimeLeft(uint32_t date);
	static uint32_t getDuration(uint32_t init, uint32_t end);
	
private:
	Task tasks[TASK_MAX_SIZE];
	uint8_t taskCount;
	uint32_t time;
	void startTask(Task *t);
	void stopTask(Task *t);
};


extern Tasker tasker;
#endif

