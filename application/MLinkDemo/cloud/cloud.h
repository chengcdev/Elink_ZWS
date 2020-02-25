
#ifndef _CLOUD_H_
#define _CLOUD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    TOPIC_CTRLECHO      = 0,
    TOPIC_NOTIFY        = 1,
    TOPIC_MANAGE        = 2,
    TOPIC_EVENTMI       = 3
}TOPIC_TYPE_E;

typedef enum{
    CLOUD_CONNECT_FAIL          = 0,
    CLOUD_CONNECT_SUCCESS       = 1,
    CLOUD_CONNECT_UNREGISTERED  = 0xff
}CLOUD_CONNECT_STATE;

typedef enum{
    CLOUD_SERVER_STATE_REPORT       = 0
}CLOUD_SERVER_E;

typedef int (*ml_cloud_logic_notify_t)(int funcId, unsigned char *data, int size);
typedef OSStatus (*mqtt_pub_func)(const unsigned char* msg, unsigned int msg_len);

/********************************************************
 * function: mcloud_get_conn_state
 * description: 返回云端当前状态
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
unsigned char mcloud_get_conn_state( void );

#ifdef __cplusplus
}
#endif

#endif /* _CLOUD_H_ */
