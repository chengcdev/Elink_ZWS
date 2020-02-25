/*
 * main_server_manage.c
 *
 *  Created on: 2017.12.16
 *      Author: Administrator
 */

#include "coap.h"
#include "mico.h"
#include "MLinkAppDef.h"
#include "../flash/flash.h"
#include "../coap/ml_coap.h"
#include "../coap/coap_service.h"
#include "../flash/flash_storage_object.h"
#include "MLinkObject.h"
#include "MLinkCommand.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../json/json_manage_deal.h"
#include "main_logic.h"

#define os_server_manage_log(M, ...) custom_log("MAIN_SERVER_MANAGE", M, ##__VA_ARGS__)

static char *   st_structdata = NULL;//malloc(1500); //改成此方式，降低死机概率
#define MAX_STRUCTDATA_BUFFER_LEN       2176
///********************************************************
// * function: mlcoap_server_manage_get
// * description:
// * input:
// * output:
// * return:   kNoErr/kGeneralErr
// * auther:
// * other:
//*********************************************************/
//static OSStatus mlcoap_server_update_endpoint_distribute( uint8_t dev_num, PENDPOINTOBJ_EX_T pendpoint_objs )
//{
//    uint8_t count = 0;
//    uint8_t endpointCount = 0;
//    PENDPOINTOBJ_EX_T pEndpointObjTemp = NULL;
//    ENDPOINT_ELE_T endpointEle = {0};
//    int ret = 0;
//    for (count=0; count<dev_num; count++)
//    {
//        pEndpointObjTemp = pendpoint_objs + count;
//        for (endpointCount=0; endpointCount<pEndpointObjTemp->endpointNum; endpointCount++)
//        {
//            mlink_parse_endpointid(pEndpointObjTemp->endpointlist[endpointCount].endpoint.id, &endpointEle);
//            if (!strcmp(endpointEle.key, KEY_ALARM_STATE))
//            {
//                alarm_statechange_deal( endpointEle.devid, pEndpointObjTemp->endpointlist[0].value );
//            }
//        }
//    }
//    ret = storage_update_endpoint_obj(dev_num, pendpoint_objs);
//    return ret;
//}
extern int main_linkage_cron_deal(char *id, void *task_info, unsigned int info_len);


