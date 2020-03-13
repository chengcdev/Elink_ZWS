/********************************************************
 * @file: main_logic.c
 * @author: chenb
 * @brief:
 * 1. append the function that make it reboot in night.
 *      (20180907) (function:"main_monitor_reboot_timer")
 *
 */

#include "mico.h"

#include "StringUtils.h"
#include "MLinkAppDef.h"
#include "ml_coap.h"
#include "../zigbee/zigbee.h"
#include "MLinkCommand.h"
#include "../uart/uart_logic.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../uart/DTTable.h"
#include "main_logic.h"
#include "main_alarm.h"
#include "../cloud/cloud.h"
#include "../MLinkGpio/MLinkLedLogic.h"
#include "../rf433/rf433.h"

#ifdef BLE_DEVICE
#include "../ble/bluetooth_logic.h"

#endif

#ifdef BLE_ELINK
#include "../elink/elink_logic_devchanel_deal.h"
#include "../time/time.h"
#endif
#define os_main_logic_log(M, ...) custom_log("MAIN_LOGIC", M, ##__VA_ARGS__)
#define GETTIME_INTERVAL    (60)
#define TIME_ZONE           (8*60*60)
#define SEC_BOUNDARY        (60*60*4)   // 0点后计数重启的最大秒�?
#define DAY_BOUNDARY        (2)         // 等待执行重启所需最长天�?

static uint8_t g_demolishFlag = 0;     // 设备拆除 0 正常 1 异常
static uint8_t g_alarmFlag = 0;        // 设备电量告警 0 正常 1 异常
static uint8_t g_signalFlag = 128;       // 设备信号强度,实际的db�?

/********************************************************
 * function: st_coap_client_notify
 * description:  server callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus st_coap_server_notify( COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size,
                                char *srcaddr, ml_coap_ackPacket_t *packPacket )
{
    OSStatus ret = kNoErr;

    switch ( cmd )
    {
        case COAP_SYS_NOTI_CMD_RESET:
            case COAP_SYS_NOTI_CMD_FACTORY:
            case COAP_SYS_NOTI_CMD_SYNCTIME:
            case COAP_SYS_NOTI_CMD_SETMESH:
            case COAP_SYS_NOTI_CMD_SETNET:
            case COAP_SYS_NOTI_CMD_REGDEV:
            case COAP_SYS_NOTI_CMD_SETCLOUD:
            case COAP_SYS_NOTI_CMD_GETDEVINFO:
            case COAP_SYS_NOTI_CMD_SETADDR:
            case COAP_SYS_NOTI_CMD_KEEPALIVE:
            case COAP_SYS_NOTI_CMD_UPGRADE:
            case COAP_SYS_NOTI_CMD_RESETMESH:
            case COAP_SYS_NOTI_CMD_SENDADDR:
            case COAP_SYS_NOTI_CMD_CLEARDATA:
            case COAP_SYS_NOTI_CMD_ERR:
            ret = mlcoap_server_sys_notify( cmd, data, size, srcaddr, packPacket );
            break;

        case COAP_DISCOVER_NOTI_CMD_WHOIS:
            case COAP_DISCOVER_NOTI_CMD_WHO_HAV:
            case COAP_DISCOVER_NOTI_CMD_IAM:
            case COAP_DISCOVER_NOTI_CMD_IHAV:
            case COAP_DISCOVER_NOTI_CMD_ERR:
            {
            ret = mlcoap_server_discover_notify( cmd, data, size, packPacket );
        }
            break;

        case COAP_MANAGER_NOTI_CMD_GET:
            case COAP_MANAGER_NOTI_CMD_ADD:
            case COAP_MANAGER_NOTI_CMD_DEL:
            case COAP_MANAGER_NOTI_CMD_CLEAR:
            case COAP_MANAGER_NOTI_CMD_UPDATE:
            case COAP_MANAGER_NOTI_CMD_ERR:
            ret = mlcoap_server_manager_notify( cmd, data, size, packPacket );
            break;

        case COAP_CTRL_NOTI_CMD_READ:
            case COAP_CTRL_NOTI_CMD_WRITE:
            case COAP_CTRL_NOTI_CMD_READMULT:
            case COAP_CTRL_NOTI_CMD_WRITEMULT:
            case COAP_CTRL_NOTI_CMD_WRITEGROUP:
            case COAP_CTRL_NOTI_CMD_DEVSET:
            case COAP_CTRL_NOTI_CMD_SYNCSTATUS:
            case COAP_CTRL_NOTI_CMD_ERR:
            ret = mlcoap_server_ctrl_notify( cmd, data, size, srcaddr, packPacket );
            break;
        case COAP_MSG_NOTI_CMD_EVENT:
            case COAP_MSG_NOTI_CMD_LINKTASK:
            case COAP_MSG_NOTI_CMD_STATECHANGE:
            case COAP_MSG_NOTI_CMD_CHANGENOTIFY:
            case COAP_MSG_NOTI_CMD_ALLOW_NOTIFY:
            case COAP_EVENT_NOTI_CMD_OBSERVER:
            {
            os_main_logic_log("excute observe msg operate !!! srcaddr: %s", srcaddr);
            ret = mlcoap_server_msg_notify( cmd, data, size, srcaddr, packPacket );
            break;
        }
        default:
            ret = kGeneralErr;
            break;
    }

    return ret;
}

/********************************************************
 * function: st_coap_client_notify
 * description:  client callback in the coap module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus st_coap_client_notify( COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size,
                                char *srcaddr, ml_coap_ackPacket_t *packPacket )
{
    OSStatus ret = kNoErr;
    return ret;
}

#ifdef RF433_DEVICE
/********************************************************
 * function: main_discover_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_discover_notify(PDISC_RFDEV_INFO_T subDevInfo)
{
    REPORT_SUBDEV_INFO_T repSubDevInfo;
    char data[512] =
    {   0};
    uint8_t wlanStatus = 0;
    mlink_sys_get_status(SYS_WIFI_STATE, &wlanStatus);

    if (wlanStatus == NOTIFY_STATION_UP)        // wifi正常连接
    {
        memset(&repSubDevInfo, 0, sizeof(REPORT_SUBDEV_INFO_T));
        mlink_generate_subdevid(repSubDevInfo.fid);
        HexToStr(subDevInfo->mac, 6, repSubDevInfo.mac);
        mlink_generate_subdev_net_addr(subDevInfo->addr, repSubDevInfo.netaddr);

        repSubDevInfo.comm = subDevInfo->comm;
        HexToStr(subDevInfo->mid, 3, repSubDevInfo.modelid);

        /**pack disvocer notify json***/
        memset(data, 0, sizeof(data));
        json_disc_pack_i_hav(&repSubDevInfo, data);
        ml_coap_notify_obs(ML_COAP_URITYPE_DISCOVER, data, strlen(data));
    }
}
#endif

#ifdef ZIGBEE_DEVICE
/********************************************************
 * function: main_discover_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_discover_notify(PDISC_DEV_INFO_T subDevInfo)
{
    REPORT_SUBDEV_INFO_T repSubDevInfo;
    char data[512] =
    {   0};
    char modelid[3] =
    {   0};
    uint8_t setmeshFlag = 0;
    uint8_t wlanStatus = 0;
    int ret = 0;

    memset(&repSubDevInfo, 0, sizeof(REPORT_SUBDEV_INFO_T));
    // 生成 modelid
    modelid[0] = subDevInfo->devtype[0];
    modelid[1] = subDevInfo->devtype[1];
    modelid[2] = subDevInfo->devtype[2]&0x0f;
    HexToStr(modelid, 3, repSubDevInfo.modelid);

    // 如果设备处于组网状态，设备上报时让他进入组网状态， 否则获取设备状�?
    mlink_sys_get_status(SYS_MESH_STATE, &setmeshFlag);
    os_main_logic_log("setmeshFlag: %d", setmeshFlag);
    if (setmeshFlag == MESH_IN)
    {
        zigbee_setmesh(subDevInfo->ShortAddr, SETMESH_TIMEOUT);
    }
    else
    {
        //搜索设备时，不进行子设备的状态同�?- note by 2018.3.8
//        zigbee_devstatus_query(subDevInfo->ShortAddr, repSubDevInfo.modelid);
    }

    mlink_sys_get_status(SYS_WIFI_STATE, &wlanStatus);
    if (wlanStatus == NOTIFY_STATION_UP)        // wifi正常连接
    {
        mlink_generate_subdevid(repSubDevInfo.fid);
        HexToStr(&subDevInfo->IEEEAddr, 8, repSubDevInfo.mac);
        mlink_generate_subdev_net_addr(subDevInfo->ShortAddr, repSubDevInfo.netaddr);
        repSubDevInfo.comm = subDevInfo->devtype[2]>>4;

        if (subDevInfo->hardware)
        {
            sprintf(repSubDevInfo.verHardware, "%02d.%02d", (uint8_t)(subDevInfo->hardware >> 8), (uint8_t)subDevInfo->hardware);
        }
        if (subDevInfo->software)
        {
            sprintf(repSubDevInfo.verSoftware, "%02d.%02d", (uint8_t)(subDevInfo->software >> 8), (uint8_t)subDevInfo->software);
        }
        if (subDevInfo->protVer)
        {
            sprintf(repSubDevInfo.verProt, "%d", subDevInfo->protVer);
        }

        /**pack disvocer notify json***/
        memset(data, 0, sizeof(data));
        json_disc_pack_i_hav(&repSubDevInfo, data);
        ret = ml_coap_notify_obs(ML_COAP_URITYPE_DISCOVER, data, strlen(data));
        os_main_logic_log("coap observe ret: %d", ret);
    }
}
#endif

