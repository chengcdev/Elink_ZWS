/*
 * uart_logic.h
 *
 *  Created on: 2017年9月8日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_UART_UART_LOGIC_H_
#define DEMOS_APPLICATION_MLINKDEMO_UART_UART_LOGIC_H_

#ifdef __cplusplus
extern "C"{
#endif
#include "MLinkAppDef.h"
#include "../queue/queue.h"

/*uart function define*/
#if SUCK_TOP_DEVICE
#define UART_MLINK_PIC          MICO_UART_2
#else
#define UART_MLINK_PIC          MICO_UART_1
#endif

#define UART_DRIVER_BUFFER_LENGTH       (1024)
#define UART_ONE_PACKAGE_LENGTH         (128)
#define TP_FRAME_MAX_SIZE               (128)
#define ML_PARAM_MAX_SIZE       (TP_FRAME_MAX_SIZE-16)
#define UART_RECV_TIMEOUT               (25)

/***********************ZIGBEE COMMUNICATE COMMAND********************/
#define DEV_CMD_REBOOT                      0x01
#define DEV_CMD_FACTORY                     0x02
#define DEV_CMD_START_SETMESH               0x03
#define DEV_CMD_NET_SETTING_ENABLE          0x04
#define DEV_CMD_NET_RESET                   0x05
#define DEV_CMD_SEARCH_DEV                  0x06
#define DEV_CMD_GET_DEV_STATUS              0x07
#define DEV_CMD_SET_SHORTADDR               0x08
#define DEV_CMD_CLEAR_DATA                  0x0B
#define DEV_CMD_WRITE_REG                   0x11
#define DEV_CMD_READ_REG                    0x12
#define DEV_CMD_SAVE_DATA                   0x13
#define DEV_CMD_CLEAR_GROUP_ADDR            0x21
#define DEV_CMD_ADD_GROUP_ADDR              0x22
#define DEV_CMD_DEL_GROUP_ADDR              0x23
#define DEV_CMD_QUERY_GROUP_ADDR            0x24
#define DEV_CMD_PASSTHROUGH                 0x31
#define DEV_CMD_GET_NET_PARAM               0x32
#define DEV_CMD_SET_NET_PARAM               0x33

/**************************pass-through command**********************/
#define ML_REBOOT               1
#define ML_RESET                2

#define ML_ADD_SCENE            0X11
#define ML_DEL_SCENE            0X12
#define ML_CLR_SCENE            0X13
#define ML_CALL_SCENE           0X14
#define ML_GET_SCENE            0X15

#define ML_ADD_GROUP            0X21
#define ML_DEL_GROUP            0X22
#define ML_CLR_GROUP            0X23
#define ML_GROUP_READ           0X24
#define ML_GROUP_WRITE          0X25
#define ML_GET_GROUP            0X28

#define ML_DP_READ              0X31
#define ML_DP_WRITE             0X32
#define ML_DP_READ_HUGE         0X33
#define ML_DP_WRITE_HUGE        0X34
#define ML_DP_RESET             0X35
#define ML_DP_SET_REPORT        0X36
#define ML_DP_DEV_REPORT        0X37
#define ML_DP_GROUP_REPORT      0X38
#define ML_DP_SINGLE_REPORT     0X39
#define ML_DP_QUERY_ALL         0x3b
#define ML_DP_WRITE_FORWORDED   0x3c
#define ML_DP_STATE_NOTIFY      0x3d
#define ML_DP_STATE_OBSERVE_DEV 0x3e
#define ML_DP_STATE_OBSERVE_TYPE 0x3f
#define ML_DP_HEART          0x101

#define UART_SEND_MAIN_HEAD_SIZE    sizeof(UART_HEAD_T)
#define UART_SEND_SUB_HEAD_SIZE     sizeof(UART_SUBDEV_HEAD_T)
#define UART_SEND_HEAD_SIZE         UART_SEND_MAIN_HEAD_SIZE
#define UART_SEND_PTH_HEAD_SIZE     (UART_SEND_MAIN_HEAD_SIZE+UART_SEND_SUB_HEAD_SIZE)

#define UART_RSP_MAIN_HEAD_SIZE     sizeof(UART_RSP_HEAD_T)
#define UART_RSP_SUB_HEAD_SIZE      sizeof(UART_SUBDEV_RSP_HEAD_T)

#define UART_RSP_HEAD_SIZE          UART_RSP_MAIN_HEAD_SIZE
#define UART_RSP_PTH_HEAD_SIZE      (UART_RSP_HEAD_SIZE+UART_RSP_SUB_HEAD_SIZE)

#define UART_SEND_RSP_HEAD_SIZE     sizeof(UART_SEND_RSP_HEAD_T)
#define UART_SEND_RSP_PTH_HEAD_SIZE (UART_SEND_RSP_HEAD_SIZE+UART_RSP_SUB_HEAD_SIZE)


#define DEVICE_HEAT                 0x10
#define KEY_STATE_SIZE              128

typedef enum {
    PROP_TYPE,
    PROP_RW,
    PROP_RUN,
    PROP_DATA,
    PROP_REPORT,
    PROP_ADDR,
    PROP_GROUP,
    PROP_SCENE,
    PROP_EXT_CTRL,
    PROP_MAX
}PROP_TYPE_E;

typedef enum{
    DEV_NODE_UNCONFIG                   = 0,        // route or coordinator
    DEV_NODE_COORDINATOR                = 1,        // coordinator
    DEV_NODE_ROUTER                     = 2,        // router
    DEV_NODE_TERMINAL                   = 3,        // terminal device
    DEV_NODE_BUS                        = 4         // bus device
}DEV_NODE_TYPE_E;

