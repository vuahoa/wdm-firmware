/** @brief define Constants, Prototypes for the WIFI config module.
 *  @date
 *      - 2019_05_05: Create.
 * 
*/
#ifndef _HTTPD_H_
#define _HTTPD_H_

#include <ESP8266WiFi.h>
#include "wifi_inf.h"

class httpd
{
    public:
		static void init(void);
        /* Start a WebServer & wait for user settings */
        static uint32_t get_config(ROM_SETTINGS_t *settings);
	
	private:
		static String htmlDefault();
		static String htmlCfg(bool result);
		static void parse_percent(char *p_str);
		static uint8_t get_param(const char *req, const char *pName, char *val, char val_sz);
};

#endif
