/*
 * elink_packet.h
 *
 *  Created on: 2018年7月30日
 *      Author: hxfky
 */

#ifndef APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_PACKET_H_
#define APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_PACKET_H_

#define ELink_Code_heartbeat					"1000"
#define ELink_Code_heartbeat_echo				"1001"
#define ELink_Code_dev_login					"1002"
#define ELink_Code_dev_login_echo				"1003"
#define ELink_Code_dev_connect					"1004"
#define ELink_Code_dev_connect_echo				"1005"

#define ELink_Code_StatusQuery_echo				"2002"
#define ELink_Code_StatusQuery					"2003"
#define ELink_Code_Ctrl_echo					"2004"
#define ELink_Code_Ctrl							"2005"
#define ELink_Code_StatusReport					"2006"
#define ELink_Code_StatusReport_echo			"2007"
#define ELink_Code_StatusAlarm					"2008"
#define ELink_Code_StatusAlarm_echo				"2009"
#define ELink_Code_StatusFault					"2010"
#define ELink_Code_StatusFault_echo				"2011"

#define ELink_Code_SubAuth						"2012"
#define ELink_Code_SubAuth_echo					"2013"
#define ELink_Code_SubunBindReq					"2015"
#define ELink_Code_SubunBindReq_echo			"2014"
#define ELink_Code_SubBind						"2016"
#define ELink_Code_SubBind_echo					"2017"
#define ELink_Code_SubunBindReport				"2018"
#define ELink_Code_SubunBindReport_echo			"2019"

#define ELink_Code_SubOnlineReport				"2020"
#define ELink_Code_SubOnlineReport_echo			"2021"

#define ELink_Code_DevResChangeReq				"2028"
#define ELink_Code_DevResChangeReq_echo			"2029"
#define ELink_Code_DevResMangerReq				"2031"
#define ELink_Code_DevResMangerReq_echo			"2030"

#define ELink_Code_EventReport					"2032"
#define ELink_Code_EventReport_echo				"2033"

#define ELink_JSON_cmd                          "cmd"
#define ELink_JSON_deviceId						"deviceId"
#define ELink_JSON_subDeviceId				    "subDeviceId"
#define Elink_JSON_Pin                          "pin"
#define ELink_JSON_devMac                       "devMac"
#define ELink_JSON_devCTEI                      "devCTEI"
#define ELink_JSON_parentDevMac                 "parentDevMac"
#define ELink_JSON_ip                           "ip"

#define ELink_JSON_sequence						"sequence"
#define ELink_JSON_time							"time"
#define ELink_JSON_version						"version"
#define ELink_JSON_devVersion					"devVersion"
#define ELink_JSON_code							"code"
#define ELink_JSON_data							"data"
#define ELink_JSON_model						"model"
#define ELink_JSON_result						"result"

#define ELink_JSON_cmdName                      "cmdName"      // 命令名称
#define ELink_JSON_cmdParam                     "cmdParam"     // 命令参数

#define ELink_JSON_OnLine							"onLine"

#define Elink_JSON_StatusSerials				"statusSerials"
#define Elink_JSON_StatusSerial					"statusSerial"
#define ELink_JSON_StatusName					"statusName"
#define ELink_JSON_CurStatusValue               "curStatusValue"

#define ELink_JSON_ResourceSerials				"resourceSerials"
#define ELink_JSON_ResourceSerial				"resourceSerial"
#define ELink_JSON_ResourceName					"resourceName"
#define ELink_JSON_ResourceInfo					"resourceInfo"

#define ELink_JSON_Alarm						"alarm"
#define ELink_JSON_Dscp							"dscp"
#define ELink_JSON_AlarmTime					"alarmTime"

#define ELink_JSON_Error                        "error"
#define ELink_JSON_ErrorCode                    "errorCode"
#define ELink_JSON_ErrorInfo                    "errorCode"
#define ELink_JSON_ErrorTime                    "errorTime"

#define ELink_JSON_Event						"event"
#define ELink_JSON_EventId						"eventId"
#define ELink_JSON_EventName					"eventName"
#define ELink_JSON_EventInfo				    "eventInfo"

#define ELink_JSON_SerialId						"serialId"
#define ELink_JSON_sessionKey					"sessionKey"
#define ELink_JSON_tcpHost						"tcpHost"
#define ELink_JSON_token						"token"
#define ELink_JSON_udpHost						"udpHost"

#define ELink_JSON_authInterval					"authInterval"
#define ELink_JSON_heartBeat					"heartBeat"

#define ELink_JSON_result_OK					"0"

