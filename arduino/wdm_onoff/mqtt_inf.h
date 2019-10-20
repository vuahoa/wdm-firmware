/** @brief define Constants, Prototypes for the MQTT interface module.
 *  @date
 *      - 2019_09_24: Create.
 * 
*/
#ifndef _MQTT_INF_H_
#define _MQTT_INF_H_

#include "device.h"

class mqtt_inf
{
    public:
        /* Start MQTT connection */
        static void start(const char *id, const char *security, const char *server, uint16_t port, void (*cb)(uint16_t payload_sz, byte *payload));
		
		/* Manage (reconnect) MQTT connection */
		static void manager();

		/* Get connection status */
		static bool is_connected();
		
		/* Transmission functions */
        static void send_TIME_GET(const uint8_t id[], uint32_t now);
        static void send_STATUS(int dev_cnt, const DEVICE_INFO_t *dev_list);
        static void send_EVENT(int ev_cnt, const EVENT_INFO_t *ev_list);

    private:
        static void mqtt_rx_callback(char* topic, byte* payload, unsigned int len);
};

#endif
