/*
 * json_ctrl_deal.c
 *
 *  Created on: 2017年7月17日
 *      Author: Administrator
 */

#include "mico.h"
#include "json_c/json.h"
#include "MLinkCommand.h"
#include "MLinkObject.h"
#include "../include/include_coap.h"

#define os_json_ctrl_log(M, ...) custom_log("JSON_CTRL", M, ##__VA_ARGS__)

#define DEV_OBJECT_NUM_MAX                  10
#define ATTR_OBJECT_NUM_MAX                 10
#define FUNCTION_OBJ_DEFAULT_TYPE               3


/************************************************************
 * unpack and get "control service" data
*************************************************************/

/********************************************************
 * function: json_ctrl_pack_devset_get_rsp
 * description:
 * input:   1. parse_json_object:
 * output:  1. pObjattr:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_pack_devset_get_rsp( int errcode, char *errmsg, PDEVSET_INFO_T devset_info, char *json_package )
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectBody = NULL;
    struct json_object *jsonObjectAttr = NULL;
    struct json_object *jsonObjectData[ATTR_OBJECT_NUM_MAX];
    char *jsonString = NULL;
    int count = 0;

    if ((json_package == NULL) || (devset_info == NULL))
    {
        os_json_ctrl_log(" json_package or devset_info is NULL!!!");
        return kGeneralErr;
    }

    jsonObjectService = json_object_new_object();
    jsonObjectBody = json_object_new_object();

    if (jsonObjectService == NULL || jsonObjectBody == NULL)
    {
        return kGeneralErr;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_DEVSET));
    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(errcode));
    json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectBody);
    json_object_object_add(jsonObjectBody, KEY_DEVID, json_object_new_string(devset_info->devid));
    json_object_object_add(jsonObjectBody, KEY_CMD, json_object_new_string(devset_info->cmd));
    json_object_object_add(jsonObjectBody, KEY_OBJECTTYPE, json_object_new_int(devset_info->objtype));
    if (devset_info->pageInfo[0] != 0)
    {
        json_object_object_add(jsonObjectBody, KEY_PAGES, json_object_new_string(devset_info->pageInfo));
    }

    if (errcode != MLINK_RESP_OK)
    {
        if (errmsg != NULL)
        {
            json_object_object_add(jsonObjectService, PUBLIC_ERRMSG, json_object_new_string(errmsg));
        }
    }
    else
    {
        jsonObjectAttr = json_object_new_array();

        if (jsonObjectAttr == NULL)
        {
            json_object_put(jsonObjectService);/*free memory*/
            return kGeneralErr;
        }
        json_object_object_add(jsonObjectBody, KEY_ATTR, jsonObjectAttr);

        for (count=0; count<devset_info->attrNum; count++)
        {
            jsonObjectData[count] = json_object_new_object();
            if (jsonObjectData[count] == NULL)
            {
                json_object_put(jsonObjectService);/*free memory*/
                return kGeneralErr;
            }
            json_object_array_add(jsonObjectAttr, jsonObjectData[count]);
            // add member of object
            json_pack_devop_obj(devset_info->opobj+count, jsonObjectData[count]);
        }
    }



    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return kGeneralErr;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_ctrl_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    for (count=0; count < devset_info->attrNum; count++)
    {
        jsonObjectData[count] = NULL;
    }
    jsonObjectBody = NULL;
    jsonObjectService = NULL;
    return kNoErr;
}

