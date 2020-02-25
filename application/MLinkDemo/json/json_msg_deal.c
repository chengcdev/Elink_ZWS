/*
 * json_msg_deal.c
 *
 *  Created on: 2017年7月7日
 *      Author: Administrator
 */

 /*****************************************************************************
  * model:      message notify  service
  * decription:
  ******************************************************************************
  */
#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"

#define os_json_msg_log(M, ...) custom_log("JSON_MSG", M, ##__VA_ARGS__)

/********************************************************
 * function: json_msg_pack_event
 * description:
 * input:   1. uuid
 *          2. event_obj
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_event(PEVENT_REPORT_T notifyEvent, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_event = NULL;

    char *jsonString = NULL;

    if ((notifyEvent == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    json_object_event = json_object_new_object();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL)||( json_object_event == NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MESSAGE_EVENT));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    json_object_object_add(jsonObjectData, KEY_EVENTLEVEL,    json_object_new_int(notifyEvent->eventlevel));
    json_object_object_add(jsonObjectData, KEY_EVENTTYPE,     json_object_new_int(notifyEvent->eventtype));
    json_object_object_add(jsonObjectData, KEY_NOTITYPE,      json_object_new_int(notifyEvent->notitype));
    json_object_object_add(jsonObjectData, KEY_MSG,           json_object_event);

    // package event
    json_object_object_add(json_object_event, KEY_ENDPOINTID,   json_object_new_string(notifyEvent->msgContent.endpointid));
    json_object_object_add(json_object_event, KEY_KEY,     json_object_new_string(notifyEvent->msgContent.key));
    json_object_object_add(json_object_event, KEY_VALUE,     json_object_new_string(notifyEvent->msgContent.value));

    if (notifyEvent->msgContent.uuid[0] != 0)
    {
        json_object_object_add(json_object_event, KEY_UUID,     json_object_new_string(notifyEvent->msgContent.uuid));
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_msg_log("json string : %s", jsonString);

    json_object_put(jsonObjectData);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}


/********************************************************
 * function: json_msg_pack_event
 * description:
 * input:   1. uuid
 *          2. event_obj
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_statechange(PREPORT_STATE_CHANGE_T report_info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_status_array = NULL;
    struct json_object *json_object_value[10];

    char *jsonString = NULL;

    if ((report_info == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MESSAGE_STATECHANGE));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    if (report_info->devid[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_DEVID,    json_object_new_string(report_info->devid));
        json_object_object_add(jsonObjectData, KEY_STATUS,    json_object_new_string(report_info->status));
    }
    if (report_info->subdevNum != 0)
    {
        PSTATE_OBJ_T pstateObj = NULL;
        int count=0;
        json_object_status_array = json_object_new_array();
        json_object_object_add(jsonObjectData, KEY_SUBDEV,     json_object_status_array);

        // add status member
        for (count=0; count<report_info->subdevNum; count++)
        {
            pstateObj = report_info->stateObj+count;
            json_object_value[count] = json_object_new_object();
            if (pstateObj->uuid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_UUID, json_object_new_string(pstateObj->uuid));
            }
            else if (pstateObj->subdevid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_DEVID, json_object_new_string(pstateObj->subdevid));
            }
            if (pstateObj->endpointid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_ENDPOINTID, json_object_new_string(pstateObj->endpointid));
            }
            if (pstateObj->addr[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_ADDR, json_object_new_string(pstateObj->addr));
            }
            json_object_object_add(json_object_value[count], KEY_STATUS, json_object_new_string(pstateObj->status));

            json_object_array_add(json_object_status_array, json_object_value[count]);
        }
    }


    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
//    os_json_msg_log("json string : %s", json_package);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_msg_pack_event
 * description:
 * input:   1. uuid
 *          2. event_obj
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_statuschange(PREPORT_STATE_CHANGE_T report_info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_status_array = NULL;
    struct json_object *json_object_value[10];

    char *jsonString = NULL;

    if ((report_info == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MESSAGE_STATUSCHANGE));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    if (report_info->devid[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_DEVID,    json_object_new_string(report_info->devid));
        json_object_object_add(jsonObjectData, KEY_STATUS,    json_object_new_string(report_info->status));
    }
    if (report_info->subdevNum != 0)
    {
        PSTATE_OBJ_T pstateObj = NULL;
        int count=0;
        json_object_status_array = json_object_new_array();
        json_object_object_add(jsonObjectData, KEY_SUBDEV,     json_object_status_array);

        // add status member
        for (count=0; count<report_info->subdevNum; count++)
        {
            pstateObj = report_info->stateObj+count;
            json_object_value[count] = json_object_new_object();
            if (pstateObj->uuid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_UUID, json_object_new_string(pstateObj->uuid));
            }
            else if (pstateObj->subdevid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_DEVID, json_object_new_string(pstateObj->subdevid));
            }
            if (pstateObj->endpointid[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_ENDPOINTID, json_object_new_string(pstateObj->endpointid));
            }
            if (pstateObj->addr[0] != 0)
            {
                json_object_object_add(json_object_value[count], KEY_ADDR, json_object_new_string(pstateObj->addr));
            }
            json_object_object_add(json_object_value[count], KEY_STATUS, json_object_new_string(pstateObj->status));

            json_object_array_add(json_object_status_array, json_object_value[count]);
        }
    }


    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
//    os_json_msg_log("json string : %s", json_package);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_msg_pack_event
 * description:
 * input:   1. uuid
 *          2. event_obj
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_linknotify(PREPORT_STATE_CHANGE_T report_info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_status_array = NULL;

    char *jsonString = NULL;

    if ((report_info == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL)||( json_object_status_array == NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MESSAGE_LINKTASK));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    json_object_object_add(jsonObjectData, KEY_LINKID,    json_object_new_int(report_info->devid));




    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_msg_log("json string : %s", json_package);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_msg_pack_changenotify
 * description:
 * input:   1. uuid
 *          2. dev_func_obj
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_changenotify(char *uuid, PDEV_FUNCTION_OBJ_T dev_func_obj, int num, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *json_object_function = NULL;
    char *jsonString = NULL;

    if ((dev_func_obj == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();


    if ((jsonObjectService==NULL) || (jsonObjectData==NULL)||( json_object_function == NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD,    json_object_new_string(MESSAGE_CHANGENOTIFY));
    json_object_object_add(jsonObjectService, PUBLIC_DATA,   jsonObjectData);
    json_object_object_add(jsonObjectData, KEY_UUID,      json_object_new_string(uuid));

    if (num == 1)
    {
        PDEV_FUNCTION_OBJ_T devFucObj = dev_func_obj;
        json_object_function = json_object_new_object();
        json_object_object_add(jsonObjectData, KEY_ATTR,  json_object_function);
        json_object_object_add(json_object_function, KEY_KEY,  json_object_new_string(devFucObj->key));
        json_object_object_add(json_object_function, KEY_VALUE,json_object_new_string(devFucObj->value));

        // object to string
        jsonString = json_object_to_json_string(jsonObjectService);
    }
    else
    {
        int memberCount = 0;
        struct json_object *json_object_func_member[20];
        PDEV_FUNCTION_OBJ_T devFucObj = NULL;
        json_object_function = json_object_new_array();
        json_object_object_add(jsonObjectData, KEY_ATTR,  json_object_function);
        for (memberCount=0; memberCount>num; memberCount++)
        {
            devFucObj = dev_func_obj+memberCount;
            json_object_func_member[memberCount] = json_object_new_object();
            json_object_array_add(json_object_function, json_object_func_member[memberCount]);
            json_object_object_add(json_object_func_member, KEY_KEY, json_object_new_string(devFucObj->key));
            json_object_object_add(json_object_func_member, KEY_VALUE, json_object_new_string(devFucObj->value));
        }
        // object to string
        jsonString = json_object_to_json_string(jsonObjectService);
    }



    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_msg_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_msg_pack_allow_notify
 * description:
 * input:   1. info
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_msg_pack_allow_notify( ALLOW_NOTIFY_INFO_T *info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;

    char *jsonString = NULL;

    if ((info == NULL) || (json_package == NULL))
    {
        os_json_msg_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MESSAGE_ALLOW_NOTIFY));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);


    if (info->uuid[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_UUID,     json_object_new_string(info->uuid));
    }
    if (info->panid[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_ADDR,     json_object_new_string(info->panid));
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_msg_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_msg_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_msg_unpack_event
 * description:
 * input:   1. data
 * output:  1. notifyEvent
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_msg_unpack_event(char *data, PEVENT_REPORT_T notifyEvent)
{
    if ((data == NULL) || (notifyEvent == NULL))
    {
        return kGeneralErr;
    }

    struct json_object *json_parse_object = NULL;
    int len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (FALSE == status)
    {
        return kGeneralErr;
    }
    json_parse_object = json_tokener_parse(string);
    if (NULL != json_parse_object)
    {
        json_object_object_foreach(json_parse_object, key, val){
            if (!strcmp(key, KEY_EVENTLEVEL))
            {
                notifyEvent->eventlevel = json_object_get_int(val);
//                os_json_msg_log("%s: %d", key, notifyEvent->eventlevel);
            }
            else if (!strcmp(key, KEY_EVENTTYPE))
            {
                notifyEvent->eventtype = json_object_get_int(val);
//                os_json_msg_log("%s: %d", key, notifyEvent->eventtype);
            }
            else if (!strcmp(key, KEY_NOTITYPE))
            {
                notifyEvent->notitype = json_object_get_int(val);
//                os_json_msg_log("%s: %d", key, notifyEvent->notitype);
            }
            else if (!strcmp(key, KEY_MSG))
            {
                json_unpack_msg_content(val, &notifyEvent->msgContent);
                os_json_msg_log("%s", key);
            }
        }
    }
    json_object_put(json_parse_object);/*free memory*/
    json_parse_object=NULL;

    return kNoErr;
}

