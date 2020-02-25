/*
 * json_manage_deal.c
 *
 *  Created on: 2017年8月1日
 *      Author: Administrator
 */


#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"
#include "json_manage_deal.h"

#define os_json_manage_log(M, ...) //custom_log("JSON_MANAGE", M, ##__VA_ARGS__)


#define OBJECT_MEMBER_NUM                       32

/********************************************************
 * function: json_manage_pack_resp_get
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_manage_pack_resp_get(OBJECT_ID_E objectId, char *pObjData, POBJECT_PAGE_INFO ppage_info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_item = NULL;
    struct json_object *json_object_member[OBJECT_MEMBER_NUM] = {0};
    char *jsonString = NULL;
    uint8_t objNum = 0;
    uint8_t index = 0;
    char pageInfoStr[32] = {0};

    memset(pageInfoStr, 0, sizeof(pageInfoStr));

    if (json_package == NULL || pObjData == NULL)
    {
        os_json_manage_log(" uuid or json_package is NULL!!!");
        return kGeneralErr;
    }


    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    json_object_item = json_object_new_array();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return kGeneralErr;
    }

    // package the base of command infomation contains command/errorcode/body
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_GET));
    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(0));
    json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectData);
    json_object_object_add(jsonObjectData, KEY_CLASSID, json_object_new_int(objectId));
    json_object_object_add(jsonObjectData, KEY_ITEM, json_object_item);

    if (ppage_info != NULL)
    {
        objNum = ppage_info->itemsCurPage;

        sprintf(pageInfoStr, "[%d|%d|%d|%d]", ppage_info->totalnum, ppage_info->itemperpage, ppage_info->pagenum, ppage_info->pageindex);
        json_object_object_add(jsonObjectData, KEY_PAGES, json_object_new_string(pageInfoStr));
    }
    else
    {
        objNum = 1;
    }

    if (objNum > OBJECT_MEMBER_NUM)
    {
        objNum = OBJECT_MEMBER_NUM;
    }
    for (index=0; index < objNum; index++)
    {
        json_object_member[index] = json_object_new_object();
    }

    // package data
    switch (objectId)
    {
        case NETDEV_OBJ_ID:
            {
                PNETDEVOBJ_T pObjDataTemp = (PNETDEVOBJ_T)pObjData;
                os_json_manage_log("the net device  total number is %d", objNum);
                for (index=0; index < objNum; index++)
                {
                    os_json_manage_log("the net device  total number is %d", objNum);
                    json_pack_netdev_obj((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case DEVICE_OBJ_ID:
            {
                PDEVICEOBJ_T pObjDataTemp = (PDEVICEOBJ_T)pObjData;

                for (index=0; index < objNum; index++)
                {
                    json_pack_device_obj((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case SIPDEV_OBJ_ID:
            {
                PSIMPLEDEVICEOBJ_T pSimpDevObj = (PSIMPLEDEVICEOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_simple_device_obj((pSimpDevObj+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case DEVSTATUS_OBJ_ID:
            {
                PDEVSTATUSOBJ_T pdevStatus = (PDEVSTATUSOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_device_status_obj((pdevStatus+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case SCENE_OBJ_ID:
            {
                PSCENEOBJ_T pSceneObjTemp = (PSCENEOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_scene_obj(pSceneObjTemp+index, json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case ROOM_OBJ_ID:
            {
                PROOMOBJ_T pRoomObjTemp = (PROOMOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_room_obj(pRoomObjTemp+index, json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case SCHEDULE_OBJ_ID:
            {
//                PSCHEDULEOBJ_T pScheduleObjTemp = (PSCHEDULEOBJ_T)pObjData;
//                json_pack_room_obj(pScheduleObjTemp+index, json_object_member);
//                json_object_array_add(jsonObjectData, json_object_member[index]);
            }
            break;
        case EVENT_OBJ_ID:
            {
                PEVENTOBJ_T pEventObjTemp = (PEVENTOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_event_obj((pEventObjTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case ENDPOINT_OBJ_ID:
            {
#ifdef VERSION_ENDPOINT_12
                PENDPOINTOBJ_EX_T pEndpointObj = (PENDPOINTOBJ_EX_T)pObjData;
                PENDPOINTOBJ_EX_T pEndpointTemp = NULL;
                struct json_object *json_object_sub_array[DEVICE_NUM] = {NULL};
                struct json_object *json_object_sub_object[DEVICE_NUM][ENDPOINTLIST_NUM];
                for (index=0; index < objNum; index++)
                {
                    json_object_sub_array[index] = json_object_new_array();
                    pEndpointTemp = pEndpointObj+index;
                    for (int count=0; count < pEndpointTemp->endpointNum; count++)
                    {
                        json_object_sub_object[index][count] = json_object_new_object();
                    }
                    json_pack_endpoint_obj(pEndpointTemp, json_object_member[index], json_object_sub_array[index], json_object_sub_object[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
#else
                PSINGLE_DEV_ENDPOINTOBJ_T pEndpointObj = (PSINGLE_DEV_ENDPOINTOBJ_T)pObjData;
                PSINGLE_DEV_ENDPOINTOBJ_T pEndpointTemp = NULL;
                struct json_object *json_object_sub_array[DEVICE_NUM] = {NULL};
                struct json_object *json_object_sub_object[DEVICE_NUM][ENDPOINTLIST_NUM];
                for (index=0; index < objNum; index++)
                {
                    json_object_sub_array[index] = json_object_new_array();
                    pEndpointTemp = pEndpointObj+index;
                    for (int count=0; count < pEndpointTemp->endpointNum; count++)
                    {
                        json_object_sub_object[index][count] = json_object_new_object();
                    }
                    json_pack_endpoint_obj(pEndpointTemp, json_object_member[index], json_object_sub_array[index], json_object_sub_object[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
#endif
            }
            break;
        case ENDPOINTLINK_OBJ_ID:
        {
            PENDPOINTLINK_OBJ_T pEndpointLinkObj = (PENDPOINTLINK_OBJ_T)pObjData;
            PENDPOINTLINK_OBJ_T pEndpointLinkTemp = NULL;
            os_json_manage_log("endpointlink object num is %d", objNum);
            for (index=0; index < objNum; index++)
            {
                pEndpointLinkTemp = pEndpointLinkObj+index;
                json_pack_endpointlink_obj(pEndpointLinkTemp, json_object_member[index]);
                json_object_array_add(json_object_item, json_object_member[index]);
            }
        }
            break;
        case CLOUDPARAM_OBJ_ID:
            {
                PCLOUDOBJ_T pObjDataTemp = (PCLOUDOBJ_T)pObjData;
                os_json_manage_log("the cloudparam total number is %d", objNum);
                for (index=0; index < objNum; index++)
                {
                    os_json_manage_log("the net device  total number is %d", objNum);
                    json_pack_cloudparam_obj((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case DEVINFO_OBJ_ID:
            {
                PDEVINFOOBJ_T pObjDataTemp = (PDEVINFOOBJ_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_device_info((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
        case LINKAGE_INPUT_OBJ_ID:
            {
                PLINKAGE_INPUT_TASK_T pObjDataTemp = (PLINKAGE_INPUT_TASK_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_linkage_input_obj((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            {
                PLINKAGE_OUTPUT_TASK_T pObjDataTemp = (PLINKAGE_OUTPUT_TASK_T)pObjData;
                for (index=0; index < objNum; index++)
                {
                    json_pack_linkage_output_obj((pObjDataTemp+index), json_object_member[index]);
                    json_object_array_add(json_object_item, json_object_member[index]);
                }
            }
            break;
        default:
            break;
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_manage_log("object to string fail");
        return kGeneralErr;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
//    os_json_manage_log("json string : %s", jsonString);

    for (index=0; index < objNum; index++)
    {
        if (json_object_member[index] != NULL)
        {
            json_object_put(json_object_member[index]);/*free memory*/
            json_object_member[index]=NULL;
        }
    }

    json_object_put(jsonObjectData);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
    return kNoErr;
}


