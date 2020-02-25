#include "../coap/ml_coap.h"
#include "mico.h"
#include "MLinkAppDef.h"
#include "MLinkCommand.h"
#include "../mqtt/mqtt_client.h"
#include "cloud.h"

#define os_cloud_log(M, ...) custom_log("CLOUD_LOGIC", M, ##__VA_ARGS__)

static mqtt_pub_func g_pubTopicFunc[4] = {NULL};
static ml_cloud_logic_notify_t cloud_notify = NULL;
static unsigned char *g_cloud_connect = NULL;

/********************************************************
 * function: cloud_pub_topic
 * description: 云端主题发布
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void cloud_pub_topic(TOPIC_TYPE_E topic, unsigned char *msg, unsigned int msg_len)
{
    g_pubTopicFunc[topic](msg, msg_len);
}

/********************************************************
 * function: cloud_mqtt_rev_proc_Passthrough
 * description: MQTT 控制命令处理
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int cloud_mqtt_rev_proc_ctrl( uint8_t *revData, uint32_t datalen )
{
	int ret = 0;
	os_cloud_log("ctrl data[%d]: %s ", datalen, revData);
	/****
	 *1.  需进行AES解密处理，待处理
	 *
	 */
    char cmd[20] = {0};
    char *dataObj = malloc(1500);
    int cmdId = 0;
    ml_coap_ackPacket_t ackPacket = {0};
    memset(dataObj, 0, 1500);
    memset(&ackPacket, 0, sizeof(ml_coap_ackPacket_t));

	json_get_cmd(revData, datalen, cmd, dataObj);
	cmdId = json_get_ctrl_cmd_id(cmd);

	st_coap_server_notify(cmdId, dataObj, strlen(dataObj), NULL, &ackPacket);
	if (ackPacket.data != NULL)
	{
	    cloud_pub_topic(TOPIC_CTRLECHO, ackPacket.data, ackPacket.datalen);
	    free(ackPacket.data);
	}
	if (dataObj != NULL)
	{
	    free(dataObj);
	    dataObj = NULL;
	}

	return ret;
}

/********************************************************
 * function: cloud_mqtt_rev_proc_Passthrough
 * description: MQTT 透传数据处理
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int cloud_mqtt_rev_proc_Passthrough( uint8_t *revData, uint32_t datalen)
{
	int ret = 0;
	os_cloud_log("passthrough data[%d]: %s ",datalen,revData);
	return ret;
}

/********************************************************
 * function: cloud_mqtt_rev_proc_manage
 * description: MQTT 管理类处理
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
static int cloud_mqtt_rev_proc_manage( uint8_t *revData, uint32_t datalen)
{
    int ret = 0;
    os_cloud_log("manage data[%d]: %s ",datalen,revData);
    char cmd[20] = {0};
    char *dataObj = malloc(1500);
    int cmdId = 0;
    memset(dataObj, 0, 1500);
    json_get_cmd(revData, datalen, cmd, dataObj);
    cmdId = json_get_sys_cmd_id(cmd);
    st_coap_server_notify(cmdId, dataObj, strlen(dataObj), NULL, NULL);
    if (dataObj != NULL)
    {
        free(dataObj);
        dataObj = NULL;
    }
    return ret;
}

/********************************************************
 * function: mcloud_get_conn_state
 * description: 返回云端连接状态值
 * input:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
unsigned char  mcloud_get_conn_state( void )
{
    return *g_cloud_connect;
}

int mcloud_init_local_ver_info( char *str )
{
    if (str == NULL)
    {
        return kGeneralErr;
    }
    DEVINFOOBJ_T devInfo= {0};
    NETDEV_MQTT_INITINFO initInfo = {0};
    storage_read_devinfo_obj(&devInfo);
    if (devInfo.firmver[0] != 0)
    {
        sprintf(initInfo.ver, "%s", devInfo.firmver);
        initInfo.vertype = FIRMWARE_VER;
    }
    else if (devInfo.ver[0] != 0)
    {
        sprintf(initInfo.ver, "%s", devInfo.ver);
        initInfo.vertype = SOFTWARE_VER;
    }
    json_sys_pack_init_ver_info(&initInfo, str);
    return kNoErr;
}

int mcloud_conn_notify( unsigned char connect_state )
{
    uint8_t state = 0;
    os_cloud_log("mcloud_conn_notify!!!");
    if (g_cloud_connect == NULL)
    {
        os_cloud_log("g_cloud_connect is null!!!");
        return kGeneralErr;
    }
    if (connect_state == TRUE)
    {
        *g_cloud_connect = CLOUD_CONNECT_SUCCESS;
    }
    else if (*g_cloud_connect != (uint8_t)0xff)
    {
        *g_cloud_connect = CLOUD_CONNECT_FAIL;
    }
    state = *g_cloud_connect;
    if (cloud_notify != NULL)
    {
        cloud_notify(CLOUD_SERVER_STATE_REPORT, &state, 1);
    }
    return kNoErr;
}

int mlcloud_init( app_context_t * const inContext,ml_cloud_logic_notify_t mlcloud_callback)
{
	OSStatus err = kNoErr;
	TOPIC_RECV_DEAL_FUN_T topic = {0};

    if (inContext == NULL)
    {
        os_cloud_log("inContext is NULL !!");
    }
    cloud_notify = mlcloud_callback;
    g_cloud_connect = &inContext->appStatus.run_status.onlineStatus;
    mqtt_set_connect_callback(mcloud_conn_notify);
    err = MLinkStartMQTTClient(inContext);
    os_cloud_log("MLinkStartMQTTClient!!!!");

	topic.topic_ctrl_recv_deal = cloud_mqtt_rev_proc_ctrl;
    topic.topic_passthrough_recv_deal = cloud_mqtt_rev_proc_Passthrough;
    topic.topic_manage_recv_deal = cloud_mqtt_rev_proc_manage;
    topic.topic_room_ctrl_recv_deal = cloud_mqtt_rev_proc_ctrl;
    mqtt_local_ver_info_callback(mcloud_init_local_ver_info);
	mqtt_set_sub_callback(&topic);

	// 主题发布函数初始化
	g_pubTopicFunc[TOPIC_CTRLECHO] = mqtt_pub_ctrlecho;
    g_pubTopicFunc[TOPIC_MANAGE] = mqtt_pub_manage;
    g_pubTopicFunc[TOPIC_EVENTMI] = mqtt_pub_event;
    g_pubTopicFunc[TOPIC_NOTIFY] = mqtt_pub_notify;
    os_cloud_log("cloud init success !!!!");


	return err;
}


