#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "esp8266_mlib.h"
#include "mtime.h"
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
static DEVICE_INFO_t g_device_list[DEVICE_COUNT];

/* Number of devices for this Node */
static uint8_t g_device_count = 1;

// File buffer:
static uint8_t g_file_buf[FILE_CONTENT_LIMIT];

///////////////////////////////////////LOCAL FUNCTIONS/////////////////////////////////////////////
/** @brief this function is called at the second=0 of every minute and only once per minute. If not,
 *  the schedule's command may be executed multiple times.
*/
static void check_schedule(DEVICE_INFO_t *p_dev, uint8_t today, uint16_t now_min) {
    for (int i = 0; i < DEVICE_SCHEDULE_CNT; i++) {
        SCHD_INFO_t *p_sch = &p_dev->config.schedules[i];
        if ((p_sch->id != 0) && (p_sch->enable != 0) && (p_sch->days & today)) {
            if (p_sch->time == now_min) {
                device::control(p_dev->offset, p_sch->cmd);
            }
        }
    }    
}

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void device::init() {
	// Load Device settings:
    for (int i = 0; i < g_device_count; i++) {
        DEVICE_INFO_t *p_dev = &g_device_list[i];
        p_dev->offset = i + 1;
        p_dev->p = 255;
        p_dev->r = 0;
        p_dev->t = 0;
        p_dev->type = 1;
        p_dev->v = 0;
    }
	
	// Load Schedules:
	
}

// This function is called every 1s.
void device::manager() {
	static uint32_t cnt_1min;

    // Auto send STATUS to server every 1 minutes:
    if (++cnt_1min >= 60) {
        cnt_1min = 0;
        mqtt_inf::send_STATUS(g_device_count, g_device_list);
    }

    // Scheduler:
    if (mtime::is_SECOND_00()) {
        uint8_t today = mtime::get_weekday();
        uint16_t now_min = mtime::get_minute_in_day();
        for (int i = 0; i < g_device_count; i++) {
            DEVICE_INFO_t *p_dev = &g_device_list[i];
            check_schedule(p_dev, today, now_min);
        }
    }
}

uint8_t device::count() {
    return g_device_count;
}

const DEVICE_INFO_t *device::get_status()
{
    return g_device_list;
}

void device::config(uint8_t offset, const DEVICE_CONFIG_t *cfg) {
	if ((offset > 0) && (offset <= g_device_count)) {
        memcpy(&g_device_list[offset - 1].config, cfg, sizeof(DEVICE_CONFIG_t));
        store_settings();
    }	
}

void device::control(uint8_t offset, uint8_t cmd) {    
    if ((offset > 0) && (offset <= g_device_count)) {
        DEVICE_INFO_t *p_dev = &g_device_list[offset - 1];
        DB("\r\n%s: offset=%u, cmd=%u", __FUNCTION__, offset, cmd);
        if (cmd > 1) {
            cmd = 1;
        }
        if (p_dev->v != cmd) {
            DB(" -> status changed!");
            p_dev->v = cmd;
            p_dev->t = mtime::get_local_unix();
            switch (offset) {
            case 1:
                digitalWrite(LED_BUILTIN, !cmd);
                break;
            
            case 2:
                break;

            default:
                break;
            }

            // Send status to Server:
            mqtt_inf::send_STATUS(1, p_dev);
        }
    }
}

void device::toggle(uint8_t offset) {
    if ((offset > 0) && (offset <= g_device_count)) {
        DEVICE_INFO_t *p_dev = &g_device_list[offset - 1];
        uint8_t cmd = 0;
        if (p_dev->v == 0) {
            cmd = 1;
        }

DB("\r\n%s: offset=%u, v=%u -> cmd=%u", __FUNCTION__, offset, p_dev->v, cmd);

        control(offset, cmd);
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
    //uint32_t sz = esp8266_mlib::load_file(DEVICE_FILE_NAME, g_file_buf, FILE_CONTENT_LIMIT);
	
	
}

void device::store_settings() {
	
}


