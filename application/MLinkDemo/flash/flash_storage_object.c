/*
 * flash_object_storage.c
 *
 *  Created on: 2017骞�7鏈�26鏃�
 *      Author: Administrator
 */
#include <stdint.h>
#include <debug.h>
#include "mico.h"
#include "flash.h"
#include "MLinkAppDef.h"
//#include <stddef.h>
#include "MLinkObject.h"
#include "MLinkCommand.h"
#include "flash_storage_object.h"

#define os_storage_log(M, ...)   custom_log("STORAGE", M, ##__VA_ARGS__)
#define _FUNCTION_UNUSED_

#define LIVE_TIMER              (4*60*60)       // default live timer 4 hours

uint32_t g_deviceNum = 0;
uint32_t g_roomNum = 0;
uint32_t g_sceneNum = 0;
uint32_t g_scheduleNum = 0;
uint32_t g_eventNum = 0;
uint32_t g_endpointNum = 0;
uint32_t g_endpointLinkNum = 0;
uint32_t g_linkageInputNum = 0;
uint32_t g_linkageOutputNum = 0;

static DEV_ENDPOINT_STATUS_T g_deviceAttrStatus[DEVICE_NUM] = {0};

#ifdef VERSION_ENDPOINT_12
void storage_add_endpoint_status(uint8_t index, PENDPOINTOBJ_EX_T pendpoint_obj);
#else
void storage_add_endpoint_status(uint8_t index, PSINGLE_DEV_ENDPOINTOBJ_T pendpoint_obj);
#endif

uint32_t  storage_get_endpointNum(void)
{
    return g_endpointNum;
}

/********************************************************
 * function: storage_del_obj
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_obj(char *obj_id, uint32_t start_addr, uint32_t *total_num, uint32_t obj_size)
{
    uint8_t objIndex;
    char *objData = malloc(obj_size);
    uint32_t objAddr = start_addr;
    OSStatus ret = kGeneralErr;
    uint32_t objTotal = *total_num;

    if (objTotal == 0)
    {
        return kNoErr;
    }
    for (objIndex = 0; objIndex < objTotal; objIndex++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objAddr, (uint8_t *)objData, obj_size);
//        os_storage_log("object id is %s", objData);
        if (0 == strcmp(obj_id, objData))
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

    if (objData)
    {
        free(objData);
    }
    return ret;
}

/********************************************************
 * function: storage_del_obj
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_obj_ex(const char *obj_id, uint32_t start_addr, uint32_t *total_num, uint32_t obj_size)
{
    uint8_t objIndex;
    unsigned char *objData = NULL;
    uint32_t objAddr = start_addr;
    OSStatus ret = kGeneralErr;
    uint32_t objTotal = *total_num;
    if (objTotal == 0)
    {
        return kNoErr;
    }
    objData = malloc(obj_size);
    if (objData != NULL)
    {
        for (objIndex = 0; objIndex < objTotal; objIndex++)
        {
            MicoFlashReadEx(MICO_PARTITION_USER, &objAddr, objData, obj_size);
//            os_storage_log("object id is %s", objData);
            if (0 == strcmp(obj_id, objData))
            {
                if (objIndex >= (objTotal-1))
                {
                    objAddr -= obj_size;
                    ret = MicoFlashEraseEx(MICO_PARTITION_USER, objAddr, obj_size);
                }
                else
                {
                    uint16_t mvNumTotal = objTotal-objIndex-1;
                    uint8_t onceNumMax = 4096/obj_size;       // 一次最多移动为4K大小的对象数
                    uint8_t mvNum = 0;
                    uint8_t mvTimes = 0;
                    uint8_t *pObjsData = NULL;
                    uint8_t count = 0;
                    uint32_t objectAddrTemp = objAddr-obj_size;
                    mvTimes = (mvNumTotal+onceNumMax-1)/onceNumMax;
                    pObjsData = (uint8_t *)malloc(onceNumMax*obj_size);
                    if (pObjsData != NULL)
                    {
                        for (count=1; count<=mvTimes; count++)
                        {
                            if (count<mvTimes)
                            {
                                mvNum = onceNumMax;
                            }
                            else
                            {
                                mvNum = mvNumTotal - onceNumMax*(count-1);
                            }
        //                    os_storage_log("mvNum: %d, page: %d, objectAddrTemp: %d", mvNum, count, objectAddrTemp);

                            memset(pObjsData, 0, mvNum*obj_size);
                            MicoFlashRead(MICO_PARTITION_USER, &objAddr, (uint8_t *)pObjsData, mvNum*obj_size);
        //                    os_storage_log("objectAddrTemp: %d , objAddr: %d, mvNum is %d, %s, %s", objectAddrTemp, objAddr, mvNum, pObjsData, pObjsData+(mvNum-1)*obj_size);
                            ret = MicoFlashWriteEx(MICO_PARTITION_USER, &objectAddrTemp, (uint8_t *)pObjsData, mvNum*obj_size);

        //                    objectAddrTemp = objectAddrTemp-obj_size;
        //                    objAddr = objAddr-mvNum*obj_size - obj_size;
        //                    ret = MicoFlashRead(MICO_PARTITION_USER, &objectAddrTemp, (uint8_t *)pObjsData, obj_size);
        //                    os_storage_log("objectAddrTemp: %d, objAddr: %d, %s", objectAddrTemp, objAddr, pObjsData);
        //
        //                    ret = MicoFlashRead(MICO_PARTITION_USER, &objAddr, (uint8_t *)pObjsData, (mvNum+1)*obj_size);
        //                    os_storage_log("objectAddrTemp: %d, objAddr: %d, %s", objectAddrTemp, objAddr, pObjsData);


                        }
                        MicoFlashEraseEx(MICO_PARTITION_USER, objectAddrTemp, obj_size);
                        free(pObjsData);
                    }
                }
                if (ret == kNoErr)
                {
                    os_storage_log("total_num: %d", *total_num);
                    (*total_num)--;
                }
                break;
            }
        }
        os_storage_log("current num : %d, now num: %d, ret: %d, malloc: %d, free: %d", objTotal, *total_num, ret, ((int)(4096/obj_size)+2)*obj_size, MicoGetMemoryInfo()->free_memory);

        free(objData);
    }

    return ret;
}


/********************************************************
 * function: storage_clear_obj
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_clear_obj(uint32_t start_addr, uint32_t *total_num, uint32_t obj_size)
{
    uint32_t objAddr = start_addr;
    OSStatus ret = kGeneralErr;
    uint32_t objTotal = *total_num;
    ret = MicoFlashEraseEx(MICO_PARTITION_USER, objAddr, obj_size*objTotal);
    if (kNoErr == ret)
    {
        *total_num = 0;
    }
    return ret;
}

/********************************************************
 * function: storage_write_local_obj
 * description:
 * input:   1. class:
 *          2. devid:
 * output:  1. endpoint_id
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_endpoint_replace_id(uint8_t class, char *devid, char *endpoint_id)
{
    if (endpoint_id == NULL || devid == NULL)
    {
        return;
    }
    char endpointId[ENDPOINTID_SIZE] = {0};
    uint8_t pos = 0;
    uint8_t start_pos = 0;
    char temp = 0;

    if (endpoint_id[0] < ' ' || endpoint_id[0] > '~')
    {
        return;
    }
    for (pos= 0; pos<strlen(endpoint_id); pos++)
    {
        temp = *(endpoint_id+pos);
        if (temp == '_')
        {
            start_pos = pos;
        }
    }
    sprintf(endpointId, "%d_%s_%s", class, devid, endpoint_id+start_pos+1);
    strcpy(endpoint_id, endpointId);
}

//============== save info in partition named MICO_PARTITION_PARAMETER_1 ===============
/********************************************************
 * function: storage_write_local_obj
 * description:
 * input:   1. pNetDevObj:   point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_local_obj(PNETDEVOBJ_T pNetDevObj)
{
    int ret = kNoErr;

    if (NULL == pNetDevObj)
    {
        return kGeneralErr;
    }
#if 0
    volatile uint32_t ObjectAddr = FLASH_PARAM_NETDEV_OFFSET;
    ret = MicoFlashWriteEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)pNetDevObj, sizeof(NETDEVOBJ_T));
    ObjectAddr = FLASH_PARAM_NETDEV_OFFSET;
    ret = MicoFlashWriteEx(MICO_PARTITION_PARAMETER_2, &ObjectAddr, (uint8_t *)pNetDevObj, sizeof(NETDEVOBJ_T));
#else
    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    if (pNetDevObj->appid[0] == 0)
    {
        sprintf(pNetDevObj->appid, "%s", app_context.appConfig->netdev.appid);
    }
    memcpy(&app_context.appConfig->netdev,pNetDevObj,sizeof(NETDEVOBJ_T));
    memset(app_context.appConfig->netdevEndpoint.alarmEndpoint.devid, 0, sizeof(app_context.appConfig->netdevEndpoint.alarmEndpoint.devid));
    strcpy(app_context.appConfig->netdevEndpoint.alarmEndpoint.devid, pNetDevObj->uuid);
    os_storage_log("net modelname: %s, room_uid: %s, hostname: %s", pNetDevObj->modelname, pNetDevObj->room_uid, app_context.appConfig->netdev.cloudhost);

    mico_system_context_update(mainContext);

#endif
    return ret;
}

/********************************************************
 * function: storage_read_local_devobj
 * description:
 * input:   1. pNetDevObj:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_local_devobj(PNETDEVOBJ_T pNetDevObj)
{
    if (NULL == pNetDevObj)
    {
        return kGeneralErr;
    }

    int ret = kNoErr;
    memset(pNetDevObj, 0, sizeof(NETDEVOBJ_T));
#if 0
    volatile uint32_t ObjectAddr = FLASH_PARAM_NETDEV_OFFSET;
    ret = MicoFlashReadEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)pNetDevObj, sizeof(NETDEVOBJ_T));
#else
    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(pNetDevObj,&app_context.appConfig->netdev,sizeof(NETDEVOBJ_T));
    os_storage_log("net modelname: %x, addr: %x", app_context.appConfig->netdev.modelname, app_context.appConfig->netdev.addr);

#endif
    return ret;
}

/********************************************************
 * function: storage_write_devinfo_obj
 * description:
 * input:   1. pDevInfo: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_devinfo_obj(PDEVINFOOBJ_T pDevInfo)
{
    if (NULL == pDevInfo)
    {
        return kGeneralErr;
    }
#if 1
    volatile uint32_t ObjectAddr = FLASH_USER_FIRM_INFO;
    int ret = kNoErr;
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDevInfo, sizeof(DEVINFOOBJ_T));

#else
    int ret = kNoErr;

    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(&aapp_context.ppConfig->devinfo,pDevInfo,sizeof(DEVINFOOBJ_T));
    mico_system_context_update(mainContext);
#endif
    return ret;
}

/********************************************************
 * function: storage_read_devinfo_obj
 * description:
 * input:   1. pDevInfo: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_devinfo_obj(PDEVINFOOBJ_T pDevInfo)
{
    if (NULL == pDevInfo)
    {
        return kGeneralErr;
    }

    int ret = kNoErr;
    memset(pDevInfo, 0, sizeof(DEVINFOOBJ_T));
#if 1
    volatile uint32_t ObjectAddr = FLASH_USER_FIRM_INFO;
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDevInfo, sizeof(DEVINFOOBJ_T));
#else
    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(pDevInfo,&aapp_context.ppConfig->devinfo, sizeof(DEVINFOOBJ_T));
#endif
    return ret;
}

/********************************************************
 * function: storage_write_local_endpoint
 * description:
 * input:   1. pendpoint: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_local_endpoint(PLOCAL_ENDPOINT_T pendpoint)
{
    if (NULL == pendpoint)
    {
        return kGeneralErr;
    }
    int ret = kNoErr;

    mico_Context_t* mainContext = mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(app_context.appConfig->netdevEndpoint.alarmEndpoint.devid, app_context.appConfig->netdev.uuid, sizeof(app_context.appConfig->netdevEndpoint.alarmEndpoint.devid));
    memcpy(&app_context.appConfig->netdevEndpoint, pendpoint, sizeof(LOCAL_ENDPOINT_T));
    app_context.appConfig->netdevEndpoint.alarmEndpoint.endpointNum = 1;
    mico_system_context_update(mainContext);

    return ret;
}

/********************************************************
 * function: storage_read_local_endpoint
 * description:
 * input:   1. pendpoint: point to a structure that we want to read
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_local_endpoint(PLOCAL_ENDPOINT_T plocal_endpoint)
{
    if (NULL == plocal_endpoint)
    {
        return kGeneralErr;
    }

    int ret = kNoErr;
#if 0
    volatile uint32_t ObjectAddr = FLASH_PARAM_DEVINFO_OFFSET;
    ret = MicoFlashReadEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)pDevInfo, sizeof(DEVINFOOBJ_T));
#else
    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(plocal_endpoint, &app_context.appConfig->netdevEndpoint, sizeof(app_context.appConfig->netdevEndpoint));
#endif
    return ret;
}

/********************************************************
 * function: storage_read_local_endpoint
 * description:
 * input:   1. pendpoint: point to a structure that we want to read
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_local_alarm_state(PALARM_ENDPOINT_T palarm_endpoint)
{
    if (NULL == palarm_endpoint)
    {
        os_storage_log("palarm_endpoint is NULL");
        return kGeneralErr;
    }

    int ret = kNoErr;

    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    if ((&app_context.appConfig->netdevEndpoint.alarmEndpoint) != palarm_endpoint)
    {
        memcpy(&app_context.appConfig->netdevEndpoint.alarmEndpoint, palarm_endpoint, sizeof(ALARM_ENDPOINT_T));
    }
    //note by 2018.3.15 经常写flash 会导致异常
//    ret = mico_system_context_update(mainContext);
//    os_storage_log("alarm type is %d, update result: %d", app_context.appConfig->netdevEndpoint.alarmEndpoint.alarmType, ret);
    return ret;
}

/********************************************************
 * function: storage_read_local_alarm_state
 * description:
 * input:   1. palarm_endpoint: point to a structure that we want to read
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_local_alarm_state(PALARM_ENDPOINT_T *palarm_endpoint)
{
    int ret = kNoErr;

    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    *palarm_endpoint = &app_context.appConfig->netdevEndpoint.alarmEndpoint;
    return ret;
}

/********************************************************
 * function: storage_write_local_endpoint
 * description:
 * input:   1. pDevInfo: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_local_endpoint(PENDPOINT_EX_T pendpoint, uint8_t endpoint_num)
{
    if (NULL == pendpoint)
    {
        return kGeneralErr;
    }
    int ret = kNoErr;
    LOCAL_ENDPOINT_T endpointObj = {0};
    uint8_t updateIndex = 0;
    PENDPOINT_EX_T pEndpointTemp = NULL;
    ENDPOINT_ELE_T endpointEle = {0};

    storage_read_local_endpoint(&endpointObj);

    for (updateIndex=0; updateIndex<endpoint_num; updateIndex++)
    {
        // update alarm endpoints
        pEndpointTemp = pendpoint+updateIndex;
        if (pEndpointTemp->value[0] != 0)       // if value is not null, it means alarm endpoint
        {
            mlink_parse_endpointid(pEndpointTemp->id, &endpointEle);

            if (!strcmp(endpointEle.key, "30"))
            {
                endpointObj.alarmEndpoint.alarmType = atoi(pEndpointTemp->value);
            }
        }
    }
    ret = storage_write_local_endpoint(&endpointObj);
    return ret;
}

//
/********************************************************
 * function: storage_write_cloud_obj
 * description:
 * input:   1. pNetDevObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_cloud_obj(PCLOUDOBJ_T pCloudObj)
{
    if (NULL == pCloudObj)
    {
        return kGeneralErr;
    }
    int ret = kNoErr;
#if 0
    volatile uint32_t ObjectAddr = FLASH_PARAM_CLOUDPARAM_OFFSET;
    ret = MicoFlashWriteEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)pCloudObj, sizeof(CLOUDOBJ_T));
    ObjectAddr = FLASH_PARAM_CLOUDPARAM_OFFSET;
    ret = MicoFlashWriteEx(MICO_PARTITION_PARAMETER_2, &ObjectAddr, (uint8_t *)pCloudObj, sizeof(CLOUDOBJ_T));
#else
    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(app_context.appConfig->netdev.appid, &pCloudObj->appid, sizeof(pCloudObj->appid));
    memcpy(app_context.appConfig->netdev.ask, &pCloudObj->ask, sizeof(pCloudObj->ask));
    memcpy(app_context.appConfig->netdev.psk, &pCloudObj->psk, sizeof(pCloudObj->psk));
    memcpy(app_context.appConfig->netdev.cloudhost, &pCloudObj->cloudhost, sizeof(pCloudObj->cloudhost));
    memcpy(app_context.appConfig->netdev.dsk, &pCloudObj->dsk, sizeof(pCloudObj->dsk));

    memcpy(&app_context.appConfig->cloud, pCloudObj, sizeof(CLOUDOBJ_T));
    mico_system_context_update(mainContext);
#endif
    return ret;
}

/********************************************************
 * function: storage_read_cloud_obj
 * description:
 * input:   1. pCloudObj:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_cloud_obj(PCLOUDOBJ_T pCloudObj)
{
    if (NULL == pCloudObj)
    {
        return kGeneralErr;
    }
//    volatile uint32_t ObjectAddr = FLASH_PARAM_CLOUDPARAM_OFFSET;
    int ret = kNoErr;
    memset(pCloudObj, 0, sizeof(CLOUDOBJ_T));
#if 0
    volatile uint32_t ObjectAddr = FLASH_PARAM_CLOUDPARAM_OFFSET;
    ret = MicoFlashReadEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)pCloudObj, sizeof(CLOUDOBJ_T));
#else

    mico_Context_t* mainContext=  mico_system_context_get();
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    memcpy(pCloudObj->appid, &app_context.appConfig->netdev.appid, sizeof(pCloudObj->appid));
    memcpy(pCloudObj->ask, &app_context.appConfig->netdev.ask, sizeof(pCloudObj->ask));
    memcpy(pCloudObj->cloudhost, &app_context.appConfig->netdev.cloudhost, sizeof(pCloudObj->cloudhost));
    memcpy(pCloudObj->dsk, &app_context.appConfig->netdev.dsk, sizeof(pCloudObj->dsk));
    memcpy(pCloudObj->psk, &app_context.appConfig->netdev.psk, sizeof(pCloudObj->psk));
#endif
    return ret;
}


/********************************************************
 * function: storage_write_dev_obj
 * description:  write some devices to initial address
 * input:   1. pDeviceObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_dev_obj(OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num, PDEVICEOBJ_T pDeviceObj)
{
    if (NULL == pDeviceObj)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_deviceNum > DEVICE_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_DEVICE_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = obj_num;
    uint8_t updateIndex = 0;
    DEVICEOBJ_T devObj;
    memset(&devObj, 0, sizeof(DEVICEOBJ_T));


    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        PDEVICEOBJ_T pDevObj = pDeviceObj+updateIndex;
        for (count=0; count<g_deviceNum; count++)
        {
            MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&devObj, sizeof(DEVICEOBJ_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (1)//(0 == strcmp(pDevObj->deviceId, devObj.deviceId))
                {
                    uint32_t ObjectAddrTemp = ObjectAddr - sizeof(DEVICEOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddrTemp, (uint8_t *)pDeviceObj+updateIndex*sizeof(DEVICEOBJ_T), sizeof(DEVICEOBJ_T));
                    if (ret != kNoErr)
                    {
                        os_storage_log("write device obj fail! errcode: %d", ret);
                    }
                    if (updateIndex < (obj_num - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)pDeviceObj + updateIndex*sizeof(DEVICEOBJ_T);
                        uint8_t *src = (uint8_t *)pDeviceObj + (updateIndex + 1)*sizeof(DEVICEOBJ_T);
                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(DEVICEOBJ_T));
                    }
                    updateCount --;
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (1)//(0 == strcmp(pDevObj->deviceId, devObj.deviceId))
                {
                    ObjectAddr = FLASH_USER_DEVICE_OFFSET+count*sizeof(DEVICEOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDeviceObj+updateIndex*sizeof(DEVICEOBJ_T), sizeof(DEVICEOBJ_T));
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_DEVICE_OFFSET;
    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_DEVICE_OFFSET+g_deviceNum*sizeof(DEVICEOBJ_T);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDeviceObj, updateCount*sizeof(DEVICEOBJ_T));
        if ( ret == kNoErr )
        {
            g_deviceNum += updateCount;
        }
        os_storage_log("write ret is %d, g_deviceNum = %d", ret, g_deviceNum);
    }

    return ret;
}

/********************************************************
 * function: storage_read_dev_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj:   point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_dev_obj_batch(uint8_t obj_num, uint8_t start_index, PDEVICEOBJ_T pDevice_obj)
{
    if (NULL == pDevice_obj)
    {
        os_storage_log("pDevice_obj is NULL !!!");
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_DEVICE_OFFSET+start_index*sizeof(DEVICEOBJ_T);
    int ret = FLASH_OPERATE_OK;
    memset(pDevice_obj, 0, obj_num*sizeof(DEVICEOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDevice_obj, obj_num*sizeof(DEVICEOBJ_T));

    return ret;
}

/********************************************************
 * function: storage_read_dev_obj
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_dev_obj(char *devid, PDEVICEOBJ_T pDeviceObj)
{
    if (NULL == pDeviceObj)
    {
        return kGeneralErr;
    }
    int count = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_DEVICE_OFFSET;
    int ret = kGeneralErr;

    for (count=0; count < g_deviceNum; count ++)
    {
        memset(pDeviceObj, 0, sizeof(DEVICEOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDeviceObj, sizeof(DEVICEOBJ_T));
        if (ret == kNoErr)
        {
//            os_storage_log("flash deviceId : %s, recv devid: %s", pDeviceObj->deviceId, devid);
            if(1)// (0 == strcmp(pDeviceObj->deviceId, devid))
            {
                ret = count + 1;
                return ret;
            }
        }
    }

    return kGeneralErr;
}


/********************************************************
 * function: storage_read_dev_obj
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_dev_obj_by_addr(char *addr, PDEVICEOBJ_T pDeviceObj)
{
    if (NULL == pDeviceObj)
    {
        return kGeneralErr;
    }
    int count = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_DEVICE_OFFSET;
    int ret = kGeneralErr;

    for (count=0; count < g_deviceNum; count ++)
    {
        memset(pDeviceObj, 0, sizeof(DEVICEOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pDeviceObj, sizeof(DEVICEOBJ_T));
        if (ret == kNoErr)
        {
//            os_storage_log("flash deviceId : %s, recv devid: %s", pDeviceObj->deviceId, devid);
            if (0 == strcmp(pDeviceObj->addr, addr))
            {
                ret = count + 1;
                return ret;
            }
        }
    }

    return kGeneralErr;
}

/*============== save info in partition named MICO_PARTITION_USER ===============*/
#if 0