#ifndef BLE_DEVICE
/********************************************************
 * function: main_uart_init
 * description:  init all about uart
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_uart_init( void )
{
    uart_logic_init();
    uart_discover_notify_init(main_discover_notify);
}
#endif

/********************************************************
 * function: main_switch_ctrl_content
 * description:  init all about uart
 * input:    1. content:
 *           2. input_val
 * output:  1. type
 *          2. output_val
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_get_ctrl_data( char *content, char *input_val, int *type, char* output_val )
{
    char *ctrlStr[10] = { NULL };
    uint8_t ctrlNum = 0;
    CTRL_PARAM_T param = { NULL, NULL, NULL };
    uint8_t paramNum = 0;
    uint8_t count = 0;
    int ret = 0;
    char replVal[VALUE_SIZE] = { 0 }; // 保存通配字符串锁对应的字符的数据�?: CCFF00 与input_val中CC1000匹配，FF位置匹配的是10，那么该字符数组将存�?0�?
    char replacedFmt[VALUE_SIZE] = { 0 };           // 需要被替换的字符串
    char wildcharStr[10] = WILDCARD_STR;
    char attrIdWildchar[3] = "SS";
    char attrId[3] = { 0 };

    // 获取控制�?
    mlink_parse_ctrl_content( content, ctrlStr, &ctrlNum );
//    os_main_logic_log("input val: %s, ctrl content: %s, ctrlNum: %d", input_val, content, ctrlNum);

    if ( content[0] == 0 )
    {
        return kGeneralErr;
    }
    // 获取每个控制数据信息
    for ( count = 0; count < ctrlNum; count++ )
    {
        uint8_t paramValLen = 0;
        mlink_parse_ctrl( ctrlStr[count], (char *) &param, &paramNum );
//        os_main_logic_log("input val: %s, ctrl val: %s", input_val, param.outData);

        //判断是否包含FF标识，FF标识是通配�?
        memset( replVal, 0, sizeof(replVal) );

        paramValLen = strlen( param.inValue );
        // modify by chenb    20180520
        if ( strlen( input_val ) != paramValLen )
        {
            char inputVal[16] = { 0 };
            memset( inputVal, '0', 16 );
            memcpy( inputVal, input_val, strlen( input_val ) );
            inputVal[paramValLen] = '\0';
            ret = mlink_value_cmp( inputVal, param.inValue, wildcharStr, attrIdWildchar, replVal,
                                   replacedFmt, attrId );
        }
        else
        {
            ret = mlink_value_cmp( input_val, param.inValue, wildcharStr, attrIdWildchar, replVal,
                                   replacedFmt, attrId );
        }
        if ( ret == 0 )
        {
            if ( attrId[0] != 0 )
            {
                StrToHexEx( attrId, 2, (uint8_t *) type );
            }
            else
            {
                *type = 0xffffffff;
            }
            if ( replVal[0] != 0 )
            {
                mlink_value_wildcard_replace( param.outData, replacedFmt, replVal );
            }
            if ( paramNum >= 2 )
            {
                if ( strcmp( param.outType, "SS" ) )
                {
                    *type = atoi( param.outType );
                }
            }
            if ( paramNum >= 3 )
            {
                strcpy( output_val, param.outData );
            }
            return kNoErr;
        }
    }
    return kGeneralErr;
}

/********************************************************
 * function: main_device_heart_report
 * description:
 * input:       1. devid
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_device_heart_report( char *devid )
{
    uint8_t sendData[256] = { 0 };
    REPORT_STATE_CHANGE_T stateInfo = { 0 };
    STATE_OBJ_T stateObj = { 0 };
    PDEVSTATUSOBJ_T pDevState = NULL;
    int ret = 0;
    stateInfo.subdevNum = 1;
    stateInfo.stateObj = &stateObj;
    ret = storage_get_dev_status( devid, &pDevState );
    if ( ret != 0 )
    {
        strcpy( stateInfo.stateObj->subdevid, devid );
        sprintf( stateInfo.stateObj->status, "[10|10|%u]", pDevState->status.statusInfo );
        json_msg_pack_statechange( &stateInfo, sendData );

        // 1.observe notify 2.multicast notify 3.mqtt notify
        ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
    }

}

/********************************************************
 * function: main_online_deal
 * description:
 * input:       1. devid
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_online_deal( char *devid )
{
    // flag and active device state
    storage_set_dev_online_state( devid, 1 );
}

/********************************************************
 * function: main_offline_deal
 * description:
 * input:       1. devid
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_offline_deal( char *devid )
{
    // flag and active device state
    storage_set_dev_online_state( devid, 0 );
    // coap离线上报
    main_device_heart_report( devid );

}

/********************************************************
 * function: main_coap_noti_msg_to_netdev
 * description:
 * input:       1. payload
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int main_coap_noti_msg_to_netdev( unsigned char *payload )
{
    int addrIndex = 0;
    NETDEV_ADDR_INFO sendAddrInfo = { 0 };
    for ( addrIndex = 0; addrIndex < SEND_ADDR_NUM_MAX; addrIndex++ )
    {
        mlink_sys_get_sendaddr( addrIndex, &sendAddrInfo );
        if ( sendAddrInfo.addr[0] != 0 )
        {
            os_main_logic_log("=====*********** send addr %s addrIndex:%d uuid: %s*******=======", sendAddrInfo.addr, addrIndex, sendAddrInfo.srcmac);
            mlcoap_client_send_msg_non( (unsigned char *) sendAddrInfo.addr, URI_SERVICE_MSG,
                                        payload );
        }
    }
    return kNoErr;
}

/********************************************************
 * function: main_coap_noti_event_to_netdev
 * description:
 * input:       1. payload
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
//int main_coap_noti_event_to_netdev( unsigned char *payload )
//{
//    int count = 0;
//    int num = mlink_sys_get_sendaddr_num();
//    NETDEV_ADDR_INFO netAddrInfo = {0};
//    for ( count = 0; count < num; count++ )
//    {
//        memset(&netAddrInfo, 0, sizeof(NETDEV_ADDR_INFO));
//        mlink_sys_get_sendaddr(count, &netAddrInfo);
//        if (netAddrInfo.srcmac[0] != 0)
//        {
//            mlcoap_client_send_msg_ex(netAddrInfo.addr, payload);
//        }
//    }
//}
/********************************************************
 * function: main_endpoint_link
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
static char *st_pendpointlinkList = NULL;

OSStatus main_endpoint_link( char *endpoint_id, char *input_val )
{
    PLINK_OBJ_T plinkObj = NULL;
//    PENDPOINTLINK_LIST_T pendpointlinkList = malloc(sizeof(ENDPOINTLINK_LIST_T));
    DEV_FUNCTION_OBJ_T devAttrObj = { 0 };
    int ret = 0;
    uint8_t count = 0;
    int writeResult = 0;

    /** add by 2018.3.12***/
    PENDPOINTLINK_LIST_T pendpointlinkList = NULL;
    if ( st_pendpointlinkList == NULL )
    {
        st_pendpointlinkList = malloc( sizeof(ENDPOINTLINK_LIST_T) );
    }
    pendpointlinkList = (PENDPOINTLINK_LIST_T) st_pendpointlinkList;
    /******end add *************/

    memset( pendpointlinkList, 0, sizeof(ENDPOINTLINK_LIST_T) );
    // 根据输入端点ID获取输入输出端点对象
    ret = storage_get_endpoint_linkobj( endpoint_id, pendpointlinkList );
    if ( ret == kGeneralErr )
    {
        os_main_logic_log("======Get endpoint link object fail ![%d]======", MicoGetMemoryInfo()->free_memory);
        return edLinkNotFound;
    }
//    else
//    {
//        os_main_logic_log("link num is %d, input endpointid: %s", pendpointlinkList->linkNum, endpoint_id);
//    }
    for ( count = 0; count < pendpointlinkList->linkNum; count++ )
    {
        ENDPOINT_ELE_T outEndpointEle = { 0 };
        int ret0 = 0;
        plinkObj = &pendpointlinkList->linkArray[count];
        ret0 = main_get_ctrl_data( plinkObj->ctrlContent, input_val, &devAttrObj.type,
                                   devAttrObj.value );
        if ( ret0 == kNoErr )      // 查找到匹配的输入数据
        {
            uint8_t keyLen = 0;
            mlink_parse_endpointid( plinkObj->outEndpointId, &outEndpointEle );

            keyLen = strlen( outEndpointEle.key );
            if ( keyLen > 0 )
            {
                memcpy( devAttrObj.key, outEndpointEle.key, keyLen );    // 获取输出端点的key
            }
            devAttrObj.valtype = VALTYPE_HEX_STR;
            if ( outEndpointEle.classId != NETDEV_OBJ_ID )
            {
                writeResult = mlcoap_server_ctrl_write_subdev( outEndpointEle.classId,
                                                               outEndpointEle.devid, 1, &devAttrObj,
                                                               0 );
                if ( (outEndpointEle.classId == SCENE_OBJ_ID)
                    || (writeResult == MLINK_RESP_FIND_NOTHING) )
                {
                    ret = edLinkNotFound;
                }
            }
        }
    }
    if ( pendpointlinkList->linkNum == 0 )
    {
        ret = edLinkNotFound;
    }

//    free(pendpointlinkList);
    return ret;
}

