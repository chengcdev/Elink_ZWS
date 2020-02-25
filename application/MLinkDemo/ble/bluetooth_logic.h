/*
 * bluetooth_logic.h
 *
 *  Created on: 2018年9月16日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_BLE_BLUETOOTH_LOGIC_H_
#define APPLICATION_MLINKDEMO_BLE_BLUETOOTH_LOGIC_H_

#include "mico_platform.h"
#include "../MLinkGpio/MLinkGpio.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../flash/storage_ble_logic.h"
#include "mico_system.h"
#include "../queue/queue.h"
#include "../uart/uart_logic.h"

#define BLE_OK              0
#define BLE_ERROR           (-1)

#define USE_RECORD          0   //是否有记录功能

#define BLE_LOCK_NAME           "23_BLE_COMM"   // 蓝牙开门器

#define DEFAULT_DEVNAME         "BLE_DOOR"      // 缺省设备名
#define BLE_DEFINE_DATA         "BLE_LOCK"      // 蓝牙用户自定义数据
#if 1
#define DEFAULT_USERPIN         "Bh8YtN0d"      // 默认用户PIN码
#define BLE_PUBLIC_KEY          "O9s@6f$0"      // 公共秘钥

#else
#define BLE_PUBLIC_KEY          "O9s@6f$0"      // 公共秘钥
#define DEFAULT_USERPIN         "13572468"      // 默认用户PIN码
#endif

#define BLE_OPENTYPE_BLEPHONE   0x08            //手机开锁


#define BLE_NOTI_ECHO_OK            1
#define BLE_NOTI_ECHO_ERROR         2
#define BLE_NOTI_ECHO_VERIFY_ERR    3   // 密码验证失败

#define _GET_BIT_(value, bit)   (((value)>>bit)&(0x01))
#define _BCD_TO_DEC(CHR) (((CHR>>4)&0x0F)*10+(CHR&0x0f))            // BCD 码转 十进制 CHR一个字节
#define _DEC_TO_BCD(DEC) ((((DEC/10)&0x0F)<<4)|((DEC%10)&0x0F))     // 十进制 转 BCD码 DEC一个字节
#define _BCD_TO_HEX(CHR)  ((CHR / 16) *10) + (CHR % 16)

#define WK_PIN_H()  MicoGpioOutputHigh(MICO_MK_WK);    //  透传
#define WK_PIN_L()  MicoGpioOutputLow(MICO_MK_WK);     // AT指令

#define LOCK_LSIT_COUNT                 253
#define LOCK_LIST_TEMP_COUNT            10
#define ELINK_BOUNDARY        100000000


#define CMD_BLE_UNLOCK              0x01    // 开锁
#define CMD_BLE_GET_RECORD_INFO     0x05    // 获取记录信息
#define CMD_BLE_GET_RECORD_LIST     0x06    // 获取记录
#define CMD_BLE_REMO_PASS_UNLOCK    0x08    // 远程密码开锁
#define CMD_BLE_SECURITY_CHECK      0x13    // 安全验证
#define CMD_BLE_SECURITY_CHECK_TWO  0x15    // 安全验证2
#define CMD_BLE_GET_LOCK_INFO       0x16    // 查看锁信息
#define CMD_BLE_GET_LOCK_LIST       0x17    // 获取门锁钥匙列表
#define CMD_BLE_ADD_TMP_SEC_UNLOCK  0x18    // 添加临时秘钥开门
#define CMD_BLE_DEL_TMP_SEC_UNLOCK  0x19    // 删除临时秘钥
#define CMD_BLE_ADD_DEV             0x21    // 增加设备
#define CMD_BLE_GET_PIN             0x22    // 获取用户PIN码
#define CMD_BLE_SET_PIN             0x23    // 设置用户PIN码
#define CMD_BLE_SET_FACTORY         0x24    // 设置恢复出厂
#define CMD_BLE_SET_BROADRATE       0x2A    //
#define CMD_BLE_SET_LOCKCHANNEL     0x03    //lock channel
#define CMD_BLE_SET_TIME            0X25    //设置锁时间同步


// 需要回调给主应用处理的命令
typedef enum
{
    BLE_NOTI_IDLE,
    BLE_NOTI_RESTOR_FACTORY,
    BLE_NOTI_BIND_LOCK,             //绑定状态反馈
    BLE_NOTI_NEW_RECORD,            //通知有新记录
    BLE_NOTI_UNLOCK,                //开锁命令回调
    BLE_NOTI_REMO_PASS_UNLOCK,      //远程密码开锁
    BLE_NOTI_GET_LOCK_LIST,         //获取门锁列表
    BLE_NOTI_FORCE_GET_LOCK_LIST,   //强制获取门锁列表
    BLE_NOTI_ADD_TMP_SEC_UNLOCK,    //添加临时秘钥
    BLE_NOTI_DEL_TMP_SEC_UNLOCK,    //删除临时秘钥
    BLE_NOTI_ERROR,                 //蓝牙模块异常
    BLE_NOTI_SET_TIME,              //设置时间
    BLE_NOTI_SET_USERPIN,
    BLE_NOTI_SET_BROADRATE,
    BLE_NOTI_RSSI_UPLINK,
    BLE_NOTI_GET_LOCKINFO,
    BLE_NOTI_LOCKCHANNEL,           //notify lock channel
    BLE_NOTI_STATE,                 // 状态通知
    BLE_NOTI_ONLINE,                // 在线状态
    BLE_NOTE_INIT_SUCESS,           //初始化成功
}BLE_NOTIFY_E;

typedef enum
{
    BLE_IDLE = 0,
    BLE_LOCK_BIND,
    BLE_UNLOCK,
    BLE_REMO_PASS_UNLOCK,
    BLE_GET_RECORD_INFO,
    BLE_GET_LOCK_LIST,
    BLE_FORCE_GET_LOCK_LIST,
    BLE_ADD_TMP_SEC_UNLOCK,         //添加临时秘钥
    BLE_DEL_TMP_SEC_UNLOCK,         //删除临时秘钥
    BLE_SET_TIME,                   //设置时间
    BLE_GET_RECORD_LIST,
    BLE_SET_USERPIN,
    BLE_SET_BROADRATE,
    BLE_SET_FACTORY,
    BLE_GET_LOCKINFO,
    BLE_SET_LOCKCHANGNEL,
}BLE_STATE_E;

typedef enum
{
    BLE_RECORD_TYPE_OPEN        = 0x0,              //开锁记录
    BLE_RECORD_TYPE_STATUS      = 0x01,             //锁状态
    BLE_RECORD_TYPE_OPER        = 0x02,             //操作记录
    BLE_RECORD_TYPE_TMPAUTH     = 0x09,             //临时授权
    BLE_RECORD_TYPE_ADD         = 0x03,             //添加记录
    BLE_RECORD_TYPE_DEL         = 0x04,             //删除记录
}BLE_RECORD_TYPE_E;

typedef int (*PBLE_STATE_CALLBACK)(BLE_NOTIFY_E state, char * data, int size);


/*======================宏定义=======================*/
/*  AT指令类型
**注：在MT254xCoreS-V1.0-AT指令手册中，凡是携带"?"的指令
**都是既可以查询也可以设置操作，另外AT+CON[para1]和AT+CONN[para1]
**这两个命令实际上相当于设置命令，除此之外剩下的所有命令，都是CALL命令
*/
typedef enum
{
    AT_CALL,                                        // 非查询非设置命令
    AT_GET,                                         // 查询命令
    AT_SET,                                         // 设置命令
    AT_GETR,                                        // 远程获取命令
} AT_TYPE_E;

