/*
 * json_discover_deal.c
 *
 *  Created on: 2017骞�7鏈�7鏃�
 *      Author: chenb
 */


 /*****************************************************************************
  * model:      discover device service
  * decription:
  ******************************************************************************
  */
#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"
#include "../include/include_coap.h"

#define os_json_discover_log(M, ...) custom_log("JSON_DISCOVER", M, ##__VA_ARGS__)

/********************************************************
 * function: json_pack_who_is_response
 * description:
 * input:   uuid
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_who_is_response(int errcode, char *errmsg, PNETDEVOBJ_T pnet_obj, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectBody = NULL;
    char *jsonString = NULL;

    if (json_package == NULL)
    {
        os_json_discover_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectBody = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectBody==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_WHO_IS));
    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(errcode));
    if (errcode != MLINK_RESP_OK)
    {
        if (errmsg != NULL)
        {
            json_object_object_add(jsonObjectService, PUBLIC_ERRMSG, json_object_new_string(errmsg));
        }
    }
    else
    {
        json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectBody);
        // package data
        json_pack_netdev_obj(pnet_obj, jsonObjectBody);
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_discover_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_discover_log("json string : %s", jsonString);

    json_object_put(jsonObjectBody);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectBody=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_pack_req_discover
 * description:
 * input:   uuid
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_req_discover(char *uuid, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;

    if (json_package == NULL)
    {
        os_json_discover_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_WHO_IS));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    if (uuid != NULL)
    {
        json_object_object_add(jsonObjectData, PARAM_UUID, json_object_new_string(uuid));
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_discover_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_discover_log("json string : %s", jsonString);

    json_object_put(jsonObjectData);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}


/********************************************************
 * function: json_disc_pack_i_am
 * description:
 * input:   uuid
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_disc_pack_i_am(PNETDEVOBJ_T pnetdevobj, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;
    char deviceId[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};
    if ((pnetdevobj == NULL) || (json_package == NULL))
    {
        os_json_discover_log(" netdevobj or json_package is NULL!!!");
        return;
    }
    memcpy(deviceId, pnetdevobj->deviceId, DEVICEID_SIZE);
    memcpy(name, pnetdevobj->name, NAME_SIZE);

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_I_AM));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    json_pack_netdev_obj(pnetdevobj, jsonObjectData);

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_discover_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
//    os_json_discover_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}

/********************************************************
 * function: json_disc_pack_i_hav
 * description:
 * input:   uuid
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_disc_pack_i_hav(PREPORT_SUBDEV_INFO_T pdeviceObj, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;

    if ((pdeviceObj == NULL) || (json_package == NULL))
    {
        os_json_discover_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();
    if ((jsonObjectService==NULL) || (jsonObjectData==NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_I_HAV));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    // package data
    json_object_object_add(jsonObjectData, KEY_FID, json_object_new_string(pdeviceObj->fid));
    json_object_object_add(jsonObjectData, KEY_MODELID, json_object_new_string(pdeviceObj->modelid));
    json_object_object_add(jsonObjectData, KEY_MAC, json_object_new_string(pdeviceObj->mac));
    json_object_object_add(jsonObjectData, KEY_COMM, json_object_new_int(pdeviceObj->comm));
    json_object_object_add(jsonObjectData, KEY_ADDR, json_object_new_string(pdeviceObj->netaddr));
    if (pdeviceObj->verHardware[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_VER_HARD, json_object_new_string(pdeviceObj->verHardware));
    }
    if (pdeviceObj->verSoftware[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_VER_SOFT, json_object_new_string(pdeviceObj->verSoftware));
    }
    if (pdeviceObj->verProt[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_VER_PROTO, json_object_new_string(pdeviceObj->verProt));
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_discover_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_discover_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectData=NULL;
    jsonObjectService=NULL;
}


/************************************************************
 * unpack and get "discover service" data
*************************************************************/
/********************************************************
 * function: json_disc_unpack_whois
 * description:
 * input:   1. data:           json data
 * output:  1. setMeshData:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_disc_unpack_whois(char *data, char *uuid)
{
    if (data == NULL || uuid == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

//    os_json_discover_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_discover_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_discover_log("%s", key);
            if(!strcmp(key, KEY_UUID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > UUID_SIZE)
                {
                    strLen = UUID_SIZE;
                }
                memcpy(uuid, strTemp, strLen);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_disc_unpack_who_hav
 * description:
 * input:   1. data:           json data
 * output:  1. setMeshData:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_disc_unpack_who_hav(char *data, uint32_t *timeout)
{
    if ((data == NULL) || (NULL == timeout))
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    os_json_discover_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_discover_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_discover_log("%s", key);
            if(!strcmp(key, KEY_TIMEOUT)){
                *timeout = json_object_get_int(val);
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_disc_unpack_i_am
 * description:
 * input:   1. data:           json data
 * output:  1. pNetDevObj:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_disc_unpack_i_am(char *data, PNETDEVOBJ_T pNetDevObj)
{
    if (data == NULL || pNetDevObj == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(pNetDevObj, 0, sizeof(NETDEVOBJ_T));
    os_json_discover_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_discover_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    json_unpack_netdev_obj(parse_json_object, pNetDevObj);

    json_object_put(parse_json_object);/*free memory*/
    parse_json_object=NULL;
    return kNoErr;

}