/********************************************************
 * function: main_endpointlink_deal
 * description:
 * input:   1. devid:
 *          2. key_num:
 *          3. pkey_attr:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_endpointlink_deal( char *devid, uint8_t *key_num, PKEY_ATTR_T_EX pkey_attr )
{
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pKeyAttrTemp = NULL;
    PDEV_FUNCTION_OBJ_T pKeyAttrValTemp = NULL;

    if ( key_num == NULL || devid == NULL || pkey_attr == NULL )
    {
        return kGeneralErr;
    }
    // endpoint link
    for ( attrIndex = 0; attrIndex < *key_num; attrIndex++ )
    {
        char endpointId[ENDPOINTID_SIZE] = { 0 };
        char inValue[VALUE_SIZE + 1] = { 0 };
        uint8_t data[VALUE_SIZE / 2] = { 0 };
        uint8_t size = 0;

        pKeyAttrTemp = pkey_attr + attrIndex;
        if ( (pKeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR)
            && (strlen( pKeyAttrTemp->keyState.value ) >= 32) )
        {
            continue;
        }

        pKeyAttrValTemp = (PDEV_FUNCTION_OBJ_T) &pKeyAttrTemp->keyState;

        // 生成输入端点ID
        mlink_generate_endpoint_id( DEVICE_OBJ_ID, devid, pKeyAttrValTemp->key, endpointId );
        if ( pKeyAttrValTemp->valtype != VALTYPE_HEX_STR )
        {
            mlink_translate_value( pKeyAttrValTemp->valtype, pKeyAttrValTemp->value, data, &size );
            HexToStrEx( data, size, inValue );
        }
        else
        {
            strcpy( inValue, pKeyAttrValTemp->value );
        }
        // 检查该输入端点是否要链接操作其他设�?
        main_endpoint_link( endpointId, inValue );
//        os_main_logic_log("main_endpoint_link result is %d ", ret);
//        if ( ret == edLinkNotFound )    // 如果不存在相符的输入输出链接对象或该输出不在当前设备下则将在接下来的状态上报将中将组播宣告
//        {
//            if (attrIndex != count)
//            {
//                // 加入组播数据�?
//                memcpy(pkey_attr+count, pKeyAttrTemp, sizeof(KEY_ATTR_T_EX));
//            }
//            count++;
//        }
    }
//    *key_num = count;
    return kNoErr;
}

/********************************************************
 * function: linkage_check_input
 * description:  检测实际值是否满足条�?
 * input:   1. cmp_type_val: 比较类型�?
 *          2. cmpval_type: 比较值的类型（决定是数值比较还是字符串比较�?
 *          2. cndt_val1:      条件�?
 *          3. cndt_val2:      条件�?
 *          3. actual_val:   实际�?
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int linkage_check_input_is_match( COMPARE_TYPE_E cmp_type_val, int cmpval_type, char *cndt_val1,
                                  char *cndt_val2, char *actual_val )
{
    int cmpResult = 0;
    int isMatch = FALSE;

    // compare the value between condition and actual value.
    if ( cmpval_type == VALTYPE_HEX_STR )
    {
        cmpResult = HexStrCmp( actual_val, cndt_val1 );
        if ( (cmp_type_val == CMP_WITHIN) && cndt_val2 )
        {
            cmpResult *= HexStrCmp( actual_val, cndt_val2 );
        }
    }
    else
    {
        int conditionVal = 0;
        int actualVal = atoi( actual_val );
        StrToHexEx( cndt_val1, strlen( cndt_val1 ), &conditionVal );
        cmpResult = actualVal - conditionVal;
        if ( (cmp_type_val == CMP_WITHIN) && cndt_val2 )
        {
            conditionVal = 0;
            StrToHexEx( cndt_val2, strlen( cndt_val2 ), &conditionVal );
            cmpResult *= (actualVal - conditionVal);
        }
    }

    // check if the result
    switch ( cmp_type_val )
    {
        case CMP_EQUAL:     // '='
            if ( cmpResult == 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_LESS:      // '<'
            if ( cmpResult < 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_LEQ:       // '<='
            if ( cmpResult <= 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_GTR:       // '>'
            if ( cmpResult > 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_GEQ:       // '>='
            if ( cmpResult >= 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_WITHIN:    // '<= && >='
            if ( cmpResult <= 0 )
            {
                isMatch = TRUE;
            }
            break;
        case CMP_IGNORE:
            isMatch = TRUE;
            break;
        default:
            break;
    }
    os_main_logic_log("linkage_check_input_is_match: %d, cmpResult: %d, val: %s, condition: %s", isMatch, cmpResult, actual_val, cndt_val1);

    return isMatch;
}

/********************************************************
 * function: main_linkage_deal
 * description:
 * input:   1. endpointId: 端点ID
 *          2. val:   上报端点状态�?
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_linkage_single_deal( uint32_t classid, char *devid, char *key, int valtype, char *val )
{
    PLINKAGE_INPUT_TASK_T plinkageInputTask = malloc( sizeof(LINKAGE_INPUT_TASK_T) );
    int lnkTaskNum = storage_get_object_num( LINKAGE_INPUT_OBJ_ID );
    int lnkTaskIndex = 0;
    uint8_t inputNum = 0;
    char *logicParam[4] = { NULL, NULL, NULL, NULL }; // 分别用于指向 以下四个元素字符串： 1.输入端点ID; 2.判断条件; 3.�?  4.延时标识;
    uint8_t logicParamNum = 0;
    char endpointId[ENDPOINTID_SIZE] = { 0 };

    mlink_generate_endpoint_id( classid, devid, key, endpointId );
    for ( lnkTaskIndex = 0; lnkTaskIndex < lnkTaskNum; lnkTaskIndex++ )
    {
        // 读取联动输入条件
        storage_read_linkage_input_obj( 1, lnkTaskIndex, plinkageInputTask );

        // 解析联动输入条件获取其endpointid 和判断条件以及延时等信息
        for ( inputNum = 0; inputNum < plinkageInputTask->inputNum; inputNum++ )
        {
            mlink_parse_space_data( plinkageInputTask->lnkInput[inputNum].logic, '|', logicParam,
                                    &logicParamNum );

            if ( !strcmp( logicParam[0], endpointId ) )
            {
                int isMatch = TRUE;
                if ( logicParamNum >= 3 )
                {
                    if ( *logicParam[1] != 0 )
                    {
                        uint8_t cmpTpVal = (uint8_t) atoi( logicParam[1] );
                        isMatch = linkage_check_input_is_match( cmpTpVal, valtype, logicParam[2],
                                                                NULL, val );
                    }
                    os_main_logic_log("cmpTpVal: %s, isMatch: %d", logicParam[1], isMatch);
                    if ( isMatch == TRUE )
                    {
                        mlink_update_linkage_status( plinkageInputTask->linkageId,
                                                     plinkageInputTask->lnkInput[inputNum].index,
                                                     atoi( logicParam[3] ) );
                    }
                }
                else
                {
                    os_main_logic_log("param Num: %d", logicParamNum);
                }
            }
        }
    }
    if ( plinkageInputTask )
        free( plinkageInputTask );
}

/********************************************************
 * function: main_linkage_deal
 * description:
 * input:   1. devid:
 *          2. key_num:
 *          3. pkey_attr:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_linkage_deal( char *devid, uint8_t key_num, PKEY_ATTR_T_EX pkey_attr )
{
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pKeyAttrTemp = NULL;

    for ( attrIndex = 0; attrIndex < key_num; attrIndex++ )
    {
        pKeyAttrTemp = pkey_attr + attrIndex;
        if ( (pKeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR)
            && (strlen( pKeyAttrTemp->keyState.value ) >= 32) )
        {
            continue;
        }
        if ( strcmp( pKeyAttrTemp->keyType, "10" ) )
        {
//            os_main_logic_log("actual val is %s", value);
            main_linkage_single_deal( DEVICE_OBJ_ID, devid, pKeyAttrTemp->keyState.key,
                                      pKeyAttrTemp->keyState.valtype,
                                      pKeyAttrTemp->keyState.value );
        }
    }
}

/********************************************************
 * function: main_check_statechange
 * description:  init all about uart
 * input:    1. keyType:
 *           2. pevent_obj:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_check_statechange( const char *key_type, PEVENTOBJ_T pevent_obj )
{
    int ret = 0;
    ret = storage_check_event( NOTIFY_STATE_CHANGES, key_type, pevent_obj );
    return ret;
}

/********************************************************
 * function: main_check_general_event
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_check_event( const char *key_type, PEVENTOBJ_T pevent_obj )
{
    int ret = 0;
    ret = storage_check_event( NOTIFY_ALARM, key_type, pevent_obj );
    if ( ret == FALSE )
    {
        ret = storage_check_event( NOTIFY_EVENT, key_type, pevent_obj );
    }
    return ret;
}
#ifdef BLE_DEVICE
/********************************************************
 * function: main_event_report
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_event_report( uint8_t *devid, uint8_t *key_num, PKEY_ATTR_T pkey_attr )
{
    uint8_t sendData[256] = { 0 };
    char changeState[64] = { 0 };
    PKEY_ATTR_T pKeyAttrTemp = NULL;
    uint8_t attrIndex = 0;
    int cmd = 0;
    uint8_t eventtype[2] = { 0 };
//    os_main_logic_log("value: %s devid %s", pkey_attr->keyState.value,devid);
    if ( (devid == NULL) || (pkey_attr == NULL) )
    {
        return FALSE;
    }
//    storage_set_mesh_status()

    for ( attrIndex = 0; attrIndex < *key_num; attrIndex++ )
    {
        OSStatus ret = TRUE;
        EVENTOBJ_T eventObj = { 0 };
        char endpointId[ENDPOINTID_SIZE] = { 0 };

        pKeyAttrTemp = pkey_attr + attrIndex;
        if ( 0 == strcmp( pKeyAttrTemp->keyType, "10" ) )
        {
            continue;
        }
        cmd = atoi( pKeyAttrTemp->keyState.key );
        ret = storage_check_event( pKeyAttrTemp->keyType, &eventObj );
        // os_main_logic_log("key type : %s.key:%s notitype: %d  cmd :%d ret: %d", pKeyAttrTemp->keyType, pKeyAttrTemp->keyState.key,eventObj.notitype,cmd,ret);
        ret = TRUE;
        eventObj.notitype = NOTIFY_EVENT;
        if ( ret == TRUE )
        {
            if ( (eventObj.notitype == NOTIFY_ALARM) || (eventObj.notitype == NOTIFY_EVENT) )
            {
                EVENT_REPORT_T notifyEvent = { 0 };
                memset( &notifyEvent, 0, sizeof(EVENT_REPORT_T) );
                if ( cmd == COAP_CTRL_NOTI_CMD_ALARMEVENT )
                {
                    StrToHex( pKeyAttrTemp->keyState.value, strlen( pKeyAttrTemp->keyState.value ),
                              eventtype );
                    if ( _GET_BIT_( eventtype[0], 1 ) )
                    {
                        eventObj.eventlevel = MLINK_EVENTLEVEL_ALARM;
                        eventObj.eventtype = MLINK_EVENTTYPE_UNDER_BATTERY;
                        eventObj.notitype = MLINK_NOTITYPE_ALARM;
                    }
                    else if ( _GET_BIT_( eventtype[0], 2 ) )
                    {
                        eventObj.eventlevel = MLINK_EVENTLEVEL_ALARM;
                        notifyEvent.eventtype = MLINK_EVENTTYPE_ALARM_INFO;
                        notifyEvent.notitype = MLINK_NOTITYPE_ALARM;
                    }
                    else if ( _GET_BIT_(
                        eventtype[0],
                        4 ) || _GET_BIT_(eventtype[0], 5) || _GET_BIT_(eventtype[0], 6) )
                    {
                        eventObj.eventlevel = MLINK_EVENTLEVEL_ALARM;
                        eventObj.eventtype = MLINK_EVENTTYPE_ALARM_INFO;
                        eventObj.notitype = MLINK_NOTITYPE_ALARM;
                    }

                    notifyEvent.eventlevel = eventObj.eventlevel;
                    notifyEvent.eventtype = eventObj.eventtype;
                    notifyEvent.notitype = eventObj.notitype;
                }
                else
                {
                    notifyEvent.eventlevel = MLINK_EVENTLEVEL_NORMAL_EVENT;   //eventObj.eventlevel;
                    notifyEvent.eventtype = MLINK_EVENTTYPE_UNLOCK_RECORD;     //eventObj.eventtype;
                    notifyEvent.notitype = MLINK_NOTITYPE_EVENT;       // eventObj.notitype;
                }

                mlink_generate_endpoint_id( DEVICE_OBJ_ID, devid, pKeyAttrTemp->keyState.key,
                                            notifyEvent.msgContent.endpointid );
                strcpy( notifyEvent.msgContent.key, pKeyAttrTemp->keyState.key );
                //strcpy(notifyEvent.msgContent.value, pKeyAttrTemp->keyState.value);
                //memcpy(notifyEvent.msgContent.value, pKeyAttrTemp->keyState.value,9);
                sprintf( changeState, "HEX:%s", pKeyAttrTemp->keyState.value );
                os_main_logic_log("changeState: %s\n",changeState);
                memcpy( notifyEvent.msgContent.value, changeState, strlen( changeState ) );
                //PrintfByteHex("keyvalue",notifyEvent.msgContent.value,9);
                if ( eventObj.notitype == NOTIFY_EVENT )
                {
                    json_msg_pack_event( &notifyEvent, sendData );
                    os_main_logic_log("sendData[%s]\n",sendData);
                    ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
                    if ( mcloud_get_conn_state( ) )
                    {
                        cloud_pub_topic( TOPIC_EVENTMI, sendData, strlen( sendData ) );
                    }

//                    mlcoap_client_send_msg_multicast(sendData);
                }
                else
                {
#ifdef BLE_DEVICE
                    memset( sendData, 0, 256 );
                    json_msg_pack_event( &notifyEvent, sendData );
                    os_main_logic_log("sendData[%s]\n",sendData);
                    // 报警上报
                    ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
                    if ( mcloud_get_conn_state( ) )
                    {
                        cloud_pub_topic( TOPIC_EVENTMI, sendData, strlen( sendData ) );
                    }
//                    mlcoap_client_send_msg_multicast(sendData);
#else
                    alarm_distribute(devid, &notifyEvent);
#endif
                }
            }
        }
    }

    return TRUE;
}

/********************************************************
 * function: main_scene_report
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_scene_report( char *devid, uint8_t key_num, PKEY_ATTR_T pkey_attr )
{

    char sendData[448] = { 0 };
    char changeState[256] = { 0 };
    REPORT_STATE_CHANGE_T stateInfo = { 0 };
    STATE_OBJ_T state_obj = { 0 };
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pkeyAttrTemp = NULL;
    if ( (devid == NULL) || (pkey_attr == NULL) )
    {
        os_main_logic_log("param is error");
        return FALSE;
    }
    stateInfo.stateObj = &state_obj;
    if ( key_num > 0 )
    {
        int len = 0;
        NETDEVOBJ_T netDev = { 0 };
        EVENTOBJ_T eventObj = { 0 };
        uint8_t count = 0;
        int ret = 0;
        storage_read_local_devobj( &netDev );
        memcpy( stateInfo.devid, netDev.uuid, sizeof(stateInfo.devid) );
        changeState[0] = '[';
        if ( key_num == 1 ) // 如果只有一个状态上报，那么生成端点ID，上报时就携带端点ID
        {
            pkeyAttrTemp = pkey_attr;
            mlink_generate_endpoint_id( DEVICE_OBJ_ID, devid, pkeyAttrTemp->keyState.key,
                                        stateInfo.stateObj->endpointid );
        }
        for ( attrIndex = 0; attrIndex < key_num; attrIndex++ )
        {
            pkeyAttrTemp = pkey_attr + attrIndex;

            //            ret = main_check_statechange(pkeyAttrTemp->keyType, &eventObj);
            ret = TRUE;
            if ( ret == TRUE )
            {
                if ( pkeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR )   // 如果数据大小超过4个字节，使用HEX 的形�?
                {
                    sprintf( changeState, "%s%s|%s|HEX:%s,", changeState, pkeyAttrTemp->keyType,
                             pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
//                        sprintf(changeState, "%s%s|HEX:%s,", changeState, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                    if ( strlen( pkeyAttrTemp->keyState.value ) < 32
                        && (0 != strcmp( pkeyAttrTemp->keyType, KEY_IR_REPORT )) )
                    {
                        storage_update_endpoint_status( devid, (PKEY_ATTR_T) pkeyAttrTemp );
                    }
                }
            }
            count++;

        }
        if ( count == 0 )
        {
            os_main_logic_log("nothing to be sended");
            return TRUE;
        }

        len = strlen( changeState );
        changeState[len - 1] = ']';
        //        os_main_logic_log("state change report is %s", changeState);
        stateInfo.subdevNum = 1;
        memcpy( stateInfo.stateObj->status, changeState, strlen( changeState ) );
        memcpy( stateInfo.stateObj->subdevid, devid, DEVICEID_SIZE );

        json_msg_pack_statuschange( &stateInfo, sendData );

        os_main_logic_log("sendData[%s]\n",sendData);
        mlcoap_client_send_msg_multicast( sendData );
    }
    return TRUE;
}

#else

/********************************************************
 * function: main_event_report
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_event_report(char *devid, uint8_t *key_num, PKEY_ATTR_T_EX pkey_attr)
{
    uint8_t sendData[256] =
    {   0};
    PKEY_ATTR_T_EX pKeyAttrTemp = NULL;
    uint8_t attrIndex = 0;
    os_main_logic_log("value: %s devid %s", pkey_attr->keyState.value,devid);
    if ((devid == NULL) || (pkey_attr == NULL))
    {
        return FALSE;
    }

//    storage_set_mesh_status()
    for ( attrIndex = 0; attrIndex < *key_num; attrIndex ++ )
    {
        OSStatus ret = TRUE;
        EVENTOBJ_T eventObj =
        {   0};

        pKeyAttrTemp = pkey_attr + attrIndex;
        if (0 == strcmp(pKeyAttrTemp->keyType, "10") || (pKeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR))
        {
            continue;
        }
        ret = main_check ret = TURE;
//        os_main_logic_log("key type : %s. notitype: %d", pKeyAttrTemp->keyType, eventObj.notitype);

        if (ret == TRUE)
        {
            EVENT_REPORT_T notifyEvent =
            {   0};
            memset(&notifyEvent, 0, sizeof(EVENT_REPORT_T));
            notifyEvent.eventlevel = eventObj.eventlevel;
            notifyEvent.eventtype = eventObj.eventtype;
            notifyEvent.notitype = eventObj.notitype;
            mlink_generate_endpoint_id(DEVICE_OBJ_ID, devid, pKeyAttrTemp->keyState.key, notifyEvent.msgContent.endpointid);
            strcpy(notifyEvent.msgContent.key, pKeyAttrTemp->keyState.key);
            strcpy(notifyEvent.msgContent.value, pKeyAttrTemp->keyState.value);

            os_main_logic_log("eventObj.notify: %d", eventObj.notitype);
            if ((eventObj.notitype == NOTIFY_EVENT) || (eventObj.notitype == NOTIFY_STATE_CHANGES))
            {
                json_msg_pack_event(&notifyEvent, sendData);
                ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                if (mcloud_get_conn_state())
                {
                    cloud_pub_topic(TOPIC_EVENTMI, sendData, strlen(sendData));
                }
//                mlcoap_client_send_msg_multicast(sendData);
                main_coap_noti_msg_to_netdev(sendData);

            }
            else
            {
#ifdef BLE_DEVICE
                memset(sendData, 0, 256);
                json_msg_pack_event(&notifyEvent, sendData);
                os_main_logic_log("sendData[%s]\n",sendData);
                //
                ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                if (mcloud_get_conn_state())
                {
                    cloud_pub_topic(TOPIC_EVENTMI, sendData, strlen(sendData));
                }
                mlcoap_client_send_msg_multicast(sendData);
#else
                alarm_distribute(devid, &notifyEvent);
#endif
            }
        }
    }

    return TRUE;
}
#endif
/********************************************************
 * function: main_statechange_report
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
#if IS_REPORT_STATUS_TIMER
//static char *g_reportData = NULL;
OSStatus main_statechange_report( char *net_addr, char *devid, uint8_t key_num,
                                  PKEY_ATTR_T_EX pkey_attr )
{
    char sendData[448] = { 0 };
    char changeState[256] = { 0 };
    REPORT_STATE_CHANGE_T stateInfo = { 0 };
    STATE_OBJ_T state_obj = { 0 };
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pkeyAttrTemp = NULL;

    if ( (devid == NULL) || (pkey_attr == NULL) )
    {
        os_main_logic_log("param is error");
        return FALSE;
    }
    if ( (key_num == 1) && (0 == strcmp( pkey_attr->keyType, "10" )) )
    {
        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
        int ret = 0;
        ret = storage_get_dev_status( devid, &pdevStatusObj );
        if ( ret > 0 )
        {
            STATUS_INFO_T statusInfo = { 0 };
            statusInfo.statusInfo = atoi( pkey_attr->keyState.value );
            statusInfo.status_info.devStatus = 1;
            if ( statusInfo.statusInfo != pdevStatusObj->status.statusInfo )
            {
                pdevStatusObj->status.statusInfo = statusInfo.statusInfo;
                stateInfo.subdevNum = 1;
                stateInfo.stateObj = &state_obj;
                memcpy( stateInfo.stateObj->subdevid, devid, DEVICEID_SIZE );
                sprintf( stateInfo.stateObj->status, "[10|10|%d]", statusInfo.statusInfo );
                json_msg_pack_statuschange( &stateInfo, sendData );
                // 1.observe notify 2.multicast notify 3.mqtt notify
                ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
                if ( mcloud_get_conn_state( ) )
                {
                    cloud_pub_topic( TOPIC_NOTIFY, sendData, (unsigned int) strlen( sendData ) );
                }
            }
        }

//        storage_update_dev_status(devid, atoi(pkey_attr->keyState.value));
        return TRUE;
    }
    stateInfo.stateObj = &state_obj;
    if ( key_num > 0 )
    {
        int len = 0;
        NETDEVOBJ_T netDev = { 0 };
        EVENTOBJ_T eventObj = { 0 };
        uint8_t count = 0;
        int ret = 0;
        storage_read_local_devobj( &netDev );
        memcpy( stateInfo.devid, netDev.uuid, sizeof(stateInfo.devid) );
        changeState[0] = '[';
        if ( key_num == 1 ) // 如果只有一个状态上报，那么生成端点ID，上报时就携带端点ID
        {
            pkeyAttrTemp = pkey_attr;
            mlink_generate_endpoint_id( DEVICE_OBJ_ID, devid, pkeyAttrTemp->keyState.key,
                                        stateInfo.stateObj->endpointid );
        }
        for ( attrIndex = 0; attrIndex < key_num; attrIndex++ )
        {
            pkeyAttrTemp = pkey_attr + attrIndex;

//            ret = main_check_statechange(pkeyAttrTemp->keyType, &eventObj);
            ret = TRUE;
            if ( ret == TRUE )
            {
                if ( pkeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR )   // 如果数据大小超过4个字节，使用HEX 的形�?
                {
                    sprintf( changeState, "%s%s|%s|HEX:%s,", changeState, pkeyAttrTemp->keyType,
                             pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
//                    sprintf(changeState, "%s%s|HEX:%s,", changeState, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                    if ( strlen( pkeyAttrTemp->keyState.value ) < 32
                        && (0 != strcmp( pkeyAttrTemp->keyType, KEY_IR_REPORT )) )
                    {
                        storage_update_endpoint_status( devid, (PKEY_ATTR_T) pkeyAttrTemp );
                    }
                }
                else if ( pkeyAttrTemp->keyState.valtype < VALTYPE_HEX_STR )
                {
                    if ( !strcmp( pkeyAttrTemp->keyType, "10" ) )   // Record device status
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status( devid, &pdevStatusObj );
                        pdevStatusObj->status.statusInfo = atoi( pkeyAttrTemp->keyState.value );
                        pdevStatusObj->status.status_info.devStatus = 1;
//                        os_main_logic_log("keyval: %s, pdevStatusObj->status.statusInfo : %d", pkeyAttrTemp->keyState.value, pdevStatusObj->status.statusInfo);

//                        storage_update_dev_status(devid, );
                        sprintf( changeState, "%s%s|%s|%d,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pdevStatusObj->status.statusInfo );
                    }
                    #ifdef BLE_DEVICE
                    else if ( !strcmp( pkeyAttrTemp->keyType, "1004" ) ) //绑定结果上报
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status( devid, &pdevStatusObj );

                        sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
                    }
                    else if ( !strcmp( pkeyAttrTemp->keyType, "1000" ) ) //开锁结果上�?
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status( devid, &pdevStatusObj );

                        sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
                    }
                    else if ( !strcmp( pkeyAttrTemp->keyType, "1001" ) ) //设置pin码结果上�?
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status( devid, &pdevStatusObj );

                        sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
                    }
                    else if ( !strcmp( pkeyAttrTemp->keyType, "1022" ) ) //获取信息失败上报
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status( devid, &pdevStatusObj );

                        sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
                    }
                    #endif
                    else
                    {
                        storage_update_endpoint_status( devid, (PKEY_ATTR_T) pkeyAttrTemp );
                        sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                                 pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
//                        sprintf(changeState, "%s%s|HEX:%s,", changeState, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                    }
                }
                count++;
            }
            else
            {
                os_main_logic_log("Can't find endpoint key!!  key = %s", pkeyAttrTemp->keyType);
            }
        }
        if ( count == 0 )
        {
            os_main_logic_log("nothing to be sended");
            return TRUE;
        }

        len = strlen( changeState );
        changeState[len - 1] = ']';
//        os_main_logic_log("state change report is %s", changeState);
        stateInfo.subdevNum = 1;
        memcpy( stateInfo.stateObj->status, changeState, strlen( changeState ) );
        memcpy( stateInfo.stateObj->subdevid, devid, DEVICEID_SIZE );
        if ( net_addr != NULL )
        {
            strcpy( stateInfo.stateObj->addr, net_addr );
        }

//        json_msg_pack_statechange(&stateInfo, sendData);
        json_msg_pack_statuschange( &stateInfo, sendData );

        os_main_logic_log("sendData[%s]\n",sendData);
        ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
        if ( mcloud_get_conn_state( ) )
        {
            cloud_pub_topic( TOPIC_NOTIFY, sendData, strlen( sendData ) );
            // 加延时操作保证能够发送TCP数据
            msleep( 20 );
        }

    }
    return TRUE;
}

/********************************************************
 * function: main_statechange_report
 * description:  init all about uart
 * input:    1. devid:
 *           2. key:  point to 'uint16_t'
 *           3. addr: addr of device object ( panid.addr )
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_statechange_report_ex( char *net_addr, uint8_t key_num, PKEY_ATTR_T_EX pkey_attr )
{
    uint8_t sendData[488] = { 0 };
    char changeState[256] = { 0 };
    REPORT_STATE_CHANGE_T stateInfo = { 0 };
    STATE_OBJ_T state_obj = { 0 };
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pkeyAttrTemp = NULL;

    if ( net_addr == NULL || pkey_attr == NULL )
    {
        return FALSE;
    }
    if ( (key_num == 1) && (0 == strcmp( pkey_attr->keyType, "10" )) )
    {
        return TRUE;
    }

    stateInfo.stateObj = &state_obj;
    if ( key_num > 0 )
    {
        int len = 0;

        changeState[0] = '[';
        for ( attrIndex = 0; attrIndex < key_num; attrIndex++ )
        {
            pkeyAttrTemp = pkey_attr + attrIndex;
            if ( pkeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR )       // 如果数据大小超过4个字节，使用HEX 的形�?
            {
                sprintf( changeState, "%s%s|%s|HEX:%s,", changeState, pkeyAttrTemp->keyType,
                         pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
            }
            else if ( pkeyAttrTemp->keyState.valtype < VALTYPE_HEX_STR )
            {
                sprintf( changeState, "%s%s|%s|%s,", changeState, pkeyAttrTemp->keyType,
                         pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value );
            }
        }

        len = strlen( changeState );
        changeState[len - 1] = ']';
//        os_main_logic_log("state change report is %s", changeState);
        stateInfo.subdevNum = 1;
        memcpy( stateInfo.stateObj->status, changeState, strlen( changeState ) );
        memcpy( stateInfo.stateObj->addr, net_addr, NET_ADDR_SIZE );

        json_msg_pack_statechange( &stateInfo, sendData );
        ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
    }
    return TRUE;
}

#else
OSStatus main_statechange_report(uint8_t *devid, uint8_t key_num, PKEY_ATTR_T_EX pkey_attr)
{
    uint8_t sendData[256] =
    {   0};
    char changeState[128] =
    {   0};
    REPORT_STATE_CHANGE_T stateInfo =
    {   0};
    STATE_OBJ_T state_obj =
    {   0};
    uint8_t attrIndex = 0;
    PKEY_ATTR_T_EX pkeyAttrTemp = NULL;
    uint8_t meshState = 0;
    if (devid == NULL || pkey_attr == NULL)
    {
        return FALSE;
    }
    mlink_sys_get_status(SYS_MESH_STATE, &meshState);
    if ((key_num == 1) && (0==strcmp(pkey_attr->keyType, "10") && (meshState==MESH_IN)))
    {
        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
        storage_get_dev_status(devid, &pdevStatusObj);
        pdevStatusObj->status.statusInfo = atoi(pkey_attr->keyState.value);
        pdevStatusObj->status.status_info.devStatus = 1;

//        storage_update_dev_status(devid, atoi(pkey_attr->keyState.value));
        return TRUE;
    }
    stateInfo.stateObj = &state_obj;
    if (key_num > 0)
    {
        int len = 0;
        NETDEVOBJ_T netDev =
        {   0};
        EVENTOBJ_T eventObj =
        {   0};
        uint8_t count = 0;
        int ret = 0;
        storage_read_local_devobj(&netDev);
        memcpy(stateInfo.devid, netDev.uuid, sizeof(stateInfo.devid));
        changeState[0] = '[';
        if (key_num == 1) // 如果只有一个状态上报，那么生成端点ID，上报时就携带端点ID
        {
            pkeyAttrTemp = pkey_attr;
            mlink_generate_endpoint_id(DEVICE_OBJ_ID, devid, pkeyAttrTemp->keyState.key, stateInfo.stateObj->endpointid);
        }
        for (attrIndex = 0; attrIndex < key_num; attrIndex ++)
        {
            pkeyAttrTemp = pkey_attr+attrIndex;
            if (meshState == MESH_OUT)
            {
                ret = storage_check_event(pkeyAttrTemp->keyType, &eventObj);
            }

            if (((eventObj.notitype == NOTIFY_STATE_CHANGES) && (ret == TRUE)) || (meshState == MESH_IN))
            {
                if (pkeyAttrTemp->keyState.valtype == VALTYPE_HEX_STR)     // 如果数据大小超过4个字节，使用HEX 的形�?
                {
                    sprintf(changeState, "%s%s|HEX:%s,", changeState, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                    storage_update_endpoint_status(devid, &pkeyAttrTemp->keyState);
                }
                else if (pkeyAttrTemp->keyState.valtype < VALTYPE_HEX_STR)
                {
                    if (!strcmp(pkeyAttrTemp->keyType, "10"))   // Record device status
                    {
                        PDEVSTATUSOBJ_T pdevStatusObj = NULL;
                        storage_get_dev_status(devid, &pdevStatusObj);
                        pdevStatusObj->status.statusInfo = atoi(pkeyAttrTemp->keyState.value);
                        pdevStatusObj->status.status_info.devStatus = 1;
                        os_main_logic_log("keyval: %s, pdevStatusObj->status.statusInfo : %d", pkeyAttrTemp->keyState.value, pdevStatusObj->status.statusInfo);

//                        storage_update_dev_status(devid, );
                        sprintf(changeState, "%s%s|%d,", changeState, pkeyAttrTemp->keyState.key, pdevStatusObj->status.statusInfo);
                    }
                    else
                    {
                        storage_update_endpoint_status(devid, &pkeyAttrTemp->keyState);
                        sprintf(changeState, "%s%s|%s,", changeState, pkeyAttrTemp->keyState.key, pkeyAttrTemp->keyState.value);
                    }
                }
                count++;
            }
        }
        if (count == 0)
        {
            return TRUE;
        }
        len = strlen(changeState);
        changeState[len-1] = ']';
//        os_main_logic_log("state change report is %s", changeState);
        stateInfo.subdevNum = 1;

        memcpy(stateInfo.stateObj->status, changeState, strlen(changeState));
        memcpy(stateInfo.stateObj->subdevid, devid, DEVICEID_SIZE);
//        mlink_generate_endpoint_id(NETDEV_OBJ_ID, stateInfo.stateObj->subdevid, NULL, stateInfo.stateObj->endpointid);

        json_msg_pack_statechange(&stateInfo, sendData);

        // 1.observe notify 2.multicast notify 3.mqtt notify
        ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));

        mlcoap_client_send_msg_multicast(sendData);

        if (mcloud_get_conn_state())
        {
            cloud_pub_topic(TOPIC_NOTIFY, sendData, strlen(sendData));
            // 加延时操作保证能够发送TCP数据
            msleep(20);
        }
    }
    return TRUE;
}
#endif

/********************************************************
 * function: main_state_report
 * description:  init all about uart
 * input:    1. net_addr: zigbee网络地址
 *           2. devid: 子设备ID指针
 *           3. dev_num
 *           4. pdev_attr
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
OSStatus main_devstate_deal( char *net_addr, char *devid, uint8_t dev_num,
                             PKEY_ATTR_T_EX pdev_attr )
{
    uint8_t num = 0;
    uint8_t setmeshState = 0;

    num = dev_num;
    if ( (pdev_attr == NULL) || (devid == NULL) )
    {
        os_main_logic_log("param is error!!!");
        return FALSE;
    }
    os_main_logic_log("memory: %d", MicoGetMemoryInfo()->free_memory);
    mlink_sys_get_status( SYS_MESH_STATE, &setmeshState );

    // flag and active device state
    main_online_deal( devid );
    if ( setmeshState != MESH_IN )
    {
#ifndef RF433_DEVICE
        // endpoint link
        main_endpointlink_deal( devid, &num, pdev_attr );
#endif
        // statechange report
        main_statechange_report( net_addr, devid, num, pdev_attr );

        main_linkage_deal( devid, num, pdev_attr );

    }

    // event report
    main_event_report( devid, &num, pdev_attr );

    return TRUE;
}

#ifdef BLE_DEVICE
/*************************************************
 Function:         get_bcd_to_dec
 Description:     获取bcd码转10进制
 Input:            1. data

 Output:         TCP_SUBDEV_RSP_HEAD_T
 Return:
 Others:
 *************************************************/
