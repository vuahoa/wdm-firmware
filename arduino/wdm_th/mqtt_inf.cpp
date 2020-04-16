#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "esp8266_mlib.h"
#include "mqtt_inf.h"

#define DB        Serial.printf
#define DB_print  Serial.print
#define ERR       Serial.printf
#ifndef DB
  #define DB
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
#define OPU_TIME_GET            0x41
#define OPU_STATUS              0x42

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
static WiFiClient espClient;
static PubSubClient client(espClient);
static char mqtt_client[CLIENT_SZ] = "wdm-0";
static char mqtt_username[USERNAME_SZ] = "wdm-user";
static char mqtt_password[PASSWORD_SZ] = "wdm-pass";
static char mqtt_server[SERVER_SZ] = "smtvietnam.net";
static uint16_t mqtt_port;
static char mqtt_sub_topic[TOPIC_SZ] = "wdm/d/1";
static char mqtt_pub_topic[TOPIC_SZ] = "wdm/s/1";

//static uint8_t node_id[NODE_ID_SZ];

void (*pf_callback)(uint16_t payload_sz, byte *payload);

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////
void mqtt_inf::start(const char *id, const char *security, const char *server, uint16_t port, 
                    void (*cb)(uint16_t payload_sz, byte *payload))
{
    DB("\r\n%s: id=%s, sec=%s, server=%s, port=%d", __FUNCTION__,
        id, security, server, port);

    pf_callback = cb;
    
    snprintf(mqtt_client, CLIENT_SZ, "wdm-%s", id);
    snprintf(mqtt_username, USERNAME_SZ, "wdm-user-%s", id);
    snprintf(mqtt_password, PASSWORD_SZ, "%s", security);

    snprintf(mqtt_server, SERVER_SZ, "%s", server);
    mqtt_port = port;
    
    snprintf(mqtt_sub_topic, TOPIC_SZ, "wdm/d/%s", id);
    snprintf(mqtt_pub_topic, TOPIC_SZ, "wdm/s/%s", id);

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

void mqtt_inf::send_STATUS(int dev_cnt, const DEVICE_INFO_t *p_dev) {
	uint8_t arr[64];
	uint8_t i = 0;
    uint8_t id[8];

    WiFi.macAddress(id); 

    arr[i++] = FRAME_MARK;
    arr[i++] = OPU_STATUS;
    memcpy(&arr[i], id, 6);
    i += 6;
    arr[i++] = 1;
    arr[i++] = p_dev->offset;
    arr[i++] = p_dev->type;
    arr[i++] = p_dev->r;
    arr[i++] = p_dev->p;
    esp8266_mlib::u32_to_buf(p_dev->v, &arr[i]);
    i += 4;
    
	DB("\r\n%s: len=%d", __FUNCTION__, i);
	int ret = client.publish(mqtt_pub_topic, arr, i);
    DB(" -> ret=%d", ret);
}

///////////////////////////////////////PRIVATE FUNCTIONS///////////////////////////////////////////
void mqtt_inf::mqtt_rx_callback(char* topic, byte* payload, unsigned int len) {
    //DB("\r\n%s: topic=%s, payload len=%d, payload:%s", __FUNCTION__, topic, len, payload);
    if (strcmp(topic, mqtt_sub_topic) == 0) {
        if (pf_callback != NULL) {
            pf_callback(len, payload);
        }    
    } else {
        ERR("\r\n -> %s.invalid topic: rx=%s vs sub=%s", __FUNCTION__, topic, mqtt_sub_topic);
    }
}