/********************************************************
 * function: json_pack_ctrl_response
 * description:
 * input:   pdev_attr_read
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_ctrl_read_response(int errcode, char *errmsg, PDEVATTR_READ_T pdev_attr_read, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectBody = NULL;
    struct json_object *jsonObjectData[ATTR_OBJECT_NUM_MAX];
    char *jsonString = NULL;
    int count = 0;

    if ((json_package == NULL) || (pdev_attr_read == NULL))
    {
        os_json_ctrl_log(" uuid or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();

    if (jsonObjectService == NULL)
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_READ));
    json_object_object_add(jsonObjectService, PUBLIC_ERRCODE, json_object_new_int(errcode));
    json_object_object_add(jsonObjectService, KEY_DEVID, json_object_new_string(pdev_attr_read->devid));

    if (errcode != MLINK_RESP_OK)
    {
        if (errmsg != NULL)
        {
            json_object_object_add(jsonObjectService, PUBLIC_ERRMSG, json_object_new_string(errmsg));
        }
    }
    else
    {
        jsonObjectBody = json_object_new_array();
        if (jsonObjectBody == NULL)
        {
            json_object_put(jsonObjectService);/*free memory*/
            return;
        }
        json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectBody);

        for (count=0; count<pdev_attr_read->attrnum; count++)
        {
            jsonObjectData[count] = json_object_new_object();
            if (jsonObjectData[count] == NULL)
            {
                json_object_put(jsonObjectService);/*free memory*/
                return;
            }
            json_object_array_add(jsonObjectBody, jsonObjectData[count]);
            // add member of object
            json_pack_devop_obj_ex(pdev_attr_read->devattrobj+count, jsonObjectData[count]);
        }
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_ctrl_log("json string : %s", jsonString);

    json_object_put(jsonObjectService);/*free memory*/
    for (count=0; count<pdev_attr_read->attrnum; count++)
    {
        jsonObjectData[count] = NULL;
    }
    jsonObjectBody = NULL;
    jsonObjectService = NULL;

}

/********************************************************
 * function: json_pack_ctrl_response
 * description:
 * input:   pdev_attr_read
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_ctrl_readmult_response(int errcode, char *errmsg, PDEVATTR_READMULT_T pdev_attr_readmult, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectBody = NULL;
    struct json_object *json_object_dev[DEV_OBJECT_NUM_MAX];
    struct json_object *json_object_dev_attr[DEV_OBJECT_NUM_MAX];
    struct json_object *json_object_attr[ATTR_OBJECT_NUM_MAX];
    char *jsonString = NULL;
    int dev_count = 0;

    if ((json_package == NULL) || (pdev_attr_readmult == NULL))
    {
        os_json_ctrl_log(" pdev_attr_readmult or json_package is NULL!!!");
        return;
    }

    jsonObjectService = json_object_new_object();


    if (jsonObjectService == NULL)
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_READMULT));
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
        jsonObjectBody = json_object_new_array();
        json_object_object_add(jsonObjectService, PUBLIC_BODY, jsonObjectBody);

        for (dev_count=0; dev_count<pdev_attr_readmult->devnum; dev_count++)
        {
            int attr_count = 0;
            json_object_dev[dev_count] = json_object_new_object();
            json_object_dev_attr[dev_count] = json_object_new_array();
            json_object_array_add(jsonObjectBody, json_object_dev[dev_count]);
            // add member of dev object
            json_object_object_add(json_object_dev[dev_count], KEY_DEVID, json_object_new_string(pdev_attr_readmult->devattrreadobj->devid));
            json_object_object_add(json_object_dev[dev_count], KEY_ATTR, json_object_dev_attr[dev_count]);

            for (attr_count = 0;attr_count<pdev_attr_readmult->devattrreadobj->attrnum; attr_count++)
            {
                json_object_attr[attr_count] = json_object_new_object();
                json_object_array_add(json_object_dev_attr[dev_count], json_object_attr[attr_count]);
                json_pack_devop_obj_ex(pdev_attr_readmult->devattrreadobj->devattrobj+attr_count, json_object_attr[attr_count]);
            }
        }
    }



    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return;
    }

    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_ctrl_log("json string : %s", jsonString);

//    json_object_put(jsonObjectData);/*free memory*/
    json_object_put(jsonObjectService);/*free memory*/

    jsonObjectBody = NULL;
    jsonObjectService = NULL;

}