static int get_bcd_to_dec(char *data)
{
    int ret = 0;
    ret = _BCD_TO_DEC( (unsigned char ) (data[1]) )
                                         * 100+ _BCD_TO_DEC((unsigned char) (data[2]));
    return ret;
}
/*************************************************
 Function:         get_del_record
 Description:     获取删除记录
 Input:            1. data
                   2.locktype
 Output:         BLEUSER_LISTRECORDLIST
 Return:
 Others:
 *************************************************/
static BLEUSER_LISTRECORDLIST get_del_record(uint8_t locktype,char *data)
{
    BLEUSER_LISTRECORDLIST Ulocklist;
    memset( &Ulocklist, 0, sizeof(BLEUSER_LISTRECORDLIST) );
    if(locktype == BLE_RECORD_TYPE_TMPAUTH)
    {
        Ulocklist.usrid = get_bcd_to_dec(data);
        Ulocklist.keytype = ELINK_EVENT_INFO_PASS;
        Ulocklist.mode = 3;
        elink_set_dev_restmpFlag( 3 );

        Ulocklist.usrid = Ulocklist.usrid*100 + 4;
        return Ulocklist;
    }
    else
    {
       Ulocklist.usrid =  get_bcd_to_dec(data);
       if ( locktype == 1 )   //密码
       {
           Ulocklist.keytype = ELINK_EVENT_INFO_PASS;
       }
       else if ( locktype == 2 ) // 刷卡
       {
           Ulocklist.keytype = ELINK_EVENT_INFO_CARD;
       }
       else if ( locktype == 3 )  // 指纹
       {
           Ulocklist.keytype = ELINK_EVENT_INFO_FINGER;
       }
       Ulocklist.mode = 3;
       Ulocklist.usrid = Ulocklist.usrid*100 + Ulocklist.keytype;
       return Ulocklist;
    }
    return Ulocklist;
}
/*************************************************
 Function:         get_add_record
 Description:     获取添加记录
 Input:            1. data
                   2.locktype
 Output:         BLEUSER_LISTRECORDLIST
 Return:
 Others:
 *************************************************/