#define ELink_JSON_online_OK                    "1"
#define ELink_JSON_del_OK                       "3"
#define ELINK_JSON_dsp                          "连续操作攻击"
#define ELINK_JSON_alarm                        "电量告警"
#define ELink_JSON_Sucess                       "sucesss"
#define ELink_JSON_authResult                   "authResult"

#define ELink_StatusName_BATTERY_WARN			"BATTERY_WARN"
#define ELink_StatusName_SIGNAL					"SIGNAL"
#define ELink_StatusName_DEMOLISH				"DEMOLISH"		//设备拆除
#define ELink_StatusName_LOCK_STATE				"LOCK_STATE"	//门锁状态

#define ELink_ResName_KEY_LIST					"KEY_LIST"		//钥匙列表
#define ELink_ResName_TEMP_KEY_LIST				"TEMP_KEY_LIST"	//临时钥匙列表

#define ELink_ResInfo_KEY_ID					"KEY_ID"		//钥匙ID
#define ELink_ResInfo_KEY_TYPE					"KEY_TYPE"		//钥匙类型
#define ELink_ResInfo_MODE                      "MODE"          // 1:新增 2:修改 3:删除
#define ELink_ResInfo_ADMIN_PASS				"ADMIN_PASS"		//管理员密码
#define ELink_ResInfo_TEMP_PASS					"TEMP_PASS"			//临时密码
#define ELink_ResInfo_START_TIME				"START_TIME"		//生效时间
#define ELink_ResInfo_END_TIME					"END_TIME"			//失效时间
#define ELink_ResInfo_LIFECYCLE					"LIFECYCLE"			//可用次数

#define ELink_CrtlName_OPEN_LOCK                "OPEN_LOCK"         //远程开锁
#define ELink_CrtlName_OPEN_NET                 "OPEN_NET"          //远程加网

#define ELink_EventName_LOCK_OPEN				"LOCK_OPEN"			//开锁
#define ELink_EventName_LOCK_ALARM				"LOCK_ALARM"			//报警
#define ELink_EventInfo_TIME					"TIME"					//时间
#define ELink_EventInfo_ALARM_TYPE				"ALARM_TYPE"			//报警类型

#define ELink_AlarmInfo_LINGER                  "LINGER"            // 有人逗留
#define ELink_AlarmInfo_PASS_ATK                "PASS_ATK"          // 密码攻击
#define ELink_AlarmInfo_HIJACK                  "HIJACK"            // 挟持告警
#define ELink_AlarmInfo_APP_ATK                 "APP_ATK"           // APP密码攻击
#define ELink_AlarmInfo_FINGER_ATK              "FINGER_ATK"        // 指纹攻击
#define ELink_AlarmInfo_CARD_ATK                "CARD_ATK"          // 卡片攻击
#define ELINK_AlarmInfo_BATTERY_WARN            "BATTERY_WARN"      // 电池电量

#define ELink_errInfo_OK                      0                   // 请求成功
#define ELink_errInfo_INVALID_PARA            100001              // 请求参数非法
#define ELink_errInfo_ID_NO_EXSIT             200001              // 设备id不存在
#define ELink_errInfo_PIN_ERROR               200002              // 设备pin错误
#define ELink_errInfo_AUTH_ERR                200003              // 设备鉴权失败
#define ELink_errInfo_NO_ONLINE               300001              // 设备或子设备不在线
#define ELink_errInfo_NO_STRCODE              300002              // 设备不支持该命令
#define ELink_errInfo_UNKOWN_ERR              899999              // 设备侧未知错误

typedef struct
{
    unsigned int sequence;
    unsigned int msglen;
    unsigned char *msg;
} stElinkPacket;

typedef struct
{
    unsigned char sessionKey[32];
    unsigned char tcpHost[128];
    unsigned char udphost[128];
    unsigned char token[64];
    int heartBeat;
    int loginlasttime;
    int temptime;
    int time;
    int authInterval;
    int devConnectFlag;
    int devConnectFd;
    int authResult;
    int authSucess;
    int online;
} stELink_CommInfo;

typedef struct
{
    unsigned char devid[48];
    unsigned char pin[48];
    unsigned char subdevid[48];
    unsigned char Resequence[8];
} stELink_DevInfo;

typedef struct
{
    unsigned char code[8];
    unsigned char *data;
    unsigned char *token;
    unsigned char *deviceid;
} stElink_Msg_ciph, *PstELink_Msg_ciph;

typedef struct
{
    unsigned char deviceid[32];
    unsigned char sequence[8];
    unsigned int time;
    unsigned char ver[16];
} stElink_Msg_DevLogin, *PstElink_Msg_DevLoginh;

#define ELink_MSG_PACK(outmsg,inmsg) sprintf(outmsg,"CTS%s\r\n",inmsg);

#endif /* APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_PACKET_H_ */
