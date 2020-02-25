/*
 * main_server_discover.c
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
#include "../json/json_disc_deal.h"
#include "main_logic.h"

#define os_server_disc_msg_log(M, ...) custom_log("MAIN_SERVER_DISC_MSG", M, ##__VA_ARGS__)

//static uint8_t g_observeMsgflag = 0;

/********************************************************
 * function: mlcoap_server_msg_linktask
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_msg_state_parse(unsigned char *content, uint8_t *num, PSTATE_DATA_T state_data)
{
    char *pStateArray[ENDPOINTLIST_NUM] = {0};
    char *param[3] = {0};
    uint8_t stateNum = 0;
    uint8_t paramNum = 0;
    char *value[2] = {0};
    uint8_t valueNum = 0;
    uint8_t count = 0;
    int ret = 0;
    if (( content==NULL )||( num==NULL )||( state_data==NULL ))
    {
        return kGeneralErr;
    }

    // 解析获取状态数组
    ret = mlink_parse_state_array_content(content, pStateArray, &stateNum);
    if (ret == kNoErr)
    {
//        os_server_disc_msg_log("state num is %d !!!", stateNum);
        for (count=0; count<stateNum; count++)
        {
            os_server_disc_msg_log("pStateArray[%d]: %s", count, pStateArray[count]);
            // 解析状态获取key和value值
            ret = mlink_parse_state_content(pStateArray[count], param, &paramNum);
            if (paramNum<3)
            {
                return kGeneralErr;
            }
            strcpy(state_data[count].keyType, param[0]);
            strcpy(state_data[count].key, param[1]);
            mlink_parse_state_value(param[2], value, &valueNum);
            if (!strcmp(value[0], "HEX"))
            {
                strcpy(state_data[count].value, value[1]);
            }
            else
            {
                uint32_t val = 0;
                char valueTemp[64] = {0};
                val = (uint32_t)atoi(value[0]);
                HexToStrEx(&val, 4, valueTemp);
                sprintf(state_data[count].value, "%s%s", state_data[count].value, valueTemp);
            }
            os_server_disc_msg_log("num: %d, value: %s", valueNum, state_data[count].value);
            (*num) ++;
        }
    }
    return ret;
}

/********************************************************
 * function: service_statechange_subdev_deal
 * description:  对接收到的子设备状态通知处理
 * input:    1. dev_num:   设备数量
 *           2. pdev_state_obj: 指向设备状态对象数组
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus service_statechange_subdev_deal(int dev_num, PSTATE_OBJ_T pdev_state_obj)
{
    STATE_DATA_T stateData[ENDPOINTLIST_NUM] = {0};
    uint8_t stateNum = 0;
    uint8_t keyCount = 0;
    PSTATE_OBJ_T pStateObjTemp = NULL;
    char endpointId[ENDPOINTID_SIZE] = {0};
    uint8_t count = 0;
    char *pContentTemp = NULL;
    int ret = 0;

    for (count=0; count < dev_num; count++)
    {
        pStateObjTemp = pdev_state_obj + count;
        // add 20180515   scene report deal
        if (pStateObjTemp->endpointid[0] != 0)
        {
            ENDPOINT_ELE_T endpointEle = {0};
            memcpy(endpointId, pStateObjTemp->endpointid, ENDPOINTID_SIZE);
            mlink_parse_endpointid(endpointId, &endpointEle);
            if (endpointEle.classId == SCENE_OBJ_ID)
            {
                mlcoap_server_ctrl_write_subdev(SCENE_OBJ_ID, endpointEle.devid, 0, NULL, 0);
                continue;
            }
        }

        memset(stateData, 0, sizeof(stateData));
        pStateObjTemp->status[strlen(pStateObjTemp->status)-1] = '\0';
        pContentTemp = pStateObjTemp->status+1;
        // 值分解
        ret = mlcoap_msg_state_parse(pContentTemp, &stateNum, stateData);
//            os_server_disc_msg_log("mlcoap_msg_state_parse ===   ret: %d, stateNum:%d, devid: %s", ret, stateNum, pStateObjTemp->subdevid);
        if (kGeneralErr == ret)
        {
            return kGeneralErr;
        }

        if (pStateObjTemp->addr[0] != 0)
        {
            zigbee_statechange_notify(pStateObjTemp->addr, stateNum, stateData);
        }
        for (keyCount = 0; keyCount<stateNum; keyCount++)
        {
            memset(endpointId, 0, ENDPOINTID_SIZE);
            os_server_disc_msg_log("devid: %s, key: %s", pStateObjTemp->subdevid, stateData[keyCount].key);
            // genenrate endpointId based on key
            mlink_generate_endpoint_id(DEVICE_OBJ_ID, pStateObjTemp->subdevid, stateData[keyCount].key, endpointId);
            // 判断并执行输入输出端点链接
            main_endpoint_link(endpointId, stateData[keyCount].value);

            // 判断联动处理
            if (strlen(stateData[keyCount].value)<=8)
            {
                int val = 0;
                char value[12] = {0};
                StrToHexEx(stateData[keyCount].value, 8, (uint8_t *)&val);
                sprintf(value, "%d", val);
                main_linkage_single_deal( DEVICE_OBJ_ID, pStateObjTemp->subdevid, stateData[keyCount].key, VALTYPE_INT, value );
            }
            else
            {
                main_linkage_single_deal( DEVICE_OBJ_ID, pStateObjTemp->subdevid, stateData[keyCount].key, VALTYPE_HEX_STR, stateData[keyCount].value );
            }
        }
    }
    return kGeneralErr;
}

/********************************************************
 * function: mlcoap_server_msg_statechange
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_msg_statechange(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    REPORT_STATE_CHANGE_T stateInfo = {0};
    if (NULL != packPacket)
    {
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    // 解析获取变更数据
    json_msg_unpack_statechange(data, &stateInfo);

    if (stateInfo.stateObj != NULL)
    {
        service_statechange_subdev_deal(stateInfo.subdevNum, stateInfo.stateObj);
        // 释放动态空间
        free(stateInfo.stateObj);
    }
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_msg_event
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void mlcoap_server_msg_event(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    EVENT_REPORT_T notifyEvent = {0};
    if (NULL != packPacket)
    {
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    json_msg_unpack_event(data, &notifyEvent);
    // 输入输出端点链接处理
    main_endpoint_link(notifyEvent.msgContent.endpointid, notifyEvent.msgContent.value);

//    main_linkage_deal();
    // judge link
    os_server_disc_msg_log("event %s trigger!!!", notifyEvent.msgContent.endpointid);
}

/********************************************************
 * function: mlcoap_server_msg_linktask
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_msg_linktask(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_msg_changenotify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus mlcoap_server_msg_changenotify(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    REPORT_STATE_CHANGE_T changeInfo = {0};
    uint8_t count = 0;
    char *pContentTemp = NULL;
    if (NULL != packPacket)
    {
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    // 解析获取变更数据
    json_msg_unpack_statechange(data, &changeInfo);

//    if (changeInfo.stateObj != NULL)
//    {
//        for (count=0; count<changeInfo.subdevNum; count++)
//        {
//            changeInfo.stateObj->status[strlen(changeInfo.stateObj->status)-1] = '\0';
//            pContentTemp = changeInfo.stateObj->status+1;
//            // 值分解
////            mlcoap_msg_state_parse();
//
//            // 判断并执行输入输出端点链接
//            //    main_endpoint_link(notifyEvent.msgContent.endpointid, )
//        }

        // 释放动态空间
        if (changeInfo.stateObj != NULL)
        {
            free(changeInfo.stateObj);
        }

//    }
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_msg_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_msg_state_noitfy(unsigned char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    uint8_t count = 0;
    PDEV_ENDPOINT_STATUS_T pdevEndpointsState = NULL;
//    static char *sendData = NULL;
    char sendData[1024] = {0};
    char changeState[512] = {0};
    REPORT_STATE_CHANGE_T stateInfo = {0};
    STATE_OBJ_T stateObj[2] = {0};
    PDEVSTATUSOBJ_T pdevStateObj = NULL;
    uint8_t devNum = storage_get_device_num();
    NETDEVOBJ_T netDev = {0};

    stateInfo.stateObj = &stateObj;
    os_server_disc_msg_log("dev num is %d", devNum);
    storage_read_local_devobj(&netDev);
    memcpy(stateInfo.devid, netDev.uuid, sizeof(stateInfo.devid));
    stateInfo.subdevNum = 0;

    for (count=0; count<devNum; count++)
    {
        uint8_t devIndex = count%2;
        memset(&stateObj[devIndex], 0, sizeof(STATE_OBJ_T));
        memset(changeState, 0, sizeof(changeState));

        pdevEndpointsState = storage_get_dev_endpoints_status(count);
        if (pdevEndpointsState != NULL)
        {
            if (pdevEndpointsState->endpointNum > 0)
            {
                int len = 0;
                PKEY_ATTR_T pkeyAttrTemp = NULL;
                uint8_t attrIndex = 0;
                PENDPOINT_STATUS_T endpointStatusTemp = NULL;
                uint8_t isAlarmflag = 0;

//                storage_get_dev_status_starting( count+1, &pdevStateObj );
                storage_get_dev_status(pdevEndpointsState->fid, &pdevStateObj);
                os_server_disc_msg_log("fid: %s", pdevEndpointsState->fid);
                changeState[0] = '[';
                if (pdevStateObj != NULL)
                {
                    sprintf(changeState, "%s10|10|%d,", changeState, pdevStateObj->status.statusInfo);
                }

                for (attrIndex = 0; attrIndex < pdevEndpointsState->endpointNum; attrIndex ++)
                {
                    endpointStatusTemp = pdevEndpointsState->endpointStatus+attrIndex;
                    pkeyAttrTemp = &endpointStatusTemp->keyAttr;
                    if ((strcmp(pkeyAttrTemp->keyType, KEY_ALARM_STATE)!=0) && (strcmp(pkeyAttrTemp->keyType, KEY_ALARM_DELAY)!=0) )
                    {
                        if (pkeyAttrTemp->keyState.key[0] != 0)
                        {
                            if (pkeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR)       // 如果数据大小超过4个字节，使用HEX 的形式
                            {
                                sprintf(changeState, "%s%s|%s|HEX:%s,", changeState, pkeyAttrTemp->keyType, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                            }
                            else if (pkeyAttrTemp->keyState.valtype < VALTYPE_HEX_STR)
                            {
                                sprintf(changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                            }
                        }

                    }
                    else
                    {
                        isAlarmflag = 1;
                    }
                }
                if (isAlarmflag)
                {
                    ALARM_ENDPOINT_T alarmEndpoint = {0};
                    storage_search_dev_alarm(pdevEndpointsState->fid, &alarmEndpoint);
                    sprintf(changeState, "%s30|30|%d,31|31|%d,", changeState, alarmEndpoint.alarmType, alarmEndpoint.alarmDelay);
                }
//                alarmIndex = storage_search_dev_alarm(devid, &alarmEndpoint);


                len = strlen(changeState);
                changeState[len-1] = ']';
                stateInfo.subdevNum++;
                os_server_disc_msg_log("sub device num is %d, state change report is %s", stateInfo.subdevNum, changeState);

                memcpy(stateObj[devIndex].status, changeState, strlen(changeState));
                memcpy(stateObj[devIndex].subdevid, pdevEndpointsState->fid, DEVICEID_SIZE);
        //        mlink_generate_endpoint_id(NETDEV_OBJ_ID, stateInfo.stateObj->subdevid, NULL, stateInfo.stateObj->endpointid);
                if ((stateInfo.subdevNum == 2) || (count+1 == devNum))
                {
                    memset(sendData, 0, sizeof(sendData));
                    json_msg_pack_statechange(&stateInfo, sendData);

                    // 1.observe notify 2.multicast notify 3.mqtt notify
                    ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                    stateInfo.subdevNum = 0;
                }
            }
            else
            {
                if (stateInfo.subdevNum > 0)
                {
                    memset(sendData, 0, sizeof(sendData));
                    json_msg_pack_statechange(&stateInfo, sendData);

                    // 1.observe notify 2.multicast notify 3.mqtt notify
                    ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                    stateInfo.subdevNum = 0;
                }
                break;
            }
        }
        else
        {
            os_server_disc_msg_log("endpoint state is NULL");
        }
    }
    return kNoErr;
}

/********************************************************
 * function: mlcoap_server_msg_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_msg_allow_noitfy(unsigned char *data, int size,  char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
    ALLOW_NOTIFY_INFO_T info = {0};
    if (NULL != packPacket)
    {
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    json_msg_unpack_allow_notify(data, &info);
    if (info.uuid[0] != 0)
    {
        NETDEV_ADDR_INFO addrInfo = {0};
        char *temp = NULL;

        temp = STRTOK(srcaddr, ":");
        strcpy(addrInfo.addr, temp);
        memcpy(addrInfo.uuid, info.uuid, UUID_SIZE);
        memcpy(addrInfo.panid, info.panid, 4);

        os_server_disc_msg_log("uuid: %s, srcaddr: %s", info.uuid, temp);
        mlink_sys_add_state_sendaddr(&addrInfo);
    }
}

/********************************************************
 * function: mlcoap_server_msg_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_msg_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
//    g_observeMsgflag = 1;
    OSStatus ret = -1;//kNoErr;
    debug(" mlcoap_server_msg_notify: cmd[%d] size[%d]\n", cmd, size);
    switch(cmd)
    {
        case COAP_MSG_NOTI_CMD_EVENT:
            {
//                mlcoap_server_msg_event(data, size, packPacket);
            }
            break;
        case COAP_MSG_NOTI_CMD_LINKTASK:
//            mlcoap_server_msg_linktask(data, size, packPacket);
            break;
        case COAP_MSG_NOTI_CMD_STATECHANGE:
//            mlcoap_server_msg_statechange(data, size, packPacket);
            break;
        case COAP_MSG_NOTI_CMD_CHANGENOTIFY:
//            mlcoap_server_msg_changenotify(data, size, packPacket);
            break;
        case COAP_EVENT_NOTI_CMD_OBSERVER:
        {
            // 设备订阅时，取消查询设备端点状态处理 2018.3.8
//            mlcoap_server_msg_state_noitfy(data, size, packPacket);
//            os_server_disc_msg_log("event observe !!!! memory: %d", MicoGetMemoryInfo()->free_memory);
            ret = kNoErr;
        }
        break;
        case COAP_MSG_NOTI_CMD_ALLOW_NOTIFY:
        {
//            mlcoap_server_msg_allow_noitfy(data, size, srcaddr, packPacket);
            ret = kNoErr;
        }
            break;
        default:
            ret = kGeneralErr;
            break;
    }

    // response itself
    if (packPacket->data == NULL)
    {
//        os_server_disc_msg_log("RESPONSE DATA");
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        packPacket->datalen = 0;
    }

    return ret;
}

/********************************************************
 * function: mlcoap_server_discover_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_discover_notify(COAP_NOTI_CMD_ID_E cmd, char *data, int size, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = kNoErr;

    debug(" mlcoap_server_discover_notify: cmd[%d] size[%d]\n", cmd, size);

    NETDEVOBJ_T netDevObj;
    if (packPacket->data == NULL)
    {
        packPacket->data = malloc(512);
        memset(packPacket->data, 0, 512);
    }
    if (cmd == COAP_DISCOVER_NOTI_CMD_ERR)
    {
        json_pack_resp_errCode(COAP_DISCOVER_NOTI_CMD_ERR, MLINK_RESP_ERROR, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }
    else if (cmd == COAP_DISCOVER_NOTI_CMD_WHOIS)
    {
        // notify discover
        char uuid[UUID_SIZE] = {0};
        char sendData[1024] = {0};
        // response ok
        memset(packPacket->data, 0, 512);


        json_disc_unpack_whois(data, uuid);
        memset(&netDevObj, 0 , sizeof(NETDEVOBJ_T));
        storage_read_local_devobj(&netDevObj);
        if (uuid[0] != 0)
        {
//            os_server_disc_msg_log("netDevObj.uuid: %s, uuid: %s", netDevObj.uuid, uuid);
            if (0 != strcmp(netDevObj.uuid, uuid))
            {
                packPacket->datalen = 0;
                packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_BADREQ);
                return ret;
            }
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
            json_pack_who_is_response(MLINK_RESP_OK, NULL, &netDevObj, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
        }
        json_disc_pack_i_am(&netDevObj, sendData);
        ml_coap_notify_obs(ML_COAP_URITYPE_DISCOVER, sendData, strlen(sendData));
    }
    else if (cmd == COAP_DISCOVER_NOTI_CMD_WHO_HAV)
    {
        uint32_t timeout = 0;
#ifdef ZIGBEE_DEVICE
        json_disc_unpack_who_hav(data, &timeout);
        if (timeout == 0)
        {
            timeout = DISCOVER_TIMEOUT_DEFAULT;
        }
        zigbee_discover_device(timeout);
#endif
        // response ok
        json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
        packPacket->datalen = strlen(packPacket->data);
        packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
    }

    return ret;
}