typedef enum
{
    AT_TEST,
    AT_HELP,
    AT_VERS,
    AT_NAME,
    AT_RENEW,
    AT_RESET,
    AT_ROLE,
    AT_NOTI,
    AT_IMME,
    AT_START,
    AT_ADST,

    AT_BAUD,
    AT_FLOW,
    AT_PARI,
    AT_STOP,

    AT_ADVI,
    AT_POWE,

    AT_SCAN,
    AT_RANG,
    AT_SHOW,
    AT_CON,
    AT_CONM,
    AT_CONN,
    AT_CONNL,
    AT_SNAME,
    AT_SRSSI,

    AT_ISCON,
    AT_DISCON,
    AT_DISC,
    AT_LADDR,
    AT_CLEAR,
    AT_RADD,
    AT_SAVE,

    AT_TYPE,
    AT_PWDM,
    AT_PWD,

    AT_PASS,
    AT_UADT,
    AT_MAC,
    AT_RSSI,
    AT_TEMP,

    AT_LED,
    AT_PWM,
    AT_PDIR,
    AT_PDAT,

    AT_PWRM,
    AT_SLEEP,
    AT_WAKE,
    AT_BATC,
    AT_BATT,
    AT_R,
    AT_STAS,
} AT_CMD_E;

typedef enum
{
    LOCK_OPERA_EVENT = 0,       // 门操作记录
    LOCK_STATE_EVENT = 1,       // 锁状态记录
    LOCK_OPEN_EVENT = 2         // 开门状态记录
}LOCK_EVENT_E;

