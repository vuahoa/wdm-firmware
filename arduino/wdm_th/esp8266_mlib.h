/**	@brief define Constants, Types & Prototypes for TSU Library module.
		@date
			- 2018_07_19: Create.
*/
#ifndef _TSU_LIB_H_
#define _TSU_LIB_H_

#include "Arduino.h"

// LED pin:
#define LED_BUILTIN 				2
#define LED_1_PIN					14

// Reboot cause params:
#define PWR_BOOT_POR      			0x00000000
#define PWR_BOOT_SOFT     			0x01010101
#define PWR_BOOT_SLEEP    			0x02020202

/* RTC Magic number */
#define RTC_MAGIC_VALUE             0x41424344

class esp8266_mlib
{
	public:
		esp8266_mlib();
		
		static void init();
        static const uint8_t *get_id();
        static const char *get_id_str();
		
		static uint32_t get_boot_cause();
		static void enter_sleep(uint32_t us);
		static void soft_reboot();
		
		static uint32_t load_file(const char *file_name, char *str, uint8_t str_sz);
		static bool save_file(const char *file_name, const char *content);

		static uint32_t buf_to_u32(uint8_t buf[]);
		static void u32_to_buf(uint32_t u32, uint8_t buf[]);	

		static uint16_t buf_to_u16(uint8_t buf[]);
		static void u16_to_buf(uint16_t u16, uint8_t buf[]);
};

#endif
