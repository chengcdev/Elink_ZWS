/*
 * elink_logic_devchanel_deal.h
 *
 *  Created on: 2018年7月31日
 *      Author: hxfky
 */
#ifndef DEMOS_APPLICATION_MLINKDEMO_ELINK_LOGIC_DEVCHANEL_DEAL_H_
#define DEMOS_APPLICATION_MLINKDEMO_ELINK_LOGIC_DEVCHANEL_DEAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "MLinkAppDef.h"
#include "../queue/queue.h"
#include "mico.h"
#include "json_c/json.h"
#include "../ble/bluetooth_logic.h"
#define ELINK_FRAME_MAX_SIZE               (2048)
    typedef struct
    {
        int TcpFd;
        uint32_t dataSize;
        uint8_t data[ELINK_FRAME_MAX_SIZE];
    }TCP_FRAME_DATA_T, *PTCP_FRAME_DATA_T;

    typedef struct
    {
        uint8_t keytype;		 // 钥匙类型
        uint8_t mode;           // 1添加 3删除
        uint16_t userid;// 钥匙序号
        uint8_t adminpwd[10];// 管理密码
        uint8_t tmppwd[10];// 临时密码
        int start_time;// 生效时间
        int stop_time;// 失效时间
        uint8_t usecnt;// 可用次数
        uint8_t open_join_net;// 打开网关加网状态
        int time;// 开锁时间
    }TCP_SUBDEV_RSP_HEAD_T, *PTCP_SUBDEV_RSP_HEAD_T;

#define ELINK_OPERATION_SUCCESS      0
#define ELINK_OPERATION_FAIL         1

    typedef enum
    {
        ELINK_ALARM_LINGER_STATE = 0,     //有人逗留
        ELINK_ALARM_PASS_ATK_STATE = 1,//密码攻击
        ELINK_ALARM_HIJACK_STATE = 2,//挟持告警
        ELINK_ALARM_APP_ATK_STATE = 3,//APP密码攻击
        ELINK_ALARM_FINGER_ATK_STATE = 4,//指纹攻击
        ELINK_ALARM_CARD_ATK_STATE = 5,//卡片攻击
        ELINK_ALARM_BATTERY_WARN_STATE = 6//电池电量
    }ELINK_ALARM_STATE_E;

    typedef enum
    {
        ELINK_EVENT_INFO_LOCAL = 0,     //机械钥匙
        ELINK_EVENT_INFO_FINGER = 1,//指纹
        ELINK_EVENT_INFO_PASS = 2,//密码
        ELINK_EVENT_INFO_CARD = 3,//卡片
        ELINK_EVENT_INFO_APP = 4,//App远程开锁
        ELINK_EVENT_INFO_TMPAUTHPASS = 9,//a临时授权

    }ELINK_EVENT_INFO_E;

    typedef enum
    {
        ELINK_LOCK_STATE_NO_OPEN_LOCK = 0,     //代表锁未开启，门外无法打开门锁
        ELINK_LOCK_STATE_OPEN_LOCK = 1,//代表锁已开启，门外可以打开门锁
        ELINK_LOCK_STATE_ISLOCKED = 2,//代表锁已反锁
        ELINK_LOCK_STATE_ONLY_LOCAL = 3,//代表锁死状态，只可使用机械钥匙开启
        ELINK_LOCK_STATE_ABNORMAL = 4,//代表虚掩，状态异常
        ELINK_LOCK_STATE_CLOSE_NORMAL = 5,//代表门已关闭，门锁已上锁
        ELINK_LOCK_STATE_NO_CLOSE_ABNORMAL = 6,//代表门未关闭，门锁未上锁
        ELINK_LOCK_STATE_CLOSE_ISLOCKED = 7,//代表门已关闭，已反锁
        ELINK_LOCK_STATE_OTHER = 99//代表其他状态
    }ELINK_LOCK_STATE_E;

    typedef enum
    {
        ELINK_RETURN_OK = 0,     //成功
        ELINK_INVALID_PARA = 1,//参数非法
        ELINK_DEV_ID_NO_EXSIT = 2,//ID不存在
        ELINK_DEV_PIN_ERROR = 3,//PIN错误
        ELINK_DEV_AUTH_ERR = 4,//设备鉴权失败
        ELINK_DEV_NO_ONLINE = 5,//设备不在线
        ELINK_DEV_NO_STRCODE = 6,//设备不支持该命令
        ELINK_DEV_UNKOWN_ERR = 7,//设备侧未知错误
    }ELINK_RETURN_E;

    typedef struct
    {
        int TcpFd;
        int size;
        unsigned char *data;
    }TCP_RECV_PACKET, *PTCP_RECV_PACKET;

    typedef int (*tcp_recv_deal_callback)(char *codetype,PTCP_SUBDEV_RSP_HEAD_T tcp_elink_sub_rev);

#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_ELINK_LOGIC_DEVCHANEL_DEAL_H_ */
