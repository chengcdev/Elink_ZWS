/*
 * MLinkCommand.h
 *
 *  Created on: 2017年7月6日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKCOMMAND_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKCOMMAND_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "MLinkObject.h"
#include "flash/flash_storage_object.h"

#define PUBLIC_CMD            "cmd"
#define PUBLIC_DATA           "data"
#define PUBLIC_ERRCODE        "errorcode"
#define PUBLIC_ERRMSG         "errormsg"
#define PUBLIC_BODY           "body"

#define SERVICE_UNKNOWN        "unknow"


/***********************System Service**************************/

#define SYS_SETMESH           "setmesh"
#define SYS_RESET             "reset"
#define SYS_FACTORY           "factory"
#define SYS_SYNCTIME          "synctime"
#define SYS_SETNET            "setnet"
#define SYS_REGDEV            "regdev"
#define SYS_SETCLOUD          "setcloud"
#define SYS_GETDEVINFO        "getdevinfo"
#define SYS_KEEPALIVE         "keepalive"
#define SYS_SETADDR           "setaddr"
#define SYS_UPGRADE           "upgrade"
#define SYS_RESETMESH         "resetmesh"
#define SYS_UPGRADERESULT     "upgraderesult"
#define SYS_INIT              "init"
#define SYS_SENDADDR          "sendaddr"
#define SYS_CLEARDATA        "cleardata"

/****************************************************************/

/*********************** Discovering Service ********************/

#define DISCOVER_WHO_IS         "who-is"
#define DISCOVER_I_AM           "i-am"
#define DISCOVER_WHO_HAV        "who-hav"
#define DISCOVER_I_HAV          "i-hav"

/****************************************************************/

//===============================================================

/************************ Manager Service ***********************/
#define MANAGER_GET             "get"                 // get the object
#define MANAGER_ADD             "add"                 // add a new object
#define MANAGER_DEL             "del"                 // delete a object
#define MANAGER_CLEAR           "clear"               // delete all object
#define MANAGER_UPDATE          "update"              // update the object


/****************************************************************/

//===============================================================

/************************* Control Service **********************/
#define CTRL_READ                "read"
#define CTRL_WRITE               "write"
#define CTRL_READMULT            "readmult"
#define CTRL_WRITEMULT           "writemult"
#define CTRL_WRITEGROUP          "writegroup"
#define CTRL_SYNCSTATUS          "syncstatus"
#define CTRL_DEVSET              "devset"
#define CTRL_GET                 "get"
#define CTRL_SET                 "set"
#define CTRL_DEL                 "del"
#define CTRL_CLEAR               "clear"

/****************************************************************/

//===============================================================

/************************ Event Message Service *****************/

#define MESSAGE_EVENT                "event"           // report the event
#define MESSAGE_CHANGENOTIFY         "changenotify"    // notify the change of property
#define MESSAGE_LINKTASK             "linktask"
#define MESSAGE_STATECHANGE         "statechange" //"statuschange"//
#define MESSAGE_STATUSCHANGE         "statuschange"//
#define MESSAGE_ALLOW_NOTIFY         "allownotify"

/***************************************************************/

//===============================================================

/**********************Command Param String**********************/
#define PARAM_UUID                    "uuid"
#define PARAM_FLAG                    "flag"
#define PARAM_SSID                    "ssid"
#define PARAM_KEY                     "key"
#define PARAM_SECURITY                "security"



/*********************Command Param Struction********************/
typedef struct{

    char devid[16];
    uint16_t flag;
    uint16_t timeout;
}SETMESH_T, *PSETMESH_T;

typedef struct{
    char ver[16];
    char md5[48];
    char url[128];
    char innerid[20];
    int vertype;
    int force;
}UPGRADE_PARAM_T, *PUPGRADE_PARAM_T;

typedef struct{
    char ssid[32];
    char key[32];
    int  security;
}NET_PARAM_T, *PNET_PARAM_T;


typedef struct{
    char netDevId[48];
    char result[8];
    char innerid[20];
}UPGRADE_RESULT_PARAM_T, *PUPGRADE_RESULT_PARAM_T;

