/**
 ******************************************************************************
 * @file    json_sys_deal.c
 * @author  chenb
 * @version V1.0.0
 * @date    5-July-2017
 * @brief   MiCO JSON format analysis.
 ******************************************************************************
  */



 /*****************************************************************************
  * model:      system service
  * decription:
  ******************************************************************************
  */
#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"

#define os_json_sys_log(M, ...) //custom_log("JSON_SYS", M, ##__VA_ARGS__)


/************************************************************
 * package "setmesh" data
*************************************************************/

/********************************************************
 * function: json_sys_setmesh
 * description:
 * input:   1. uuid
 *          2. flag
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_sys_pack_setmesh(PSETMESH_T setMeshData, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;

    if ((setMeshData == NULL) || (json_package == NULL))
    {
        os_json_sys_log("setmesh data struct is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }

    // package cmd
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SETMESH));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    json_object_object_add(jsonObjectData, KEY_DEVID, json_object_new_string(setMeshData->devid));
    json_object_object_add(jsonObjectData, PARAM_FLAG, json_object_new_int(setMeshData->flag));

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_sys_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_sys_log("json string : %s", jsonString);

    json_object_put(jsonObjectData);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;

}

/********************************************************
 * function: json_sys_regnet
 * description: regedit the device to network
 * input:   1. uuid
 *          2. flag
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_sys_pack_regnet(void)
{

}

/********************************************************
 * function: json_sys_regnet
 * description: regedit the device to network
 * input:   1. uuid
 *          2. flag
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_sys_pack_getdevinfo_resp(PDEV_DETAIL_INFO_T pDevInfo, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;


    if ((json_package == NULL) || (pDevInfo == NULL))
    {
        os_json_sys_log(" pDevInfo or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }

    // package the base of command infomation contains command/errorcode/body
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_GETDEVINFO));
    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(0));
    json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectData);
    json_object_object_add(jsonObjectData, KEY_SOFTVER, json_object_new_string(pDevInfo->software));
    json_object_object_add(jsonObjectData, KEY_HARDWARE, json_object_new_string(pDevInfo->hardware));
    json_object_object_add(jsonObjectData, KEY_MODELID, json_object_new_string(pDevInfo->modelid));
    json_object_object_add(jsonObjectData, KEY_PROT, json_object_new_string(pDevInfo->protocol));
    json_object_object_add(jsonObjectData, KEY_ADDR, json_object_new_string(pDevInfo->addr));
    json_object_object_add(jsonObjectData, KEY_MAC, json_object_new_string(pDevInfo->mac));
    json_object_object_add(jsonObjectData, KEY_STATUS, json_object_new_int(pDevInfo->status));

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_sys_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_sys_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_sys_regnet
 * description: UpgradeResult the device to network
 * input:   1. uuid
 *          2. flag
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_sys_pack_upgrade_result(PUPGRADE_RESULT_PARAM_T result_param, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;


    if ((json_package == NULL) || (result_param == NULL))
    {
        os_json_sys_log(" result_param or json_package is NULL!!!");
        return;
    }


    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }

    // package the base of command infomation contains command/errorcode/body
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_UPGRADERESULT));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    json_object_object_add(jsonObjectData, KEY_DEVID, json_object_new_string(result_param->netDevId));
    json_object_object_add(jsonObjectData, KEY_RESULT, json_object_new_string(result_param->result));
    json_object_object_add(jsonObjectData, KEY_INNERID, json_object_new_string(result_param->innerid));


    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_sys_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_sys_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_sys_regnet
 * description: UpgradeResult the device to network
 * input:   1. uuid
 *          2. flag
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_sys_pack_init_ver_info(PNETDEV_MQTT_INITINFO pnet_dev_ver_info, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;


    if ((json_package == NULL) || (pnet_dev_ver_info == NULL))
    {
        os_json_sys_log(" pnet_dev_ver_info or json_package is NULL!!!");
        return;
    }


    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();

    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }


    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_INIT));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    json_object_object_add(jsonObjectData, KEY_VER, json_object_new_string(pnet_dev_ver_info->ver));
    json_object_object_add(jsonObjectData, KEY_VERTYPE, json_object_new_int(pnet_dev_ver_info->vertype));


    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_sys_log("object to string fail");
        return;
    }

    sprintf(json_package, "%s", jsonString);
    os_json_sys_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/************************************************************
 * unpack and get "setmesh" data
*************************************************************/
/********************************************************
 * function: json_sys_unpack_setmesh
 * description: get setmesh data
 * input:   1. data:           json data
 * output:  1. setMeshData:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_setmesh(char *data, PSETMESH_T setMeshData)
{
    if (data == NULL || setMeshData == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;


    os_json_sys_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(setMeshData->devid))
                {
                    strLen = sizeof(setMeshData->devid);
                }
                memcpy(setMeshData->devid, strTemp, strLen);

            }
            else if(!strcmp(key, PARAM_FLAG)){
                setMeshData->flag = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_TIMEOUT))
            {
                setMeshData->timeout = json_object_get_int(val);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_unpack_devid
 * description: unpack data to get device id
 * input:   1. data:           json data
 * output:  1. devid:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_devid(char *data, char *devid)
{
    if (data == NULL || devid == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DEVICEID_SIZE)
                {
                    strLen = DEVICEID_SIZE;
                }
                memcpy(devid, strTemp, strLen);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;
}

/********************************************************
 * function: json_sys_unpack_reset
 * description:
 * input:   1. data:           json data
 * output:  1. devid:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_reset(char *data, char *devid)
{
    return json_unpack_devid(data, devid);
}

/********************************************************
 * function: json_sys_unpack_resetmesh
 * description:
 * input:   1. data:           json data
 * output:  1. devid:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_resetmesh(char *data, char *devid)
{
    return json_unpack_devid(data, devid);
}

/********************************************************
 * function: json_sys_unpack_setmesh
 * description: get setmesh data
 * input:   1. data:           json data
 * output:  1. setMeshData:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_synctime(char *data, uint32_t *utc_time)
{
    if (data == NULL || utc_time == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_TIME)){
                *utc_time = json_object_get_int(val);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_sys_unpack_factory
 * description: parse "factory" json command
 * input:   1. data:           json data
 * output:  1. devid:    the device id which we want to restore factory
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_factory(char *data, char *devid)
{
    return json_unpack_devid(data, devid);
}

/********************************************************
 * function: json_sys_unpack_getdevinfo
 * description:
 * input:   1. data:     json data
 * output:  1. devid:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_getdevinfo(char *data, char *devid)
{
    return json_unpack_devid(data, devid);

}

/********************************************************
 * function: json_sys_unpack_netparam
 * description: get network param
 * input:   1. data:           json data
 * output:  1. net_param:       network data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_netparam(char *data, PNET_PARAM_T net_param)
{
    if (data == NULL || net_param == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    os_json_sys_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
//            os_json_sys_log("%s", key);
            if(!strcmp(key, PARAM_SSID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(net_param->ssid))
                {
                    strLen = sizeof(net_param->ssid);
                }
                memcpy(net_param->ssid, strTemp, strLen);

            }
            else if(!strcmp(key, PARAM_SECURITY)){
                net_param->security = json_object_get_int(val);
            }
            else if (!strcmp(key, PARAM_KEY)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(net_param->key))
                {
                    strLen = sizeof(net_param->key);
                }
                memcpy(net_param->key, strTemp, strLen);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_sys_unpack_reg_devInfo
 * description: get network param
 * input:   1. data:           json data
 * output:  1. pregObject:     the data about regist
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_reg_devInfo(char *data, PDEVICEOBJ_T pregObject)
{
    if (data == NULL || pregObject == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    os_json_sys_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pregObject->deviceId))
                {
                    strLen = sizeof(pregObject->deviceId);
                }
                memcpy(pregObject->deviceId, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_UUID))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pregObject->uuid))
                {
                    strLen = sizeof(pregObject->uuid);
                }
                memcpy(pregObject->uuid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_NAME))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pregObject->name))
                {
                    strLen = sizeof(pregObject->name);
                }
                memcpy(pregObject->name, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_MAC))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pregObject->mac))
                {
                    strLen = sizeof(pregObject->mac);
                }
                memcpy(pregObject->mac, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_MODELID))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pregObject->modelid))
                {
                    strLen = sizeof(pregObject->modelid);
                }
                memcpy(pregObject->modelid, strTemp, strLen);
            }
            else if(!strcmp(key, KEY_CHANNEL)){
                pregObject->ch = json_object_get_int(val);
            }
            else if(!strcmp(key, KEY_DEVTYPE)){
                pregObject->devtype = json_object_get_int(val);
            }
            else if(!strcmp(key, KEY_STATUS)){
                pregObject->status.statusInfo = json_object_get_int(val);
            }
            else if(!strcmp(key, KEY_COMM)){
                pregObject->comm = json_object_get_int(val);
            }

        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}


/********************************************************
 * function: json_sys_unpack_setaddr
 * description: set subdevice IP
 * input:   1. data:           json data
 * output:  1. devid:          the device you want to set IP
 *          2. addr:           subdevice IP
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_setaddr(char *data, char *devid, char *addr)
{
    if ((data == NULL) || (devid == NULL) || (addr == NULL))
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    os_json_sys_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal !!!");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DEVICEID_SIZE)
                {
                    strLen = DEVICEID_SIZE;
                }
                memcpy(devid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_ADDR))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NET_ADDR_SIZE)
                {
                    strLen = NET_ADDR_SIZE;
                }
                memcpy(addr, strTemp, strLen);
            }

        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_sys_unpack_upgrade
 * description: parse "factory" json command
 * input:   1. data:           json data
 * output:  1. upgrade_param:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_upgrade(char *data, PUPGRADE_PARAM_T upgrade_param)
{
    if (data == NULL || upgrade_param == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal: %s", string);
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_VER)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(upgrade_param->ver, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_URL))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(upgrade_param->url, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_MD5))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(upgrade_param->md5, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_INNERID))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(upgrade_param->innerid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_VERTYPE))
            {
                upgrade_param->vertype = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_FORCE))
            {
                upgrade_param->force = json_object_get_int(val);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_sys_unpack_sendaddr
 * description: parse "factory" json command
 * input:   1. data:           json data
 * output:  1. upgrade_param:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_sendaddr(char *data, PSENDADDR_PARAM_T sendaddr_param)
{
    if (data == NULL || sendaddr_param == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal: %s", string);
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_SRCMAC)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(sendaddr_param->srcmac, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_ADDR))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(sendaddr_param->addr, strTemp, strLen);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_sys_unpack_sendaddr
 * description: parse "factory" json command
 * input:   1. data:           json data
 * output:  1. upgrade_param:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_sys_unpack_cleardata(char *data, char *devid)
{
    if (data == NULL || devid == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_sys_log("json data is illegal: %s", string);
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_sys_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                memcpy(devid, strTemp, strLen);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;
}




