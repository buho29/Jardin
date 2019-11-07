// 
// 
// 
#include "Task.h"



void Task::setup(uint32_t dateStart, uint32_t dateStop, p_callbackTask callback)
{
	this->start = dateStart;
	this->stop = dateStop;
	this->callback = callback;
}

void Task::addEvent(p_callbackTask callback)
{
	this->callback = callback;
}


Task * Tasker::setTimeout(p_callbackTask callback,uint16_t s )
{
	uint32_t dateStart = getTickTime(hour(), minute(), second() + s);

	Task * t = add(dateStart, dateStart + s, callback);
	t->mode = 1;// lo marcamos para q se ejecute solo una vez

	return t;
}

Task * Tasker::setInterval(p_callbackTask callback, uint16_t s)
{
	uint32_t dateStart = getTickTime(hour(), minute(), second() + s);

	Task * t = add(dateStart, dateStart + s, callback);
	t->mode = 2;// lo marcamos para q se repita

	return t;
}

Task * Tasker::setInterval(p_callbackTask callback, uint8_t h, uint8_t m, uint8_t s)
{
	uint32_t dateStart = getTickTime(h, m, s);

	Task * t = add(dateStart, dateStart+1, callback);
	t->mode = 2;// lo marcamos para q se repita

	return t;
}


Task * Tasker::add(uint32_t dateStart, uint32_t dateStop,
	p_callbackTask callback)
{

	//buscar algun "borrado"
	Task *t = NULL;
	for (uint8_t i = 0; i < taskCount; i++)
	{
		t = &tasks[i];
		if (t->mode == -1)
			break;
		t = NULL;
	}

	// si lo encontramos lo reiniciamos
	if (t != NULL) {
		t->enabled = true;
		t->runing = false;
		t->mode = 0;
		t->setup(
			dateStart,	// inicio 
			dateStop,	// fin 
			callback	// callback
		);
		//Serial.printf("reciclado id %d!\n",t->id);
		return t;
	}
	// si no, se crea uno si se puede
	if (taskCount < TASK_MAX_SIZE) {
		t = &tasks[taskCount];
		t->setup(
			dateStart,	// inicio 
			dateStop,	// fin 
			callback	// callback
		);
		tasks[taskCount].id = taskCount;
		taskCount++;
		return t;
	}

	Serial.printf("excceded task ,max size is %d", TASK_MAX_SIZE);
	return NULL;
}

void Tasker::remove(Task * task)
{
	if (task == nullptr) {
		Serial.println("Remove(task) is null");
		return;
	}
	task->mode = -1;
}

Task * Tasker::get(uint8_t id)
{
	for (uint8_t i = 0; i < taskCount; i++)
	{
		Task *t = &tasks[i];
		if (t->id == id) return t;
	}
	return nullptr;//TODO 
}

void Tasker::startTask(Task * t)
{
	t->runing = true;
	if (t->mode == 1)// es un setTimeOut
		t->mode = -1;// lo reciclamos

	t->callback(t);
	
	if (t->mode == 2) {//es un setInterval

		// recuperamos el interval de tiempo
		uint16_t ms = t->stop - t->start;

		// si es diario (24h) nos salimos
		if (ms == 1) return;
		
		// lo detenemos
		t->runing = false;

		t->start = getTickTime(hour(), minute(), second() + ms);
		//todo
		t->stop = t->start + ms;
	}
}

void Tasker::stopTask(Task * t)
{
	t->runing = false;
	if (t->mode == 0)
		t->callback(t);
}

void Tasker::check()
{
	if (millis() - time > TASK_INTERVAL) {
		time = millis();
		uint32_t current = Tasker::getTickNow();

		for (uint8_t i = 0; i < taskCount; i++)
		{
			Task * t = &tasks[i];

			//12:00 -> 12:01
			if ( t->mode >= 0 && t->start < t->stop ) {
				
				if ( current >= t->start && current < t->stop
					&& !t->runing ){

					startTask(t);
				}
				else if ( current > t->start && current >= t->stop
					&& t->runing ){

					stopTask(t);
				}
			}
			//23:59 -> 0:02
			else if ( t->mode == 0 && t->start > t->stop ) {
				

				if ( current > t->start && (current > t->stop)
					&& !t->runing ){

					startTask(t);
				}
				else if ( current < t->start && current > t->stop
					&& t->runing){

					stopTask(t);
				}
			}
		}
	}
	
}

// static

uint32_t Tasker::getTickTime(uint16_t h, uint16_t m, uint16_t s)
{
	return (h * 3600 + m * 60 + s)% TASK_TICKS_24H; 
}

uint32_t Tasker::getTickNow()
{
	return now() % TASK_TICKS_24H;
}

void Tasker::tickTimeToChar(char* result, uint32_t date)
{

	uint8_t h, m, s;
	uint32_t time = date;

	s = time % 60;
	time /= 60;
	m = time % 60;
	time /= 60;
	h = time % 24;

	snprintf(result, 9, "%02d:%02d:%02d",h,m,s);
}

uint32_t Tasker::howTimeLeft(uint32_t date)
{
	uint32_t _now = Tasker::getTickNow();
	return getDuration(_now, date);
}

uint32_t Tasker::getDuration(uint32_t init, uint32_t end)
{
	if (init > end)
		return TASK_TICKS_24H - init + end;
	else return end - init;
}


Tasker tasker;

