#ifndef _APP_INIT_
#define _APP_INIT_

#include <stdint.h>

void touchApp_Init();
void appRun();

typedef enum {
	BLE_Entry,
	BLE_Start,
	BLE_Stop
} BLEInfo;

void logBLE(BLEInfo info, uint8_t rssi, uint8_t *address, const char *local_name);

#endif
