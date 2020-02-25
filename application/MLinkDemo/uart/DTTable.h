
#ifndef DTTable_h__
#define DTTable_h__

//#include "config.h"

//��������
enum {
	DT_GATEWAY=1,
	DT_DEVICETYPE,
	DT_VERSION,
	DT_SCENECALL,
	
	DT_HEART = 10,
	DT_THRESHOLD_MUTATION_ALARM = 20,
	DT_UNDERVOLTAGE_ALARM       = 21,
	DT_TAMPER_ALARM             = 22,
	DT_FAULT_ALARM              = 23,

	DT_ALARM_STATE              = 30,
	DT_ALARM_DELAY              = 31,

	DT_RELAY=101,
	DT_DIMMER,
	DT_CURTAIN,
	DT_ROAD,
	
	DT_KEY_PRESS_10x4=201,
	DT_KEY_NORMAL_10x3,
	DT_KEY_LED_10x2,
	
	DT_KEY_LED=221,
	DT_KEY_EVENT,
	DT_KEY_SEL,

	DT_PM                       = 301,
	DT_VOC                      = 302,
	DT_HCHO                     = 303,
	DT_CO2                      = 304,
	DT_TEMPRETURE               = 305,
	DT_WETNESS                  = 306

};

typedef struct {
	uint16_t dt;
	uint16_t size;
}DTDEF;

extern DTDEF gDTDef[];

int DT_Length(uint16_t dataType);

#endif // DTTable_h__