static BLEUSER_LISTRECORDLIST get_add_record(uint8_t locktype,char *data)
{
     BLEUSER_LISTRECORDLIST Ulocklist;
     memset( &Ulocklist, 0, sizeof(BLEUSER_LISTRECORDLIST) );
     Ulocklist.usrid = get_bcd_to_dec(data);
     if(locktype == 1)   //密码
     {
         Ulocklist.keytype = ELINK_EVENT_INFO_PASS;
     }
     else if(locktype == 2) // 刷卡
     {
         Ulocklist.keytype = ELINK_EVENT_INFO_CARD;
     }
     else if(locktype == 3)  // 指纹
     {
         Ulocklist.keytype = ELINK_EVENT_INFO_FINGER;
     }

     Ulocklist.mode = 1;
     Ulocklist.usrid = Ulocklist.usrid*100 + Ulocklist.keytype;
     return Ulocklist;
}

/*************************************************
 Function:         get_tcp_lockinfo_time
 Description:     获取lockinfo_time
 Input:            1. cmd 开锁类�?
                   2 data1
                   3 data2
 Output:         TCP_SUBDEV_RSP_HEAD_T
 Return:
 Others:
 *************************************************/
static TCP_SUBDEV_RSP_HEAD_T get_tcp_lockinfo_time(ELINK_EVENT_INFO_E cmd,char *data)
{
    TCP_SUBDEV_RSP_HEAD_T tcp_subdev_rsp_head_time;
    mytime_struct openlock_time;
    memset(&tcp_subdev_rsp_head_time,0,sizeof(TCP_SUBDEV_RSP_HEAD_T));
    memset(&openlock_time,0,sizeof(mytime_struct));
    tcp_subdev_rsp_head_time.userid = _BCD_TO_DEC( (unsigned char ) (data[1]) )
                                     * 100+ _BCD_TO_DEC((unsigned char) (data[2]));
    openlock_time.nYear = ((data[4] >> 4) * 10) + (data[4] & 0x0f) + 2000;
                   openlock_time.nMonth = ((data[5] >> 4) * 10) + (data[5] & 0x0f);
                   openlock_time.nDay = ((data[6] >> 4) * 10) + (data[6] & 0x0f);
                   openlock_time.nHour = ((data[7] >> 4) * 10) + (data[7] & 0x0f) - 8;
                   openlock_time.nMin = ((data[8] >> 4) * 10) + (data[8] & 0x0f);
                   openlock_time.nSec = ((data[9] >> 4) * 10) + (data[9] & 0x0f);
    tcp_subdev_rsp_head_time.time = mytime_2_utc_sec( &openlock_time, false );
    switch(cmd)
    {
        case ELINK_EVENT_INFO_LOCAL:
        case ELINK_EVENT_INFO_FINGER:
        case ELINK_EVENT_INFO_PASS:
        case ELINK_EVENT_INFO_CARD:
        case ELINK_EVENT_INFO_APP:
        {
            tcp_subdev_rsp_head_time.keytype = cmd;
            tcp_subdev_rsp_head_time.userid = tcp_subdev_rsp_head_time.userid*100 + tcp_subdev_rsp_head_time.keytype;
        }
        break;
        case ELINK_EVENT_INFO_TMPAUTHPASS:
        {
            tcp_subdev_rsp_head_time.keytype = 4;
                        tcp_subdev_rsp_head_time.userid = tcp_subdev_rsp_head_time.userid*100 + tcp_subdev_rsp_head_time.keytype;
        }
        default:
            break;
    }
    return tcp_subdev_rsp_head_time;

}
/*************************************************
 Function:         main_ble_callback_proc
 Description:      蓝牙模块操作回调处理函数
 Input:            1. state
 2. data
 3. size
 4. deviceId
 Output:           �?
 Return:
 Others:
 *************************************************/
