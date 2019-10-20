/**	@brief implement basic functions: booting, default LED.
	  @date
		- 2019_01_07: Create.
    - 2019_05_05: Review.
*/
#include "Arduino.h"
#include "FS.h"
#include <ESP8266WiFi.h>
#include "esp8266_mlib.h"


//#define DB      Serial.printf
#ifndef DB
#define DB
#endif


/* PUBLIC functions */
esp8266_mlib::esp8266_mlib()
{
    SPIFFS.begin();
}

/**
    Configure the default LED.
    Configure the Serial port.
    Initialize the SPI File system.
*/
void esp8266_mlib::init()
{
    SPIFFS.begin();
}

const uint8_t *esp8266_mlib::get_id()
{
    static uint8_t mac_addr[8];
    WiFi.macAddress(mac_addr);
    return mac_addr;
}

const char *esp8266_mlib::get_id_str()
{
    static char id_str[16];
    uint8_t mac_addr[8];
    WiFi.macAddress(mac_addr);
    sprintf(id_str, "%02x%02x%02x%02x%02x%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], mac_addr[6]);
    return id_str;
}
		
/**	@brief get Boot cause.
*/
uint32_t esp8266_mlib::get_boot_cause()
{
    uint32_t ret = 0;
    ESP.rtcUserMemoryRead(0x0000, &ret, 4);
    DB("\r\n: ret=%08lXh", __FUNCTION__, ret);
    if ((ret != PWR_BOOT_POR) && (ret != PWR_BOOT_SOFT) && (ret != PWR_BOOT_SLEEP)) {
        DB(" -> invalid RTC mem!");
        ret = PWR_BOOT_POR;
    }
    return ret;
}

/**	@brief write a Marker to RTC memrory, then enter DeepSleep mode.
		@param us: time in DeepSleep mode. The CPU will wake up after that time. Unit: microseconds.
		@note the GPIO16 must be connected to RST to wakeup CPU.
*/
void esp8266_mlib::enter_sleep(uint32_t us)
{
    uint32_t u32 = PWR_BOOT_SLEEP;
    DB("\r\n -> %s", __FUNCTION__);
    ESP.rtcUserMemoryWrite(0x0000, &u32, 4);
    ESP.deepSleep(us, WAKE_RF_DISABLED);
}

/**	@brief write a Marker to RTC memory, then issue a Software reboot.
*/
void esp8266_mlib::soft_reboot()
{
    uint32_t u32 = PWR_BOOT_SOFT;
    DB("\r\n -> %s", __FUNCTION__);
    ESP.rtcUserMemoryWrite(0x0000, &u32, 4);
    ESP.restart();
}

/** @brief reading a text file content.
    @param *file_name: the name of the file to read, must like "/xxxxx.yyy"
    @param *str: pointer to the output buffer, provided by caller.
    @param str_sz: size of the output buffer.
    @return number of read bytes.
*/
uint32_t esp8266_mlib::load_file(const char *file_name, char *str, uint8_t str_sz)
{
    uint32_t ret = 0;
    DB("\r\n%s: file=%s", __FUNCTION__, file_name);
    if (str_sz > 0) {
        File file = SPIFFS.open(file_name, "r");
        if (!file) {
            DB(" -> open file failed!");
        } else {
            size_t sz = file.size();
            if (sz >= str_sz) {
                sz = str_sz - 1;
            }
            file.readBytes(str, sz);
            file.close();
            str[sz] = 0; // terminate string
            ret = sz;
        }
    }
    DB(" -> ret=%d, content=[%s]", ret, str);
    return ret;
}

/** @brief store a string to a file.
    @param *file_name: name of the file to store to, always override the file.
    @param *content: the string to store.
    @return true if store successully, false if not.
*/
bool esp8266_mlib::save_file(const char *file_name, const char *content)
{
    DB("\r\n%s: fname=%s, content=%s", __FUNCTION__, file_name, content);
    File f = SPIFFS.open(file_name, "w");
    if (!f) {
        DB(" -> open file failed!");
        return false;
    }
    f.write((uint8_t *)content, strlen(content));
    f.close();
    return true;
}

uint32_t esp8266_mlib::buf_to_u32(uint8_t buf[])
{
    uint32_t ret = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                   ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    return ret;
}

void esp8266_mlib::u32_to_buf(uint32_t u32, uint8_t buf[])
{
    buf[0] = (uint8_t)(u32 & 0xff);
    buf[1] = (uint8_t)((u32 >> 8) & 0xff);
    buf[2] = (uint8_t)((u32 >> 16) & 0xff);
    buf[3] = (uint8_t)((u32 >> 24) & 0xff);
}

uint16_t esp8266_mlib::buf_to_u16(uint8_t buf[])
{
    uint16_t ret = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
    return ret;
}

void esp8266_mlib::u16_to_buf(uint16_t u16, uint8_t buf[])
{
    buf[0] = (uint8_t)(u16 & 0xff);
    buf[1] = (uint8_t)((u16 >> 8) & 0xff);
}