/********************************************************
 * function: json_pack_ctrl_response
 * description:
 * input:   pdev_attr_read
 * output:  json_package
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_ctrl_write( PDEVATTR_WRITE_T pdevattr_write_struct, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *jsonObjectAttrArray = NULL;
    struct json_object *jsonObjectArrayMember = NULL;
    int attrIndex = 0;
    int attrNum = 0;
    char *jsonString = NULL;


    if ((pdevattr_write_struct == NULL) || (json_package == NULL))
    {
        return;
    }
    jsonObjectService = json_object_new_object();
    jsonObjectData = json_object_new_object();

    if ((jsonObjectService == NULL) || (jsonObjectData == NULL))
    {
        return;
    }
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_WRITE));
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    json_object_object_add(jsonObjectData, KEY_CLASSID, json_object_new_int(pdevattr_write_struct->classID));
    if (pdevattr_write_struct->devid[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_DEVID, json_object_new_string(pdevattr_write_struct->devid));
    }
    if (pdevattr_write_struct->netaddr[0] != 0)
    {
        json_object_object_add(jsonObjectData, KEY_ADDR, json_object_new_string(pdevattr_write_struct->netaddr));
    }

    if (pdevattr_write_struct->opobj != NULL)
    {
        PDEV_FUNCTION_OBJ_EX_T pdevFunObj = NULL;

        jsonObjectAttrArray = json_object_new_array();
        if (jsonObjectAttrArray == NULL)
        {
            json_object_put(jsonObjectService);/*free memory*/
            return;
        }
        if (pdevattr_write_struct->num > ENDPOINTLIST_NUM)
        {
            attrNum = ENDPOINTLIST_NUM;
        }
        else
        {
            attrNum = pdevattr_write_struct->num;
        }
        json_object_object_add(jsonObjectData, KEY_ATTR, jsonObjectAttrArray);

        for (attrIndex = 0; attrIndex < attrNum; attrIndex++)
        {
            pdevFunObj = pdevattr_write_struct->opobj + attrIndex;
            jsonObjectArrayMember = json_object_new_object();
            json_object_array_add(jsonObjectAttrArray, jsonObjectArrayMember);
            json_pack_devop_obj_ex(pdevFunObj, jsonObjectArrayMember);
        }
    }

    // object to string
    jsonString = json_object_to_json_string(jsonObjectService);

    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return;
    }
    memcpy(json_package, jsonString, strlen(jsonString));
    os_json_ctrl_log("json string : %s", json_package);

    json_object_put(jsonObjectService);/*free memory*/
}

/********************************************************
 * function: json_ctrl_unpack_read
 * description:
 * input:   1. data:          json data
 * output:  1. pObjattr:      param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_pack_read( POBJATTR_T pObjattr, char *json_package )
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    char *jsonString = NULL;

    if ( pObjattr == NULL || json_package == NULL )
    {
        return kGeneralErr;
    }

    jsonObjectService = json_object_new_object();
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_READ));
    jsonObjectData = json_object_new_object();
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    json_object_object_add(jsonObjectData, KEY_CLASSID, json_object_new_int(pObjattr->class));
    json_object_object_add(jsonObjectData, KEY_DEVID, json_object_new_string(pObjattr->devid));
    json_object_object_add(jsonObjectData, KEY_TYPE, json_object_new_int(pObjattr->type));
    json_object_object_add(jsonObjectData, KEY_KEY, json_object_new_string(pObjattr->keystr));


    jsonString = json_object_to_json_string(jsonObjectService);
    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return;
    }
    memcpy(json_package, jsonString, strlen(jsonString));

    json_object_put(jsonObjectService);/*free memory*/

}

