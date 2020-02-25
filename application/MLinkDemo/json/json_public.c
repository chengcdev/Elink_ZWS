/**
 ******************************************************************************
 * @file    json_public.c
 * @author  chenb
 * @version V1.0.0
 * @date    5-July-2017
 * @brief   MiCO JSON format analysis.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************
 */

#include "json_checker.h"
#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"
#include "../include/include_coap.h"

#define os_json_log(M, ...) //custom_log("JSON_PUBLIC", M, ##__VA_ARGS__)


/********************************************************
 * function: json_get_error_msg
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_get_error_msg(MLINK_RESPONSE_STATUS_E state, char *errorMsg)
{
    if (errorMsg == NULL)
    {
        return FALSE;
    }

    switch (state)
    {
        case MLINK_RESP_ERROR:
            memcpy(errorMsg, MLINK_ERR_GENERAL_ERROR, strlen(MLINK_ERR_GENERAL_ERROR));
            break;
        case MLINK_RESP_ERR_PUBLIC:
            memcpy(errorMsg, MLINK_ERR_PUBLIC_PARAM_ERROR, strlen(MLINK_ERR_PUBLIC_PARAM_ERROR));
        case MLINK_RESP_ILLEGAL:
            memcpy(errorMsg, MLINK_ERR_DATA_ILLEGAL, strlen(MLINK_ERR_DATA_ILLEGAL));
            break;
        case MLINK_RESP_LACK_PARAM:
            memcpy(errorMsg, MLINK_ERR_LACK_PARAM, strlen(MLINK_ERR_LACK_PARAM));
            break;
        case MLINK_RESP_CHECK_FAIL:
            memcpy(errorMsg, MLINK_ERR_DATA_VALIDATION_FAILURE, strlen(MLINK_ERR_DATA_VALIDATION_FAILURE));
            break;
        case MLINK_RESP_OBJECT_EXISTED:
            memcpy(errorMsg, MLINK_ERR_OBJECT_EXISTED, strlen(MLINK_ERR_OBJECT_EXISTED));
            break;
        case MLINK_RESP_FIND_NOTHING:
            memcpy(errorMsg, MLINK_ERR_NO_FINDING_OBJECT, strlen(MLINK_ERR_NO_FINDING_OBJECT));
            break;
        default:
            break;
    }
    return TRUE;
}

/********************************************************
 * function: json_service_pack_resp_err
 * description:
 * input:   1. cmdId:            json data
 *          2. state
 * output:  1. json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_resp_errCode(COAP_NOTI_CMD_ID_E cmdId, MLINK_RESPONSE_STATUS_E state, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    char *jsonString = NULL;
    char errorMsg[64] = {0};

    if (json_package == NULL)
    {
        os_json_log("json_package is NULL!!!");
        return kGeneralErr;
    }
    memset(errorMsg, 0, sizeof(errorMsg));
    jsonObjectService = json_object_new_object();

    // package data
    if (jsonObjectService==NULL)
    {
        return kGeneralErr;
    }
    switch (cmdId)
    {   // system service
        case COAP_SYS_NOTI_CMD_RESET:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_RESET));
            break;
        case COAP_SYS_NOTI_CMD_FACTORY:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_FACTORY));
            break;
        case COAP_SYS_NOTI_CMD_SYNCTIME:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SYNCTIME));
            break;
        case COAP_SYS_NOTI_CMD_SETMESH:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SETMESH));
            break;
        case COAP_SYS_NOTI_CMD_SETNET:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SETNET));
            break;
        case COAP_SYS_NOTI_CMD_REGDEV:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_REGDEV));
            break;
        case COAP_SYS_NOTI_CMD_SETCLOUD:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SETCLOUD));
            break;
        case COAP_SYS_NOTI_CMD_GETDEVINFO:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_GETDEVINFO));
            break;
        case COAP_SYS_NOTI_CMD_KEEPALIVE:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_KEEPALIVE));
            break;
        case COAP_SYS_NOTI_CMD_SETADDR:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_SETADDR));
            break;
        case COAP_SYS_NOTI_CMD_UPGRADE:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_UPGRADE));
            break;
        case COAP_SYS_NOTI_CMD_RESETMESH:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_RESETMESH));
            break;
        case COAP_SYS_NOTI_CMD_CLEARDATA:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SYS_CLEARDATA));
            break;

        // discover service
        case COAP_DISCOVER_NOTI_CMD_WHOIS:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_WHO_IS));
            break;
        case COAP_DISCOVER_NOTI_CMD_IAM:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_I_AM));
            break;
        case COAP_DISCOVER_NOTI_CMD_WHO_HAV:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(DISCOVER_WHO_HAV));
            break;

        // manager service
        case COAP_MANAGER_NOTI_CMD_GET:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_GET));
            break;
        case COAP_MANAGER_NOTI_CMD_ADD:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_ADD));
            break;
        case COAP_MANAGER_NOTI_CMD_DEL:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_DEL));
            break;
        case COAP_MANAGER_NOTI_CMD_CLEAR:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_CLEAR));
            break;
        case COAP_MANAGER_NOTI_CMD_UPDATE:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(MANAGER_UPDATE));
            break;

        // control service
        case COAP_CTRL_NOTI_CMD_READ:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_READ));
            break;
        case COAP_CTRL_NOTI_CMD_WRITE:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_WRITE));
            break;
        case COAP_CTRL_NOTI_CMD_READMULT:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_READMULT));
            break;
        case COAP_CTRL_NOTI_CMD_WRITEMULT:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_WRITEMULT));
            break;
        case COAP_CTRL_NOTI_CMD_WRITEGROUP:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_WRITEGROUP));
            break;
        case COAP_CTRL_NOTI_CMD_DEVSET:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_DEVSET));
            break;
        // unknow service
        default:
            json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(SERVICE_UNKNOWN));
            break;
    }

    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(state));
    if (state != MLINK_RESP_OK)
    {
        json_get_error_msg(state, errorMsg);
        json_object_object_add(jsonObjectService, PUBLIC_ERRMSG, json_object_new_string(errorMsg));

    }


    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_log("object to string fail");
        return kGeneralErr;
    }

    sprintf(json_package, "%s", jsonString);
    os_json_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    jsonObjectService=NULL;
    return kNoErr;
}


/********************************************************
 * function: json_get_cmd
 * description:
 * input:   1. data:            json data
 *          2. len :
 * output:  1. cmd:
 *          2. outdata:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int json_get_cmd(char *data, uint32_t len, char *cmd, char *outdata)
{
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    char *string = data;
    if ((data == NULL) || (cmd == NULL))
    {
        return FALSE;
    }

    os_json_log("data:%s, len = %d\n", string, len);
    int status = json_checker_string(string, len);
    if (!status)
    {
        os_json_log("json data is illegal");
        return FALSE;
    }
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_log("%s", key);
            if(!strcmp(key, PUBLIC_CMD)){
                strTemp = json_object_get_string(val);
                if (strlen(strTemp) < 20)
                {
                    memcpy(cmd, strTemp, strlen(strTemp));
                }
                else
                {
                    os_json_log("cmdLen more then 20", strlen(strTemp));
                }
            }
            else if(!strcmp(key, PUBLIC_DATA)){
                strTemp = json_object_get_string(val);
                memcpy(outdata, strTemp, strlen(strTemp));
            }
        }
        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
}

/********************************************************
 * function: json_pack_device_info
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_device_info(PDEVINFOOBJ_T pdevinfo, json_object *package_object)
{
    if ((pdevinfo == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    if ((pdevinfo->ver[0] >= ' ') && (pdevinfo->ver[0] <= '~'))
    {
        json_object_object_add(package_object, KEY_VER,         json_object_new_string(pdevinfo->ver));
    }
    if ((pdevinfo->firmver[0] >= ' ') && (pdevinfo->firmver[0] <= '~'))
    {
        json_object_object_add(package_object, KEY_FIRMVER,     json_object_new_string(pdevinfo->firmver));
    }
    return kNoErr;
}

/********************************************************
 * function: json_pack_device_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_device_obj(PDEVICEOBJ_T pdevice_obj, json_object *package_object)
{
    char deviceId[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};

    if ((pdevice_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }


    memcpy(deviceId, pdevice_obj->deviceId, DEVICEID_SIZE);
    memcpy(name, pdevice_obj->name, NAME_SIZE);

    json_object_object_add(package_object, KEY_DEVID,      json_object_new_string(deviceId));
    json_object_object_add(package_object, KEY_NAME,       json_object_new_string(name));
    json_object_object_add(package_object, KEY_MAC,        json_object_new_string_len(pdevice_obj->mac, (strlen(pdevice_obj->mac)<=MAC_SIZE)?(strlen(pdevice_obj->mac)):MAC_SIZE));
    json_object_object_add(package_object, KEY_ADDR,       json_object_new_string_len(pdevice_obj->uuid, (strlen(pdevice_obj->uuid)<=NET_ADDR_SIZE)?strlen(pdevice_obj->uuid):NET_ADDR_SIZE));
    json_object_object_add(package_object, KEY_MODELID,    json_object_new_string(pdevice_obj->modelid));
    json_object_object_add(package_object, KEY_CHANNEL,    json_object_new_int(pdevice_obj->ch));
    json_object_object_add(package_object, KEY_DEVTYPE,    json_object_new_int(pdevice_obj->devtype));
    json_object_object_add(package_object, KEY_COMM,       json_object_new_int(pdevice_obj->comm));
    json_object_object_add(package_object, KEY_STATUS,     json_object_new_int(pdevice_obj->status.statusInfo));
    json_object_object_add(package_object, KEY_OFFLINE,    json_object_new_int(pdevice_obj->offline));

    return kNoErr;
}

/********************************************************
 * function:  json_unpack_device_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_device_obj(json_object *pjson_object, PDEVICEOBJ_T pdevice_obj)
{
    os_json_log("\n");
    if ((pdevice_obj == NULL) || (pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pdevice_obj, 0, sizeof(DEVICEOBJ_T));

    json_object_object_foreach(pjson_object, key, val){
//                os_json_log("%s", key);
                if(!strcmp(key, KEY_UUID)){
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > UUID_SIZE)
                    {
                        strLen = UUID_SIZE;
                    }
                    memset(pdevice_obj->uuid, 0, UUID_SIZE);
                    memcpy(pdevice_obj->uuid, strTemp, strLen);

                    os_json_log("%s: %s", key, pdevice_obj->uuid);
                }
                else if (!strcmp(key, KEY_DEVID))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > DEVICEID_SIZE)
                    {
                        strLen = DEVICEID_SIZE;
                    }
                    memset(pdevice_obj->deviceId, 0, DEVICEID_SIZE);
                    memcpy(pdevice_obj->deviceId, strTemp, strLen);
                    os_json_log("%s: %s", key, pdevice_obj->deviceId);

                }
                else if (!strcmp(key, KEY_MAC))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > MAC_SIZE)
                    {
                        strLen = MAC_SIZE;
                    }
                    memset(pdevice_obj->mac, 0, MAC_SIZE);
                    memcpy(pdevice_obj->mac, strTemp, MAC_SIZE);
                    os_json_log("%s: %s", key, pdevice_obj->mac);

                }
                else if (!strcmp(key, KEY_ADDR))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > NET_ADDR_SIZE)
                    {
                        strLen = NET_ADDR_SIZE;
                    }
                    memset(pdevice_obj->addr, 0, NET_ADDR_SIZE);
                    memcpy(pdevice_obj->addr, strTemp, NET_ADDR_SIZE);
                    os_json_log("%s: %s", key, pdevice_obj->addr);

                }
                else if (!strcmp(key, KEY_MODELID))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > MODELID_SIZE)
                    {
                        strLen = MODELID_SIZE;
                    }
                    memset(pdevice_obj->modelid, 0, MODELID_SIZE);
                    memcpy(pdevice_obj->modelid, strTemp, MODELID_SIZE);
                    os_json_log("%s: %s", key, pdevice_obj->modelid);

                }
                else if (!strcmp(key, KEY_NAME))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > NAME_SIZE)
                    {
                        strLen = NAME_SIZE;
                    }
                    memset(pdevice_obj->name, 0, NAME_SIZE);
                    memcpy(pdevice_obj->name, strTemp, strlen(strTemp));
                    os_json_log("%s: %s", key, pdevice_obj->name);

                }
                else if (!strcmp(key, KEY_COMM))
                {
                    pdevice_obj->comm = json_object_get_int(val);
                    os_json_log("%s: %d", key, pdevice_obj->comm);

                }
                else if (!strcmp(key, KEY_CHANNEL))
                {
                    pdevice_obj->ch = json_object_get_int(val);
                    os_json_log("%s: %d", key, pdevice_obj->ch);
                }
                else if (!strcmp(key, KEY_DEVTYPE))
                {
                    pdevice_obj->devtype = json_object_get_int(val);
                    os_json_log("%s: %d", key, pdevice_obj->devtype);
                }
                else if (!strcmp(key, KEY_STATUS))
                {
                    pdevice_obj->status.statusInfo = json_object_get_int(val);
                    os_json_log("%s: %d", key, pdevice_obj->status.statusInfo);
                }
                else if (!strcmp(key, KEY_OFFLINE))
                {
                    pdevice_obj->offline = json_object_get_int(val);
                }

            }

    return kNoErr;
}

/********************************************************
 * function: json_pack_simple_device_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_simple_device_obj(PSIMPLEDEVICEOBJ_T pSimpleDev_obj, json_object *package_object)
{
    char deviceId[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};

    if ((pSimpleDev_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }


    memcpy(deviceId, pSimpleDev_obj->sid, DEVICEID_SIZE);
    memcpy(name, pSimpleDev_obj->name, NAME_SIZE);

    json_object_object_add(package_object, KEY_SID,      json_object_new_string(deviceId));
    json_object_object_add(package_object, KEY_NAME,       json_object_new_string(name));
    json_object_object_add(package_object, KEY_DEVTYPE,    json_object_new_int(pSimpleDev_obj->devtype));
    json_object_object_add(package_object, KEY_COMM,       json_object_new_int(pSimpleDev_obj->comm));
    json_object_object_add(package_object, KEY_STATUS,     json_object_new_int(pSimpleDev_obj->status.statusInfo));

    return kNoErr;
}

/********************************************************
 * function: json_pack_device_status_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_device_status_obj(PDEVSTATUSOBJ_T pDevStatus_obj, json_object *package_object)
{
    char deviceId[DEVICEID_SIZE+1] = {0};

    if ((pDevStatus_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }


    memcpy(deviceId, pDevStatus_obj->devid, DEVICEID_SIZE);

    json_object_object_add(package_object, KEY_DEVID,      json_object_new_string(deviceId));
    json_object_object_add(package_object, KEY_STATUS,     json_object_new_int(pDevStatus_obj->status.statusInfo));

    return kNoErr;
}

/********************************************************
 * function: json_pack_room_obj
 * description:
 * input:   1. proom_obj:               room object struct
 * output:  1. package_object:                  the final object provided to caller
 *          2. json_array_object:
 *          3. json_subobject:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_room_obj(PROOMOBJ_T proom_obj, json_object *package_object)
{
    if ((proom_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }
    char name[NAME_SIZE+1] = {0};
    memcpy(name, proom_obj->name, NAME_SIZE);
    json_object_object_add(package_object, KEY_OBJECTID,     json_object_new_string(proom_obj->roomId));
    json_object_object_add(package_object, KEY_NAME,         json_object_new_string(name));
    json_object_object_add(package_object, KEY_DEVLIST,      json_object_new_string(proom_obj->devidList));

    return kNoErr;
}

/********************************************************
 * function: json_unpack_room_obj
 * description:
 * input:   1. pjson_object:               json object
 * output:  1. proom_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_room_obj(json_object *pjson_object, PROOMOBJ_T proom_obj)
{
    if ((proom_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(proom_obj, 0, sizeof(ROOMOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NAME_SIZE)
                {
                    strLen = NAME_SIZE;
                }
                memset(proom_obj->name, 0, NAME_SIZE);
                memcpy(proom_obj->name, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_OBJECTID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(proom_obj->roomId))
                {
                    strLen = sizeof(proom_obj->roomId);
                }
                memset(proom_obj->roomId, 0, sizeof(proom_obj->roomId));
                memcpy(proom_obj->roomId, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_DEVLIST))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(proom_obj->devidList))
                {
                    strLen = sizeof(proom_obj->devidList);
                }
                memset(proom_obj->devidList, 0, sizeof(proom_obj->devidList));
                memcpy(proom_obj->devidList, strTemp, strlen(strTemp));
            }
        }
    return kNoErr;
}

/********************************************************
 * function: json_pack_scene_obj
 * description:
 * input:   1. proom_obj:               room object struct
 * output:  1. package_object:                  the final object provided to caller
 *          2. json_array_object:
 *          3. json_subobject:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_scene_obj(PSCENEOBJ_T pscene_obj, json_object *package_object)
{
    if ((pscene_obj == NULL) || (package_object == NULL) )
    {
        return kGeneralErr;
    }
    char name[NAME_SIZE+1] = {0};
    memcpy(name, pscene_obj->sceneName, NAME_SIZE);
    json_object_object_add(package_object, KEY_OBJECTID,     json_object_new_string(pscene_obj->sceneId));
    json_object_object_add(package_object, KEY_NAME,         json_object_new_string(name));

    return kNoErr;
}

/********************************************************
 * function: json_pack_scene_obj
 * description:
 * input:   1. proom_obj:               room object struct
 * output:  1. package_object:                  the final object provided to caller
 *          2. json_array_object:
 *          3. json_subobject:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_scene_obj(json_object *pjson_object, PSCENEOBJ_T pscene_obj)
{
    if ((pscene_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pscene_obj, 0, sizeof(SCENEOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NAME_SIZE)
                {
                    strLen = NAME_SIZE;
                }
                memset(pscene_obj->sceneName, 0, NAME_SIZE);
                memcpy(pscene_obj->sceneName, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_OBJECTID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DEVICEID_SIZE)
                {
                    strLen = DEVICEID_SIZE;
                }
                memset(pscene_obj->sceneId, 0, DEVICEID_SIZE);
                memcpy(pscene_obj->sceneId, strTemp, strlen(strTemp));
            }
//            else if (!strcmp(key, KEY_ACTIONLIST))
//            {
//                array_list *list = NULL;
//                uint8_t num = 0;
//                list = json_object_get_array(val);
//                pscene_obj->actionObjNum = list->length;
//                for (num=0; num<list->length; num++)
//                {
//                    json_object_object_foreach(list->array[num], subkey, subval)
//                    {
//                        if (!strcmp(key, KEY_CLASSID))
//                        {
//                            pscene_obj->ActionObj[num].classid = json_object_get_int(subval);
//                        }
//                        else if (!strcmp(key, KEY_OBJECTID))
//                        {
//                            pscene_obj->ActionObj[num].id = json_object_get_int(subval);
//                        }
//                        else if (!strcmp(key, KEY_ACTIONCODE))
//                        {
//                            pscene_obj->ActionObj[num].id = json_object_get_int(subval);
//                        }
//                        else if (!strcmp(key, KEY_KEY))
//                        {
//                            pscene_obj->ActionObj[num].key = json_object_get_int(subval);
//                        }
//                        else if (!strcmp(key, KEY_VALUE))
//                        {
//                            strTemp = json_object_get_string(subval);
//                            memcpy(pscene_obj->ActionObj[num].value, strTemp, strlen(strTemp));
//                        }
//                    }
//                }
//            }
        }
    return kNoErr;
}


/********************************************************
 * function: json_pack_device_obj
 * description:
 * input:   1. pevent_obj:      event object struct
 * output:  1. package_object:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_event_obj(PEVENTOBJ_T pevent_obj, json_object *package_object)
{
    if ((pevent_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    char name[NAME_SIZE+1] = {0};
    memcpy(name, pevent_obj->name, NAME_SIZE);
    json_object_object_add(package_object, KEY_KEY,         json_object_new_string(pevent_obj->key));
    json_object_object_add(package_object, KEY_NAME,        json_object_new_string(name));
    json_object_object_add(package_object, KEY_OBJECTID,    json_object_new_string(pevent_obj->id));
    json_object_object_add(package_object, KEY_NOTITYPE,    json_object_new_int(pevent_obj->notitype));
    json_object_object_add(package_object, KEY_ENABLED,     json_object_new_int(pevent_obj->enabled));
    json_object_object_add(package_object, KEY_EVENTLEVEL,  json_object_new_int(pevent_obj->eventlevel));
    json_object_object_add(package_object, KEY_EVENTTYPE,   json_object_new_int(pevent_obj->eventtype));

    return kNoErr;
}

/********************************************************
 * function: json_unpack_event_obj
 * description:
 * input:   1. proom_obj:               room object struct
 * output:  1. package_object:                  the final object provided to caller
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_event_obj(json_object *pjson_object, PEVENTOBJ_T pevent_obj)
{
    if ((pevent_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pevent_obj, 0, sizeof(EVENTOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_ID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pevent_obj->id))
                {
                    strLen = sizeof(pevent_obj->id)-1;
                }
                memset(pevent_obj->id, 0, sizeof(pevent_obj->id));
                memcpy(pevent_obj->id, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pevent_obj->name))
                {
                    strLen = sizeof(pevent_obj->name)-1;
                }
                memset(pevent_obj->name, 0, sizeof(pevent_obj->name));
                memcpy(pevent_obj->name, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_KEY))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pevent_obj->key))
                {
                    strLen = sizeof(pevent_obj->key)-1;
                }
                memset(pevent_obj->key, 0, sizeof(pevent_obj->key));
                memcpy(pevent_obj->key, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_EVENTLEVEL))
            {
                pevent_obj->eventlevel = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_EVENTTYPE))
            {
                pevent_obj->eventtype = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_ENABLED))
            {
                pevent_obj->enabled = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_NOTITYPE))
            {
                pevent_obj->notitype = json_object_get_int(val);
            }
        }
    return kNoErr;
}


/********************************************************
 * function: json_pack_cloudparam_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_cloudparam_obj(PCLOUDOBJ_T pcloud_obj, json_object *package_object)
{
    if ((pcloud_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    json_object_object_add(package_object, KEY_APPID,       json_object_new_string(pcloud_obj->appid));
    json_object_object_add(package_object, KEY_ASK,         json_object_new_string(pcloud_obj->ask));
    json_object_object_add(package_object, KEY_CLOUDHOST,   json_object_new_string(pcloud_obj->cloudhost));
    json_object_object_add(package_object, KEY_DSK,         json_object_new_string(pcloud_obj->dsk));
    json_object_object_add(package_object, KEY_PSK,         json_object_new_string(pcloud_obj->psk));

    return kNoErr;
}

/********************************************************
 * function: json_unpack_cloudparam_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pschedule_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_cloudparam_obj(json_object *pjson_object, PCLOUDOBJ_T pcloud_obj)
{
    if ((pcloud_obj == NULL) || (pjson_object == NULL))
    {
        return kGeneralErr;
    }

    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pcloud_obj, 0, sizeof(CLOUDOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_APPID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pcloud_obj->appid))
                {
                    strLen = sizeof(pcloud_obj->appid)-1;
                }
                memcpy(pcloud_obj->appid, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_ASK))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pcloud_obj->ask))
                {
                    strLen = sizeof(pcloud_obj->ask)-1;
                }
                memcpy(pcloud_obj->ask, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_CLOUDHOST))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pcloud_obj->cloudhost))
                {
                    strLen = sizeof(pcloud_obj->cloudhost)-1;
                }
                memcpy(pcloud_obj->cloudhost, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_DSK))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pcloud_obj->dsk))
                {
                    strLen = sizeof(pcloud_obj->dsk)-1;
                }
                memcpy(pcloud_obj->dsk, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_PSK))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pcloud_obj->psk))
                {
                    strLen = sizeof(pcloud_obj->psk)-1;
                }
                memcpy(pcloud_obj->psk, strTemp, strlen(strTemp));
            }
        }

    return kNoErr;
}


/********************************************************
 * function: json_pack_schedule_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_schedule_obj(PSCHEDULEOBJ_T pschedule_obj, json_object *package_object)
{
    if ((pschedule_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    char name[NAME_SIZE+1] = {0};
    memcpy(name, pschedule_obj->name, NAME_SIZE);
    json_object_object_add(package_object, KEY_DT,          json_object_new_string(pschedule_obj->dt));
    json_object_object_add(package_object, KEY_NAME,        json_object_new_string(name));
    json_object_object_add(package_object, KEY_FLAG,        json_object_new_int(pschedule_obj->flag));
    json_object_object_add(package_object, KEY_OBJECTID,    json_object_new_int(pschedule_obj->id));
    json_object_object_add(package_object, KEY_TYPE,        json_object_new_int(pschedule_obj->type));

    return kNoErr;
}

/********************************************************
 * function: json_unpack_schedule_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pschedule_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_schedule_obj(json_object *pjson_object, PSCHEDULEOBJ_T pschedule_obj)
{
    if ((pschedule_obj == NULL) || (pjson_object == NULL))
    {
        return kGeneralErr;
    }

    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pschedule_obj, 0, sizeof(SCHEDULEOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NAME_SIZE)
                {
                    strLen = NAME_SIZE;
                }
                memcpy(pschedule_obj->name, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_DT))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > DT_SIZE)
                {
                    strLen = DT_SIZE;
                }
                memcpy(pschedule_obj->dt, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_FLAG))
            {
                pschedule_obj->flag = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_OBJECTID))
            {
                pschedule_obj->id = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_TYPE))
            {
                pschedule_obj->type = json_object_get_int(val);
            }
        }

    return kNoErr;
}

/********************************************************
 * function:  json_pack_simpdevice_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_simpdevice_obj(PSIMPLEDEVICEOBJ_T psipdev_obj, json_object *package_object)
{
    if ((psipdev_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    char name[NAME_SIZE+1] = {0};
    memcpy(name, psipdev_obj->name, NAME_SIZE);

    json_object_object_add(package_object, KEY_SID,           json_object_new_string(psipdev_obj->sid));
    json_object_object_add(package_object, KEY_NAME,          json_object_new_string(name));
    json_object_object_add(package_object, KEY_DEVTYPE,       json_object_new_int(psipdev_obj->devtype));
    json_object_object_add(package_object, KEY_COMM,          json_object_new_int(psipdev_obj->comm));
    json_object_object_add(package_object, KEY_STATUS,        json_object_new_int(psipdev_obj->status.statusInfo));

    return kNoErr;
}

/********************************************************
 * function:  json_pack_devstatus_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_devstatus_obj(PDEVSTATUSOBJ_T pdev_status_obj, json_object *package_object)
{
    char deviceId[DEVICEID_SIZE] = {0};
    if ((pdev_status_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }
    memset(deviceId, 0, sizeof(deviceId));
    memcpy(deviceId, pdev_status_obj->devid, DEVICEID_SIZE);

    json_object_object_add(package_object, KEY_DEVID,           json_object_new_string(deviceId));
    json_object_object_add(package_object, KEY_STATUS,          json_object_new_int(pdev_status_obj->status.statusInfo));

    return kNoErr;
}

/********************************************************
 * function:  json_pack_devattr_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_devattr_obj(PDEVATTROBJ_T pdev_attr_obj, json_object *package_object)
{
    if ((pdev_attr_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    json_object_object_add(package_object, KEY_KEY,           json_object_new_string(pdev_attr_obj->key));
    json_object_object_add(package_object, KEY_VALTYPE,       json_object_new_int(pdev_attr_obj->valtype));
    json_object_object_add(package_object, KEY_VALUNIT,       json_object_new_int(pdev_attr_obj->valunit));
    json_object_object_add(package_object, KEY_EVENTID,       json_object_new_int(pdev_attr_obj->eventid));
    json_object_object_add(package_object, KEY_LINKID,        json_object_new_int(pdev_attr_obj->linkid));
    json_object_object_add(package_object, KEY_VALUE,         json_object_new_string(pdev_attr_obj->value));

    return kNoErr;
}

/********************************************************
 * function:  json_pack_devop_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_devop_obj(PDEV_FUNCTION_OBJ_T pdev_oper_obj, json_object *package_object)
{
    if ((pdev_oper_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    json_object_object_add(package_object, KEY_KEY,           json_object_new_string(pdev_oper_obj->key));
    json_object_object_add(package_object, KEY_VALTYPE,       json_object_new_int(pdev_oper_obj->valtype));
    json_object_object_add(package_object, KEY_TYPE,          json_object_new_int(pdev_oper_obj->type));
    json_object_object_add(package_object, KEY_VALUE,         json_object_new_string(pdev_oper_obj->value));

    return kNoErr;
}

/********************************************************
 * function:  json_pack_devop_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_devop_obj_ex(PDEV_FUNCTION_OBJ_EX_T pdev_oper_obj, json_object *package_object)
{
    if ((pdev_oper_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    json_object_object_add(package_object, KEY_KEY,           json_object_new_string(pdev_oper_obj->key));
    json_object_object_add(package_object, KEY_VALTYPE,       json_object_new_int(pdev_oper_obj->valtype));
    json_object_object_add(package_object, KEY_TYPE,          json_object_new_int(pdev_oper_obj->type));
    json_object_object_add(package_object, KEY_VALUE,         json_object_new_string(pdev_oper_obj->value));

    return kNoErr;
}



/********************************************************
 * function: json_unpack_devattr_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_devattr_obj(json_object *pjson_object, PDEVATTROBJ_T pdevattr_obj)
{
    if ((pdevattr_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pdevattr_obj, 0, sizeof(DEVATTROBJ_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NAME_SIZE)
                {
                    strLen = NAME_SIZE;
                }
                memset(pdevattr_obj->name, 0, NAME_SIZE);
                memcpy(pdevattr_obj->name, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_VALUE))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > VALUE_SIZE)
                {
                    strLen = VALUE_SIZE;
                }
                memset(pdevattr_obj->value, 0, VALUE_SIZE);
                memcpy(pdevattr_obj->value, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_EVENTID))
            {
                pdevattr_obj->eventid = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_KEY))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > KEY_SIZE)
                {
                    strLen = KEY_SIZE;
                }
                memset(pdevattr_obj->key, 0, KEY_SIZE);
                memcpy(pdevattr_obj->key, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_LINKID))
            {
                pdevattr_obj->linkid = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_VALTYPE))
            {
                pdevattr_obj->valtype = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_VALUNIT))
            {
                pdevattr_obj->valunit = json_object_get_int(val);
            }
        }
    return kNoErr;
}

/********************************************************
 * function: json_unpack_devattr_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_devop_obj(json_object *pjson_object, PDEV_FUNCTION_OBJ_T pdevop_obj)
{
    if ((pdevop_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pdevop_obj, 0, sizeof(DEV_FUNCTION_OBJ_T));
    pdevop_obj->type = 3;                   // default value
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_KEY))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > KEY_SIZE)
                {
                    strLen = KEY_SIZE;
                }
                memset(pdevop_obj->key, 0, KEY_SIZE);
                memcpy(pdevop_obj->key, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_VALUE))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > VALUE_SIZE_EX)
                {
                    strLen = VALUE_SIZE_EX-1;
                }
                memset(pdevop_obj->value, 0, VALUE_SIZE_EX);
                memcpy(pdevop_obj->value, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_TYPE))
            {
                pdevop_obj->type = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_VALTYPE))
            {
                pdevop_obj->valtype = json_object_get_int(val);
            }
        }
    return kNoErr;
}

/********************************************************
 * function: json_unpack_devattr_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_devop_obj_ex(json_object *pjson_object, PDEV_FUNCTION_OBJ_EX_T pdevop_obj)
{
    if ((pdevop_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pdevop_obj, 0, sizeof(DEV_FUNCTION_OBJ_EX_T));
    pdevop_obj->type = 3;                   // default value
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_KEY))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > KEY_SIZE)
                {
                    strLen = KEY_SIZE;
                }
                memset(pdevop_obj->key, 0, KEY_SIZE);
                memcpy(pdevop_obj->key, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_VALUE))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > VALUE_SIZE_EX_1)
                {
                    strLen = VALUE_SIZE_EX_1-1;
                }
                memset(pdevop_obj->value, 0, VALUE_SIZE_EX_1);
                memcpy(pdevop_obj->value, strTemp, strlen(strTemp));
            }
            else if (!strcmp(key, KEY_TYPE))
            {
                pdevop_obj->type = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_VALTYPE))
            {
                pdevop_obj->valtype = json_object_get_int(val);
            }
        }
    return kNoErr;
}


/********************************************************
 * function: json_pack_linkage_input_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_linkage_input(PLINKAGE_INPUT_T plnk_input, json_object *package_object)
{
    if (plnk_input == NULL || package_object == NULL)
    {
        return kGeneralErr;
    }

    json_object_object_add(package_object, KEY_INDEX,         json_object_new_int(plnk_input->index));
    json_object_object_add(package_object, KEY_LOGIC,         json_object_new_string(plnk_input->logic));
}

/********************************************************
 * function: json_pack_linkage_input_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_linkage_input_obj(PLINKAGE_INPUT_TASK_T plnk_input_task, json_object *package_object)
{
    uint8_t linkageInputCount = 0;
    struct json_object *jsonObjectData = NULL;
    struct json_object *jsonSubObjectArray = NULL;

    if ((plnk_input_task == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    if ((plnk_input_task->linkageId[0] >= ' ') && (plnk_input_task->linkageId[0] <= '~'))
    {
        json_object_object_add(package_object, KEY_ID,         json_object_new_string(plnk_input_task->linkageId));
    }
    json_object_object_add(package_object, KEY_CFLAG,         json_object_new_int(plnk_input_task->cflag));

    if (plnk_input_task->inputNum != 0)
    {
        jsonSubObjectArray = json_object_new_array();
        for (linkageInputCount = 0; linkageInputCount < plnk_input_task->inputNum; linkageInputCount++)
        {
            jsonObjectData = json_object_new_object();
            json_pack_linkage_input(&plnk_input_task->lnkInput[linkageInputCount], jsonObjectData);
            json_object_array_add(jsonSubObjectArray, jsonObjectData);
        }
        json_object_object_add(package_object, KEY_INPUT,         jsonSubObjectArray);

    }


    return kNoErr;
}

/********************************************************
 * function: json_unpack_linkage_input_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. plinkage_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_linkage_input_obj(json_object *pjson_object, PLINKAGE_INPUT_TASK_T plinkage_obj)
{
    if ((plinkage_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(plinkage_obj, 0, sizeof(LINKAGE_INPUT_TASK_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_ID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > LINKID_SIZE)
                {
                    strLen = LINKID_SIZE-1;
                }
                memset(plinkage_obj->linkageId, 0, LINKID_SIZE);
                memcpy(plinkage_obj->linkageId, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_CFLAG))
            {
                plinkage_obj->cflag = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_INPUT))
            {
                array_list *list = NULL;
                uint8_t num = 0;
                list = json_object_get_array(val);
                plinkage_obj->inputNum = list->length;
                for (num=0; num<list->length; num++)
                {
                    json_object_object_foreach(list->array[num], subkey, subval)
                    {
                        if (!strcmp(subkey, KEY_INDEX))
                        {
                            plinkage_obj->lnkInput[num].index = json_object_get_int(subval);
                        }
                        else if (!strcmp(subkey, KEY_LOGIC))
                        {
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > INPUT_SIZE)
                            {
                                strLen = INPUT_SIZE - 1;
                            }
                            memset(plinkage_obj->lnkInput[num].logic, 0, INPUT_SIZE);
                            memcpy(plinkage_obj->lnkInput[num].logic, strTemp, strLen);
                        }
                    }
                }
            }
        }
    return kNoErr;
}

/********************************************************
 * function: json_pack_linkage_output_obj
 * description:
 * input:   1. plnk_output_task:
 * output:  1. package_object:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_linkage_output_obj(PLINKAGE_OUTPUT_TASK_T plnk_output_task, json_object *package_object)
{
    uint8_t linkageOutputCount = 0;
    struct json_object *jsonObjectData = NULL;
    struct json_object *jsonSubObjectArray = NULL;

    if ((plnk_output_task == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    if ((plnk_output_task->linkageId[0] >= ' ') && (plnk_output_task->linkageId[0] <= '~'))
    {
        json_object_object_add(package_object, KEY_ID,         json_object_new_string(plnk_output_task->linkageId));
    }
    json_object_object_add(package_object, KEY_NAME,         json_object_new_string(plnk_output_task->name));
    json_object_object_add(package_object, KEY_CFLAG,         json_object_new_int(plnk_output_task->cflag));

    if (plnk_output_task->outputNum != 0)
    {
        jsonSubObjectArray = json_object_new_array();
        for (linkageOutputCount = 0; linkageOutputCount < plnk_output_task->outputNum; linkageOutputCount++)
        {
            jsonObjectData = json_object_new_object();
            json_object_object_add(jsonObjectData, KEY_DO,      json_object_new_string(plnk_output_task->output[linkageOutputCount]));
            json_object_array_add(jsonSubObjectArray, jsonObjectData);
        }
        json_object_object_add(package_object, KEY_OUTPUT,         jsonSubObjectArray);
    }

    return kNoErr;
}



/********************************************************
 * function: json_unpack_linkage_output_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. plinkage_obj:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_linkage_output_obj( json_object *pjson_object, PLINKAGE_OUTPUT_TASK_T plinkage_obj )
{
    if ((plinkage_obj == NULL) ||(pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(plinkage_obj, 0, sizeof(LINKAGE_OUTPUT_TASK_T));
    json_object_object_foreach(pjson_object, key, val){
            os_json_log("%s", key);
            if (!strcmp(key, KEY_ID))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > LINKID_SIZE)
                {
                    strLen = LINKID_SIZE-1;
                }
                memset(plinkage_obj->linkageId, 0, LINKID_SIZE);
                memcpy(plinkage_obj->linkageId, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_NAME))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > NAME_SIZE)
                {
                    strLen = NAME_SIZE-1;
                }
                memset(plinkage_obj->name, 0, NAME_SIZE);
                memcpy(plinkage_obj->name, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_CFLAG))
            {
                plinkage_obj->cflag = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_OUTPUT))
            {
                array_list *list = NULL;
                uint8_t num = 0;
                list = json_object_get_array(val);
                plinkage_obj->outputNum = list->length;
                for (num=0; num<list->length; num++)
                {
                    json_object_object_foreach(list->array[num], subkey, subval)
                    {

                        if (!strcmp(subkey, KEY_DO))
                        {
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > OUTPUT_SIZE)
                            {
                                strLen = OUTPUT_SIZE - 1;
                            }
                            memset(plinkage_obj->output[num], 0, OUTPUT_SIZE);
                            memcpy(plinkage_obj->output[num], strTemp, strLen);
                        }
                    }
                }
            }
        }
    return kNoErr;
}


/********************************************************
 * function: json_pack_netdev_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_netdev_obj(PNETDEVOBJ_T pdevice_obj, json_object *package_object)
{
    char deviceId[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};

    if ((pdevice_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }


    memcpy(deviceId, pdevice_obj->deviceId, DEVICEID_SIZE);
    memcpy(name, pdevice_obj->name, NAME_SIZE);

    json_object_object_add(package_object, KEY_DEVID,      json_object_new_string(deviceId));
    json_object_object_add(package_object, KEY_NAME,       json_object_new_string(name));
    json_object_object_add(package_object, KEY_MAC,        json_object_new_string(pdevice_obj->mac));
    json_object_object_add(package_object, KEY_ADDR,       json_object_new_string(pdevice_obj->addr));
    json_object_object_add(package_object, KEY_MODELID,    json_object_new_string(pdevice_obj->modelid));
    json_object_object_add(package_object, KEY_UUID,       json_object_new_string(pdevice_obj->uuid));
    json_object_object_add(package_object, KEY_CHANNEL,    json_object_new_int(pdevice_obj->ch));
    json_object_object_add(package_object, KEY_DEVTYPE,    json_object_new_int(pdevice_obj->devtype));
    json_object_object_add(package_object, KEY_COMM,       json_object_new_int(pdevice_obj->comm));
    json_object_object_add(package_object, KEY_STATUS,     json_object_new_int(pdevice_obj->status.statusInfo));

    json_object_object_add(package_object, KEY_PSK,       json_object_new_string(pdevice_obj->psk));
    json_object_object_add(package_object, KEY_DSK,       json_object_new_string(pdevice_obj->dsk));

    json_object_object_add(package_object, KEY_APPID,       json_object_new_string(pdevice_obj->appid));
    json_object_object_add(package_object, KEY_ASK,       json_object_new_string(pdevice_obj->ask));

    json_object_object_add(package_object, KEY_CLOUDHOST,       json_object_new_string(pdevice_obj->cloudhost));
    json_object_object_add(package_object, KEY_MODELNAME,    json_object_new_string(pdevice_obj->modelname));

    if (pdevice_obj->room_uid[0] != 0)
    {
        json_object_object_add(package_object, KEY_ROOMUID, json_object_new_string(pdevice_obj->room_uid));
    }

    return kNoErr;
}

/********************************************************
 * function:  json_unpack_netdev_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_netdev_obj(json_object *pjson_object, PNETDEVOBJ_T pdevice_obj)
{
    os_json_log("\n");
    if ((pdevice_obj == NULL) || (pjson_object == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pdevice_obj, 0, sizeof(NETDEVOBJ_T));
    json_object_object_foreach(pjson_object, key, val){
                os_json_log("%s", key);
                if(!strcmp(key, KEY_UUID)){
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > UUID_SIZE)
                    {
                        strLen = UUID_SIZE-1;
                    }
                    memset(pdevice_obj->uuid, 0, UUID_SIZE);
                    memcpy(pdevice_obj->uuid, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_DEVID))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > DEVICEID_SIZE)
                    {
                        strLen = DEVICEID_SIZE;
                    }
                    memset(pdevice_obj->deviceId, 0, DEVICEID_SIZE);
                    memcpy(pdevice_obj->deviceId, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_MAC))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > MAC_SIZE)
                    {
                        strLen = MAC_SIZE-1;
                    }
                    memset(pdevice_obj->mac, 0, MAC_SIZE);
                    memcpy(pdevice_obj->mac, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_ADDR))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > sizeof(pdevice_obj->addr))
                    {
                        strLen = 2;
                    }
                    memset(pdevice_obj->addr, 0, sizeof(pdevice_obj->addr));
                    memcpy(pdevice_obj->addr, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_MODELID))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > MODELID_SIZE)
                    {
                        strLen = MODELID_SIZE-1;
                    }
                    memset(pdevice_obj->modelid, 0, MODELID_SIZE);
                    memcpy(pdevice_obj->modelid, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_MODELNAME))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > sizeof(pdevice_obj->modelname))
                    {
                        strLen = sizeof(pdevice_obj->modelname)-1;
                    }
                    memset(pdevice_obj->modelname, 0, sizeof(pdevice_obj->modelname));
                    memcpy(pdevice_obj->modelname, strTemp, strLen);
                }

                else if (!strcmp(key, KEY_NAME))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > NAME_SIZE)
                    {
                        strLen = NAME_SIZE-1;
                    }
                    memset(pdevice_obj->name, 0, NAME_SIZE);
                    memcpy(pdevice_obj->name, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_COMM))
                {
                    pdevice_obj->comm = json_object_get_int(val);
                }
                else if (!strcmp(key, KEY_CHANNEL))
                {
                    pdevice_obj->ch = json_object_get_int(val);
                }
                else if (!strcmp(key, KEY_DEVTYPE))
                {
                    pdevice_obj->devtype = json_object_get_int(val);
                }
                else if (!strcmp(key, KEY_STATUS))
                {
                    pdevice_obj->status.statusInfo = json_object_get_int(val);
                }
                else if (!strcmp(key, KEY_APPID))
				{
                	 strTemp = json_object_get_string(val);
					strcpy(pdevice_obj->appid ,strTemp);
				}
                else if (!strcmp(key, KEY_ASK))
				{
                	strTemp = json_object_get_string(val);
					strcpy(pdevice_obj->ask ,strTemp);
				}
                else if (!strcmp(key, KEY_DSK))
				{
               	 strTemp = json_object_get_string(val);
					strcpy(pdevice_obj->dsk ,strTemp);
				}
                else if (!strcmp(key, KEY_PSK))
				{
                	strTemp = json_object_get_string(val);
					strcpy(pdevice_obj->psk ,strTemp);
				}
                else if (!strcmp(key, KEY_CLOUDHOST))
				{
                	strTemp = json_object_get_string(val);
					strcpy(pdevice_obj->cloudhost ,strTemp);
				}
                else if (!strcmp(key, KEY_ROOMUID))
                {
                    strTemp = json_object_get_string(val);
                    strcpy(pdevice_obj->room_uid ,strTemp);
                }
    }
    return kNoErr;
}

#ifdef VERSION_ENDPOINT_12
/********************************************************
 * function: json_pack_endpoint_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_endpoint_obj(PENDPOINTOBJ_EX_T pendpoint_obj, json_object *package_object, json_object *json_array_object, json_object **json_subobject)
{
    char fid[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};
    int endpointIndex=0;

    if ((pendpoint_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    memcpy(fid, pendpoint_obj->fid, DEVICEID_SIZE);
    json_object_object_add(package_object, KEY_FID,         json_object_new_string(fid));
    json_object_object_add(package_object, KEY_ENDPOINT,    json_array_object);

    for (endpointIndex=0; endpointIndex < pendpoint_obj->endpointNum; endpointIndex++)
    {
        memcpy(name, pendpoint_obj->endpointlist[endpointIndex].name, NAME_SIZE);
        json_object_array_add(json_array_object, *(json_subobject+endpointIndex));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_ID,       json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].id));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_NAME,     json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].name));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_DEVTYPE,  json_object_new_int(pendpoint_obj->endpointlist[endpointIndex].devtype));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_KEYTYPE,  json_object_new_int(pendpoint_obj->endpointlist[endpointIndex].keytype));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_KEY,      json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].key));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_VALUE,      json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].value));
        os_json_log("endpointid:%s, index=%d", pendpoint_obj->endpointlist[endpointIndex].id, endpointIndex);
    }
    return kNoErr;
}
#else
/********************************************************
 * function: json_pack_endpoint_obj
 * description:
 * input:   1. data:            json data
 * output:  1. uuid:            param data
 *          2. classID:         the type of class ID
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_endpoint_obj(PSINGLE_DEV_ENDPOINTOBJ_T pendpoint_obj, json_object *package_object, json_object *json_array_object, json_object **json_subobject)
{
    char fid[DEVICEID_SIZE+1] = {0};
    char name[NAME_SIZE+1] = {0};
    int endpointIndex=0;

    if ((pendpoint_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    memcpy(fid, pendpoint_obj->fid, DEVICEID_SIZE);
    json_object_object_add(package_object, KEY_FID,         json_object_new_string(fid));
    json_object_object_add(package_object, KEY_ENDPOINT,    json_array_object);

    for (endpointIndex=0; endpointIndex < pendpoint_obj->endpointNum; endpointIndex++)
    {
        memcpy(name, pendpoint_obj->endpointlist[endpointIndex].name, NAME_SIZE);
        json_object_array_add(json_array_object, *(json_subobject+endpointIndex));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_ID,       json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].id));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_NAME,     json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].name));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_DEVTYPE,  json_object_new_int(pendpoint_obj->endpointlist[endpointIndex].devtype));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_KEYTYPE,  json_object_new_int(pendpoint_obj->endpointlist[endpointIndex].keytype));
        json_object_object_add(*(json_subobject+endpointIndex), KEY_KEY,      json_object_new_string(pendpoint_obj->endpointlist[endpointIndex].key));
        os_json_log("endpointid:%s, index=%d", pendpoint_obj->endpointlist[endpointIndex].id, endpointIndex);
    }
    return kNoErr;
}
#endif

/********************************************************
 * function:  json_unpack_endpoint_obj
 * description:
 * input:   1. data:            json data
 * output:  1. pEndpoint_obj:
 *   The member endpointlist of this variable will be malloc in the function \
 *   and please free it in the function who call this function
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_endpoint_obj(json_object *pjson_object, PENDPOINTOBJ_EX_T pEndpoint_obj)
{
    if ((pjson_object == NULL) || (pEndpoint_obj == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pEndpoint_obj, 0, sizeof(ENDPOINTOBJ_EX_T));
    json_object_object_foreach(pjson_object, key, val){
                if(!strcmp(key, KEY_FID)){
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > DEVICEID_SIZE)
                    {
                        strLen = DEVICEID_SIZE;
                    }
                    memset(pEndpoint_obj->fid, 0, DEVICEID_SIZE);
                    memcpy(pEndpoint_obj->fid, strTemp, strLen);
                    os_json_log("%s: %s", key, pEndpoint_obj->fid);
                }
                else if (!strcmp(key, KEY_ENDPOINT))
                {
                    array_list *EPList = json_object_get_array(val);
                    int num = 0;
                    os_json_log("%s: num=%d", key, EPList->length);

                    pEndpoint_obj->endpointNum = EPList->length;
                    if (pEndpoint_obj->endpointNum != 0)
                    {
                        //pEndpoint_obj->endpointlist = malloc(EPList->length * sizeof(ENDPOINT_T));
                        for (num=0; (num<EPList->length)&&(num<ENDPOINTLIST_NUM); num++)
                        {
                            memset(&pEndpoint_obj->endpointlist[num], 0, sizeof(ENDPOINT_EX_T));
                            json_object_object_foreach(EPList->array[num], subkey, subval)
                            {
    #ifdef VERSION_ENDPOINT_12
                                if (!strcmp(subkey, KEY_ID))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].id))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].id)-1;
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].id, strTemp, strLen);
                                    os_json_log("endpointid:%s", pEndpoint_obj->endpointlist[num].id);
                                }
                                else if (!strcmp(subkey, KEY_NAME))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].name))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].name);
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].name, strTemp, strlen(strTemp));
                                }
                                else if (!strcmp(subkey, KEY_KEY))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].key))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].key)-1;
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].key, strTemp, strlen(strTemp));
                                }
                                else if (!strcmp(subkey, KEY_DEVTYPE))
                                {
                                    pEndpoint_obj->endpointlist[num].devtype = json_object_get_int(subval);
                                }
                                else if (!strcmp(subkey, KEY_VALTYPE))
                                {
                                    pEndpoint_obj->endpointlist[num].keytype = json_object_get_int(subval);
                                }
                                else if (!strcmp(subkey, KEY_KEYTYPE))
                                {
                                    pEndpoint_obj->endpointlist[num].keytype = json_object_get_int(subval);
                                }
                                else if (!strcmp(subkey, KEY_VALUE))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].value))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].value);
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].value, strTemp, strlen(strTemp));
                                }
    #else
                                if (!strcmp(subkey, KEY_ID))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].endpoint.id))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].endpoint.id)-1;
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].endpoint.id, strTemp, strLen);
                                    os_json_log("endpointid:%s", pEndpoint_obj->endpointlist[num].endpoint.id);
                                }
                                else if (!strcmp(subkey, KEY_NAME))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].endpoint.name))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].endpoint.name);
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].endpoint.name, strTemp, strlen(strTemp));
                                }
                                else if (!strcmp(subkey, KEY_KEY))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].endpoint.key))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].endpoint.key)-1;
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].endpoint.key, strTemp, strlen(strTemp));
                                }
                                else if (!strcmp(subkey, KEY_DEVTYPE))
                                {
                                    pEndpoint_obj->endpointlist[num].endpoint.devtype = json_object_get_int(subval);
                                }
                                else if (!strcmp(subkey, KEY_KEYTYPE))
                                {
                                    pEndpoint_obj->endpointlist[num].endpoint.keytype = json_object_get_int(subval);
                                }
                                else if (!strcmp(subkey, KEY_VALUE))
                                {
                                    strTemp = json_object_get_string(subval);
                                    strLen = strlen(strTemp);
                                    if (strLen > sizeof(pEndpoint_obj->endpointlist[num].value))
                                    {
                                        strLen = sizeof(pEndpoint_obj->endpointlist[num].value);
                                    }
                                    memcpy(pEndpoint_obj->endpointlist[num].value, strTemp, strlen(strTemp));
                                }
    #endif
                                os_json_log("subkey: %s", subkey);
                            }
                        }
                    }
                }
    }
    return kNoErr;
}

/********************************************************
 * function: json_pack_endpointlink_obj
 * description:
 * input:   1. pendpointLink_obj:            json data
 * output:  1. package_object:            param data
 *          2. json_array_object:
 *          3. json_subobject:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_pack_endpointlink_obj( PENDPOINTLINK_OBJ_T pendpointLink_obj, json_object *package_object )
{
    char linkid[LINKIDEX_SIZE+1] = {0};

    if ((pendpointLink_obj == NULL) || (package_object == NULL))
    {
        return kGeneralErr;
    }

    memcpy(linkid, pendpointLink_obj->linkId, LINKIDEX_SIZE);
    json_object_object_add(package_object, KEY_ID,       json_object_new_string(linkid));
    json_object_object_add(package_object, KEY_IN,       json_object_new_string(pendpointLink_obj->linkObj.inEndpointId));
    json_object_object_add(package_object, KEY_OUT,      json_object_new_string(pendpointLink_obj->linkObj.outEndpointId));
    json_object_object_add(package_object, KEY_CTRL,     json_object_new_string(pendpointLink_obj->linkObj.ctrlContent));

    return kNoErr;
}

/********************************************************
 * function:  json_unpack_endpoint_obj
 * description:
 * input:   1. data:            json data
 * output:  1. pEndpoint_obj:
 *   The member endpointlist of this variable will be malloc in the function \
 *   and please free it in the function who call this function
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_endpointlink_obj(json_object *pjson_object, PENDPOINTLINK_OBJ_T pEndpointLink_obj)
{
    if ((pjson_object == NULL) || (pEndpointLink_obj == NULL))
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    memset(pEndpointLink_obj, 0, sizeof(ENDPOINTLINK_OBJ_T));
    json_object_object_foreach(pjson_object, key, val){
                if(!strcmp(key, KEY_ID)){
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > LINKIDEX_SIZE)
                    {
                        strLen = LINKIDEX_SIZE-1;
                    }
                    memset(pEndpointLink_obj->linkId, 0, LINKIDEX_SIZE);
                    memcpy(pEndpointLink_obj->linkId, strTemp, strLen);
                    os_json_log("%s:%s", key, pEndpointLink_obj->linkId);
                }
                else if (!strcmp(key, KEY_IN))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > sizeof(pEndpointLink_obj->linkObj.inEndpointId))
                    {
                        strLen = sizeof(pEndpointLink_obj->linkObj.inEndpointId);
                    }
                    memcpy(pEndpointLink_obj->linkObj.inEndpointId, strTemp, strLen);
                }
                else if (!strcmp(key, KEY_OUT))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > sizeof(pEndpointLink_obj->linkObj.outEndpointId))
                    {
                        strLen = sizeof(pEndpointLink_obj->linkObj.outEndpointId);
                    }
                    memcpy(pEndpointLink_obj->linkObj.outEndpointId, strTemp, strlen(strTemp));
                }
                else if (!strcmp(key, KEY_CTRL))
                {
                    strTemp = json_object_get_string(val);
                    strLen = strlen(strTemp);
                    if (strLen > sizeof(pEndpointLink_obj->linkObj.ctrlContent))
                    {
                        strLen = sizeof(pEndpointLink_obj->linkObj.ctrlContent)-1;
                    }
                    memcpy(pEndpointLink_obj->linkObj.ctrlContent, strTemp, strlen(strTemp));
                }
    }
    return kNoErr;
}



/********************************************************
 * function: json_unpack_obj
 * description:
 * input:   1. classId:            json data
 * output:  1. json_object_object:
 *          2. pObjData:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_unpack_obj(int classId, json_object *json_object_object, char *pObjData)
{
    switch (classId)
    {
        case DEVICE_OBJ_ID:
            json_unpack_device_obj(json_object_object, (PDEVICEOBJ_T)pObjData);
            break;
        case SCENE_OBJ_ID:
            json_unpack_scene_obj(json_object_object, (PSCENEOBJ_T)pObjData);
            break;
        case ROOM_OBJ_ID:
            json_unpack_room_obj(json_object_object, (PROOMOBJ_T)pObjData);
            break;
        case GROUP_OBJ_ID:
            break;
        case CLOUDPARAM_OBJ_ID:
            json_unpack_cloudparam_obj(json_object_object, (PCLOUDOBJ_T)pObjData);
            break;
        case SCHEDULE_OBJ_ID:
            json_unpack_schedule_obj(json_object_object, (PSCHEDULEOBJ_T)pObjData);
            break;
        case NETDEV_OBJ_ID:
            json_unpack_netdev_obj(json_object_object, (PNETDEVOBJ_T)pObjData);
            break;
        case ENDPOINT_OBJ_ID:
            json_unpack_endpoint_obj(json_object_object, (PENDPOINTOBJ_EX_T)pObjData);
            break;
        case ENDPOINTLINK_OBJ_ID:
            json_unpack_endpointlink_obj(json_object_object, (PENDPOINTLINK_OBJ_T)pObjData);
            break;
        case EVENT_OBJ_ID:
            json_unpack_event_obj(json_object_object, (PEVENTOBJ_T)pObjData);
            break;
        case LINKAGE_INPUT_OBJ_ID:
            json_unpack_linkage_input_obj(json_object_object, (PLINKAGE_INPUT_TASK_T)pObjData);
            break;
        case LINKAGE_OUTPUT_OBJ_ID:
            json_unpack_linkage_output_obj(json_object_object, (PLINKAGE_OUTPUT_TASK_T)pObjData);
            break;
        default:
            break;
    }
}

/********************************************************
 * function:  json_unpack_msg_content
 * description:
 * input:   1. data:            json data
 * output:  1. pEndpoint_obj:
 *   The member endpointlist of this variable will be malloc in the function \
 *   and please free it in the function who call this function
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_msg_content(json_object *pjson_object, PMSG_CONTENT_T msg_content)
{
    if ((pjson_object == NULL) || (msg_content == NULL))
    {
        return kGeneralErr;
    }

    char *string = NULL;
    uint8_t strLen = 0;
    json_object_object_foreach(pjson_object, key, val){
        if (!strcmp(key, KEY_UUID))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(msg_content->uuid))
            {
                strLen = sizeof(msg_content->uuid)-1;
            }
            memcpy(msg_content->uuid, string, strLen);
        }
        else if (!strcmp(key, KEY_ENDPOINTID))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(msg_content->endpointid))
            {
                strLen = sizeof(msg_content->endpointid)-1;
            }
            memcpy(msg_content->endpointid, string, strLen);
        }
        else if (!strcmp(key, KEY_VALUE))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(msg_content->value))
            {
                strLen = sizeof(msg_content->value)-1;
            }
            memcpy(msg_content->value, string, strLen);
        }
        else if (!strcmp(key, KEY_KEY))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(msg_content->key))
            {
                strLen = sizeof(msg_content->key)-1;
            }
            memcpy(msg_content->key, string, strLen);
        }
    }
    return kNoErr;
}

/********************************************************
 * function:  json_unpack_msg_content
 * description:
 * input:   1. data:            json data
 * output:  1. pEndpoint_obj:
 *   The member endpointlist of this variable will be malloc in the function \
 *   and please free it in the function who call this function
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_unpack_msg_subdev_state(json_object *pjson_object, PSTATE_OBJ_T state_obj)
{
    if ((pjson_object == NULL) || (state_obj == NULL))
    {
        return;
    }

    char *string = NULL;
    uint8_t strLen = 0;
    json_object_object_foreach(pjson_object, key, val){
        if (!strcmp(key, KEY_UUID))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(state_obj->uuid))
            {
                strLen = sizeof(state_obj->uuid)-1;
            }
            memcpy(state_obj->uuid, string, strLen);
        }
        else if (!strcmp(key, KEY_ENDPOINTID))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(state_obj->endpointid))
            {
                strLen = sizeof(state_obj->endpointid)-1;
            }
            memcpy(state_obj->endpointid, string, strLen);
        }
        else if (!strcmp(key, KEY_STATUS))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(state_obj->status))
            {
                strLen = sizeof(state_obj->status)-1;
            }
            memcpy(state_obj->status, string, strLen);
        }
        else if (!strcmp(key, KEY_DEVID))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strlen(string) >= sizeof(state_obj->subdevid))
            {
                strLen = sizeof(state_obj->subdevid)-1;
            }
            memcpy(state_obj->subdevid, string, strLen);
        }
        else if (!strcmp(key, KEY_ADDR))
        {
            string = json_object_get_string(val);
            strLen = strlen(string);
            if (strLen > sizeof(state_obj->addr))
            {
                strLen = sizeof(state_obj->addr) - 1;
            }
            memcpy(state_obj->addr, string, strLen);
        }
    }
}

/********************************************************
 * function: mlcoap_get_ctrl_cmd_id
 * description: get command ID on the basis of param 'cmd'
 * input:   1. cmd
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
int json_get_ctrl_cmd_id(char *cmd)
{
    int cmd_id = COAP_SERVER_NOTI_CMD_ERR;
    // switch command string to command ID
    if (!strcmp(cmd, CTRL_READ))                   // get device object
    {
        cmd_id = COAP_CTRL_NOTI_CMD_READ;
    }
    else if (!strcmp(cmd, CTRL_READMULT))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_READMULT;
    }
    else if (!strcmp(cmd, CTRL_WRITE))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_WRITE;
    }
    else if (!strcmp(cmd, CTRL_WRITEMULT))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_WRITEMULT;
    }
    else if (!strcmp(cmd, CTRL_WRITEGROUP))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_WRITEGROUP;
    }
    else if (!strcmp(cmd, CTRL_DEVSET))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_DEVSET;
    }
    else if (!strcmp(cmd, CTRL_SYNCSTATUS))
    {
        cmd_id = COAP_CTRL_NOTI_CMD_SYNCSTATUS;
    }
    return cmd_id;
}

/********************************************************
 * function: json_get_manage_cmd_id
 * description: get command ID on the basis of param 'cmd'
 * input:   1. cmd
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
int json_get_manage_cmd_id(char *cmd)
{
    int cmd_id = COAP_SERVER_NOTI_CMD_ERR;
    // switch command string to command ID
    if (!strcmp(cmd, MANAGER_GET))                   // get device object
    {
        cmd_id = COAP_MANAGER_NOTI_CMD_GET;
    }
    else if (!strcmp(cmd, MANAGER_ADD))
    {
        cmd_id = COAP_MANAGER_NOTI_CMD_ADD;
    }
    else if (!strcmp(cmd, MANAGER_DEL))
    {
        cmd_id = COAP_MANAGER_NOTI_CMD_DEL;
    }
    else if (!strcmp(cmd, MANAGER_CLEAR))
    {
        cmd_id = COAP_MANAGER_NOTI_CMD_CLEAR;
    }
    else if (!strcmp(cmd, MANAGER_UPDATE))
    {
        cmd_id = COAP_MANAGER_NOTI_CMD_UPDATE;
    }
    return cmd_id;
}

/********************************************************
 * function: json_get_sys_cmd_id
 * description: get command ID on the basis of param 'cmd'
 * input:   1. cmd
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
int json_get_sys_cmd_id(char *cmd)
{
    int cmd_id = COAP_SERVER_NOTI_CMD_ERR;

    // switch command string to command ID
    if (!strcmp(cmd, SYS_SETMESH))                   // discover device
    {
        cmd_id = COAP_SYS_NOTI_CMD_SETMESH;
    }
    else if (!strcmp(cmd, SYS_FACTORY))
    {
        cmd_id = COAP_SYS_NOTI_CMD_FACTORY;
    }
    else if (!strcmp(cmd, SYS_RESET))
    {
        cmd_id = COAP_SYS_NOTI_CMD_RESET;
    }
    else if (!strcmp(cmd, SYS_SYNCTIME))
    {
        cmd_id = COAP_SYS_NOTI_CMD_SYNCTIME;
    }
    else if (!strcmp(cmd, SYS_GETDEVINFO))
    {
        cmd_id = COAP_SYS_NOTI_CMD_GETDEVINFO;
    }
    else if (!strcmp(cmd, SYS_SETNET))
    {
        cmd_id = COAP_SYS_NOTI_CMD_SETNET;
    }
    else if (!strcmp(cmd, SYS_SETCLOUD))
    {
        cmd_id = COAP_SYS_NOTI_CMD_SETCLOUD;
    }
    else if (!strcmp(cmd, SYS_SETADDR))
    {
        cmd_id = COAP_SYS_NOTI_CMD_SETADDR;
    }
    else if (!strcmp(cmd, SYS_KEEPALIVE))
    {
        cmd_id = COAP_SYS_NOTI_CMD_KEEPALIVE;
    }
    else if (!strcmp(cmd, SYS_UPGRADE))
    {
        cmd_id = COAP_SYS_NOTI_CMD_UPGRADE;
    }
    else if (!strcmp(cmd, SYS_RESETMESH))
    {
        cmd_id = COAP_SYS_NOTI_CMD_RESETMESH;
    }
    else if (!strcmp(cmd, SYS_INIT))
    {
        cmd_id = COAP_SYS_NOTI_CMD_MQTT_INIT;
    }
    else if (!strcmp(cmd, SYS_SENDADDR))
    {
        cmd_id = COAP_SYS_NOTI_CMD_SENDADDR;
    }
    else if (!strcmp(cmd, SYS_CLEARDATA))
    {
        cmd_id = COAP_SYS_NOTI_CMD_CLEARDATA;
    }

    return cmd_id;
}


/********************************************************
 * function: json_get_msg_cmd_id
 * description: get command ID on the basis of param 'cmd'
 * input:   1. cmd
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
int json_get_msg_cmd_id(char *cmd)
{
    int cmd_id = COAP_SERVER_NOTI_CMD_ERR;
    // switch command string to command ID
    if (!strcmp(cmd, MESSAGE_EVENT))                   // get device object
    {
        cmd_id = COAP_MSG_NOTI_CMD_EVENT;
    }
    else if (!strcmp(cmd, MESSAGE_LINKTASK))
    {
        cmd_id = COAP_MSG_NOTI_CMD_LINKTASK;
    }
    else if (!strcmp(cmd, MESSAGE_STATECHANGE))
    {
        cmd_id = COAP_MSG_NOTI_CMD_STATECHANGE;
    }
    else if (!strcmp(cmd, MESSAGE_CHANGENOTIFY))
    {
        cmd_id = COAP_MSG_NOTI_CMD_CHANGENOTIFY;
    }
    else if (!strcmp(cmd, MESSAGE_ALLOW_NOTIFY))
    {
        cmd_id = COAP_MSG_NOTI_CMD_ALLOW_NOTIFY;
    }

    return cmd_id;
}

