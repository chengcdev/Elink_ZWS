/*
 * MLinkSysParam.c
 *
 *  Created on: 2018年2月26日
 *      Author: Administrator
 */

#include "mico.h"
#include "MLinkAppDef.h"
#include "../cloud/cloud.h"
#include "../main/main_alarm.h"
#include "../MLinkGpio/MLinkLedLogic.h"
#include "../MLinkCommand.h"
#define sys_status_log(M, ...) custom_log("SYS_STATUS", M, ##__VA_ARGS__)


static current_app_status_t *g_appStatus = NULL;
static PSEND_ADDR_T g_sendAddr = NULL;
static LINKAGE_STATUS_INFO_T g_lnkStatusInfo[LINKAGE_INPUT_TASK_NUM] = {0};

typedef void (*ml_linkage_output_deal)( char *lnk_id, uint8_t output_start_index );

static ml_linkage_output_deal linkage_output_deal_cb = NULL;


/********************************************************
 * function: mlink_set_linkage_input_cflag
 * description:
 * input:           1. lnk_id:
 *                  2. logic_index: start with zero
 *                  3. delay_time:  (uint/seconds)
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   更新联动状态置相应的标识位为1或记录为延时标识，如果条件都满足则触发联动动作
*********************************************************/
void mlink_set_linkage_input_cflag( uint8_t linkage_index, uint32_t cflag )
{
    g_lnkStatusInfo[linkage_index].cflagEffective ^= ((g_lnkStatusInfo[linkage_index].cflag^cflag) & g_lnkStatusInfo[linkage_index].cflag);
    g_lnkStatusInfo[linkage_index].cflag = cflag;
    sys_status_log("cflageffective: %x, cflag: %x", g_lnkStatusInfo[linkage_index].cflagEffective, cflag);
}

/********************************************************
 * function: mlink_set_linkage_output_delay
 * description:
 * input:           1. link_id:
 *                  2. output_index: start with zero
 *                  3. delay_time:  (uint/seconds)
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_set_linkage_output_delay( char *link_id , uint8_t output_index, uint16_t delay_time )
{
    uint8_t linkageIndex = 0;
    int lnkTaskNum = 0;
    if (link_id == NULL)
    {
        return;
    }
    lnkTaskNum = storage_get_object_num( LINKAGE_INPUT_OBJ_ID );

    for (linkageIndex=0; linkageIndex<lnkTaskNum; linkageIndex++)
    {
        if (!strcmp(link_id, g_lnkStatusInfo[linkageIndex].linkageId))
        {
            if (g_lnkStatusInfo[linkageIndex].outputDelay.startIndex == output_index)
            {
                return;
            }
        //    sys_status_log("start delay :%d", delay_time);
            g_lnkStatusInfo[linkageIndex].outputDelay.startIndex = output_index;
            g_lnkStatusInfo[linkageIndex].outputDelay.delay = delay_time;
            break;
        }
    }

}

/********************************************************
 * function: mlink_update_linkage_status
 * description:
 * input:           1. lnk_id:
 *                  2. logic_index: start with zero
 *                  3. delay_time:  (uint/seconds)
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   更新联动状态置相应的标识位为1或记录为延时标识，如果条件都满足则触发联动动作
*********************************************************/
void mlink_add_linkage_status( uint8_t lnk_index, char *lnk_id, uint32_t cflag )
{
    strcpy(g_lnkStatusInfo[lnk_index].linkageId, lnk_id);
    g_lnkStatusInfo[lnk_index].cflag = cflag;
    g_lnkStatusInfo[lnk_index].cflagEffective = 0;
    g_lnkStatusInfo[lnk_index].lnkInputDelayNum = 0;
}

