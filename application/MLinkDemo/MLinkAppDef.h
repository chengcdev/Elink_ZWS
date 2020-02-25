/**
******************************************************************************
* @file    MLinkAppEntrace.h
* @author  huangxf
* @version V1.0.0
* @date    2017骞�5鏈�23鏃�
* @brief   This file provides xxx functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2017 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
******************************************************************************
*  @brief:
*  当代码用作吸顶式网关的时候，即#define DEVICE_TYPE  DEVICE_SUCK_TOP_ZIGBEE .
*  需要更改 "platform_config.h" 文件和  "platform.h" 下的
*  #define SUCK_TOP_DEVICE    1    (20180907)
******************************************************************************
*/ 
#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKAPPDEF_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKAPPDEF_H_

#include "MLinkDebug.h"
#include "mico_rtos.h"

#include "MLinkObject.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define DEVICE_RF433             0
#define DEVICE_ZIGBEE            1
#define DEVICE_SUCK_TOP_ZIGBEE   2
#define DEVICE_SUCK_TOP_RF433    3
#define DEVICE_BLE               4
#define DEVICE_ELINK_BLE         5

#define RF433_MID           "010001"
#define ZIGBEE_MID          "010002"


#define DEVICE_TYPE         DEVICE_ELINK_BLE

#if (DEVICE_TYPE == DEVICE_RF433)       //RF433网关
    #ifndef SUCK_TOP_DEVICE
        #define SUCK_TOP_DEVICE    0
    #endif
    #define RF433_DEVICE          1
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "MS-G5112F"
    #define YL_PROTOCOL          "MiLink"

    #define SOFT_VER            "1.2.191"
    #define MODEL_ID            "010001"
#elif (DEVICE_TYPE == DEVICE_ZIGBEE)     // zigbee网关
    #ifndef SUCK_TOP_DEVICE
        #define SUCK_TOP_DEVICE    0
    #endif
    #define ZIGBEE_DEVICE          1
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "MS-G5112G"
    #define YL_PROTOCOL          "MiLink"

    #define SOFT_VER            "1.2.191"
    #define MODEL_ID            "010002"
#elif(DEVICE_TYPE == DEVICE_SUCK_TOP_ZIGBEE)  // 吸顶式zigbee网关
    #ifndef SUCK_TOP_DEVICE
        #define SUCK_TOP_DEVICE    1
    #endif
    #define ZIGBEE_DEVICE          1
    #undef ZIGBEE_MID
    #define ZIGBEE_MID          "010012"
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "MS-G5122G"
    #define YL_PROTOCOL          "MiLink"

    #define SOFT_VER            "1.0.1"
    #define MODEL_ID            "010012"
#elif(DEVICE_TYPE == DEVICE_SUCK_TOP_RF433)
    #ifndef SUCK_TOP_DEVICE
        #define SUCK_TOP_DEVICE    1
    #endif
    #define RF433_DEVICE          1
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "MS-G5112F"
    #define YL_PROTOCOL          "MiLink"

    #define SOFT_VER            "1.2.1901"
    #define MODEL_ID            "010001"
#elif(DEVICE_TYPE == DEVICE_BLE)

    #define BLE_DEVICE          1
    #define SCENE_LINK          1
    #define  BLE_ELINK          1
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "MS-G5112B"
    #define YL_PROTOCOL          "MiLink"
    #define SOFT_VER            "1.0.0"
    #define MODEL_ID            "010004"    //BLE网关
#elif(DEVICE_TYPE == DEVICE_ELINK_BLE)
    #define BLE_DEVICE          1
    #define SCENE_LINK          1
    #define  BLE_ELINK          1
    #define YL_FIRMWARE_REV     "308002.027"
    #define YL_MANUFACTURER      "ML."
    #define YL_MODELNAME         "ELINKZWS"
    #define YL_PROTOCOL          "MiLink"
    #define SOFT_VER            "1.0.0"
    #define MODEL_ID            "02C324"    //elink指纹锁
#endif

#define MLINK_DEBUG
#define TEST                2
#if (TEST == 1)
#define DEV_ID              "00B03000FF010410014453904847955389"    // 01
#elif(TEST == 2)
#define DEV_ID              "00B03000FF010410014170829966587436"    // 02
//#define DEV_ID              "00B1450104100110013856828796606523"
#elif(TEST == 3)
#define DEV_ID              "00B03000FF010410010349777855769314"    // 03
#endif

#define DEV_MODEL_NAME		"MiLink-GW"
#define GATEWAY_TYPE            3           // it means gateway