/********************************************************
 * function: json_msg_unpack_statechange
 * description: set subdevice IP
 * input:   1. data:           json data
 * output:  1. devid:          the device you want to set IP
 *          2. addr:           subdevice IP
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_msg_unpack_statechange(char *data, PREPORT_STATE_CHANGE_T report_info)
{
    if ((data == NULL) || (report_info == NULL))
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
//    char *string = malloc(strlen(data)+1);
    char *string = data;

    int status = 0;
    array_list *arrayList = NULL;


//    memset(string, 0, sizeof(string));
//    memcpy(string, data, len);
    os_json_msg_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_msg_log("json data is illegal !!!");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
//            os_json_msg_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen >= sizeof(report_info->devid))
                {
                    strLen = sizeof(report_info->devid) - 1;
                }
                memcpy(report_info->devid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_STATUS))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > STATUS_SIZE)
                {
                    strLen = STATUS_SIZE - 1;
                }
                memcpy(report_info->status, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_SUBDEV))
            {
                uint8_t memberIndex = 0;
                PSTATE_OBJ_T pstatusTemp = NULL;
                arrayList = json_object_get_array(val);
                report_info->subdevNum = arrayList->length;
                if (report_info->stateObj == NULL)
                {
                    report_info->stateObj = (PSTATE_OBJ_T)malloc(report_info->subdevNum*sizeof(STATE_OBJ_T));
                    if (report_info->stateObj != NULL)
                    {
                        memset(report_info->stateObj, 0, report_info->subdevNum*sizeof(STATE_OBJ_T));
                        for (memberIndex=0; memberIndex < arrayList->length; memberIndex++)
                        {
                            pstatusTemp = report_info->stateObj + memberIndex;
                            json_unpack_msg_subdev_state(arrayList->array[memberIndex], pstatusTemp);
                        }
                    }
                }

            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    //free(string);// note by 2018.3.19 回调函数之后已经释放了，否则会导致串口发送队列异常
    return kNoErr;

}

/********************************************************
 * function: json_msg_unpack_allow_notify
 * description:
 * input:   1. data
 * output:  1. notifyEvent
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_msg_unpack_allow_notify( char *data, ALLOW_NOTIFY_INFO_T *info )
{
     if ((data == NULL) || (info == NULL))
     {
         return kGeneralErr;
     }

     struct json_object *json_parse_object = NULL;
     int len = strlen(data);
     char *string = data;
     int status = 0;

     status = json_checker_string(string, len);
     if (FALSE == status)
     {
         return kGeneralErr;
     }
     json_parse_object = json_tokener_parse(string);
     if (NULL != json_parse_object)
     {
         json_object_object_foreach(json_parse_object, key, val){
             if (!strcmp(key, KEY_UUID))
             {
                 int strLen = 0;
                 string = json_object_get_string(val);
                 strLen = strlen(string);
                 if (strlen(string) >= UUID_SIZE)
                 {
                     strLen = UUID_SIZE-1;
                 }
                 memcpy(info->uuid, string, strLen);
//                os_json_msg_log("%s: %d", key, notifyEvent->eventlevel);
             }
             else if (!strcmp(key, KEY_ADDR))
             {
                 int strLen = 0;
                 string = json_object_get_string(val);
                 strLen = strlen(string);
                 if (strlen(string) >= 4)
                 {
                     strLen = 4-1;
                 }
                 memcpy(info->panid, string, strLen);
             }
         }
     }
     json_object_put(json_parse_object);/*free memory*/
     json_parse_object=NULL;

     return kNoErr;
}



