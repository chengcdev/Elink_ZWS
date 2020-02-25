/*
 * main_server_sys.c
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
#include "../flash/flash_storage_distribute.h"
#include "MLinkObject.h"
#include "MLinkCommand.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../json/json_sys_deal.h"
#include "../ota/ota.h"
#include "main_logic.h"
#include "../cloud/cloud.h"
#include "../zigbee/zigbee.h"

#define os_server_sys_log(M, ...) custom_log("MAIN_SERVER_SYS", M, ##__VA_ARGS__)

static mico_mutex_t g_server_sys_mutex;

static mico_timer_t g_timer_handle;
static char g_verInfo[16] = {0};
static int g_verType = 0;
static char g_innerId[20] = {0};
static uint8_t g_sysNetTransType = 0;

/********************************************************
 * function: server_sys_connect_ap
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_connect_ap(PNET_PARAM_T param)
{
    network_InitTypeDef_adv_st  wNetConfigAdv={0};
    /* Initialize wlan parameters */
    strcpy((char*)wNetConfigAdv.ap_info.ssid, param->ssid);   /* wlan ssid string */

    if (param->key) {
        strcpy((char*)wNetConfigAdv.key, param->key);                /* wlan key string or hex data in WEP mode */
        wNetConfigAdv.key_len = strlen(param->key);                  /* wlan key length */
    } else {
        wNetConfigAdv.key_len = 0;
    }
    wNetConfigAdv.ap_info.security = param->security;                  /* wlan security mode */
    //wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;        /* wlan security mode */
    wNetConfigAdv.ap_info.channel = 0;                            /* Select channel automatically */
    wNetConfigAdv.dhcpMode = DHCP_Client;                         /* Fetch Ip address from DHCP server */
    wNetConfigAdv.wifi_retry_interval = 100;                      /* Retry interval after a failure connection */

    /* Connect Now! */
    os_server_sys_log("connecting to %s...\r\n", wNetConfigAdv.ap_info.ssid);
    micoWlanStartAdv(&wNetConfigAdv);
}

