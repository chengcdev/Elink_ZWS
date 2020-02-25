#include "mico.h"

#include "ml_coap.h"
#include "MLinkCommand.h"
#include "coap_service.h"

#define os_coap_logic_log(M, ...) custom_log("COAP_LOGIC", M, ##__VA_ARGS__)

coap_logic_notify_t g_coap_server_callback = NULL;


static void mlcoap_discover_devices(void)
{
    char payload[100] = {0};
//    json_pack_req_discover(NULL, payload);
    mlcoap_client_send_discover_multicast(payload);
}

/********************************************************
 * function: mlcoap_get_disc_cmd_id
 * description: get command ID on the basis of param 'cmd'
 * input:   1. cmd
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
static int mlcoap_get_disc_cmd_id(char *cmd)
{
    int cmd_id = COAP_SERVER_NOTI_CMD_ERR;
    // switch command string to command ID
    if (!strcmp(cmd, DISCOVER_WHO_IS))                   // discover device
    {
        cmd_id = COAP_DISCOVER_NOTI_CMD_WHOIS;
    }
    else if (!strcmp(cmd, DISCOVER_WHO_HAV))
    {
        cmd_id = COAP_DISCOVER_NOTI_CMD_WHO_HAV;
    }
    return cmd_id;
}

/********************************************************
 * function: mlcoap_service_logic
 * description: get command ID on the basis of param 'cmd'
 * input:   1. service： 服务类型
 *          2. data： payload 数据
 *          3. size: payload 数据大小
 *          4. srcaddr: 客户端host 携带端口号（类似： "192.168.1.1:8080"）
 * output:  1. packPacket： 应答数据
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
static int mlcoap_service_logic(ML_COAP_URITYPE_E service, unsigned char *data, int size, char *srcaddr, ml_coap_ackPacket_t *packPacket)
{
    int ret =TRUE;
//    os_coap_logic_log("coap recv count: %d", count++);
    if (data == NULL)
    {
        if (service == ML_COAP_URITYPE_DISCOVER)
        {
            if (g_coap_server_callback)
                ret =  g_coap_server_callback(COAP_DISCOVER_NOTI_CMD_WHOIS, NULL, 0, srcaddr, packPacket);
        }
        else if (service == ML_COAP_URITYPE_MSG)
        {
            if (g_coap_server_callback)
                ret = g_coap_server_callback(COAP_EVENT_NOTI_CMD_OBSERVER, NULL, 0, srcaddr, packPacket);
        }
    }
    else
    {
        char cmd[20] = {0};
        char *dataObj = malloc(2048);
        int coap_cmd_id = 0;
        memset(dataObj, 0, 2048);
        memset(cmd, 0, 20);
        json_get_cmd(data, size, cmd, dataObj);
        switch (service)
        {
            case ML_COAP_URITYPE_SYS:
                coap_cmd_id = json_get_sys_cmd_id(cmd);
                break;
            case ML_COAP_URITYPE_CTRL:
                coap_cmd_id = json_get_ctrl_cmd_id(cmd);//mlcoap_get_ctrl_cmd_id(cmd);
                break;

            case ML_COAP_URITYPE_MANAGER:
                coap_cmd_id = json_get_manage_cmd_id(cmd);
                break;

            case ML_COAP_URITYPE_DISCOVER:
                coap_cmd_id = mlcoap_get_disc_cmd_id(cmd);
                break;
            case ML_COAP_URITYPE_MSG:
                coap_cmd_id = json_get_msg_cmd_id(cmd);
                break;
            default:
                coap_cmd_id = COAP_SERVER_NOTI_CMD_ERR;
                break;
        }
        if ( coap_cmd_id == COAP_SERVER_NOTI_CMD_ERR )
        {
            packPacket->echoValue = MLCOAP_RET_SERVER_ERR;
        }
        else
        {
            if (g_coap_server_callback)
                ret =  g_coap_server_callback(coap_cmd_id, dataObj, strlen(dataObj), srcaddr, packPacket);
        }
        if (dataObj != NULL)
        {
            free(dataObj);
            dataObj = NULL;
        }
    }

    return ret;
}


/********************************************************
 * function: mlcoap_start_work
 * description:
 * input:
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
static void mlcoap_start_work( void )
{
    mlcoap_init_service_callback(mlcoap_service_logic);


    /********client operate*******/

    //mlcoap_discover_devices();

    /********service operate******/


}
static mico_system_monitor_t mico_monitor_coap_login;
static mico_system_monitor_t mico_monitor_coap_client;

/********************************************************
 * function: st_coap_logic_thread_proc
 * description:   coap 服务端处理线程
 * input:
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
static void st_coap_logic_thread_proc( void *arg )
{
    mlcoap_service_init();
    mlcoap_client_init();
    mlcoap_start_work();
    mico_system_monitor_register(&mico_monitor_coap_login, 10*1000);  // You must update time within 10

    while(1){
        ml_poll_coap();
        if (0 == ml_coap_service_check())      // check if coap service is live.
        {
            mico_system_monitor_update(&mico_monitor_coap_login, 10*1000);
        }
        else
        {
            os_coap_logic_log("ml coap service is err");
        }
       // mlcoap_client_poll_context();
    }
}

/********************************************************
 * function: st_coap_client_thread_proc
 * description:  coap 客户端处理线程
 * input:
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
static void st_coap_client_thread_proc( void *arg )
{
    mico_system_monitor_register(&mico_monitor_coap_client, 10*1000);

    while(1){
      //  ml_poll_coap();
        mlcoap_client_poll_context();

        mico_system_monitor_update(&mico_monitor_coap_client, 10*1000);
    }
    os_coap_logic_log("ml coap client is err");

}

/********************************************************
 * function: coap_logic_init
 * description:  coap 逻辑处理初始化
 * input:   1. coap_server_callback： 服务端回调
 *          2. coap_client_callback：客户端回调
 * output:
 * return:   command ID
 * auther:   chenb
 * other:
*********************************************************/
OSStatus coap_logic_init(coap_logic_notify_t coap_server_callback, coap_logic_notify_t coap_client_callback)
{
	OSStatus err = kNoErr;
	err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "coap_loigc_thread",
	                                   (mico_thread_function_t)st_coap_logic_thread_proc, 0x1500, 0 );//0x1500

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "coap_loigc_client_thread",
                                       (mico_thread_function_t)st_coap_client_thread_proc, 0x800, 0 );

	require_noerr_string( err, exit, "ERROR: Unable to start the coap module thread." );
	g_coap_server_callback = coap_server_callback;
exit:
	return err;
}


