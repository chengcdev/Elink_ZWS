#include "mico.h"
#include "DTTable.h"

DTDEF gDTDef[] = {
	{DT_GATEWAY, 4},
	{DT_DEVICETYPE, 3},
	{DT_VERSION, 2},
	{DT_SCENECALL, 2},
	{DT_RELAY, 1},
	{DT_DIMMER, 1},
	{DT_CURTAIN, 1},
	{DT_ROAD, 2},
	{DT_KEY_PRESS_10x4, 40},
	{DT_KEY_NORMAL_10x3, 30},
	{DT_KEY_LED_10x2, 20},
	{DT_KEY_LED, 1},
	{DT_KEY_EVENT, 1},
	{DT_KEY_SEL, 2},
	{0, 0}
};

int DT_Length(uint16_t dataType)
{
	int i;
	for (i = 0; gDTDef[i].dt != 0 ; i++) {
		if (gDTDef[i].dt == dataType) {
			return gDTDef[i].size;
		}		
	}
    return 0;
}


