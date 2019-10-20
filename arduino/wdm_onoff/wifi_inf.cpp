#include "Arduino.h"
#include "FS.h"
#include <ESP8266WiFi.h>
#include "esp8266_mlib.h"
#include "httpd.h"
#include "wifi_inf.h"


#define DB      Serial.printf
#ifndef DB
  #define DB
#endif

///////////////////////////////////////LOCAL CONSTANTS/////////////////////////////////////////////
/* WIFI ROM settings file name */
const char *ROM_SETTINGS_FILE_NAME = "/wdm_cfg.txt";

/* ROM setting size */
#define ROM_SETTINGS_SIZE				200

/* Default Password in AP mode */
const char *WIFI_PASSWORD_DEFAULT = "wdm-open";

/* NVM magic number */
#define NVM_MAGIC_NUMBER		0x41424344

/* WIFI modes */

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
/* Default DNS servers */
static IPAddress g_dns1(8,8,8,8);
static IPAddress g_dns2(208,67,222,222);

/* ROM settings */
static struct ROM_SETTINGS_t g_rom_settings;

/* WIFI status */
static struct WIFI_STATUS_t g_wifi_status;

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void wifi_inf::start()
{
    uint8_t mac_addr[8];
    char ssid[32];

	// Load settings:
    WiFi.macAddress(mac_addr);    
	load_rom_settings();
    
    DB(" -> magic=%08Xh, ssid=%s, pwd=%s, server=%s:%u, sec=%s, tz=%d", g_rom_settings.magic_number,
        g_rom_settings.ssid, g_rom_settings.password, g_rom_settings.server_addr, 
        g_rom_settings.server_port, g_rom_settings.security, g_rom_settings.timezone);
        
	if (g_rom_settings.magic_number != NVM_MAGIC_NUMBER) {
		g_wifi_status.mode = WIFI_AP;
		sprintf(ssid, "wsm-%02x%02x%02x%02x%02x%02x",
			mac_addr[0], mac_addr[1],mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		DB("\r\n -> start wifi in AP mode: ssid=%s", ssid);

        digitalWrite(LED_BUILTIN, LOW);
		WiFi.softAP(ssid, WIFI_PASSWORD_DEFAULT);
		httpd::init();
	} else {
		if ((g_rom_settings.timezone < -24 * 60) || (g_rom_settings.timezone > 24 * 60)) {
			DB(" -> invalid timezone -> use default value");
			g_rom_settings.timezone = 7 * 60;			
		}
		
		g_wifi_status.mode = WIFI_STA;
		DB("\r\n -> start wifi in STA mode");
		WiFi.mode(WIFI_STA);
        WiFi.begin(g_rom_settings.ssid, g_rom_settings.password);
        WiFi.setAutoConnect(true);
		
		// Wait up to 5s for connecting:
		int stat = 0;
		int i = 0;
		for (i = 0; i < 50; i++) {
			stat = WiFi.status();
			if (stat == WL_CONNECTED) {
				g_wifi_status.local_ip = WiFi.localIP();
				g_wifi_status.gateway = WiFi.gatewayIP();
				g_wifi_status.subnet = WiFi.subnetMask();
				g_wifi_status.is_connected = 1;
				DB(" -> connected: Ip=%08lXh, GW=%08lXh, Sub=%08lXh", 
					g_wifi_status.local_ip, g_wifi_status.gateway, g_wifi_status.subnet);
				break;
			}
			DB("%d", stat);
			delay(100);    
		}
	}
}

const ROM_SETTINGS_t * wifi_inf::get_settings()
{
    return &g_rom_settings;    
}

void wifi_inf::manager()
{
	ROM_SETTINGS_t settings;
	
	if (g_wifi_status.mode == WIFI_AP) {
		while (true) {
			if (httpd::get_config(&settings)) {
				DB("\r\n -> having new settings -> store...");
                memcpy(&g_rom_settings, &settings, sizeof(ROM_SETTINGS_t));
                g_rom_settings.magic_number = NVM_MAGIC_NUMBER;
                store_rom_settings();
				
				DB("\r\n -> reboot now!");
                ESP.restart();
            }			
		}		
	}	
}

void wifi_inf::factory_reset()
{
    DB("\r\n -> factory_reset!");
	g_rom_settings.magic_number = 0;
	sprintf(g_rom_settings.ssid, "");
	sprintf(g_rom_settings.password, "");
	sprintf(g_rom_settings.server_addr, "");
	g_rom_settings.server_port = 0;
	sprintf(g_rom_settings.security, "");
	g_rom_settings.timezone = 7 * 60;
	store_rom_settings();
}

///////////////////////////////////////PRIVATE FUNCTIONS///////////////////////////////////////////
/**
 * Load settings from ROM memory (Non-volatile).
 * File format (text):
 *  Line-0: [MagicNumber][LF]
 *  Line-1: [ssid][LF]
 *  Line-2: [password][LF]
 *  Line-3: [server][LF]
 *  Line-4: [server-port][LF]
 *  Line-5: [security][LF]
 *  Line-6: [timezone][LF]
*/
void wifi_inf::load_rom_settings()
{
    char file_buf[ROM_SETTINGS_SIZE];
    uint32_t sz = esp8266_mlib::load_file(ROM_SETTINGS_FILE_NAME, file_buf, ROM_SETTINGS_SIZE);
    char *p1 = NULL;
    char *p2 = NULL;

    DB("\r\n%s: content=[%s]", __FUNCTION__, file_buf);
    if (sz > 0) {
        p1 = file_buf;
        
        // MagicNumber:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
			g_rom_settings.magic_number = atoi(p1);
            p1 = p2 + 1;
        }
		
		// SSID:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
            snprintf(g_rom_settings.ssid, CFG_SSID_SZ, "%s", p1);
            p1 = p2 + 1;
        }

        // password:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
            snprintf(g_rom_settings.password, CFG_PASSWORD_SZ, "%s", p1);
            p1 = p2 + 1;
        }

        // ServerAddress:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
            snprintf(g_rom_settings.server_addr, CFG_SERVER_SZ, "%s", p1);
            p1 = p2 + 1;
        }
		
		// ServerPort:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
			g_rom_settings.server_port = atoi(p1);
            p1 = p2 + 1;
        }

        // security:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
            snprintf(g_rom_settings.security, CFG_SECURITY_SZ, "%s", p1);
            p1 = p2 + 1;
        }
		
		// Timezone:
        p2 = strchr(p1, '\n');
        if (p2 != NULL) {
            *p2 = '\0';
			g_rom_settings.timezone = atoi(p1);
            p1 = p2 + 1;
        }
    }
    DB(" -> ssid=%s, pwd=%s, server=%s:%u, sec=%s, tz=%d",
        g_rom_settings.ssid, g_rom_settings.password, g_rom_settings.server_addr, 
        g_rom_settings.server_port, g_rom_settings.security, g_rom_settings.timezone);
}

