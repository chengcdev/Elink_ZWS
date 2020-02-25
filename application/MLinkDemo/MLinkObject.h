/*
 * MLinkObject.h
 *
 *  Created on: 2017骞�7鏈�7鏃�
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKOBJECT_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKOBJECT_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "mico.h"
//#define TRUE     kNoErr
//#define FALSE    kGeneralErr

#define  VERSION_ENDPOINT_12    1           // 此版本一个子设备含12个端点


#define DEVICEID_SIZE           48//16
#define ROOMID_SIZE             DEVICEID_SIZE
#define OBJID_SIZE              24
#define ID_SIZE                 8
#define UUID_SIZE               48//16                  // max size is 16 byte.
#define NAME_SIZE               48
#define MAC_SIZE                20
#define MODELID_SIZE            12
#define KEY_SIZE                8
#define KEY_STR_SIZE            16
#define VALUE_SIZE              16
#define VALUE_SIZE_EX           32//64
#define VALUE_SIZE_EX_1         160//64
#define DT_SIZE                 32
#define VERIFY_SIZE             4
#define SUBDEV_MAC_SIZE         6
#define ENDPOINTID_SIZE         24
#define CTRL_CONTENT_SIZE       128
#define NET_ADDR_SIZE           12
#define VER_HARD_SIZE           16
#define VER_SOFT_SIZE           16
#define VER_PROT_SIZE           16
#define DEVLIST_SIZE            512
#define STATUS_SIZE             64
#define SUBDEVICE_STATUS_SIZE   256
#define LINKID_SIZE             8
#define OUTPUT_SIZE             64
#define INPUT_SIZE              64
#define LINKIDEX_SIZE           24

#define DEVICE_NUM              32
#define ROOM_DEVICE_NUM         20
#define SCENE_ACTION_NUM        32
#define LOOPACTION_ACTION_NUM   3
#define INPUTS_NUM              16
#define OUTPUTS_NUM             16
#define ENDPOINTLINK_NUM        8
#define LOCAL_ENDPOINT_NUM      2
#define LINKAGE_INPUT_NUM       16      // linkage task input
#define LINKAGE_OUTPUT_NUM      16      // linkage task output
#define LINKAGE_INPUT_TASK_NUM   32     // the number of input task
#define LINKAGE_OUTPUT_TASK_NUM  32     // the number of output task

#ifdef VERSION_ENDPOINT_12
#define ENDPOINTLIST_NUM        12      // The max endpoints's num is 16 in the device
#else
#define ENDPOINTLIST_NUM        16      // The max endpoints's num is 16 in the device
#endif

#define WILDCARD_STR            "FF"

#define KEY_DEVID                       "devid"
#define KEY_FID                         "fid"
#define KEY_ID                          "id"
#define KEY_SID                         "sid"
#define KEY_ADDR                        "addr"
#define KEY_UUID                        "uuid"
#define KEY_NAME                        "name"
#define KEY_MAC                         "mac"
#define KEY_CHANNEL                     "ch"
#define KEY_MODELID                     "modelid"
#define KEY_DEVTYPE                     "devtype"
#define KEY_STATUS                      "status"
#define KEY_COMM                        "comm"
#define KEY_OFFLINE                     "offline"
#define KEY_KEY                         "key"
#define KEY_ENDPOINTID                  "endpointid"
#define KEY_ENDPOINT                    "endpoint"
#define KEY_INPUT                       "input"
#define KEY_OUTPUT                      "output"
#define KEY_LINK                        "link"
#define KEY_INDEX                       "index"
#define KEY_LOGIC                       "logic"
#define KEY_CFLAG                       "cflag"
#define KEY_DO                          "do"
#define KEY_IN                          "in"
#define KEY_OUT                         "out"
#define KEY_CTRL                        "ctrl"
#define KEY_CLASSID                     "classid"
#define KEY_DEVLIST                     "devlist"
#define KEY_NOTITYPE                    "notitype"
#define KEY_OBJECTID                    "id"
#define KEY_ENABLED                     "enabled"
#define KEY_EVENTLEVEL                  "eventlevel"
#define KEY_EVENTTYPE                   "eventtype"
#define KEY_MSG                         "msg"
#define KEY_DT                          "dt"
#define KEY_FLAG                        "flag"
#define KEY_TYPE                        "type"
#define KEY_ACTIONLIST                  "actionlist"
#define KEY_ACTIONCODE                  "actioncode"
#define KEY_VALTYPE                     "valtype"
#define KEY_VALUE                       "value"
#define KEY_VALUNIT                     "valunit"
#define KEY_LINKID                      "linkid"
#define KEY_EVENTID                     "eventid"
#define KEY_RANGE                       "range"
#define KEY_DESC                        "desc"
#define KEY_CODE                        "code"
#define KEY_STEP                        "step"
#define KEY_UNIT                        "unit"
#define KEY_CMD                         "cmd"
#define KEY_VER                         "ver"
#define KEY_VERTYPE                     "vertype"
#define KEY_FORCE                       "force"
#define KEY_FIRMVER                     "firmver"
#define KEY_URL                         "url"
#define KEY_MD5                         "md5"
#define KEY_INNERID                     "innerid"
#define KEY_RESULT                      "result"
#define KEY_PROT                        "protocol"
#define KEY_FIRMVER                     "firmver"
#define KEY_KEYTYPE                     "keytype"

#define KEY_OBJECTLIST                  "objectlist"
#define KEY_OBJ                         "obj"
#define KEY_OBJECT                      "object"
#define KEY_OBJECTID_EX                 "objid"
#define KEY_OBJECTTYPE                  "objtype"
#define KEY_ATTR                        "attr"
#define KEY_LIMIT                       "limit"
#define KEY_PAGE                        "page"
#define KEY_PAGES                       "pages"
#define KEY_ITEM                        "item"

#define KEY_ASK							"ask"
#define KEY_APPID						"appid"
#define KEY_DSK							"dsk"
#define KEY_PSK							"psk"
#define KEY_CLOUDHOST					"cloudhost"
#define KEY_ROOMUID                     "room_uid"

#define KEY_SUBDEV                      "subdev"
#define KEY_MODELNAME                   "modelname"
#define KEY_HARDWARE                    "hardware"
#define KEY_SOFTVER                     "softver"
#define KEY_PROTOCOL                    "protocol"
#define KEY_VER_HARD                    "ver_hard"
#define KEY_VER_SOFT                    "ver_soft"
#define KEY_VER_PROTO                   "ver_proto"

#define KEY_TIME                        "time"
#define KEY_TIMEOUT                     "timeout"

#define KEY_SRCMAC                      "srcmac"


#define KEY_ALARM_STATE                 "30"
#define KEY_ALARM_DELAY                 "31"
#define KEY_IR_REPORT                   "604"
#define KEY_NET_PARAM                   "7"

#define KEY_BLE_OPERATE_RESULT          "1003"
#define KEY_BLE_LOCK_STATE              "1010"
#define KEY_BLE_LOCK_RECORD             "1011"
#define KEY_BLE_LOCK_GETINFO            "1022"
typedef enum{
    SYSTEM_OBJ_ID                       = 0,
    NETDEV_OBJ_ID,
    DEVICE_OBJ_ID,
    SIPDEV_OBJ_ID,
    DEVATTR_OBJ_ID,
    FILE_OBJ_ID,
    ENDPOINT_OBJ_ID,
    DEVSTATUS_OBJ_ID,
    SCENE_OBJ_ID                        = 10,
    ROOM_OBJ_ID,
    GROUP_OBJ_ID,
    DELAY_OBJ_ID,
    SCHEDULE_OBJ_ID,
    DEVINFO_OBJ_ID                      = 20,
    CLOUDPARAM_OBJ_ID,

    //
    LINKAGE_OUTPUT_OBJ_ID                = 100,
    LINKAGE_INPUT_OBJ_ID,
    ENDPOINTLINK_OBJ_ID                 = 110,

    EVENT_OBJ_ID                        = 50,
}OBJECT_ID_E;

typedef enum{
    SOFTWARE_VER                = 0,
    FIRMWARE_VER                = 1
}VER_TYPE_E;

typedef enum{
    ALARM_STATE             = 0,
    ALARM_DELAY             = 1
}ALARM_ENDPOINT_TYPE_E;

typedef enum{
    EVENTLEV_GLOBAL_WARNING             = 0,
    EVENTLEV_NOTIFY                     = 1,
    EVENTLEV_NORMAL                     = 2,
}EVENT_LEVEL_E;

typedef enum{
    EVENTTYPE_UNKNOW                    = 0,
    EVENTTYPE_DEVICE_CLOSE              = 1,
    EVENTTYPE_OUTAGE                    = 2,
    EVENTTYPE_WARNING_INFO              = 3,
    EVENTTYPE_FAULT_INFO                = 4,
    EVENTTYPE_STATUS_CHANGE             = 10,
    EVENTTYPE_VALUE_CHANGE              = 11,
    EVENTTYPE_COMMAND_FAIL              = 12,
    EVENTTYPE_OFFSET_THRESHOLD          = 13,
    EVENTTYPE_STRAY_OUTSIDE             = 14,
    EVENTTYPE_UNDERVOLTAGE              = 15
}EVENT_TYPE_E;

typedef enum{
    NOTIFY_ALARM                        = 0,
    NOTIFY_STATE_CHANGES                = 1,
    NOTIFY_EVENT                        = 2,
}NOTIFY_TYPE_E;

typedef enum{
    MESH_OUT                            = 0,        // 退出组网
    MESH_IN                             = 1         // 进入组网
}MESH_TYPE_E;

typedef enum{
    CMP_IGNORE                  = 0,
    CMP_EQUAL                   = 1,
    CMP_LESS                    = 2,
    CMP_LEQ                     = 3,
    CMP_GTR                     = 4,
    CMP_GEQ                     = 5,
    CMP_WITHIN                  = 6
}COMPARE_TYPE_E;

// network device object

typedef union{
    struct{
        uint8_t devStatus;
        uint8_t batteryStatus;
        uint8_t alarmType;
        uint8_t faultStatus;
    }status_info;
    uint32_t statusInfo;
}STATUS_INFO_T, *PSTATUS_INFO_T;

typedef struct {
    char deviceId[DEVICEID_SIZE];
    char uuid[UUID_SIZE];
    char name[NAME_SIZE];
    char mac[MAC_SIZE];
    char submac[VALUE_SIZE];
    char sublockpin[VALUE_SIZE];
    char addr[4];
    int  ch;
    char modelid[MODELID_SIZE];
    int  devtype;
    STATUS_INFO_T  status;
    int comm;
    char dsk[32];
    char psk[32];
    char cloudhost[64];
    char appid[16];
    char ask[32];
    char room_uid[16];
    char modelname[32];
}NETDEVOBJ_T, *PNETDEVOBJ_T;

typedef struct {
    char devid[UUID_SIZE];      // main device id
    char addr[4];              // panid
}SIP_NETDEVOBJ_T, *PSIP_NETDEVOBJ_T;

// device object
typedef struct {
    char deviceId[VALUE_SIZE];
    char uuid[VALUE_SIZE];
    char name[NAME_SIZE];
    char addr[NET_ADDR_SIZE];
    char mac[MAC_SIZE];
    int  ch;
    char modelid[MODELID_SIZE];
    int  devtype;
    STATUS_INFO_T  status;
    int  comm;
    int offline;
}DEVICEOBJ_T, *PDEVICEOBJ_T;

// simple device object
typedef struct {
    char sid[DEVICEID_SIZE];
    char name[NAME_SIZE];
    int  devtype;
    STATUS_INFO_T  status;
    int  comm;
}SIMPLEDEVICEOBJ_T, *PSIMPLEDEVICEOBJ_T;

// device status object
typedef struct {
    char devid[DEVICEID_SIZE];
    uint32_t liveTime;
    STATUS_INFO_T  status;
    uint32_t restTime;
}DEVSTATUSOBJ_T, *PDEVSTATUSOBJ_T;

// device operating capactity object
typedef struct {
    int  key;
    char value[VALUE_SIZE];
}DEVOPEROBJ_T, *PDEVOPEROBJ_T;

typedef struct{
    int  type;
    int  valtype;
    char key[KEY_SIZE];
    char value[VALUE_SIZE_EX];
}DEV_FUNCTION_OBJ_T, *PDEV_FUNCTION_OBJ_T;

typedef struct{
    int  type;
    int  valtype;
    char key[KEY_SIZE];
    char value[VALUE_SIZE_EX_1];
}DEV_FUNCTION_OBJ_EX_T, *PDEV_FUNCTION_OBJ_EX_T;

typedef struct{
    char keyType[8];
    DEV_FUNCTION_OBJ_T keyState;
}KEY_ATTR_T, *PKEY_ATTR_T;

typedef struct{
    char keyType[8];
    DEV_FUNCTION_OBJ_EX_T keyState;
}KEY_ATTR_T_EX, *PKEY_ATTR_T_EX;

// device's attribute object
typedef struct {
    char  devid[DEVICEID_SIZE];
    int  valtype;
    int  valunit;
    char name[NAME_SIZE];
    char value[VALUE_SIZE];
    char key[KEY_SIZE];
    int  linkid;
    int  eventid;
}DEVATTROBJ_T, *PDEVATTROBJ_T;

// action object
typedef struct {
    int  classid;
    int  id;
    int  actionCode;
    int  key;
    char value[VALUE_SIZE];
}ACTIONOBJ_T, *PACTIONOBJ_T;

// scene object
typedef struct {
    char sceneId[DEVICEID_SIZE];
    char sceneName[NAME_SIZE];
}SCENEOBJ_T, *PSCENEOBJ_T;

typedef struct {
    char roomId[ROOMID_SIZE];
    char name[NAME_SIZE];
    char devidList[DEVLIST_SIZE];
}ROOMOBJ_T, *PROOMOBJ_T;

typedef struct {
    int  classid;
    char uuid[UUID_SIZE];
    char key[KEY_SIZE];
    char value[VALUE_SIZE];

}LINK_DEVOBJ_INFO_T, *PLINK_DEVOBJ_INFO_T;

// device link object
typedef struct {
    int  id;
    char name[NAME_SIZE+1];
    int  inputNum;
    LINK_DEVOBJ_INFO_T InputInfo[INPUTS_NUM];       // json array input device infomation
    int  outputNum;                     // the number of output devices' info
    LINK_DEVOBJ_INFO_T OutInfo[OUTPUTS_NUM];     // json array output device infomation
}DEVLINKOBJ_T, *PDEVLINKOBJ_T;

typedef struct {
    int index;
    char logic[INPUT_SIZE];
}LINKAGE_INPUT_T, *PLINKAGE_INPUT_T;

// device link input object
typedef struct {
    char linkageId[LINKID_SIZE];
    unsigned int cflag;
    unsigned int inputNum;
    LINKAGE_INPUT_T lnkInput[LINKAGE_INPUT_NUM];
}LINKAGE_INPUT_TASK_T, *PLINKAGE_INPUT_TASK_T;

// device link output object
typedef struct {
    char linkageId[LINKID_SIZE];
    char name[NAME_SIZE];
    unsigned int cflag;
    unsigned int outputNum;
    char output[LINKAGE_OUTPUT_NUM][OUTPUT_SIZE];
}LINKAGE_OUTPUT_TASK_T, *PLINKAGE_OUTPUT_TASK_T;


typedef struct {
    uint16_t index;
    uint16_t delay;
}LINKAGE_INPUT_DELAY, *PLINKAGE_INPUT_DELAY;

typedef struct {
    uint16_t startIndex;        // start ouput from startIndex
    uint16_t delay;             // do output after delay times
}LINKAGE_OUTPUT_DELAY, *PLINKAGE_OUTPUT_DELAY;

typedef struct {
    char linkageId[LINKID_SIZE];
    unsigned int cflag;
    unsigned int cflagEffective;
    unsigned int lnkInputDelayNum;
    LINKAGE_OUTPUT_DELAY outputDelay;
    LINKAGE_INPUT_DELAY inputDelay[LINKAGE_INPUT_NUM];
}LINKAGE_STATUS_INFO_T, *PLINKAGE_STATUS_INFO_T;

typedef struct{
    char inEndpointId[ENDPOINTID_SIZE];
    char outEndpointId[ENDPOINTID_SIZE];
    char ctrlContent[CTRL_CONTENT_SIZE];
}LINK_OBJ_T, *PLINK_OBJ_T;

typedef struct{
    char linkId[LINKIDEX_SIZE];
    LINK_OBJ_T linkObj;
}ENDPOINTLINK_OBJ_T, *PENDPOINTLINK_OBJ_T;

typedef struct{
    int linkNum;
    LINK_OBJ_T linkArray[ENDPOINTLINK_NUM];
}ENDPOINTLINK_LIST_T, *PENDPOINTLINK_LIST_T;        // 用于存放最多含有8个端点的端点列表

// event object
typedef struct {
    char  id[ID_SIZE];
    char name[NAME_SIZE];
    int  eventlevel;
    int  eventtype;
    int  notitype;
    int  enabled;
    char key[128];
}EVENTOBJ_T, *PEVENTOBJ_T;


// schedule object
typedef struct {
    int  id;
    char name[NAME_SIZE];
    int  type;
    int  flag;
    char dt[DT_SIZE];
}SCHEDULEOBJ_T, *PSCHEDULEOBJ_T;

// loopaction
typedef struct {
    int  id;
    char name[NAME_SIZE];
    int  eventlevel;
    int  eventtype;
    ACTIONOBJ_T actionlist[LOOPACTION_ACTION_NUM];
}LOOPACTION_T, *PLOOPACTION_T;


// device info object
typedef struct {

    char ver[32];
    char firmver[32];

}DEVINFOOBJ_T, *PDEVINFOOBJ_T;

typedef struct {
    char mac[MAC_SIZE];
    char addr[NET_ADDR_SIZE];
    char modelid[MODELID_SIZE];
    char hardware[32];
    char software[32];
    char protocol[8];
    char firmver[32];
    uint32_t status;
}DEV_DETAIL_INFO_T, *PDEV_DETAIL_INFO_T;


typedef struct{
    char appid[16];
    char ask[16];
    char dsk[16];
    char psk[16];
    char cloudhost[16];
}CLOUDOBJ_T, *PCLOUDOBJ_T;



typedef struct{
    char id[ENDPOINTID_SIZE];
    char name[NAME_SIZE];
    int  devtype;
    int  keytype;
    char key[32];
}ENDPOINT_T, *PENDPOINT_T;

#ifdef VERSION_ENDPOINT_12
typedef struct{
    char id[ENDPOINTID_SIZE];
    char name[NAME_SIZE];
    int  devtype;
    int  keytype;
    char key[32];
    char value[VALUE_SIZE];
}ENDPOINT_EX_T, *PENDPOINT_EX_T;

#else
typedef struct{
    char fid[DEVICEID_SIZE];
    int  endpointNum;
    ENDPOINT_T endpointlist[ENDPOINTLIST_NUM];
}SINGLE_DEV_ENDPOINTOBJ_T, *PSINGLE_DEV_ENDPOINTOBJ_T;

typedef struct{
    ENDPOINT_T endpoint;
    char value[VALUE_SIZE];
}ENDPOINT_EX_T, *PENDPOINT_EX_T;

#endif

typedef struct{
    char fid[DEVICEID_SIZE];
    int  endpointNum;
    ENDPOINT_EX_T endpointlist[ENDPOINTLIST_NUM];
}ENDPOINTOBJ_EX_T, *PENDPOINTOBJ_EX_T;

// network transfer structure

typedef struct{
    uint16_t classId;
    char key[KEY_SIZE];
    //uint16_t key;
    char devid[DEVICEID_SIZE];
}ENDPOINT_ELE_T, *PENDPOINT_ELE_T;

#define IS_REPORT_STATUS_TIMER      1

#if IS_REPORT_STATUS_TIMER

typedef struct{
    char endpointId[ENDPOINTID_SIZE];
    KEY_ATTR_T keyAttr;
    char oldvalue[VALUE_SIZE_EX];
}ENDPOINT_STATUS_T, *PENDPOINT_STATUS_T;

#else
typedef struct{
    char endpointId[ENDPOINTID_SIZE];
    DEV_FUNCTION_OBJ_T keyAttr;
}ENDPOINT_STATUS_T, *PENDPOINT_STATUS_T;
#endif

typedef struct{
    char fid[DEVICEID_SIZE];
    uint32_t endpointNum;
    uint32_t addr;
    PENDPOINT_STATUS_T endpointStatus;
}DEV_ENDPOINT_STATUS_T, *PDEV_ENDPOINT_STATUS_T;

typedef struct{
    DEV_ENDPOINT_STATUS_T devEndpoints;
    void *devNext;
}DEVICE_ATTRS_STATUS_T, *PDEVICE_ATTRS_STATUS_T;

typedef struct{
    char devid[DEVICEID_SIZE];
//    ENDPOINT_T endpointlist[2];
    uint8_t endpointNum;
    uint8_t alarmType;
    uint8_t alarmDelay;
    uint8_t workState;              // 1: protection  0:removal
}ALARM_ENDPOINT_T, *PALARM_ENDPOINT_T;

typedef struct{
    ALARM_ENDPOINT_T alarmEndpoint;
    // other endpoint info
}LOCAL_ENDPOINT_T, *PLOCAL_ENDPOINT_T;

typedef struct{
    uint32_t alarmCount;
    ALARM_ENDPOINT_T devAlarm[DEVICE_NUM];
}ALARM_RECORD_T, *PALARM_RECORD_T;

typedef struct{
    char *inValue;
    char *outType;
    char *outData;
}CTRL_PARAM_T, *PCTRL_PARAM_T;

typedef struct{
    char srcmac[24];
    char addr[32];
}SENDADDR_PARAM_T, *PSENDADDR_PARAM_T;

typedef struct{
    char linkId[LINKID_SIZE];
    int index;
}CRON_TASK_INFO, *PCRON_TASK_INFO;

typedef struct{
    char srcmac[24];
    char addr[32];
    char uuid[UUID_SIZE];
    char panid[4];
}NETDEV_ADDR_INFO, *PNETDEV_ADDR_INFO;

typedef struct{
    int classid;
    char uuid[UUID_SIZE];
    char devid[DEVICEID_SIZE];
    char addr[NET_ADDR_SIZE];
    char key[KEY_SIZE];
}SYN_STATUS_OBJ_T, *PSYN_STATUS_OBJ_T;

typedef struct
{
    int num;
    PSYN_STATUS_OBJ_T synstatusObj;
}SYN_STATUS_T, *PSYN_STATUS_T;


#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKOBJECT_H_ */
