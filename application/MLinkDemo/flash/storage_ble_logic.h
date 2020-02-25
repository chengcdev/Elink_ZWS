/*
 * storage_ble_logic.h
 *
 *  Created on: 2018年9月17日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_FLASH_STORAGE_BLE_LOGIC_H_
#define APPLICATION_MLINKDEMO_FLASH_STORAGE_BLE_LOGIC_H_
#include "mico.h"
#include "flash_storage_object.h"

#define BLU_MAC_LEN                 12    // 蓝牙 MAC 码长度
#define BLU_PWD_LEN                 8     // 蓝牙 PWD 长度

#define MAX_BLERECORD_NUM       100 // 最大操作记录条数
#define BLERECORD_LEN           10  // 每条操作记录的长度
#define MAX_LOCAL_TOAL          5   //最多支持绑定的锁个数
#define BLE_MAX_LSIT_LEN        20  //每帧最大长度
#define BLE_LIST_TMEPMAX_NUM       10  // 临时秘钥最大个数
#define BLE_TMP_KEY_LIST_DATA_LEN    3  // 临时钥匙应答数据长度
#define hfuflash_write(a, b, c)         MicoFlashWriteEx()                    10

#define FLASH_USER_LOCKSTATUE_OFFSET    (BLOCK_SIZE*46 + FLASH_USER_LOCKSTATUE_SIZE)
#define FLASH_USER_LOCKSTATUE_SIZE      (BLOCK_SIZE*1)
#define FLASH_USER_LOCKTEMLIST_OFFSET   (BLOCK_SIZE*44 + FLASH_USER_LOCKTEMLIST_SIZE)
#define FLASH_USER_LOCKTEMLIST_SIZE     (BLOCK_SIZE*1)


#if 1
typedef struct
{
    uint8_t     PSWMode;                // 密码模式: 0 正常 1 未设置过
    uint8_t     Power_Left;             // 电池电量: 1-100%
    uint8_t     PowerMode;              // 电源模式: 0 正常模式 1 省电模式

    uint8_t     AlarmCode;              // 报警类型
    uint8_t     LockState;              // 锁状态: 0x01 门反锁 0x02 门没有上锁 0x03验证失败
    uint8_t     RSSI_Uplink;            // 信号强弱: 0-100%, 0代表离线状态
    uint8_t     ErrorCode;

     char       UserPin[9];             // 用户PIN码
     char       LockMac[13];            // 锁mac码
     char       LocalMac[13];           // 本地mac码
     char       Version[8];
}SYSTEM_INFO;



#else
typedef struct
{
    unsigned char   UserPin[9];
    unsigned char   LockMac[13];
}LockmacUserpin,*PLockmacUserpin;
typedef struct
{

    uint8_t     PSWMode;                // 密码模式: 0 正常 1 未设置过
    uint8_t     Power_Left;             // 电池电量: 1-100%
    uint8_t     PowerMode;              // 电源模式: 0 正常模式 1 省电模式

    uint8_t     AlarmCode;              // 报警类型
    uint8_t     LockState;              // 锁状态: 0x01 门反锁 0x02 门没有上锁 0x03验证失败
    uint8_t     RSSI_Uplink;            //  信号强弱: 0-100%, 0代表离线状态
    uint8_t     ErrorCode;
    unsigned char       LocalMac[13];           // 锁mac码
    unsigned char       Version[8];
    LockmacUserpin      MacUserpin[MAX_LOCAL_TOAL];             //用户PIN码
    unsigned char       LockID;                                 //锁ID
}SYSTEM_INFO;
#endif

// 操作记录列表
typedef struct
{
    unsigned int Counts;                                // 记录数
    char Record[MAX_BLERECORD_NUM][BLERECORD_LEN];      // 记录
}BLEUSER_RECORDLIST, *PBLEUSER_RECORDLIST;

// 门锁列表记录列表
typedef struct
{
    uint16_t usrid;
    uint8_t keytype;
    uint8_t mode;
}BLEUSER_LISTRECORDLIST, *PBLEUSER_LISTRECORDLIST;

typedef struct{
    char presentLockMac[MAC_SIZE];      // 当前锁的mac 地址信息
    char presentUsrPin[UUID_SIZE];      // 用户 pin 码
    char deviceId[DEVICEID_SIZE];       // 锁的 deviceId
}PRESENTSYSINFO;

#if 1
int stor_set_userpin(char *userpin);
char * stor_get_userpin(void);
int stor_set_lockmac(char *lockmac);
char * stor_get_lockmac(void);
#else
int stor_set_userpin(char *userpin,unsigned char ID);
char * stor_get_userpin(unsigned char ID);
int stor_set_lockmac(char *lockmac,unsigned char ID);
char * stor_get_lockmac(unsigned char ID);
#endif

int stor_set_powermode(int mode);
int stor_get_powermode(void);
int stor_set_localmac(char *localmac);

void stor_set_lockstate_ex(int state);
uint8_t stor_get_lockstate(void);
void stor_set_powerleft(uint8_t value);
int stor_info_reset(void);
SYSTEM_INFO *stor_get_sysinfo(void);
int stor_info_init(void);
void get_locklist_num( void );

#if USE_RECORD
BLEUSER_RECORDLIST *stor_get_record(void);
int stor_record_lock(BLEUSER_RECORDLIST *pRecordlist);
#endif

void stor_set_rssi_uplink(uint8_t rssiValue);
uint8_t stor_get_rssi_uplink(void);
char * storage_get_present_lockmac(void);
char * storage_get_present_userpin(void);
OSStatus storage_set_present_lockpin(void);

#endif /* APPLICATION_MLINKDEMO_FLASH_STORAGE_BLE_LOGIC_H_ */