/********************************************************
 * function: storage_write_room_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_room_obj(OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num, PROOMOBJ_T pRoomObj)
{
    if (NULL == pRoomObj)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_roomNum > ROOM_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ROOM_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = obj_num;
    uint8_t updateIndex = 0;
    ROOMOBJ_T roomObj;
    memset(&roomObj, 0, sizeof(ROOMOBJ_T));


    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        PROOMOBJ_T pDevObjTemp = pRoomObj+updateIndex;
        for (count=0; count<g_roomNum; count++)
        {
            MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&roomObj, sizeof(ROOMOBJ_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (0 == strcmp(pDevObjTemp->roomId, roomObj.roomId))
                {
                    if (updateIndex < (obj_num - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)pRoomObj + updateIndex*sizeof(ROOMOBJ_T);
                        uint8_t *src = (uint8_t *)pRoomObj + (updateIndex + 1)*sizeof(ROOMOBJ_T);
                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(ROOMOBJ_T));
                    }
                    updateCount --;
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (0 == strcmp(pDevObjTemp->roomId, roomObj.roomId))
                {
                    ObjectAddr = FLASH_USER_ROOM_OFFSET+count*sizeof(ROOMOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pRoomObj+updateIndex*sizeof(ROOMOBJ_T), sizeof(ROOMOBJ_T));
                    if (ret == kNoErr)
                    {
                        ret = FLASH_OPERATE_OK;
                    }
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_ROOM_OFFSET;
    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_ROOM_OFFSET+g_roomNum*sizeof(ROOMOBJ_T);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pRoomObj, updateCount*sizeof(ROOMOBJ_T));
        if ( ret == kNoErr )
        {
            g_roomNum += updateCount;
        }
        os_storage_log("write ret is %d, g_roomNum = %d", ret, g_roomNum);
    }

    return ret;
}
#else
/********************************************************
 * function: storage_write_room_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_room_obj( uint8_t obj_num, uint8_t startIndex, PROOMOBJ_T pRoomObj)
{
    if (pRoomObj == NULL)
    {
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ROOM_OFFSET + startIndex*sizeof(ROOMOBJ_T);
    int ret = kNoErr;
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pRoomObj, obj_num*sizeof(ROOMOBJ_T));
    os_storage_log("objectAddr is :%x", ObjectAddr);
    return ret;
}
#endif


/********************************************************
 * function: storage_read_room_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_room_obj_batch( unsigned char obj_num, unsigned char startIndex, PROOMOBJ_T pRoomObj )
{
    if (NULL == pRoomObj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ROOM_OFFSET + startIndex * sizeof(ROOMOBJ_T);
    int ret = 0;
    memset(pRoomObj, 0, obj_num*sizeof(ROOMOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pRoomObj, obj_num*sizeof(ROOMOBJ_T));
    os_storage_log("room storage addr: %x, room name:%s", ObjectAddr, pRoomObj->name);
    return ret;
}

/********************************************************
 * function: storage_read_roomobj_id
 * description:
 * input:   1. deviceId: which we want to search
 * output:  2. pDevInfo:
 * return:  the number of room object we find
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_roomobj_id(char *roomId, PROOMOBJ_T pRoomObj)
{
    int count = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_ROOM_OFFSET;
    int ret = kGeneralErr;
    if (NULL == pRoomObj)
    {
        return ret;
    }


    for (count = 0; count<g_roomNum; count++)
    {
        memset(pRoomObj, 0, sizeof(ROOMOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pRoomObj, sizeof(ROOMOBJ_T));
        if ((ret == kNoErr) && (0 == strcmp(roomId, pRoomObj->roomId)))
        {
            ret = count + 1;
            os_storage_log("find out the device !!!");
            return ret;
        }
    }
    return kGeneralErr;
}



/********************************************************
 * function: storage_write_scene_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_scene_obj(OBJECT_UPDATE_TYPE_E update_type, uint8_t sceneNum, PSCENEOBJ_T pscene_obj)
{
    if (NULL == pscene_obj)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_sceneNum > SCENE_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_SCENE_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = sceneNum;
    uint8_t updateIndex = 0;
    SCENEOBJ_T sceneObj;
    memset(&sceneObj, 0, sizeof(SCENEOBJ_T));


    for (updateIndex = 0; updateIndex < sceneNum; updateIndex ++)
    {
        PSCENEOBJ_T pSceneObj = pscene_obj+updateIndex;
        for (count=0; count<g_sceneNum; count++)
        {
            MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&sceneObj, sizeof(SCENEOBJ_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (0 == strcmp(pSceneObj->sceneId, sceneObj.sceneId))
                {
                    if (updateIndex < (sceneNum - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)pscene_obj + updateIndex*sizeof(SCENEOBJ_T);
                        uint8_t *src = (uint8_t *)pscene_obj + (updateIndex + 1)*sizeof(SCENEOBJ_T);
                        memcpy(dst, src, (sceneNum-updateIndex-1)*sizeof(SCENEOBJ_T));
                    }
                    updateCount --;
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (0 == strcmp(pSceneObj->sceneId, sceneObj.sceneId))
                {
                    ObjectAddr = FLASH_USER_SCENE_OFFSET+count*sizeof(SCENEOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pscene_obj+updateIndex*sizeof(SCENEOBJ_T), sizeof(SCENEOBJ_T));
                    if (ret == kNoErr)
                    {
                        ret = FLASH_OPERATE_OK;
                    }
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_SCENE_OFFSET;
    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_SCENE_OFFSET+g_sceneNum*sizeof(SCENEOBJ_T);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pscene_obj, updateCount*sizeof(SCENEOBJ_T));
        if ( ret == kNoErr )
        {
            g_sceneNum += updateCount;
        }
        os_storage_log("write ret is %d, g_sceneNum = %d", ret, g_sceneNum);
    }

    return ret;
}

/********************************************************
 * function: storage_read_scene_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. obj_num:   the number of object you want to read
 *          2. start_index: start to read from which
 * output:  1.pSceneObj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_scene_obj_batch(uint8_t obj_num, uint8_t start_index, PSCENEOBJ_T pSceneObj)
{
    if (NULL == pSceneObj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_SCENE_OFFSET+start_index*sizeof(SCENEOBJ_T);
    int ret = 0;
    os_storage_log("scene num is %d, startIndex: %d", obj_num, start_index);
    memset(pSceneObj, 0, obj_num * sizeof(SCENEOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pSceneObj, obj_num * sizeof(SCENEOBJ_T));

    return ret;
}

/********************************************************
 * function: storage_read_scene_obj
 * description:   read some info of devices from initial address
 * input:   1. objid:   point to a structure that we want to save
 * output:  1. pscene_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_scene_obj(char *objid, PSCENEOBJ_T pscene_obj)
{
    if (NULL == pscene_obj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_SCENE_OFFSET;
    OSStatus ret = 0;

    int count = 0;
    SCENEOBJ_T sceneObj;
    memset(&sceneObj, 0, sizeof(SCENEOBJ_T));

    for (count=0; count<g_sceneNum; count++)
    {
        memset(&sceneObj, 0, sizeof(SCENEOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&sceneObj, sizeof(SCENEOBJ_T));
        if ((ret == kNoErr) && (0 == strcmp(sceneObj.sceneId, objid)))
        {
            memcpy(pscene_obj, &sceneObj, sizeof(SCENEOBJ_T));
            ret = count+1;
            return ret;
        }
//        else
//        {
//            os_storage_log("sceneObj.sceneId: %s", sceneObj.sceneId);
//        }
    }
    if (count>=g_sceneNum)
    {
        return kGeneralErr;
    }

    return ret;
}


/********************************************************
 * function: storage_write_devlink_obj
 * description:  write some devices to initial address
 * input:   1. obj_num:  point to a structure that we want to save
 *          2. start_index:     the number of object you want to write
 *          3. pinput_obj: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_linkage_input_obj( uint8_t obj_num, uint8_t start_index, PLINKAGE_INPUT_TASK_T pinput_obj )
{
    if (NULL == pinput_obj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LINKAGE_INPUT_OFFSET+start_index*sizeof(LINKAGE_INPUT_TASK_T);
    int ret = kNoErr;
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pinput_obj, obj_num*sizeof(LINKAGE_INPUT_TASK_T));

    return ret;
}

/********************************************************
 * function: storage_read_linkage_input_obj
 * description:   read devlink
 * input:   1. obj_num:
 *          2. start_index:
 * output:  1. pinput_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_linkage_input_obj( uint8_t obj_num, uint8_t start_index, PLINKAGE_INPUT_TASK_T pinput_obj )
{
    if (NULL == pinput_obj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LINKAGE_INPUT_OFFSET+start_index*sizeof(LINKAGE_INPUT_TASK_T);
    int ret = kNoErr;
    memset(pinput_obj, 0, obj_num*sizeof(LINKAGE_INPUT_TASK_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pinput_obj, obj_num*sizeof(LINKAGE_INPUT_TASK_T));
    return ret;
}


/********************************************************
 * function: storage_write_linkage_output_obj
 * description:  write some devices to initial address
 * input:   1. obj_num:
 *          2. start_index:
 *          3. poutput_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_linkage_output_obj( uint8_t obj_num, uint8_t start_index, PLINKAGE_OUTPUT_TASK_T poutput_obj )
{
    if (NULL == poutput_obj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LINKAGE_OUTPUT_OFFSET+start_index*sizeof(LINKAGE_OUTPUT_TASK_T);
    int ret = kNoErr;
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)poutput_obj, obj_num*sizeof(LINKAGE_OUTPUT_TASK_T));

    return ret;
}

/********************************************************
 * function: storage_read_linkage_output_obj
 * description:   read devlink
 * input:   1. obj_num:
 *          2. start_index:
 * output:  1. poutput_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_linkage_output_obj( uint8_t obj_num, uint8_t start_index, PLINKAGE_OUTPUT_TASK_T poutput_obj )
{
    if (NULL == poutput_obj)
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LINKAGE_OUTPUT_OFFSET+start_index*sizeof(LINKAGE_OUTPUT_TASK_T);
    int ret = kNoErr;
    memset(poutput_obj, 0, obj_num*sizeof(LINKAGE_OUTPUT_TASK_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)poutput_obj, obj_num*sizeof(LINKAGE_OUTPUT_TASK_T));
    return ret;
}

/********************************************************
 * function: storage_write_event_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:       the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_event_obj(OBJECT_UPDATE_TYPE_E update_type, unsigned char obj_num, PEVENTOBJ_T pevent_obj)
{

    if (NULL == pevent_obj)
    {
        return FLASH_OPERATE_FAIL;
    }
    if ((update_type == OBJECT_UPDATE_ADD) && (g_eventNum > EVENT_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_EVENT_OFFSET;
    int ret = FLASH_OPERATE_OK;
    uint8_t count = 0;
    uint8_t updateCount = obj_num;
    uint8_t updateIndex = 0;
    EVENTOBJ_T eventObj;
    memset(&eventObj, 0, sizeof(EVENTOBJ_T));

    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        PEVENTOBJ_T pEventObj = pevent_obj+updateIndex;
        for (count=0; count<g_eventNum; count++)
        {
            MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&eventObj, sizeof(EVENTOBJ_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (!strcmp(pEventObj->id, eventObj.id))
                {
                    if (updateIndex < (obj_num - 1))                      // Delete member had exist from the add list
                    {
                        uint8_t *dst = (uint8_t *)pevent_obj + updateIndex*sizeof(EVENTOBJ_T);
                        uint8_t *src = (uint8_t *)pevent_obj + (updateIndex + 1)*sizeof(EVENTOBJ_T);
                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(EVENTOBJ_T));
                    }
                    updateCount --;
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (!strcmp(pEventObj->id, eventObj.id))
                {
                    ObjectAddr = FLASH_USER_EVENT_OFFSET + count*sizeof(EVENTOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pevent_obj+updateIndex*sizeof(EVENTOBJ_T), sizeof(EVENTOBJ_T));
                    if (ret == kNoErr)
                    {
                        ret = FLASH_OPERATE_OK;
                    }
                    break;
                }
            }
        }

        ObjectAddr = FLASH_USER_EVENT_OFFSET;
    }

    if (update_type == OBJECT_UPDATE_ADD)
    {
        if (updateCount == 0)
        {
            return FLASH_OBJECT_EXIST;
        }
        ObjectAddr = FLASH_USER_EVENT_OFFSET+g_eventNum*sizeof(EVENTOBJ_T);
        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pevent_obj, updateCount*sizeof(EVENTOBJ_T));
        if ( ret == kNoErr )
        {
            g_eventNum += updateCount;
        }
        os_storage_log("write ret is %d, g_eventNum = %d", ret, g_eventNum);
    }
    return ret;
}

/********************************************************
 * function: storage_read_event_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_event_obj( char *event_id, PEVENTOBJ_T pevent_obj )
{
    if (NULL == pevent_obj)
    {
        return kGeneralErr;
    }
    int count = 0;
    volatile uint32_t ObjectAddr = FLASH_USER_EVENT_OFFSET;
    int ret = kGeneralErr;

    for (count=0; count < g_eventNum; count ++)
    {
        memset(pevent_obj, 0, sizeof(EVENTOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pevent_obj, sizeof(EVENTOBJ_T));
        if (ret == kNoErr)
        {
            if (!strcmp(pevent_obj->id, event_id))
            {
                ret = count + 1;
                os_storage_log("flash id : %s, find event_id: %s", pevent_obj->id, event_id);
                return ret;
            }
        }
    }
    return ret;
}

/********************************************************
 * function: storage_read_dev_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. pDeviceObj:   point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_event_obj_batch(uint8_t obj_num, uint8_t start_index, PEVENTOBJ_T pevent_obj)
{
    if (NULL == pevent_obj)
    {
        os_storage_log("pevent_obj is NULL !!!");
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_EVENT_OFFSET+start_index*sizeof(EVENTOBJ_T);
    int ret = FLASH_OPERATE_OK;
    memset(pevent_obj, 0, obj_num*sizeof(EVENTOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pevent_obj, obj_num*sizeof(EVENTOBJ_T));

    return ret;
}

/********************************************************
 * function: storage_write_schedule_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_schedule_obj(OBJECT_UPDATE_TYPE_E update_type, PSCHEDULEOBJ_T pschedule_obj)
{
    if (NULL == pschedule_obj || (g_scheduleNum>=SCHEDULE_NUM))
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_SCHEDULE_OFFSET;
    int ret = FLASH_OPERATE_OK;

    int count = 0;
    SCHEDULEOBJ_T scheduleObj;
    for (count=0; count<g_scheduleNum; count++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&scheduleObj, sizeof(SCHEDULEOBJ_T));
        if (scheduleObj.id == pschedule_obj->id)
        {
            if (update_type == OBJECT_UPDATE_ADD)
            {
                return FLASH_OBJECT_EXIST;
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                break;
            }

        }
    }

    ObjectAddr = FLASH_USER_SCHEDULE_OFFSET+count*sizeof(SCHEDULEOBJ_T);
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pschedule_obj, sizeof(SCHEDULEOBJ_T));
    if (ret == kNoErr && (update_type == OBJECT_UPDATE_ADD))
    {
        g_scheduleNum++;
    }

    return ret;
}

/********************************************************
 * function: storage_read_schedule_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_schedule_obj(PSCHEDULEOBJ_T pScheduleObj, unsigned char num, unsigned char startIndex)
{
    if (NULL == pScheduleObj)
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_SCHEDULE_OFFSET;
    int ret = FLASH_OPERATE_OK;
    memset(pScheduleObj, 0, num*sizeof(SCHEDULEOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pScheduleObj, num*sizeof(SCHEDULEOBJ_T));
    return ret;
}

/********************************************************
 * function: storage_write_loopaction_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_loopaction_obj(PLOOPACTION_T ploopActionObj, unsigned char num, unsigned char startIndex)
{
    if (NULL == ploopActionObj)
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LOOPACTION_OFFSET;
    int ret = FLASH_OPERATE_OK;
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)ploopActionObj, num*sizeof(LOOPACTION_T));

    return ret;
}

/********************************************************
 * function: storage_read_loopaction_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_loopaction_obj(PLOOPACTION_T ploopActionObj, unsigned char num, unsigned char startIndex)
{
    if (NULL == ploopActionObj)
    {
        return FLASH_OPERATE_FAIL;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_LOOPACTION_OFFSET;
    int ret = FLASH_OPERATE_OK;
    memset(ploopActionObj, 0, num*sizeof(LOOPACTION_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)ploopActionObj, num*sizeof(LOOPACTION_T));
    return ret;
}



#ifdef VERSION_ENDPOINT_12

/********************************************************
 * function: storage_write_endpoint_obj
 * description:  save endpoint
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_endpoint_obj( uint8_t obj_num, uint8_t start_index, PENDPOINTOBJ_EX_T pendpoint_obj )
{
    if ((start_index+obj_num)>=ENDPOINT_NUM || (NULL == pendpoint_obj))
    {
        os_storage_log("Single device endpoints num is %d, pendpoint_obj maybe NULL ", obj_num+start_index+1);
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+start_index*sizeof(ENDPOINTOBJ_EX_T);
    int ret = kNoErr;
//    os_storage_log("endpointlink write index is %d !!!", start_index);
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pendpoint_obj, sizeof(ENDPOINTOBJ_EX_T));
    return ret;
}

/********************************************************
 * function: storage_write_endpoint_obj_ex
 * description:  save endpoint
 * input:   1. update_type:
 *          2. obj_num:
 *          3. pEndpointObj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_endpoint_obj_ex( OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num, PENDPOINTOBJ_EX_T pEndpointObj )
{
    if ( NULL == pEndpointObj )
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    int ret = kNoErr;
    uint8_t count = 0;
    uint8_t updateCount = 0;
    uint8_t updateIndex = 0;
    ENDPOINTOBJ_EX_T endpointObj;
    PENDPOINTOBJ_EX_T pEndpointObjTemp;

    os_storage_log("endpoint fid: %s", pEndpointObj->fid);
    memset(&endpointObj, 0, sizeof(ENDPOINTOBJ_EX_T));
    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        pEndpointObjTemp = pEndpointObj+updateIndex;
        for (count=0; count<g_endpointNum; count++)
        {
            ret = MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(ENDPOINTOBJ_EX_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (0 == strcmp(pEndpointObjTemp->fid, endpointObj.fid))        // 更新同设备下的端点信息
                {
                    uint8_t updateEndpointNum = 0;
                    uint8_t localEndpointNum = 0;
                    updateCount = 0;
                    for (updateEndpointNum = 0; updateEndpointNum < pEndpointObjTemp->endpointNum; updateEndpointNum++)
                    {
                        for (localEndpointNum=0; localEndpointNum < endpointObj.endpointNum; localEndpointNum++)
                        {
                            if (0 == strcmp(pEndpointObjTemp->endpointlist[updateEndpointNum].id, endpointObj.endpointlist[localEndpointNum].id))
                            {
                                // If id is exist, we cover it
                                memcpy(&endpointObj.endpointlist[localEndpointNum], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_EX_T));
                                break;
                            }
                        }
                        if (localEndpointNum == endpointObj.endpointNum)
                        {
                            if ((localEndpointNum+updateCount) < ENDPOINTLIST_NUM)
                            {
                                memcpy(&endpointObj.endpointlist[localEndpointNum+updateCount], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_EX_T));
                                updateCount ++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    endpointObj.endpointNum += updateCount;
                    ObjectAddr -= sizeof(ENDPOINTOBJ_EX_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(ENDPOINTOBJ_EX_T));
                    if ((updateCount > 0)&&(ret == kNoErr))
                    {
                        storage_add_endpoint_status(count, &endpointObj);
                    }
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (0 == strcmp(pEndpointObjTemp->fid, endpointObj.fid))
                {
                    uint8_t updateEndpointNum = 0;
                    uint8_t localEndpointNum = 0;
                    for (updateEndpointNum = 0; updateEndpointNum < pEndpointObjTemp->endpointNum; updateEndpointNum++)
                    {
                        for (localEndpointNum=0; localEndpointNum < endpointObj.endpointNum; localEndpointNum++)
                        {
                            if (!strcmp(pEndpointObjTemp->endpointlist[updateEndpointNum].id, endpointObj.endpointlist[localEndpointNum].id))
                            {
                                memcpy(&endpointObj.endpointlist[localEndpointNum], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_EX_T));
                                break;
                            }
                        }
                    }
                    ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+count*sizeof(ENDPOINTOBJ_EX_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(ENDPOINTOBJ_EX_T));
                    break;
                }

            }
        }
        if (update_type == OBJECT_UPDATE_ADD)
        {
            if (count >= g_endpointNum)
            {
                ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+g_endpointNum*sizeof(ENDPOINTOBJ_EX_T);
                ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObjTemp, sizeof(ENDPOINTOBJ_EX_T));
                storage_add_endpoint_status(g_endpointNum, pEndpointObjTemp);
                if ( ret == kNoErr )
                {
                    g_endpointNum ++;
                }
            }

        }
        ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    }

    return ret;
}


/********************************************************
 * function: storage_read_endpoint_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. num:  the number of object you want to read
 *          2. startIndex: tell us to get infomation from which index
 * output:  1. pEndpointObj: You must
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_endpoint_obj_batch(unsigned char obj_num, unsigned char startIndex, PENDPOINTOBJ_EX_T pEndpointObj)
{
    if (NULL == pEndpointObj)
    {
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET + startIndex * sizeof(ENDPOINTOBJ_EX_T);
    int ret = kNoErr;
    memset(pEndpointObj, 0, obj_num*sizeof(ENDPOINTOBJ_EX_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObj, obj_num*sizeof(ENDPOINTOBJ_EX_T));
//    os_storage_log("endpoint object startIndex: %d, obj_num: %d, object addr: %x", startIndex, obj_num, ObjectAddr);
    return ret;
}

/********************************************************
 * function: storage_read_endpoint_obj_batch
 * description:   read endpoint info matching fid
 * input:   1. fid:
 * output:  1. pEndpointObj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_endpoint_obj(char *fid, PENDPOINTOBJ_EX_T pEndpointObj)
{
    if (NULL == pEndpointObj || (NULL == fid))
    {
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    OSStatus ret = 0;
    uint8_t index=0;

    for (index=0; index < g_endpointNum; index++)
    {
        memset(pEndpointObj, 0, sizeof(ENDPOINTOBJ_EX_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObj, sizeof(ENDPOINTOBJ_EX_T));
        if (0 == strcmp(fid, pEndpointObj->fid))
        {
            ret = index+1;
            return ret;
        }
    }
    if (index >= g_endpointNum)
    {
        os_storage_log("ret = %d, index=%d, g_endpointNum=%d", ret, index, g_endpointNum);
        ret = kGeneralErr;
    }
    return ret;
}

#else

/********************************************************
 * function: storage_write_endpoint_obj
 * description:  save endpoint
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_endpoint_obj(  uint8_t obj_num, uint8_t start_index, PSINGLE_DEV_ENDPOINTOBJ_T pendpoint_obj )
{
    if ((start_index+obj_num)>=ENDPOINT_NUM || (NULL == pendpoint_obj))
    {
        os_storage_log("Single device endpoints num is %d, pendpoint_obj maybe NULL ", obj_num+start_index+1);
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+start_index*sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
    int ret = kNoErr;
//    os_storage_log("endpointlink write index is %d !!!", start_index);
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pendpoint_obj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
    return ret;
}

/********************************************************
 * function: storage_write_endpoint_obj_ex
 * description:  save endpoint
 * input:   1. update_type:
 *          2. obj_num:
 *          3. pEndpointObj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_endpoint_obj_ex( OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num, PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObj )
{
    if ( NULL == pEndpointObj )
    {
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    int ret = kNoErr;
    uint8_t count = 0;
    uint8_t updateCount = 0;
    uint8_t updateIndex = 0;
    SINGLE_DEV_ENDPOINTOBJ_T endpointObj;
    PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObjTemp;

    os_storage_log("endpointid: %s", pEndpointObj->endpointlist[0].id);
    memset(&endpointObj, 0, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
    {
        pEndpointObjTemp = pEndpointObj+updateIndex;
        for (count=0; count<g_endpointNum; count++)
        {
            ret = MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
            if (update_type == OBJECT_UPDATE_ADD)
            {
                if (0 == strcmp(pEndpointObjTemp->fid, endpointObj.fid))
                {
                    uint8_t updateEndpointNum = 0;
                    uint8_t localEndpointNum = 0;
                    updateCount = 0;
                    for (updateEndpointNum = 0; updateEndpointNum < pEndpointObjTemp->endpointNum; updateEndpointNum++)
                    {
                        for (localEndpointNum=0; localEndpointNum < endpointObj.endpointNum; localEndpointNum++)
                        {
                            if (0 == strcmp(pEndpointObjTemp->endpointlist[updateEndpointNum].id, endpointObj.endpointlist[localEndpointNum].id))
                            {
                                // If id is exist, we cover it
                                memcpy(&endpointObj.endpointlist[localEndpointNum], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_T));
                                break;
                            }
                        }
                        if (localEndpointNum == endpointObj.endpointNum)
                        {
                            memcpy(&endpointObj.endpointlist[localEndpointNum], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_T));
                            updateCount ++;
                        }
                    }
                    endpointObj.endpointNum += updateCount;
                    ObjectAddr -= sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                    break;
                }
            }
            else if (update_type == OBJECT_UPDATE_MODIFY)
            {
                if (0 == strcmp(pEndpointObjTemp->fid, endpointObj.fid))
                {
                    uint8_t updateEndpointNum = 0;
                    uint8_t localEndpointNum = 0;
                    for (updateEndpointNum = 0; updateEndpointNum < pEndpointObjTemp->endpointNum; updateEndpointNum++)
                    {
                        for (localEndpointNum=0; localEndpointNum < endpointObj.endpointNum; localEndpointNum++)
                        {
                            if (!strcmp(pEndpointObjTemp->endpointlist[updateEndpointNum].id, endpointObj.endpointlist[localEndpointNum].id))
                            {
                                memcpy(&endpointObj.endpointlist[localEndpointNum], &pEndpointObjTemp->endpointlist[updateEndpointNum], sizeof(ENDPOINT_T));
                                break;
                            }
                        }
                    }
                    ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+count*sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                    break;
                }

            }
        }
        if (update_type == OBJECT_UPDATE_ADD)
        {
            if (count >= g_endpointNum)
            {
                ObjectAddr = FLASH_USER_ENDPOINT_OFFSET+g_endpointNum*sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
                ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObjTemp, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                os_storage_log("endpoint storage address: 0x%x", ObjectAddr);
                storage_add_endpoint_status(g_endpointNum, pEndpointObjTemp);
                if ( ret == kNoErr )
                {
                    g_endpointNum ++;
                }
            }

        }
        ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    }

    return ret;
}


/********************************************************
 * function: storage_read_endpoint_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. num:  the number of object you want to read
 *          2. startIndex: tell us to get infomation from which index
 * output:  1. pEndpointObj: You must
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_endpoint_obj_batch(unsigned char obj_num, unsigned char startIndex, PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObj)
{
    if (NULL == pEndpointObj)
    {
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET + startIndex * sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
    int ret = kNoErr;
    memset(pEndpointObj, 0, obj_num*sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObj, obj_num*sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
//    os_storage_log("endpoint object startIndex: %d, obj_num: %d, object addr: %x", startIndex, obj_num, ObjectAddr);
    return ret;
}

/********************************************************
 * function: storage_read_endpoint_obj_batch
 * description:   read endpoint info matching fid
 * input:   1. fid:
 * output:  1. pEndpointObj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_endpoint_obj(char *fid, PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObj)
{
    if (NULL == pEndpointObj || (NULL == fid))
    {
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINT_OFFSET;
    OSStatus ret = 0;
    uint8_t index=0;

    for (index=0; index < g_endpointNum; index++)
    {
        memset(pEndpointObj, 0, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
        if (0 == strcmp(fid, pEndpointObj->fid))
        {
            ret = index+1;
            return ret;
        }
    }
    if (index >= g_endpointNum)
    {
        os_storage_log("ret = %d, index=%d, g_endpointNum=%d", ret, index, g_endpointNum);
        ret = kGeneralErr;
    }
    return ret;
}
#endif


/********************************************************
 * function: storage_write_endpointlink_obj
 * description:  save endpoint
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_endpointlink_obj( uint8_t obj_num, uint8_t start_index, PENDPOINTLINK_OBJ_T pendpointlink_obj )
{
    if ((start_index+obj_num)>=ENDPOINTLINKOBJ_NUM || (NULL == pendpointlink_obj))
    {
        os_storage_log("endpointlink obj num will be %d, pendpointlink_obj maybe null", obj_num+start_index+1);
        return kGeneralErr;
    }
    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET+start_index*sizeof(ENDPOINTLINK_OBJ_T);
    int ret = kNoErr;
    os_storage_log("endpointlink write index is %d !!!", start_index);
    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pendpointlink_obj, sizeof(ENDPOINTLINK_OBJ_T));
    return ret;
}

///********************************************************
// * function: storage_write_endpoint_obj
// * description:  save endpoint
// * input:   1. pRoomObj: point to a structure that we want to save
// *          2. num:     the number of object you want to write
// *          3. startIndex: tell us that operation will excute from which index
// * output:
// * return:
// * auther:   chenb
// * other:
//*********************************************************/
//OSStatus storage_add_endpointlink_obj( OBJECT_UPDATE_TYPE_E update_type, uint8_t obj_num, PENDPOINTLINK_OBJ_T pEndpointlinkObj )
//{
//    if ( NULL == pEndpointlinkObj )
//    {
//        return kGeneralErr;
//    }
//    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
//    volatile uint32_t ObjectAddrTemp = FLASH_USER_ENDPOINTLINK_OFFSET;
//    int ret = kNoErr;
//    uint8_t count = 0;
//    uint8_t updateCount = obj_num;
//    uint8_t updateIndex = 0;
//    ENDPOINTLINK_OBJ_EX_T endpointlinkObj;
//    PENDPOINTLINK_OBJ_EX_T pEndpointLinkObjTemp;
//    memset(&endpointlinkObj, 0, sizeof(ENDPOINTLINK_OBJ_T));
//    os_storage_log("obj_num: %d", obj_num);
//    for (updateIndex = 0; updateIndex < obj_num; updateIndex ++)
//    {
//        pEndpointLinkObjTemp = pEndpointlinkObj+updateIndex;
//        os_storage_log("devid: %s, linkNum:%d, inEndpoinId: %s, outEndpointId: %s", pEndpointLinkObjTemp->linkId, pEndpointLinkObjTemp->linkNum, pEndpointLinkObjTemp->linkObj[0].inEndpointId, pEndpointLinkObjTemp->linkObj[0].outEndpointId);
//
//        for (count=0; count < g_endpointLinkNum; count++)
//        {
//            ret = MicoFlashRead(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointlinkObj, sizeof(ENDPOINTLINK_OBJ_EX_T));
//
//            if (update_type == OBJECT_UPDATE_ADD)
//            {
//                if (0 == strcmp(pEndpointLinkObjTemp->linkId, endpointlinkObj.linkId))
//                {
//                    if (updateIndex < (obj_num-1))                      // Delete member had exist from the add list
//                    {
//                        uint8_t *dst = (uint8_t *)pEndpointlinkObj + updateIndex*sizeof(ENDPOINTLINK_OBJ_EX_T);
//                        uint8_t *src = (uint8_t *)pEndpointlinkObj + (updateIndex + 1)*sizeof(ENDPOINTLINK_OBJ_EX_T);
//                        memcpy(dst, src, (obj_num-updateIndex-1)*sizeof(ENDPOINTLINK_OBJ_EX_T));
//                    }
//                    updateCount --;
//                    break;
//                }
//            }
//            else if (update_type == OBJECT_UPDATE_MODIFY)
//            {
//                os_storage_log("%d: update \"%s\" to \"%s\" !", count, pEndpointLinkObjTemp->linkId, endpointlinkObj.linkId);
//                if (0 == strcmp(pEndpointLinkObjTemp->linkId, endpointlinkObj.linkId))
//                {
//                    ObjectAddrTemp = ObjectAddr - sizeof(ENDPOINTLINK_OBJ_EX_T);
//                    ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddrTemp, (uint8_t *)pEndpointLinkObjTemp, sizeof(ENDPOINTLINK_OBJ_EX_T));
//                    break;
//                }
//            }
//        }
//        ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
//    }
//
//    if (update_type == OBJECT_UPDATE_ADD)
//    {
//        if (updateCount == 0)
//        {
//            return ret;
//        }
//        ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET+g_endpointLinkNum*sizeof(ENDPOINTLINK_OBJ_EX_T);
//        ret = MicoFlashWriteEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pEndpointlinkObj, updateCount*sizeof(ENDPOINTLINK_OBJ_EX_T));
//        os_storage_log("endpointLink storage address: 0x%x", ObjectAddr);
//        if ( ret == kNoErr )
//        {
//            g_endpointLinkNum += updateCount;
//        }
//    }
//    return ret;
//}