/********************************************************
 * function: json_pack_ctrl_synstatus
 * description:
 * input:   syn_status_info:
 *          syn_num:
 * output:  json_package:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void json_pack_ctrl_synstatus( SYN_STATUS_INFO_T *syn_status_info, uint8_t syn_num, char *json_package)
{
    struct json_object *jsonObjectService = NULL;
    struct json_object *jsonObjectData = NULL;
    struct json_object *jsonObjectArray = NULL;
    struct json_object *jsonObjectTemp = NULL;
    char *jsonString = NULL;
    uint8_t count = 0;

    if ( syn_status_info == NULL || json_package == NULL )
    {
        return kGeneralErr;
    }

    jsonObjectService = json_object_new_object();
    json_object_object_add(jsonObjectService, PUBLIC_CMD, json_object_new_string(CTRL_SYNCSTATUS));
    jsonObjectData = json_object_new_object();
    json_object_object_add(jsonObjectService, PUBLIC_DATA, jsonObjectData);

    if (syn_num)
    {
        jsonObjectArray = json_object_new_array();
        if (jsonObjectArray)
        {
            json_object_object_add(jsonObjectData, KEY_DEVLIST, jsonObjectArray);
            for (count=0; count < syn_num; count++)
            {
                jsonObjectTemp = json_object_new_object();
                json_object_object_add(jsonObjectTemp, KEY_ADDR, json_object_new_string(syn_status_info->addr));
                json_object_object_add(jsonObjectTemp, KEY_KEY, json_object_new_string(syn_status_info->key));
                json_object_array_add(jsonObjectArray, jsonObjectTemp);
            }
        }
    }

    jsonString = json_object_to_json_string(jsonObjectService);
    if (jsonString == NULL)
    {
        json_object_put(jsonObjectService);/*free memory*/
        os_json_ctrl_log("object to string fail");
        return;
    }
    memcpy(json_package, jsonString, strlen(jsonString));

    json_object_put(jsonObjectService);/*free memory*/

}

/********************************************************
 * function: json_ctrl_unpack_read_attr_json
 * description:
 * input:   1. parse_json_object:
 * output:  1. pObjattr:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_read_attr_json(struct json_object *parse_json_object, POBJATTR_T pObjattr)
{
    char *strTemp = NULL;
    int strLen = 0;
    if ((parse_json_object != NULL)&&(pObjattr != NULL))
    {
        json_object_object_foreach(parse_json_object, key, val){
            if(!strcmp(key, KEY_DEVID)){
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pObjattr->devid))
                {
                    strLen = sizeof(pObjattr->devid)-1;
                }
                memcpy(pObjattr->devid, strTemp, strLen);
                os_json_ctrl_log("%s:%s", key, pObjattr->devid);
            }
            else if (!strcmp(key, KEY_KEY))
            {
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pObjattr->keystr))
                {
                    strLen = sizeof(pObjattr->keystr)-1;
                }
                memcpy(pObjattr->keystr, strTemp, strLen);
                os_json_ctrl_log("%s:%s", key, pObjattr->keystr);
            }
            else if (!strcmp(key, KEY_TYPE))
            {
                pObjattr->type = json_object_get_int(val);
                os_json_ctrl_log("%s:%d", key, pObjattr->type);
            }
            else if (!strcmp(key, KEY_CLASSID))
            {
                pObjattr->class = json_object_get_int(val);
                os_json_ctrl_log("%s:%d", key, pObjattr->class);
            }
        }
    }
}

/********************************************************
 * function: json_ctrl_unpack_read
 * description:
 * input:   1. data:          json data
 * output:  1. pObjattr:      param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_read(char *data, POBJATTR_T pObjattr)
{
    if (data == NULL || pObjattr == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(pObjattr, 0, sizeof(OBJATTR_T));
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);
    json_ctrl_unpack_read_attr_json(parse_json_object, pObjattr);

    json_object_put(parse_json_object);/*free memory*/
    parse_json_object=NULL;
    return kNoErr;

}

