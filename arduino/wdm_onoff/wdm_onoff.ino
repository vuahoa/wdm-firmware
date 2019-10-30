/** @brief implement WDM-ONOFF device:
 *      - Device type: ONOFF.
 *      - One device per Node.
 *      - Press & hold RESET button for 3s then release -> do factory reset.
 *      - Press RESET button (and release immediately) to issue a Toggle
 * command.
 *
 */
#include <Arduino.h>
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

    // Init Device:
    device::init();

    // Init WIFI connection:
    wifi_inf::start();

    // Init MQTT management:
    const char *id = esp8266_mlib::get_id_str();
    const ROM_SETTINGS_t *cfg = wifi_inf::get_settings();
    mqtt_inf::start(id, cfg->security, cfg->server_addr, cfg->server_port);

    // Start ticker:
    g_ticker.attach_ms(1000, timer_1s);
}

void loop() {
    #define SYNC_CYCLE (1 * 60)
    static uint16_t sync_cnt = SYNC_CYCLE - 1;

    // Wait for user settings in AP mode:
    wifi_inf::manager();

    // Capture RESET button:
    int bt_event = capture_button();
    if (bt_event != 0) {
        DB("\r\n -> bt_event=%u", bt_event);
        if (bt_event == 2) {
            wifi_inf::factory_reset();
            esp8266_mlib::soft_reboot();
        } else if (bt_event == 1) {
            device::toggle(1);
        }
    }

    // Handle MQTT connection with server:
    mqtt_inf::manager();

    // 1 second checker:
    if (g_1s_flg) {
        g_1s_flg = 0;
        
        // SYNC time with server: every 15 minutes
        if (mqtt_inf::is_connected()) {
            if (++sync_cnt >= SYNC_CYCLE) {
                sync_cnt = 0;
                mqtt_inf::send_TIME_GET(esp8266_mlib::get_id(), mtime::get_local_unix());
            }
        }

        // Scheduler:
        device::manager();
    }
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
    if ((pb_cnt > 0) && (pb_cnt < 10)) {
        return 1;
    } else if ((pb_cnt > 30) && (pb_cnt <= 60)) {
        return 2;
    }
    return 0;
}

void led_write(uint8_t state) { digitalWrite(LED_BUILTIN, !state); }
