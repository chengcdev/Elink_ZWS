/*
 * main_server_manage.h
 *
 *  Created on: 2017年12月16日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_MANAGE_H_
#define APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_MANAGE_H_


#ifdef __cplusplus
extern "C"{
#endif

/********************************************************
 * function: mlcoap_server_manager_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_manager_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, ml_coap_ackPacket_t *packPacket);


#ifdef __cplusplus
}
#endif




#endif /* APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_MANAGE_H_ */
