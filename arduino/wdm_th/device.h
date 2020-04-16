/** @brief define Constants, Prototypes for the Device management.
 *  @date
 *      - 2019_09_25: Create.
 * 
*/
#ifndef _DEVICE_H_
#define _DEVICE_H_

/* Device types */
#define DEV_TYPE_SMOKE              1
#define DEV_TYPE_TEMPERATURE        2
#define DEV_TYPE_HUMIDITY           3
#define DEV_TYPE_ONOFF              4

/* Maximum schedules per device */
#define DEVICE_SCHEDULE_CNT			10

struct DEVICE_INFO_t {
    uint8_t  type;
    uint8_t  offset;
    int32_t  v;         // Value
    int32_t  r;         // RSSI: [-128; 0]dBm
    uint8_t  p;         // Power: [0 - 100]%
    uint32_t t;         // Changed time: UTC Unix second.
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

typedef struct {
	uint8_t offset;
	uint8_t enable;
    SCHD_INFO_t schedules[DEVICE_SCHEDULE_CNT];
} DEVICE_CONFIG_t;

struct EVENT_INFO_t {
    uint8_t type;
    uint32_t time; 
    uint32_t detail;   
};

class device
{
    public:
		static void init();
				
		static void schd_manager();
		static void schd_update(SCHD_INFO_t *schd);
		static void schd_remove_all();
		static void schd_remove(uint8_t sch_id);
		
		static void config(const DEVICE_CONFIG_t *cfg);		
		static void control(uint8_t offset, uint8_t cmd);
		
    private:
		static void schd_load();
		static void schd_store();

};

#endif