typedef enum{
    SETMESH_COORDINATOR             = 0,        // route or coordinator
    SETMESH_ROUTER                  = 1,        // coordinator
    SETMESH_TERMINAL                = 2,        // router
    SETMESH_TERMINAL_RECV           = 3
}SETMESH_DEVICE_TYPE_E;

typedef enum{
    SUB_NET_STATE_HAND_UP           = 0,                // 初始状态需要发送启动协调器  未组网
    SUB_NET_STATE_INITIAL           = 1,                // 组网中
    SUB_NET_STATE_SEARCH            = 2,                // 组网中
    SUB_NET_STATE_JOINT             = 3,                // 组网中
    SUB_NET_STATE_RESTART_JOINT     = 4,                // 组网中
    SUB_NET_STATE_TERMINAL_UNAUTHORIZE = 5,             // 组网中
    SUB_NET_STATE_TERMINAL          = 6,        // 终端                 组网成功
    SUB_NET_STATE_ROUTE             = 7,        // 路由                 组网成功
    SUB_NET_STATE_CODNT_STARTING    = 8,        // 协调器启动中       组网中
    SUB_NET_STATE_CODNT             = 9,        // 协调器             组网成功
    SUB_NET_STATE_ORPHAN            = 10,       // 孤儿节点
    SUB_NET_STATE_UNDEFINE          = 11        //
}SUB_DEV_NET_STATE_E;

typedef enum{
    UART_RESP_STAT_OK               = 0,
    UART_RESP_STAT_WR_E2PROM_ERR    = 0xf8,
    UART_RESP_STAT_ERR_OPERATE      = 0xf9,
    UART_RESP_STAT_MEMORY_OVERFLOW  = 0xfa,
    UART_RESP_STAT_MEMORY_ERR       = 0xfb,
    UART_RESP_STAT_WRONG_LENGTH     = 0xfc,
    UART_RESP_STAT_PARAM_ILLEGAL    = 0xfd,
    UART_RESP_STAT_CMD_ILLEGAL      = 0xfe,
    UART_RESP_STAT_CACH_OVERFLOW    = 0xff,
}UART_RESP_STAT_E;

typedef enum{
    CONFIG_DIR_SEND                 = 0x00,
    CONFIG_COM_ACK                  = 0x40,
    CONFIG_SEND_ACK                 = 0x80,
    CONFIG_SEND_RSP_ACK             = 0xc0
}FRAME_CONFIG_E;

#pragma pack(1)
typedef struct{
    uint16_t shortAddr;
    unsigned char panid;
}UART_SUBDEV_ADDR, *PUART_SUBDEV_ADDR;

typedef union{
    struct{
        uint16_t op      :11;
        uint16_t dir     :1;
        uint16_t resp    :1;
        uint16_t batch   :1;
        uint16_t reserve :2;
    }op_struct;
    uint16_t op;
}UART_PTH_OP, *PUART_PTH_OP;

typedef struct{
    unsigned char frame_head;
    unsigned char config;
    unsigned char frame_len;
    unsigned char function_code;
    uint16_t srcAddr;
}UART_HEAD_T, *PUART_HEAD_T;

typedef struct{
    unsigned char frame_head;
    unsigned char config;
    unsigned char frame_len;
    unsigned char function_code;
    uint16_t destAddr;
}UART_SEND_RSP_HEAD_T, *PUART_SEND_RSP_HEAD_T;

typedef struct{
    unsigned char frame_head;
    unsigned char config;
    unsigned char frame_len;
    unsigned char function_code;
}UART_RSP_HEAD_T, *PUART_RSP_HEAD_T;

typedef struct{
    unsigned char     mid[3];
    UART_SUBDEV_ADDR  daddr;
    unsigned char     spand;
    UART_PTH_OP       u_op;
}UART_SUBDEV_HEAD_T, *PUART_SUBDEV_HEAD_T;

typedef struct{
    unsigned char     mid[3];
    UART_SUBDEV_ADDR  daddr;
    unsigned char     spand;
    UART_PTH_OP       u_op;
    unsigned char     state;
}UART_SUBDEV_RSP_HEAD_T, *PUART_SUBDEV_RSP_HEAD_T;

typedef struct{
    int size;
    unsigned char *data;
}UART_RECV_PACKET, *PUART_RECV_PACKET;

typedef struct{
    uint8_t  nodeType;
    uint64_t IEEEAddr;
    uint16_t ShortAddr;
    uint8_t  devtype[3];
    uint16_t hardware;
    uint16_t software;
    uint8_t  protVer;
}DISC_DEV_INFO_T, *PDISC_DEV_INFO_T;

typedef struct{
    uint8_t mac[8];
    uint32_t addr;
    uint8_t mid[3];
    uint8_t comm;
}DISC_RFDEV_INFO_T, *PDISC_RFDEV_INFO_T;

#pragma pack()

typedef struct{
    uint32_t dataSize;
    uint8_t  data[TP_FRAME_MAX_SIZE];
}UART_FRAME_DATA_T, *PUART_FRAME_DATA_T;

typedef int (*uart_recv)(int fincId, char *data, int size);
typedef int (*uart_recv_deal_callback)(PUART_RECV_PACKET recvPacket);
typedef void (*discover_notify_callback_t)(PDISC_DEV_INFO_T subDevInfo);




#ifdef __cplusplus
}
#endif



#endif /* DEMOS_APPLICATION_MLINKDEMO_UART_UART_LOGIC_H_ */