/********************************************************
 * function: json_ctrl_unpack_write
 * description:  write object attribute
 * input:   1. data:           json data from client
 * output:  1. pdevattr_write_struct:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_write_json(struct json_object *parse_json_object, PDEVATTR_WRITE_T pdevattr_write_struct)
{
    char *strTemp = NULL;

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
//            os_json_ctrl_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pdevattr_write_struct->devid))
                {
                    strLen = sizeof(pdevattr_write_struct->devid);
                }
                memcpy(pdevattr_write_struct->devid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_CLASSID))
            {
                pdevattr_write_struct->classID = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_ADDR))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pdevattr_write_struct->netaddr))
                {
                    strLen = sizeof(pdevattr_write_struct->netaddr)-1;
                }
                memcpy(pdevattr_write_struct->netaddr, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_ATTR))
            {
                uint8_t jsonType = json_object_get_type(val);
                if (jsonType == json_type_object)
                {
                    pdevattr_write_struct->num = 1;
                    if (pdevattr_write_struct->opobj == NULL)
                    {
                        pdevattr_write_struct->opobj = malloc(sizeof(DEV_FUNCTION_OBJ_EX_T));
                        memset(pdevattr_write_struct->opobj, 0, sizeof(DEV_FUNCTION_OBJ_EX_T));
                    }
                    pdevattr_write_struct->opobj->type = FUNCTION_OBJ_DEFAULT_TYPE;
                    json_unpack_devop_obj_ex(val, pdevattr_write_struct->opobj);
                }
                else if (jsonType == json_type_array)
                {
                    array_list *list = NULL;
                    uint8_t count = 0;
                    PDEV_FUNCTION_OBJ_EX_T pDevFunObj = NULL;
                    list = json_object_get_array(val);
                    pdevattr_write_struct->num = list->length;
                    if (pdevattr_write_struct->opobj == NULL)
                    {
                        pdevattr_write_struct->opobj = malloc(list->length * sizeof(DEV_FUNCTION_OBJ_EX_T));
                        memset(pdevattr_write_struct->opobj, 0, list->length * sizeof(DEV_FUNCTION_OBJ_EX_T));
                    }
                    for (count=0; count<list->length; count++)
                    {
                        pDevFunObj = pdevattr_write_struct->opobj + count;
                        pDevFunObj->type = FUNCTION_OBJ_DEFAULT_TYPE;
                        json_unpack_devop_obj_ex(list->array[count], pDevFunObj);
                    }
                }
            }
        }

    }
    return kNoErr;
}

/********************************************************
 * function: json_ctrl_unpack_write
 * description:  write object attribute
 * input:   1. data:           json data from client
 * output:  1. pdevattr_write_struct:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_write(char *data, PDEVATTR_WRITE_T pdevattr_write_struct)
{
    if (data == NULL || pdevattr_write_struct == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(pdevattr_write_struct, 0, sizeof(DEVATTR_WRITE_T));
    os_json_ctrl_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    json_ctrl_unpack_write_json(parse_json_object, pdevattr_write_struct);
    json_object_put(parse_json_object);/*free memory*/
    parse_json_object=NULL;
    return kNoErr;

}