/********************************************************
 * function: storage_read_endpoint_obj_batch
 * description:   read some info of devices from initial address
 * input:   1. num:  the number of object you want to read
 *          2. startIndex: tell us to get infomation from which index
 * output:  1. pEndpointObj: You must
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_endpointlink_obj_batch(unsigned char obj_num, unsigned char start_index, PENDPOINTLINK_OBJ_T pendpointlink_obj)
{
    if (NULL == pendpointlink_obj)
    {
        os_storage_log("read batch. pendpointlink_obj is NULL!!!");
        return kGeneralErr;
    }

    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET + start_index * sizeof(ENDPOINTLINK_OBJ_T);
    int ret = kNoErr;
    memset(pendpointlink_obj, 0, obj_num*sizeof(ENDPOINTLINK_OBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pendpointlink_obj, obj_num*sizeof(ENDPOINTLINK_OBJ_T));
    return ret;
}

///********************************************************
// * function: storage_read_endpointlink_obj
// * description:   read endpointlink info matching fid
// * input:   1. inputid:
// * output:  1. pEndpointObj:
// * return:
// * auther:   chenb
// * other:
//*********************************************************/
//OSStatus storage_read_endpointlink_obj(char *inputid, PENDPOINTLINK_OBJ_T pendpointlink_obj)
//{
//    if (NULL == pendpointlink_obj || (NULL == inputid))
//    {
//        return kGeneralErr;
//    }
//
//    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
//    int ret = kNoErr;
//    uint8_t index=0;
//    for (index=0; index < g_endpointLinkNum; index++)
//    {
//        memset(pendpointlink_obj, 0, sizeof(ENDPOINTLINK_OBJ_T));
//        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)pendpointlink_obj, sizeof(ENDPOINTLINK_OBJ_T));
//        if ((ret == kNoErr) && (!strcmp(inputid, pendpointlink_obj->linkObj.inEndpointId)))
//        {
//            ret = index+1;
//            return ret;
//        }
//    }
//
//    return ret;
//}