/********************************************************
 * function: sercive_manage_linkage_cron_deal
 * description:
 * input:   1. flag:  0: add task, 1: delete task
 *          2. input_task:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus servive_manage_linkage_cron_deal(uint8_t flag, PLINKAGE_INPUT_TASK_T input_task)
{
    int count = 0;
    char *logicParam[4] = {NULL, NULL, NULL, NULL};       // 分别用于指向 以下四个元素字符串： 1.输入端点ID; 2.判断条件; 3.值;  4.延时标识;
    uint8_t logicParamNum = 0;
    ENDPOINT_ELE_T endpointEle = {0};
    CRON_TASK_INFO cronTaskInfo = {0};
    for (count=0; count < input_task->inputNum; count++)
    {
        mlink_parse_space_data( input_task->lnkInput[count].logic, '|', logicParam, &logicParamNum );
        mlink_parse_endpointid(logicParam[0], &endpointEle);
        if (endpointEle.classId == SCHEDULE_OBJ_ID)
        {
            if (flag == 0)
            {
                cronTaskInfo.index = input_task->lnkInput[count].index;
                strcpy(cronTaskInfo.linkId, input_task->linkageId);
                os_server_manage_log("add schedule === cronid: %s, expression: %s", logicParam[0], logicParam[2]);
//                cron_add_task(logicParam[0], logicParam[2], main_linkage_cron_deal, (uint8_t *)&cronTaskInfo, sizeof(CRON_TASK_INFO));
            }
            else
            {
                os_server_manage_log("del schedule === cronid: %s", logicParam[0]);
//                cron_del_task(logicParam[0]);
            }
            os_server_manage_log("update cron success");
            return kNoErr;
        }
    }
    return kGeneralErr;
}

/********************************************************
 * function: mlcoap_server_manage_get
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_get(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = kNoErr;
    MANAGE_PARAM_GET_T manage_get_param;
    OBJECT_PAGE_INFO pageInfo;
    char *pObjData = NULL;

    int objectIndex = 0;                        // from 0

    memset(&pageInfo, 0, sizeof(OBJECT_PAGE_INFO));
    memset(&manage_get_param, 0, sizeof(MANAGE_PARAM_GET_T));
    manage_get_param.limit = 10;            // default value is 10
    manage_get_param.page = 1;              // default value is 1 page

    json_manage_unpack_get(data, &manage_get_param);

    // Set variable "pageInfo" and "objectIndex"
    if (manage_get_param.objid[0] == 0)
    {
        if (manage_get_param.limit == 0)
        {
            manage_get_param.limit = 10;
        }
        if (manage_get_param.page == 0)
        {
            manage_get_param.page = 1;
        }
        pageInfo.itemperpage = manage_get_param.limit;
        pageInfo.pageindex = manage_get_param.page;
        pageInfo.totalnum = storage_get_object_num(manage_get_param.classid);
        pageInfo.pagenum = mlink_get_total_pages(pageInfo.totalnum, pageInfo.itemperpage);
        pageInfo.itemsCurPage = mlink_get_curpage_size(pageInfo.totalnum, pageInfo.itemperpage, pageInfo.pageindex);
        objectIndex = (pageInfo.pageindex - 1)*pageInfo.itemperpage;
    }
    os_server_manage_log("classid: %d", manage_get_param.classid);
    os_server_manage_log("pageinfo: [%d %d %d %d %d %d] \n", pageInfo.itemperpage,pageInfo.pageindex,
                         pageInfo.totalnum,pageInfo.pagenum,pageInfo.itemsCurPage,
                         objectIndex );
    // Read object data frome flash
    switch (manage_get_param.classid)
    {
        case NETDEV_OBJ_ID:
            {
                PNETDEVOBJ_T pnetDevObj = malloc(sizeof(NETDEVOBJ_T));
                memset(pnetDevObj, 0, sizeof(NETDEVOBJ_T));
                storage_read_local_devobj(pnetDevObj);
                os_server_manage_log("mac: %s", pnetDevObj->mac);
                memset(manage_get_param.objid, 0, sizeof(manage_get_param.objid));
                memcpy(manage_get_param.objid, pnetDevObj->uuid, strlen(pnetDevObj->uuid));
                pObjData = (char *)pnetDevObj;
            }
            break;
        case DEVICE_OBJ_ID:
            {
                PDEVICEOBJ_T pdeviceObj = NULL;
//                    static uint32_t s_count = 0;
//                    s_count++;
//                    os_server_manage_log("get count is %d", s_count);
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pdeviceObj = (PDEVICEOBJ_T)malloc(sizeof(DEVICEOBJ_T));

                    ret = storage_read_dev_obj(manage_get_param.objid, pdeviceObj);
                }
                else
                {
                    pdeviceObj = (PDEVICEOBJ_T)malloc(pageInfo.itemsCurPage * sizeof(DEVICEOBJ_T));
                    memset(pdeviceObj, 0, pageInfo.itemsCurPage*sizeof(DEVICEOBJ_T));
                    ret = storage_read_dev_obj_batch(pageInfo.itemsCurPage, objectIndex, pdeviceObj);
                }
                pObjData = (char *)pdeviceObj;
            }
            break;
        case SIPDEV_OBJ_ID:
            {
                PSIMPLEDEVICEOBJ_T pSimpDevObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pSimpDevObj = (PSIMPLEDEVICEOBJ_T)malloc(sizeof(SIMPLEDEVICEOBJ_T));
                    memset(pSimpDevObj, 0, sizeof(SIMPLEDEVICEOBJ_T));
                    ret = storage_get_sipdev_obj(manage_get_param.objid, pSimpDevObj);
                }
                else
                {
                    pSimpDevObj = (PSIMPLEDEVICEOBJ_T)malloc(pageInfo.itemsCurPage * sizeof(SIMPLEDEVICEOBJ_T));
                    memset(pSimpDevObj, 0, pageInfo.itemsCurPage*sizeof(SIMPLEDEVICEOBJ_T));
                    ret = storage_get_sipdev_obj_batch(pageInfo.itemsCurPage, objectIndex, pSimpDevObj);
                }

                pObjData = (char *)pSimpDevObj;
            }
            break;

        case DEVSTATUS_OBJ_ID:
            {
                PDEVSTATUSOBJ_T pdevStatus = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pdevStatus = (PDEVSTATUSOBJ_T)malloc(sizeof(DEVSTATUSOBJ_T));
                    storage_get_dev_status(manage_get_param.objid, &pdevStatus);
                }
                else
                {
                    pdevStatus = (PDEVSTATUSOBJ_T)malloc(pageInfo.itemsCurPage * sizeof(DEVSTATUSOBJ_T));
                    memset(pdevStatus, 0, pageInfo.itemsCurPage*sizeof(DEVSTATUSOBJ_T));
                    storage_get_dev_status_batch( pageInfo.itemsCurPage, objectIndex, pdevStatus );
                }
                if (pdevStatus == NULL)
                {
                    ret = kGeneralErr;
                }
                pObjData = (char *)pdevStatus;
            }
            break;
        case SCENE_OBJ_ID:
            {
                PSCENEOBJ_T psceneObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    psceneObj = (PSCENEOBJ_T)malloc(sizeof(SCENEOBJ_T));
                    memset(psceneObj, 0, sizeof(SCENEOBJ_T));
                    ret = storage_read_scene_obj(manage_get_param.objid, psceneObj);
                }
                else
                {
                    psceneObj = malloc(pageInfo.itemsCurPage * sizeof(SCENEOBJ_T));
                    memset(psceneObj, 0, pageInfo.itemsCurPage*sizeof(SCENEOBJ_T));
                    ret = storage_read_scene_obj_batch(pageInfo.itemsCurPage, objectIndex, psceneObj);
                }
                pObjData = (char *)psceneObj;
            }
            break;
        case ROOM_OBJ_ID:
            {
                PROOMOBJ_T pRoomObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pRoomObj = (PROOMOBJ_T)malloc(sizeof(ROOMOBJ_T));
                    memset(pRoomObj, 0, sizeof(ROOMOBJ_T));
                    ret = storage_read_roomobj_id(manage_get_param.objid, pRoomObj);
                }
                else
                {
                    pRoomObj = malloc(pageInfo.itemsCurPage * sizeof(ROOMOBJ_T));
                    memset(pRoomObj, 0, pageInfo.itemsCurPage*sizeof(ROOMOBJ_T));
                    ret = storage_read_room_obj_batch(pageInfo.itemsCurPage, objectIndex, pRoomObj);
                }
                pObjData = (char *)pRoomObj;
            }
            break;
        case EVENT_OBJ_ID:
            {
                PEVENTOBJ_T pEventObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pEventObj = (PEVENTOBJ_T)malloc(sizeof(EVENTOBJ_T));
                    memset(pEventObj, 0, sizeof(EVENTOBJ_T));
                    ret = storage_read_event_obj(manage_get_param.objid, pEventObj);
                }
                else
                {
                    pEventObj = malloc(pageInfo.itemsCurPage * sizeof(EVENTOBJ_T));
                    memset(pEventObj, 0, pageInfo.itemsCurPage*sizeof(EVENTOBJ_T));
                    ret = storage_read_event_obj_batch(pageInfo.itemsCurPage, objectIndex, pEventObj);
                }
                pObjData = (char *)pEventObj;
            }
            break;
        case ENDPOINT_OBJ_ID:
            {
#ifdef VERSION_ENDPOINT_12
                PENDPOINTOBJ_EX_T pEndpointObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pEndpointObj = (PENDPOINTOBJ_EX_T)malloc(sizeof(ENDPOINTOBJ_EX_T));
                    ret = storage_read_endpoint_obj(manage_get_param.objid, pEndpointObj);
                }
                else
                {
                    pEndpointObj = (PENDPOINTOBJ_EX_T)malloc(pageInfo.itemsCurPage * sizeof(ENDPOINTOBJ_EX_T));
                    memset((char *)pEndpointObj, 0, pageInfo.itemsCurPage * sizeof(ENDPOINTOBJ_EX_T));
                    ret = storage_read_endpoint_obj_batch(pageInfo.itemsCurPage, objectIndex, pEndpointObj);
                }
                pObjData = (char *)pEndpointObj;
#else
                PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pEndpointObj = (PSINGLE_DEV_ENDPOINTOBJ_T)malloc(sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                    ret = storage_read_endpoint_obj(manage_get_param.objid, pEndpointObj);
                }
                else
                {
                    pEndpointObj = (PSINGLE_DEV_ENDPOINTOBJ_T)malloc(pageInfo.itemsCurPage * sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                    memset((char *)pEndpointObj, 0, pageInfo.itemsCurPage * sizeof(SINGLE_DEV_ENDPOINTOBJ_T));
                    ret = storage_read_endpoint_obj_batch(pageInfo.itemsCurPage, objectIndex, pEndpointObj);
                }
                pObjData = pEndpointObj;
#endif
            }

            break;
        case ENDPOINTLINK_OBJ_ID:
            {
                PENDPOINTLINK_OBJ_T pEndpointlinkObj = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pEndpointlinkObj = (PENDPOINTLINK_OBJ_T)malloc(sizeof(ENDPOINTLINK_OBJ_T));
                    ret = storage_get_endpointlink_by_linkid(manage_get_param.objid, pEndpointlinkObj);
                }
                else
                {
                    pEndpointlinkObj = (PENDPOINTLINK_OBJ_T)malloc(pageInfo.itemsCurPage * sizeof(ENDPOINTLINK_OBJ_T));
                    os_server_manage_log("pageInfo.itemsCurPage: %d", pageInfo.itemsCurPage);
                    memset((char *)pEndpointlinkObj, 0, pageInfo.itemsCurPage * sizeof(ENDPOINTLINK_OBJ_T));
                    ret = storage_read_endpointlink_obj_batch(pageInfo.itemsCurPage, objectIndex, pEndpointlinkObj);
                }
                pObjData = (char *)pEndpointlinkObj;
            }
            break;
        case CLOUDPARAM_OBJ_ID:
            {
                PCLOUDOBJ_T pCloudObj = NULL;
                pCloudObj = (PCLOUDOBJ_T)malloc(sizeof(CLOUDOBJ_T));
                ret = storage_read_cloud_obj( pCloudObj );
                pObjData = (char *)pCloudObj;
            }
            break;
        case DEVINFO_OBJ_ID:
            {
                PDEVINFOOBJ_T pDevInfo = NULL;
                pDevInfo = (PDEVINFOOBJ_T)malloc(sizeof(DEVINFOOBJ_T));
                ret = storage_read_devinfo_obj( pDevInfo );
                pObjData = (char *)pDevInfo;
            }
            break;
        case LINKAGE_INPUT_OBJ_ID:
            {
                PLINKAGE_INPUT_TASK_T pLinkageInput = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pLinkageInput = (PLINKAGE_INPUT_TASK_T)malloc(sizeof(LINKAGE_INPUT_TASK_T));
                    memset(pLinkageInput, 0, sizeof(LINKAGE_INPUT_TASK_T));
                    ret = storage_get_obj_by_id( manage_get_param.classid, manage_get_param.objid, pLinkageInput);
                }
                else
                {
                    pLinkageInput = malloc(pageInfo.itemsCurPage * sizeof(LINKAGE_INPUT_TASK_T));
                    memset(pLinkageInput, 0, pageInfo.itemsCurPage*sizeof(LINKAGE_INPUT_TASK_T));
                    ret = storage_read_linkage_input_obj(pageInfo.itemsCurPage, objectIndex, pLinkageInput);
                }
                pObjData = (char *)pLinkageInput;
            }
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            {
                PLINKAGE_OUTPUT_TASK_T pLinkageOutput = NULL;
                if (manage_get_param.objid[0] != 0)         // get one object appointed
                {
                    pLinkageOutput = (PLINKAGE_OUTPUT_TASK_T)malloc(sizeof(LINKAGE_OUTPUT_TASK_T));
                    memset(pLinkageOutput, 0, sizeof(LINKAGE_OUTPUT_TASK_T));
                    ret = storage_get_obj_by_id( manage_get_param.classid, manage_get_param.objid, pLinkageOutput);
                }
                else
                {
                    pLinkageOutput = malloc(pageInfo.itemsCurPage * sizeof(LINKAGE_OUTPUT_TASK_T));
                    memset(pLinkageOutput, 0, pageInfo.itemsCurPage*sizeof(LINKAGE_OUTPUT_TASK_T));
                    ret = storage_read_linkage_output_obj(pageInfo.itemsCurPage, objectIndex, pLinkageOutput);
                }
                pObjData = (char *)pLinkageOutput;

            }
            break;
        default:
            break;
    }

    // Packet json string with object data
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(2048);
        memset(packPacket->data, 0, 2048);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }
    os_server_manage_log("ret =  %d", ret );
    if (ret != kGeneralErr)
    {
        if (manage_get_param.objid[0] != 0)
        {
            json_manage_pack_resp_get(manage_get_param.classid, pObjData, NULL, packPacket->data);
        }
        else
        {
            json_manage_pack_resp_get(manage_get_param.classid, pObjData, &pageInfo, packPacket->data);
        }
    }
    else
    {
        json_pack_resp_errCode(COAP_MANAGER_NOTI_CMD_GET, MLINK_RESP_FIND_NOTHING, packPacket->data);
    }

    packPacket->datalen = strlen(packPacket->data);
    if (pObjData != NULL)
    {
        free(pObjData);
        pObjData = NULL;
    }

    return ret;
}

/********************************************************
 * function: mlcoap_server_manage_add
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_add(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    int classID = 0;
    char *structdata = NULL;
    OSStatus ret = 0;
    uint32_t errCode = 0;
//    structdata = malloc(2000);
    uint8_t objNum = 0;
    OBJECT_UPDATE_TYPE_E type = OBJECT_UPDATE_ADD;

    if (st_structdata==NULL)
        st_structdata = malloc(MAX_STRUCTDATA_BUFFER_LEN);

    structdata = st_structdata;

//    if (structdata == NULL)
//    {
//        return kGeneralErr;
//    }

    memset(structdata, 0, MAX_STRUCTDATA_BUFFER_LEN);
    json_manage_unpack_add(data, &classID, structdata, &objNum);
    os_server_manage_log("classId: %d, objNum: %d\r\n", classID, objNum);
    switch (classID)
    {
        case DEVICE_OBJ_ID:
            {
                PDEVICEOBJ_T pdata = (PDEVICEOBJ_T)structdata;
                ret = storage_add_device_obj( objNum, pdata );
//                if (ret != kGeneralErr)
//                {
//                    uint8_t meshState = 0;
//                    mlink_sys_get_status(SYS_MESH_STATE, &meshState);
//                    mlink_led_mesh_join();
//                }
            }
            break;
        case SCENE_OBJ_ID:
            ret = storage_add_scene_obj( objNum, (PSCENEOBJ_T)structdata);
            break;
        case ROOM_OBJ_ID:
            ret = storage_add_room_obj( objNum, (PROOMOBJ_T)structdata );
            break;
        case SCHEDULE_OBJ_ID:
//            ret = storage_write_schedule_obj(type, (PSCHEDULEOBJ_T)structdata, 0);
            break;
        case NETDEV_OBJ_ID:
        {
            mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
            ret = storage_add_local_obj((PNETDEVOBJ_T)structdata);
        }
            break;
        case ENDPOINT_OBJ_ID:
        {
            storage_add_endpoint_obj((PENDPOINTOBJ_EX_T)structdata);
        }
            break;
        case ENDPOINTLINK_OBJ_ID:
            ret = storage_add_endpointlink_obj(objNum, (PENDPOINTLINK_OBJ_T)structdata);
            break;
        case EVENT_OBJ_ID:
            ret = storage_write_event_obj(type, objNum, (PEVENTOBJ_T)structdata);
            break;
        case LINKAGE_INPUT_OBJ_ID:
        {
            ret = storage_add_linkage_input_obj( objNum, (PLINKAGE_INPUT_TASK_T)structdata );
        }
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            ret = storage_add_linkage_output_obj( objNum, (PLINKAGE_OUTPUT_TASK_T)structdata );
            break;
        default:
            break;
    }

//    if (structdata != NULL)
//    {
//        free(structdata);
//    }
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    if (packPacket->data != NULL)
    {
        memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
        os_server_manage_log("ret = %d", ret);
        if (ret != kGeneralErr)
        {
            errCode = MLINK_RESP_OK;
        }
        else
        {
            errCode = MLINK_RESP_ERR_PUBLIC;
        }

        json_pack_resp_errCode(COAP_MANAGER_NOTI_CMD_ADD, errCode, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_manage_del
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_del(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = 0;
    uint32_t errCode = 0;
    MANAGE_DEL_PARAM_T manage_del_param;
    memset(&manage_del_param, 0, sizeof(MANAGE_DEL_PARAM_T));
    json_manage_unpack_del(data, &manage_del_param);

    switch (manage_del_param.classid)
    {
        case DEVICE_OBJ_ID:
            storage_del_device(manage_del_param.objId);
            break;
        case SCENE_OBJ_ID:
            storage_del_scene_obj(manage_del_param.objId);
            break;
        case ROOM_OBJ_ID:
            storage_del_room_obj(manage_del_param.objId);
            break;
        case GROUP_OBJ_ID:

            break;

        case SCHEDULE_OBJ_ID:
            //storage_write_schedule_obj((PSCHEDULEOBJ_T)structdata, &devNum, 0);
            break;
        case EVENT_OBJ_ID:
            storage_del_event_obj(manage_del_param.objId);
            break;
        case ENDPOINT_OBJ_ID:
        {
            char objId[OBJID_SIZE] = {0};
            char *element[3] = {NULL, NULL, NULL};
            uint8_t num = 0;
            memcpy(objId, manage_del_param.objId, OBJID_SIZE);
            mlink_parse_space_data(objId, '_', element, &num);
            if (num == 1)
            {
                os_server_manage_log("del the endpoints of device %s.", manage_del_param.objId);
                storage_del_dev_endpoints_obj(manage_del_param.objId);
            }
            else
            {
                storage_del_endpoint_obj(manage_del_param.objId);
            }
        }
            break;
        case ENDPOINTLINK_OBJ_ID:
            storage_del_endpointlink_obj(manage_del_param.objId);
            break;
        case LINKAGE_INPUT_OBJ_ID:
        {
            LINKAGE_INPUT_TASK_T lnkInputObj = {0};
            int objIndex = 0;
            objIndex = storage_get_obj_by_id(LINKAGE_INPUT_OBJ_ID, manage_del_param.objId, &lnkInputObj);
            if (objIndex >= 0)
            {
                servive_manage_linkage_cron_deal(1, &lnkInputObj);
            }
            storage_del_linkage_input_obj(manage_del_param.objId);
        }
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            storage_del_linkage_output_obj(manage_del_param.objId);
            break;
        default:
            break;
    }

    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }

    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
    if (ret != kGeneralErr)
    {
        errCode = MLINK_RESP_OK;
    }
    else
    {
        errCode = MLINK_RESP_FIND_NOTHING;
    }

    json_pack_resp_errCode(COAP_MANAGER_NOTI_CMD_DEL, errCode, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_manage_update
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_update_bak(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    int classID = 0;
    char *structdata = NULL;
    OSStatus ret = 0;
    uint32_t errCode = 0;
    structdata = malloc(1500);
    uint8_t devNum = 0;
    if (structdata == NULL)
    {
        return kGeneralErr;
    }
    memset(structdata, 0, 1500);
    json_manage_unpack_update(data, &classID, structdata, &devNum);
    switch (classID)
    {
        case DEVICE_OBJ_ID:
            ret = storage_update_device_obj( devNum, (PDEVICEOBJ_T)structdata );
            break;
        case SCENE_OBJ_ID:
            ret = storage_update_scene_obj( devNum, (PSCENEOBJ_T)structdata );
            break;
        case ROOM_OBJ_ID:
            ret = storage_update_room_obj( devNum, (PROOMOBJ_T)structdata );
            break;
        case GROUP_OBJ_ID:

            break;
        case CLOUDPARAM_OBJ_ID:
            ret = storage_update_cloud_obj((PCLOUDOBJ_T)structdata);
            break;
        case LINKAGE_INPUT_OBJ_ID:
            ret = storage_update_linkage_input_obj(devNum, (PLINKAGE_INPUT_TASK_T)structdata);
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            ret = storage_update_linkage_output_obj(devNum, (PLINKAGE_OUTPUT_TASK_T)structdata);
            break;
        case SCHEDULE_OBJ_ID:
            ret = storage_update_schedule_obj((PSCHEDULEOBJ_T)structdata);
            break;
        case NETDEV_OBJ_ID:
            ret = storage_update_local_obj((PNETDEVOBJ_T)structdata);
            break;
        case ENDPOINT_OBJ_ID:
        {
            ret = storage_update_endpoint_obj(devNum, (PENDPOINTOBJ_EX_T)structdata);
        }
            break;
        case EVENT_OBJ_ID:
            ret = storage_update_event_obj( devNum, (PEVENTOBJ_T)structdata );
            break;
        case ENDPOINTLINK_OBJ_ID:
            ret = storage_update_endpointlink_obj(devNum, (PENDPOINTLINK_OBJ_T)structdata);
            break;
        default:
            break;
    }
    if (structdata != NULL)
    {
        free(structdata);
    }
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
    if (ret == kGeneralErr)
    {
        errCode = MLINK_RESP_FIND_NOTHING;
    }
    else
    {
        errCode = MLINK_RESP_OK;
    }

    json_pack_resp_errCode(COAP_MANAGER_NOTI_CMD_UPDATE, errCode, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);

    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_manage_update
 * description:
 * input:       1. data: json数据中的
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_update(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    int classID = 0;
    char *structdata = NULL;
    OSStatus ret = 0;
    uint32_t errCode = 0;
    if (st_structdata==NULL)
        st_structdata = malloc(MAX_STRUCTDATA_BUFFER_LEN);

    structdata = st_structdata;

    uint8_t objNum = 0;
//    if (structdata == NULL)
//    {
//        return kGeneralErr;
//    }
    memset(structdata, 0, MAX_STRUCTDATA_BUFFER_LEN);
    json_manage_unpack_update(data, &classID, structdata, &objNum);
    switch (classID)
    {
        case DEVICE_OBJ_ID:
            ret = storage_update_device_obj( objNum, (PDEVICEOBJ_T)structdata );
            break;
        case SCENE_OBJ_ID:
            ret = storage_update_scene_obj( objNum, (PSCENEOBJ_T)structdata );
            break;
        case ROOM_OBJ_ID:
            ret = storage_update_room_obj( objNum, (PROOMOBJ_T)structdata );
            break;
        case GROUP_OBJ_ID:

            break;
        case CLOUDPARAM_OBJ_ID:
            ret = storage_update_cloud_obj((PCLOUDOBJ_T)structdata);
            break;

        case SCHEDULE_OBJ_ID:
            ret = storage_update_schedule_obj((PSCHEDULEOBJ_T)structdata);
            break;
        case NETDEV_OBJ_ID:
        {
            PNETDEVOBJ_T pnetObj = (PNETDEVOBJ_T)structdata;
            mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
            ret = storage_update_local_obj((PNETDEVOBJ_T)structdata);

            msleep(200);
            MLinkBonjourInfoUpdate();

        }
            break;
        case ENDPOINT_OBJ_ID:
        {
            ret = storage_update_endpoint_obj(objNum, (PENDPOINTOBJ_EX_T)structdata);
        }
            break;
        case EVENT_OBJ_ID:
            ret = storage_update_event_obj( objNum, (PEVENTOBJ_T)structdata );
            break;
        case ENDPOINTLINK_OBJ_ID:
            ret = storage_update_endpointlink_obj(objNum, (PENDPOINTLINK_OBJ_T)structdata);
            break;
        case LINKAGE_INPUT_OBJ_ID:
            ret = storage_update_linkage_input_obj(objNum, (PLINKAGE_INPUT_TASK_T)structdata);
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            ret = storage_update_linkage_output_obj(objNum, (PLINKAGE_OUTPUT_TASK_T)structdata);
            break;
        default:
            break;
    }
//    if (structdata != NULL)
//    {
//        free(structdata);
//    }
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
    if (ret == kGeneralErr)
    {
        errCode = MLINK_RESP_FIND_NOTHING;
    }
    else
    {
        errCode = MLINK_RESP_OK;
    }

    json_pack_resp_errCode(COAP_MANAGER_NOTI_CMD_UPDATE, errCode, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);

    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_manage_clear
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_manage_clear(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    return kNoErr;
}


/********************************************************
 * function: mlcoap_server_manager_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_manager_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = kNoErr;

    os_server_manage_log(" mlcoap_server_manager_notify: cmd[%d] size[%d]\n", cmd, size);
    switch(cmd)
    {
        case COAP_MANAGER_NOTI_CMD_GET:
            {
                mlcoap_server_manage_get(data, size, packPacket);
            }
            break;
        case COAP_MANAGER_NOTI_CMD_ADD:
            {
//                mlcoap_server_manage_add(data, size, packPacket);
            }
            break;
        case COAP_MANAGER_NOTI_CMD_DEL:
            {
//                mlcoap_server_manage_del(data, size, packPacket);
            }
            break;
        case COAP_MANAGER_NOTI_CMD_CLEAR:
            {
//                mlcoap_server_manage_clear(data, size, packPacket);
            }
            break;
        case COAP_MANAGER_NOTI_CMD_UPDATE:
            {
                mlcoap_server_manage_update(data, size, packPacket);
            }
            break;

        case COAP_MANAGER_NOTI_CMD_ERR:
            {
                json_pack_resp_errCode(cmd, MLINK_RESP_ERROR, packPacket->data);
                packPacket->datalen = strlen(packPacket->data);
                packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
            }
            break;
    default:
        ret = kGeneralErr;
        break;
    }
    return ret;
}



