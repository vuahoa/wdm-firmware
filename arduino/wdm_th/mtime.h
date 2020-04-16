/**	@brief define Constants, Types & Prototypes for the TIME module.
		@date
			- 2019_09_25: Create.
*/
#ifndef _MTIME_H_
#define _MTIME_H_

#include "Arduino.h"

class mtime
{
	public:
        static uint32_t ticker_cb();
		static void set_local_unix(uint32_t unix);
		
		static uint32_t get_local_unix();        		
		static uint8_t get_weekday();
		static uint16_t get_minute_in_day();
};

#endif