///********************************************************
// * function: storage_read_endpointlink_obj
// * description:   read endpointlink info matching fid
// * input:   1. inputid:
// * output:  1. pEndpointObj:
// * return:
// * auther:   chenb
// * other:
//*********************************************************/
//OSStatus storage_read_endpointlink_output(char *devid, char *input_endpoint, PLINK_OBJ_T link_obj)
//{
//    if (NULL == devid || (NULL == input_endpoint) ||(NULL == link_obj))
//    {
//        return kGeneralErr;
//    }
//
//    volatile uint32_t ObjectAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
//    OSStatus ret = kNoErr;
//    ENDPOINTLINK_OBJ_EX_T endpointLinkObj = {0};
//    uint8_t objIndex=0;
//    for (objIndex=0; objIndex < g_endpointLinkNum; objIndex++)
//    {
//        memset(&endpointLinkObj, 0, sizeof(ENDPOINTLINK_OBJ_EX_T));
//        ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&endpointLinkObj, sizeof(ENDPOINTLINK_OBJ_EX_T));
//        if ((ret == kNoErr) && (!strcmp(devid, endpointLinkObj.linkId)))
//        {
//            uint8_t linkIndex = 0;
//            for (linkIndex=0; linkIndex < endpointLinkObj.linkNum; linkIndex++)
//            {
//                if (!strcmp(input_endpoint, endpointLinkObj.linkObj[linkIndex].inEndpointId))
//                {
//                    memcpy(link_obj, &endpointLinkObj.linkObj[linkIndex], sizeof(LINK_OBJ_T));
//                    ret = objIndex+1;
//                    return ret;
//                }
//            }
//
//        }
//    }
//
//    return kGeneralErr;
//}

