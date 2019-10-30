#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "esp8266_mlib.h"
#include "mtime.h"
#include "device.h"
#include "wifi_inf.h"
#include "mqtt_inf.h"

#define DB        Serial.printf
#define DB_print  Serial.print
#define ERR       Serial.printf
#ifndef DB
  #define DB
#endif
#ifndef ERR
    define ERR
#endif

///////////////////////////////////////LOCAL CONSTANTS/////////////////////////////////////////////
#define CLIENT_SZ           32
#define USERNAME_SZ         32
#define PASSWORD_SZ         32
#define TOPIC_SZ            32
#define SERVER_SZ           64

// Frame parameters:
#define FRAME_MARK          0x01
#define FRAME_ID_OFFSET     2
#define FRAME_DATA_OFFSET   8

// Opcodes:
#define OPU_TIME_GET        0x41
#define OPU_STATUS          0x42

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
static WiFiClient espClient;
static PubSubClient client(espClient);
static char mqtt_client[CLIENT_SZ] = "wdm-0";
static char mqtt_username[USERNAME_SZ] = "wdm-user";
static char mqtt_password[PASSWORD_SZ] = "wdm-pass";
static char mqtt_server[SERVER_SZ] = "smtvietnam.net";
static uint16_t mqtt_port;
static char mqtt_sub_topic[TOPIC_SZ] = "wdm/dev/sub/1";
static char mqtt_pub_topic[TOPIC_SZ] = "wdm/dev/pub/1";

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void mqtt_inf::start(const char *id, const char *security, const char *server, uint16_t port)
{
    DB("\r\n%s: id=%s, sec=%s, server=%s, port=%d", __FUNCTION__,
        id, security, server, port);

    snprintf(mqtt_client, CLIENT_SZ, "wdm-%s", id);
    snprintf(mqtt_username, USERNAME_SZ, "wdm-user-%s", id);
    snprintf(mqtt_password, PASSWORD_SZ, "%s", security);

    snprintf(mqtt_server, SERVER_SZ, "%s", server);
    mqtt_port = port;
    
    snprintf(mqtt_sub_topic, TOPIC_SZ, "wdm/dev/sub/%s", id);
    snprintf(mqtt_pub_topic, TOPIC_SZ, "wdm/dev/pub/%s", id);

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqtt_rx_callback);
    
    DB("\r\n -> client=%s, account=%s/%s, sub=%s, pub=%s", 
        mqtt_client, mqtt_username, mqtt_password, mqtt_sub_topic, mqtt_pub_topic);
}

void mqtt_inf::manager()
{
	if (!client.connected()) {
        DB("\r\nReconnect MQTT...");
        if (client.connect(mqtt_client, mqtt_username, mqtt_password)) {
            DB(" -> connected");
            client.subscribe(mqtt_sub_topic);
        } else {
            DB(" -> failed, rc=%d", client.state());
            delay(1000);
        }
    } else {
        client.loop();
    }
}

bool mqtt_inf::is_connected() {
	return client.connected();	
}

void mqtt_inf::send_TIME_GET(const uint8_t id[], uint32_t now) {
	uint8_t arr[64];
	uint8_t i = 0;

    arr[i++] = FRAME_MARK;
    arr[i++] = OPU_TIME_GET;
    memcpy(&arr[i], id, 6);
    i += 6;
    esp8266_mlib::u32_to_buf(now, &arr[i]);
    i += 4;

	DB("\r\n%s: len=%d", __FUNCTION__, i);
	client.publish(mqtt_pub_topic, arr, i);
}

/** @brief send OPU_STATUS packet to Server.
 *  @note data()        = [DevCnt(1)=M][DeviceStatusList(M x 12)]
        DeviceStatus(12)= [offset(1)][type(1)][rssi(1)][power(1)][value(4)][time(4)]
*/
void mqtt_inf::send_STATUS(int dev_cnt, const DEVICE_INFO_t *dev_list)
{
    uint8_t arr[128];
    const uint8_t *id = esp8266_mlib::get_id();
	uint8_t i = 0;
    uint8_t k = 0;

    arr[i++] = FRAME_MARK;
    arr[i++] = OPU_STATUS;
    memcpy(&arr[i], id, 6);
    i += 6;
    arr[i++] = dev_cnt;
    for (k = 0; k < dev_cnt; k++) {
        const DEVICE_INFO_t *p_dev = &dev_list[k];
        arr[i++] = p_dev->offset;
        arr[i++] = p_dev->type;
        arr[i++] = p_dev->r;
        arr[i++] = p_dev->p;
        esp8266_mlib::u32_to_buf(p_dev->v, &arr[i]);
        i += 4;
        esp8266_mlib::u32_to_buf(mtime::get_local_unix(), &arr[i]);
        i += 4;
    }

	DB("\r\n%s: len=%d", __FUNCTION__, i);
	client.publish(mqtt_pub_topic, arr, i);
}

///////////////////////////////////////PRIVATE FUNCTIONS///////////////////////////////////////////
/** @brief Process RX packet.
 *  @note packet format:
 *      MQTT.Payload() = [mark(1)][opcode(1)][rx_id(6)][data()]
 * 
*/
void mqtt_inf::mqtt_rx_callback(char* topic, byte* payload, unsigned int len) {
    DB("\r\n%s: topic=%s, payload len=%d", __FUNCTION__, topic, len);
    if (strcmp(topic, mqtt_sub_topic) != 0) {  
        ERR("\r\n -> %s.invalid topic: rx=%s vs sub=%s", __FUNCTION__, topic, mqtt_sub_topic);
        return;
    }

    // Process RX packet: [mark(1)][opcode(1)][rx_id(6)][data()]
    int byte_index = 0;
    uint8_t mark = payload[byte_index++];
    uint8_t opcode = payload[byte_index++];
    uint8_t *rx_id = &payload[byte_index];
    byte_index += 6;
    DB(" -> mark=%02Xh, opcode=%02Xh, rx_id=%02X", mark, opcode, rx_id[0]);

    // Validate params:    
    if (mark != 0x01) {
        DB("\r\n -> Invalid marker!");
        return;
    }
    const uint8_t *local_id = esp8266_mlib::get_id();
    for (int i = 0; i < 6; i++) {
        if (local_id[i] != rx_id[i]) {
            DB(" -> invalid rx_id!");
            return;
        }
    }

    switch (opcode) {
    case 'a': {
        // data() = [currentTime(4)]
        uint32_t t = esp8266_mlib::buf_to_u32(&payload[8]);
        uint32_t t_local = t + wifi_inf::get_settings()->timezone * 60;
        DB("\r\n -> OPH_TIME: t_utc=%lu -> t_local=%lu", t, t_local);
        mtime::set_local_unix(t_local);
    } break;

    case 'c': {
        /*  data = {"offset":number, "en":number, "name":string,
               "disp":string, "sch":[ [id1, enable, days, time, cmd], [id2,
               enable, days, time, cmd]'
                                            ...
                                    ]
                            }
            */
        
    } break;

    case 'd': {
        // data() = [offset(1)][cmd(1)]
        uint8_t offset = payload[byte_index++];
        uint8_t cmd = payload[byte_index++];
        DB("\r\n -> OPH_COMMAND: offset=%d, cmd=%d", offset, cmd);
        device::control(offset, cmd);
    } break;
    
    default:
        DB(" -> unknown Opcode=%02xh!", opcode);
        break;
    }
}
