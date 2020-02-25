/* ml_coap.h -- ml coap header file for ML CoAP stack
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef _ML_COAP_H_
#define _ML_COAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/include_coap.h"

#define NDEBUG

/**是否启用数据验证功能**/
#define IS_USE_MLCOAP_Authorized    0// 1

/** 是否启用数据加密功能**/
#define IS_USE_MLCOAP_ENCRYPT       0// 1


#define ML_COAP_SYS_MCAST_ADDR      "239.0.0.224"
#define ML_COAP_DEFAULT_PORT        "5683"
#define ML_COAP_DEFAULT_ADDR        "0.0.0.0"
#define ML_COAP_DEFAULT_ADDR_IPV6   "::"

#define ML_COAP_OPTION_Authorized       55
#define ML_COAP_OPTION_Encrypt          56

// #define ML_COAP_RESOURCE_CHECK_TIME      2
#define ML_COAP_MAX_OBS                 128

#define ML_COAP_SERVICE_OFFSET          1000
#define ML_COAP_OBS_MAXLEN              1024

#define ML_COAP_DEFAULT_TOKEN           "mlink"

#define URI_COAP                "coap"
#define URI_SERVICE_SYS         "1.0/service/sys"
#define URI_SERVICE_MANAGER     "1.0/service/manager"
#define URI_SERVICE_CTRL        "1.0/service/ctrl"
#define URI_SERVICE_MSG         "1.0/service/eventmsg"
#define URI_SERVICE_DISCOVER    "1.0/service/discover"

//#define URI_DISCOVER_MulitCast   "coap://239.0.0.224/api/1.0/service/discover"
#define Hostname_MulitCast      "239.0.0.224"
#define TOKEN_LENGTH_MAX        16
/** 接口服务类型**/
typedef enum{
    ML_COAP_URITYPE_SYS =0,
    ML_COAP_URITYPE_MANAGER,
    ML_COAP_URITYPE_CTRL,
    ML_COAP_URITYPE_MSG,
    ML_COAP_URITYPE_DISCOVER,
}ML_COAP_URITYPE_E;

/** 请求应答包结构**/
typedef struct ml_coap_reqPacket_t {
    unsigned short id;          /* transaction id (network byte order!) */
    unsigned char token[TOKEN_LENGTH_MAX];      /* the actual token, if any */
    unsigned char *srcaddr;
    unsigned char encrypt[8];
    unsigned char verify[8];
    unsigned short length;      /**< PDU length (including header, options, data)  */
    unsigned char *data;        /**< payload */
} ml_coap_reqPacket_t;

/**  应答数据包结构 **/
typedef struct ml_coap_ackPacket_t
{
    unsigned int  echoValue;
    unsigned int datalen;
    unsigned char *data;
}ml_coap_ackPacket_t;

typedef struct ml_coap_revMsg_t
{
    unsigned int  srcAddr;
    unsigned int echoValue;
    unsigned int msgLen;
    unsigned char *msgData;
}ml_coap_revMsg_t;

typedef struct ml_coap_notifyMsg_t
{
    unsigned short uriType;
    unsigned short notifySize;
    unsigned char notifyData[ML_COAP_OBS_MAXLEN];
}ml_coap_notifyMsg_t;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef int (*coap_logic_notify_t)(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket);
typedef int (*coap_service_data_deal_t)(ML_COAP_URITYPE_E service, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket);

/** coap 服务接口回调处理函数声明 **/
typedef int (*ml_coap_restful_deal_handler_t)(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket);

/**
 *  init ml coap   
 *
 * @return >=0 success  <0 error.
 */
int  ml_coap_init();
/**
 *  轮询侦听coap sock
 *
 * @return 
 */
void ml_poll_coap(void);
/**
 *  退出销毁coap
 *
 */
void ml_exit_coap(void);

/**
 *  coap 注册接口服务回调处理
 *
 * @param uritype    订阅观察服务类型
 * @param handle   对应接口服务的回调处理函数句柄
 */
void ml_coap_reg_handler(ML_COAP_URITYPE_E uritype,ml_coap_restful_deal_handler_t *handle);

/**
 *   往订阅者通知订阅的数据内容
 *
 * @param uritype    订阅观察服务类型
 * @param notifydata 通知的数据内容
 * @param datalen  通知数据长度
 * @return >=0on success or <0 on error.
 */
int  ml_coap_notify_obs(ML_COAP_URITYPE_E uritype , char *notifydata, unsigned int datalen);


#ifdef __cplusplus
}
#endif

#endif /* _ML_COAP_H_ */
