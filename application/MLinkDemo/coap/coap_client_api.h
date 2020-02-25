/* mlcoap_client_api.h -- ml mlcoap_client_api header file for ML CoAP client api interfaces 
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef _MLCOAP_CLIENT_API__H_
#define _MLCOAP_CLIENT_API__H_



#ifdef __cplusplus
extern "C" {
#endif

#define MLCOAP_CLIENT_WAIT_MAX 				2//5
#define MLCOAP_CLIENT_DEFAULT_OBSSECOND		30
#define MLCOAP_CLIENT_BLOCKSIZE				"1024"

typedef enum
{
	MLCOAP_ACK_OK		=0,
	MLCOAP_ACK_TIMEOUT	=100,
}MLCOAP_ACK_E;


/** coap ����ӿڻص����������� **/
//typedef int (*mlcoap_client_ack_handler)(int reqpdu_tid,unsigned int ackcode,const unsigned char *ackdata,size_t acklen);
typedef int (*mlcoap_client_ack_handler)(int reqpdu_tid,ml_coap_revMsg_t *pRevMsg);
/*** coap subscribe data callback handle ***/
typedef void (*mlcoap_client_subscribe_handler)(const unsigned char *data,size_t len);


int mlcoap_client_init(void);


/**  ��ѯ����coap client  ���յ� coap revice ����
  * 	�ú�����Ҫ��Ӧ�ò��ڵ����߳��е���
  *  timeout default  50us
  * @return >=0   or <0 on error.
  */
int mlcoap_client_poll_context(void);

/**�����;�����ӦӦ�����ݣ�����post ��ʽ
  * 	 unsigned char *strMethod,
  * @param struri ����uri path eg: coap://192.168.1.12/time
  * @param data  �������ݵ�payload ����
  * @param handle  Ӧ����Ӧ�ص�����
  * @return >=0  on success �������ϢID   or <0 on error.
  */
int mlcoap_client_send_msg(unsigned char *hostname,unsigned char *struri,unsigned char *data ,
		mlcoap_client_ack_handler handle);


 /**Ŀǰ�ݲ�֧�ֶ��ĳ�ʱ��ֻ��ͨ��ȡ���ӿڽ��ж���ȡ��
  * 	 unsigned char *strMethod,
  * @param struri ����uri path eg: coap://192.168.1.12/time
  * @param data  �������ݵ�payload ����
  * @return >=0  on success �������ϢID   or <0 on error.
  */
//int mlcoap_client_send_msg_non(unsigned char *struri,unsigned char *data);
int mlcoap_client_send_msg_non(unsigned char *hostname,unsigned char *struri,unsigned char *data);

int mlcoap_client_send_msg_non_ack(unsigned char *hostname,unsigned char *struri,unsigned char *data,
		mlcoap_client_ack_handler handle);


/**�����Ĺ۲�ĳ����Ϣ
 /**Ŀǰ�ݲ�֧�ֶ��ĳ�ʱ��ֻ��ͨ��ȡ���ӿڽ��ж���ȡ��
  * 	 
  * @param struri �����ĵ�uri path eg: coap://192.168.1.12/time
  * @param data  �������ݵ�payload ����
  * @param handle ����Ӧ�����ݻص�
  * @param subs_handle ����֪ͨ���ݻص�
  * @return >=0  on success �������ϢID   or <0 on error.
  */
int mlcoap_client_subscribe(unsigned char *struri,unsigned char *data,
		mlcoap_client_ack_handler handle,mlcoap_client_subscribe_handler subs_handle);


/** ȡ�����ĳ������
 *      
 *
 * @param tid	 ������ʱ���ص���ϢID
 * @return >=0 on success �������ϢID  or <0 on error.
 */
int mlcoap_client_subscrible_clear(int tid);


		
#ifdef __cplusplus
}
#endif

#endif /* _MLCOAP_CLIENT_API__H_ */