/********************************************************
 * function: mlink_del_linkage_status_info
 * description:
 * input:           1. lnk_id:  为NULL时删除所有联动信息,否则删除指定联动信息
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   删除联动时同时删除与联动相关的信息
*********************************************************/
void mlink_del_linkage_status_info( char *lnk_id )
{
    int size = sizeof(LINKAGE_STATUS_INFO_T);
    if (lnk_id == NULL)
    {
        memset(g_lnkStatusInfo, 0, LINKAGE_INPUT_TASK_NUM*sizeof(LINKAGE_STATUS_INFO_T));
    }
    else
    {
        int lnkIndex = 0;
        for (lnkIndex=0; lnkIndex<LINKAGE_INPUT_TASK_NUM; lnkIndex++)
        {
            if (g_lnkStatusInfo[lnkIndex].linkageId[0] == 0)
            {
                break;
            }

            if (0 == strcmp(g_lnkStatusInfo[lnkIndex].linkageId, lnk_id))
            {
                if (lnkIndex < (LINKAGE_INPUT_TASK_NUM-1))
                {
                    memcpy(&g_lnkStatusInfo[lnkIndex], &g_lnkStatusInfo[lnkIndex+1], size*(LINKAGE_INPUT_TASK_NUM-1-lnkIndex));
                }
                else
                {
                    memset(&g_lnkStatusInfo[lnkIndex], 0, size);
                }
            }
        }
    }
}

/********************************************************
 * function: mlink_clear_linkage_status_info
 * description:
 * input:           1. linkage_index:  清除指定联动信息
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   删除联动时同时删除与联动相关的信息
*********************************************************/
void mlink_clear_linkage_status_info( char *linkage_id )
{
    uint8_t linkage_index = 0;
    int lnkTaskNum = 0;

    if (linkage_id == NULL)
    {
        return;
    }
    lnkTaskNum = storage_get_object_num( LINKAGE_INPUT_OBJ_ID );

    for (linkage_index=0; linkage_index<lnkTaskNum; linkage_index++)
    {
        if (!strcmp(linkage_id, g_lnkStatusInfo[linkage_index].linkageId))
        {
            if (linkage_index < LINKAGE_OUTPUT_TASK_NUM)
            {
                g_lnkStatusInfo[linkage_index].cflagEffective = 0;
                g_lnkStatusInfo[linkage_index].lnkInputDelayNum = 0;
                g_lnkStatusInfo[linkage_index].outputDelay.startIndex = 0;
                g_lnkStatusInfo[linkage_index].outputDelay.delay = 0;
            }
            break;
        }
    }
}

/********************************************************
 * function: mlink_update_linkage_status
 * description:
 * input:           1. lnk_id:
 *                  2. logic_index: start with zero
 *                  3. delay_time:  (uint/seconds)
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   更新联动状态置相应的标识位为1或记录为延时标识，如果条件都满足则触发联动动作
*********************************************************/
void mlink_update_linkage_status( char *lnk_id, uint8_t logic_index, uint16_t delay_time )
{
    int lnkTaskNum = 0;
    uint8_t lnkTaskIndex = 0;
    lnkTaskNum = storage_get_object_num( LINKAGE_OUTPUT_OBJ_ID );
    uint8_t delayIndex = 0;
    PLINKAGE_STATUS_INFO_T pLnkStatusInfoTemp = NULL;
    uint32_t flag = 0;

    for (lnkTaskIndex=0; lnkTaskIndex<lnkTaskNum; lnkTaskIndex++)
    {
        pLnkStatusInfoTemp = &g_lnkStatusInfo[lnkTaskIndex];
        if (!strcmp( pLnkStatusInfoTemp->linkageId, lnk_id ))
        {
            flag = 1<<logic_index;
            if (pLnkStatusInfoTemp->cflagEffective & flag)    // if condition had been trigger, exist.
            {
                sys_status_log("condition had been trigger.");
                break;
            }
            if (delay_time != 0)
            {
                // 循环查询该索引是否已经添加到延时索引中
                for (delayIndex=0; delayIndex < pLnkStatusInfoTemp->lnkInputDelayNum; delayIndex++)
                {
                    if ( pLnkStatusInfoTemp->inputDelay[delayIndex].index == logic_index )    // 延时条件已经记录过
                    {
                        break;
                    }
                }
                if (delayIndex >= pLnkStatusInfoTemp->lnkInputDelayNum)
                {
                    pLnkStatusInfoTemp->inputDelay[delayIndex].index = logic_index;
                    pLnkStatusInfoTemp->inputDelay[delayIndex].delay = delay_time;
                    pLnkStatusInfoTemp->lnkInputDelayNum ++;
                }
            }
            else
            {
                pLnkStatusInfoTemp->cflagEffective |= flag;
                sys_status_log("current cflag val: %x, flag: %x", pLnkStatusInfoTemp->cflagEffective, flag);
                // 如果条件满足则触发联动控制
                if (pLnkStatusInfoTemp->cflagEffective == pLnkStatusInfoTemp->cflag)
                {
                    if (linkage_output_deal_cb)
                    {
                        linkage_output_deal_cb(pLnkStatusInfoTemp->linkageId, 0);
                    }
                }
            }
            sys_status_log("logic_index: %x, cflag effective: %x, cflag:%x", logic_index, pLnkStatusInfoTemp->cflagEffective,  pLnkStatusInfoTemp->cflag);
            break;
        }
    }
}

