/*
 * storage_ble_logic.c
 *
 *  Created on: 2018年9月17日
 *      Author: Administrator
 */


#include "mico.h"
#include "flash_storage_object.h"
#include "storage_ble_logic.h"
#include "../ble/bluetooth_logic.h"
#include "flash.h"
#include <debug.h>
#include "MLinkAppDef.h"

//#define WRITE_FLAG_ADDR     (4096)              // 写标识位  在flash中存放的起始位置,该信息用于指明是否已经有信息存放于此
//#define SYSINFO_ADDR        (4096+10)           // 蓝牙设备信息在flash中存放的起始位置
//#define RECORD_ADDR         (4096+200)          // 开锁记录信息在flash中存放的起始位置
//#define LOCKSTATUE_ADDR     (4096+1000)         // 锁状态信息在flash中存放的起始位置

#define os_storage_log(M, ...)  custom_log("UART LOG", M, ##__VA_ARGS__)
#define os_storage_log_trace() //custom_log_trace("UART RECV")

#if USE_RECORD
static BLEUSER_RECORDLIST g_recordList;
#endif

static SYSTEM_INFO g_sysInfo;           // 蓝牙连接信息缓存

//static uint8_t g_writeFlag = 0;         // 写标识位
//static uint8_t g_rebootTimes = 0;
PRESENTSYSINFO presentsysinfo;
static uint32_t g_lockStateNum = 0;
static uint32_t g_locklistNum = 0;   // 临时秘钥列表个数
/********************************************************
 * function: storage_read_lockmacpin_obj
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_lockmacpin_obj(char *devid,PDEVICEOBJ_T pDeviceObj)
{
    int ret = 0;
    ret = storage_read_dev_obj(devid, pDeviceObj);
    if (ret > 0)
    {
        ret = kInProgressErr;
    }
    return kGeneralErr;
}
/********************************************************
 * function: storage_read_lockdeviceid_obj
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus  storage_read_lockdeviceid_obj(char *mac,PDEVICEOBJ_T pDeviceObj)
{
    if (NULL == pDeviceObj)
    {
        return kGeneralErr;
    }
    int devNum = storage_get_object_num(DEVICE_OBJ_ID);
    int count = 0;
    int ret = 0;
    for (count=0; count < devNum; count ++)
    {
        memset(pDeviceObj, 0, sizeof(DEVICEOBJ_T));
        storage_read_dev_obj_batch(1, count, pDeviceObj);
        if (0 == strcmp(pDeviceObj->mac, mac))
        {
            ret = kInProgressErr;
            return ret;
        }
    }
    return kGeneralErr;
}


/********************************************************
 * function: storage_get_present_id
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to get id
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
char * storage_get_present_id(void)
{
    return presentsysinfo.deviceId;
}
/********************************************************
 * function: storage_get_present_lockmac
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to get lockmac
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
char * storage_get_present_lockmac(void)
{
//    sprintf(presentsysinfo.presentLockMac,"%s","4D4C00000844");
    return presentsysinfo.presentLockMac;
}
/********************************************************
 * function: storage_get_present_userpin
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to get userpin
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
char * storage_get_present_userpin(void)
{
//    sprintf(presentsysinfo.presentUsrPin,"%s","13572468");
    return presentsysinfo.presentUsrPin;
}

void storage_set_present_userpin(char *pin)
{
    strcpy(presentsysinfo.presentUsrPin,pin);
}
/********************************************************
 * function: storage_set_present_lockpin
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to set id
             loacmac userpin
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
OSStatus storage_set_present_lockpin(void)
{

    int ret = kNoErr;
    unsigned char  deviceId[9] = {0};
    DEVICEOBJ_T DeviceObj;
    memset(&DeviceObj,0,sizeof(DEVICEOBJ_T));
    ret = storage_read_lockmacpin_obj(&deviceId,&DeviceObj);
    memcpy(presentsysinfo.presentLockMac,DeviceObj.mac,sizeof(DeviceObj.mac));
    os_storage_log("presentsysinfo.presentLockMac[%s]",presentsysinfo.presentLockMac);
    memcpy(presentsysinfo.presentUsrPin,DeviceObj.uuid,sizeof(DeviceObj.uuid));
    os_storage_log("presentsysinfo.presentUsrPin[%s]",presentsysinfo.presentUsrPin);
    memcpy(presentsysinfo.deviceId,deviceId,sizeof(DeviceObj.deviceId));
    if(presentsysinfo.presentLockMac[0] != 0xffffffff)
    {
        ret = kNoErr;
    }
    os_storage_log("ret : %d",ret);
    return ret;
}

OSStatus storage_get_mac_sucess()
{
    int ret = kNoErr;
    if(presentsysinfo.presentLockMac[0] != 0xffffffff)
    {
        ret = kNoErr;
    }
    else
    {
        ret = kGeneralErr;
    }
    return ret;
}
/********************************************************
 * function: storage_set_present_deviceId
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to set id
             loacmac userpin
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
OSStatus storage_set_present_deviceId(char * mac)
{
    if(mac == NULL)
    {
        return kGeneralErr;
    }
    int ret = kNoErr;
    DEVICEOBJ_T DeviceObj;
    memset(&DeviceObj,0,sizeof(DEVICEOBJ_T));
    ret = storage_read_lockdeviceid_obj(mac,&DeviceObj);
    memcpy(presentsysinfo.deviceId,DeviceObj.deviceId,sizeof(DeviceObj.deviceId));
    return ret;
}


/********************************************************
 * function: storage_set_present_deviceId
 * description:
 * input:   1. pLockPinObj: point to a structure that we want to set id
             loacmac userpin
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
OSStatus storage_set_present_devicePin(char * mac)
{
    if(mac == NULL)
    {
        return kGeneralErr;
    }
    int ret = kNoErr;
    DEVICEOBJ_T DeviceObj;
    memset(&DeviceObj,0,sizeof(DEVICEOBJ_T));
    storage_read_lockdeviceid_obj(mac,&DeviceObj);
    memcpy(presentsysinfo.presentUsrPin,DeviceObj.uuid,sizeof(DeviceObj.uuid));
    return ret;
}

/********************************************************
 * function: storage_set_present_deviceId
 * description:
 * input:   1. ble_index:
 * output:  2. mac : mac指针必须保证所指向的空间不为空且至少12个字节
 * return:
 * auther:
 * other:
*********************************************************/
OSStatus storage_get_ble_mac( unsigned char ble_index, char *mac )
{
    DEVICEOBJ_T devObj = {0};
    if (mac == NULL)
    {
        return kGeneralErr;
    }
    storage_read_dev_obj_batch(1, ble_index, &devObj);
    if ((devObj.mac[0] == (char)0xff) || (devObj.mac[0] == (char)0))
    {
        return kGeneralErr;
    }
    memcpy(mac, devObj.mac, BLU_MAC_LEN);
    return kNoErr;
}


