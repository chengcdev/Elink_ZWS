/*
 * main_server_discover.h
 *
 *  Created on: 2017年12月16日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_DISC_MSG_H_
#define APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_DISC_MSG_H_



#ifdef __cplusplus
extern "C"{
#endif

#include "mico.h"
#include "../include/include_coap.h"

typedef struct{
    char keyType[8];
    char key[8];
    char value[64];
}STATE_DATA_T, *PSTATE_DATA_T;

//OSStatus mlcoap_server_discover_notify(COAP_NOTI_CMD_ID_E cmd, char *data, int size, ml_coap_ackPacket_t *packPacket);


#ifdef __cplusplus
}
#endif



#endif /* APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_DISC_MSG_H_ */
