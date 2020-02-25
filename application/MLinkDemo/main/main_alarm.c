/*
 * main_alarm.c
 *
 *  Created on: 2018年1月1日
 *      Author: Administrator
 */

#include "mico.h"
#include "MLinkCommand.h"
#include "../cloud/cloud.h"
#include "ml_coap.h"
#include "main_alarm.h"
#include "MLinkAppDef.h"

#define os_alarm_log(M, ...) custom_log("ALARM_LOGIC", M, ##__VA_ARGS__)
#define ALARM_REPORT_SIZE       256
#define ALARM_TIMER_MAX         6
#define ALARM_TRIGGER_EVENT     "20"
#define ALARM_DEFAULT_ALERT_TIME   30

static QUEUE queue_alarm;
static mico_timer_t g_alarm_timer_handle;
static uint8_t g_alarm_timer = 0;           // 定时报警时长
static uint8_t g_alarm_alert_timer = 0;

/*************************************************
  Function:     alarm_init_queue
  Description:  初始化安防队列
  Input:
    queue       队列指针
    data_size   数据节点大小
  Output:
  Return:
    成功返回0
*************************************************/
int alarm_init_queue(QUEUE *queue)
{
    return init_queue(queue,sizeof(ALARM_INFO));
}

/*************************************************
  Function:     alarm_lookup_node
  Description:  查找安防节点
  Input:
    palarm_info       安防报警信息
  Output:
  Return:   返回已存在的安防节点
*************************************************/
pQUEUE_NODE alarm_lookup_node(char *endpoint_id)
{
    pQUEUE_NODE palarmNode = queue_alarm.head;
    PALARM_INFO palarmInfo = NULL;
    if (endpoint_id == NULL)
    {
        return NULL;
    }
    for (; palarmNode!=NULL; palarmNode=palarmNode->next)
    {
        palarmInfo = (PALARM_INFO)palarmNode->data;
        if (!strcmp(endpoint_id, palarmInfo->alarmEvent.msgContent.endpointid))
        {
            return palarmNode;
        }
    }
    return NULL;
}

/*************************************************
  Function:     alarm_check_node
  Description:  检查队列中是否存在该安防信息
  Input:
    alarm_info       安防报警信息
  Output:
  Return:
    成功      AU_SUCCESS
*************************************************/
int alarm_check_node(char *endpoint_id)
{
    pQUEUE_NODE palarmNode = NULL;
    palarmNode = alarm_lookup_node(endpoint_id);
    if (palarmNode == NULL)    // 设备未加入到队列中
    {
        return kNoErr;
    }
    else        // 设备报警已经加入到队列中
    {
        return kDuplicateErr;
    }
}



/*************************************************
  Function:     alarm_push_node
  Description:  报警入队
  Input:
    queue       队列指针
    data        欲入队的数据指针
  Output:
  Return:
    成功      AU_SUCCESS
    失败      AU_FAILURE
*************************************************/
int alarm_push_node( PALARM_INFO palarm_info )
{
    int ret = 0;
    ret = alarm_check_node(palarm_info->alarmEvent.msgContent.endpointid);
    os_alarm_log("check value is %d", ret);
    if (ret == kNoErr)
    {
        ret = push_node(&queue_alarm,(void*)palarm_info);
    }
    return ret;
}

/*************************************************
  Function:     alarm_pop_node
  Description:  报警数据出队
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
PALARM_INFO alarm_pop_node(QUEUE *queue)
{
    return (PALARM_INFO)pop_node(queue);
}

/*************************************************
  Function:     alarm_del_node
  Description:  删除当前报警节点
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
int alarm_del_node( pQUEUE_NODE node )
{
    return del_node(&queue_alarm, node);
}

/*************************************************
  Function:     alarm_empty_node
  Description:  清空报警队列
  Input:
    queue       队列指针
  Output:
  Return:
    成功      AU_SUCCESS
*************************************************/
int alarm_empty_node(QUEUE *queue)
{
    return empty_node(queue);
}

