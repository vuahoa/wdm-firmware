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

/* Maximum Number of devices for a Node: use for multiple projects, for each project, developer
should set the 'g_device_count' for the real number of devices. */
#define DEVICE_COUNT                10

/* Maximum number of schedules */
#define SCHD_CNT			10


///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
static SCHD_INFO_t g_schd_list[SCHD_CNT];

static DEVICE_INFO_t g_device_list[DEVICE_COUNT];

/* Number of devices for this Node */
static uint8_t g_device_count = 1;

// File buffer:
static uint8_t g_file_buf[FILE_CONTENT_LIMIT];

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void device::init() {
	// Load Device settings:
	
	// Load Schedules:
	
}

void device::manager() {
	
}

uint8_t device::count() {
    return g_device_count;
}

const DEVICE_INFO_t *device::get_status()
{
    return g_device_list;
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

void device::config(uint8_t offset, const DEVICE_CONFIG_t *cfg) {
	if ((offset > 0) && (cfg->offset <= g_device_count)) {
        memcpy(&g_device_list[offset - 1].config, cfg, sizeof(DEVICE_CONFIG_t));
        store_settings();
    }	
}

void device::control(uint8_t offset, uint8_t cmd) {
    if ((offset > 0) && (cfg->offset <= g_device_count)) {
        switch (offset) {
        case 1:
            digitalWrite(LED_BUILTIN, !cmd);
            break;
        
        case 2:
            break;

        default:
            break;
        }
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
void device::load_settings() {
    uint32_t sz = esp8266_mlib::load_file(DEVICE_FILE_NAME, g_file_buf, FILE_CONTENT_LIMIT);
	
	
}

void device::store_settings() {
	
}