int main_ble_callback_proc( BLE_NOTIFY_E state, char * data, int size )
{
    uint16_t key_id;
    TCP_SUBDEV_RSP_HEAD_T elink_tcp_lockinfo_time;
    stBLELockStatus lockstate;
    elink_set_dev_busystate( 1 );
    os_main_logic_log("state : %d",state);
    switch ( state )
    {
        case BLE_NOTE_INIT_SUCESS:
            {
                if(storage_get_mac_sucess() != 0)
                {
                    ble_bind_lock_start();
                    ble_start_mesh_timer(60);
                }
                break;
            }
        case BLE_NOTI_BIND_LOCK:
        case BLE_NOTI_GET_LOCKINFO:
            {
                if(storage_get_mac_sucess() != 0)
                {
                    mlink_gpio_beep_start(3);
                }
            break;
        }

        case BLE_NOTI_SET_USERPIN:
            {
            uint8_t open_state;
            open_state = data[0];
            os_main_logic_log("open_state : %d",open_state);
            if ( _GET_BIT_(open_state, 7) == 0 )      //操作成功
            {
                storage_set_present_lockpin( );
                memset( &lockstate, 0, sizeof(stBLELockStatus) );
                lockstate.isOperSuccess = 1;
                stor_lockstatue_write( OBJECT_UPDATE_ADD, 1, &lockstate );
                mlink_gpio_beep_start(1);
                ble_setmesh_config_exit();
            }
            else
            {
                mlink_gpio_beep_start(3);
            }
            break;
        }
        case BLE_NOTI_RSSI_UPLINK:
            {
            os_main_logic_log("data[0] : %d g_signalFlag : %d",data[0],g_signalFlag);
            if ( g_signalFlag != data[0] && elink_get_join_net_status() == 1)
            {
                elink_set_sub_status( data[0] );
                elink_sub_signalreport( );
            }
            else
            {
                elink_set_dev_busystate( 0 );
            }
            g_signalFlag = data[0];

            break;
        }
        case BLE_NOTI_ADD_TMP_SEC_UNLOCK:
            {
            if(_GET_BIT_(data[0], 7) == 0)
            {
                BLEUSER_LISTRECORDLIST Ulocklist;
                Ulocklist.keytype = ELINK_EVENT_INFO_PASS;
                Ulocklist.usrid = get_bcd_to_dec(data);

                if ( Ulocklist.usrid > 10 )   // 失败
                {
                    elink_set_dev_errstrcode( ELINK_DEV_UNKOWN_ERR );
                    elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );
                }
                else
                {

                    msleep( 20 );
                    Ulocklist.usrid = Ulocklist.usrid*100 + 4; // 这个跟本地密码区分开
                    elink_set_dev_restmpFlag( 1 );   // 新增

                    elink_set_sub_reslist( &Ulocklist, 1 );
                    elink_sub_restmpchangereport( );
                    msleep( 20 );
                    stor_templocklist_write( OBJECT_UPDATE_ADD, 1, &Ulocklist );
                }


            }

            break;
        }
        case BLE_NOTI_DEL_TMP_SEC_UNLOCK:
            {
            if(_GET_BIT_(data[0], 7) == 0)
            {
                BLEUSER_LISTRECORDLIST Ulocklist;
               Ulocklist.keytype = ELINK_EVENT_INFO_PASS;
               Ulocklist.usrid = _BCD_TO_DEC( (unsigned char ) (data[1]) )
                   * 10+ _BCD_TO_DEC((unsigned char) (data[2]));
               if ( Ulocklist.usrid > 10 )   // 失败
               {
                   elink_set_dev_errstrcode( ELINK_DEV_UNKOWN_ERR );
                   elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );
               }
               else
               {

                   msleep( 20 );
                   Ulocklist.usrid = Ulocklist.usrid*100 + 4;  //这个跟本地密码区分开
                   elink_set_dev_restmpFlag( 3 );   // 删除
                   elink_set_sub_reslist( &Ulocklist, 1 );
                   elink_sub_restmpchangereport( );
                   msleep( 20 );
                   storage_del_blelsit_obj( &Ulocklist.usrid );
               }

            }


            break;
        }
        case BLE_NOTI_REMO_PASS_UNLOCK:
            case BLE_NOTI_STATE:
            case BLE_NOTI_SET_TIME:
            case BLE_NOTI_ERROR:
            {
            uint8_t lockstate;
            lockstate = data[0];      // 门状�?
            os_main_logic_log("lockstate : %d g_alarmFlag : %d g_demolishFlag : %d",lockstate,g_alarmFlag,g_demolishFlag);
            if ( _GET_BIT_(lockstate, 7) == 0 )      //操作成功
            {
                if ( state != BLE_NOTI_STATE )
                {
//                    elink_set_dev_errstrcode( ELINK_RETURN_OK );
//                    elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );
                }
                msleep( 20 );
                if ( _GET_BIT_(lockstate, 0) == 0 && _GET_BIT_(lockstate, 3) == 0 )   // 开锁成�?
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_OPEN_LOCK );
                    elink_sub_statusreport( );
                }
                if ( _GET_BIT_(lockstate, 0) == 1 && _GET_BIT_(lockstate, 3) == 0 )     // 反锁
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_ISLOCKED );
                    elink_sub_statusreport( );
                }
                if ( _GET_BIT_(lockstate, 3) == 1 && _GET_BIT_(lockstate, 0) == 0 )   // 锁未上锁
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_NO_CLOSE_ABNORMAL );
                    elink_sub_statusreport( );
                }
                if ( _GET_BIT_(lockstate, 1) == 0 )    // 低压
                {
                    if ( g_alarmFlag == 1 )
                    {
                        elink_set_sub_alarmstatus( ELINK_ALARM_BATTERY_WARN_STATE );  //电量告警
                        elink_set_sub_batterystatus( 0 );
                        elink_sub_alarmreport( );
                    }
                    g_alarmFlag = 0;

                }
                if ( _GET_BIT_(lockstate, 2) == 0 )    // 防拆故障
                {
                    if ( g_demolishFlag == 1 )
                    {
                        elink_set_sub_status( ELINK_LOCK_STATE_NO_OPEN_LOCK );
                        elink_sub_demolishreport( );
                    }
                    g_demolishFlag = 0;
                }
                if ( _GET_BIT_( lockstate, 4 ) )  // 门未关超时故�?
                {

                }

            }
            else
            {
                if ( state != BLE_NOTI_STATE )
                {
//                    elink_set_dev_errstrcode( ELINK_DEV_UNKOWN_ERR );
//                    elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );
                }

            }
            break;
        }
        case BLE_NOTI_NEW_RECORD:
            {
            uint8_t open_locktype;
            uint8_t record_type;
            open_locktype = *(data + 3);       // 开锁类�?
            record_type = data[0];           // 记录类型

            BLEUSER_LISTRECORDLIST Ulocklist;

            os_main_logic_log("record_type: %d open_locktype : %d",record_type,open_locktype);
            PrintfByteHex( "BLE_NOTI_NEW_RECORD ", data, size );
            if ( record_type == BLE_RECORD_TYPE_STATUS )  //锁状态记�?
            {
                if ( _GET_BIT_(open_locktype, 0) == 1 )   // 代表反锁记录
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_ISLOCKED );
                    elink_sub_statusreport( );

                }
                if ( _GET_BIT_(open_locktype, 1) == 1 )    // 低压报警
                {
                    elink_set_sub_alarmstatus( ELINK_ALARM_BATTERY_WARN_STATE );  //电量告警
                    elink_set_sub_batterystatus( 1 );
                    elink_sub_alarmreport( );
                    g_alarmFlag = 1;
                }
                if ( _GET_BIT_(open_locktype, 2) == 1 )    // 防拆报警
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_OPEN_LOCK );
                    elink_sub_demolishreport( );
                    g_demolishFlag = 1;
                }
                if(_GET_BIT_(open_locktype, 6) == 1)   // 连续操作失败报警
                {
                    elink_set_sub_alarmstatus( ELINK_ALARM_PASS_ATK_STATE );  //密码攻击
                    elink_sub_alarmreport( );


                }
            }
            else if ( record_type == BLE_RECORD_TYPE_OPER )  // 操作记录
            {
                memset( &Ulocklist, 0, sizeof(BLEUSER_LISTRECORDLIST) );

                key_id = get_bcd_to_dec(data);
                Ulocklist.usrid = key_id;
                Ulocklist.keytype = open_locktype;
                Ulocklist.mode = 1;
                Ulocklist.usrid = Ulocklist.usrid*100 + Ulocklist.keytype;
                elink_set_sub_reslist( &Ulocklist, 1 );
                elink_sub_reschangereport( );
            }
            else if ( record_type == BLE_RECORD_TYPE_ADD )
            {
                Ulocklist = get_add_record(open_locktype,data);
                elink_set_sub_reslist( &Ulocklist, 1 );
                elink_sub_reschangereport( );
            }
            else if ( record_type == BLE_RECORD_TYPE_DEL )
            {
                memset( &Ulocklist, 0, sizeof(BLEUSER_LISTRECORDLIST) );
                if(open_locktype == BLE_RECORD_TYPE_TMPAUTH)
                {
                    Ulocklist = get_del_record(open_locktype,data);
                    elink_set_sub_reslist( &Ulocklist, 1 );
                    elink_sub_restmpchangereport( );
                    msleep( 20 );

                    storage_del_blelsit_obj( &Ulocklist.usrid );
                }
                else
                {
                    Ulocklist = get_del_record(open_locktype,data);
                    elink_set_sub_reslist( &Ulocklist, 1 );
                    elink_sub_reschangereport( );
                }

            }
            else
            {

                switch ( open_locktype )
                {
                    case ELINK_EVENT_INFO_LOCAL:
                        {

                        elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_LOCAL,data);
                        elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                        elink_sub_eventreport( );

                        break;
                    }
                    case ELINK_EVENT_INFO_FINGER:   // 代表密码
                    {

                       elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_PASS,data);
                       elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                       elink_sub_eventreport( );
                       break;
                    }
                    case ELINK_EVENT_INFO_TMPAUTHPASS:
                        {
                        elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_TMPAUTHPASS,data);
                        elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                        elink_sub_eventreport( );
                        break;
                    }
                    case ELINK_EVENT_INFO_PASS:  // 代表刷卡

                        {
                        elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_CARD,data);
                        elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                        elink_sub_eventreport( );
                        break;
                    }
                    case ELINK_EVENT_INFO_CARD:   // 代表指纹
                        {
                        elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_FINGER,data);
                        elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                        elink_sub_eventreport( );
                        break;
                    }
                    case ELINK_EVENT_INFO_APP:
                        {
                        elink_tcp_lockinfo_time = get_tcp_lockinfo_time(ELINK_EVENT_INFO_APP,data);
                        elink_set_sub_eventinfo( &elink_tcp_lockinfo_time );
                        elink_sub_eventreport( );
                        break;
                    }
                    default:
                        break;
                }
            }

        }
            break;
        case BLE_NOTI_GET_LOCK_LIST:
        case BLE_NOTI_FORCE_GET_LOCK_LIST:
            {
            uint8_t i;
            uint8_t ret = 0;
            uint8_t *usr_id = (uint8_t *) malloc( size );
            uint8_t lockstate;
//           PrintfByteHex("BLE_NOTI_GET_LOCK_LIST ", data, size);
            size = size - 1;
            memcpy( usr_id, &data[1], size );
            size = size / 3;
            BLEUSER_LISTRECORDLIST Ulocklist[LOCK_LSIT_COUNT];
            BLEUSER_LISTRECORDLIST UlockTemplist[LOCK_LIST_TEMP_COUNT];
            lockstate = data[0];      // 门状�?
            if ( _GET_BIT_(lockstate, 7) == 0 )      //操作成功
            {
                msleep( 20 );
                if ( _GET_BIT_(lockstate, 0) == 0 && _GET_BIT_(lockstate, 3) == 0 )   // 开锁成�?
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_OPEN_LOCK );
//                    elink_sub_statusreport( );
                }
                else if ( _GET_BIT_(lockstate, 0) == 1 && _GET_BIT_(lockstate, 3) == 0 )     // 反锁
                {
                    elink_set_sub_status( ELINK_LOCK_STATE_ISLOCKED );
//                    elink_sub_statusreport( );
                }

                msleep( 500 );
                os_main_logic_log("size : %d",size);
                elink_set_dev_busystate( 1 );
                memset( Ulocklist, 0, sizeof(BLEUSER_LISTRECORDLIST) * LOCK_LSIT_COUNT );
                for ( i = 0; i < size; i++ )
                {
                    Ulocklist[i].usrid = _BCD_TO_DEC(
                        (unsigned char ) (usr_id[i * 3]) )
                                         * 100
                                         + _BCD_TO_DEC(
                                                        (unsigned char ) (usr_id[i * 3 + 1]) );
                    Ulocklist[i].keytype = usr_id[i * 3 + 2];
                    //                    os_main_logic_log("Ulocklist[i].usrid[%d]Ulocklist[i].keytype[%d]\n",Ulocklist[i].usrid, Ulocklist[i].keytype);
                    Ulocklist[i].usrid  = Ulocklist[i].usrid *100 + Ulocklist[i].keytype;
                }
                elink_set_sub_reslist( &Ulocklist, size );
                elink_sub_resreport( );

            }

            msleep( 500 );
            ret = stor_locktemplist_read(&UlockTemplist);
            if(ret > 0 && ret != 0xff)
            {
                elink_set_dev_restmpFlag( 2 );
                elink_set_sub_reslist( &UlockTemplist, ret );
                elink_sub_restmpreport( );
            }
            else
            {
                elink_set_dev_restmpFlag( 2 );
                elink_set_sub_reslist( NULL, 0);
                elink_sub_restmpreport( );
            }

            free( usr_id );
            usr_id = NULL;
            break;
        }
        default:
            break;
    }
}
#endif

/********************************************************
 * function: mlcoap_server_discover_notify
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_mutex_init( void )
{
    mlcoap_server_sys_mutex_init( );
    mlcoap_server_ctrl_mutex_init( );
}

/********************************************************
 * function: main_add_linkage_status
 * description:   添加联动状�?
 * input:
 * output:
 * return:
 * auther:
 * other:
 *********************************************************/
void main_add_linkage_status( uint8_t lnk_index, char *lnk_id, uint32_t cflag )
{
    mlink_add_linkage_status( lnk_index, lnk_id, cflag );
}

/********************************************************
 * function: main_del_linkage_status_info
 * description:
 * input:           1. lnk_id:  为NULL时清除所有联动信�?
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   删除联动时同时删除与联动相关的信�?
 *********************************************************/
void main_del_linkage_status_info( char *lnk_id )
{
    mlink_del_linkage_status_info( lnk_id );
}

/********************************************************
 * function: main_set_linkage_input_cflag
 * description:   更新联动任务cflag�?
 * input:
 * output:
 * return:
 * auther:
 * other:
 *********************************************************/
void main_set_linkage_input_cflag( uint8_t lnk_index, uint32_t cflag )
{
    mlink_set_linkage_input_cflag( lnk_index, cflag );
}

/********************************************************
 * function: main_linkage_output_deal
 * description:   联动输出操作
 * input:
 * output:
 * return:
 * auther:
 * other:
 *********************************************************/
void main_linkage_output_deal( char *lnk_id, uint8_t output_start_index )
{
    int linkageOutputIndex = 0;
    LINKAGE_OUTPUT_TASK_T lnkOutputTask = { 0 };
    uint8_t outputCtrlIndex = output_start_index;
    char *outputParam[4] = { NULL, NULL, NULL, NULL };        // 用于指向1.endpointid   2.控制点属性ID 3.控制�?
    uint8_t paramNum = 0;
    ENDPOINT_ELE_T endpointInfo = { 0 };
    DEV_FUNCTION_OBJ_T devFunc = { 0 };
    os_main_logic_log("*** deal linkage output ***");
    linkageOutputIndex = storage_get_obj_by_id( LINKAGE_OUTPUT_OBJ_ID, lnk_id, &lnkOutputTask );
    for ( ; outputCtrlIndex < lnkOutputTask.outputNum; outputCtrlIndex++ )
    {
        mlink_parse_space_data( lnkOutputTask.output[outputCtrlIndex], '|', outputParam,
                                &paramNum );
        mlink_parse_endpointid( outputParam[0], &endpointInfo );

        if ( (endpointInfo.classId == DEVICE_OBJ_ID) || (endpointInfo.classId == SCENE_OBJ_ID) )
        {
//            os_main_logic_log("exec scene!!!");
            strcpy( devFunc.key, endpointInfo.key );
            strcpy( devFunc.value, outputParam[2] );
            devFunc.type = atoi( outputParam[1] );
            devFunc.valtype = VALTYPE_HEX_STR;
            os_main_logic_log("type: %d, val: %s", devFunc.type, devFunc.value);
            mlcoap_server_ctrl_write_subdev( endpointInfo.classId, endpointInfo.devid, 1, &devFunc,
                                             0 );
        }
        else if ( endpointInfo.classId == DELAY_OBJ_ID )
        {
            mlink_set_linkage_output_delay( lnk_id, outputCtrlIndex + 1, atoi( outputParam[2] ) );
            break;
        }

    }

    if ( outputCtrlIndex >= lnkOutputTask.outputNum )
    {
        mlink_clear_linkage_status_info( lnk_id );
    }
}