/********************************************************
 * function: alarm_trigger_deal
 * description:  报警事件触发处理
 * input:    1. alarm_notify:  报警通知事件信息
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
int alarm_trigger_deal(PEVENT_REPORT_T palarm_notify)
{
    char *sendData = NULL;
    if (palarm_notify == NULL)
    {
        os_alarm_log("palarm_notify is NULL!!!");
        return kGeneralErr;
    }
    sendData = malloc(ALARM_REPORT_SIZE);
    if (sendData != NULL)
    {
        memset(sendData, 0, ALARM_REPORT_SIZE);
        json_msg_pack_event(palarm_notify, sendData);

        // 报警上报
        ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
        if (mcloud_get_conn_state())
        {
            cloud_pub_topic(TOPIC_EVENTMI, sendData, strlen(sendData));
        }
        os_alarm_log("alarm is touching!");
//        mlcoap_client_send_msg_multicast(sendData);
        main_coap_noti_msg_to_netdev(sendData);
        free(sendData);

        // 报警蜂鸣器响5s
        if (g_alarm_timer == 0)
        {
            g_alarm_timer = ALARM_TIMER_MAX;
        }

        os_alarm_log("notify success !!!!");
    }

    return kNoErr;
}


/********************************************************
 * function: alarm_dev_dealy_timer_init
 * description:   延时报警处理
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_dev_delay_timer_deal( void )
{
    pQUEUE_NODE palarmNode = queue_alarm.head;
    PALARM_INFO palarmInfo = NULL;
    pQUEUE_NODE pnodeTemp = NULL;
    while (palarmNode)
    {
        if (g_alarm_alert_timer)
        {
            g_alarm_alert_timer --;
            break;
        }
        pnodeTemp = palarmNode;
        palarmNode = palarmNode->next;            // 指向下一节点

        palarmInfo = (PALARM_INFO)pnodeTemp->data;
        os_alarm_log("currenNode: 0x%x, preNode: 0x%x, nextNode:0x%x", pnodeTemp, pnodeTemp->prev, pnodeTemp->next);
        if (pnodeTemp->data == NULL)
        {
            os_alarm_log("alarmNode data is NULL!!!");
            alarm_del_node( pnodeTemp );
            continue;
        }
        if (palarmInfo->delayTime <= 1)
        {
            // 处理报警业务
            alarm_trigger_deal(&palarmInfo->alarmEvent);

            // 删除当前节点
            alarm_del_node( pnodeTemp );
        }
        else if (palarmInfo->delayTime > 1)
        {
            palarmInfo->delayTime--;
            os_alarm_log("device: %s, delayTime: %d", palarmInfo->alarmEvent.msgContent.endpointid, palarmInfo->delayTime);
        }
    }
}

/********************************************************
 * function: alarm_beep_timer_deal
 * description:  开启与关闭报警蜂鸣器
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_beep_timer_deal()
{
    if (g_alarm_timer != 0)
    {
#if !SUCK_TOP_DEVICE
        if (g_alarm_timer == ALARM_TIMER_MAX)
        {
            mlink_gpio_beep_on();
        }
        else if (g_alarm_timer == 1)
        {
            mlink_gpio_beep_off();
        }
        else
        {
            mlink_gpio_beep_trigger();
        }
#else
        if (g_alarm_timer == ALARM_TIMER_MAX)
        {
            mlink_led_alarm();
        }
        else if (g_alarm_timer == 1)
        {
            mlink_led_standby();
        }
#endif
        g_alarm_timer --;
    }
}


/********************************************************
 * function: alarm_timer_deal
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_timer_deal()
{
    alarm_dev_delay_timer_deal();
    alarm_beep_timer_deal();
}

/********************************************************
 * function: alarm_dev_delay_timer_init
 * description:   initialization every module
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_dev_delay_timer_init()
{
    mico_init_timer(&g_alarm_timer_handle, 1*1000, alarm_timer_deal, NULL);
    mico_start_timer(&g_alarm_timer_handle);
}

/********************************************************
 * function: alarm_event_deal
 * description:
 * input:    1. dev_alarm_state:
 *           2. palarm_info:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_event_deal( uint8_t dev_alarm_state, PALARM_INFO palarm_info )
{
//    os_alarm_log("dev_alarm_state: %d, delay: %d", dev_alarm_state, palarm_info->delayTime );
    if ((palarm_info->delayTime == 0) && ((g_alarm_alert_timer == 0) || (dev_alarm_state == ALARM_MODE_HOME)))
    {
        alarm_trigger_deal( &palarm_info->alarmEvent );
    }
    else
    {
        //加入报警链表
        alarm_push_node( palarm_info );
    }
}

/********************************************************
 * function: alarm_home_deal
 * description:  执行安防在家
 * input:    1. dev_alarm_state:
 *           2. palarm_info:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_home_deal( uint8_t dev_alarm_state, PALARM_INFO palarm_info )
{
    os_alarm_log("dev_alarm_state: %d", dev_alarm_state);
    if (dev_alarm_state == ALARM_MODE_HOME)    // 24小时警戒
    {
        alarm_event_deal(dev_alarm_state, palarm_info);
    }
}
/********************************************************
 * function: alarm_outgoing_deal
 * description: 执行安防外出
 * input:    1. dev_alarm_state:
 *           2. palarm_info:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_outgoing_deal( uint8_t dev_alarm_state, PALARM_INFO palarm_info )
{
    alarm_event_deal(dev_alarm_state, palarm_info);
}

/********************************************************
 * function: alarm_night_deal
 * description:  执行安防夜间操作
 * input:    1. dev_alarm_state:
 *           2. palarm_info:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_night_deal( uint8_t dev_alarm_state, PALARM_INFO palarm_info )
{
    os_alarm_log("alarm_night_deal ! dev state is %d", dev_alarm_state);
    if ((dev_alarm_state == ALARM_MODE_HOME) || (dev_alarm_state == ALARM_MODE_NIGHT))    // 24小时警戒
    {
        alarm_event_deal(dev_alarm_state, palarm_info);
    }
}

/********************************************************
 * function: alarm_back_to_normal_deal
 * description:  安防状态恢复
 * input:    1. palarm_info:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_back_to_normal_deal(PALARM_INFO palarm_info)
{
    pQUEUE_NODE palarmNode = alarm_lookup_node(palarm_info->alarmEvent.msgContent.endpointid);
    if (NULL != palarmNode)       // 存在于报警定时队列中的删除该节点
    {
        alarm_del_node(palarmNode);
    }
    else                         // 防区不处于延时报警队列中的节点则上报防区状态恢复
    {
        alarm_trigger_deal(&palarm_info->alarmEvent);
    }
}

/********************************************************
 * function: alarm_set_work_state
 * description:  更新安防设备的工作状态
 * input:    1. gw_alarm:  网关的布防状态（在家外出夜间）
 *           2. dev_alarm:  子设备布防属性（在家外出夜间）
 * output:   1.alarm_work： 通过网关的布防状态与子设备的布防属性获取子设备的安防状态。
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_set_work_state( ALARM_MODE_E gw_alarm, ALARM_MODE_E dev_alarm, uint8_t *alarm_work )
{
    switch(gw_alarm)
    {
        case ALARM_MODE_HOME:
            if (dev_alarm == ALARM_MODE_HOME)
            {
                *alarm_work = ALARM_WORKING;
            }
            else
            {
                *alarm_work = ALARM_NONWORKING;
            }
            break;
        case ALARM_MODE_OUTGOING:
            if (dev_alarm != ALARM_MODE_NIGHT)
            {
                *alarm_work = ALARM_WORKING;
            }
            else
            {
                *alarm_work = ALARM_NONWORKING;
            }
            break;
        case ALARM_MODE_NIGHT:
            *alarm_work = ALARM_WORKING;
            break;
        default:
            break;
    }
}

/********************************************************
 * function: alarm_statechange_deal
 * description:   安防状态变更通知
 * input:    1. device_id:   主设备或子设备ID
 *           2. val:  设备安防类型
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_statechange_deal( char *device_id, char *val )
{
    PALARM_ENDPOINT_T plocalAlarmEndpoint = NULL;
    PALARM_ENDPOINT_T pdevAlarmEndpoint = NULL;
    PALARM_ENDPOINT_T pdevAlarmEndpointTemp = NULL;
    uint32_t alarmNum = 0;
    uint8_t count = 0;
    char *sendData = NULL;
    REPORT_STATE_CHANGE_T stateInfo = {0};
    PSTATE_OBJ_T pstateObjTemp = NULL;
    const uint8_t reportNumMax = DEVICE_NUM/8;

    if ((device_id == NULL) || (val == NULL))
    {
        return;
    }
    sendData = malloc(1500);
    if (sendData == NULL)
    {
        os_alarm_log("malloc sendData fail !!!");
        return;
    }
    memset(sendData, 0, 1500);
    storage_read_local_alarm_state(&plocalAlarmEndpoint);
    storage_read_dev_alarm_status(&alarmNum, &pdevAlarmEndpoint);

    os_alarm_log("set device \"%s\" alarm %s", device_id, val);
    if (!strcmp(device_id, plocalAlarmEndpoint->devid))
    {
//        os_alarm_log("stateObj size is : %d", reportNumMax*sizeof(STATE_OBJ_T));
        stateInfo.stateObj = malloc(reportNumMax*sizeof(STATE_OBJ_T));

        memset(stateInfo.stateObj, 0, sizeof(reportNumMax*sizeof(STATE_OBJ_T)));
        plocalAlarmEndpoint->alarmType = (uint8_t)atoi(val);
        sprintf(stateInfo.status, "[30|30|%d]", plocalAlarmEndpoint->alarmType);
        strcpy(stateInfo.devid, device_id);
        if (alarmNum != 0)
        {
            for (count=0; count < alarmNum; count++)
            {
                pstateObjTemp = stateInfo.stateObj+count%reportNumMax;
                memset(pstateObjTemp, 0, sizeof(STATE_OBJ_T));
                pdevAlarmEndpointTemp = pdevAlarmEndpoint+count;
                alarm_set_work_state(plocalAlarmEndpoint->alarmType, pdevAlarmEndpointTemp->alarmType, &pdevAlarmEndpointTemp->workState);
                strcpy(pstateObjTemp->subdevid, pdevAlarmEndpointTemp->devid);
                sprintf(pstateObjTemp->status, "[11|11|%d]", pdevAlarmEndpointTemp->workState);
                stateInfo.subdevNum++;
                if ((stateInfo.subdevNum%reportNumMax) == 0)
                {
                    json_msg_pack_statechange(&stateInfo, sendData);
                    ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                    if (mcloud_get_conn_state())
                    {
                        cloud_pub_topic(TOPIC_NOTIFY, sendData, strlen(sendData));
                    }
//                    mlcoap_client_send_msg_multicast(sendData);
                    memset(stateInfo.stateObj, 0, sizeof(reportNumMax*sizeof(STATE_OBJ_T)));
                    pstateObjTemp = stateInfo.stateObj;
                    memset(&stateInfo, 0, sizeof(REPORT_STATE_CHANGE_T));
                    stateInfo.stateObj = pstateObjTemp;
                }
            }
        }

        if ((alarmNum == 0) || (stateInfo.subdevNum != 0))
        {

            json_msg_pack_statechange(&stateInfo, sendData);
            ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
            if (mcloud_get_conn_state())
            {
                cloud_pub_topic(TOPIC_NOTIFY, sendData, strlen(sendData));
            }
////            mlcoap_client_send_msg_multicast(sendData);
        }
    }
    else
    {
        stateInfo.stateObj = malloc(sizeof(STATE_OBJ_T));
        for (count=0; count<alarmNum; count++)
        {
            memset(stateInfo.stateObj, 0, sizeof(STATE_OBJ_T));
            pdevAlarmEndpointTemp = pdevAlarmEndpoint + count;
            if (!strcmp(device_id, pdevAlarmEndpointTemp->devid))
            {
                pdevAlarmEndpointTemp->alarmType = (uint8_t)atoi(val);
                alarm_set_work_state(plocalAlarmEndpoint->alarmType, pdevAlarmEndpointTemp->alarmType, &pdevAlarmEndpointTemp->workState);
                stateInfo.subdevNum = 1;
                strcpy(stateInfo.stateObj->subdevid, device_id);
                sprintf(stateInfo.stateObj->status, "[11|11|%d]", pdevAlarmEndpointTemp->workState);
                json_msg_pack_statechange(&stateInfo, sendData);
                ml_coap_notify_obs(ML_COAP_URITYPE_MSG, sendData, strlen(sendData));
                if (mcloud_get_conn_state())
                {
                    cloud_pub_topic(TOPIC_NOTIFY, sendData, strlen(sendData));
                }
//                mlcoap_client_send_msg_multicast(sendData);

                break;
            }
        }
    }
    if (sendData != NULL)
    {
        if (stateInfo.stateObj != NULL)
        {
            free(stateInfo.stateObj);
        }
        free(sendData);
    }
    if (alarmNum != 0)
    {
        storage_record_dev_alarm_state(alarmNum, pdevAlarmEndpoint);
    }

    mlink_sys_set_status(SYS_UPDTAE_STATE, 1);
    storage_write_local_alarm_state(plocalAlarmEndpoint);
}

/********************************************************
 * function: alarm_gateway_statechange_deal
 * description:  网关安防状态变更处理
 * input:    1. device_id:
 *           2. gw_alarm:  point to 'uint16_t'
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_gateway_statechange_deal( char *device_id,  ALARM_MODE_E gw_alarm )
{
    switch (gw_alarm)
    {
        case ALARM_MODE_HOME:
            alarm_statechange_deal(device_id, "0");
            alarm_empty_node(&queue_alarm);
            break;
        case ALARM_MODE_NIGHT:
            alarm_statechange_deal(device_id, "2");
            g_alarm_alert_timer = ALARM_DEFAULT_ALERT_TIME;
            break;
        case ALARM_MODE_OUTGOING:
            alarm_statechange_deal(device_id, "1");
            g_alarm_alert_timer = ALARM_DEFAULT_ALERT_TIME;
            break;
    }
}


/********************************************************
 * function: alarm_distribute
 * description:
 * input:    1. device_id:
 *           2. palarm_notify:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_distribute( char *device_id, PEVENT_REPORT_T palarm_notify)
{
    PALARM_ENDPOINT_T localAlarmEndpoint = NULL;
    ALARM_ENDPOINT_T devAlarmEndpoint = {0};
    ALARM_INFO alarmInfo = {0};
    int ret = 0;
    if (device_id == NULL || (palarm_notify == NULL))
    {
        os_alarm_log("device_id is 0x%x, palarm_notify: 0x%x", device_id, palarm_notify);
        return;
    }
    ret = storage_search_dev_alarm(device_id, &devAlarmEndpoint);
    if (ret == kGeneralErr )
    {
//        os_alarm_log("Look up alarm fail !!!");
        return;
    }
    storage_read_local_alarm_state(&localAlarmEndpoint);

    memcpy(&alarmInfo.alarmEvent, palarm_notify, sizeof(EVENT_REPORT_T));
    alarmInfo.delayTime = devAlarmEndpoint.alarmDelay;

    os_alarm_log("alarm notify val is %s", palarm_notify->msgContent.value);

    // If the event is not trigger alarm, the gateway alarms right now !
    if (strcmp(palarm_notify->msgContent.key, ALARM_TRIGGER_EVENT) != 0)
    {
        if (atoi(palarm_notify->msgContent.value) != 0)
        {
            alarm_trigger_deal(&alarmInfo.alarmEvent);
        }
    }
    else    // trigger
    {
//        if (atoi(palarm_notify->msgContent.value) == 0)
//        {
//            os_alarm_log("alarm is recover");
//            // 安防恢复
//            alarm_back_to_normal_deal(&alarmInfo);
//        }
//        else
        if (atoi(palarm_notify->msgContent.value) != 0)
        {
            switch (localAlarmEndpoint->alarmType)
            {
                case ALARM_MODE_HOME:
                    alarm_home_deal(devAlarmEndpoint.alarmType, &alarmInfo);
                    break;
                case ALARM_MODE_OUTGOING:
                    alarm_outgoing_deal(devAlarmEndpoint.alarmType, &alarmInfo);
                    break;
                case ALARM_MODE_NIGHT:
                    alarm_night_deal(devAlarmEndpoint.alarmType, &alarmInfo);
                    break;
            }
        }
    }

}

/********************************************************
 * function: alarm_work_state_init
 * description:  安防工作状态初始化
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_work_state_init( void )
{
    PALARM_ENDPOINT_T plocalAlarmEndpoint = NULL;
    PALARM_ENDPOINT_T pdevAlarmEndpoint = NULL;
    PALARM_ENDPOINT_T pdevAlarmEndpointTemp = NULL;
    uint32_t alarmNum = 0;
    uint32_t count = 0;

    storage_read_local_alarm_state(&plocalAlarmEndpoint);
    storage_read_dev_alarm_status(&alarmNum, &pdevAlarmEndpoint);
    for (count=0; count<alarmNum; count++)
    {
        pdevAlarmEndpointTemp = pdevAlarmEndpoint+count;
        alarm_set_work_state(plocalAlarmEndpoint->alarmType, pdevAlarmEndpointTemp->alarmType, &pdevAlarmEndpointTemp->workState);
    }
    if (alarmNum != 0)
    {
        storage_record_dev_alarm_state(alarmNum, pdevAlarmEndpoint);
    }

}

/********************************************************
 * function: alarm_logic_init
 * description:  上电时安防逻辑初始化
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void alarm_logic_init( void )
{
    os_alarm_log("init alarm logic !!!");
    alarm_init_queue(&queue_alarm);     // 安防队列初始化
    alarm_dev_delay_timer_init();       // 安防延时报警定时器初始化
    alarm_work_state_init();
}