/********************************************************
 * function: json_manage_unpack_get
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_manage_unpack_get(char *data, PMANAGE_PARAM_GET_T param_struct)
{
    if (data == NULL || param_struct == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int strLen = 0;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    os_json_manage_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_manage_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            if(!strcmp(key, KEY_UUID)){
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > UUID_SIZE)
                {
                    strLen = UUID_SIZE;
                }
                memset(param_struct->uuid, 0, UUID_SIZE);
                memcpy(param_struct->uuid, strTemp, strLen);
                os_json_manage_log("%s:%s", key, param_struct->uuid);

            }
            else if (!strcmp(key, KEY_CLASSID))
            {
                param_struct->classid = json_object_get_int(val);
                os_json_manage_log("%s:%d", key, param_struct->classid);

            }
            else if (!strcmp(key, KEY_OBJECTID_EX))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DEVICEID_SIZE)
                {
                    strLen = DEVICEID_SIZE;
                }
                memset(param_struct->objid, 0, DEVICEID_SIZE);
                memcpy(param_struct->objid, strTemp, strLen);
                os_json_manage_log("%s:%s", key, param_struct->objid);

            }
            else if (!strcmp(key, KEY_PAGE))
            {
                param_struct->page = json_object_get_int(val);
                os_json_manage_log("%s:%d", key, param_struct->page);
            }
            else if (!strcmp(key, KEY_LIMIT))
            {
                param_struct->limit = json_object_get_int(val);
                os_json_manage_log("%s:%d", key, param_struct->limit);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}



/********************************************************
 * function: json_manage_unpack_add
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 *          3. outdata:         point to structure on the basis of classID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_manage_unpack_add(char *data, int *classID, char *outdata, uint8_t *num)
{
    if (data == NULL || classID == NULL || outdata==NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    struct json_object *json_object_key_object = NULL;
    struct json_object *json_object_dev_object = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;
    int jsonType = 0;

    os_json_manage_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_manage_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {

        json_object_object_foreach(parse_json_object, key, val){
            if (!strcmp(key, KEY_CLASSID))
            {
                *classID = json_object_get_int(val);
                os_json_manage_log("%s:%d", key, *classID);
            }
            else if (!strcmp(key, KEY_OBJ))
            {
                jsonType = json_object_get_type(val);
                json_object_key_object = val;
                os_json_manage_log("%s: jsontype: %d", key, jsonType);
            }
        }
        if (json_object_key_object != NULL)
        {
            if (jsonType == json_type_object)
            {
                json_object_dev_object = json_object_key_object;
                json_unpack_obj(*classID, json_object_dev_object, outdata);

                *num = 1;
            }
            else if (jsonType == json_type_array)
            {
                struct array_list *arrayList = json_object_get_array(json_object_key_object);
                int size = 0;
                *num = arrayList->length;
                os_json_manage_log("array member:%d", *num);

                size = storage_get_obj_size(*classID);
                for (uint8_t index = 0; index < arrayList->length; index++)
                {
                    json_unpack_obj(*classID, arrayList->array[index], outdata+index*(size));
                }
            }
        }


        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_manage_unpack_del
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 *          3. outdata:         point to structure on the basis of classID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_manage_unpack_del(char *data, PMANAGE_DEL_PARAM_T pmanage_del_param)
{
    if (data == NULL || pmanage_del_param == NULL )
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;


    os_json_manage_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_manage_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_manage_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DEVICEID_SIZE)
                {
                    strLen = DEVICEID_SIZE;
                }
                memset(pmanage_del_param->devId, 0, sizeof(pmanage_del_param->devId));
                memcpy(pmanage_del_param->devId, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_CLASSID))
            {
                pmanage_del_param->classid = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_OBJECTID_EX))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > OBJID_SIZE)
                {
                    strLen = OBJID_SIZE-1;
                }
                memset(pmanage_del_param->objId, 0, sizeof(pmanage_del_param->devId));
                memcpy(pmanage_del_param->objId, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_LIMIT))
            {
                pmanage_del_param->limit = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_PAGE))
            {
                pmanage_del_param->page = json_object_get_int(val);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;
}

/********************************************************
 * function: json_manage_unpack_add
 * description:
 * input:   1. data:
 * output:  1. classID:
 *          2. outdata:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_manage_unpack_update(char *data, int *classID, unsigned char *outdata, uint8_t *objnum)
{
    json_manage_unpack_add(data, classID, outdata, objnum);
    return kNoErr;

}


