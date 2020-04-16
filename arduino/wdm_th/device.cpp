#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "esp8266_mlib.h"
#include "mqtt_inf.h"
#include "device.h"

#define DB      Serial.printf
#define ERR     Serial.printf
#ifndef DB
  #define DB
#endif

///////////////////////////////////////LOCAL CONSTANTS/////////////////////////////////////////////
/* Device config storage file */
const char *DEVICE_FILE_NAME = "/device.cfg";

/* Schedules storage file */
const char *SCHEDULE_FILE_NAME = "/schedule.cfg";

/* Maximum schedule file content size */
#define FILE_CONTENT_LIMIT			256

/* Maximum number of schedules */
#define SCHD_CNT			10

#define DEV_CNT				5

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
static SCHD_INFO_t g_schd_list[SCHD_CNT];

static DEVICE_CONFIG_t g_device_configs[DEV_CNT];

// File buffer:
static uint8_t g_file_buf[FILE_CONTENT_LIMIT];

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void device::init() {
	// Load Device settings:
	
	// Load Schedules:
	
}

void device::schd_manager() {
	
}

void device::schd_update(SCHD_INFO_t *schd) {
	uint32_t i = 0;
	SCHD_INFO_t *p = NULL;
	
	// Update existing:
	for (i = 0; i < SCHD_CNT; i++) {
		p = &g_schd_list[i];
		if (p->is_used && (p->id == schd->id)) {
			memcpy(p, schd, sizeof(SCHD_INFO_t));
			break;
		}
	}
	
	// Add New:
	if (i >= SCHD_CNT) {
		for (i = 0; i < SCHD_CNT; i++) {
			p = &g_schd_list[i];
			if (!p->is_used) {
				memcpy(p, schd, sizeof(SCHD_INFO_t));
				p->is_used = 1;
				break;
			}
		}
	}
}

void device::schd_remove_all() {
	uint32_t i = 0;
	SCHD_INFO_t *p = NULL;
	
	for (i = 0; i < SCHD_CNT; i++) {
		p = &g_schd_list[i];
		p->is_used = 0;
	}
}

void device::schd_remove(uint8_t id) {
	uint32_t i = 0;
	SCHD_INFO_t *p = NULL;
	
	for (i = 0; i < SCHD_CNT; i++) {
		p = &g_schd_list[i];
		if (p->id == id) {
			p->is_used = 0;
			break;
		}
	}
}

void device::config(const DEVICE_CONFIG_t *cfg) {
	if ((cfg->offset > 0) && (cfg->offset < DEV_CNT)) {
        memcpy(&g_device_configs[cfg->offset - 1], cfg, sizeof(DEVICE_CONFIG_t));

    }	
}

void device::control(uint8_t offset, uint8_t cmd) {
	if (offset == 1) {
		digitalWrite(LED_BUILTIN, !cmd);		
	}
}
		
///////////////////////////////////////PRIVATE FUNCTIONS///////////////////////////////////////////
/**
 * Load settings from ROM memory (Non-volatile).
 * File format (text):
 *  Line-0: [MagicNumber][LF]
 *  Line-1: [ScheduleCount][LF]		<= 10
 *  Line-2: [Schedule-1][LF]
 *  Line-3: [Schedule-2][LF]
 *  Line-4: [Schedule-3][LF]
....
*/
void device::schd_load() {
    //uint32_t sz = esp8266_mlib::load_file(SCHEDULE_FILE_NAME, g_file_buf, FILE_CONTENT_LIMIT);
	
	
}

void device::schd_store() {
	
}

// void device::load() {

// }

// void device::store() {
    
// }