/********************************************************
 * function: mlink_lnk_input_status_timing_deal
 * description: 处理联动条件延时触发
 * input:           1. linkage_index:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_lnk_input_status_timing_deal(  uint8_t linkage_index )
{
    uint8_t delayIndex = 0;
    PLINKAGE_STATUS_INFO_T plnkStatusInfo = NULL;
    plnkStatusInfo = &g_lnkStatusInfo[linkage_index];

    for (delayIndex=0; delayIndex<plnkStatusInfo->lnkInputDelayNum; delayIndex++)
    {
        if (--plnkStatusInfo->inputDelay[delayIndex].delay == 0)
        {
//            sys_status_log("delay end start exec");
            plnkStatusInfo->cflagEffective |= (1<<plnkStatusInfo->inputDelay[delayIndex].index);
            if (plnkStatusInfo->cflagEffective == plnkStatusInfo->cflag)
            {
                plnkStatusInfo->lnkInputDelayNum = 0;
                if (linkage_output_deal_cb)
                {
                    linkage_output_deal_cb(plnkStatusInfo->linkageId, 0);
                }
            }
        }
    }
}

/********************************************************
 * function: mlink_lnk_input_status_timing_deal
 * description: 联动定时或延时执行
 * input:           1. linkage_index:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_lnk_output_status_timing_deal(  uint8_t linkage_index )
{
    PLINKAGE_STATUS_INFO_T plnkStatusInfo = NULL;
    plnkStatusInfo = &g_lnkStatusInfo[linkage_index];
    if ((plnkStatusInfo->outputDelay.startIndex != 0) && (plnkStatusInfo->outputDelay.delay != 0))
    {
        plnkStatusInfo->outputDelay.delay--;
        if (plnkStatusInfo->outputDelay.delay == 0)
        {
            if (linkage_output_deal_cb)
            {
                linkage_output_deal_cb(plnkStatusInfo->linkageId, plnkStatusInfo->outputDelay.startIndex);
            }
        }
    }
}

/********************************************************
 * function: mlink_monitor_perform_linkage
 * description:
 * input:           1. lnk_id:
 *                  2. logic_index: start with zero
 *                  3. delay_time:  (uint/seconds)
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   1s 执行一次该处理
*********************************************************/
void mlink_monitor_perform_linkage( void )
{
    int linkageNum = storage_get_object_num(LINKAGE_OUTPUT_OBJ_ID);
    int linkageIndex = 0;
    for (linkageIndex=0; linkageIndex < linkageNum; linkageIndex++)
    {
        mlink_lnk_input_status_timing_deal(linkageIndex);
        mlink_lnk_output_status_timing_deal(linkageIndex);
    }

}