/********************************************************
 * function: main_linkage_cron_deal
 * description:   定时触发联动
 * input:
 * output:
 * return:
 * auther:
 * other:
 *********************************************************/
int main_linkage_cron_deal( char *id, void *task_info, unsigned int info_len )
{
    if ( task_info )
    {
        CRON_TASK_INFO taskInfo = { 0 };
        memcpy( &taskInfo, task_info, info_len );

        mlink_update_linkage_status( taskInfo.linkId, taskInfo.index, 0 );
    }
    else
    {
        os_main_logic_log("task info is NULL");
    }
    return kNoErr;
}

#if 0
/********************************************************
 * function: main_sys_regularly_check_handle
 * description:   定时检测处理设备一些状�?
 * input:
 * output:
 * return:
 * auther:
 * other:
 *********************************************************/
void main_sys_regularly_check_handle()
{
    static uint8_t timeCount = 0;
    timeCount = (timeCount++)%60;
    if (timeCount == 0)
    {
        main_monitor_dev_online_status( NULL );
    }
//    mlink_monitor_perform_linkage();
}

/********************************************************
 * function: main_monitor_status_timer_init
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_status_timer_init()
{
//    mico_init_timer(&g_timer_handle, 60*1000, main_monitor_dev_online_status, NULL);
    mico_init_timer(&g_timer_handle, 1000, main_sys_regularly_check_handle, NULL);
    mico_start_timer(&g_timer_handle);
#if IS_REPORT_STATUS_TIMER
    OSStatus err = kNoErr;
    err=mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "main_statechage_thread",
        (mico_thread_function_t)main_dev_status_change_timer_deal,
        0x1500, 0 );        //0x1500
    require_noerr_string( err, exit, "ERROR: Unable to start the main_statechage_thread thread." );

    exit:
    return;
//    mico_init_timer(&g_timer_handle_statechage, 1000, main_dev_status_change_timer_deal, NULL);
//    mico_start_timer(&g_timer_handle_statechage);
#endif
}
#else

/********************************************************
 * function: main_backups_sys_partition
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_backups_sys_partition( void )
{
    mico_Context_t* mainContext = mico_system_context_get( );
    app_context_t app_context;
    uint32_t storageAddress = FLASH_USER_ALARM_OFFSET;
    uint8_t *data = NULL;
    uint32_t dataLen = sizeof(application_config_t);

    md5_context md5;
    uint8_t hash[MD5_DIGEST_SIZE];

    InitMd5( &md5 );

    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    Md5Update( &md5, (uint8_t*) app_context.appConfig, dataLen );
    Md5Final( &md5, hash );

    // get data from flash with backup block
    data = malloc( dataLen + 20 );
    MicoFlashRead( MICO_PARTITION_USER, &storageAddress, data, dataLen + 20 );
    memset( data, 0, 4 );

    if ( memcmp( data + 4 + dataLen, hash, MD5_DIGEST_SIZE ) )
    {
        memcpy( data + 4, (uint8_t*) app_context.appConfig, dataLen );
        memcpy( data + 4 + dataLen, hash, MD5_DIGEST_SIZE );
        storageAddress = FLASH_USER_ALARM_OFFSET;
        MicoFlashWriteEx( MICO_PARTITION_USER, &storageAddress, data, dataLen + 20 );
        os_main_logic_log("start to backup, len: %d", dataLen);
    }

    if ( data )
    {
        free( data );
    }
}

/********************************************************
 * function: main_restore_sys_partition
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_restore_sys_partition( void )
{
    mico_Context_t* mainContext = mico_system_context_get( );
    app_context_t app_context;
    uint32_t storageAddress = FLASH_USER_ALARM_OFFSET;
    uint8_t *data = NULL;
    uint32_t dataLen = sizeof(application_config_t);
    uint32_t flag = 0;

    md5_context md5;
    uint8_t hash[MD5_DIGEST_SIZE];

    InitMd5( &md5 );

    app_context.appConfig = mico_system_context_get_user_data( mainContext );
    Md5Update( &md5, (uint8_t*) app_context.appConfig, dataLen );
    Md5Final( &md5, hash );

    // get data from flash with backup block
    data = malloc( dataLen + 20 );
    MicoFlashRead( MICO_PARTITION_USER, &storageAddress, data, dataLen + 20 );
    memcpy( &flag, data, 4 );

    if ( (memcmp( data + 4 + dataLen, hash, MD5_DIGEST_SIZE )) && (flag == 0) )
    {
        memcpy( (uint8_t*) app_context.appConfig, data + 4, dataLen );
        mico_system_context_update( mainContext );
    }

    if ( data )
    {
        free( data );
    }
}

/********************************************************
 * function: main_monitor_sys_partition
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_sys_partition( void )
{
    uint8_t state = 0;
    mlink_sys_get_status( SYS_UPDTAE_STATE, &state );
    if ( state )
    {
        main_backups_sys_partition( );
        mlink_sys_set_status( SYS_UPDTAE_STATE, 0 );
    }
}

/********************************************************
 * function: main_monitor_status_timer_init
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_dev_online_status( void *arg )
{
    PDEVSTATUSOBJ_T pdevStatus = NULL;
    PDEVSTATUSOBJ_T pdevStatusTemp = NULL;
    uint8_t count = 0;
    uint8_t devNum = storage_get_device_num( );
    storage_get_dev_status_starting( 0, &pdevStatus );

    for ( count = 1; count <= devNum; count++ )
    {
        pdevStatusTemp = pdevStatus + count;
        if ( pdevStatusTemp->restTime != 0 )
        {
            pdevStatusTemp->restTime--;
//            os_main_logic_log("device restTime is %d", pdevStatusTemp->restTime);
            if ( pdevStatusTemp->restTime == 0 )
            {
                main_offline_deal( pdevStatusTemp->devid );
            }
        }
    }
}

/********************************************************
 * function: main_monitor_report_statechange
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_report_statechange( void )
{
    uint8_t sendData[512] = { 0 };
    uint8_t bFind = 0;
    char changeState[384] = { 0 };
    REPORT_STATE_CHANGE_T stateInfo = { 0 };
    STATE_OBJ_T state_obj = { 0 };
    NETDEVOBJ_T netDev = { 0 };
    static uint8_t stCurrDevIndex_timer = 0;
    memset( sendData, '0', 384 );

    uint8_t devIndex = 0;

    uint8_t endpointIndex = 0;
    PDEV_ENDPOINT_STATUS_T pdevEndpointTemp = NULL;
    PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;
    DEV_ENDPOINT_STATUS_T devEndpointTemp = { 0 };
    uint32_t endpointNum = storage_get_endpointNum( );

    stateInfo.stateObj = &state_obj;
    storage_read_local_devobj( &netDev );
    memcpy( stateInfo.devid, netDev.uuid, sizeof(stateInfo.devid) );

    if ( stCurrDevIndex_timer >= endpointNum )
    {
        stCurrDevIndex_timer = 0;
    }
    devIndex = stCurrDevIndex_timer;

    // report endpoints status which is changed in every device.
    for ( devIndex = 0; devIndex < endpointNum; devIndex++ )
    {
        pdevEndpointTemp = storage_get_dev_endpoints_status( devIndex ); //&g_deviceAttrStatus[devIndex];

        memcpy( &devEndpointTemp, pdevEndpointTemp, sizeof(DEV_ENDPOINT_STATUS_T) );
        pdevEndpointTemp = &devEndpointTemp;
        if ( pdevEndpointTemp == NULL || pdevEndpointTemp->fid[0] == 0 )
            continue;

        // generate status string with format like "[keytype|key|val, keytype0|key0|val0, ... ]"
        memset( changeState, 0, sizeof(changeState) );
        changeState[0] = '[';
        for ( endpointIndex = 0; endpointIndex < pdevEndpointTemp->endpointNum; endpointIndex++ )
        {
            pEndpointStatusTemp = pdevEndpointTemp->endpointStatus + endpointIndex;
            if ( pEndpointStatusTemp == NULL || pEndpointStatusTemp->endpointId[0] == 0 )
            {
                continue;
            }

            if ( pEndpointStatusTemp->oldvalue[0] == 0 )
            {
                memcpy( pEndpointStatusTemp->oldvalue, pEndpointStatusTemp->keyAttr.keyState.value,
                        VALUE_SIZE_EX );
                continue;
            }
//            os_main_logic_log("state change report [%d %d] %s [%x %s] [%x %s ]",
//                              devIndex,endpointIndex,pEndpointStatusTemp->endpointId,
//                              pEndpointStatusTemp->oldvalue[0],
//                              pEndpointStatusTemp->oldvalue,
//                              pEndpointStatusTemp->keyAttr.value[0],
//                              pEndpointStatusTemp->keyAttr.value);
//            if (0==strcmp("602", pEndpointStatusTemp->keyAttr.keyType))
//            {
//                os_main_logic_log("key:%s, oldVal: %s, val:%s", pEndpointStatusTemp->keyAttr.keyState.key, pEndpointStatusTemp->oldvalue, pEndpointStatusTemp->keyAttr.keyState.value);
//            }
            if ( strlen( pEndpointStatusTemp->keyAttr.keyState.value )
                >= sizeof(pEndpointStatusTemp->oldvalue) )
            {
                pEndpointStatusTemp->keyAttr.keyState.value[sizeof(pEndpointStatusTemp->oldvalue)
                    - 1] = 0;
            }
//             if   (memcmp(pEndpointStatusTemp->oldvalue,pEndpointStatusTemp->keyAttr.value,
//                        sizeof(pEndpointStatusTemp->oldvalue))!=0  )
            if ( strcmp( pEndpointStatusTemp->oldvalue,
                         pEndpointStatusTemp->keyAttr.keyState.value )
                 != 0 )
            {
                os_main_logic_log("old: %s, val: %s", pEndpointStatusTemp->oldvalue, pEndpointStatusTemp->keyAttr.keyState.value);
                memcpy( pEndpointStatusTemp->oldvalue, pEndpointStatusTemp->keyAttr.keyState.value,
                VALUE_SIZE_EX );

                if ( pEndpointStatusTemp->keyAttr.keyState.valtype == VALTYPE_HEX_STR ) // 如果数据大小超过4个字节，使用HEX 的形�?
                {
                    sprintf( changeState, "%s%s|%s|HEX:%s,", changeState,
                             pEndpointStatusTemp->keyAttr.keyType,
                             pEndpointStatusTemp->keyAttr.keyState.key,
                             pEndpointStatusTemp->keyAttr.keyState.value );
                }
                else if ( pEndpointStatusTemp->keyAttr.keyState.valtype < VALTYPE_HEX_STR )
                {
                    sprintf( changeState, "%s%s|%s|%s,", changeState,
                             pEndpointStatusTemp->keyAttr.keyType,
                             pEndpointStatusTemp->keyAttr.keyState.key,
                             pEndpointStatusTemp->keyAttr.keyState.value );
                }

                bFind = 1;
            }
        }
        if ( bFind )
        {
            int len = strlen( changeState );
            changeState[len - 1] = ']';
            stateInfo.subdevNum = 1;
            strcpy( stateInfo.stateObj->status, changeState );
            memcpy( stateInfo.stateObj->subdevid, pdevEndpointTemp->fid, DEVICEID_SIZE );
            memset( sendData, 0, sizeof(sendData) );
            json_msg_pack_statechange( &stateInfo, sendData );

            // 1.observe notify 2.multicast notify 3.mqtt notify
            ml_coap_notify_obs( ML_COAP_URITYPE_MSG, sendData, strlen( sendData ) );
//            msleep(20);
//            mlcoap_client_send_msg_multicast(sendData);

            if ( mcloud_get_conn_state( ) )
            {
                cloud_pub_topic( TOPIC_NOTIFY, sendData, (unsigned int) strlen( sendData ) );
            }
//            os_main_logic_log("state change report sendData[%d] %s",strlen(sendData), sendData);

            break;
        }

    }

}

/********************************************************
 * function: main_monitor_perform_linkage
 * description:   监测联动状态并执行相应的联动操�?
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_perform_linkage( void )
{
    mlink_monitor_perform_linkage( );
}

/********************************************************
 * function: main_announce_online_timer
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_announce_online_timer( )
{
    static char jsonStr[128] = { 0 };
    if ( jsonStr[0] == 0 )
    {
        NETDEVOBJ_T netDevObj = { 0 };
        storage_read_local_devobj( &netDevObj );
        if ( (netDevObj.uuid[0] <= '~') && (netDevObj.uuid[0] >= ' ')
             && (strlen( netDevObj.uuid ) < 12) )
        {
            ALLOW_NOTIFY_INFO_T info = { 0 };
            memcpy( info.uuid, netDevObj.uuid, UUID_SIZE - 1 );
            memcpy( info.panid, netDevObj.addr, sizeof(netDevObj.addr) );
            json_msg_pack_allow_notify( &info, jsonStr );
        }
        else
        {
            return;
        }
    }
    mlcoap_client_send_msg_multicast( jsonStr );
}

/********************************************************
 * function: main_monitor_modify_alarm_state
 * description: 监测更新安防状�?
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_modify_alarm_state( void )
{
    static uint32_t timerCount = 0;
    /////    以下代码 为临时处理方�?//////////////////////
    if ( timerCount++ % 5 == 4 )
    {
        PALARM_ENDPOINT_T palarmEndpoint = NULL;
        uint8_t alarmStateFlag = 0;

        storage_read_local_alarm_state( &palarmEndpoint );
        mlink_sys_get_status( SYS_ALARM_STATE, &alarmStateFlag );

        if ( palarmEndpoint->alarmType != alarmStateFlag )
        {
            alarmStateFlag = palarmEndpoint->alarmType;
            mlink_sys_set_status( SYS_ALARM_STATE, alarmStateFlag );
            os_main_logic_log("mico_system_context_update alarmType");
        }
    }
}

/*************************************************
 Function:     main_monitor_reboot_timer
 Description:  半夜重启
 Input:    �?
 Output:       �?
 Return:           �?
 Others:
 *************************************************/
