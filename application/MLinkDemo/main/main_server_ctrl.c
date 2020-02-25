/*
 * main_server_ctrl.c
 *
 *  Created on: 2017.12.16
 *      Author: Administrator
 */


#include "coap.h"
#include "mico.h"
#include "MLinkAppDef.h"
#include "../coap/ml_coap.h"
#include "../coap/coap_service.h"
#include "../flash/flash_storage_distribute.h"
#include "../zigbee/zigbee.h"
#include "MLinkObject.h"
#include "MLinkCommand.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../json/json_ctrl_deal.h"
#include "main_logic.h"
#include "../uart/DTTable.h"
#include "main_alarm.h"

#ifdef BLE_DEVICE
#include "../ble/bluetooth_logic.h"
#endif
#define os_server_ctrl_log(M, ...) custom_log("MAIN_SERVER_CTRL", M, ##__VA_ARGS__)

static mico_mutex_t g_server_ctrl_mutex;

/********************************************************
 * function: mlcoap_devset_scene
 * description:  情景操作处理
 * input:    1. op_cmd
 *           2. psip_param
 *           3. pop_attrs
 * output:   1. page_info
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_devset_scene(char *op_cmd, PDEVSET_SIP_PARAM_T psip_param, PDEV_FUNCTION_OBJ_T pop_attrs, char *page_info)
{
    uint8_t count = 0;
    int ret = 0;
    if ((op_cmd == NULL) || (psip_param == NULL) || (pop_attrs == NULL) || (page_info == NULL) )
    {
        os_server_ctrl_log("op_cmd: %x, pop_attrs: %x", op_cmd, pop_attrs);
        return kGeneralErr;
    }

#ifdef ZIGBEE_DEVICE
    if (0 == strcmp(op_cmd, CTRL_SET))
    {
        // 添加或修改情景
        ret = zigbee_add_scene((uint8_t *)&psip_param->addr, psip_param->modelid, psip_param->objid, pop_attrs);
    }
    else if (0 == strcmp(op_cmd, CTRL_DEL))
    {
        if ( 0 == psip_param->objid[0] )     // objid 为空时 执行清空组操作
        {
            ret = zigbee_clear_scene(&psip_param->addr, psip_param->modelid, pop_attrs->key);
        }
        else    // 删除指定组
        {
            ret = zigbee_del_scene(&psip_param->addr, psip_param->modelid, psip_param->objid, pop_attrs->key);
        }
    }
    else if (0 == strcmp(op_cmd, CTRL_GET))
    {
        // 获取情景
        ret = zigbee_get_scene(psip_param, pop_attrs, page_info);
        os_server_ctrl_log("keyVal: %s, key: %s", pop_attrs->value, pop_attrs->key);
    }
    else if (0 == strcmp(op_cmd, CTRL_CLEAR))
    {
        // 清空情景
        ret = zigbee_clear_scene(&psip_param->addr, psip_param->modelid, pop_attrs->key);
    }
#endif

    return ret;
}

/********************************************************
 * function: mlcoap_devset_group
 * description:  组操作处理
 * input:    1. op_cmd
 *           2. psip_param
 *           3. pop_attrs
 * output:   1. page_info
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_devset_group(char *op_cmd, PDEVSET_SIP_PARAM_T psip_param, PDEV_FUNCTION_OBJ_T pop_attrs, char *page_info)
{
    uint8_t count = 0;
    int ret = 0;
    if ((op_cmd == NULL) || (psip_param == NULL) || (pop_attrs == NULL) || (page_info == NULL) )
    {
        os_server_ctrl_log("op_cmd: %x, pop_attrs: %x", op_cmd, pop_attrs);
        return kGeneralErr;
    }

#ifdef ZIGBEE_DEVICE
    if (0 == strcmp(op_cmd, CTRL_SET))
    {
        // 添加或修改组
        ret = zigbee_add_group(&psip_param->addr, psip_param->modelid, psip_param->objid, pop_attrs->key);
    }
    else if (0 == strcmp(op_cmd, CTRL_DEL))
    {
        if ( 0 == psip_param->objid[0] )     // group 为空时 执行清空组操作
        {
            ret = zigbee_clear_group(&psip_param->addr, psip_param->modelid, pop_attrs->key);
        }
        else    // 删除指定组
        {
            ret = zigbee_del_group(&psip_param->addr, psip_param->modelid, psip_param->objid, pop_attrs->key);
        }
    }
    else if (0 == strcmp(op_cmd, CTRL_GET))
    {
        // 获取组
        ret = zigbee_get_group(psip_param, pop_attrs, page_info);
        os_server_ctrl_log("keyVal: %s, key: %s", pop_attrs->value, pop_attrs->key);
    }
    else if (0 == strcmp(op_cmd, CTRL_CLEAR))
    {
        // 清空组信息
        ret = zigbee_clear_group(&psip_param->addr, psip_param->modelid, pop_attrs->key);
    }
#endif

    return ret;
}

/********************************************************
 * function: server_ctrl_scene_default_link
 * description:  情景默认联动处理
 * input:   sceneid
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus server_ctrl_scene_default_link( char *sceneid )
{
    PALARM_ENDPOINT_T palarmEndpoint = NULL;
//    REPORT_STATE_CHANGE_T stateInfo = {0};
    int ret = 0;
    uint8_t alarmType = 0;
    int id = atoi(sceneid);
    storage_read_local_alarm_state(&palarmEndpoint);
    if (id == 1)
    {
        alarmType = ALARM_MODE_OUTGOING;
    }
    else if (id == 2)
    {
        alarmType = ALARM_MODE_HOME;
    }
    else if (id == 3)
    {
        alarmType = ALARM_MODE_NIGHT;
    }
    else
    {
        return kGeneralErr;
    }

    if (palarmEndpoint->alarmType != alarmType)
    {
        palarmEndpoint->alarmType = alarmType;
        alarm_gateway_statechange_deal(palarmEndpoint->devid, palarmEndpoint->alarmType);
//        ret = storage_write_local_alarm_state( palarmEndpoint );
    }

    return ret;
}


/********************************************************
 * function: server_ctrl_start_setmesh
 * description:   开始组网
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void server_ctrl_start_setmesh()
{
#ifndef RF433_DEVICE
    uint16_t type = 0;

    zigbee_get_node_type(0x8000, &type);
    if (type == 2 || type == 3)  // node type is route or terminal
    {
        zigbee_setmesh_start_coordinate(0x8000, SETMESH_TIMEOUT);
    }
    else
    {
        zigbee_device_allow_setmesh(0xffff, SETMESH_TIMEOUT);
    }
#else
    rf433_setmesh(0xffff, SETMESH_TIMEOUT);
#endif
}

/********************************************************
 * function: server_ctrl_stop_setmesh
 * description:   停止组网
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void server_ctrl_stop_setmesh()
{
#ifndef RF433_DEVICE
    zigbee_out_of_setmesh(0xffff);
#else
    rf433_out_of_setmesh(0xffff);
#endif
}


/********************************************************
 * function: mlcoap_server_ctrl_scene_link
 * description:  情景联动
 * input:    1. sceneid
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlcoap_server_ctrl_scene_link( char *sceneid )
{
    // 查询联动任务，并作处理
    main_linkage_single_deal(SCENE_OBJ_ID, sceneid, NULL, VALTYPE_SHORT, sceneid);

    // 默认联动
    server_ctrl_scene_default_link(sceneid);

}

/********************************************************
 * function: mlcoap_server_ctrl_write_local
 * description:
 * input:    1. devid
 *           2. endpoint_num
 *           3. local_endpoint
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_write_local(char *devid, uint8_t endpoint_num, PDEV_FUNCTION_OBJ_EX_T local_endpoint)
{
    uint8_t count = 0;
    uint16_t keyCode = 0;
    PDEV_FUNCTION_OBJ_EX_T endpointAttrTemp = NULL;
    int ret = 0;

    if ( (local_endpoint == NULL) || (devid == NULL))
    {
        return MLINK_RESP_ERROR;
    }
#if (defined ZIGBEE_DEVICE)
    if (!strcmp(local_endpoint->key, KEY_NET_PARAM))
    {
        ret = zigbee_set_net_param(local_endpoint->value, local_endpoint->valtype);
        if (ret == -1)
        {
            ret = kGeneralErr;
        }
        else
        {
            ret = kNoErr;
        }
    }
    else
#endif
    {
        PALARM_ENDPOINT_T palarmEndpoint = NULL;
        storage_read_local_alarm_state(&palarmEndpoint);

        for (count=0; count < endpoint_num; count++)
        {
            endpointAttrTemp = local_endpoint + count;
            keyCode = (uint16_t)atoi(endpointAttrTemp->key);
            if (keyCode == DT_ALARM_STATE)
            {
                palarmEndpoint->alarmType = atoi(local_endpoint->value);
                alarm_gateway_statechange_deal(palarmEndpoint->devid, palarmEndpoint->alarmType);
            }
            else if (keyCode == DT_ALARM_DELAY)
            {
                palarmEndpoint->alarmDelay = atoi(local_endpoint->value);
            }
        }
        mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
        ret = storage_write_local_alarm_state( palarmEndpoint );
    }

    if (ret == kNoErr)
    {
        return MLINK_RESP_OK;
    }
    else
    {
        return MLINK_RESP_ERROR;
    }
}

/********************************************************
 * function: mlcoap_server_ctrl_write_subdev
 * description:
 * input:   1. class_id
 *          2. devid
 *          3. endpoint_num
 *          4. endpoints_attr
 *          5. wait_time_ms
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_ctrl_write_subdev(int class_id, char *devid, uint8_t endpoint_num, PDEV_FUNCTION_OBJ_EX_T endpoints_attr, uint32_t wait_time_ms)
{
    DEVICEOBJ_T devObj = {0};
    PDEV_FUNCTION_OBJ_EX_T endpointAttrTemp = NULL;
    int daddr = 0;
    uint8_t objIndex = 0;
    int ret = 0;
    if (devid == NULL)
    {
        return kGeneralErr;
    }
    if (class_id == SCENE_OBJ_ID)
    {
        SCENEOBJ_T sceneObj = {0};
        ret = storage_read_scene_obj(devid, &sceneObj);
    }
    else if (class_id == DEVICE_OBJ_ID)
    {
        ret = storage_search_devobj_devid(devid, &devObj);
    }

//    os_server_ctrl_log("classid: %d, devid: %s, ret: %d", class_id, devid, ret);

    if (ret <= 0)
    {
        ret = MLINK_RESP_FIND_NOTHING;
    }
    else
    {
        if (class_id == DEVICE_OBJ_ID)
        {
            ALARM_ENDPOINT_T alarmEndpoint = {0};
            uint32_t alarmIndex = 0;
            mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, &daddr);
            for (objIndex=0; objIndex < endpoint_num; objIndex++)
            {
                endpointAttrTemp = endpoints_attr + objIndex;
                if ((0==strcmp(endpointAttrTemp->key, KEY_ALARM_STATE)) || (0 == strcmp(endpointAttrTemp->key, KEY_ALARM_DELAY)))
                {
                    alarmIndex = storage_search_dev_alarm(devid, &alarmEndpoint);
                    os_server_ctrl_log("key: %s, value: %s, devid: %s", endpointAttrTemp->key, endpointAttrTemp->value, alarmEndpoint.devid);
                    if (0==strcmp(endpointAttrTemp->key, KEY_ALARM_STATE))
                    {
                        alarmEndpoint.alarmType = (uint8_t)atoi(endpointAttrTemp->value);
                        alarm_statechange_deal( devid, endpointAttrTemp->value );
                    }
                    else if (0 == strcmp(endpointAttrTemp->key, KEY_ALARM_DELAY))
                    {
                        alarmEndpoint.alarmDelay = (uint8_t)atoi(endpointAttrTemp->value);
                    }
                }
                else
                {
                    if (devObj.comm == TELECOM_RF433)
                    {
#ifdef RF433_DEVICE
                        ret = rf433_logic_write_attr(class_id, &daddr, endpointAttrTemp);
#endif
                    }
                    else if (devObj.comm == TELECOM_ZIGBEE)
                    {
#ifdef ZIGBEE_DEVICE
                        // writing object attribute
                        ret = zigbee_write_attr(class_id, devObj.modelid, &daddr, endpointAttrTemp, wait_time_ms);
#endif
                    }
                }
            }
            if (alarmEndpoint.devid[0] != 0)
            {
                ret = storage_update_dev_alarm_state(1, &alarmEndpoint);
            }
        }
        else if (class_id == SCENE_OBJ_ID)
        {
#ifdef ZIGBEE_DEVICE
            daddr = atoi(devid);
            ret = zigbee_write_attr(SCENE_OBJ_ID, "000000", &daddr, NULL, 0);
#endif
            mlcoap_server_ctrl_scene_link(devid);
        }
        if (ret == kNoErr)
        {
            ret = MLINK_RESP_OK;
        }
        else
        {
            os_server_ctrl_log("ret is %d", ret);

            ret = MLINK_RESP_ERROR;
        }
    }
    return ret;
}

/********************************************************
 * function: mlcoap_server_ctrl_read_local
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_read_local(char *device_id, char *key_code, PDEVATTR_READ_T pdevice_attr_t)
{
    if ((device_id == NULL) || (pdevice_attr_t == NULL))
    {
        return MLINK_RESP_ERROR;
    }
    int ret = MLINK_RESP_OK;
    strcpy(pdevice_attr_t->devid, device_id);

#if (defined ZIGBEE_DEVICE)
    if (!strcmp(key_code, KEY_NET_PARAM))
    {
        ret = zigbee_get_net_param(pdevice_attr_t->devattrobj->value, &pdevice_attr_t->devattrobj->valtype);
        pdevice_attr_t->devattrobj->type = 3;
        strcpy(pdevice_attr_t->devattrobj->key, KEY_NET_PARAM);
        if (ret != -1)
        {
            pdevice_attr_t->attrnum = 1;
        }
    }
    else
#endif
    {
        PALARM_ENDPOINT_T palarmEndpoint = NULL;
        // 读取安防信息值
        storage_read_local_alarm_state(&palarmEndpoint);
        pdevice_attr_t->attrnum = 1;
        if (pdevice_attr_t->devattrobj != NULL)
        {
            PDEV_FUNCTION_OBJ_EX_T pattrObj = NULL;
            pattrObj = pdevice_attr_t->devattrobj;
            strcpy(pattrObj->key, KEY_ALARM_STATE);
            pattrObj->type = 3;
            pattrObj->valtype = 0;
            sprintf(pattrObj->value, "%d", palarmEndpoint->alarmType);
        }
    }

    return ret;

}

/********************************************************
 * function: mlcoap_server_ctrl_read_subdev
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_read_subdev(char *device_id, char *key_code, PDEVATTR_READ_T pdevice_attr_t)
{
    DEVICEOBJ_T devObj = {0};
    PDEV_ENDPOINT_STATUS_T pdevEndpointsStatus = NULL;
    PDEV_FUNCTION_OBJ_EX_T pattrObj = NULL;
    PENDPOINT_STATUS_T endpointStatusTemp = NULL;
    uint32_t daddr = 0;
    int ret = 0;
    uint8_t count = 0;

    if ((device_id == NULL) || (pdevice_attr_t == NULL) || (pdevice_attr_t->devattrobj == NULL))
    {
        return MLINK_RESP_ERROR;
    }

    ret = storage_read_dev_obj(device_id, &devObj);
    mlink_parse_subdev_net_addr( devObj.comm, devObj.addr, &daddr );
    pattrObj = pdevice_attr_t->devattrobj;
    if (*key_code == 0)
    {
        uint8_t alarmEndpointNum = 0;
        pdevEndpointsStatus = storage_get_endpoints_status(device_id);
        pdevice_attr_t->attrnum = pdevEndpointsStatus->endpointNum;
        strcpy(pdevice_attr_t->devid, pdevEndpointsStatus->fid);
        for (count=0; count<pdevice_attr_t->attrnum; count++)
        {
            endpointStatusTemp = pdevEndpointsStatus->endpointStatus+count;
            memcpy(pattrObj+count, &endpointStatusTemp->keyAttr.keyState, sizeof(DEV_FUNCTION_OBJ_T));
        }
        ret = storage_get_alarm_endpoint_attr(device_id, KEY_ALARM_DELAY, pattrObj+count);
        if (ret == kNoErr)
        {
            count++;
            pdevice_attr_t->attrnum++;
        }
        ret = storage_get_alarm_endpoint_attr(device_id, KEY_ALARM_STATE, pattrObj+count);
        if (ret == kNoErr)
        {
            count++;
            pdevice_attr_t->attrnum++;
        }
        // 取消读属性时，查询设备端点状态处理 2018.3.8
//        ret = zigbee_read_attrs(devObj.modelid, &daddr);
    }
    else
    {
        pdevice_attr_t->attrnum = 1;
        strcpy(pdevice_attr_t->devid, device_id);
        if ((0 == strcmp(key_code, KEY_ALARM_DELAY)) || (0 == strcmp(key_code, KEY_ALARM_STATE)))
        {
            ret = storage_get_alarm_endpoint_attr(device_id, key_code, (PDEV_FUNCTION_OBJ_T)pattrObj);
        }
        else
        {
            pdevEndpointsStatus = storage_get_endpoints_status(device_id);
            for (count=0; count < pdevEndpointsStatus->endpointNum; count++)
            {
                endpointStatusTemp = pdevEndpointsStatus->endpointStatus+count;
                os_server_ctrl_log("keycode: %s, key: %s", key_code, endpointStatusTemp->keyAttr.keyState.key);
                if (!strcmp(endpointStatusTemp->keyAttr.keyState.key, key_code))
                {
                    memcpy(pattrObj, &endpointStatusTemp->keyAttr.keyState, sizeof(DEV_FUNCTION_OBJ_T));
                    os_server_ctrl_log("key: %s", pattrObj->key);
                    break;
                }
            }

            if (devObj.comm == TELECOM_ZIGBEE)
            {
#ifdef ZIGBEE_DEVICE
                ret = zigbee_read_attr(devObj.modelid, &daddr, key_code, pattrObj);
                if (strlen(pattrObj->value)<VALUE_SIZE_EX)          // 设备保存状态值最多只能保存32字节的字符串数据.
                {
                    KEY_ATTR_T keyAttr = {0};
                    memcpy(&keyAttr.keyState, pattrObj, sizeof(DEV_FUNCTION_OBJ_T));
                    storage_update_endpoint_status(device_id, &keyAttr);
                }
#endif
            }
        }
    }
    if (ret == kNoErr)
    {
        ret = MLINK_RESP_OK;
    }
    else
    {
        ret = MLINK_RESP_ERROR;
    }
    return ret;
}

/********************************************************
 * function: mlcoap_server_ctrl_read
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_read(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    int errCode = 0;
    OBJATTR_T objAttr;
    DEVATTR_READ_T devAttrRead;
    char keyCode[KEY_SIZE] = {0};
    ENDPOINT_ELE_T endpointId_ele = {0};

    memset(&objAttr, 0, sizeof(OBJATTR_T));
    memset(&devAttrRead, 0, sizeof(DEVATTR_READ_T));
    objAttr.class = DEVICE_OBJ_ID;          // default class id
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(1500);
        memset(packPacket->data, 0, 1500);
    }

    json_ctrl_unpack_read(data, &objAttr);

    if (objAttr.devid[0] == 0)
    {
        json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_READ, MLINK_RESP_LACK_PARAM, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        return kNoErr;
    }
    else
    {
        uint16_t classId = (uint16_t)objAttr.class;
        char deviceId[OBJID_SIZE] = {0};
        if (devAttrRead.devattrobj == NULL)
        {
            devAttrRead.devattrobj = (PDEV_FUNCTION_OBJ_EX_T)malloc(ENDPOINTLIST_NUM * sizeof(DEV_FUNCTION_OBJ_EX_T));
        }
        if (classId == ENDPOINT_OBJ_ID)
        {
            mlink_parse_endpointid(objAttr.devid, &endpointId_ele);
            strcpy(deviceId, endpointId_ele.devid);
            classId = endpointId_ele.classId;
            strcpy(keyCode, endpointId_ele.key);
        }
        else
        {
            strcpy(deviceId, objAttr.devid);

            classId = (uint16_t)objAttr.class;
            if (objAttr.keystr[0] != 0)
            {
                strcpy(keyCode, objAttr.keystr);
            }
        }
        os_server_ctrl_log("read keyCode is %s", keyCode);
        if (classId == NETDEV_OBJ_ID)
        {
            errCode = mlcoap_server_ctrl_read_local(deviceId, keyCode, &devAttrRead);
        }
        else
        {
            errCode = mlcoap_server_ctrl_read_subdev(deviceId, keyCode, &devAttrRead);
        }

        if (errCode == MLINK_RESP_ERROR)
        {
            if (devAttrRead.devattrobj != NULL)
            {
                free(devAttrRead.devattrobj);
                devAttrRead.devattrobj = NULL;
            }
            json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_READ, MLINK_RESP_ERROR, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
            return kNoErr;
        }
        errCode = MLINK_RESP_OK;
    }
    if (errCode == MLINK_RESP_OK)
    {
        memcpy(devAttrRead.devid, objAttr.devid, strlen(objAttr.devid));
        json_pack_ctrl_read_response(MLINK_RESP_OK, NULL, &devAttrRead, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }
    if (devAttrRead.devattrobj != NULL)
    {
        free(devAttrRead.devattrobj);
        devAttrRead.devattrobj = NULL;
    }
    return kNoErr;
}

#ifdef BLE_DEVICE
/********************************************************
 * function: mlcoap_server_crtl_bluetooth
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_crtl_bluetooth(COAP_CTRL_CMD_ID_E cmd, char *devid,PDEV_FUNCTION_OBJ_T dev_attr)
{
    OSStatus ret = kNoErr;
    int i;

    uint8_t hex_data[16] = {0};
    if (devid == NULL)
    {
       return kGeneralErr;
    }
    switch(cmd)
    {

        case COAP_CTRL_NOTI_CMD_USRPIN:
        {
            DEVICEOBJ_T DeviceObj;
            StrToHexEx(dev_attr->value, strlen(dev_attr->value), hex_data);
            os_server_ctrl_log("hex_data[%s]\n",hex_data);
            ret = storage_read_dev_obj(devid, &DeviceObj);
            if (ret > 0)
            {
                memcpy(DeviceObj.uuid,dev_attr->value,16);
                memcpy(DeviceObj.deviceId, devid, sizeof(DeviceObj.deviceId));
                ret = storage_set_present_lockpin();
                if(ret == kInProgressErr)
                {
                    ble_set_userpin(hex_data);
                }
                else
                {
                    return ret;
                }
                if(ret == 1)
                {
                    ret = kNoErr;
                }
            }
            else
            {
                ret = kGeneralErr;
            }
        }
            return ret;
        case COAP_CTRL_NOTI_CMD_UNLOCK:
            storage_set_present_lockpin();
            ble_unlock_start();
            return ret;
        case COAP_CTRL_NOTI_CMD_LOCKGALLERY:
            ret = storage_set_present_lockpin();
            if(ret == kInProgressErr)
            {
                ble_set_channel();
            }
            else
            {
                return ret;
            }
            if(ret == 1)
            {
                ret = kNoErr;
            }
            return ret;
         default:
            ret = kGeneralErr;
            return ret;
    }
}
#endif

/********************************************************
 * function: mlcoap_server_ctrl_write
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_write(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    DEVATTR_WRITE_T devattr = {0};
    int errCode = 0;
    memset(&devattr, 0, sizeof(DEVATTR_WRITE_T));

    json_ctrl_unpack_write(data, &devattr);

    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);

    if ((devattr.devid[0] == 0) && (devattr.uuid[0] == 0) && (devattr.netaddr[0] == 0))
    {
        os_server_ctrl_log("param is illegal !!!");
        errCode = MLINK_RESP_ILLEGAL;
    }
    else
    {
        if ((strlen(devattr.devid) < OBJID_SIZE) && (devattr.devid[0] != 0))
        {
            char devid[DEVICEID_SIZE] ={0};
            uint16_t classId = 0;
            ENDPOINT_ELE_T endpointParse = {0};
            if (devattr.classID == ENDPOINT_OBJ_ID)
            {
                mlink_parse_endpointid(devattr.devid, &endpointParse);
                classId = endpointParse.classId;
                strcpy(devid, endpointParse.devid);

                if (0 == devattr.opobj->key[0])
                {
                    strcpy(devattr.opobj->key, endpointParse.key);
                }
            }
            else
            {
                classId = devattr.classID;
                strcpy(devid, devattr.devid);
            }
            if (classId == NETDEV_OBJ_ID)
            {
                errCode = mlcoap_server_ctrl_write_local(devid, devattr.num, devattr.opobj);
            }
#ifdef BLE_DEVICE
            if ((classId == DEVICE_OBJ_ID) || (classId == SCENE_OBJ_ID))
            {
                uint16_t key = 0;
                key = atoi(devattr.opobj->key);//type key
                errCode = mlcoap_server_crtl_bluetooth(key,devid,devattr.opobj);
            }
#else
            else if ((classId == DEVICE_OBJ_ID) || (classId == SCENE_OBJ_ID))
            {
                errCode = mlcoap_server_ctrl_write_subdev(classId, devid, devattr.num, devattr.opobj, 1500);
            }
#endif

        }
#ifdef ZIGBEE_DEVICE
        else if (devattr.netaddr[0] != 0)
        {
            unsigned char addr[4] = {0};
            NETDEVOBJ_T netDevObj = {0};
            uint8_t panid = 0;

            storage_read_local_devobj(&netDevObj);
            StrToHexEx(netDevObj.addr, strlen(netDevObj.addr), &panid);
            mlink_parse_subdev_net_addr(TELECOM_ZIGBEE, devattr.netaddr, addr);

            if (panid == addr[2])
            {
                char modelid[MODELID_SIZE] = "000000";
                errCode = zigbee_write_attr(DEVICE_OBJ_ID, modelid, addr, devattr.opobj, 0);
            }
        }
#endif
        else
        {
            errCode = MLINK_RESP_ILLEGAL;
        }
    }

    json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_WRITE, errCode, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    if (devattr.opobj != NULL)
    {
        free(devattr.opobj);
    }
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_ctrl_readmult
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_readmult(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    POBJATTR_T pdevAttr = malloc((DEVICE_NUM/2)*sizeof(OBJATTR_T));
    POBJATTR_T pdevAttrTemp = NULL;
    DEVATTR_READMULT_T devReadmultEcho = {0};
    DEVATTR_READ_T devAttrReadEcho[DEVICE_NUM/2];
    PDEV_FUNCTION_OBJ_T funAttr[DEVICE_NUM/2][ENDPOINT_NUM] = {NULL};
    DEVICEOBJ_T devObj = {0};
    ENDPOINT_ELE_T endpointId_ele = {0};
    char endpointid[ENDPOINTID_SIZE] = {0};
    OSStatus ret = 0;
    uint8_t devNum = 0;
    uint8_t devIndex = 0;
    char keyCode[KEY_SIZE] = {0};

    devReadmultEcho.devattrreadobj = devAttrReadEcho;
    memset(pdevAttr, 0, (DEVICE_NUM/2)*sizeof(OBJATTR_T));
    json_ctrl_unpack_readmult(data, pdevAttr, &devNum);
    devReadmultEcho.devnum = devNum;
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(1500);
        memset(packPacket->data, 0, 1500);
    }

    for (devIndex = 0; devIndex < devNum; devIndex++)
    {
        pdevAttrTemp = pdevAttr+devIndex;
        if (pdevAttrTemp->class == ENDPOINT_OBJ_ID)
        {
            ENDPOINT_ELE_T endpointId = {0};
            mlink_parse_endpointid(pdevAttrTemp->devid, &endpointId);
            strcpy(keyCode, endpointId.key);
            os_server_ctrl_log("classid: %d, deviceid: %s, key: %d", endpointId.classId, endpointId.devid, endpointId.key);
            if (endpointId.classId == DEVICE_OBJ_ID)        // device attribute
            {
                ret = storage_read_dev_obj(endpointId.devid, &devObj);
                if (ret != kGeneralErr)
                {
                    PKEY_ATTR_T pkeyAttr = storage_get_endpoint_status(endpointId.devid, pdevAttrTemp->devid);
                    funAttr[devIndex][0] = &pkeyAttr->keyState;
                    devAttrReadEcho[devIndex].attrnum = 1;
                }
            }
        }
        else if (pdevAttrTemp->class == DEVICE_OBJ_ID || pdevAttrTemp->class == 0)
        {
            strcpy(keyCode, pdevAttrTemp->keystr);
            ret = storage_read_dev_obj(pdevAttrTemp->devid, &devObj);
            if (keyCode[0] == 0)
            {
                uint8_t count = 0;
                PDEV_ENDPOINT_STATUS_T pDevEndpointsAtr = NULL;
                pDevEndpointsAtr = storage_get_endpoints_status(pdevAttrTemp->devid);
                if (pDevEndpointsAtr == NULL)
                {
                    continue;
                }
                else
                {
                    PENDPOINT_STATUS_T endpointStatusTemp = NULL;

                    for (count = 0; count < pDevEndpointsAtr->endpointNum; count++)
                    {
                        endpointStatusTemp = pDevEndpointsAtr->endpointStatus + count;
                        funAttr[devIndex][count] = &endpointStatusTemp->keyAttr;
                    }
                    devAttrReadEcho[devIndex].attrnum = pDevEndpointsAtr->endpointNum;
                }
            }
            else
            {
                PKEY_ATTR_T pkeyAttr = storage_get_endpoint_status(pdevAttrTemp->devid, endpointid);
                memset(endpointid, 0, sizeof(endpointid));
                memcpy(endpointId_ele.devid, pdevAttrTemp->devid, strlen(pdevAttrTemp->devid));
                mlink_generate_endpoint_id(DEVICE_OBJ_ID, pdevAttrTemp->devid, keyCode, endpointid);
                os_server_ctrl_log("deviceId: %s, keycode: %s, endpointid:%s", pdevAttrTemp->devid, keyCode, endpointid);
                funAttr[devIndex][0] = &pkeyAttr->keyState;
                os_server_ctrl_log("endpoint: %s, key: %s, value: %s",endpointid, funAttr[devIndex][0]->key, funAttr[devIndex][0]->value);
                devAttrReadEcho[devIndex].attrnum = 1;
            }
        }
        memcpy(devAttrReadEcho[devIndex].devid, pdevAttrTemp->devid, sizeof(pdevAttrTemp->devid));
        devAttrReadEcho[devIndex].devattrobj = funAttr;

    }

//    if (ret != )
//    {
//        json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_READMULT, MLINK_RESP_ERROR, packPacket->data);
//        packPacket->datalen = strlen(packPacket->data);
//        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
//        os_server_ctrl_log("response error!!!");
//        return kNoErr;
//    }

    json_pack_ctrl_readmult_response(MLINK_RESP_OK, NULL, &devReadmultEcho, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_ctrl_writemult
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_writemult(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    DEVATTR_WRITEMULT_T devAttrWritemult = {0};
    PDEVATTR_WRITE_T pDevAttrWrite = NULL;
    DEVICEOBJ_T devObj = {0};
//    PDEV_FUNCTION_OBJ_T pDevFunObj = NULL;
    int daddr = 0;
    int ret = 0;
    uint8_t objIndex = 0;
    uint8_t attrIndex = 0;

    json_ctrl_unpack_writemult(data, &devAttrWritemult);

    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);

    if (devAttrWritemult.write_attr == NULL)
    {
        json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_WRITEMULT, MLINK_RESP_LACK_PARAM, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        return kNoErr;
    }

    for (objIndex = 0; objIndex < devAttrWritemult.devnum; objIndex++)
    {
        pDevAttrWrite = devAttrWritemult.write_attr + objIndex;
        if ((pDevAttrWrite->devid[0] == 0)||(pDevAttrWrite->opobj == NULL))
        {
            continue;
        }
        else
        {
            if (pDevAttrWrite->classID == ENDPOINT_OBJ_ID)
            {
                ENDPOINT_ELE_T endpointParse = {0};
                mlink_parse_endpointid(pDevAttrWrite->devid, &endpointParse);
                ret = storage_read_dev_obj(endpointParse.devid, &devObj);
            }
            else
            {
                ret = storage_read_dev_obj(pDevAttrWrite->devid, &devObj);
            }

            if (ret == kGeneralErr)
            {
                json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_WRITEMULT, MLINK_RESP_FIND_NOTHING, packPacket->data);
                packPacket->datalen = strlen(packPacket->data);
                packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
                if (pDevAttrWrite->opobj != NULL)
                {
                    free(pDevAttrWrite->opobj);
                }
                continue;
            }
            ret = MLINK_RESP_OK;

        }

        mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, &daddr);
        for (attrIndex=0; attrIndex < pDevAttrWrite->num; attrIndex++)
        {
            if (devObj.comm == TELECOM_RF433)
            {
                ret = rf433_logic_write_attr(pDevAttrWrite->classID, &daddr, pDevAttrWrite->opobj+attrIndex);
            }
            else if (devObj.comm == TELECOM_ZIGBEE)
            {
                // writing object attribute
                ret = zigbee_write_attr(pDevAttrWrite->classID, devObj.modelid, &daddr, pDevAttrWrite->opobj+attrIndex, 1000);
            }
            mico_thread_msleep(50);
        }
        if (pDevAttrWrite->opobj != NULL)
        {
            free(pDevAttrWrite->opobj);
        }
    }
    if (devAttrWritemult.write_attr != NULL)
    {
        free( devAttrWritemult.write_attr );
    }

    json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_WRITEMULT, MLINK_RESP_OK, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);

    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_ctrl_writegroup
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_writegroup(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
#ifdef ZIGBEE_DEVICE
    // writing group objects attribute
    zigbee_writegroup_attr();
#endif

    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
    }
    memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);

    json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_WRITEGROUP, MLINK_RESP_OK, packPacket->data);
    packPacket->datalen = strlen(packPacket->data);
    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_ctrl_devset
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_ctrl_devset(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    DEVSET_INFO_T devsetInfo;
    DEVICEOBJ_T devObj = {0};
    uint8_t attrCount = 0;
    int ret = 0;
    DEVSET_SIP_PARAM_T sipParam = {0};
    memset(&devsetInfo, 0, sizeof(DEVSET_INFO_T));
    json_ctrl_unpack_devset(data, &devsetInfo);
    if (devsetInfo.opobj != NULL)
    {
        ret = storage_read_dev_obj(devsetInfo.devid, &devObj);
        os_server_ctrl_log("ret: %d, objid: %s, objtype: %d, cmd: %s",ret, devsetInfo.objid, devsetInfo.objtype, devsetInfo.cmd);
        if (ret != kGeneralErr)
        {
            PDEV_FUNCTION_OBJ_T attrTemp = NULL;
            mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, (uint8_t *)&sipParam.addr);
            sipParam.limit = devsetInfo.limit;
            sipParam.page = devsetInfo.page;
            memcpy(sipParam.objid, devsetInfo.objid, sizeof(sipParam.objid));
            memcpy(sipParam.modelid, devObj.modelid, sizeof(sipParam.modelid));


            if (devsetInfo.objtype == SCENE_OBJ_ID)     // 情景设置
            {
                for (attrCount = 0; attrCount<devsetInfo.attrNum; attrCount++)
                {
                    attrTemp = devsetInfo.opobj+attrCount;
                    mlcoap_devset_scene( devsetInfo.cmd, &sipParam, attrTemp, devsetInfo.pageInfo);
                }
            }
            else if (devsetInfo.objtype == GROUP_OBJ_ID)     // 组设置
            {
                for (attrCount =0; attrCount<devsetInfo.attrNum; attrCount++)
                {
                    attrTemp = devsetInfo.opobj+attrCount;
                    mlcoap_devset_group( devsetInfo.cmd, &sipParam, attrTemp, devsetInfo.pageInfo);
                }
            }
            else
            {
                os_server_ctrl_log("obj type is %d!!!", devsetInfo.objtype);
            }
            if (0 == strcmp(devsetInfo.cmd, CTRL_GET))
            {
                if (packPacket->data == NULL)
                {
                    packPacket->data = malloc(1500);
                    memset(packPacket->data, 0, 1500);
                }
                json_ctrl_pack_devset_get_rsp(MLINK_RESP_OK, NULL, &devsetInfo, packPacket->data);
            }
            else
            {
                os_server_ctrl_log("devsetInfo.cmd: %s ", devsetInfo.cmd);
            }
        }
        else
        {
            os_server_ctrl_log("ret is %d!!!", ret);
        }

        // response
        if ( packPacket->data == NULL )
        {
            packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
            memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
            if (ret == kGeneralErr)
            {
                json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_DEVSET, MLINK_RESP_ERROR, packPacket->data);
            }
            else
            {
                json_pack_resp_errCode(COAP_CTRL_NOTI_CMD_DEVSET, MLINK_RESP_OK, packPacket->data);
            }
        }

        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);

        if (devsetInfo.opobj != NULL)
        {
            free(devsetInfo.opobj);
        }
    }

    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_ctrl_syncstatus
 * description:   状态同步请求的处理
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_ctrl_syncstatus(unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
    if (data == NULL || srcaddr == NULL)
    {
        return kGeneralErr;
    }
    SYN_STATUS_T syncDev = {0};
    int ret = 0;
    uint8_t count = 0;
    ret = json_ctrl_unpack_synstatus(data, &syncDev);
    if (ret == kNoErr)
    {
        PSYN_STATUS_OBJ_T psyncObjTemp = NULL;
        DEVICEOBJ_T devObj = {0};
        char endpointId[ENDPOINTID_SIZE] = {0};
        uint32_t addr = 0;
        PKEY_ATTR_T pKeyAttr = NULL;
        REPORT_STATE_CHANGE_T stateInfo = {0};
        char jsonString[256] = {0};
        char *srcaddrTemp = STRTOK(srcaddr, ":");
        stateInfo.stateObj = malloc(sizeof(STATE_OBJ_T));
        for (count=0; count<syncDev.num; count++)
        {
            psyncObjTemp = syncDev.synstatusObj+count;
            if (psyncObjTemp->addr[0] != 0)
            {
//                mlink_parse_subdev_net_addr(TELECOM_ZIGBEE, psyncObjTemp->addr, (uint8_t *)&addr);
                // get device obj according to addr
                ret = storage_read_dev_obj_by_addr(psyncObjTemp->addr, &devObj);
                if (ret != kGeneralErr)
                {
                    memset(stateInfo.stateObj, 0, sizeof(STATE_OBJ_T));
                    stateInfo.subdevNum = 1;
                    strcpy(stateInfo.stateObj->addr, psyncObjTemp->addr);
                    mlink_generate_endpoint_id(DEVICE_OBJ_ID, devObj.deviceId, psyncObjTemp->key, endpointId);
                    pKeyAttr = storage_get_endpoint_status(devObj.deviceId, endpointId);
                    if (pKeyAttr)
                    {
                        if (pKeyAttr->keyState.valtype == VALTYPE_HEX_STR)       // 如果数据大小超过4个字节，使用HEX 的形式
                        {
                            sprintf(stateInfo.stateObj->status, "[%s|%s|HEX:%s]", pKeyAttr->keyType, pKeyAttr->keyState.key, pKeyAttr->keyState.value);
                        }
                        else if (pKeyAttr->keyState.valtype < VALTYPE_HEX_STR)
                        {
                            sprintf(stateInfo.stateObj->status, "[%s|%s|%s]", pKeyAttr->keyType, pKeyAttr->keyState.key, pKeyAttr->keyState.value);
                        }
                    }
                    json_msg_pack_statechange(&stateInfo, jsonString);
                    mlcoap_client_send_msg_ex(srcaddrTemp, jsonString);
                }
            }
        }
        if (stateInfo.stateObj)
        {
            free(stateInfo.stateObj);
        }
    }
    if (syncDev.synstatusObj)
    {
        free(syncDev.synstatusObj);
    }
    packPacket->echoValue = MLCOAP_RET_OK;
}

/********************************************************
 * function: mlcoap_server_ctrl_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_ctrl_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = kNoErr;
    MUTEX_LOCK(&g_server_ctrl_mutex);
//    os_server_ctrl_log(" mlcoap_server_ctrl_notify: cmd[%d] size[%d]\n", cmd, size);
    switch(cmd)
    {
        case COAP_CTRL_NOTI_CMD_READ:
            {
                mlcoap_server_ctrl_read(data, size, packPacket);
            }
            break;
        case COAP_CTRL_NOTI_CMD_READMULT:
//            mlcoap_server_ctrl_readmult(data, size, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_WRITE:
            mlcoap_server_ctrl_write(data, size, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_WRITEMULT:
//            mlcoap_server_ctrl_writemult(data, size, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_WRITEGROUP:
//            mlcoap_server_ctrl_writegroup(data, size, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_DEVSET:
//            mlcoap_server_ctrl_devset(data, size, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_SYNCSTATUS:
//            mlcoap_server_ctrl_syncstatus(data, size, srcaddr, packPacket);
            break;
        case COAP_CTRL_NOTI_CMD_ERR:
            {
//                json_pack_resp_errCode(cmd, MLINK_RESP_ERROR, packPacket->data);
//                packPacket->datalen = strlen(packPacket->data);
//                packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
            }
        default:
            ret = kGeneralErr;
            break;
    }
    MUTEX_UNLOCK(&g_server_ctrl_mutex);

    return ret;
}

/********************************************************
 * function: mlcoap_client_ctrl_write
 * description:
 * input:   1. net_panid: 网络地址 panid
 *          2. pattr_data:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_client_ctrl_write(char *net_panid, PDEVATTR_WRITE_T pattr_data)
{
    if ((net_panid != NULL)&&(pattr_data != NULL))
    {
        char payload[512] = {0};
        int addrIndex = 0;
        NETDEV_ADDR_INFO sendAddrInfo  = {0};

        for (addrIndex=0; addrIndex < SEND_ADDR_NUM_MAX; addrIndex++)
        {
            mlink_sys_get_sendaddr(addrIndex, &sendAddrInfo);
            if (( 0==strcmp(sendAddrInfo.panid, net_panid) )&&( sendAddrInfo.addr[0] != 0 ))
            {
                json_pack_ctrl_write(pattr_data, payload);
        //        mlcoap_client_send_msg_multicast(payload);
                os_server_ctrl_log("=====*********** send addr %s addrIndex:%d uuid: %s*******=======", sendAddrInfo.addr, addrIndex, sendAddrInfo.uuid);
                mlcoap_client_send_ctrl_non((unsigned char *)sendAddrInfo.addr, payload);
            }
        }
    }
    return kNoErr;

}

/********************************************************
 * function: mlcoap_server_sys_mutex_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_ctrl_mutex_init( void )
{
    mico_rtos_init_mutex(&g_server_ctrl_mutex);
}
