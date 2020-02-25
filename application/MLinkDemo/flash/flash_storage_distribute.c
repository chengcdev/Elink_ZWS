/*
 * flash_storage_distribute.c
 *
 *  Created on: 2018年1月8日
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
#include "../MLinkPublic/MLinkPublic.h"
#define os_storage_log(M, ...) custom_log("STORAGE_DEAL", M, ##__VA_ARGS__)

#define PAGE_SIZE        4

extern uint32_t g_deviceNum;
extern uint32_t g_roomNum;
extern uint32_t g_sceneNum;
extern uint32_t g_scheduleNum;
extern uint32_t g_eventNum;
extern uint32_t g_endpointNum;
extern uint32_t g_endpointLinkNum;
extern uint32_t g_linkageInputNum;
extern uint32_t g_linkageOutputNum;

static DEVSTATUSOBJ_T g_devStatusObj[DEVICE_NUM+1] = {0};
static char *g_objectData = NULL;


OSStatus storage_update_dev_alarm_state( uint8_t alarm_num, PALARM_ENDPOINT_T pdevice_alarm_state );
int storage_get_dev_status( char *devid, PDEVSTATUSOBJ_T *pdev_state_obj );

/********************************************************
 * function: storage_get_obj_size
 * description:  获取对象结构体大小
 * input:   1. classid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int storage_get_obj_size( uint32_t classid )
{
    int size = -1;
    switch( classid )
    {
        case DEVICE_OBJ_ID:
            size = sizeof(DEVICEOBJ_T);
            break;
        case SCENE_OBJ_ID:
            size = sizeof(SCENEOBJ_T);
            break;
        case ROOM_OBJ_ID:
            size = sizeof(ROOMOBJ_T);
            break;
        case GROUP_OBJ_ID:
            break;
        case CLOUDPARAM_OBJ_ID:
            size = sizeof(CLOUDOBJ_T);
            break;
        case SCHEDULE_OBJ_ID:
            size = sizeof(SCHEDULEOBJ_T);
            break;
        case NETDEV_OBJ_ID:
            size = sizeof(NETDEVOBJ_T);
            break;
        case ENDPOINT_OBJ_ID:
            size = sizeof(ENDPOINTOBJ_EX_T);
            break;
        case ENDPOINTLINK_OBJ_ID:
            size = sizeof(ENDPOINTLINK_OBJ_T);
            break;
        case EVENT_OBJ_ID:
            size = sizeof(EVENTOBJ_T);
            break;
        case LINKAGE_INPUT_OBJ_ID:
            size = sizeof(LINKAGE_INPUT_TASK_T);
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            size = sizeof(LINKAGE_OUTPUT_TASK_T);
            break;
        default:
            size = -1;
            break;
    }
    return size;
}

/********************************************************
 * function: storage_get_obj_batch
 * description:  批量获取指定对象类型的数据
 * input:   1. classid:
 *          2. page_size:
 *          3. current_page:

 * output:  1. current_page_size
 *          2. obj_data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int storage_get_obj_batch( uint32_t classid, uint8_t obj_num, uint8_t start_index, void *obj_data )
{
    int ret = kNoErr;
//    os_storage_log("classid : %d, start_index: %d, obj_num: %d", classid, start_index, obj_num);
    switch( classid )
    {
        case LINKAGE_INPUT_OBJ_ID:
            ret = storage_read_linkage_input_obj(obj_num, start_index, (PLINKAGE_INPUT_TASK_T)obj_data);
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            ret = storage_read_linkage_output_obj(obj_num, start_index, (PLINKAGE_OUTPUT_TASK_T)obj_data);
            break;
        default:
            ret = kGeneralErr;
            break;
    }
    return ret;
}

/********************************************************
 * function: storage_page_get_obj
 * description:  获取某个对象类型的某一页的记录
 * input:   1. classid:
 *          2. page_size:
 *          3. current_page:

 * output:  1. current_page_size
 *          2. obj_data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_page_get_obj( uint32_t classid, uint8_t page_size, uint8_t current_page, uint8_t *current_page_size, void *obj_data)
{
    int ret = kNoErr;
    if ((current_page_size==NULL) || (obj_data==NULL))
    {
        return kGeneralErr;
    }
    else
    {
        uint8_t objNum = storage_get_object_num(classid);
        *current_page_size = mlink_get_curpage_size(objNum, page_size, current_page);
        ret = storage_get_obj_batch(classid, *current_page_size, (current_page-1)*page_size, obj_data);
    }
    return ret;
}

/********************************************************
 * function: storage_get_obj_by_id
 * description:  获取某个对象的某一页的记录
 * input:   1. classid:
 *          2. page_size:
 *          3. current_page:

 * output:  1. current_page_size
 *          2. obj_data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_obj_by_id( uint32_t classid, char *obj_id, void *obj_data)
{
    uint32_t objNum = storage_get_object_num(classid);
    uint8_t pagesNumber = 0;//mlink_get_total_pages(objNum, PAGE_SIZE);
    uint8_t pageIndex = 0;
    uint8_t pageSize = 0;
    uint8_t currentPageSize = 0;
    uint32_t objIndex = 0;
    uint32_t objectStructSize = storage_get_obj_size(classid);
    char *dataTemp = NULL;
    void *objData = NULL;
    if ((obj_id == NULL) || (obj_data == NULL) || (objectStructSize == -1))
    {
        return kGeneralErr;
    }
    if (g_objectData == NULL)
    {
        g_objectData = malloc(4096);
        memset(g_objectData, 0, 4096);
    }
    pageSize = 4096/objectStructSize;
    pagesNumber = mlink_get_total_pages(objNum, pageSize);
    objData = g_objectData;
    for (pageIndex=1; pageIndex <= pagesNumber; pageIndex++)
    {
        storage_page_get_obj(classid, pageSize, pageIndex, &currentPageSize, objData);
        for (objIndex = 0; objIndex < currentPageSize; objIndex++)
        {
            dataTemp = (char *)objData + objectStructSize*objIndex;
            if (!strcmp(dataTemp, obj_id))
            {
                memcpy(obj_data, dataTemp, objectStructSize);
                return ((pageIndex-1)*pageSize + objIndex);
            }
        }
    }
    return kGeneralErr;
}


/********************************************************
 * function: storage_page_get_endpointlink
 * description:  获取某一页的记录
 * input:   1. page_size:
 *          2. current_page:  从 1 开始
 * output:  1. pendpointlink_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_page_get_endpointlink( uint8_t page_size, uint8_t current_page, PENDPOINTLINK_OBJ_T pendpointlink_obj)
{
    if (pendpointlink_obj == NULL)
    {
        os_storage_log("storage_page_get_endpointlink, pendpointlink_obj is NULL");
        return kGeneralErr;
    }
    return storage_read_endpointlink_obj_batch(page_size, page_size*(current_page-1), pendpointlink_obj);
}

/********************************************************
 * function: storage_page_get_endpoint_linkobj
 * description:  按页获取输入输出端点对象
 * input:   1. page_size:
 *          2. current_page:  从 1 开始
 * output:  1. pendpointlink_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
static char *st_getPendpointlinkObj =NULL;
OSStatus storage_page_get_endpoint_linkobj( char *input_endpoint, uint8_t page_size, uint8_t current_page, uint8_t current_size, PENDPOINTLINK_LIST_T pendpointlink_list)
{
    int ret = 0;
    if ((input_endpoint == NULL) || (pendpointlink_list == NULL) || (page_size > 5))
    {
        os_storage_log("input_endpoint or pendpointlink_list is NULL .");
        return kGeneralErr;
    }
    PENDPOINTLINK_OBJ_T pendpointlinkObj =NULL;
    if (st_getPendpointlinkObj==NULL){
        st_getPendpointlinkObj =malloc(5 * sizeof(ENDPOINTLINK_OBJ_T));
    }
    pendpointlinkObj = (PENDPOINTLINK_OBJ_T)st_getPendpointlinkObj;

    PENDPOINTLINK_OBJ_T pendpointlinkObjTemp = NULL;
    int count = 0;
    if ( pendpointlinkObj == NULL )
    {
        os_storage_log("malloc endpointlinkobj fail!  [memory free: %d]", MicoGetMemoryInfo()->free_memory);
        return kGeneralErr;
    }
    ret = storage_page_get_endpointlink(page_size, current_page, pendpointlinkObj);
//    os_storage_log("read pendpointlinkObj")
    if (ret == kNoErr)
    {
        for (count=0; count < current_size; count++)
        {
            pendpointlinkObjTemp = pendpointlinkObj + count;
            if (!strcmp(input_endpoint, pendpointlinkObjTemp->linkObj.inEndpointId))
            {
                memcpy(&pendpointlink_list->linkArray[pendpointlink_list->linkNum], &pendpointlinkObjTemp->linkObj, sizeof(LINK_OBJ_T));
                pendpointlink_list->linkNum++;
            }
        }
        return kNoErr;
    }
    else
    {
        return ret;
    }


//    free(pendpointlinkObj);
}

/********************************************************
 * function: storage_page_get_endpoint_linkobj
 * description:  按页获取输入输出端点对象
 * input:   1. page_size:
 *          2. current_page:  从 1 开始
 * output:  1. pendpointlink_obj: 调用该函数前 该指针所指向的数据要先清空
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_endpoint_linkobj( char *input_endpoint, PENDPOINTLINK_LIST_T pendpointlink_list)
{
    uint32_t count = 0;
    uint32_t totalNum = storage_get_object_num(ENDPOINTLINK_OBJ_ID);
    uint8_t pages = 0;
    uint8_t limit = 5;              // 按页读取，每页读取最大对象数量
    uint8_t pageSize = 0;
    uint16_t fullNum = 0;           // 表示每页记录数都是最大值时的总记录数
    int ret = 0;
    pages = mlink_get_total_pages(totalNum, limit);
    fullNum = pages*limit;
    for (count=0; count<pages; count++)
    {
        if ((count < pages-1) || (fullNum == totalNum))
        {
            pageSize = limit;
        }
        else
        {
            pageSize = totalNum%limit;
        }
        ret = storage_page_get_endpoint_linkobj(input_endpoint, limit, count+1, pageSize, pendpointlink_list);
        // add by 2018.3.3
        if (pendpointlink_list->linkNum >totalNum)
            pendpointlink_list->linkNum =totalNum;
        if (ret == kGeneralErr)
        {
            return kGeneralErr;
        }
    }
    return kNoErr;
}


/********************************************************
 * function: storage_check_endpointlink_obj
 * description:  save endpoint
 * input:   1. pendpointlink_obj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_lookup_endpointlink_obj( char *input_endpoint, uint8_t *obj_index, PLINK_OBJ_T plink_obj)
{
    uint32_t count = 0;
    uint32_t total = storage_get_object_num(ENDPOINTLINK_OBJ_ID);
    uint8_t page = 0;
    uint8_t limit = 5;              // 按页读取，每页读取最大对象数量
    uint8_t objNum = 0;
    uint8_t objIndex = 0;
    PENDPOINTLINK_OBJ_T pEndpointlinkObj = NULL;
    PENDPOINTLINK_OBJ_T objTemp = NULL;
    int ret = kNotFoundErr;
    if (input_endpoint == NULL)
    {
        os_storage_log("input_endpoint is NULL");
        return kGeneralErr;
    }
    if (total == 0)
    {
        os_storage_log("current endpointlink num is 0 !!!");
        return kNotFoundErr;
    }
    pEndpointlinkObj = (PENDPOINTLINK_OBJ_T)malloc(limit*sizeof(ENDPOINTLINK_OBJ_T));
    memset(pEndpointlinkObj, 0, limit*sizeof(ENDPOINTLINK_OBJ_T));
    if ((total%limit) != 0)
    {
        page ++;
    }
    page += total/limit;        // 总页数
    objNum = limit;
    os_storage_log("look up endpoint: %s. total: %d, page: %d, limit: %d, objNum: %d", input_endpoint, total, page, limit, objNum);

    for ( count = 0; count<page; count++ )
    {
        if ((count == page-1)&&(total%limit != 0))
        {
            objNum = total%limit;
        }
        storage_read_endpointlink_obj_batch(objNum, count*limit, pEndpointlinkObj);
        for (objIndex=0; objIndex<objNum; objIndex++)
        {
            objTemp = pEndpointlinkObj + objIndex;
//            os_storage_log("link input endpointid : %s", objTemp->linkObj.inEndpointId);
            if (!strcmp(input_endpoint, objTemp->linkObj.inEndpointId))
            {
                if (obj_index != NULL)
                {
                    *obj_index = count*page + objIndex;
                    os_storage_log("Found out endpointlink! index = %d", *obj_index);
                }
                if (plink_obj != NULL)
                {
                    memcpy(plink_obj, &objTemp->linkObj, sizeof(LINK_OBJ_T));
                }
                ret = kDuplicate;
                if (pEndpointlinkObj != NULL)
                {
                    free(pEndpointlinkObj);
                }
                return ret;
            }
        }
    }
    if (pEndpointlinkObj != NULL)
    {
        free(pEndpointlinkObj);
    }
    return ret;
}

/********************************************************
 * function: storage_page_check_endpoint_linkobj
 * description:  按页检查是否存在该链接
 * input:   1. endpoint_linkid: 输入输出链接ID
 *          2. page_size:
 *          3. current_page:  从 1 开始
 *          4. current_size: 当前页记录数量
 * output:  1. link_index
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_page_check_endpoint_linkobj( char *endpoint_linkid, uint32_t page_size, uint8_t current_page, uint8_t current_size, uint16_t *link_index )
{
    uint8_t count = 0;
    PENDPOINTLINK_OBJ_T pendpointlinkObj = NULL;
    PENDPOINTLINK_OBJ_T pendpointlinkObjTemp = NULL;
    if ((endpoint_linkid == NULL) || (link_index == NULL))
    {
        return kGeneralErr;
    }
    pendpointlinkObj = malloc(page_size * sizeof(ENDPOINTLINK_OBJ_T));
    if (pendpointlinkObj == NULL)
    {
        return kGeneralErr;
    }
    memset(pendpointlinkObj, 0, page_size*sizeof(ENDPOINTLINK_OBJ_T));
    storage_page_get_endpointlink(page_size, current_page, pendpointlinkObj);
    for (count=0; count < current_size; count++)
    {
        pendpointlinkObjTemp = pendpointlinkObj + count;
        if (!strcmp(pendpointlinkObjTemp->linkId, endpoint_linkid))
        {
            *link_index = count;
            free(pendpointlinkObj);
            os_storage_log("Find out this endpointlinkobj. ");
            return kNoErr;
        }
    }
    free(pendpointlinkObj);
    return kGeneralErr;
}

/********************************************************
 * function: storage_check_endpointlink_obj
 * description:  save endpoint
 * input:   1. pendpointlink_obj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_check_endpointlink_obj( char *endpoint_linkid, uint16_t *obj_index)
{
    uint32_t pagination = 0;
    uint32_t totalNum = storage_get_object_num(ENDPOINTLINK_OBJ_ID);
    uint8_t pages = 0;
    uint8_t limit = 5;              // 按页读取，每页读取最大对象数量
    uint8_t pageSize = 0;
    uint16_t fullNum = 0;           // 表示每页记录数都是最大值时的总记录数
    int ret = 0;
    pages = mlink_get_total_pages(totalNum, limit);
    fullNum = pages*limit;
    for (pagination=1; pagination<=pages; pagination++)
    {
        if ((pagination < pages) || (fullNum == totalNum))
        {
            pageSize = limit;
        }
        else
        {
            pageSize = totalNum%limit;
        }
        ret = storage_page_check_endpoint_linkobj(endpoint_linkid, limit, pagination, pageSize, obj_index);
        if (ret == kNoErr)
        {
            *obj_index = *obj_index + (pagination-1)*limit;
            return kNoErr;
        }
    }
    return kGeneralErr;
}

OSStatus storage_get_dev_by_addr(uint8_t *addr, PDEVICEOBJ_T pdev_obj)
{
    int count = 0;
    DEVICEOBJ_T deviceObj = {0};
    uint32_t daddr[2] = {0};
    if (pdev_obj == NULL)
    {
        return kGeneralErr;
    }
    for (count = 0; count<g_deviceNum; count++)
    {
        storage_read_dev_obj_batch(1, count, &deviceObj);
        memcpy((uint8_t *)&daddr[0], addr, 3);
//        mlink_parse_subdev_net_addr(deviceObj.comm, pdevObj->addr, (uint8_t *)&daddr[0]);
        mlink_parse_subdev_net_addr(deviceObj.comm, deviceObj.addr, (uint8_t *)&daddr[1]);
        os_storage_log("device addr is %x", daddr[1]);
        if (daddr[0] == daddr[1])
        {
            memcpy(pdev_obj, &deviceObj, sizeof(DEVICEOBJ_T));
            return kNoErr;
        }
    }
    return kGeneralErr;
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
OSStatus storage_update_local_softver(char *ver, int vertype)
{
    if (NULL == ver)
    {
        return kGeneralErr;
    }
    DEVINFOOBJ_T devInfo = {0};
    int ret = kNoErr;

    storage_read_devinfo_obj(&devInfo);

    if (vertype == FIRMWARE_VER)
    {
        if (0 > strcmp(devInfo.firmver, ver))
        {
            strcpy(devInfo.firmver, ver);
            ret = storage_write_devinfo_obj(&devInfo);
        }
    }
    else if (vertype == SOFTWARE_VER)
    {
        if (0 > strcmp(devInfo.ver, ver))
        {
            strcpy(devInfo.ver, ver);
            ret = storage_write_devinfo_obj(&devInfo);
        }
    }
    return ret;
}


/********************************************************
 * function: storage_update_device_obj
 * description:
 * input:   1. obj_num:
 *          2. pDeviceObj
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_update_device_obj( uint8_t obj_num, PDEVICEOBJ_T pDeviceObj )
{
    uint8_t count = 0;
    PDEVSTATUSOBJ_T pdevStateObj = NULL;
    for (count=0; count<obj_num; count++)
    {
        storage_get_dev_status(pDeviceObj->deviceId, &pdevStateObj);
        pdevStateObj->liveTime = pDeviceObj->offline;
    }
    return storage_write_dev_obj(OBJECT_UPDATE_MODIFY, obj_num, pDeviceObj);
}

/********************************************************
 * function: storage_update_cloud_obj
 * description:
 * input:   1. pCloudObj: point to a structure that we want to update
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_cloud_obj(PCLOUDOBJ_T pCloudObj)
{

    return storage_write_cloud_obj(pCloudObj);
}

/********************************************************
 * function: storage_update_room_obj
 * description:  write some devices to initial address
 * input:   1. obj_num:  point to a structure that we want to save
 *          2. pRoomObj:     the number of object you want to write
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_room_obj( uint8_t room_num, PROOMOBJ_T proom_obj )
{
    uint8_t updateIndex = 0;
    uint8_t readIndex = 0;
    uint8_t updateRoomNum = room_num;
    int ret = kNoErr;
    PROOMOBJ_T pUpdateRoomObjTemp;
    PROOMOBJ_T pRoomObj = NULL;
    PROOMOBJ_T pReadRoomObjTemp = NULL;
    if (g_roomNum != 0)
    {
        pRoomObj = (PROOMOBJ_T)malloc(g_roomNum*sizeof(ROOMOBJ_T));
        storage_read_room_obj_batch(g_roomNum, 0, pRoomObj);
    }
    for (updateIndex=0; updateIndex<updateRoomNum; updateIndex++)
    {
        pUpdateRoomObjTemp = proom_obj + updateIndex;
        for (readIndex=0; readIndex < g_roomNum; readIndex++)
        {
            pReadRoomObjTemp = pRoomObj+readIndex;
            if (0 == strcmp(pUpdateRoomObjTemp->roomId, pReadRoomObjTemp->roomId))
            {
                ret = storage_write_room_obj(1, readIndex, pUpdateRoomObjTemp);
                break;
            }
        }
    }
    return ret;
//    return storage_write_room_obj(OBJECT_UPDATE_MODIFY, obj_num, pRoomObj);
}

/********************************************************
 * function: storage_update_scene_obj
 * description:  write some devices to initial address
 * input:   1. pRoomObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_scene_obj( uint8_t sceneNum, PSCENEOBJ_T pscene_obj )
{
    return storage_write_scene_obj(OBJECT_UPDATE_MODIFY, sceneNum, pscene_obj);
}

/********************************************************
 * function: storage_update_event_obj
 * description:  write some devices to initial address
 * input:   1. obj_num:  point to a structure that we want to save
 *          2. pevent_obj:       the number of object you want to write
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_event_obj( unsigned char obj_num, PEVENTOBJ_T pevent_obj )
{
    return storage_write_event_obj(OBJECT_UPDATE_MODIFY, obj_num, pevent_obj);
}

/********************************************************
 * function: storage_update_schedule_obj
 * description:  write some devices to initial address
 * input:   1. update_type: point to a structure that we want to save
 *          2. pschedule_obj:     the number of object you want to write
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_schedule_obj( PSCHEDULEOBJ_T pschedule_obj )
{
    return storage_write_schedule_obj( OBJECT_UPDATE_MODIFY, pschedule_obj );
}

/********************************************************
 * function: storage_update_loopaction_obj
 * description:  write some devices to initial address
 * input:   1. ploopActionObj:  point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_loopaction_obj(PLOOPACTION_T ploopActionObj, unsigned char num, unsigned char startIndex)
{
    return storage_write_loopaction_obj(ploopActionObj, num, startIndex);
}

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: storage_update_endpoint_obj
 * description:  save endpoint
 * input:   1. obj_num:     the number of object you want to write
 *          2. pEndpointObj: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_dev_endpoint( PENDPOINTOBJ_EX_T pendpoint_obj )
{
    if (pendpoint_obj == NULL)
    {
        return kGeneralErr;
    }
    else
    {
        ENDPOINTOBJ_EX_T singleDevEndpoints = {0};
        int ret = 0;
        ret = storage_read_endpoint_obj(pendpoint_obj->fid, &singleDevEndpoints);
        if (ret != kGeneralErr)
        {
            uint8_t count = 0;
            uint8_t updateCount = 0;
            uint8_t startIndex = ret - 1;
            ALARM_ENDPOINT_T alarmEndpoint = {0};
            for (updateCount=0; updateCount<pendpoint_obj->endpointNum; updateCount++)
            {
                for (count=0; count<singleDevEndpoints.endpointNum; count++)
                {
                    if (!strcmp(pendpoint_obj->endpointlist[updateCount].id, singleDevEndpoints.endpointlist[count].id))
                    {
                        ENDPOINT_ELE_T endpointEle = {0};
                        mlink_parse_endpointid(pendpoint_obj->endpointlist[updateCount].id, &endpointEle);
                        memcpy(&singleDevEndpoints.endpointlist[count], &pendpoint_obj->endpointlist[updateCount], sizeof(ENDPOINT_EX_T));

                        if ((0 == strcmp(endpointEle.key, KEY_ALARM_DELAY))||(0 == strcmp(endpointEle.key, KEY_ALARM_STATE)))
                        {
                            ret = storage_get_alarm_endpoint(pendpoint_obj->fid, &alarmEndpoint);
                            storage_update_dev_alarm_state(1, &alarmEndpoint);
                        }
                        break;
                    }
                }
            }
            ret = storage_write_endpoint_obj(1, startIndex, &singleDevEndpoints);
        }
        return ret;
    }
}
#else
/********************************************************
 * function: storage_update_endpoint_obj
 * description:  save endpoint
 * input:   1. obj_num:     the number of object you want to write
 *          2. pEndpointObj: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_dev_endpoint( PENDPOINTOBJ_EX_T pendpoint_obj )
{
    if (pendpoint_obj == NULL)
    {
        return kGeneralErr;
    }
    else
    {
        SINGLE_DEV_ENDPOINTOBJ_T singleDevEndpoints = {0};
        int ret = 0;
        ret = storage_read_endpoint_obj(pendpoint_obj->fid, &singleDevEndpoints);
        if (ret != kGeneralErr)
        {
            uint8_t count = 0;
            uint8_t updateCount = 0;
            uint8_t startIndex = ret - 1;
            ALARM_ENDPOINT_T alarmEndpoint = {0};
            for (updateCount=0; updateCount<pendpoint_obj->endpointNum; updateCount++)
            {
                for (count=0; count<singleDevEndpoints.endpointNum; count++)
                {
                    if (!strcmp(pendpoint_obj->endpointlist[updateCount].endpoint.id, singleDevEndpoints.endpointlist[count].id))
                    {
                        ENDPOINT_ELE_T endpointEle = {0};
                        mlink_parse_endpointid(pendpoint_obj->endpointlist[updateCount].endpoint.id, &endpointEle);
                        memcpy(&singleDevEndpoints.endpointlist[count], &pendpoint_obj->endpointlist[updateCount].endpoint, sizeof(ENDPOINT_T));

                        if ((0 == strcmp(endpointEle.key, KEY_ALARM_DELAY))||(0 == strcmp(endpointEle.key, KEY_ALARM_STATE)))
                        {
                            ret = storage_get_alarm_endpoint(pendpoint_obj->fid, &alarmEndpoint);
                            if ((ret != kGeneralErr) && (ret != 0))
                            {
                                if (!strcmp(endpointEle.key, KEY_ALARM_DELAY))
                                {
                                    memcpy(&pendpoint_obj->endpointlist[updateCount].endpoint, &alarmEndpoint.endpointlist[1], sizeof(ENDPOINT_T));
                                }
                                else if (!strcmp(endpointEle.key, KEY_ALARM_STATE))
                                {
                                    memcpy(&pendpoint_obj->endpointlist[updateCount].endpoint, &alarmEndpoint.endpointlist[0], sizeof(ENDPOINT_T));
                                }
                            }
                            storage_update_dev_alarm_state(1, &alarmEndpoint);
                        }
                        break;
                    }
                }
            }
            ret = storage_write_endpoint_obj(1, startIndex, &singleDevEndpoints);
        }
        return ret;
    }
}
#endif

/********************************************************
 * function: storage_update_endpoint_obj
 * description:  save endpoint
 * input:   1. obj_num:     the number of object you want to write
 *          2. pEndpointObj: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_endpoint_obj( uint8_t obj_num, PENDPOINTOBJ_EX_T pendpoint_obj )
{
    char devid[32]=  {0};
    int ret = 0;
    uint8_t count = 0;
    PENDPOINTOBJ_EX_T pEndpointObjTemp = NULL;
    mlink_get_local_id(devid);
    for (count=0; count < obj_num; count++)
    {
        pEndpointObjTemp = pendpoint_obj + count;
        if (!strcmp(pEndpointObjTemp->fid, devid))
        {
            ret = storage_update_local_endpoint( pEndpointObjTemp->endpointlist, obj_num );
        }
        else
        {
            storage_update_dev_endpoint(pEndpointObjTemp);
//            ret = storage_write_endpoint_obj_ex( OBJECT_UPDATE_MODIFY, pEndpointObjTemp->endpointNum, pEndpointObjTemp->endpointlist );
        }
    }
    return ret;
}

/********************************************************
 * function: storage_update_endpointlink_obj
 * description:  save endpoint
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:     the number of object you want to write
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_endpointlink_obj( uint8_t obj_num, PENDPOINTLINK_OBJ_T pendpointlink_obj )
{
    uint16_t objIndex = 0;
    uint8_t count = 0;
    int ret = 0;
    PENDPOINTLINK_OBJ_T pEndpointlinkTemp = NULL;
    if (pendpointlink_obj == NULL)
    {
        os_storage_log("Update endpointlink. pendpointlink_obj is null !!!");
        return kGeneralErr;
    }
    for (count = 0; count< obj_num; count++)
    {
        pEndpointlinkTemp = pendpointlink_obj + count;
        ret = storage_check_endpointlink_obj(pEndpointlinkTemp->linkId, &objIndex);
        if (ret == kNoErr)
        {
            os_storage_log("update the %dth link.", objIndex);
            ret = storage_write_endpointlink_obj( 1, objIndex, pEndpointlinkTemp );
        }
    }
    return kNoErr;
}

/********************************************************
 * function: storage_update_local_obj
 * description:  save endpoint
 * input:   1. plocal_net_obj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_local_obj( PNETDEVOBJ_T local_net_obj )
{
    NETDEVOBJ_T netObj = {0};
    OSStatus ret = kNoErr;
    storage_read_local_devobj(&netObj);
    if (strcmp(netObj.uuid, local_net_obj->uuid))
    {
        if (strlen(netObj.uuid) >= 12)              //  Only when uuid is mac,  uuid can be updated.
        {
            ret = storage_write_local_obj( local_net_obj );
        }
    }
    else
    {
#if 1
        if (memcmp(local_net_obj, &netObj, sizeof(NETDEVOBJ_T)))
        {
            if ((netObj.deviceId[0] == 0)||(0 == strcmp(local_net_obj->deviceId, netObj.deviceId)))
            {
                ret = storage_write_local_obj( local_net_obj );
            }
        }
#else
        if ((netObj.deviceId[0] == 0) || ((uint8_t)netObj.deviceId[0] >= 0x7f))
        {
            ret = storage_write_local_obj( local_net_obj );
        }
        else if (0 == strcmp(netObj.deviceId, local_net_obj->deviceId))
        {
            if (strcmp(netObj.room_uid, local_net_obj->room_uid))
            {
                ret = storage_write_local_obj( local_net_obj );
            }
            else
            {
                ret = kNoErr;
            }
        }
#endif
    }
    return ret;
}


/********************************************************
 * function: storage_update_one_linkage_input_obj
 * description:  save endpoint
 * input:   1. plinkage_input_obj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_one_linkage_input_obj( PLINKAGE_INPUT_TASK_T plinkage_input_obj )
{
    int linkageIndex = 0;
    int ret = kNoErr;
    LINKAGE_INPUT_TASK_T lnkInputObj = {0};

    linkageIndex = storage_get_obj_by_id(LINKAGE_INPUT_OBJ_ID, plinkage_input_obj->linkageId, &lnkInputObj);
    os_storage_log("find this input id result :%d, id : %s", linkageIndex, plinkage_input_obj->linkageId);
    if (linkageIndex >= 0)     // find the linkage object. Update object
    {
//        os_storage_log("find this input id is :%d", linkageIndex);
        ret = storage_write_linkage_input_obj(1, linkageIndex, plinkage_input_obj);
        if (ret == kNoErr)
        {
            servive_manage_linkage_cron_deal(1, &lnkInputObj);
            if (( lnkInputObj.cflag != plinkage_input_obj->cflag ))
            {
                main_set_linkage_input_cflag(linkageIndex, plinkage_input_obj->cflag);
            }
            servive_manage_linkage_cron_deal(0, plinkage_input_obj);
        }
    }
    else
    {
        ret = kGeneralErr;
    }
    return ret;
}

/********************************************************
 * function: storage_update_linkage_input_obj
 * description:  save endpoint
 * input:   1. plocal_net_obj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_linkage_input_obj( uint8_t obj_num, PLINKAGE_INPUT_TASK_T plinkage_input_obj )
{
    uint8_t count = 0;
    int ret = 0;
    PLINKAGE_INPUT_TASK_T linkageInputObjTemp = NULL;
    if ( plinkage_input_obj == NULL )
    {
        return kGeneralErr;
    }
    for (count = 0; count < obj_num; count++)
    {
        linkageInputObjTemp = plinkage_input_obj + count;
        ret = storage_update_one_linkage_input_obj( linkageInputObjTemp );
    }
    os_storage_log("update number: %d, update result: %d", count, ret);
    return ret;
}

/********************************************************
 * function: storage_update_one_linkage_output_obj
 * description:  save endpoint
 * input:   1. plocal_net_obj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_one_linkage_output_obj( PLINKAGE_OUTPUT_TASK_T plinkage_output_obj )
{
    int linkageIndex = 0;
    int ret = kNoErr;
    LINKAGE_OUTPUT_TASK_T lnkOutputObj = {0};

    linkageIndex = storage_get_obj_by_id(LINKAGE_OUTPUT_OBJ_ID, plinkage_output_obj->linkageId, &lnkOutputObj);
    if (linkageIndex >= 0)     // find the linkage object. Update object
    {
        ret = storage_write_linkage_output_obj(1, linkageIndex, plinkage_output_obj);
    }
    else
    {
        ret = kGeneralErr;
    }
    return ret;
}

/********************************************************
 * function: storage_update_linkage_output_obj
 * description:  save endpoint
 * input:   1. plocal_net_obj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_linkage_output_obj( uint8_t obj_num, PLINKAGE_OUTPUT_TASK_T plinkage_output_obj )
{
    uint8_t count = 0;
    int ret = 0;
    PLINKAGE_OUTPUT_TASK_T linkageOutputObjTemp = NULL;
    if ( plinkage_output_obj == NULL )
    {
        return kGeneralErr;
    }
    for (count = 0; count < obj_num; count++)
    {
        linkageOutputObjTemp = plinkage_output_obj + count;
        ret = storage_update_one_linkage_output_obj( linkageOutputObjTemp );
    }
    return ret;
}


/********************************************************
 * function: storage_update_dev_alarm_state
 * description:  save endpoint
 * input:   1. alarm_num: point to a structure that we want to save
 *          2. pdevice_alarm_state:     the number of object you want to write
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_update_dev_alarm_state( uint8_t alarm_num, PALARM_ENDPOINT_T pdevice_alarm_state )
{
    uint32_t devNum = 0;
    uint32_t count = 0;
    PALARM_ENDPOINT_T pDevAlarmState = NULL;
    PALARM_ENDPOINT_T pDevAlarmTemp = NULL;
    PALARM_ENDPOINT_T palarmUpdateTemp = NULL;
    uint8_t updateCount = 0;
    storage_read_dev_alarm_status(&devNum, &pDevAlarmState);
    for (updateCount=0; updateCount<alarm_num; updateCount++)
    {
        palarmUpdateTemp = pdevice_alarm_state + updateCount;
//        count++;
        for (count=0; count < devNum; count++)
        {
            pDevAlarmTemp = pDevAlarmState + count;
            if (!strcmp(pDevAlarmTemp->devid, palarmUpdateTemp->devid))
            {
                memcpy(pDevAlarmTemp, palarmUpdateTemp, sizeof(ALARM_ENDPOINT_T));
                os_storage_log("device exist!!!");
                break;
            }
        }
    }
    os_storage_log("update state end!!!");
    mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
    return storage_write_dev_alarm_state(devNum, pDevAlarmState);


//    return storage_write_endpointlink_obj( OBJECT_UPDATE_MODIFY, alarm_num, pEndpointlinkObj );
}

/********************************************************
 * function: storage_add_device_obj
 * description:
 * input:   1. sceneNum: point to a structure that we want to save
 *          2. pscene_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_device_status( PDEVSTATUSOBJ_T pdev_status_obj )
{
    if (pdev_status_obj != NULL)
    {
        memcpy(g_devStatusObj+g_deviceNum+1, pdev_status_obj, sizeof(DEVSTATUSOBJ_T));
    }
}

/********************************************************
 * function: storage_add_local_obj
 * description:
 * input:   1. sceneNum: point to a structure that we want to save
 *          2. pscene_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_local_obj( PNETDEVOBJ_T local_net_obj )
{
    return storage_update_local_obj( local_net_obj );
}
/********************************************************
 * function: storage_add_device_obj
 * description:
 * input:   1. sceneNum: point to a structure that we want to save
 *          2. pscene_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_device_obj( uint8_t obj_num, PDEVICEOBJ_T pdev_obj )
{
    uint8_t count = 0;
    int ret = 0;
    PDEVSTATUSOBJ_T pdevStateObj = NULL;
    PDEVICEOBJ_T pdevObjTemp = NULL;
    for (count=0; count < obj_num; count++)
    {
        pdevObjTemp = pdev_obj + count;
        pdevStateObj = NULL;
        ret = storage_get_dev_status(pdevObjTemp->deviceId, &pdevStateObj);
        if (ret != 0)
        {
            pdevStateObj->liveTime = pdevObjTemp->offline;
        }
        else
        {
            pdevStateObj = malloc(sizeof(DEVSTATUSOBJ_T));
            if (pdevStateObj != NULL)
            {
                strcpy(pdevStateObj->devid, pdevObjTemp->deviceId);
                pdevStateObj->liveTime = pdevObjTemp->offline;
                pdevStateObj->restTime = pdevObjTemp->offline;
                pdevStateObj->status.statusInfo = 1;
                storage_add_device_status(pdevStateObj);
                free(pdevStateObj);
            }
        }
    }
    #ifdef BLE_DEVICE
    storage_set_present_lockpin(pdev_obj->deviceId);
    //ble_bind_lock_start();
    ble_bind_lock_bymac(pdev_obj->mac);//begin bind lock
#endif
    return storage_write_dev_obj(OBJECT_UPDATE_ADD, obj_num, pdev_obj);
}

/********************************************************
 * function: storage_add_scene_obj
 * description:
 * input:   1. sceneNum: point to a structure that we want to save
 *          2. pscene_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_scene_obj( uint8_t sceneNum, PSCENEOBJ_T pscene_obj )
{
    return storage_write_scene_obj(OBJECT_UPDATE_ADD, sceneNum, pscene_obj);
}

/********************************************************
 * function: storage_add_room_obj
 * description:
 * input:   1. room_num:
 *          2. proom_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_room_obj( uint8_t room_num, PROOMOBJ_T proom_obj )
{
    uint8_t addIndex = 0;
    uint8_t readIndex = 0;
    uint8_t addRoomNum = room_num;
    int ret = kNoErr;
    PROOMOBJ_T pAddRoomObjTemp;
    PROOMOBJ_T pRoomObj = NULL;
    PROOMOBJ_T pReadRoomObjTemp = NULL;
    if (g_roomNum != 0)
    {
        pRoomObj = (PROOMOBJ_T)malloc(g_roomNum*sizeof(ROOMOBJ_T));
        storage_read_room_obj_batch(g_roomNum, 0, pRoomObj);
    }
    for (addIndex=0; addIndex<addRoomNum; addIndex++)
    {
        pAddRoomObjTemp = proom_obj + addIndex;
        for (readIndex=0; readIndex < g_roomNum; readIndex++)
        {
            pReadRoomObjTemp = pRoomObj+readIndex;
            if (0 == strcmp(pAddRoomObjTemp->roomId, pReadRoomObjTemp->roomId))
            {
                ret = storage_write_room_obj(1, readIndex, pAddRoomObjTemp);
                if (addIndex < (addRoomNum - 1))                      // Delete member had exist from the add list
                {
                    uint8_t *dst = (uint8_t *)proom_obj + addIndex*sizeof(ROOMOBJ_T);
                    uint8_t *src = (uint8_t *)proom_obj + (addIndex + 1)*sizeof(ROOMOBJ_T);
                    memcpy(dst, src, (addRoomNum-addIndex-1)*sizeof(ROOMOBJ_T));
                }
                addRoomNum --;
                break;
            }
        }
    }
    if (addRoomNum != 0)
    {
        ret = storage_write_room_obj( addRoomNum, readIndex, proom_obj);
        if (ret == kNoErr)
        {
            g_roomNum = g_roomNum+addRoomNum;
        }
    }
    if (pRoomObj != NULL)
    {
        free(pRoomObj);
    }
    return ret;
}

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: storage_add_endpoint_obj
 * description:  save endpoint
 * input:   1. pEndpointExObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_endpoint_obj( PENDPOINTOBJ_EX_T pEndpointExObj )
{
    uint8_t endpointCount = 0;
    OSStatus ret = 0;
    ALARM_ENDPOINT_T alarmEndpoint = {0};

    for (endpointCount=0; endpointCount < pEndpointExObj->endpointNum; endpointCount++)
    {
        if ((0==strcmp(pEndpointExObj->endpointlist[endpointCount].key, KEY_ALARM_DELAY))
                || (0==strcmp(pEndpointExObj->endpointlist[endpointCount].key, KEY_ALARM_STATE)))  // special endpoint
        {
            ENDPOINT_ELE_T endpointEle = {0};
            mlink_parse_endpointid(pEndpointExObj->endpointlist[endpointCount].id, &endpointEle);
            strcpy(alarmEndpoint.devid, pEndpointExObj->fid);
            alarmEndpoint.endpointNum++;
            if (!strcmp(endpointEle.key,KEY_ALARM_STATE))
            {
                if (pEndpointExObj->endpointlist[endpointCount].value[0] == 0)
                {
                    alarmEndpoint.alarmType = 0;
                }
                else
                {
                    alarmEndpoint.alarmType = (uint8_t)atoi(pEndpointExObj->endpointlist[endpointCount].value);
                }
            }
            else if (!strcmp(endpointEle.key, KEY_ALARM_DELAY))
            {
                if (pEndpointExObj->endpointlist[endpointCount].value[0] == 0)
                {
                    alarmEndpoint.alarmDelay = 0;
                }
                else
                {
                    alarmEndpoint.alarmDelay = (uint8_t)atoi(pEndpointExObj->endpointlist[endpointCount].value);
                }
            }
        }
    }
    os_storage_log("alarm endpoint num is %d", alarmEndpoint.endpointNum);
    if (alarmEndpoint.endpointNum != 0)
    {
        ret = storage_add_alarm_endpoint( &alarmEndpoint );
    }
    ret = storage_write_endpoint_obj_ex(OBJECT_UPDATE_ADD, 1, pEndpointExObj);
    return ret;
}
#else
/********************************************************
 * function: storage_add_endpoint_obj
 * description:  save endpoint
 * input:   1. pEndpointExObj: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_endpoint_obj( PENDPOINTOBJ_EX_T pEndpointExObj )
{
    PSINGLE_DEV_ENDPOINTOBJ_T  endpointTemp = (PSINGLE_DEV_ENDPOINTOBJ_T)malloc(sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
    uint8_t endpointCount = 0;
    OSStatus ret = 0;
    ALARM_ENDPOINT_T alarmEndpoint = {0};
    memset(endpointTemp, 0, sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
    strcpy(endpointTemp->fid, pEndpointExObj->fid);
    endpointTemp->endpointNum = pEndpointExObj->endpointNum;

    for (endpointCount=0; endpointCount < pEndpointExObj->endpointNum; endpointCount++)
    {
        memcpy(&endpointTemp->endpointlist[endpointCount], &pEndpointExObj->endpointlist[endpointCount].endpoint, sizeof(ENDPOINT_T));
        if (pEndpointExObj->endpointlist[endpointCount].value[0] != 0 || (0==strcmp(pEndpointExObj->endpointlist[endpointCount].endpoint.key, KEY_ALARM_DELAY))
                || (0==strcmp(pEndpointExObj->endpointlist[endpointCount].endpoint.key, KEY_ALARM_STATE)))  // special endpoint
        {
            ENDPOINT_ELE_T endpointEle = {0};
            mlink_parse_endpointid(pEndpointExObj->endpointlist[endpointCount].endpoint.id, &endpointEle);
            strcpy(alarmEndpoint.devid, pEndpointExObj->fid);
            memcpy(&alarmEndpoint.endpointlist[alarmEndpoint.endpointNum], &pEndpointExObj->endpointlist[endpointCount].endpoint, sizeof(ENDPOINT_T));
            alarmEndpoint.endpointNum++;
            if (!strcmp(endpointEle.key,KEY_ALARM_STATE))
            {
                if (pEndpointExObj->endpointlist[endpointCount].value[0] != 0)
                {
                    alarmEndpoint.alarmType = 0;
                }
                else
                {
                    alarmEndpoint.alarmType = (uint8_t)atoi(pEndpointExObj->endpointlist[ALARM_STATE].value);
                }
            }
            else if (!strcmp(endpointEle.key, KEY_ALARM_DELAY))
            {
                if (pEndpointExObj->endpointlist[endpointCount].value[0] != 0)
                {
                    alarmEndpoint.alarmDelay = 0;
                }
                else
                {
                    alarmEndpoint.alarmDelay = (uint8_t)atoi(pEndpointExObj->endpointlist[ALARM_DELAY].value);
                }
            }
        }
    }
    os_storage_log("alarm endpoint num is %d", alarmEndpoint.endpointNum);
    if (alarmEndpoint.endpointNum != 0)
    {
        ret = storage_add_alarm_endpoint( &alarmEndpoint );
    }
    ret = storage_write_endpoint_obj_ex(OBJECT_UPDATE_ADD, 1, endpointTemp);
    free(endpointTemp);
    return ret;
}
#endif



/********************************************************
 * function: storage_add_alarm_endpoint
 * description:  save endpoint
 * input:   1. alarm_endpoint: point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_alarm_endpoint( PALARM_ENDPOINT_T palarm_endpoint )
{
    PALARM_ENDPOINT_T palarmEndpointCur = NULL;
    PALARM_ENDPOINT_T palarmEndpointTemp = NULL;
    uint32_t alarmNum = 0;
    uint8_t alarmDevCount = 0;
    if (palarm_endpoint == NULL)
    {
        os_storage_log("palarm_endpoint point to NULL!!!");
        return kGeneralErr;
    }
    storage_read_dev_alarm_status( &alarmNum, &palarmEndpointCur );
    for (alarmDevCount = 0; alarmDevCount < alarmNum; alarmDevCount++)
    {
        palarmEndpointTemp = palarmEndpointCur + alarmDevCount;
        if (!strcmp(palarmEndpointTemp->devid, palarm_endpoint->devid))
        {
            palarm_endpoint->alarmType = palarmEndpointTemp->alarmType;
            palarm_endpoint->alarmDelay = palarmEndpointTemp->alarmDelay;
            break;
        }
    }
    os_storage_log("alarmDevCount is %d", alarmDevCount);
    if (alarmDevCount == alarmNum)
    {
        memcpy(palarmEndpointCur+alarmNum, palarm_endpoint, sizeof(ALARM_ENDPOINT_T));
        alarmNum ++;
    }
    mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
    return storage_write_dev_alarm_state( alarmNum, palarmEndpointCur );
}


/********************************************************
 * function: storage_add_endpointlink_obj
 * description:  save endpoint
 * input:   1. obj_num:
 *          2. pendpointlink_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_endpointlink_obj( uint8_t obj_num, PENDPOINTLINK_OBJ_T pendpointlink_obj )
{
    PENDPOINTLINK_OBJ_T pEndpointLinkTemp = NULL;
    uint32_t count = 0;
    int ret = 0;
    uint16_t objIndex = 0;
    for (count = 0; count < obj_num; count++)
    {
        pEndpointLinkTemp = pendpointlink_obj +count;
        ret = storage_check_endpointlink_obj(pEndpointLinkTemp->linkId, &objIndex);
        if (ret == kNoErr)      // This linkid is exist, replace it .
        {
            storage_write_endpointlink_obj(1, objIndex, pEndpointLinkTemp);
            continue;
        }
        else
        {
            objIndex = g_endpointLinkNum;
        }
        ret = storage_write_endpointlink_obj(1, objIndex, pEndpointLinkTemp);
        if ((ret == kNoErr) && (objIndex == g_endpointLinkNum))     // 保存成功后，如果此设备之前是不存在的，则总数量自增1
        {
            g_endpointLinkNum++;
            os_storage_log("Add endpointlink success!!! total: %d", g_endpointLinkNum);
        }
        ret = kNoErr;
    }
    return ret;

}

/********************************************************
 * function: storage_add_linkage_input_obj
 * description:  save endpoint
 * input:   1. obj_num:
 *          2. pendpointlink_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_linkage_input_obj( uint8_t obj_num, PLINKAGE_INPUT_TASK_T plinkage_input_obj )
{
    int ret = kNoErr;
    uint8_t addLnkNum = obj_num;
    PLINKAGE_INPUT_TASK_T lnkInputObjTemp = NULL;
    if (plinkage_input_obj == NULL)
    {
        return kGeneralErr;
    }
    os_storage_log("obj_num is %d, memory free: %d", obj_num, MicoGetMemoryInfo()->free_memory);

    if ( g_linkageInputNum != 0 )
    {
        int count = 0;
        for (count = 0; count < addLnkNum; count++)
        {
            lnkInputObjTemp = plinkage_input_obj + count;
            ret = storage_update_one_linkage_input_obj(lnkInputObjTemp);
            if (ret != kGeneralErr)     // find the linkage object. Update object
            {
                if (count < (addLnkNum - 1))                      // Delete member had exist from the add list
                {
                    uint8_t *dst = (uint8_t *)plinkage_input_obj + count*sizeof(LINKAGE_INPUT_TASK_T);
                    uint8_t *src = (uint8_t *)plinkage_input_obj + (count + 1)*sizeof(LINKAGE_INPUT_TASK_T);
                    memcpy(dst, src, (addLnkNum-count-1)*sizeof(LINKAGE_INPUT_TASK_T));
                }
                count --;
                addLnkNum--;
            }
        }
    }

    if (addLnkNum != 0)
    {
        ret = storage_write_linkage_input_obj(addLnkNum, g_linkageInputNum, plinkage_input_obj);
        if (ret == kNoErr)
        {
            for (uint8_t addLnkIndex=0; addLnkIndex < addLnkNum; addLnkIndex++)
            {
                lnkInputObjTemp = plinkage_input_obj + addLnkIndex;
                servive_manage_linkage_cron_deal(0, lnkInputObjTemp);
                main_add_linkage_status(g_linkageInputNum+addLnkIndex, lnkInputObjTemp->linkageId, lnkInputObjTemp->cflag);
            }
            g_linkageInputNum += addLnkNum;
        }
    }
    os_storage_log("memory free: %d",  MicoGetMemoryInfo()->free_memory);

    return ret;
}

/********************************************************
 * function: storage_add_linkage_output_obj
 * description:  save endpoint
 * input:   1. obj_num:
 *          2. pendpointlink_obj:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_add_linkage_output_obj( uint8_t obj_num, PLINKAGE_OUTPUT_TASK_T plinkage_output_obj )
{
    int ret = kNoErr;
    uint8_t addLnkNum = obj_num;
    if (plinkage_output_obj == NULL)
    {
        return kGeneralErr;
    }
    os_storage_log("obj_num is %d, memory free: %d", obj_num, MicoGetMemoryInfo()->free_memory);

    if ( g_linkageOutputNum != 0 )
    {
        int count = 0;
        PLINKAGE_OUTPUT_TASK_T lnkOutputObjTemp = NULL;
        for (count = 0; count < addLnkNum; count++)
        {
            lnkOutputObjTemp = plinkage_output_obj + count;
            ret = storage_update_one_linkage_output_obj(lnkOutputObjTemp);
            if (ret != kGeneralErr)     // find the linkage object. Update object
            {
                if (count < (addLnkNum - 1))                      // Delete member had exist from the add list
                {
                    uint8_t *dst = (uint8_t *)plinkage_output_obj + count*sizeof(LINKAGE_OUTPUT_TASK_T);
                    uint8_t *src = (uint8_t *)plinkage_output_obj + (count + 1)*sizeof(LINKAGE_OUTPUT_TASK_T);
                    memcpy(dst, src, (addLnkNum-count-1)*sizeof(LINKAGE_OUTPUT_TASK_T));
                }
                count --;
                addLnkNum--;
            }
        }
    }

    if (addLnkNum != 0)
    {
        ret = storage_write_linkage_output_obj(addLnkNum, g_linkageOutputNum, plinkage_output_obj);
        if (ret == kNoErr)
        {
            g_linkageOutputNum += addLnkNum;
        }
    }
    os_storage_log("memory free: %d",  MicoGetMemoryInfo()->free_memory);

    return ret;
}


/********************************************************
 * function: storage_get_special_endpointlink
 * description:  save endpoint
 * input:   1. device_id:
 * output:  1. palarm_endpoint
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_endpointlink_by_linkid( char *endpointlink_id,  PENDPOINTLINK_OBJ_T pendpointlink_obj )
{
    if ((endpointlink_id == NULL) || (pendpointlink_obj == NULL))
    {
        return kGeneralErr;
    }
    PENDPOINTLINK_OBJ_T pendpointlnkObj = NULL;
    PENDPOINTLINK_OBJ_T pendpointlnkObjTmp = NULL;
    uint8_t pageSize = 5;
    uint8_t currPage = 0;
    uint8_t currSize = 0;
    uint32_t totalNum = storage_get_object_num(ENDPOINTLINK_OBJ_ID);
    uint8_t pageNum = mlink_get_total_pages(totalNum, pageSize);
    pendpointlnkObj = malloc(pageSize * sizeof(ENDPOINTLINK_OBJ_T));
    int count = 0;
    int ret = 0;

    if (pendpointlnkObj == NULL)
    {
        return kGeneralErr;
    }
    for (currPage=1; currPage<=pageNum; currPage++)     // look up from every page
    {
        ret = storage_page_get_endpointlink(pageSize, currPage, pendpointlnkObj);
        if (ret == kNoErr)
        {
            if (currPage<pageNum)
            {
                currSize = pageSize;
            }
            else
            {
                currSize = totalNum - (currPage-1)*pageSize;
            }
            for (count=0; count < currSize; count++)
            {
                pendpointlnkObjTmp = pendpointlnkObj + count;
                if (!strcmp(endpointlink_id, pendpointlnkObjTmp->linkId))
                {
                    memcpy(pendpointlink_obj, pendpointlnkObjTmp, sizeof(ENDPOINTLINK_OBJ_T));
                    free(pendpointlnkObj);
                    return kNoErr;
                }
            }
        }
        else
        {
            break;
        }
    }

    free(pendpointlnkObj);
    return kGeneralErr;
}

/********************************************************
 * function: storage_get_alarm_endpoint
 * description:  save endpoint
 * input:   1. device_id:
 * output:  1. palarm_endpoint
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_alarm_endpoint( char *device_id, PALARM_ENDPOINT_T palarm_endpoint )
{
    if ((device_id == NULL)||(palarm_endpoint == NULL))
    {
        return kGeneralErr;
    }
    else
    {
        int ret = 0;
        uint8_t count = 0;
        uint32_t alarmNum = 0;
        PALARM_ENDPOINT_T palarmEndpoint = NULL;
        PALARM_ENDPOINT_T palarmEndpointTemp = NULL;
        ret = storage_read_dev_alarm_status(&alarmNum, &palarmEndpoint);
        if (ret == kNoErr)
        {
            if (alarmNum != 0)
            {
                for (count=0; count < alarmNum; count++)
                {
                    palarmEndpointTemp = palarmEndpoint + count;
                    if (!strcmp(palarmEndpointTemp->devid, device_id))
                    {
                        memcpy(palarm_endpoint, palarmEndpointTemp, sizeof(ALARM_ENDPOINT_T));
                        return count+1;
                    }
                }
            }
            return 0;
        }
        else
        {
            return kGeneralErr;
        }
    }
}
/********************************************************
 * function: storage_get_alarm_endpoint_attr
 * description:  save endpoint
 * input:   1. device_id:
 *          2. key_code
 * output:  1. alarm_attr
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_alarm_endpoint_attr( char *device_id,  char *key_code, PDEV_FUNCTION_OBJ_T alarm_attr )
{
    uint32_t alarmNum = 0;
    uint32_t alarmCount = 0;
    int ret = 0;
    PALARM_ENDPOINT_T palarmCurrent = NULL;
    PALARM_ENDPOINT_T palarmTemp = NULL;
    if (alarm_attr == NULL)
    {
        os_storage_log("alarm_attr is NULL!!!");
        return kGeneralErr;
    }
    os_storage_log("device_id: %s, key_code: %s", device_id, key_code);
    storage_read_dev_alarm_status( &alarmNum, &palarmCurrent );
    for (alarmCount=0; alarmCount < alarmNum; alarmCount++)
    {
        palarmTemp = palarmCurrent + alarmCount;
        os_storage_log("palarmTemp: 0x%x, devid is %s", palarmTemp, palarmTemp->devid);
        if (!strcmp(palarmTemp->devid, device_id))
        {
            strcpy(alarm_attr->key, key_code);
            alarm_attr->type = 3;
            alarm_attr->valtype = 0;
            if (!strcmp(key_code, KEY_ALARM_STATE))
            {
                sprintf(alarm_attr->value, "%d", palarmTemp->alarmType);
            }
            else if (!strcmp(key_code, KEY_ALARM_DELAY))
            {
                sprintf(alarm_attr->value, "%d", palarmTemp->alarmDelay);
            }
            break;
        }
    }
    if ( alarmCount >= alarmNum )
    {
        ret = kGeneralErr;
    }
    else
    {
        ret = kNoErr;
    }
    os_storage_log("ret value is %d", ret);
    return ret;
}


/********************************************************
 * function: storage_del_alarm_dev_endpoint
 * description:
 * input:   1. devid:  point to a structure that we want to del
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_clear_alarm_dev_endpoint( void )
{
    uint32_t alarmDevNum = 0;
    OSStatus ret = 0;
    PALARM_ENDPOINT_T pAlarmDevice;
    ret = storage_read_dev_alarm_status(&alarmDevNum, &pAlarmDevice);
    if (ret == kNoErr )
    {
        if (alarmDevNum != 0)
        {
            memset(pAlarmDevice, 0, alarmDevNum*sizeof(ALARM_ENDPOINT_T));
            mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
            ret = storage_write_dev_alarm_state(alarmDevNum, pAlarmDevice);
        }
        else
        {
            ret = kNoErr;
        }
    }
    return ret;
}

/********************************************************
 * function: storage_del_alarm_dev_endpoint
 * description:
 * input:   1. devid:  point to a structure that we want to del
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_alarm_dev_endpoint( char *devid )
{
    uint32_t alarmDevNum = 0;
    OSStatus ret = 0;
    PALARM_ENDPOINT_T pAlarmDevice;
    ret = storage_read_dev_alarm_status(&alarmDevNum, &pAlarmDevice);
    if (ret == kNoErr)
    {
        uint32_t count = 0;
        PALARM_ENDPOINT_T alarmTemp = NULL;
        for (count=0; count < alarmDevNum; count++)
        {
            alarmTemp = pAlarmDevice + count;
            if (!strcmp(alarmTemp->devid, devid))
            {
                if (count < (alarmDevNum-1))
                {
                    memcpy(alarmTemp, pAlarmDevice+count+1, (alarmDevNum-count-1)*sizeof(ALARM_ENDPOINT_T));
                }
                memset(pAlarmDevice+(alarmDevNum-1), 0, sizeof(ALARM_ENDPOINT_T));
                mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
                storage_write_dev_alarm_state(alarmDevNum-1, pAlarmDevice);
                return kNoErr;
            }
        }
    }
    return kNotFoundErr;
}


/********************************************************
 * function: storage_del_devinfo_obj
 * description:
 * input:   1. pDevInfo:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_device_obj( char *objid )
{
    uint32_t structSize = sizeof(DEVICEOBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_DEVICE_OFFSET;
    uint32_t *objNumTotal = &g_deviceNum;

    if ((NULL == objid) || (0 == objid[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj(objid, startAddr, objNumTotal, structSize);
    }
    return ret;
}

/********************************************************
 * function: storage_del_devinfo_obj
 * description:
 * input:   1. pDevInfo:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_room_obj(uint8_t *room_id)
{
    uint32_t structSize = sizeof(ROOMOBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_ROOM_OFFSET;
    uint32_t *objNumTotal = &g_roomNum;//storage_get_object_num(ROOM_OBJ_ID);

    if ((NULL == room_id) || (0 == room_id[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj(room_id, startAddr, objNumTotal, structSize);
    }
    return ret;
}

/********************************************************
 * function: storage_del_devinfo_obj
 * description:
 * input:   1. pDevInfo:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_scene_obj(uint8_t *scene_id)
{
    uint32_t structSize = sizeof(SCENEOBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_SCENE_OFFSET;
    uint32_t *objNumTotal = &g_sceneNum;//storage_get_object_num(SCENE_OBJ_ID);

    if ((NULL == scene_id) || (0 == scene_id[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj(scene_id, startAddr, objNumTotal, structSize);
    }
    return ret;
}

/********************************************************
 * function: storage_del_event_obj
 * description:
 * input:   1. pDevInfo:  point to a structure that we want to save
 * output:
 * return:
 * auther:   chenb
 * other:   if we can't find the object, we will think that it's failed to delete
*********************************************************/
OSStatus storage_del_event_obj(char *event_id)
{
    uint32_t structSize = sizeof(EVENTOBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_EVENT_OFFSET;
    uint32_t *objNumTotal = &g_eventNum;//storage_get_object_num(EVENT_OBJ_ID);

    if ((NULL == event_id) || (0 == event_id[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj(event_id, startAddr, objNumTotal, structSize);
    }
    return ret;
}

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: storage_del_dev_endpoints_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_dev_endpoints_obj(char *fid)
{
    uint32_t structSize = sizeof(ENDPOINTOBJ_EX_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_ENDPOINT_OFFSET;
    uint32_t *objNumTotal = &g_endpointNum;//storage_get_object_num(ENDPOINT_OBJ_ID);
    os_storage_log("del device fid : %s", fid);

    if ((NULL == fid) || (0 == fid[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
        storage_clear_alarm_dev_endpoint();
    }
    else
    {
        // 删除时申请的空间较大有可能导致设备异常，改调用storage_del_obj_ex函数
        ret = storage_del_obj_ex(fid, startAddr, objNumTotal, structSize);
//        ret = storage_del_obj(fid, startAddr, objNumTotal, structSize);
        storage_del_alarm_dev_endpoint(fid);
    }

    return ret;
}

/********************************************************
 * function: storage_del_endpoint_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_endpoint_obj( char *endpoint_id )
{
    ENDPOINT_ELE_T endpointEle = {0};
    ENDPOINTOBJ_EX_T devEndpointsObj = {0};
    int ret = 0;
    mlink_parse_endpointid(endpoint_id, &endpointEle);
    ret = storage_read_endpoint_obj(endpointEle.devid, &devEndpointsObj);
    if (ret != kGeneralErr)
    {
        uint8_t count = 0;
        for (count=0; count<devEndpointsObj.endpointNum; count++)
        {
            if (!strcmp(endpoint_id, devEndpointsObj.endpointlist[count].id))
            {
                if (count >= (devEndpointsObj.endpointNum-1))
                {
                    memset(&devEndpointsObj.endpointlist[count], 0, sizeof(ENDPOINT_EX_T));
                }
                else
                {
                    memcpy(&devEndpointsObj.endpointlist[count], &devEndpointsObj.endpointlist[count+1], (devEndpointsObj.endpointNum-count)*sizeof(ENDPOINT_EX_T));
                }
                devEndpointsObj.endpointNum --;
                ret = storage_write_endpoint_obj(1, ret-1, &devEndpointsObj);
                break;
            }
        }

    }
    return kNoErr;
}

#else
/********************************************************
 * function: storage_del_dev_endpoints_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_dev_endpoints_obj(char *fid)
{
    uint32_t structSize = sizeof(SINGLE_DEV_ENDPOINTOBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_ENDPOINT_OFFSET;
    uint32_t *objNumTotal = &g_endpointNum;//storage_get_object_num(ENDPOINT_OBJ_ID);

    if ((NULL == fid) || (0 == fid[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
        storage_clear_alarm_dev_endpoint();
    }
    else
    {
        // 删除时申请的空间较大有可能导致设备异常，改调用storage_del_obj_ex函数
        ret = storage_del_obj_ex(fid, startAddr, objNumTotal, structSize);
//        ret = storage_del_obj(fid, startAddr, objNumTotal, structSize);
        storage_del_alarm_dev_endpoint(fid);
    }

    return ret;
}

/********************************************************
 * function: storage_del_endpoint_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_endpoint_obj( char *endpoint_id )
{
    ENDPOINT_ELE_T endpointEle = {0};
    SINGLE_DEV_ENDPOINTOBJ_T devEndpointsObj = {0};
    int ret = 0;
    mlink_parse_endpointid(endpoint_id, &endpointEle);
    ret = storage_read_endpoint_obj(endpointEle.devid, &devEndpointsObj);
    if (ret != kGeneralErr)
    {
        uint8_t count = 0;
        for (count=0; count<devEndpointsObj.endpointNum; count++)
        {
            if (!strcmp(endpoint_id, devEndpointsObj.endpointlist[count].id))
            {
                if (count >= (devEndpointsObj.endpointNum-1))
                {
                    memset(&devEndpointsObj.endpointlist[count], 0, sizeof(ENDPOINT_T));
                }
                else
                {
                    memcpy(&devEndpointsObj.endpointlist[count], &devEndpointsObj.endpointlist[count+1], (devEndpointsObj.endpointNum-count)*sizeof(ENDPOINT_T));
                }
                devEndpointsObj.endpointNum --;
                ret = storage_write_endpoint_obj(1, ret-1, &devEndpointsObj);
                break;
            }
        }

    }
    return kNoErr;
}

#endif

/********************************************************
 * function: storage_del_linkage_input_obj
 * description:   read some info of devices from initial address
 * input:   1. pRoomObj: point to a structure that we want to save
 *          2. num:  the number of object you want to read
 *          3. startIndex: tell us that operation will excute from which index
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_linkage_input_obj(unsigned char *fid)
{
    uint32_t structSize = sizeof(LINKAGE_INPUT_TASK_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_LINKAGE_INPUT_OFFSET;
    uint32_t *objNumTotal = &g_linkageInputNum;

    if ((NULL == fid) || (0 == fid[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
        main_del_linkage_status_info(NULL);
    }
    else
    {
        main_del_linkage_status_info(fid);
        ret = storage_del_obj_ex(fid, startAddr, objNumTotal, structSize);
    }

    return ret;
}

/********************************************************
 * function: storage_del_linkage_output_obj
 * description:   read some info of devices from initial address
 * input:   1. fid:

 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_linkage_output_obj( char *fid )
{
    uint32_t structSize = sizeof(LINKAGE_OUTPUT_TASK_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_LINKAGE_OUTPUT_OFFSET;
    uint32_t *objNumTotal = &g_linkageOutputNum;

    if ((NULL == fid) || (0 == fid[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj_ex(fid, startAddr, objNumTotal, structSize);
    }

    return ret;
}

/********************************************************
 * function: storage_del_endpointlink_obj
 * description:
 * input:   1. inputid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_endpointlink_obj(unsigned char *link_id)
{
    uint32_t structSize = sizeof(ENDPOINTLINK_OBJ_T);
    OSStatus ret = 0;
    uint32_t startAddr = FLASH_USER_ENDPOINTLINK_OFFSET;
    uint32_t *objNumTotal = &g_endpointLinkNum;//storage_get_object_num(ENDPOINTLINK_OBJ_ID);

    if ((NULL == link_id) || (0 == link_id[0]))
    {
        ret = storage_clear_obj(startAddr, objNumTotal, structSize);
    }
    else
    {
        ret = storage_del_obj_ex(link_id, startAddr, objNumTotal, structSize);
    }
    return ret;
}


/********************************************************
 * function: storage_del_device_status_obj
 * description:
 * input:   1. inputid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_del_device_status_obj(char *device_id)
{
    if ((NULL == device_id) || (0 == device_id[0]))
    {
        memset(&g_devStatusObj[1], 0, g_deviceNum*(sizeof(DEVSTATUSOBJ_T)));
    }
    else
    {
        uint32_t devIndex = 0;
        for ( devIndex=1; devIndex <= g_deviceNum; devIndex++ )
        {
            if (!strcmp(device_id, g_devStatusObj[devIndex].devid))
            {
                memcpy(g_devStatusObj+devIndex, g_devStatusObj+devIndex+1, (g_deviceNum-devIndex)*sizeof(DEVSTATUSOBJ_T));
                memset(g_devStatusObj+g_deviceNum, 0, sizeof(DEVSTATUSOBJ_T));
            }

        }
    }
    return kNoErr;
}


/********************************************************
 * function: storage_del_device
 * description:
 * input:   1. device_id:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_del_device(char *device_id)
{
    storage_del_device_status_obj(device_id);
    storage_del_device_obj(device_id);
    storage_del_dev_endpoints_status(device_id);
    storage_del_dev_endpoints_obj(device_id);
}



///********************************************************
// * function: storage_search_devobj_devid
// * description:
// * input:   1. deviceId: which we want to search
// * output:  2. pDevInfo:
// * return:
// * auther:   chenb
// * other:
//*********************************************************/
//OSStatus storage_search_devobj_devid(char *deviceId, PDEVICEOBJ_T pDevObj)
//{
//    return storage_read_dev_obj( deviceId, pDevObj );
//}

/********************************************************
 * function: storage_search_devobj_devid
 * description:
 * input:   1. deviceId: which we want to search
 * output:  2. pDevInfo:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_search_dev_alarm(char *device_id, PALARM_ENDPOINT_T palarm_endpoint)
{
    PALARM_ENDPOINT_T alarmEndpointHead = NULL;
    PALARM_ENDPOINT_T alarmEndpointTemp = NULL;
    uint32_t alarmNum = 0;
    uint32_t count = 0;
    if ((palarm_endpoint == NULL) || (device_id == NULL))
    {
        return kGeneralErr;
    }
    storage_read_dev_alarm_status(&alarmNum, &alarmEndpointHead);
//    os_storage_log("alarmNum: %d", alarmNum);
    for (count =0; count<alarmNum; count++)
    {
        alarmEndpointTemp = alarmEndpointHead+count;
//        os_storage_log("%d: devid is %s", count, alarmEndpointTemp->devid);

        if (!strcmp(alarmEndpointTemp->devid, device_id))
        {
            memcpy(palarm_endpoint, alarmEndpointTemp, sizeof(ALARM_ENDPOINT_T));
            break;
        }
    }
    os_storage_log("alarmNum: %d, count: %d: devid is %s <> %s", alarmNum, count, alarmEndpointTemp->devid, device_id);
    if (count >= alarmNum)
    {
        return kGeneralErr;
    }
    else
    {
        return count;
    }
}


/********************************************************
 * function: storage_get_dev_status
 * description:
 * input:   1. devid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int storage_get_dev_status( char *devid, PDEVSTATUSOBJ_T *pdev_state_obj )
{
    uint8_t index = 0;
    for (index = 0; index < g_deviceNum; index ++)
    {
        if (0 == strcmp(devid, g_devStatusObj[index+1].devid))
        {
            if (*pdev_state_obj == NULL)
            {
                *pdev_state_obj = g_devStatusObj+index+1;
            }
            else
            {
                memcpy(*pdev_state_obj, g_devStatusObj+index+1, sizeof(DEVSTATUSOBJ_T));
            }
            return index+1;
        }
    }
//    os_storage_log("status index is %d, devid: %s", index, devid);
    return 0;
}

/********************************************************
 * function: storage_update_dev_status
 * description:
 * input:   1. devid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
static void storage_update_dev_status_info( uint8_t number, PDEVSTATUSOBJ_T pdev_state_obj )
{
    if (pdev_state_obj != NULL)
    {
//        os_storage_log("pdev_state_obj: %s, deviceid: %s", pdev_state_obj->devid, g_devStatusObj[number].devid);
        memcpy(&g_devStatusObj[number], pdev_state_obj, sizeof(DEVSTATUSOBJ_T));
    }
}

/********************************************************
 * function: storage_update_dev_status
 * description:
 * input:   1. devid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_update_dev_status( char *devid, uint32_t status )
{
    PDEVSTATUSOBJ_T pdevStatus = NULL;
    DEVSTATUSOBJ_T devStatus = {0};
    int ret = 0;
    pdevStatus = &devStatus;
    ret = storage_get_dev_status(devid, &pdevStatus);
    if (ret != 0)
    {
        pdevStatus->restTime = pdevStatus->liveTime * 2;         // 4 hour
        pdevStatus->status.statusInfo = status;
        storage_update_dev_status_info(ret, pdevStatus);
    }
}

/********************************************************
 * function: storage_set_dev_online_state
 * description:
 * input:   1. devid:
 *          2. online: 1 表示在线   0表示离线
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_set_dev_online_state( char *devid, uint8_t online )
{
    DEVSTATUSOBJ_T devStatus = {0};
    PDEVSTATUSOBJ_T pdevStatus = NULL;
    pdevStatus = &devStatus;
    int ret =0;
//    os_storage_log("devid: %s, online: %d", devid, online);
    ret = storage_get_dev_status(devid, &pdevStatus);
    if (ret != 0)
    {
        devStatus.restTime = devStatus.liveTime*2;         // 4 hour
        devStatus.status.status_info.devStatus = online;
        storage_update_dev_status_info(ret, pdevStatus);
    }
}

/********************************************************
 * function: storage_get_dev_status_starting
 * description:
 * input:   1. number:
 * output:  2. pdev_status
 * return:  pdev_status 所指向的设备状态总数量
 * auther:   chenb
 * other:
*********************************************************/
int storage_get_dev_status_starting( uint8_t number, PDEVSTATUSOBJ_T *pdev_status)
{
    *pdev_status = g_devStatusObj + number;
    return g_deviceNum-number;
}

/********************************************************
 * function: storage_get_sipdev_obj
 * description:
 * input:   1. devid:
 * output:  1. pSimpleDeviceObj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_sipdev_obj(char *devid, PSIMPLEDEVICEOBJ_T pSimpleDeviceObj)
{
    if ((NULL == devid) || (NULL == pSimpleDeviceObj))
    {
        return kGeneralErr;
    }
    DEVICEOBJ_T devObj = {0};
    PDEVSTATUSOBJ_T pdevStatus = NULL;
    OSStatus ret = 0;
    ret = storage_read_dev_obj(devid, &devObj);
    pSimpleDeviceObj->comm = devObj.comm;
    pSimpleDeviceObj->devtype = devObj.devtype;
    memcpy(pSimpleDeviceObj->name, devObj.name, strlen(devObj.name));
    memcpy(pSimpleDeviceObj->sid, devObj.deviceId, strlen(devObj.deviceId));
    storage_get_dev_status(devObj.deviceId, &pdevStatus);
    pSimpleDeviceObj->status.statusInfo = pdevStatus->status.statusInfo;

    os_storage_log("sip devid is %s", devObj.deviceId);
    return ret;
}

/********************************************************
 * function: storage_get_sipdev_obj_batch
 * description:
 * input:   1. obj_num:
 *          2. start_index
 * output:  1. pSimpleDevObj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus storage_get_sipdev_obj_batch(uint8_t obj_num, uint8_t start_index, PSIMPLEDEVICEOBJ_T pSimpleDevObj)
{
    if (NULL == pSimpleDevObj)
    {
        os_storage_log("pSimpleDevObj is NULL !!!");
        return kGeneralErr;
    }

    PDEVICEOBJ_T pdevObj = (PDEVICEOBJ_T)malloc(obj_num * sizeof(DEVICEOBJ_T));
    PSIMPLEDEVICEOBJ_T pSimpleDevObjTemp = pSimpleDevObj;
    PDEVSTATUSOBJ_T pdevStatus = NULL;
    uint8_t devIndex = 0;
    OSStatus ret = 0;
    PDEVICEOBJ_T pdevObjHead = NULL;

    pdevObjHead = pdevObj;
    os_storage_log("obj_num: %d, start_index: %d", obj_num, start_index);
    memset(pdevObj, 0, obj_num * sizeof(DEVICEOBJ_T));
    ret = storage_read_dev_obj_batch(obj_num, start_index, pdevObj);
    storage_get_dev_status_starting( start_index+1, &pdevStatus );
    for (devIndex=0; devIndex < obj_num; devIndex++, pSimpleDevObjTemp++, pdevStatus++, pdevObj++)
    {
        pSimpleDevObjTemp->comm = pdevObj->comm;
        pSimpleDevObjTemp->devtype = pdevObj->devtype;
        memcpy(pSimpleDevObjTemp->name, pdevObj->name, strlen(pdevObj->name));
        memcpy(pSimpleDevObjTemp->sid, pdevObj->deviceId, strlen(pdevObj->deviceId));
        pSimpleDevObjTemp->status.statusInfo = pdevStatus->status.statusInfo;
        os_storage_log("pdevObj->deviceId: %s", pSimpleDevObjTemp->sid);
    }

    if (NULL != pdevObjHead)
    {
        free(pdevObjHead);
        pdevObj = NULL;
    }
    return ret;
}


/********************************************************
 * function: storage_get_dev_status_batch
 * description:
 * input:   1. devid:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_get_dev_status_batch(uint8_t dev_num, uint8_t start_index, PDEVSTATUSOBJ_T pdev_status)
{
    if ( pdev_status == NULL )
    {
        return;
    }
    memcpy(pdev_status, g_devStatusObj+start_index+1, dev_num*(sizeof(DEVSTATUSOBJ_T)));
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
void storage_init_device_status( void )
{
    NETDEVOBJ_T netdevObj = { 0 };
    DEVICEOBJ_T deviceObj = {0};
    uint8_t index = 0;
    memset(g_devStatusObj, 0, sizeof(g_devStatusObj));
    storage_read_local_devobj(&netdevObj);
    memcpy(g_devStatusObj[0].devid, netdevObj.uuid, sizeof(g_devStatusObj[0].devid));
    g_devStatusObj[0].status.statusInfo = 0;
    if (g_devStatusObj[0].devid[0] == 0)
    {
        g_devStatusObj[0].status.status_info.devStatus = 0xff;
    }

    // value device's status
    for (index = 0; index < g_deviceNum; index++)
    {
        storage_read_dev_obj_batch(1, index, &deviceObj);
        memcpy(g_devStatusObj[index+1].devid, deviceObj.deviceId, sizeof(deviceObj.deviceId));
        g_devStatusObj[index+1].status.statusInfo = 0;
        g_devStatusObj[index+1].liveTime = deviceObj.offline;
        g_devStatusObj[index+1].restTime = g_devStatusObj[index+1].liveTime;
    }
}

/********************************************************
 * function: storage_get_object_num
 * description:  server callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus storage_get_object_num(int classid)
{
    int objectNum = 0;
    switch (classid)
    {
        case DEVICE_OBJ_ID:
        case SIPDEV_OBJ_ID:
        case DEVSTATUS_OBJ_ID:
            objectNum = g_deviceNum;
            break;
        case SCENE_OBJ_ID:
            objectNum = g_sceneNum;
            break;
        case ROOM_OBJ_ID:
            objectNum = g_roomNum;
            break;
        case SCHEDULE_OBJ_ID:
            objectNum = g_scheduleNum;
            break;
        case EVENT_OBJ_ID:
            objectNum = g_eventNum;
            break;
        case ENDPOINT_OBJ_ID:
            objectNum = g_endpointNum;
            break;
        case ENDPOINTLINK_OBJ_ID:
            objectNum = g_endpointLinkNum;
            break;
        case CLOUDPARAM_OBJ_ID:
        case NETDEV_OBJ_ID:
        case DEVINFO_OBJ_ID:
            objectNum = 1;
            break;
        case LINKAGE_INPUT_OBJ_ID:
            objectNum = g_linkageInputNum;
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            objectNum = g_linkageOutputNum;
            break;
        default:
            objectNum = 0;
            break;
    }
    return objectNum;
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
void storage_init_status( void )
{
    storage_init_endpoint_status();
    storage_init_device_status();
}

/********************************************************
 * function: init_sub_device_info
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void storage_init_scene_obj(void)
{
    SCENEOBJ_T sceneObjTmp = {0};
    storage_read_scene_obj_batch(1, 0, &sceneObjTmp);
    if ((sceneObjTmp.sceneId[0] == 0) || ((uint8_t)sceneObjTmp.sceneId[0] >= 0x7f))
    {
        SCENEOBJ_T sceneObjArray[7] = {0};
        sceneObjArray[0].sceneId[0] = '1';
        strcpy(sceneObjArray[0].sceneName, "外出");
        sceneObjArray[1].sceneId[0] = '2';
        strcpy(sceneObjArray[1].sceneName, "在家");
        sceneObjArray[2].sceneId[0] = '3';
        strcpy(sceneObjArray[2].sceneName, "夜间");
        sceneObjArray[3].sceneId[0] = '4';
        strcpy(sceneObjArray[3].sceneName, "会客");
        sceneObjArray[4].sceneId[0] = '5';
        strcpy(sceneObjArray[4].sceneName, "就餐");
        sceneObjArray[5].sceneId[0] = '6';
        strcpy(sceneObjArray[5].sceneName, "节电");
        sceneObjArray[6].sceneId[0] = '7';
        strcpy(sceneObjArray[6].sceneName, "普通");
        storage_write_scene_obj(OBJECT_UPDATE_ADD, 7, sceneObjArray);
    }
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
void storage_init_local_dev_info( PNETDEVOBJ_T net_dev_obj )
{
    if (net_dev_obj != NULL)
    {
        uint32_t srandNo = 0;
        uint8_t randVal = 0;
        memset(net_dev_obj, 0, sizeof(NETDEVOBJ_T));
        IPStatusTypedef ipPara;

        micoWlanGetIPStatus(&ipPara, Station);
        strcpy(net_dev_obj->mac, ipPara.mac);
        for (uint8_t count=0; count<12; count++)
        {
            // change lower case to upper case example 'a'->'A'
            if ((net_dev_obj->mac[count]<='f') && (net_dev_obj->mac[count]>='a'))
            {
                net_dev_obj->mac[count] -= 0x20;
            }
        }
        net_dev_obj->devtype = GATEWAY_TYPE;
        net_dev_obj->status.status_info.devStatus = 0xff;          // Default unregister (1B)(online / offline / unregister)
        StrToHex(&net_dev_obj->mac[4], 8, (uint8_t *)&srandNo);
        srand(srandNo);
        randVal = rand( ) % 255 + 1;
        sprintf( net_dev_obj->addr, "%02X", randVal );

        //memcpy(net_dev_obj->uuid, net_dev_obj->mac, MAC_SIZE);
        memcpy( net_dev_obj->uuid, DEV_ID, strlen( DEV_ID ) );
        memcpy( net_dev_obj->deviceId, DEV_ID, strlen( DEV_ID ) );
        os_storage_log("net_dev_obj->uuid : %s net_dev_obj->deviceId : %s",net_dev_obj->uuid,net_dev_obj->deviceId);
        memcpy( net_dev_obj->modelid, MODEL_ID, strlen( MODEL_ID ) );
        memcpy( net_dev_obj->name, DEV_MODEL_NAME, strlen( DEV_MODEL_NAME ) );
        memcpy(net_dev_obj->modelname, YL_MODELNAME, strlen(YL_MODELNAME));

        strcpy(net_dev_obj->appid, APPID_DEFAULT);
        strcpy(net_dev_obj->ask, APP_SECRET);
        strcpy(net_dev_obj->cloudhost, CLOUD_HOST);
        strcpy(net_dev_obj->dsk, DEVICE_SECRET_DEFAULT);
        strcpy(net_dev_obj->psk, PSK_DEFAULT);
    }
}

/********************************************************
 * function: storage_clear_data_block
 * description:  还原数据块数据为出厂默认
 * input:   1. page_size:
 *          2. current_page:  从 1 开始
 * output:  1. pendpointlink_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void storage_clear_data_block()
{
    storage_write_dev_alarm_state(0, NULL);
    storage_factory_flash();
    g_sceneNum = 0;
    g_eventNum = 0;
    storage_init_event_obj();
    storage_init_scene_obj();
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
void storage_init_local_ver_info( void )
{
    DEVINFOOBJ_T devInfo = {0};
    storage_read_devinfo_obj(&devInfo);
    if (((uint8_t)devInfo.ver[0] > 0x7f) || (devInfo.ver[0] == (char)0))
    {
        memset(&devInfo, 0, sizeof(DEVINFOOBJ_T));
        sprintf(devInfo.ver, "%s", SOFT_VER);
        storage_write_devinfo_obj(&devInfo);
    }
    else if (0>strcmp(devInfo.ver, SOFT_VER))
    {
        strcpy(devInfo.ver, SOFT_VER);
        storage_write_devinfo_obj(&devInfo);
    }
}