/********************************************************
 * function: json_ctrl_unpack_readmult
 * description:  read objects attribute
 * input:   1. data:           json data from client
 * output:  1. pObjattr:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_readmult(char *data, POBJATTR_T pObjattr, uint8_t *devNum)
{
    if (data == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;
    array_list *array = NULL;

    os_json_ctrl_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_ctrl_log("%s", key);
            if (!strcmp(key, KEY_DEVLIST))
            {
                array = json_object_get_array(val);
            }
        }

        // parse array
        if (array != NULL)
        {
            uint8_t index=0;
            *devNum = array->length;
            for (index=0; index < *devNum; index++)
            {
                json_ctrl_unpack_read_attr_json(array->array[index], pObjattr+index);
            }
        }

        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_ctrl_unpack_writemult
 * description:  write object attribute
 * input:   1. data:           json data from client
 * output:  1. pdevattr_write_struct:    param data
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus json_ctrl_unpack_writemult(char *data, PDEVATTR_WRITEMULT_T pdevattr_writemult_t)
{
    if (data == NULL || pdevattr_writemult_t == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    array_list *array = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(pdevattr_writemult_t, 0, sizeof(DEVATTR_WRITE_T));
    os_json_ctrl_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_ctrl_log("%s", key);
            if (!strcmp(key, KEY_DEVLIST))
            {
                uint8_t devIndex = 0;
                PDEVATTR_WRITE_T pWriteAttr = NULL;
                array = json_object_get_array(val);
                pdevattr_writemult_t->devnum = array->length;
                if (NULL == pdevattr_writemult_t->write_attr)
                {
                    pdevattr_writemult_t->write_attr = (PDEVATTR_WRITE_T)malloc(array->length*sizeof(DEVATTR_WRITE_T));
                    memset(pdevattr_writemult_t->write_attr, 0, array->length*sizeof(DEVATTR_WRITE_T));
                }
                for (devIndex = 0; devIndex < array->length; devIndex++)
                {
                    pWriteAttr = pdevattr_writemult_t->write_attr + devIndex;
                    json_ctrl_unpack_write_json(array->array[devIndex], pWriteAttr);
                }
            }

        }


        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_ctrl_unpack_devset
 * description:  write object attribute
 * input:   1. data:           json data from client
 * output:  1. pdev_set_struct:    param data
 * return:
 * auther:   chenb
 * other:    注：在次函数中将会申请一个动态空间，由调用者来释放该空间
*********************************************************/
OSStatus json_ctrl_unpack_devset(char *data, PDEVSET_INFO_T pdev_set_struct)
{
    if (data == NULL || pdev_set_struct == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    char *strTemp = NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(pdev_set_struct, 0, sizeof(DEVSET_INFO_T));
    os_json_ctrl_log("data:%s, len = %d\n", string, len);
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);

    if (parse_json_object != NULL)
    {
        json_object_object_foreach(parse_json_object, key, val){
            os_json_ctrl_log("%s", key);
            if(!strcmp(key, KEY_DEVID)){
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pdev_set_struct->devid))
                {
                    strLen = sizeof(pdev_set_struct->devid)-1;
                }
                memcpy(pdev_set_struct->devid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_CMD))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pdev_set_struct->cmd))
                {
                    strLen = sizeof(pdev_set_struct->cmd);
                }
                memcpy(pdev_set_struct->cmd, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_OBJECTID_EX))
            {
                int strLen = 0;
                strTemp = json_object_get_string(val);
                strLen = strlen(strTemp);
                if (strLen > sizeof(pdev_set_struct->objid))
                {
                    strLen = sizeof(pdev_set_struct->objid);
                }
                memcpy(pdev_set_struct->objid, strTemp, strLen);
            }
            else if (!strcmp(key, KEY_OBJECTTYPE))
            {
                pdev_set_struct->objtype = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_LIMIT))
            {
                pdev_set_struct->limit = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_PAGE))
            {
                pdev_set_struct->page = json_object_get_int(val);
            }
            else if (!strcmp(key, KEY_ATTR))
            {
                uint8_t jsonType = json_object_get_type(val);
                if (jsonType == json_type_object)
                {
                    pdev_set_struct->attrNum = 1;
                    if (pdev_set_struct->opobj == NULL)
                    {
                        pdev_set_struct->opobj = malloc(sizeof(DEV_FUNCTION_OBJ_T));
                        memset(pdev_set_struct->opobj, 0, sizeof(DEV_FUNCTION_OBJ_T));
                    }
                    json_unpack_devop_obj(val, pdev_set_struct->opobj);
                }
                else if (jsonType == json_type_array)
                {
                    array_list *list = NULL;
                    uint8_t count = 0;
                    PDEV_FUNCTION_OBJ_T pDevFunObj = NULL;
                    list = json_object_get_array(val);
                    pdev_set_struct->attrNum = list->length;
                    pdev_set_struct->opobj = malloc((list->length) * sizeof(DEV_FUNCTION_OBJ_T));
                    memset(pdev_set_struct->opobj, 0, list->length * sizeof(DEV_FUNCTION_OBJ_T));
                    for (count=0; count<list->length; count++)
                    {
                        pDevFunObj = pdev_set_struct->opobj + count;
                        json_unpack_devop_obj(list->array[count], pDevFunObj);
                    }
                }
            }

        }

        json_object_put(parse_json_object);/*free memory*/
        parse_json_object=NULL;
    }
    return kNoErr;

}