// 串口接收数据结构
typedef struct
{
    signed char Cmd;                            // 命令值
    unsigned char ErrorFlg;                     // 包错误标志
    unsigned short RemainNum;                   // 多包时剩余包数
    unsigned int datalen;                       // 收到数据长度
    char data[1024]; //1024                      // 收到的数据,最大256帧,每帖15个字节
}RecvPacket, * PRecvPacket;

//0正常 1异常
typedef struct
{
    unsigned char isLocked;                                  //是否反锁 0未反锁
    unsigned char isLowBattery;                              //低压 0正常
    unsigned char isFaultDestory;                            //防拆故障 0正常
    unsigned char isFaultLock;                               //锁舌故障 0正常
    unsigned char isDoorNoClose;                             //门未关超时故障
    unsigned char isOperSuccess;                             //操作状况 0成功 1失败 2验证失败
    unsigned char eventtype;                                 //门状态                                //锁ID
    unsigned char g_adminpwd[5];                             // 管理员密码
}stBLELockStatus,* PstBLELockStatus;



typedef enum
{
    // CTRL server
    COAP_CTRL_NOTI_CMD_UNLOCK       = 1000,     //open the lock
    COAP_CTRL_NOTI_CMD_USRPIN,                  //modi ble usrpin
    COAP_CTRL_NOTI_CMD_LOCKGALLERY,             //modi lock gallery
    COAP_CTRL_NOTI_CMD_DOORSTATE    = 1010,     //door state
    COAP_CTRL_NOTI_CMD_UNLOCKRECORD,            //unlock record
    COAP_CTRL_NOTI_CMD_OPERATECORD,             // operate record
    COAP_CTRL_NOTI_CMD_ALARMEVENT,              // alarm event
}COAP_CTRL_CMD_ID_E;

int ble_logic_init(PBLE_STATE_CALLBACK pBleCallback);
int ble_uart_recv_callback( PUART_RECV_PACKET recvPacket );
int ble_bind_lock_start(void);
int ble_bind_lock_end(void);
int ble_bind_lock_bymac(char *blemac);
int ble_set_userpin(char *userpin);
int ble_set_factory(void);
int ble_unlock_start(void);
int ble_set_broadrate(unsigned char mode);
int ble_get_lockinfo(void);
int ble_set_channel(void);
int ble_add_tmp_pwd_start(uint16_t id,uint8_t locktype,uint8_t *adminpwd,uint8_t *pwd,mico_rtc_time_t * eff_time,mico_rtc_time_t * no_eff_time,uint8_t cnt);
int ble_del_tmp_pwd_start(uint16_t id,uint8_t * adminpwd);
int ble_remote_pwd_unlock_start(uint8_t* password);
int ble_get_lock_list(uint8_t flag);
int ble_init_finish(void);

#endif /* APPLICATION_MLINKDEMO_BLE_BLUETOOTH_LOGIC_H_ */
