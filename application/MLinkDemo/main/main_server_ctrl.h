/*
 * main_server_ctrl.h
 *
 *  Created on: 2017年12月16日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_CTRL_H_
#define APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_CTRL_H_


#ifdef __cplusplus
extern "C"{
#endif

//typedef enum{
//    NORMAL_VALUE                            = 0,        // 基本参数类型
//    MULT_VALUE                              = 1,        // 多个参数的数据类型
//    ENCODE_VALUE                            = 2         // 数据流类型采用base64位
//}VALUE_TYPE_E;

/********************************************************
 * function: mlcoap_server_ctrl_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_ctrl_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket);



#ifdef __cplusplus
}
#endif



#endif /* APPLICATION_MLINKDEMO_MAIN_MAIN_SERVER_CTRL_H_ */