/**
 * Store settings to ROM memory.
*/
void wifi_inf::store_rom_settings()
{
    char file_buf[ROM_SETTINGS_SIZE] = "";
    char str[64];

	snprintf(str, CFG_PASSWORD_SZ, "%u\n", g_rom_settings.magic_number);
    strcat(file_buf, str);
    snprintf(str, CFG_SSID_SZ, "%s\n", g_rom_settings.ssid);
	strcat(file_buf, str);
    snprintf(str, CFG_PASSWORD_SZ, "%s\n", g_rom_settings.password);
    strcat(file_buf, str);
    snprintf(str, CFG_SERVER_SZ, "%s\n", g_rom_settings.server_addr);
    strcat(file_buf, str);
	snprintf(str, CFG_PASSWORD_SZ, "%u\n", g_rom_settings.server_port);
    strcat(file_buf, str);
    snprintf(str, CFG_SECURITY_SZ, "%s\n", g_rom_settings.security);
    strcat(file_buf, str);
	snprintf(str, CFG_SECURITY_SZ, "%d\n", g_rom_settings.timezone);
    strcat(file_buf, str);
    DB("\r\n%s: -> content=[%s]", __FUNCTION__, file_buf);
    esp8266_mlib::save_file(ROM_SETTINGS_FILE_NAME, file_buf);
}
