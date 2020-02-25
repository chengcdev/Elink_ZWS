/*
 * main_server_sys.h
 *
 *  Created on: 2017年12月16日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_SYS_H_
#define APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_SYS_H_



#ifdef __cplusplus
extern "C"{
#endif

#include "../include/include_coap.h"


/********************************************************
 * function: mlcoap_server_sys_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_sys_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket);


#ifdef __cplusplus
}
#endif




#endif /* APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_SYS_H_ */