/********************************************************
 * function: mlink_sys_get_status
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_get_status(SYSTEM_STATE_E state_type, uint8_t *state)
{
    if (g_appStatus == NULL)
    {
        sys_status_log("g_appStatus IS NULL");
        return;
    }
    if (state == NULL)
    {
        switch (state_type)
        {
            case SYS_MESH_STATE:
                state = &g_appStatus->run_status.setmeshStatus;
                break;
            case SYS_UPGRADE_STATE:
                state = &g_appStatus->run_status.upgradeStatus;
                break;
            case SYS_ALARM_STATE:
                state = &g_appStatus->run_status.alarmState;
                break;
            case SYS_ONLINE_STATE:
                state = &g_appStatus->run_status.onlineStatus;
                break;
            case SYS_WIFI_STATE:
                state = &g_appStatus->run_status.wlanStatus;
                break;
            case SYS_EASYLINK_STATE:
                state = &g_appStatus->run_status.easylinkStatus;
                break;
            case SYS_UPDTAE_STATE:
                state = &g_appStatus->run_status.sysUpdateStatus;
                break;
            default:
                break;
        }
    }
    else
    {
        switch (state_type)
        {
            case SYS_MESH_STATE:
                *state = g_appStatus->run_status.setmeshStatus;
                break;
            case SYS_UPGRADE_STATE:
                *state = g_appStatus->run_status.upgradeStatus;
                break;
            case SYS_ALARM_STATE:
                *state = g_appStatus->run_status.alarmState;
                break;
            case SYS_ONLINE_STATE:
                *state = g_appStatus->run_status.onlineStatus;
                break;
            case SYS_WIFI_STATE:
                *state = g_appStatus->run_status.wlanStatus;
                break;
            case SYS_EASYLINK_STATE:
                *state = g_appStatus->run_status.easylinkStatus;
                break;
            case SYS_UPDTAE_STATE:
                *state = g_appStatus->run_status.sysUpdateStatus;
                break;
            default:
                break;
        }
    }
}

/********************************************************
 * function: mlink_sys_set_status
 * description:
 * input:       1. state_type:
 *              2. state
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_set_status(SYSTEM_STATE_E state_type, uint8_t state)
{
    if (g_appStatus == NULL)
    {
        sys_status_log("g_appStatus IS NULL");
        return;
    }
    switch (state_type)
    {
        case SYS_MESH_STATE:
            sys_status_log("mesh state: %d, setmeshStatus: %d", state, g_appStatus->run_status.setmeshStatus);
            if (state == MESH_IN)
            {
                mlink_led_setmesh_start();
            }
            else if (g_appStatus->run_status.setmeshStatus == MESH_IN)
            {
                mlink_led_standby();
            }
            g_appStatus->run_status.setmeshStatus = state;
            break;
        case SYS_UPGRADE_STATE:
        {
            if (state == 1)
            {
                mlink_led_upgrade();
            }
            else
            {
                mlink_led_standby();
            }
            g_appStatus->run_status.upgradeStatus = state;

        }
            break;
        case SYS_ALARM_STATE:
            g_appStatus->run_status.alarmState = state;
            break;
        case SYS_ONLINE_STATE:
            g_appStatus->run_status.onlineStatus = state;
            break;
        case SYS_WIFI_STATE:
        {
            g_appStatus->run_status.wlanStatus = state;
        }
            break;
        case SYS_EASYLINK_STATE:
        {
            if (state == RESTART_EASYLINK)
            {
                mlink_led_easylink_start();
            }
            else if (state == EXIT_EASYLINK)
            {
                mlink_led_standby();

            }
            g_appStatus->run_status.easylinkStatus = state;
        }
            break;
        case SYS_UPDTAE_STATE:
            g_appStatus->run_status.sysUpdateStatus = state;
            break;
        default:
            break;
    }
}

#if 0

/********************************************************
 * function: mlcoap_server_ota_success
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int mlink_sys_check_sendaddr( char *srcmac )
{
    uint8_t sendAddrNum = g_sendAddr->addrNum;
    uint8_t count = 0;
    for (count=0; count<sendAddrNum; count++)
    {

        if (g_sendAddr->addrInfo[count].srcmac[0] != 0)
        {
            if (!strcmp(g_sendAddr->addrInfo[count].srcmac, srcmac))
            {
                sys_status_log("this device is exist ");
                return count+1;
            }
            else
            {
                sys_status_log("srcmac: %s != %s", srcmac, g_sendAddr->addrInfo[count].srcmac);
            }
        }
    }
    return kGeneralErr;
}

/********************************************************
 * function: mlink_update_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
int mlink_update_state_sendaddr( uint8_t addr_index, PSENDADDR_PARAM_T sendaddr_content )
{
    if (sendaddr_content != NULL)
    {
        memcpy(&g_sendAddr->addrInfo[addr_index], sendaddr_content, sizeof(SENDADDR_PARAM_T));
        sys_status_log("addr_index: %d, mac: %s", addr_index, g_sendAddr->addrInfo[addr_index].srcmac);

        return TRUE;
    }
}

/********************************************************
 * function: mlink_sys_add_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_add_state_sendaddr( PSENDADDR_PARAM_T sendaddr_param )
{
    int ret = 0;
    if (sendaddr_param == NULL)
    {
        return;
    }

    ret = mlink_sys_check_sendaddr( sendaddr_param->srcmac );
    if (ret == kGeneralErr)
    {
        ret = mlink_update_state_sendaddr(g_sendAddr->addrNum, sendaddr_param);
        if (ret == TRUE)
        {
            g_sendAddr->addrNum++;
        }
    }
    else
    {
        mlink_update_state_sendaddr(ret-1, sendaddr_param);
    }
}

/********************************************************
 * function: mlink_sys_add_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_get_sendaddr( uint8_t addrIndex, PSENDADDR_PARAM_T sendaddr_param )
{
    if (sendaddr_param != NULL)
    {
        memcpy(sendaddr_param, &g_sendAddr->addrInfo[addrIndex], sizeof(SENDADDR_PARAM_T));
    }
}
#else

/********************************************************
 * function: mlink_sys_check_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int mlink_sys_check_sendaddr( PNETDEV_ADDR_INFO pnetdev_addr_info )
{
    uint8_t sendAddrNum = g_sendAddr->addrNum;
    uint8_t count = 0;
    if (pnetdev_addr_info == NULL)
    {
        return kGeneralErr;
    }
    for (count=0; count<sendAddrNum; count++)
    {
        if (g_sendAddr->addrInfo[count].uuid[0] != 0)
        {
            if (!strcmp(g_sendAddr->addrInfo[count].uuid, pnetdev_addr_info->uuid))
            {
                sys_status_log("this device is exist ");
                return count+1;
            }
            else
            {
                sys_status_log("uuid: %s != %s", pnetdev_addr_info->uuid, g_sendAddr->addrInfo[count].uuid);
            }
        }
        else if (g_sendAddr->addrInfo[count].srcmac[0] != 0)
        {
            if (!strcmp(g_sendAddr->addrInfo[count].srcmac, pnetdev_addr_info->srcmac))
            {
                sys_status_log("this device is exist ");
                return count+1;
            }
            else
            {
                sys_status_log("srcmac: %s != %s", pnetdev_addr_info->srcmac, g_sendAddr->addrInfo[count].srcmac);
            }
        }
    }
    return kGeneralErr;
}

/********************************************************
 * function: mlink_update_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
int mlink_update_state_sendaddr( uint8_t addr_index, PNETDEV_ADDR_INFO sendaddr_content )
{
    if (sendaddr_content != NULL)
    {
        memcpy(&g_sendAddr->addrInfo[addr_index], sendaddr_content, sizeof(NETDEV_ADDR_INFO));
        sys_status_log("addr_index: %d, uuid: %s", addr_index, g_sendAddr->addrInfo[addr_index].uuid);

        return TRUE;
    }
}

/********************************************************
 * function: mlink_sys_add_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_add_state_sendaddr( PNETDEV_ADDR_INFO sendaddr_param )
{
    int ret = 0;
    if (sendaddr_param == NULL)
    {
        return;
    }

    ret = mlink_sys_check_sendaddr( sendaddr_param );
    if (ret == kGeneralErr)
    {
        ret = mlink_update_state_sendaddr(g_sendAddr->addrNum, sendaddr_param);
        if (ret == TRUE)
        {
            g_sendAddr->addrNum++;
        }
    }
    else
    {
        mlink_update_state_sendaddr(ret-1, sendaddr_param);
    }
}

/********************************************************
 * function: mlink_sys_add_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_get_sendaddr( uint8_t addrIndex, PNETDEV_ADDR_INFO sendaddr_param )
{
    if (sendaddr_param != NULL)
    {
        memcpy(sendaddr_param, &g_sendAddr->addrInfo[addrIndex], sizeof(NETDEV_ADDR_INFO));
    }
}

/********************************************************
 * function: mlink_sys_get_sendaddr_num
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
//int mlink_sys_get_sendaddr_num( void )
//{
//    return g_sendAddr->addrNum;
//}

/********************************************************
 * function: mlink_sys_add_state_sendaddr
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlink_sys_get_sendaddr_by_panid( uint8_t panid, PNETDEV_ADDR_INFO sendaddr_param )
{
    uint16_t panidTemp = 0;
    uint8_t count = 0;

    if (sendaddr_param == NULL)
    {
        return kGeneralErr;
    }
    for (count=0; count < g_sendAddr->addrNum; count++)
    {
        StrToHex(g_sendAddr->addrInfo[count].panid, strlen(g_sendAddr->addrInfo[count].panid), (uint8_t *)&panidTemp);
        if (panid == panidTemp)
        {
            memcpy(sendaddr_param, &g_sendAddr->addrInfo[count], sizeof(NETDEV_ADDR_INFO));
            return kNoErr;
        }
        else
        {
            sys_status_log("panid %d != %d", panid, panidTemp);
        }
    }
    return kGeneralErr;
}

#endif




/********************************************************
 * function: mlink_sys_status_start
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_status_start( current_app_status_t *app_status )
{
    g_appStatus = app_status;
}

/********************************************************
 * function: mlink_sys_alarm_state_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_alarm_state_init( void )
{
    NETDEVOBJ_T netDev = {0};
    LOCAL_ENDPOINT_T localEndpoint = {0};

    storage_read_local_endpoint(&localEndpoint);
    storage_read_local_devobj(&netDev);

    if (netDev.deviceId[0] == 0)
    {
        mlink_sys_set_status(SYS_ONLINE_STATE, 0xff);
    }
    if (localEndpoint.alarmEndpoint.alarmType != 0)
    {
        mlink_sys_set_status(SYS_ALARM_STATE, localEndpoint.alarmEndpoint.alarmType);
    }
    else
    {
        mlink_sys_set_status(SYS_ALARM_STATE, ALARM_MODE_HOME);
    }
}

/********************************************************
 * function: mlink_sys_sendaddr_info_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_sendaddr_info_init( void )
{
    g_sendAddr = &g_appStatus->sendAddr;
    memset(g_sendAddr, 0, sizeof(SEND_ADDR_T));
}

/********************************************************
 * function: mlink_sys_local_edp_state_linkage_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_local_edp_state_linkage_init()
{
    char devid[DEVICEID_SIZE] = {0};
    char value[8] = {0};
    uint8_t alarmState = 0;
    mlink_sys_get_status(SYS_ALARM_STATE, &alarmState);
    sprintf(value, "%d", alarmState);
    mlink_get_local_id(devid);

    main_linkage_single_deal(DEVICE_OBJ_ID, devid, "30", 0, value);
}

/********************************************************
 * function: mlink_sys_linkage_status_info_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_linkage_status_info_init( ml_linkage_output_deal linkage_output_callback )
{
    int linkageInputNum = 0;
    int linkageIndex = 0;
    LINKAGE_INPUT_TASK_T linkageInputTask = {0};

    sys_status_log("mlink_sys_linkage_status_info_init");
    linkage_output_deal_cb = linkage_output_callback;
    memset(g_lnkStatusInfo, 0, sizeof(g_lnkStatusInfo));
    linkageInputNum = storage_get_object_num(LINKAGE_INPUT_OBJ_ID);

    // initial every linkage info
    for ( linkageIndex=0; linkageIndex < linkageInputNum; linkageIndex++ )
    {
        storage_read_linkage_input_obj(1, linkageIndex, &linkageInputTask);

        strcpy(g_lnkStatusInfo[linkageIndex].linkageId, linkageInputTask.linkageId);
        g_lnkStatusInfo[linkageIndex].cflag           = linkageInputTask.cflag;
        g_lnkStatusInfo[linkageIndex].cflagEffective  = 0;
        sys_status_log("linkageIndex: %d, cflag: %x", linkageIndex, g_lnkStatusInfo[linkageIndex].cflag);
    }

    mlink_sys_local_edp_state_linkage_init();
}

/********************************************************
 * function: mlink_sys_param_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void mlink_sys_param_init( ml_linkage_output_deal linkage_output_callback )
{
    sys_status_log("linkage param init");
    mlink_sys_alarm_state_init();

    mlink_sys_sendaddr_info_init();

    mlink_sys_linkage_status_info_init(linkage_output_callback);

}