/********************************************************
 * function: json_ctrl_unpack_synstatus
 * description:
 * input:   1. data:           json data from client
 * output:  1. syn_status_info:    param data
 * return:
 * auther:   chenb
 * other:    注：在次函数中将会申请一个动态空间，由调用者来释放该空间
*********************************************************/
OSStatus json_ctrl_unpack_synstatus(char *data, SYN_STATUS_T *syn_status_info)
{
    if (data == NULL || syn_status_info == NULL)
    {
        return kGeneralErr;
    }
    struct json_object *parse_json_object=NULL;
    int  len = strlen(data);
    char *string = data;
    int status = 0;

    memset(syn_status_info, 0, sizeof(SYN_STATUS_T));
    status = json_checker_string(string, len);
    if (!status)
    {
        os_json_ctrl_log("json data is illegal");
        return kGeneralErr;
    }

    // parse reciving data
    parse_json_object = json_tokener_parse(string);
    if (parse_json_object)
    {
        json_object_object_foreach(parse_json_object, key, val){
            if (!strcmp(key, KEY_DEVLIST))
            {
                uint8_t devIndex = 0;
                PSYN_STATUS_OBJ_T pSynStatus = NULL;
                array_list *array = NULL;
                char *strTemp = NULL;
                array = json_object_get_array(val);
                syn_status_info->num = array->length;
                if (NULL == syn_status_info->synstatusObj)
                {
                    syn_status_info->synstatusObj = (PSYN_STATUS_OBJ_T)malloc(array->length*sizeof(SYN_STATUS_OBJ_T));
                    memset(syn_status_info->synstatusObj, 0, array->length*sizeof(SYN_STATUS_OBJ_T));
                }
                for (devIndex = 0; devIndex < array->length; devIndex++)
                {
                    pSynStatus = syn_status_info->synstatusObj + devIndex;
                    json_object_object_foreach(array->array[devIndex], subkey, subval){
                        if (!strcmp(subkey, KEY_CLASSID))
                        {
                            pSynStatus->classid = json_object_get_int(subval);
                        }
                        else if (!strcmp(subkey, KEY_UUID))
                        {
                            int strLen = 0;
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > sizeof(pSynStatus->uuid))
                            {
                                strLen = sizeof(pSynStatus->uuid)-1;
                            }
                            memcpy(pSynStatus->uuid, strTemp, strLen);
                        }
                        else if (!strcmp(subkey, KEY_DEVID))
                        {
                            int strLen = 0;
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > sizeof(pSynStatus->devid))
                            {
                                strLen = sizeof(pSynStatus->devid)-1;
                            }
                            memcpy(pSynStatus->devid, strTemp, strLen);
                        }
                        else if (!strcmp(subkey, KEY_ADDR))
                        {
                            int strLen = 0;
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > sizeof(pSynStatus->addr))
                            {
                                strLen = sizeof(pSynStatus->addr)-1;
                            }
                            memcpy(pSynStatus->addr, strTemp, strLen);
                        }
                        else if (!strcmp(subkey, KEY_KEY))
                        {
                            int strLen = 0;
                            strTemp = json_object_get_string(subval);
                            strLen = strlen(strTemp);
                            if (strLen > sizeof(pSynStatus->key))
                            {
                                strLen = sizeof(pSynStatus->key)-1;
                            }
                            memcpy(pSynStatus->key, strTemp, strLen);
                        }
                    }
                }
            }
        }
    }

    json_object_put(parse_json_object);/*free memory*/
    parse_json_object=NULL;
    return kNoErr;

}
