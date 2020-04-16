
#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "esp8266_mlib.h"
#include "wifi_inf.h"

#include "udp_inf.h"

#include <Wire.h>
#include "DFRobot_SHT20.h"

#define DB      	Serial.printf
#define DB_print 	Serial.print

const int PIN_SCL = 5;
const int PIN_SDA = 4;
const int PIN_BT_RESET = 13;
const int PIN_LED = 14;

DFRobot_SHT20    sht20;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BT_RESET, INPUT);
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(74880);

  esp8266_mlib::init();

  // Signal LED:
  led_write(HIGH);

  // Init SHT20:
  sht20.initSHT20();

  // Capture Reset button:
  if (capture_reset()) {
    DB(" -> enter SETUP (AP) mode now!");
    wifi_inf::start(1);
  } else {
    // Start WIFI connection:
    wifi_inf::start(0);
  }

  // Read Temperature & Humidity:
  int16_t t = 0, h = 0;
  read_th(&t, &h);

  // Wait for new settings when in AP mode:
  wifi_inf::manager();

  // Init UDP:
  const ROM_SETTINGS_t *p_cfg = wifi_inf::get_settings();
  const WIFI_STATUS_t *p_wf = wifi_inf::get_status();
  udp_inf::init(p_wf->node_id, p_cfg->security, p_wf->server_ip, 7523);

  // Send status to server:
  DEVICE_INFO_t dev;
  dev.v = t;
  dev.offset = 1;
  dev.p = 100;
  dev.r = WiFi.RSSI();
  dev.type = DEV_TYPE_TEMPERATURE;
  udp_inf::send_STATUS(1, &dev);

  // Sleep:
  esp8266_mlib::enter_sleep(30 * 1000 * 1000);
}

void loop() {
#if 0
  // Read Temperature & Humidity:
  read_th();

  // Wait for new settings when in AP mode:
  wifi_inf::manager();


  // Send status to server:

  // Sleep:
  esp8266_mlib::enter_sleep(30 * 1000 * 1000);
#endif
}

/*  Capture RESET button: if the button is hold for 3-6 seconds then release -> do factory reset.
*/
int capture_reset() {
  int pb_cnt = 0;

  while (true) {
    int pb = digitalRead(PIN_BT_RESET);
    if (pb == HIGH) {
      break;
    }
    DB(".");
    pb_cnt++;
    if ((pb_cnt >= 10) && (pb_cnt <= 30)) {
      led_write(HIGH);
    } else {
      led_write(LOW);
    }
    delay(100);
  };
  if ((pb_cnt >= 10) && (pb_cnt <= 30)) {
    return 1;
  }
  return 0;
}

void led_write(uint8_t state) {
  digitalWrite(PIN_LED, !state);
}

void callback(uint16_t payload_sz, byte *payload) {

    uint8_t mark = payload[0];
    uint8_t opcode = payload[1];
    uint8_t *rx_id = &payload[2];

  switch (opcode) {
    case 'a':
      break;

    case 'c':
      break;

    case 'd':
      break;

    default:
      DB(" -> unknown Opcode=%02xh!", opcode);
      break;
  }
}

void read_th(int16_t *t, int16_t *h) {
  DB("\r\n%s: t_start=%u", __FUNCTION__, millis());
  float humid = sht20.readHumidity();                  // Read Humidity
  float temp = sht20.readTemperature();               // Read Temperature
  *t = (int16_t)(temp * 10);
  *h = (int16_t)(humid * 10);
  DB(" -> t_end=%u: T=%d, H=%d", millis(), *t, *h);
}