/********************************************************
 * function: stor_lockstatue_write
 * description:  save lockstatue
 * input:   cnt and lockstatue
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
OSStatus stor_lockstatue_write(OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num,PstBLELockStatus plockstatue)
{

    if (NULL == plockstatue)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_lockStateNum > DEVICE_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LOCKSTATUE_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = obj_num;
    uint8_t updateIndex = 0;
    stBLELockStatus lockstatue;
    memset(&lockstatue, 0, sizeof(stBLELockStatus));

    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        PstBLELockStatus pstatue = plockstatue+updateIndex;
        for (count=0; count<g_lockStateNum; count++)
        {
            MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&lockstatue, sizeof(stBLELockStatus));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if(1)// (0 == strcmp(pstatue->deviceId, lockstatue.deviceId))
                {
                    uint32_t ObjectAddrTemp = ObjectAddr - sizeof(stBLELockStatus);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddrTemp, (uint8_t *)plockstatue+updateIndex*sizeof(stBLELockStatus), sizeof(stBLELockStatus));
                    if (ret != kNoErr)
                    {
                        os_storage_log("write device obj fail! errcode: %d", ret);
                    }
                    if (updateIndex < (obj_num - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)plockstatue + updateIndex*sizeof(stBLELockStatus);
                        uint8_t *src = (uint8_t *)plockstatue + (updateIndex + 1)*sizeof(stBLELockStatus);
                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(stBLELockStatus));
                    }
                    updateCount --;
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                //os_storage_log("pDevObj->deviceId[%s][%s]\n",pstatue->deviceId,lockstatue.deviceId);
                if (1)//(0 == strcmp(pstatue->deviceId, lockstatue.deviceId))
                {
                    ObjectAddr = FLASH_USER_LOCKSTATUE_OFFSET+count*sizeof(stBLELockStatus);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)plockstatue+updateIndex*sizeof(stBLELockStatus), sizeof(stBLELockStatus));
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_LOCKSTATUE_OFFSET;
    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_LOCKSTATUE_OFFSET+g_lockStateNum*sizeof(stBLELockStatus);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)plockstatue, updateCount*sizeof(stBLELockStatus));
       if ( ret == kNoErr )
       {
           g_lockStateNum += updateCount;
       }
//        os_storage_log("write ret is %d, g_lockStateNum = %d", ret, g_lockStateNum);
    }

    return ret;

}

/********************************************************
 * function: stor_lockstatue_read
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus stor_lockstatue_read(PstBLELockStatus plockstatue)
{
    if (NULL == plockstatue)
    {
        return kGeneralErr;
    }
    int count = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_LOCKSTATUE_OFFSET;
    int ret = kGeneralErr;

    for (count=0; count < 1; count ++)
    {
        memset(plockstatue, 0, sizeof(stBLELockStatus));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)plockstatue, sizeof(stBLELockStatus));
        if (ret == kNoErr)
        {
            if(1)//if (!strcmp(plockstatue->deviceId, devid))
            {
                ret = count + 1;
                return ret;
            }
        }
    }
    return ret;
}

/********************************************************
 * function: stor_templocklist_write
 * description:  save templocklist
 * input:   cnt and locklist update_type
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
OSStatus stor_templocklist_write(OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num,PBLEUSER_LISTRECORDLIST plockTempList)
{

    if (NULL == plockTempList)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_locklistNum > BLE_LIST_TMEPMAX_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LOCKTEMLIST_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = obj_num;
    uint8_t updateIndex = 0;
    BLEUSER_LISTRECORDLIST locktemplist;
    memset(&locktemplist, 0, sizeof(BLEUSER_LISTRECORDLIST));

    for ( updateIndex = 0; updateIndex < obj_num; updateIndex++ )
    {
        PBLEUSER_LISTRECORDLIST pstatue = plockTempList + updateIndex;
        for ( count = 0; count < g_locklistNum; count++ )
        {
            MicoFlashReadEx( MICO_PARTITION_USER, &ObjectAddr, (uint8_t *) &locktemplist,
                             sizeof(BLEUSER_LISTRECORDLIST) );
            if ( update_type == OBJECT_UPDATE_ADD )
            {
                os_storage_log("pstatue->usrid : %d locktemplist.usrid : %d",pstatue->usrid,locktemplist.usrid);
                if( pstatue->usrid == locktemplist.usrid)
//                if ( 0 == strcmp( pstatue->usrid, locktemplist.usrid ) )
                {

                    os_storage_log("pstatue->usrid : %d locktemplist.usrid : %d",pstatue->usrid,locktemplist.usrid);
                    uint32_t ObjectAddrTemp = ObjectAddr - sizeof(BLEUSER_LISTRECORDLIST);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddrTemp, (uint8_t *)plockTempList+updateIndex*sizeof(BLEUSER_LISTRECORDLIST), sizeof(BLEUSER_LISTRECORDLIST));
                    if (ret != kNoErr)
                    {
                        os_storage_log("write device obj fail! errcode: %d", ret);
                    }
                    if (updateIndex < (obj_num - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)plockTempList + updateIndex*sizeof(BLEUSER_LISTRECORDLIST);
                        uint8_t *src = (uint8_t *)plockTempList + (updateIndex + 1)*sizeof(BLEUSER_LISTRECORDLIST);
                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(BLEUSER_LISTRECORDLIST));
                    }
                    updateCount --;

                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                //os_storage_log("pDevObj->deviceId[%s][%s]\n",pstatue->deviceId,lockstatue.deviceId);
                if ( 0 == strcmp( plockTempList->usrid, locktemplist.usrid ) )
                {
                    ObjectAddr = FLASH_USER_LOCKTEMLIST_OFFSET+count*sizeof(BLEUSER_LISTRECORDLIST);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)plockTempList+updateIndex*sizeof(BLEUSER_LISTRECORDLIST), sizeof(stBLELockStatus));
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_LOCKTEMLIST_OFFSET;


    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_LOCKTEMLIST_OFFSET+g_locklistNum*sizeof(BLEUSER_LISTRECORDLIST);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)plockTempList, updateCount*sizeof(BLEUSER_LISTRECORDLIST));
       if ( ret == kNoErr )
       {
           g_locklistNum += updateCount;
       }
        os_storage_log("write ret is %d, g_lockStateNum = %d", ret, g_locklistNum);
    }

    return ret;

}

/********************************************************
 * function: stor_locktemplist_read
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
OSStatus stor_locktemplist_read(BLEUSER_LISTRECORDLIST *plocklist)
{
    if (NULL == plocklist)
    {
        return FLASH_OPERATE_FAIL;
    }
    uint8_t index = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_LOCKTEMLIST_OFFSET;
    int ret = FLASH_OPERATE_OK;
    os_storage_log(" g_lockStateNum = %d", g_locklistNum);
    for (index = 0; index < g_locklistNum; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&plocklist[index], sizeof(BLEUSER_LISTRECORDLIST));
    }
    return g_locklistNum ;
}
/********************************************************
 * function: storage_del_blelsit_obj
 * description:
 * input:   1. pDevInfo:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_blelsit_obj( uint16_t *objid )
{
    uint32_t structSize = sizeof(BLEUSER_LISTRECORDLIST);
    BLEUSER_LISTRECORDLIST blelist_obj;
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_LOCKTEMLIST_OFFSET;
    uint32_t *objNumTotal = &g_locklistNum;
    memset(&blelist_obj,0,sizeof(BLEUSER_LISTRECORDLIST));
    ret = stor_del_templsit(objid, startAddr, objNumTotal, structSize,&blelist_obj);
    return ret;
}
/********************************************************
 * function: stor_del_templsit
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus stor_del_templsit(uint16_t *obj_id, uint32_t start_addr, uint32_t *total_num, uint32_t obj_size,BLEUSER_LISTRECORDLIST *blelsit_obj)
{
    uint8_t objIndex;
    uint32_t objAddr = start_addr;
    OSStatus ret = kGeneralErr;
    uint32_t objTotal = *total_num;
    uint8_t data_id[2] = {0};
    if (objTotal == 0)
    {
        return kNoErr;
    }
    for (objIndex = 0; objIndex < objTotal; objIndex++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objAddr, (uint8_t *)blelsit_obj, obj_size);
        os_storage_log("blelsit_obj->usrid,%d *obj_id : %d",blelsit_obj->usrid, *obj_id);
        if (0 == strcmp(blelsit_obj->usrid, *obj_id))
        {
            if (objIndex >= (objTotal-1))
            {
                objAddr -= obj_size;
                ret = MicoFlashEraseEx(MICO_PARTITION_USER, objAddr, obj_size);
            }
            else
            {
                uint8_t *pObjsData = (uint8_t *)malloc((objTotal-objIndex)*obj_size);
                uint32_t objectAddrTemp = objAddr;
                memset(pObjsData, 0, (objTotal-objIndex)*obj_size);
                MicoFlashRead(MICO_PARTITION_USER, &objectAddrTemp, (uint8_t *)pObjsData, (objTotal-objIndex-1)*obj_size);
                objectAddrTemp = objAddr - obj_size;
                ret = MicoFlashWriteEx(MICO_PARTITION_USER, &objectAddrTemp, (uint8_t *)pObjsData, (objTotal-objIndex)*obj_size);
                if (pObjsData)
                {
                    free(pObjsData);
                }

            }
            if (ret == kNoErr)
            {
                os_storage_log("total_num: %d", *total_num);
                (*total_num)--;
                os_storage_log("total_num: %d", *total_num);
            }
            break;
        }
    }
    os_storage_log("current num : %d, now num: %d, ret: %d", objTotal, *total_num, ret);
    return ret;
}
/********************************************************
 * function: storage_get_locklist_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_locklist_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_LOCKTEMLIST_OFFSET;
    BLEUSER_LISTRECORDLIST locklistObj;
    for (index = 0; index < BLE_LIST_TMEPMAX_NUM; index++)
    {
        MicoFlashReadEx(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&locklistObj, sizeof(BLEUSER_LISTRECORDLIST));
        if((locklistObj.usrid != 0) && (locklistObj.usrid != 0xffff))
        {
            count ++;
        }
        else
        {
            break;
        }
    }
    return count;
}
/**
 *  获取临时秘钥列表总数
 */