/********************************************************
 * function: storage_del_device_status_obj
 * description:
 * input:   1. inputid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
//OSStatus storage_read_dev_alarm_status(unsigned char *device_id, uint32_t *alarm_state, uint32_t *alarm_delay)
//{
//    if ((NULL == device_id) || (NULL == val))
//    {
//        return kGeneralErr;
//    }
//    else
//    {
//        uint8_t count = 0;
////        memset(pNetDevObj, 0, sizeof(NETDEVOBJ_T));
//        mico_Context_t* mainContext=  mico_system_context_get();
//        app_context_t app_context;
//        app_context.appConfig = mico_system_context_get_user_data( mainContext );
//        for (count=0; count < g_deviceNum; count++)
//        {
//            if (!strcmp((app_context.appConfig->devAlarm[count].devid), device_id))
//            {
//                *alarm_state = app_context.appConfig->devAlarm[count].alarmType;
//                *alarm_delay = app_context.appConfig->devAlarm[count].alarmDelay;
//                return kNoErr;
//            }
//        }
//    }
//    return kGeneralErr;
//}

/********************************************************
 * function: storage_del_device_status_obj
 * description:
 * input:
 * output:  1. dev_num:
 *          2. pdev_alarm_attr: a point point to NULL
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_read_dev_alarm_status( uint32_t *alarm_num, PALARM_ENDPOINT_T *pdev_alarm_attr )
{
    if (NULL == alarm_num)
    {
        return kGeneralErr;
    }
    else
    {
        mico_Context_t* mainContext =  mico_system_context_get();
        app_context_t app_context;
        app_context.appConfig = mico_system_context_get_user_data( mainContext );
        *pdev_alarm_attr = app_context.appConfig->alarmRecord.devAlarm;
        *alarm_num = app_context.appConfig->alarmRecord.alarmCount;

        if (*alarm_num > DEVICE_NUM)
        {
            *alarm_num = 0;
        }
//        if (*alarm_num != 0)
//        {
//            os_storage_log("read alarm num is %d, last device id: %s", *alarm_num, app_context.appConfig->alarmRecord.devAlarm[*alarm_num-1].devid);
//        }
        return kNoErr;
    }
}

/********************************************************
 * function: storage_write_dev_alarm_state
 * description:
 * input:   1. alarm_num:
 *          2. dev_alarm_attr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_write_dev_alarm_state( uint8_t alarm_num, PALARM_ENDPOINT_T dev_alarm_attr)
{
    if (alarm_num > DEVICE_NUM)
    {
        os_storage_log("write fail!!! alarm num is %d. alarm_attr head maybe null", alarm_num);
        return kGeneralErr;
    }
    else
    {
        mico_Context_t* mainContext=  mico_system_context_get();
        app_context_t app_context;
        app_context.appConfig = mico_system_context_get_user_data( mainContext );
        if ((alarm_num != 0)&&(dev_alarm_attr != 0))
        {
            if (app_context.appConfig->alarmRecord.devAlarm != dev_alarm_attr)
            {
                memcpy(app_context.appConfig->alarmRecord.devAlarm, dev_alarm_attr, alarm_num*sizeof(ALARM_ENDPOINT_T));

            }
            os_storage_log("write alarm device num is %d, last one device is is %s", alarm_num, app_context.appConfig->alarmRecord.devAlarm[alarm_num-1].devid);
        }
        else if (alarm_num == 0)
        {
            memset(app_context.appConfig->alarmRecord.devAlarm, 0, DEVICE_NUM*sizeof(ALARM_ENDPOINT_T));
        }
        app_context.appConfig->alarmRecord.alarmCount = alarm_num;
        return mico_system_context_update(mainContext);
    }
}

/********************************************************
 * function: storage_record_dev_alarm_state
 * description:
 * input:   1. alarm_num:
 *          2. dev_alarm_attr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_record_dev_alarm_state( uint8_t alarm_num, PALARM_ENDPOINT_T dev_alarm_attr)
{
    if ((NULL == dev_alarm_attr) || (0 == alarm_num) || (alarm_num > DEVICE_NUM))
    {
        os_storage_log("write fail!!! alarm num is %d. alarm_attr head maybe null", alarm_num);
        return kGeneralErr;
    }
    else
    {
        mico_Context_t* mainContext=  mico_system_context_get();
        app_context_t app_context;
        app_context.appConfig = mico_system_context_get_user_data( mainContext );
        if (app_context.appConfig->alarmRecord.devAlarm != dev_alarm_attr)
        {
            memcpy(app_context.appConfig->alarmRecord.devAlarm, dev_alarm_attr, alarm_num*sizeof(ALARM_ENDPOINT_T));
        }
        app_context.appConfig->alarmRecord.alarmCount = alarm_num;
        os_storage_log("record alarm device num is %d, last one device is is %s", alarm_num, app_context.appConfig->alarmRecord.devAlarm[alarm_num-1].devid);
        return kNoErr;
    }
}

#ifdef RF433_DEVICE
/********************************************************
 * function: storage_check_devobj
 * description:
 * input:   1. uuid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_check_devobj(PDEVICEOBJ_T pdevObj)
{
    int count = 0;

    int objectAddr = FLASH_USER_DEVICE_OFFSET;
    DEVICEOBJ_T deviceObj = {0};
    if (pdevObj == NULL)
    {
        return kGeneralErr;
    }
    for (count = 0; count<g_deviceNum; count++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, &deviceObj, sizeof(DEVICEOBJ_T));

        if (0 == strcmp(pdevObj->mac, deviceObj.mac))
        {
            memcpy(pdevObj, &deviceObj, sizeof(DEVICEOBJ_T));
            os_storage_log("%s is exist\r\n", pdevObj->addr);
            return kNoErr;
        }
    }
    return kGeneralErr;
}


#else
/********************************************************
 * function: storage_check_devobj
 * description:
 * input:   1. uuid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_check_devobj(PDEVICEOBJ_T pdevObj)
{
    int count = 0;

    volatile uint32_t objectAddr = FLASH_USER_DEVICE_OFFSET;
    DEVICEOBJ_T deviceObj = {0};
    uint32_t daddr[2] = {0};
    if (pdevObj == NULL)
    {
        return kGeneralErr;
    }
    for (count = 0; count<g_deviceNum; count++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&deviceObj, sizeof(DEVICEOBJ_T));
        mlink_parse_subdev_net_addr(deviceObj.comm, pdevObj->addr, (uint8_t *)&daddr[0]);
        mlink_parse_subdev_net_addr(deviceObj.comm, deviceObj.addr, (uint8_t *)&daddr[1]);
        if (daddr[0] == daddr[1])
        {
            if (0 == strcmp(pdevObj->modelid, deviceObj.modelid))       // 20180601
            {
                memcpy(pdevObj, &deviceObj, sizeof(DEVICEOBJ_T));
                return kNoErr;
            }
            else
            {
                os_storage_log("modelid is wrong");
            }
        }
    }
    return kGeneralErr;
}
#endif

/********************************************************
 * function: storage_get_devobj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_devobj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_DEVICE_OFFSET;
    DEVICEOBJ_T deviceObj;
    for (index = 0; index < DEVICE_NUM; index++)
    {
        MicoFlashReadEx(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&deviceObj, sizeof(DEVICEOBJ_T));

        if(1)// ((deviceObj.deviceId[0] >= ' ') && (deviceObj.deviceId[0] <= '~'))
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

/********************************************************
 * function: storage_get_roomObj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_roomObj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_ROOM_OFFSET;
    ROOMOBJ_T roomObj = {0};
    for (index=0, count=0; index < ROOM_NUM; index++)
    {
        MicoFlashReadEx(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&roomObj, sizeof(ROOMOBJ_T));
        if ((roomObj.roomId[0] >= ' ') && (roomObj.roomId[0] <= '~'))
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

/********************************************************
 * function: storage_get_sceneObj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_sceneObj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_SCENE_OFFSET;
    SCENEOBJ_T sceneObj;

    for (index=0, count=0; index < SCENE_NUM; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&sceneObj, sizeof(SCENEOBJ_T));
        if ((sceneObj.sceneId[0] >= ' ') && (sceneObj.sceneId[0] <= '~'))
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

/********************************************************
 * function: storage_get_eventObj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_eventObj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_EVENT_OFFSET;
    EVENTOBJ_T eventObj = {0};

    for (index=0, count=0; index < EVENT_NUM; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&eventObj, sizeof(EVENTOBJ_T));
        if ((eventObj.id[0] >= ' ') && (eventObj.id[0] <= '~'))
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

/********************************************************
 * function: storage_get_sceneObj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_scheduleObj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_SCHEDULE_OFFSET;
    SCHEDULEOBJ_T scheduleObj;

    for (index=0, count=0; index < SCHEDULE_NUM; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&scheduleObj, sizeof(SCHEDULEOBJ_T));
        if ((scheduleObj.name[0] != (char)0) && (scheduleObj.name[0] != (char)0xff))
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

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: storage_get_endpoint_obj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_endpoint_obj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_ENDPOINT_OFFSET;
    ENDPOINTOBJ_EX_T endpointObj;

    for (index=0, count=0; index < DEVICE_NUM; index++)
    {
        MicoFlashReadEx(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&endpointObj, sizeof(ENDPOINTOBJ_EX_T));
        if ((endpointObj.fid[0] >= ' ') && (endpointObj.fid[0] <= '~'))
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
#else
/********************************************************
 * function: storage_get_endpoint_obj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_endpoint_obj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_ENDPOINT_OFFSET;
    SINGLE_DEV_ENDPOINTOBJ_T endpointObj;

    for (index=0, count=0; index < DEVICE_NUM; index++)
    {
        MicoFlashReadEx(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&endpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
        if ((endpointObj.fid[0] != (char)0) && (endpointObj.fid[0] != (char)0xff))
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
#endif

/********************************************************
 * function: storage_get_endpointlink_obj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_endpointlink_obj_num( void )
{
    int index = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
    ENDPOINTLINK_OBJ_T endpointLinkObj;

    for (index=0, count=0; index < ENDPOINTLINKOBJ_NUM; index++)
    {
        memset(&endpointLinkObj, 0, sizeof(ENDPOINTLINK_OBJ_T));
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&endpointLinkObj, sizeof(ENDPOINTLINK_OBJ_T));
        if ((endpointLinkObj.linkId[0] >= ' ') && (endpointLinkObj.linkId[0] <= '~'))
        {
//            os_storage_log("ctrlContent:%s", endpointLinkObj.linkObj.ctrlContent);
            count ++;
        }
        else
        {
            break;
        }
    }
    return count;
}

/********************************************************
 * function: storage_get_lnkinput_obj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_lnkinput_obj_num( void )
{
    int lnkIndex = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_LINKAGE_INPUT_OFFSET;
    LINKAGE_INPUT_TASK_T linkageTask;

    for (lnkIndex=0, count=0; lnkIndex < LINKAGE_INPUT_TASK_NUM; lnkIndex++)
    {
        memset(&linkageTask, 0, sizeof(LINKAGE_INPUT_TASK_T));
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&linkageTask, sizeof(LINKAGE_INPUT_TASK_T));
        if ((linkageTask.linkageId[0] >= ' ') && (linkageTask.linkageId[0] <= '~'))
        {
            if (linkageTask.inputNum <= LINKAGE_INPUT_NUM)
            {
                count ++;
            }
            else
            {
                break;
            }
//            os_storage_log("lnk num is %d", linkageTask.inputNum);
        }
        else
        {
            break;
        }
    }
    return count;
}


/********************************************************
 * function: storage_get_lnkoutput_obj_num
 * description:
 * input:   1. :
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_lnkoutput_obj_num( void )
{
    int lnkIndex = 0;
    int count = 0;
    volatile uint32_t objectAddr = FLASH_USER_LINKAGE_OUTPUT_OFFSET;
    LINKAGE_OUTPUT_TASK_T linkageTask;

    for (lnkIndex=0, count=0; lnkIndex < LINKAGE_OUTPUT_TASK_NUM; lnkIndex++)
    {
        memset(&linkageTask, 0, sizeof(LINKAGE_OUTPUT_TASK_T));
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&linkageTask, sizeof(LINKAGE_OUTPUT_TASK_T));
        if ((linkageTask.linkageId[0] >= ' ') && (linkageTask.linkageId[0] <= '~'))
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


/********************************************************
 * function: storage_get_device_num
 * description:  server callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus storage_get_device_num(void)
{
    return g_deviceNum;
}

/********************************************************
 * function: storage_init_object_num
 * description:  server callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus storage_init_object_num(void)
{
    g_deviceNum = storage_get_devobj_num();
    g_roomNum = storage_get_roomObj_num();
    g_sceneNum = storage_get_sceneObj_num();
    g_scheduleNum = storage_get_scheduleObj_num();
    g_eventNum = storage_get_eventObj_num();
    g_endpointNum = storage_get_endpoint_obj_num();
    g_endpointLinkNum = storage_get_endpointlink_obj_num();
    g_linkageInputNum = storage_get_lnkinput_obj_num();
    g_linkageOutputNum = storage_get_lnkoutput_obj_num();
    os_storage_log("g_deviceNum = %d,g_roomNum= %d,g_sceneNum= %d, g_eventNum=%d, g_endpointLinkNum=%d, g_endpointNum=%d, g_linkageInputNum=%d, g_linkageOutputNum=%d\r\n",
                       g_deviceNum, g_roomNum, g_sceneNum, g_eventNum, g_endpointLinkNum, g_endpointNum, g_linkageInputNum, g_linkageOutputNum);
    return FLASH_OPERATE_OK;
}

/********************************************************
 * function: st_coap_client_notify
 * description:  server callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus storage_factory_flash(void)
{
    int ret = 0;

    // clear user space
    ret = MicoFlashErase(MICO_PARTITION_USER, FLASH_USER_START, FLASH_USER_TOTAL_SIZE);
    return ret;
}

/********************************************************
 * function: storage_init_event_obj
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_init_event_obj(void)
{
    uint32_t ObjectAddr = FLASH_USER_EVENT_OFFSET;
    int ret = kNoErr;
    EVENTOBJ_T eventInfoObj;
    memset(&eventInfoObj, 0, sizeof(EVENTOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_USER, &ObjectAddr, (uint8_t *)&eventInfoObj, sizeof(EVENTOBJ_T));

    if ((eventInfoObj.id[0] < ' ') || (eventInfoObj.id[0] > '~'))
    {
        EVENTOBJ_T eventObjArray[4];
        memset(eventObjArray, 0, 4*sizeof(EVENTOBJ_T));
        os_storage_log("device information is NULL!!! Start init event obj");
        sprintf(eventObjArray[0].id, "%d", 1);
        memcpy(eventObjArray[0].name, "状态变更通知", strlen("状态变更通知"));
        eventObjArray[0].eventlevel = 1;
        eventObjArray[0].eventtype = 10;
        eventObjArray[0].notitype = 1;
        eventObjArray[0].enabled = 1;
#ifndef BLE_DEVICE
        strcpy(eventObjArray[0].key, "[10,20,21,22,101,102,222,225,226,252,103,301,302,303,304,305,306,401,402,451,501,502,601,602,604,811]");
#else
        strcpy(eventObjArray[0].key, "[10,20,21,22,1010,1000,1001,1004]");
#endif

        sprintf(eventObjArray[1].id, "%d", 2);
        memcpy(eventObjArray[1].name, "报警事件", strlen("报警事件"));
        eventObjArray[1].eventlevel = 0;
        eventObjArray[1].eventtype = 3;
        eventObjArray[1].notitype = 0;
        eventObjArray[1].enabled = 1;
#ifndef BLE_DEVICE
        memcpy(eventObjArray[1].key, "[10,20,22]", 10);
#else
        strcpy(eventObjArray[1].key, "[10,20,22,1013]");
#endif

        sprintf(eventObjArray[2].id, "%d", 3);
        memcpy(eventObjArray[2].name, "故障报警", strlen("故障报警"));
        eventObjArray[2].eventlevel = 0;
        eventObjArray[2].eventtype = 4;
        eventObjArray[2].notitype = 0;
        eventObjArray[2].enabled = 1;
        memcpy(eventObjArray[2].key, "[23]", 4);

        sprintf(eventObjArray[3].id, "%d", 4);
        memcpy(eventObjArray[3].name, "电池欠压", strlen("电池欠压"));
        eventObjArray[3].eventlevel = 1;
        eventObjArray[3].eventtype = 15;
        eventObjArray[3].notitype = 0;
        eventObjArray[3].enabled = 1;
        memcpy(eventObjArray[3].key, "[21]", 4);

        storage_write_event_obj(OBJECT_UPDATE_ADD, 4, eventObjArray);
        return kGeneralErr;
    }

    return ret;
}

/********************************************************
 * function: storage_check_event
 * description:
 * input:       key
 * output:      pevent_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int storage_check_event(NOTIFY_TYPE_E notify_type, const char *key, PEVENTOBJ_T pevent_obj)
{
    if (pevent_obj == NULL || (strlen(key) > 32))
    {
        os_storage_log("pevent_obj is %x,  key len is %d !", pevent_obj, strlen(key));
        return FALSE;
    }
    for (uint8_t count = 0; count<g_eventNum; count++)
    {
        char keycode[32] = {0};
        uint8_t keylen = 0;
        uint8_t start = 1;
        memset(pevent_obj, 0, sizeof(EVENTOBJ_T));
        storage_read_event_obj_batch(1, count, pevent_obj);
        if (pevent_obj->notitype == notify_type)
        {
    //        os_storage_log("event name is %s, key is %s!!!", pevent_obj->name, pevent_obj->key);
            for (uint8_t pos= 1; pos<strlen(pevent_obj->key); pos++)
            {
                keylen++;
                if ((pevent_obj->key[pos] == ',') || (pevent_obj->key[pos] == ']'))
                {
                    if ((keylen-1) == strlen(key))
                    {
                        memcpy(keycode, &pevent_obj->key[start], keylen-1);
                        if (!strcmp(keycode, key))
                        {
                            return TRUE;
                        }
                    }
                    start = pos+1;
                    keylen = 0;
                }
                else if (pevent_obj->key[pos] == ' ')
                {
                    if (keylen == 0)
                    {
                        start++;
                    }
                    continue;
                }
            }
        }
    }
    return FALSE;
}

#if 0
/********************************************************
 * function: storage_check_local_obj
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_check_local_obj(void)
{
    int ret = kNoErr;
    NETDEVOBJ_T NetDevObj;
    memset(&NetDevObj, 0, sizeof(NETDEVOBJ_T));
#if 0
    uint32_t ObjectAddr = FLASH_PARAM_NETDEV_OFFSET;

    ret = MicoFlashReadEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)&NetDevObj, sizeof(NETDEVOBJ_T));
#else
    app_context_t app_context;
    app_context.appConfig = mico_system_context_get_user_data( mico_system_context_get() );
    memcpy(&NetDevObj,&app_context.appConfig->netdev,sizeof(NETDEVOBJ_T));
#endif

    if (( NetDevObj.mac[0] == (char)0) || (NetDevObj.mac[0] == (char)0xff))
    {
        os_storage_log("local obj is null!!!");
        return kGeneralErr;
    }

    return ret;
}

/********************************************************
 * function: storage_init_local_dev_info
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_local_dev_info(void)
{
    OSStatus ret = 0;
    ret = storage_check_local_obj();
    if (kNoErr != ret)
    {
//        uint8_t md5Hash[16] = {0};
//        uint8_t verify[VERIFY_SIZE+1] = {0};
        uint32_t srandNo = 0;
        NETDEVOBJ_T netDevObj;
        uint8_t randVal = 0;
        memset(&netDevObj, 0, sizeof(NETDEVOBJ_T));
//        memset(md5Hash, 0, sizeof(md5Hash));
//        memset(verify, 0, sizeof(verify));
        netDevObj.ch = 0;
        mico_system_status_wlan_t *wlan_status;
        mico_system_get_status_wlan( &wlan_status );
        sprintf(netDevObj.mac, "%c%c%c%c%c%c%c%c%c%c%c%c", wlan_status->mac[0],wlan_status->mac[1],wlan_status->mac[3], \
                wlan_status->mac[4],wlan_status->mac[6],wlan_status->mac[7],wlan_status->mac[9],wlan_status->mac[10],wlan_status->mac[12],wlan_status->mac[13],\
                wlan_status->mac[15],wlan_status->mac[16]);
        netDevObj.devtype = GATEWAY_TYPE;
        netDevObj.status.status_info.devStatus = 0xff;          // Default unregister (1B)(online / offline / unregister)
        StrToHex(&netDevObj.mac[4], 8, &srandNo);
        srand(srandNo);
        randVal = rand()%255+1;
        sprintf(netDevObj.addr, "%02X", randVal);

        memcpy(netDevObj.uuid, netDevObj.mac, MAC_SIZE);
        memcpy(netDevObj.modelid, MODEL_ID, strlen(MODEL_ID));
        memcpy(netDevObj.name, DEV_MODEL_NAME, strlen(DEV_MODEL_NAME));
        memcpy(netDevObj.modelname, YL_MODELNAME, strlen(YL_MODELNAME));

//        mlink_string_convert_md5(netDevObj.mac, 12, md5Hash, &hashLen);
//        memcpy(verify, &md5Hash[5], VERIFY_SIZE);
//        sprintf(netDevObj.uuid, "%s#%s", netDevObj.mac, verify);

        strcpy(netDevObj.appid,APPID_DEFAULT);
        strcpy(netDevObj.ask,APP_SECRET);
        strcpy(netDevObj.cloudhost,CLOUD_HOST);
        strcpy(netDevObj.dsk, DEVICE_SECRET_DEFAULT);
        strcpy(netDevObj.psk, PSK_DEFAULT);
        storage_write_local_obj(&netDevObj);
        //os_main_logic_log("%s \n", netDevObj.mac);
//        os_storage_log("netdebobj: ask:%s psk:%s dsk %s id[%s] appid[%s]\n",netDevObj.ask,netDevObj.psk,netDevObj.dsk,netDevObj.deviceId, netDevObj.appid);

    }
}


/********************************************************
 * function: storage_check_devinfo
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_check_devinfo(void)
{
    int ret = kNoErr;
    DEVINFOOBJ_T DevInfoObj = {0};
#if 0
    uint32_t ObjectAddr = FLASH_PARAM_DEVINFO_OFFSET;
    memset(&DevInfoObj, 0, sizeof(DEVINFOOBJ_T));
    ret = MicoFlashReadEx(MICO_PARTITION_PARAMETER_1, &ObjectAddr, (uint8_t *)&DevInfoObj, sizeof(DEVINFOOBJ_T));
#else
    os_storage_log("check :software %s", DevInfoObj.firmver);
    if ((DevInfoObj.ver[0] == (char)0) || (DevInfoObj.ver[0] == (char)0xff))
    {
        os_storage_log("device information is NULL!!!");
        return kGeneralErr;
    }
#endif
    return ret;
}


/********************************************************
 * function: storage_init_dev_info
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_local_ver_info(void)
{
    OSStatus ret = 0;
    ret = storage_check_devinfo();
    if (ret != kNoErr)
    {
        DEVINFOOBJ_T devinfo;
        memset(&devinfo, 0, sizeof(DEVINFOOBJ_T));
        sprintf(devinfo.ver, "%s", SOFT_VER);
        storage_write_devinfo_obj(&devinfo);
    }

}

#endif

/********************************************************
 * function: storage_init_device_status
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
PDEV_ENDPOINT_STATUS_T storage_get_dev_endpoints_status( uint8_t dev_index )
{
    return &g_deviceAttrStatus[dev_index];
}

/********************************************************
 * function: storage_get_endpoints_status
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
PDEV_ENDPOINT_STATUS_T storage_get_endpoints_status( char *deviceId )
{
    uint8_t devIndex = 0;

    for (devIndex = 0; devIndex < g_endpointNum; devIndex++)
    {
        if (0 == strcmp(g_deviceAttrStatus[devIndex].fid, deviceId))
        {
            return &g_deviceAttrStatus[devIndex];
        }
    }
    return NULL;
}

/********************************************************
 * function: storage_init_device_status
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
PKEY_ATTR_T storage_get_endpoint_status( char *deviceId, char *endpointId )
{
    PDEV_ENDPOINT_STATUS_T pDevEndpointsState = NULL;
    pDevEndpointsState = storage_get_endpoints_status(deviceId);
    if (pDevEndpointsState != NULL)
    {
        uint8_t endpointIndex = 0;
        PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;
        for (endpointIndex=0; endpointIndex < pDevEndpointsState->endpointNum; endpointIndex++)
        {
            pEndpointStatusTemp = pDevEndpointsState->endpointStatus+endpointIndex;
            if (0 == strcmp(pEndpointStatusTemp->endpointId, endpointId))
            {
                return &pEndpointStatusTemp->keyAttr;
            }
//            else
//            {
//                os_storage_log("endpointid:　%s != %s", pDevEndpointsState->endpointStatus[endpointIndex].endpointId, endpointId);
//            }
        }
    }
    return NULL;
}

/********************************************************
 * function: storage_update_endpoint_status
 * description:     update endpoint status
 * input:       1. devid
 *              2. endpointId
 *              3. pdevAttr
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:       update old status or add new endpoint status
*********************************************************/
void storage_update_endpoint_status( char *devid, PKEY_ATTR_T pdevAttr)
{
    uint8_t devIndex = 0;
    uint8_t endpointIndex = 0;
    PDEV_ENDPOINT_STATUS_T pdevEndpointTemp = NULL;
    char endpointId[ENDPOINTID_SIZE] = {0};
    PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;

    if ((devid == NULL) || (pdevAttr == NULL))
    {
        return;
    }
    mlink_generate_endpoint_id(DEVICE_OBJ_ID, devid, pdevAttr->keyState.key, endpointId);

    os_storage_log("Update endpoint status !!!\r\val:%s, endpointid:%s, key: %s", pdevAttr->keyState.value, endpointId, pdevAttr->keyState.key);
    for (devIndex = 0; devIndex < g_endpointNum; devIndex++)
    {
        pdevEndpointTemp = &g_deviceAttrStatus[devIndex];
        if (0 == strcmp(pdevEndpointTemp->fid, devid))     // 匹配设备ID号
        {
            if (pdevEndpointTemp->endpointNum == 0)
            {
                return;
            }
            else if (pdevEndpointTemp->endpointStatus == NULL)
            {
                pdevEndpointTemp->endpointNum = ENDPOINTLIST_NUM;
                pdevEndpointTemp->endpointStatus =  malloc((ENDPOINTLIST_NUM+2)*sizeof(ENDPOINT_STATUS_T));
                memset(pdevEndpointTemp->endpointStatus, 0, (ENDPOINTLIST_NUM+2)*sizeof(ENDPOINT_STATUS_T));
                strcpy(pdevEndpointTemp->endpointStatus->endpointId, endpointId);
                if (pdevAttr->keyType[0] == 0)
                {
                    memcpy(&pdevEndpointTemp->endpointStatus->keyAttr.keyState, &pdevAttr->keyState, sizeof(DEV_FUNCTION_OBJ_T));
                }
                else
                {
                    memcpy(&pdevEndpointTemp->endpointStatus->keyAttr, pdevAttr, sizeof(KEY_ATTR_T));
                }
            }
            else
            {
//                os_storage_log("find out update dev !");
                for (endpointIndex=0; endpointIndex < pdevEndpointTemp->endpointNum; endpointIndex++)
                {
                    pEndpointStatusTemp = pdevEndpointTemp->endpointStatus + endpointIndex;
//                    os_storage_log("endpointIdTemp: %x, endpointId: %s", pEndpointStatusTemp->endpointId, endpointId);
                    if (0 == strcmp(pEndpointStatusTemp->endpointId, endpointId))    // update endpoint status
                    {
                        if (pdevAttr->keyType[0] == 0)
                        {
                            memcpy(&pEndpointStatusTemp->keyAttr.keyState, &pdevAttr->keyState, sizeof(DEV_FUNCTION_OBJ_T));
                        }
                        else
                        {
                            memcpy(&pEndpointStatusTemp->keyAttr, pdevAttr, sizeof(KEY_ATTR_T));
                        }
//                        os_storage_log("update endpointId: %s, value: %s, oldval: %s", endpointId, pEndpointStatusTemp->keyAttr.keyState.value, pEndpointStatusTemp->oldvalue);
                        return;
                    }
                    else if (pEndpointStatusTemp->endpointId[0] == 0)
                    {
                        strcpy(pEndpointStatusTemp->endpointId, endpointId);
                        memset(pEndpointStatusTemp->oldvalue, 0, sizeof(pEndpointStatusTemp->oldvalue));
                        if (pdevAttr->keyType[0] == 0)
                        {
                            memcpy(&pEndpointStatusTemp->keyAttr.keyState, &pdevAttr->keyState, sizeof(DEV_FUNCTION_OBJ_T));
                        }
                        else
                        {
                            memcpy(&pEndpointStatusTemp->keyAttr, pdevAttr, sizeof(KEY_ATTR_T));
                        }
//                        os_storage_log("update success endpointId: %s, value: %s, oldval: %s", endpointId, pdevAttr->keyState.value, pEndpointStatusTemp->oldvalue);
                        return;
                    }
                }
            }
        }
        else if ( pdevEndpointTemp->fid[0] == 0 )
        {
//            memset(pdevEndpointTemp, 0, sizeof(PDEV_ENDPOINT_STATUS_T));
            strcpy(pdevEndpointTemp->fid, devid);
            pdevEndpointTemp->endpointNum = ENDPOINTLIST_NUM;
            pdevEndpointTemp->endpointStatus = malloc((ENDPOINTLIST_NUM+2)*sizeof(ENDPOINT_STATUS_T));
            if (pdevEndpointTemp->endpointStatus != NULL)
            {
                memset(pdevEndpointTemp->endpointStatus, 0, (ENDPOINTLIST_NUM+2)*sizeof(ENDPOINT_STATUS_T));
                memcpy(pdevEndpointTemp->endpointStatus->endpointId, endpointId, strlen(endpointId));
                if (pdevAttr->keyType[0] == 0)
                {
                    memcpy(&pdevEndpointTemp->endpointStatus->keyAttr.keyState, &pdevAttr->keyState, sizeof(DEV_FUNCTION_OBJ_T));
                }
                else
                {
                    memcpy(&pdevEndpointTemp->endpointStatus->keyAttr, pdevAttr, sizeof(KEY_ATTR_T));
                }
            }
            break;
        }
    }
//    return NULL;
}