void main_monitor_reboot_timer( void )
{
    unsigned char state = 0;
    static unsigned char s_TimeCheck = GETTIME_INTERVAL;
    static unsigned int s_timeCount = 0;
    static unsigned short s_TimeReboot = 0;
    static unsigned char s_TimeDay = 0xff;
    mlink_sys_get_status( SYS_ONLINE_STATE, &state );
    if ( state == 1 )
    {
        struct tm time;
        s_timeCount++;

        if ( 0 == s_TimeCheck )       // 间隔60秒获取一次时间，判断是否重启
        {
            unsigned int second = 0;
            time_t utc_time;
            s_TimeCheck = GETTIME_INTERVAL;
            // 半夜重启
            mico_time_get_utc_time( &utc_time );
            utc_time += TIME_ZONE;
            memcpy( &time, gmtime( &utc_time ), sizeof(struct tm) );

            if ( time.tm_hour >= 0 && time.tm_hour < 5 )
            {
                if ( 0 == s_TimeReboot )
                {
                    // get rand second.
                    s_TimeReboot = mlink_create_rand( SEC_BOUNDARY );
                }
                second = (time.tm_hour * 60 + time.tm_min) * 60 + time.tm_sec;
                if ( s_timeCount > second )           // 确认半夜是否重启�?
                {
                    if ( s_TimeReboot <= second )
                    {
                        if ( s_TimeDay == 0xff )
                        {
                            // generate a rand day .
                            s_TimeDay = mlink_create_rand( DAY_BOUNDARY );
                        }
                        if ( s_TimeDay-- )
                        {
                            printf( "s_TimeDay: %d\r\n", s_TimeDay );
                            s_timeCount = 0;
                        }
                        else
                        {
                            printf( "reboot time is day %d time %d:%d:%d\r\n", time.tm_mday,
                                    time.tm_hour, time.tm_min, time.tm_sec );
//                            os_main_logic_log("reboot time is day %d time %d:%d:%d", time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
                            MicoSystemReboot( );
                        }
                    }
                }
            }
        }
        else
        {
            s_TimeCheck--;
        }
    }
    else
    {
        s_TimeCheck = GETTIME_INTERVAL;
        s_timeCount = 0;
    }
}

/********************************************************
 * function: main_monitor_operate_status_handle
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_operate_status_handle( void *arg )
{
    uint32_t timeCount = 0;
    static uint32_t onlineflag = 0;
    static uint32_t unonlineflag = 0;
    uint8_t flag = 0;
    while ( 1 )
    {

        if ( (timeCount % 10) == 0 )       //  1s
        {

            main_monitor_reboot_timer( );

        }
        if ( (timeCount % 300) == 0 )       //  30s
        {

            onlineflag = get_g_elink_online_state();
            if(onlineflag != 0)
            {
                flag = 0;
                unonlineflag = 0;
                set_g_elink_online_state(flag);
            }
            else
            {
                unonlineflag ++;
                if(unonlineflag % 5 == 0)
                {
                    os_main_logic_log("elink_onlinetimeout");
                    MicoSystemReboot( );
                }

            }


        }

        timeCount++;
        msleep( 100 );
    }
}

/********************************************************
 * function: main_monitor_status_timer_init
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_monitor_status_timer_init( )
{
#if IS_REPORT_STATUS_TIMER
    OSStatus err = kNoErr;
    os_main_logic_log("init monitor status");
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "main_statechage_thread",
                                   (mico_thread_function_t) main_monitor_operate_status_handle,
                                   0x800, 0 );       //0x1500
    require_noerr_string( err, exit, "ERROR: Unable to start the main_statechage_thread thread." );

    exit:
    return;
//    mico_init_timer(&g_timer_handle_statechage, 1000, main_dev_status_change_timer_deal, NULL);
//    mico_start_timer(&g_timer_handle_statechage);
#endif
}
#endif

/********************************************************
 * function: main_scene_ctrl_deal
 * description:   子设备情景控�?
 * input:    1. scene_id
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_scene_ctrl_deal( char *scene_id )
{
    if ( scene_id != NULL )
    {
        REPORT_STATE_CHANGE_T stateInfo = { 0 };
        STATE_OBJ_T stateObj = { 0 };
        char sendData[256] = { 0 };
        stateInfo.subdevNum = 1;
        stateInfo.stateObj = &stateObj;
        sprintf( stateInfo.stateObj->endpointid, "%d_%s", SCENE_OBJ_ID, scene_id );
        json_msg_pack_statechange( &stateInfo, sendData );
        // 组播控制情景
        mlcoap_client_send_msg_multicast( sendData );

        // 情景联动控制
        mlcoap_server_ctrl_scene_link( scene_id );
    }
}

/********************************************************
 * function: main_synstatus_deal
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_synstatus_deal( uint8_t panid, SYN_STATUS_INFO_T *info )
{
    NETDEV_ADDR_INFO netInfo = { 0 };
    if ( info )
    {
        char jsonString[512] = { 0 };
        int ret = 0;
        ret = mlink_sys_get_sendaddr_by_panid( panid, &netInfo );
        if ( ret == kNoErr )
        {
            json_pack_ctrl_synstatus( info, 1, jsonString );
            os_main_logic_log("syn string: %s", jsonString);
            mlcoap_client_send_msg_non( netInfo.addr, URI_SERVICE_CTRL, jsonString );
        }
    }
}

/********************************************************
 * function: main_cloud_callback_deal
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int main_cloud_callback_deal( int funcId, char *data, int size )
{
    os_main_logic_log("cloud callback !! cmd :%d, size: %d", funcId, size);
    switch ( funcId )
    {
        case CLOUD_SERVER_STATE_REPORT:     // on/off
        {
            mlink_sys_set_status( SYS_ONLINE_STATE, *data );
//                pdevStatus = storage_get_dev_status_starting( 0 );
//                if ((pdevStatus->status.status_info.devStatus != (uint8_t)0xff) &&(pdevStatus != NULL))
//                {
//                    pdevStatus->status.status_info.devStatus = *data;
//                }
        }
            break;
    }
    return kNoErr;
}

/********************************************************
 * function: main_setmesh_trigger
 * description:   开始组�?
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_setmesh_trigger( )
{
#ifndef RF433_DEVICE
    uint8_t meshState = 0;
    mlink_sys_get_status( SYS_MESH_STATE, &meshState );
    os_main_logic_log("meshState is %d", meshState);
    if ( meshState == MESH_OUT )
    {
        server_ctrl_start_setmesh( );
    }
    else if ( meshState == MESH_IN )
    {
        server_ctrl_stop_setmesh( );
    }
#endif

}

/********************************************************
 * function: main_uart_comm_init
 * description:   initialization network communication
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int main_uart_comm_init( app_context_t * const inContext )
{
    OSStatus err = kNoErr;
    main_uart_init( );
    platformRestoreCallback( zigbee_gateway_restore );

    return err;
}

/********************************************************
 * function: main_init_subdev_info
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_init_subdev_info( void )
{
    NETDEVOBJ_T netDevObj = { 0 };
    storage_read_local_devobj( &netDevObj );
    if ( (uint8_t) netDevObj.uuid[0] >= 0x7f )
    {
        memset( &netDevObj, 0, sizeof(NETDEVOBJ_T) );
        storage_init_local_dev_info( &netDevObj );
        storage_write_local_obj( &netDevObj );
    }

    storage_init_object_num( );
    storage_init_status( );
    get_locklist_num();
    return;
}

/********************************************************
 * function: init_cron_table
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void init_cron_table( void )
{
    uint8_t linkageInput = 0;
    int lnkInputCount = 0;
    LINKAGE_INPUT_TASK_T inputTask = { 0 };

    // 联动定时部分
    linkageInput = storage_get_object_num( LINKAGE_INPUT_OBJ_ID );
    // 需测试最多添加多少个定时任务
    for ( lnkInputCount = 0; lnkInputCount < linkageInput; lnkInputCount++ )
    {
        storage_get_obj_batch( LINKAGE_INPUT_OBJ_ID, 1, lnkInputCount, (void *) &inputTask );
        servive_manage_linkage_cron_deal( 0, &inputTask );
    }
}

/********************************************************
 * function: mlcoap_server_discover_notify
 * description:   initialization every module
 * input:     1. inContext
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void main_sys_partition_maintenance( app_context_t * const inContext )
{
    if ( inContext->appConfig )
    {
        if ( inContext->appConfig->netdev.deviceId[0] == (char) 0xff )
        {
            main_restore_sys_partition( );
        }
        else
        {
            main_backups_sys_partition( );
        }
    }
}

/********************************************************
 * function: main_multi_click_distribute
 * description:   设备按键处理
 * input:    1. button_times:  按键次数
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   1. 在待机状态下按键处理�?�?1 下查看当前联网状态，未连到路由器绿灯慢闪3次，
 * 连到路由器未连入云端绿灯快闪3次，连到云端绿灯常亮 3s �?�?下子设备组网，绿灯快�?
 * 2. 在组网状态下按键的处理： �?1 下表示退出组网灯�?
 *********************************************************/
void main_multi_click_distribute( int button_times )
{
    uint8_t statusVal = 0;
    if ( button_times == 1 )
    {
        mlink_sys_get_status( SYS_MESH_STATE, &statusVal );
        if ( statusVal == MESH_IN )      // in mesh
        {
//            server_ctrl_stop_setmesh();
            return;
        }


        mlink_sys_get_status( SYS_ONLINE_STATE, &statusVal );
        if ( (statusVal != 0xff) && (statusVal != 0) )
        {
            if(elink_get_join_net_status() == 1)
            {
                mlink_led_cloud_connected( );

            }
            else
            {
                mlink_led_network_connected( );
            }

            return;
        }
        mlink_sys_get_status( SYS_WIFI_STATE, &statusVal );
        if ( NOTIFY_STATION_UP == statusVal )
        {
            if(elink_get_subonline_status() == 0)
            {
                mlink_led_network_connected( );
            }
        }
        else if ( NOTIFY_AP_UP == statusVal )
        {
            mico_Context_t* mainContext = mico_system_context_get( );
            mico_easylink( mainContext, MICO_FALSE );
            mlink_sys_get_status( SYS_MESH_STATE, &statusVal );
        }
        else
        {
            mlink_led_network_unconnected( );
        }
    }
    else if ( button_times == 5 )
    {
        mlink_sys_get_status( SYS_MESH_STATE, &statusVal );
        if ( statusVal == MESH_OUT )
        {

//            server_ctrl_start_setmesh
        }
    }
}

/********************************************************
 * function: main_logic_init
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int main_logic_init( app_context_t * const inContext )
{
    OSStatus err = kNoErr;

    // check partition and restore if we find any abnormal
    main_sys_partition_maintenance( inContext );

//    main_init_subdev_info( );
    if ( inContext != NULL )
    {
        mlink_sys_param_init( main_linkage_output_deal );
    }
//    alarm_logic_init();

    /*UART receive thread*/
    main_mutex_init( );
    time_sync_start( );

    err = coap_logic_init( st_coap_server_notify, st_coap_client_notify );
    require_noerr_string( err, exit, "ERROR: mlcoap_init error." );

    mlcloud_init( inContext, main_cloud_callback_deal );

    main_monitor_status_timer_init( );

#if defined RF433_DEVICE
    err = rf433_logic_init(NULL);
#elif defined ZIGBEE_DEVICE
    err = zigbee_logic_init(NULL);
//#elif defined BLE_DEVICE
//    err = ble_logic_init( main_ble_callback_proc );

#endif
#ifndef  BLE_DEVICE
    // start cron service
    cron_start_service();
    // init cron link table
    init_cron_table();
#endif

#ifdef BLE_ELINK
    elink_init_logic();
#endif
    exit:
    return err;
}