#define APPID_DEFAULT			"0000"
#define APP_SECRET				"mNOso0cBv29dELjH"

#define DEVICE_SECRET_DEFAULT	"0000"
#define PSK_DEFAULT             "81a&PBM6"//"tRLsxE7iqzj4SKce"

#define CLOUD_PROTOCOL_VER		"1.0"


#define CLOUD_HOST			"182.92.183.137:21883"//"yun.miligc.com"

#define ROOM_KEY            "RoomKey"


/* Demo C function call C++ function and C++ function call C function */
//#define MICO_C_CPP_MIXING_DEMO

/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000002 // if default configuration is changed, update this number
#define MAX_QUEUE_NUM                       6  // 1 remote client, 5 local server
#define MAX_QUEUE_LENGTH                    8  // each queue max 8 msg

#define LOCAL_PORT                          8080


#define MLINK_BONJOUR_SERVICE                   "_MLinkDev_mlnet._tcp.local."//"_MLinkDev._mlnet.local."  //"_easylink._tcp.local."
//#define BONJOUR_SERVICE                     	"_Mlink._tcp.local."

/* Define thread stack size */
//#define STACK_SIZE_UART_RECV_THREAD           0x2A0
//#define STACK_SIZE_LOCAL_TCP_SERVER_THREAD    0x300
//#define STACK_SIZE_LOCAL_TCP_CLIENT_THREAD    0x350
//#define STACK_SIZE_REMOTE_TCP_CLIENT_THREAD   0x500

//    typedef struct _socket_msg {
//      int ref;
//      int len;
//      uint8_t data[1];
//    } socket_msg_t;
#define SEND_ADDR_NUM_MAX           10


//
typedef enum{
    SYS_MESH_STATE                  = 0,
    SYS_UPGRADE_STATE               = 1,
    SYS_ALARM_STATE                 = 2,
    SYS_ONLINE_STATE                = 3,
    SYS_WIFI_STATE                  = 4,
    SYS_EASYLINK_STATE              = 5,
    SYS_UPDTAE_STATE                = 6
}SYSTEM_STATE_E;

/*Application's configuration stores in flash*/

/*** cloud param stores  **/
typedef struct
{
	char appid[16];
	char appSecret[34];
	char devSecret[34];
	char productSecret[34];
	char cloudhost[128];
}app_config_cloud_t;

typedef struct
{
	char devid[48];
	char uuid[48];
	char name[32];
	char mac[16];
	char modelid[16];
	char devSecret[34];
	char productSecret[34];
	char appid[16];
	char appSecret[34];
	unsigned char ch;
	unsigned int devtype;
	unsigned int status;
}app_config_maindev_t;

typedef struct
{
	app_config_maindev_t maindev;
	app_config_cloud_t	 cloud;
	NETDEVOBJ_T			netdev;
	DEVINFOOBJ_T        devinfo;
	LOCAL_ENDPOINT_T    netdevEndpoint;//endpointStatus;
	ALARM_RECORD_T      alarmRecord;
} application_config_t;

typedef struct{
    uint8_t     setmeshStatus;            // 1: mesh; 0: unmesh
    uint8_t     upgradeStatus;            // 1: coap upgrade; 2: mqtt upgrade; 0: no upgrade;
    uint8_t     onlineStatus;
    uint8_t     alarmState;
    uint8_t     wlanStatus;
    uint8_t     easylinkStatus;
    uint8_t     sysUpdateStatus;
    uint8_t     reverse;                // 预留位
}GATEWAY_STATUS_T, *PGATEWAY_STATUS_T;
#if 0
typedef struct{
    int addrNum;
    SENDADDR_PARAM_T addrInfo[SEND_ADDR_NUM_MAX];
}SEND_ADDR_T, *PSEND_ADDR_T;
#else
typedef struct{
    int addrNum;
    NETDEV_ADDR_INFO addrInfo[SEND_ADDR_NUM_MAX];
}SEND_ADDR_T, *PSEND_ADDR_T;
#endif
/*Running status*/
typedef struct  {
  /*Local clients port list*/
//  mico_queue_t*   socket_out_queue[MAX_QUEUE_NUM];
//  mico_mutex_t    queue_mtx;
  GATEWAY_STATUS_T run_status;
  DEVSTATUSOBJ_T   dev_status;
  SEND_ADDR_T      sendAddr;
} current_app_status_t;

typedef struct _app_context_t
{
  /*Flash content*/
  application_config_t*     appConfig;

  /*Running status*/
  current_app_status_t      appStatus;
} app_context_t;


#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKAPPDEF_H_ */