/********************************************************
 * function: server_sys_reset
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus server_sys_reset(void)
{
    MicoSystemReboot();

    // destroy timer
    mico_stop_timer( &g_timer_handle );
    mico_deinit_timer( &g_timer_handle );
    return kNoErr;
}

/********************************************************
 * function: server_sys_factory
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_factory( void *arg )
{
    PlatformEasyLinkButtonLongPressedCallback();
    // destroy timer
    mico_stop_timer( &g_timer_handle );
    mico_deinit_timer( &g_timer_handle );
}

/********************************************************
 * function: server_sys_ota_result_deal
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_ota_result_deal( uint8_t flag )
{
    UPGRADE_RESULT_PARAM_T resultParam = {0};
    NETDEVOBJ_T netDevObj = {0};
    char jsonResultStr[256] = {0};

    // clear upgrade flag
    mlink_sys_set_status(SYS_UPGRADE_STATE, 0);

    storage_read_local_devobj(&netDevObj);
    if (netDevObj.deviceId[0] != 0)
    {
        memcpy(resultParam.innerid, g_innerId, strlen(g_innerId));
        memcpy(resultParam.netDevId, netDevObj.deviceId, sizeof(resultParam.netDevId));
        if (flag)
        {
            resultParam.result[0] = '1';        // upgrade success
        }
        else
        {
            resultParam.result[0] = '0';        // upgrade fail
        }
        json_sys_pack_upgrade_result(&resultParam, jsonResultStr);
    }
    cloud_pub_topic(TOPIC_MANAGE, jsonResultStr, strlen(jsonResultStr));
    if (flag)
    {
        mico_thread_sleep(3);
        os_server_sys_log("publish upgrade result after 3s!!!");
    }
}

/********************************************************
 * function: server_sys_ota_success
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_ota_success( void )
{
    storage_update_local_softver(&g_verInfo, g_verType);
    server_sys_ota_result_deal(1);

}

/********************************************************
 * function: server_sys_ota_fail
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_ota_fail( void )
{
    server_sys_ota_result_deal(0);
}


/********************************************************
 * function: server_sys_get_local_detailed_info
 * description:
 * input:
 * output:   dev_detail_info
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static void server_sys_get_local_detailed_info( PDEV_DETAIL_INFO_T dev_detail_info)
{
    if (dev_detail_info != NULL)
    {
        NETDEVOBJ_T netDevObj = {0};
        DEVINFOOBJ_T devinfo;

        memset(&devinfo, 0, sizeof(DEVINFOOBJ_T));
        storage_read_local_devobj(&netDevObj);
        storage_read_devinfo_obj(&devinfo);
//        memcpy(dev_detail_info->firmver, devinfo.firmver, sizeof(devinfo.firmver));
        memcpy(dev_detail_info->software, devinfo.ver, sizeof(dev_detail_info->software));
        memcpy(dev_detail_info->mac, netDevObj.mac, sizeof(dev_detail_info->mac));
        memcpy(dev_detail_info->modelid, netDevObj.modelid, sizeof(dev_detail_info->modelid));
        strcpy(dev_detail_info->hardware, MicoGetVer());

#ifdef ZIGBEE_DEVICE        // zigbee网关�?zigbee模块的软件版本作为协议版本的参考依�?
        zigbee_get_dev_info(0x8000, dev_detail_info->protocol, NULL);
#else
        strcpy(dev_detail_info->protocol, YL_PROTOCOL);
#endif
        //                dev_detail_info->status = storage_get_dev_status_starting(0);
        mlink_sys_get_status(SYS_ONLINE_STATE, &dev_detail_info->status);
    }
}

/********************************************************
 * function: server_sys_get_subdev_detailed_info
 * description:
 * input:   devid: the device id
 * output:  dev_detail_info:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int server_sys_get_subdev_detailed_info( char *devid, PDEV_DETAIL_INFO_T dev_detail_info)
{
    int ret = kGeneralErr;

#ifdef ZIGBEE_DEVICE
    if (dev_detail_info != NULL)
    {
        DEVICEOBJ_T devObj = {0};
        DEVINFOOBJ_T devinfo;
        uint32_t addr = 0;

        memset(&devinfo, 0, sizeof(DEVINFOOBJ_T));
        storage_read_dev_obj(devid, &devObj);
        mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, (uint8_t *)&addr);

        ret = zigbee_get_dev_info(addr, dev_detail_info->software, dev_detail_info->hardware);
        if (ret != -1)
        {
            strcpy(dev_detail_info->addr, devObj.addr);
            memcpy(dev_detail_info->mac, devObj.mac, sizeof(dev_detail_info->mac));
            memcpy(dev_detail_info->modelid, devObj.modelid, sizeof(dev_detail_info->modelid));
//            strcpy(dev_detail_info->protocol, YL_PROTOCOL);
        }
    }
#endif
    return ret;
}

/********************************************************
 * function: server_sys_upgrade
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static OSStatus server_sys_upgrade( PUPGRADE_PARAM_T pUpgradeInfo)
{
    uint8_t upgradeState = 0;
    mlink_sys_get_status(SYS_UPGRADE_STATE, &upgradeState);
    if (upgradeState != 0)
    {
        return kGeneralErr;
    }
#ifdef VERSION_ENDPOINT_12
    if (memcmp(pUpgradeInfo->ver, "1.x.x", 2) == 0)
#else
        if (memcmp(pUpgradeInfo->ver, "1.1.x", 4) == 0)
#endif
    {
        mlink_sys_set_status(SYS_UPGRADE_STATE, 1);
        ota_server_init(server_sys_ota_success, server_sys_ota_fail);
        os_server_sys_log("url: %s, ver: %s, md5: %s", pUpgradeInfo->url, pUpgradeInfo->ver, pUpgradeInfo->md5);
        ota_server_start_ex(pUpgradeInfo->url, pUpgradeInfo->md5);
        memcpy(g_verInfo, pUpgradeInfo->ver, strlen(pUpgradeInfo->ver));
        memcpy(g_innerId, pUpgradeInfo->innerid, strlen(pUpgradeInfo->innerid));
        g_verType = pUpgradeInfo->vertype;
    }
    return kNoErr;
}

/********************************************************
 * function: server_sys_setmesh
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus server_sys_setmesh( PSETMESH_T pmesh_param )
{
    uint16_t devAddr = 0x8000;

    if (pmesh_param->devid[0] == 0)
    {
        devAddr = 0xffff;
    }
    else
    {
        DEVICEOBJ_T devObj = {0};
        OSStatus ret = 0;
        ret = storage_read_dev_obj(pmesh_param->devid, &devObj);
        if (ret > 0)
        {
            uint32_t netaddr = 0;
            mlink_parse_subdev_net_addr(TELECOM_ZIGBEE, devObj.addr, &netaddr);
            devAddr = (uint16_t)netaddr;
        }
        else
        {
            NETDEVOBJ_T netObj = {0};
            storage_read_local_devobj(&netObj);
            if (0 == strcmp(netObj.uuid, pmesh_param->devid))        // 本地设备的DEVICEID 是什�?
            {
                devAddr = 0x8000;
            }
        }
    }

    if (pmesh_param->flag == MESH_IN)
    {
        if ( pmesh_param->timeout == 0 )
        {
            pmesh_param->timeout = SETMESH_TIMEOUT;           // 默认进入组网时间�?0分钟
        }
#ifdef ZIGBEE_DEVICE
        return zigbee_setmesh(devAddr, pmesh_param->timeout);
#endif
#ifdef RF433_DEVICE
        return rf433_setmesh(devAddr, pmesh_param->timeout);
#endif
    }
    else if (pmesh_param->flag == MESH_OUT)
    {
#ifdef ZIGBEE_DEVICE
        return zigbee_out_of_setmesh( devAddr );
#endif
#ifdef RF433_DEVICE
        return rf433_out_of_setmesh(devAddr);
#endif
    }
    return kGeneralErr;
}



/********************************************************
 * function: server_sys_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus server_sys_sendaddr( unsigned char *data, char *srcaddr, ml_coap_ackPacket_t *packPacket )
{
    int ret = 0;
    if (data==NULL || packPacket==NULL)
    {
        return kGeneralErr;
    }
    SENDADDR_PARAM_T sendaddrParam = {0};
    NETDEV_ADDR_INFO netAddrInfo = {0};

    json_sys_unpack_sendaddr(data, &sendaddrParam);
//    os_server_sys_log("sendaddr: %s, srcaddr: %s", sendaddrParam.addr, srcaddr);
//    mlink_parse_space_data(sendaddrParam.addr, ':', param, &paramNum);
//    strcpy(ip, param[0]);
//    mlink_parse_space_data(srcaddr, ':', param, &paramNum);
//    if (paramNum >= 2)
//    {
//        strcpy(port, param[1]);
//    }
//    sprintf(sendaddrParam.addr, "%s:%s", ip, port);
//    strcpy(sendaddrParam.addr, sendaddrParam.addr);
    memcpy(&netAddrInfo, &sendaddrParam, sizeof(SENDADDR_PARAM_T));
    os_server_sys_log("sendaddr : %s", sendaddrParam.addr);
    mlink_sys_add_state_sendaddr( &netAddrInfo );

    return ret;
}

void server_sys_clear_timer_deal()
{
    static uint8_t count = 0;
    os_server_sys_log("clear deal: %d", count);
    if (count == 0)
    {
        storage_clear_data_block();
    }
    else
    {
        MicoSystemReboot();
        // destroy timer
        mico_stop_timer( &g_timer_handle );
        mico_deinit_timer( &g_timer_handle );
    }
    count++;

}

/********************************************************
 * function: server_sys_cleardata
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus server_sys_cleardata( unsigned char *data, ml_coap_ackPacket_t *packPacket )
{
    int ret = 0;
    if (data == NULL || packPacket == NULL)
    {
        return kGeneralErr;
    }
    char devid[DEVICEID_SIZE] = {0};
    json_sys_unpack_cleardata(data, devid);
    if (devid[0] == 0)
    {
        // 清空网关数据
        ret = mico_init_timer(&g_timer_handle, 1000, server_sys_clear_timer_deal, NULL);
        if (ret == kGeneralErr)
        {
            os_server_sys_log("mico_init_timer g_timer_handle fail");
        }
        /* start timer */
        ret = mico_start_timer(&g_timer_handle);
        if (ret == kGeneralErr)
        {
            os_server_sys_log("mico_start_timer g_timer_handle fail");
        }
    }
    else
    {
        DEVICEOBJ_T devObj = {0};
        uint16_t daddr = 0;
        storage_search_devobj_devid(devid, &devObj);
        mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, &daddr);

        // 清空子设备数�?