typedef struct{
    char ver[16];
    int vertype;
}NETDEV_MQTT_INITINFO, *PNETDEV_MQTT_INITINFO;


typedef struct{
    int  class;
    char devid[OBJID_SIZE];
    char keystr[KEY_SIZE];
    int  type;
}OBJATTR_T, *POBJATTR_T;

typedef struct{
    char devid[OBJID_SIZE];
    int  attrnum;
    PDEV_FUNCTION_OBJ_EX_T devattrobj;
}DEVATTR_READ_T, *PDEVATTR_READ_T;

typedef struct{
    int  devnum;
    PDEVATTR_READ_T devattrreadobj;
}DEVATTR_READMULT_T, *PDEVATTR_READMULT_T;

typedef struct{
    int  classID;
    char uuid[UUID_SIZE];
    char devid[OBJID_SIZE];
    char netaddr[NET_ADDR_SIZE];
    int num;
    PDEV_FUNCTION_OBJ_EX_T opobj;
}DEVATTR_WRITE_T, *PDEVATTR_WRITE_T;

typedef struct{
    int devnum;
    PDEVATTR_WRITE_T write_attr;
}DEVATTR_WRITEMULT_T, *PDEVATTR_WRITEMULT_T;

typedef struct{
    char devid[32];
    char cmd[16];
    int  objtype;
    char  objid[OBJID_SIZE];
    int  limit;
    int  page;
    char pageInfo[32];
    int  attrNum;
    PDEV_FUNCTION_OBJ_T  opobj;
}DEVSET_INFO_T, *PDEVSET_INFO_T;

typedef struct{
    int totalnum;
    int itemperpage;
    int pagenum;
    int pageindex;
    int itemsCurPage;
}OBJECT_PAGE_INFO, *POBJECT_PAGE_INFO;

typedef struct{
    char endpointid[ENDPOINTID_SIZE];
    char uuid[UUID_SIZE];
    char subdevid[DEVICEID_SIZE];
    char addr[NET_ADDR_SIZE];
    char status[SUBDEVICE_STATUS_SIZE];
}STATE_OBJ_T, *PSTATE_OBJ_T;

typedef struct{
    char devid[DEVICEID_SIZE];          // main device ID
    char status[STATUS_SIZE];
    int subdevNum;
    PSTATE_OBJ_T stateObj;
}REPORT_STATE_CHANGE_T, *PREPORT_STATE_CHANGE_T;

typedef struct{
    char endpointid[ENDPOINTID_SIZE];
    char uuid[UUID_SIZE];
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
}MSG_CONTENT_T, *PMSG_CONTENT_T;

typedef struct{
    int eventlevel;          // main device ID
    int eventtype;
    int notitype;
    MSG_CONTENT_T msgContent;
}EVENT_REPORT_T, *PEVENT_REPORT_T;

typedef struct{
    char fid[DEVICEID_SIZE];
    char modelid[MODELID_SIZE];
    char mac[MAC_SIZE];
    int  comm;
    char netaddr[NET_ADDR_SIZE];
    char verHardware[VER_HARD_SIZE];
    char verSoftware[VER_SOFT_SIZE];
    char verProt[VER_PROT_SIZE];
}REPORT_SUBDEV_INFO_T, *PREPORT_SUBDEV_INFO_T;

typedef struct
{
    char sceneId[8];
    int valtype;
    char value[VALUE_SIZE];
}SCENE_INFO, *PSCENE_INFO;


typedef struct
{
    int sceneNum;
    char key[KEY_SIZE];
    SCENE_INFO sceneInfo[SCENE_NUM];
}KEY_SCENE_INFO, *PKEY_SCENE_INFO;

typedef struct
{
    char uuid[UUID_SIZE];
    char panid[8];
}ALLOW_NOTIFY_INFO_T, *PALLOW_NOTIFY_INFO_T;

typedef struct
{
    char addr[NET_ADDR_SIZE];
    char key[8];
}SYN_STATUS_INFO_T, *PSYN_STATUS_INFO_T;

#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKCOMMAND_H_ */
