/** @brief define Constants, Prototypes for the Device management.
 *  @date
 *      - 2019_09_25: Create.
 * 
*/
#ifndef _DEVICE_H_
#define _DEVICE_H_



/* Maximum schedules per device */
#define DEVICE_SCHEDULE_CNT			10

typedef struct {
	uint8_t enable;
    SCHD_INFO_t schedules[DEVICE_SCHEDULE_CNT];
} DEVICE_CONFIG_t;

struct DEVICE_INFO_t {
    uint8_t  offset;
    uint8_t  type;
    int8_t  r;         // RSSI: [-128; 0]dBm
    uint8_t  p;         // Power: [0 - 100]%
    int32_t  v;         // Value
    uint32_t t;         // Changed time: UTC Unix second.

    DEVICE_CONFIG_t config;
};

typedef struct {
    uint8_t id;
	uint8_t offset;
	
    uint8_t enable;
    uint8_t days;
    uint16_t time;
	uint8_t cmd;
	
    uint8_t is_used;
} SCHD_INFO_t;



struct EVENT_INFO_t {
    uint8_t type;
    uint32_t time; 
    uint32_t detail;   
};



class device
{
    public:
		static void init();
        static void manager();

        static uint8_t count();
        static const DEVICE_INFO_t *get_status();
        static void config(uint8_t offset, const DEVICE_CONFIG_t *cfg);		
		static void control(uint8_t offset, uint8_t cmd);
		
    private:
		static void load_settings();
		static void store_settings();

};

#endif
