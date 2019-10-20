/**	@brief implement TIME functions.
	  @date
		- 2019_09_25: Create.
*/
#include "Arduino.h"
#include "mtime.h"

//#define DB      Serial.printf
#ifndef DB
  #define DB
#endif

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
static uint32_t g_localtime_unix;

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
uint32_t mtime::ticker_cb() {
	DB("\r\n%s: now=%u", __FUNCTION__, g_localtime_unix);
	// Only update after the time is updated:
	if (g_localtime_unix != 0U) {
		g_localtime_unix++;
	}
	DB(" -> now=%u", g_localtime_unix);
	return g_localtime_unix;
}

void mtime::set_local_unix(uint32_t unix)
{
	DB("\r\n%s: new_time=%u", __FUNCTION__, unix);
	g_localtime_unix = unix;			
}
		
uint32_t mtime::get_local_unix()
{
	return g_localtime_unix;	
}

/*	Get Weekday of today: 0=Monday, 1=Tuesday,...,6=Sunday
*/
uint8_t mtime::get_weekday()
{
	uint32_t d = g_localtime_unix / 86400;
	DB("\r\n%s: t=%u -> wday=%u", __FUNCTION__, g_localtime_unix, (d + 3) % 7);
	return (d + 3) % 7; // 1/1/1970 @ Thursday (wday=3)
}

uint16_t mtime::get_minute_in_day()
{
	uint32_t min = g_localtime_unix / 60;
	
	return min % 1440;
}