void get_locklist_num( void )
{
    g_locklistNum = storage_get_locklist_num( );
    os_storage_log("g_locklistNum : %d",g_locklistNum);
}
#if 0
/********************************************************
 * function: stor_info_default
 * description:  设置蓝牙连接信息默认初始值
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void stor_info_default(void)
{
    unsigned char i;
    memset(&g_sysInfo, 0, sizeof(SYSTEM_INFO));
    g_sysInfo.AlarmCode = 0;                // 报警类型
    g_sysInfo.LockState = 0;                // 锁状态: 0x01 门反锁 0x02 门没有上锁 0x03验证失败
    g_sysInfo.RSSI_Uplink = 100;            // 信号强弱: 0-100%, 0代表离线状态
    g_sysInfo.ErrorCode = 0;
    g_sysInfo.PSWMode = 1;                  // 密码模式: 0 正常 1 未设置过
    g_sysInfo.Power_Left = 100;             // 电池电量: 1-100%
    g_sysInfo.PowerMode = 0;                // 电源模式: 0 正常模式 1 省电模式

    sprintf(g_sysInfo.UserPin, "%s", DEFAULT_USERPIN);  // 用户PIN码
    sprintf(g_sysInfo.Version, "%s", "1.0");
}

int stor_set_userpin(char *userpin)
{
    if (NULL == userpin)
    {
        os_storage_log(" stor_set_userpin error, userpin is null. \n");
        return ALINK_ERR;
    }
    memset(g_sysInfo.UserPin, 0, sizeof(g_sysInfo.UserPin));
    memcpy(g_sysInfo.UserPin, userpin, 8);
    g_sysInfo.PSWMode = 0;
    os_storage_log(" stor_set_userpin: userpin[%s] \n", userpin);
    return stor_info_write();
}

char * stor_get_userpin(void)
{
    return g_sysInfo.UserPin;
}
int stor_set_powermode(int mode)
{
    g_sysInfo.PowerMode = mode;
    os_storage_log(" stor_set_powermode: PowerMode[%d] \n", mode);
    return stor_info_write();
    return ALINK_OK;
}

int stor_get_powermode(void)
{
    return g_sysInfo.PowerMode;
}

int stor_set_localmac(char *localmac)
{
    if (NULL == localmac)
    {
        os_storage_log(" stor_set_localmac error, localmac is null. \n");
        return ALINK_ERR;
    }
    memset(g_sysInfo.LocalMac, 0, sizeof(g_sysInfo.LocalMac));
    sprintf(g_sysInfo.LocalMac, "%s", localmac);
    os_storage_log(" stor_set_localmac: localmac[%s] \n", localmac);
    return stor_info_write();
}

char * stor_get_localmac(void)
{
    return g_sysInfo.LocalMac;
}

int stor_set_lockmac(char *lockmac)
{
    if (NULL == lockmac)
    {
        os_storage_log(" stor_set_lockmac error, lockmac is null. \n");
        return ALINK_ERR;
    }
    memset(g_sysInfo.LockMac, 0, sizeof(g_sysInfo.LockMac));
    sprintf(g_sysInfo.LockMac, "%s", lockmac);
    os_storage_log(" stor_set_lockmac: lockmac[%s] \n", lockmac);
    return stor_info_write();
    return ALINK_OK;
}



char * stor_get_lockmac(void)
{
    return g_sysInfo.LockMac;
}


int stor_set_lockstate(int state)
{
    g_sysInfo.LockState = state;
    os_storage_log(" stor_set_lockstate: lockstate[%d] \n", state);
    return stor_info_write();
}

void stor_set_lockstate_ex(int state)
{
    g_sysInfo.LockState = state;
    os_storage_log(" stor_set_lockstate_ex: lockstate[%d] \n", state);
}

uint8_t stor_get_lockstate(void)
{
    return g_sysInfo.LockState;
}

void stor_set_powerleft(uint8_t value)
{
    g_sysInfo.Power_Left = value;
    os_storage_log(" stor_set_powerleft: powerleft[%d] \n", value);
}

int stor_info_reset(void)
{
    os_storage_log(" ====== stor_info_reset ====== \n");
    stor_info_default();
    return stor_info_write();
}

SYSTEM_INFO *stor_get_sysinfo(void)
{
    if (g_writeFlag == 0)
    {
        stor_info_init();
    }
    return &g_sysInfo;
}

#if USE_RECORD
int stor_record_lock(BLEUSER_RECORDLIST *pRecordlist)
{
    if (NULL == pRecordlist)
    {
        os_storage_log(" stor_record_lock: list is null.\n");
        return ALINK_ERR;
    }
    memcpy(&g_recordList, pRecordlist, sizeof(BLEUSER_RECORDLIST));
    return stor_info_write();
    return ALINK_OK;
}

BLEUSER_RECORDLIST *stor_get_record(void)
{
    return &g_recordList;
}
#endif

void stor_set_rssi_uplink(uint8_t rssiValue)
{
    g_sysInfo.RSSI_Uplink = rssiValue;
}

uint8_t stor_get_rssi_uplink(void)
{
    return g_sysInfo.RSSI_Uplink;
}

int stor_info_init(void)
{
    int ret = ALINK_OK;
    int ObjectAddr;
    memset(&g_sysInfo, 0, sizeof(g_sysInfo));

    int flag = 0;
    ObjectAddr = WRITE_FLAG_ADDR;
    ret = MicoFlashReadEx(MICO_PARTITION_USER   , &ObjectAddr, (uint8_t *)&flag, 1);
    if (flag == 1)
    {
        g_writeFlag = 1;
        ret = stor_info_read();
    }
    else
    {
        g_writeFlag = 1;
        stor_info_default();
        ret = stor_info_write();
    }
    g_sysInfo.Power_Left = 100;
    g_sysInfo.RSSI_Uplink = 100;
    //stor_set_lockmac("F0C77F9D66538");

    uint8_t reboot = 0;
    ObjectAddr = WRITE_FLAG_ADDR + 4;
    ret = MicoFlashReadEx(MICO_PARTITION_USER,&ObjectAddr, (uint8_t *)&reboot, 1);
    g_rebootTimes = reboot+1;
    if (g_rebootTimes >= 255)
        g_rebootTimes = 0;
    stor_info_write();
    os_storage_log(" stor_info_init: reboot[%d] g_rebootTimes[%d]\n", reboot, g_rebootTimes);

    os_storage_log(" stor_info_init: defaultInfo[%d] ret[%d] \n", flag, ret);
    return ret;
}
#endif