//        zigbee_cleardata(daddr);
    }
    return ret;
}

/********************************************************
 * function: mlcoap_server_sys_notify
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlcoap_server_sys_notify(COAP_NOTI_CMD_ID_E cmd, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
    OSStatus ret = kNoErr;

    MUTEX_LOCK(&g_server_sys_mutex);
//    debug(" mlcoap_server_sys_notify: cmd[%d] size[%d]\n", cmd, size);
    os_server_sys_log(" mlcoap_server_sys_notify: cmd[%d] size[%d]\n", cmd, size);

    if (packPacket == NULL)
    {
        g_sysNetTransType = NETTRANS_MQTT;
    }
    else
    {
        g_sysNetTransType = NETTRANS_COAP;
    }

    if (packPacket != NULL)
    {
        if (packPacket->data == NULL)
        {
            packPacket->data = malloc(COMMOND_RESP_DATA_SIZE);
            memset(packPacket->data, 0, COMMOND_RESP_DATA_SIZE);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
    }

    switch(cmd)
    {
    case COAP_SYS_NOTI_CMD_RESET:
        {
            OSStatus err = 0;
            OSStatus ret = MLINK_RESP_FIND_NOTHING;
            char devid[DEVICEID_SIZE] = {0};
            os_server_sys_log("reset system start");
            json_sys_unpack_reset(data, devid);
            if (devid[0] == 0)
            {
                err = mico_init_timer(&g_timer_handle, 1000, server_sys_reset, NULL);

                /* start timer */
                err = mico_start_timer(&g_timer_handle);
                ret = MLINK_RESP_OK;
            }

            json_pack_resp_errCode(cmd, ret, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_FACTORY:
        {
            uint8_t devid[DEVICEID_SIZE] = {0};
            DEVICEOBJ_T deviceObj;
            json_sys_unpack_factory(data, devid);
            if (devid[0] == 0)
            {
                os_server_sys_log("factory start");
                mico_init_timer(&g_timer_handle, 1000, server_sys_factory, packPacket);

                /* start timer */
                mico_start_timer(&g_timer_handle);
            }

            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_SYNCTIME:
        {
            uint32_t utc_time = 0;
            json_sys_unpack_synctime(data, &utc_time);
//            MLink_write_time(utc_time);
            MLink_set_system_time(&utc_time);

            // response ok
            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_SETMESH:
        {
//            SETMESH_T setmeshParam;
//            memset(&setmeshParam, 0, sizeof(SETMESH_T));
//            json_sys_unpack_setmesh(data, &setmeshParam);
//            ret = server_sys_setmesh( &setmeshParam );
//            if (packPacket->data != NULL)
//            {
//                if (ret == kNoErr)
//                {
//                    // response ok
//                    json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//                    packPacket->datalen = strlen(packPacket->data);
//                    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
//                }
//                else
//                {
//                    json_pack_resp_errCode(cmd, MLINK_RESP_ERROR, packPacket->data);
//                    packPacket->datalen = strlen(packPacket->data);
//                    packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_SERVER_ERR);
//                }
//
//            }

        }
        break;

    case COAP_SYS_NOTI_CMD_SETNET:
        {
//            NET_PARAM_T param;
//            memset(&param, 0, sizeof(param));
//            json_sys_unpack_netparam(data, &param);
//            server_sys_connect_ap(&param);
//
//            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_REGDEV:
        {
            // response ok
//            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_SETCLOUD:
        {
            // response ok
//            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;

    case COAP_SYS_NOTI_CMD_GETDEVINFO:
        {
//            DEV_DETAIL_INFO_T devDetail = {0};
//            char devid[DEVICEID_SIZE] = {0};
//            int ret = 0;
//            json_sys_unpack_getdevinfo(data, devid);
//            if (devid[0] == 0)
//            {
//                server_sys_get_local_detailed_info( &devDetail );
//            }
//            else
//            {
//                ret = server_sys_get_subdev_detailed_info( devid, &devDetail );
//            }
//            // response ok
//            if (ret == kGeneralErr)
//            {
//                json_pack_resp_errCode(cmd, MLINK_RESP_ERROR, packPacket->data);
//            }
//            else
//            {
//                json_sys_pack_getdevinfo_resp(&devDetail, packPacket->data);
//            }
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;
    case COAP_SYS_NOTI_CMD_SETADDR:
        {
//            char devid[DEVICEID_SIZE] = {0};
//            char subdevIp[NET_ADDR_SIZE] = {0};
//            DEVICEOBJ_T devObj = {0};
//            uint16_t shortAddr = 0;
//            uint16_t oldShortAddr = 0;
//            uint8_t subAddr[3] = {0};
//            uint8_t mac[8] = {0};
//            int ret = kGeneralErr;
//            json_sys_unpack_setaddr(data, devid, subdevIp);
//            storage_search_devobj_devid(devid, &devObj);
//            mlink_parse_subdev_net_addr(devObj.comm, subdevIp, subAddr);
//            memcpy(&shortAddr, subAddr, 2);
//            mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, subAddr);
//            memcpy(&oldShortAddr, subAddr, 2);
//            StrToHex(devObj.mac, 16, mac);
//            // response ok
//            if (ret >= 0)
//            {
//                // set short address success!!!
//                memset(devObj.addr, 0, sizeof(devObj.addr));
//                memcpy(devObj.addr, subdevIp, strlen(subdevIp));
//                storage_update_device_obj(1, &devObj);
//                json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//            }
//            else
//            {
//                json_pack_resp_errCode(cmd, MLINK_RESP_ERROR, packPacket->data);
//            }
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;
    case COAP_SYS_NOTI_CMD_KEEPALIVE:
        {
            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
            packPacket->datalen = strlen(packPacket->data);
            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;
    case COAP_SYS_NOTI_CMD_UPGRADE:
        {
            OSStatus err = 0;
            DEVINFOOBJ_T devVerInfo;
            UPGRADE_PARAM_T upgrade_param = {0};
            err = json_sys_unpack_upgrade(data, &upgrade_param);

            if (err == kNoErr)
            {
                storage_read_devinfo_obj(&devVerInfo);
                if (0 > strcmp(devVerInfo.ver, upgrade_param.ver))
                {
                    os_server_sys_log("upgrade start: url//%s\r\nmd5:%s", upgrade_param.url, upgrade_param.md5);
                    server_sys_upgrade(&upgrade_param);
                }
                else
                {
                    server_sys_ota_fail();
                }
            }
            if (g_sysNetTransType == NETTRANS_COAP)
            {
                // response ok
                json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
                packPacket->datalen = strlen(packPacket->data);
                packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
            }
        }
        break;
    case COAP_SYS_NOTI_CMD_RESETMESH:
        {
//            OSStatus ret = MLINK_RESP_FIND_NOTHING;//MLINK_RESP_OK;
//            char devid[DEVICEID_SIZE] = {0};
//            os_server_sys_log("resetmesh start");
//            json_sys_unpack_resetmesh(data, devid);
//            if (devid[0] == 0)
//            {
//                ret = MLINK_RESP_ERROR;
//            }
//
//            json_pack_resp_errCode(cmd, ret, packPacket->data);
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);

        }
        break;
    case COAP_SYS_NOTI_CMD_SENDADDR:
        {
//            server_sys_sendaddr(data, srcaddr, packPacket);
        }
        break;
    case COAP_SYS_NOTI_CMD_CLEARDATA:
        {
//            server_sys_cleardata(data, packPacket);
//            json_pack_resp_errCode(cmd, MLINK_RESP_OK, packPacket->data);
//            packPacket->datalen = strlen(packPacket->data);
//            packPacket->echoValue = COAP_RESPONSE_CODE(MLCOAP_RET_OK);
        }
        break;
    case COAP_SYS_NOTI_CMD_ERR:
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
    MUTEX_UNLOCK(&g_server_sys_mutex);

    return ret;
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
OSStatus mlcoap_server_sys_mutex_init( void )
{
    mico_rtos_init_mutex(&g_server_sys_mutex);
}
