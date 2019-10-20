/** @brief define Constants, Prototypes for the WIFI config module.
 *  @date
 *      - 2019_05_05: Create.
 * 
*/
#ifndef _WIFI_INF_H_
#define _WIFI_INF_H_

#include <ESP8266WiFi.h>

/* Settings parameter limits */
#define CFG_SSID_SZ             32
#define CFG_PASSWORD_SZ         32
#define CFG_SERVER_SZ           64
#define CFG_SECURITY_SZ         32

/* ROM memory settings parameters */
struct ROM_SETTINGS_t {
	uint32_t magic_number;
    char ssid[CFG_SSID_SZ];
    char password[CFG_PASSWORD_SZ];
    char server_addr[CFG_SERVER_SZ];
	uint32_t server_port;
    char security[CFG_SECURITY_SZ];
	int32_t timezone;
};

/* WIFI Status */
struct WIFI_STATUS_t {
	uint8_t mode;
	uint8_t is_connected;
	uint32_t local_ip;
    uint32_t gateway;
    uint32_t subnet;	
};

class wifi_inf
{
    public:
        /* Start wifi connection: if having valid settings -> start in Station mode, 
		if not -> start in Access Point mode */
        static void start();

        /* Get current settings */
        static const ROM_SETTINGS_t *get_settings();
        
		/* Implement a simple HTTP server to receive settings when run in AP mode */
		static void manager();
		
		/* Reset all settings to default values */
		static void factory_reset();
	
	private:
		static void load_rom_settings();
		static void store_rom_settings();
};

#endif
