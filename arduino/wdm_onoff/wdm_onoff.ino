/** @brief implement WDM-ONOFF device:
 *      - Device type: ONOFF.
 *      - One device per Node.
 *      - Press & hold RESET button for 3s then release -> do factory reset.
 *      - Press RESET button (and release immediately) to issue a Toggle
 * command.
 *
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#include "device.h"
#include "esp8266_mlib.h"
#include "mqtt_inf.h"
#include "mtime.h"
#include "wifi_inf.h"

#define DB Serial.printf
#define DB_print Serial.print

const int PIN_BT_RESET = 13;

static Ticker g_ticker;
static uint8_t g_1s_flg;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    pinMode(PIN_BT_RESET, INPUT);
    Serial.begin(115200);

    // Reset pin status:
    led_write(LOW);

    // Init libraries:
    esp8266_mlib::init();
    Serial.print("\r\nesp8266_mlib.init done!");

    // Init WIFI connection:
    wifi_inf::start();

    // Init MQTT management:
    const char *id = esp8266_mlib::get_id_str();
    const ROM_SETTINGS_t *cfg = wifi_inf::get_settings();
    mqtt_inf::start(id, cfg->security, cfg->server_addr, cfg->server_port, callback);

    // Start ticker:
    g_ticker.attach_ms(1000, timer_1s);
}

void loop() {
#define SYNC_CYCLE (1 * 20)
    static uint16_t sync_cnt = SYNC_CYCLE - 1;

    // Wait for user settings in AP mode:
    wifi_inf::manager();

    // Capture RESET button:
    int bt_event = capture_button();
    if (bt_event == 2) {
        wifi_inf::factory_reset();
        esp8266_mlib::soft_reboot();
    } else if (bt_event == 1) {

    }

    // Handle MQTT connection with server:
    mqtt_inf::manager();

    // SYNC time with server: every 15 minutes
    if (g_1s_flg) {
        g_1s_flg = 0;
        if (mqtt_inf::is_connected()) {
            if (++sync_cnt >= SYNC_CYCLE) {
                sync_cnt = 0;
                mqtt_inf::send_TIME_GET(esp8266_mlib::get_id(), mtime::get_local_unix());
            }
        }
    }

    // Scheduler:
    // device::manager();
}

void timer_1s() {
    mtime::ticker_cb();
    g_1s_flg = 1;
}

/** @brief Capture RESET button events.
    @return if the button is held for 3-6 seconds then release -> return 2.
            if the button is pressed for < 1 seconds -> return 1.
            others -> return 0.
*/
int capture_button() {
    int pb_cnt = 0;

    while (true) {
        int pb = digitalRead(PIN_BT_RESET);
        if (pb == HIGH) {
            break;
        }
        DB(".");
        pb_cnt++;
        if ((pb_cnt >= 30) && (pb_cnt <= 60)) {
            led_write(HIGH);
        } else {
            led_write(LOW);
        }
        delay(100);
    };
    if (pb_cnt < 10) {
        return 1;
    } else if ((pb_cnt > 30) && (pb_cnt <= 60)) {
        return 2;
    }
    return 0;
}

void led_write(uint8_t state) { digitalWrite(LED_BUILTIN, !state); }

const size_t capacity =
    5 * JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(5) + 70;
static StaticJsonDocument<capacity> doc;

void callback(uint16_t payload_sz, byte *payload) {
    // const char *rx_id = NULL;
    uint8_t offset = 0;
    uint8_t cmd = 0;

    // DB("\r\n%s: json.cap=%d, payload len=%d, payload:%s", __FUNCTION__,
    // capacity, payload_sz, payload);
    DB_print(String("\r\n") + __FUNCTION__ + ": json.cap=" + capacity +
             ", payload_sz:" + payload_sz +
             ", payload:" + String((char *)payload));

    // DynamicJsonDocument doc(capacity);

    //   DeserializationError error = deserializeJson(doc, &payload[1]);
    //   if (error) {
    //     DB("deserializeJson() failed: ");
    //     DB(error.c_str());
    //     return;
    //   }

    uint8_t mark = payload[0];
    uint8_t opcode = payload[1];
    uint8_t *rx_id = &payload[2];
    //     rx_id = doc["id"];
    //   DB_print(String("\r\n -> rx_id=") + rx_id + ", opcode=" +
    //   String(opcode, HEX));
    //   //DB(" -> rx_id=%s, op=%2Xh", rx_id, opcode);
    //   if (strcmp(rx_id, esp8266_mlib::get_id_str()) != 0) {
    //     DB_print(" -> Invalid rx_id!");
    //     return;
    //   }

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
        offset = doc["data"]["offset"];
        cmd = doc["data"]["cmd"];
        DB("\r\n -> OPH_COMMAND: id=%s, offset=%d, cmd=%d", rx_id, offset,
           cmd);
        device::control(offset, cmd);
    } break;

    default:
        DB(" -> unknown Opcode=%02xh!", opcode);
        break;
    }
}