/********************************************************
 * function: storage_del_dev_endpoints_status
 * description:
 * input:   1. index:
 *          2. pendpoint_obj:   point to new endpoint object
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus storage_del_dev_endpoints_status( char *devid )
{
    uint8_t count = 0;
    PDEV_ENDPOINT_STATUS_T pendpointStatusTemp = NULL;
    for (count=0; count<g_endpointNum; count++)
    {
        pendpointStatusTemp = &g_deviceAttrStatus[count];
        if (pendpointStatusTemp->fid[0] == 0)
        {
            break;
        }
        if (!strcmp(pendpointStatusTemp->fid, devid))
        {
            free(pendpointStatusTemp->endpointStatus);
            pendpointStatusTemp->endpointStatus = NULL;
            if (count == g_endpointNum-1)
            {
                memset(pendpointStatusTemp, 0, sizeof(DEV_ENDPOINT_STATUS_T));
            }
            else
            {
                memcpy(pendpointStatusTemp, &g_deviceAttrStatus[count+1], (g_endpointNum - count - 1)*sizeof(DEV_ENDPOINT_STATUS_T));
                memset(&g_deviceAttrStatus[g_endpointNum - 1], 0, sizeof(DEV_ENDPOINT_STATUS_T));
            }
            break;
        }
    }
    return kNoErr;
}

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: storage_add_endpoint_status
 * description:
 * input:   1. index:
 *          2. pendpoint_obj:   point to new endpoint object
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_add_endpoint_status(uint8_t index, PENDPOINTOBJ_EX_T pendpoint_obj)
{
    uint8_t count = 0;
    ENDPOINT_ELE_T endpointEle = {0};
    uint8_t endpointNum = 0;
    PDEV_ENDPOINT_STATUS_T pDevEndpointStatusTemp = NULL;
    if ((pendpoint_obj == NULL) || (pendpoint_obj->endpointNum > ENDPOINTLIST_NUM))
    {
        return;
    }
    else
    {
        endpointNum = pendpoint_obj->endpointNum;
    }
//    os_storage_log("add endpoint status: %d, endpoint num is %d", index, pendpoint_obj->endpointNum);

    pDevEndpointStatusTemp = &g_deviceAttrStatus[index];
    if (pDevEndpointStatusTemp->endpointStatus)
    {
        PENDPOINT_STATUS_T pEndpointStatus = pDevEndpointStatusTemp->endpointStatus;
        memset(pEndpointStatus, 0, pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
        memset(pDevEndpointStatusTemp, 0, sizeof(DEV_ENDPOINT_STATUS_T));
        free(pEndpointStatus);
    }
    else
    {
        memset(pDevEndpointStatusTemp, 0, sizeof(DEV_ENDPOINT_STATUS_T));
    }

    memcpy(pDevEndpointStatusTemp->fid, pendpoint_obj->fid, sizeof(pDevEndpointStatusTemp->fid));
    if ((pendpoint_obj->endpointlist[0].keytype == 30) || (pendpoint_obj->endpointlist[0].keytype == 31))
    {
        pDevEndpointStatusTemp->endpointNum = 5;
        if (pDevEndpointStatusTemp->endpointStatus == NULL)
        {
            pDevEndpointStatusTemp->endpointStatus = malloc(pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
        }
//        else
//        {
//            free(pDevEndpointStatusTemp->endpointStatus);
//            pDevEndpointStatusTemp->endpointStatus = malloc(pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
//        }
        memset(pDevEndpointStatusTemp->endpointStatus, 0, pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
    }
    else
    {
        pDevEndpointStatusTemp->endpointNum = ((endpointNum+2)<ENDPOINTLIST_NUM)?(endpointNum+2):ENDPOINTLIST_NUM;
        if (pDevEndpointStatusTemp->endpointNum != 0)
        {
            PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;
            if (pDevEndpointStatusTemp->endpointStatus == NULL)
            {
                pDevEndpointStatusTemp->endpointStatus = malloc(pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
            }
            memset(pDevEndpointStatusTemp->endpointStatus, 0, pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
            for (count= 0; count < endpointNum; count++)
            {
                pEndpointStatusTemp = pDevEndpointStatusTemp->endpointStatus + count;

                memcpy(pEndpointStatusTemp->endpointId, pendpoint_obj->endpointlist[count].id, ENDPOINTID_SIZE);
                mlink_parse_endpointid(pEndpointStatusTemp->endpointId, &endpointEle);
                strcpy(pEndpointStatusTemp->keyAttr.keyState.key, endpointEle.key);
                pEndpointStatusTemp->keyAttr.keyState.value[0] = '0';
                pEndpointStatusTemp->keyAttr.keyState.type = 3;
                pEndpointStatusTemp->keyAttr.keyState.valtype = 4;
                sprintf(pEndpointStatusTemp->keyAttr.keyType, "%d", pendpoint_obj->endpointlist[count].keytype);
            }
        }
    }
}

/********************************************************
 * function: storage_init_endpoint_status
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_endpoint_status( void )
{
    volatile uint32_t objectAddr = FLASH_USER_ENDPOINT_OFFSET;
    ENDPOINTOBJ_EX_T endpointObj = { 0 };
    uint8_t index = 0;

    // value device's endpoint status .
    for (index = 0; index < g_endpointNum; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&endpointObj, sizeof(ENDPOINTOBJ_EX_T));
        storage_add_endpoint_status(index, &endpointObj);
    }
}
#else

/********************************************************
 * function: storage_add_endpoint_status
 * description:
 * input:   1. index:
 *          2. pendpoint_obj:   point to new endpoint object
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_add_endpoint_status(uint8_t index, PSINGLE_DEV_ENDPOINTOBJ_T pendpoint_obj)
{
    uint8_t count = 0;
    ENDPOINT_ELE_T endpointEle = {0};
    uint8_t endpointNum = 0;
    PDEV_ENDPOINT_STATUS_T pDevEndpointStatusTemp = NULL;
    if ((pendpoint_obj == NULL) || (pendpoint_obj->endpointNum > ENDPOINTLIST_NUM))
    {
        return;
    }
    else
    {
        endpointNum = pendpoint_obj->endpointNum;
    }
//    os_storage_log("add endpoint status: %d", index);

    pDevEndpointStatusTemp = &g_deviceAttrStatus[index];
    memset(pDevEndpointStatusTemp, 0, sizeof(DEV_ENDPOINT_STATUS_T));

    memcpy(pDevEndpointStatusTemp->fid, pendpoint_obj->fid, sizeof(pDevEndpointStatusTemp->fid));
    pDevEndpointStatusTemp->endpointNum = ((endpointNum+2)<ENDPOINTLIST_NUM)?(endpointNum+2):ENDPOINTLIST_NUM;
    if (pDevEndpointStatusTemp->endpointNum != 0)
    {
        PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;
        if (pDevEndpointStatusTemp->endpointStatus == NULL)
        {
            pDevEndpointStatusTemp->endpointStatus = malloc(pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
        }
        memset(pDevEndpointStatusTemp->endpointStatus, 0, pDevEndpointStatusTemp->endpointNum*sizeof(ENDPOINT_STATUS_T));
        for (count= 0; count < endpointNum; count++)
        {
            pEndpointStatusTemp = pDevEndpointStatusTemp->endpointStatus + count;

            memcpy(pEndpointStatusTemp->endpointId, pendpoint_obj->endpointlist[count].id, ENDPOINTID_SIZE);
//            os_storage_log("endpoint id is %x== %s", pEndpointStatusTemp->endpointId, pEndpointStatusTemp->endpointId);
            mlink_parse_endpointid(pEndpointStatusTemp->endpointId, &endpointEle);
            strcpy(pEndpointStatusTemp->keyAttr.key, endpointEle.key);
            pEndpointStatusTemp->keyAttr.value[0] = '0';
            pEndpointStatusTemp->keyAttr.type = 3;
            pEndpointStatusTemp->keyAttr.valtype = 0;
        }
    }

}

/********************************************************
 * function: storage_init_endpoint_status
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_endpoint_status( void )
{
    volatile uint32_t objectAddr = FLASH_USER_ENDPOINT_OFFSET;
    SINGLE_DEV_ENDPOINTOBJ_T endpointObj = { 0 };
    uint8_t index = 0;

    // value device's endpoint status .
    for (index = 0; index < g_endpointNum; index++)
    {
        MicoFlashRead(MICO_PARTITION_USER, &objectAddr, (uint8_t *)&endpointObj, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
        storage_add_endpoint_status(index, &endpointObj);
    }
}
#endif


/********************************************************
 * function: storage_init_local_endpoint
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_local_endpoint( void )
{
    LOCAL_ENDPOINT_T localEndpoint = {0};
    NETDEVOBJ_T netDevObj = {0};
    storage_read_local_endpoint(&localEndpoint);

    if ((localEndpoint.alarmEndpoint.endpointNum == 0) || (localEndpoint.alarmEndpoint.endpointNum == (char)0xff))
    {
        memset(&localEndpoint, 0, sizeof(LOCAL_ENDPOINT_T));
        storage_read_local_devobj(&netDevObj);
        strcpy(localEndpoint.alarmEndpoint.devid, netDevObj.uuid);
        localEndpoint.alarmEndpoint.endpointNum = 1;
        localEndpoint.alarmEndpoint.alarmType = 0;
        storage_write_local_endpoint(&localEndpoint);
    }
}
