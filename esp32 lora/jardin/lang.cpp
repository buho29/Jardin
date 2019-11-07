// 
// 
// 

#include "lang.h"

char * getStr(Str id)
{
	strcpy(buffer, (const char*)pgm_read_word(&(string_table[id])));
	return buffer;
}

char * getMonthStr(uint8_t month)
{
	strcpy_P(buffer, (PGM_P)pgm_read_word(&(monthNames_P[month])));
	return buffer;
}

char * getDayStr(uint8_t day)
{
	strcpy_P(buffer, (PGM_P)pgm_read_word(&(dayNames_P[day])));
	return buffer;
}