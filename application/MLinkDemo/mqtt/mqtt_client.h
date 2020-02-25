/*
 * mqtt_client.h
 *
 *  Created on: 2017年9月7日
 *      Author: admin
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MQTT_MQTT_CLIENT_H_
#define DEMOS_APPLICATION_MLINKDEMO_MQTT_MQTT_CLIENT_H_


#ifdef __cplusplus
extern "C"{
#endif

//#define MQTT_CLIENT_SSL_ENABLE  // ssl

#define MAX_MQTT_TOPIC_SIZE         (256)
#define MAX_MQTT_DATA_SIZE          (1024)
#define MAX_MQTT_SEND_QUEUE_SIZE    (5)



/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    char topic[MAX_MQTT_TOPIC_SIZE];
    char qos;
    char retained;
    uint8_t data[MAX_MQTT_DATA_SIZE];
    uint32_t datalen;
} mqtt_recv_msg_t, *p_mqtt_recv_msg_t, mqtt_send_msg_t, *p_mqtt_send_msg_t;


typedef int (*PMQTT_REV_CALLBACK)( uint8_t *revData, uint32_t datalen);
typedef int (*PMQTT_DEAL_CALLBACK)( char *str );
typedef int (*mqtt_connect_notify_t)(unsigned char connect_state);


typedef struct
{
	char *topictail;
    char topic[MAX_MQTT_TOPIC_SIZE];
    PMQTT_REV_CALLBACK handle;
} mqtt_sub_topic_t;

typedef struct{
    PMQTT_REV_CALLBACK topic_ctrl_recv_deal;
    PMQTT_REV_CALLBACK topic_passthrough_recv_deal;
    PMQTT_REV_CALLBACK topic_manage_recv_deal;
    PMQTT_REV_CALLBACK topic_room_ctrl_recv_deal;
}TOPIC_RECV_DEAL_FUN_T, *PTOPIC_RECV_DEAL_FUN_T;


void mqtt_set_sub_callback(PTOPIC_RECV_DEAL_FUN_T proc);

OSStatus mqtt_pub_notify(const unsigned char* msg,uint32_t msg_len);

OSStatus mqtt_pub_event(const unsigned char* msg,uint32_t msg_len);

OSStatus mqtt_pub_error(const unsigned char* msg,uint32_t msg_len);

OSStatus mqtt_pub_manage(const unsigned char* msg,uint32_t msg_len);

OSStatus mqtt_pub_ctrlecho(const unsigned char* msg,uint32_t msg_len);

#ifdef __cplusplus
}
#endif


#endif /* DEMOS_APPLICATION_MLINKDEMO_MQTT_MQTT_CLIENT_H_ */
