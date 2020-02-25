/**
******************************************************************************
* @file    mqtt_client.c
* @author  huangxf
* @version V1.0.0
* @date    2017骞�5鏈�23鏃�
* @brief   This file provides xxx functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2017 MXCHIP Inc.
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
******************************************************************************
*/ 

#include "../../application/MLinkDemo/MLinkAppDef.h"
#include "mico.h"
#include "MQTTClient.h"
#include "mqtt_client.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define mqtt_log(M, ...) custom_log("MQTT", M, ##__VA_ARGS__)

/******************************************************
 *                    Constants
 ******************************************************/
#if 0
#define MQTT_CLIENT_ID         "MiCO_MQTT_Client"
#define MQTT_CLIENT_USERNAME    NULL
#define MQTT_CLIENT_PASSWORD    NULL
#define MQTT_CLIENT_KEEPALIVE   30
#define MQTT_CLIENT_SUB_TOPIC   "mico/test/send"  // loop msg
#define MQTT_CLIENT_PUB_TOPIC   "mico/test/send"
#define MQTT_CMD_TIMEOUT        5000  // 5s
#define MQTT_YIELD_TMIE         5000  // 5s
#define MQTT_CLIENT_PUB_MSG     "mico_mqtt_client_test_data_1234567890"

#else
#define MQTT_CLIENT_ID         "1000120054"
#define MQTT_CLIENT_USERNAME    "sub"
#define MQTT_CLIENT_PASSWORD    "sub"

//#define MQTT_CLIENT_USERNAME    NULL
//#define MQTT_CLIENT_PASSWORD    NULL
#define MQTT_CLIENT_KEEPALIVE   30
#define MQTT_CLIENT_SUB_TOPIC   "0000/1.0/010101/9000/Ctrl"  // loop msg Passthrough
#define MQTT_CLIENT_SUB_TOPIC1   "0000/1.0/010101/9000/Passthrough"  // loop msg Passthrough

#define MQTT_CLIENT_PUB_TOPIC   "0000/1.0/010101/9000/Event11"
#define MQTT_CMD_TIMEOUT        5000  // 5s
#define MQTT_YIELD_TMIE         5000  // 5s
#define MQTT_CLIENT_PUB_MSG     "mico_mqtt_client_test_data_1234567890"

#endif
//#define MQTT_CLIENT_SSL_ENABLE  // ssl

#define MAX_MQTT_TOPIC_SIZE         (256)
#define MAX_MQTT_DATA_SIZE          (1024)
#define MAX_MQTT_SEND_QUEUE_SIZE    (5)

#ifdef MQTT_CLIENT_SSL_ENABLE

#define MQTT_SERVER             "test.mosquitto.org"
#define MQTT_SERVER_PORT        8883
char* mqtt_server_ssl_cert_str =
"-----BEGIN CERTIFICATE-----\r\n\
MIIC8DCCAlmgAwIBAgIJAOD63PlXjJi8MA0GCSqGSIb3DQEBBQUAMIGQMQswCQYD\r\n\
VQQGEwJHQjEXMBUGA1UECAwOVW5pdGVkIEtpbmdkb20xDjAMBgNVBAcMBURlcmJ5\r\n\
MRIwEAYDVQQKDAlNb3NxdWl0dG8xCzAJBgNVBAsMAkNBMRYwFAYDVQQDDA1tb3Nx\r\n\
dWl0dG8ub3JnMR8wHQYJKoZIhvcNAQkBFhByb2dlckBhdGNob28ub3JnMB4XDTEy\r\n\
MDYyOTIyMTE1OVoXDTIyMDYyNzIyMTE1OVowgZAxCzAJBgNVBAYTAkdCMRcwFQYD\r\n\
VQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwGA1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1v\r\n\
c3F1aXR0bzELMAkGA1UECwwCQ0ExFjAUBgNVBAMMDW1vc3F1aXR0by5vcmcxHzAd\r\n\
BgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hvby5vcmcwgZ8wDQYJKoZIhvcNAQEBBQAD\r\n\
gY0AMIGJAoGBAMYkLmX7SqOT/jJCZoQ1NWdCrr/pq47m3xxyXcI+FLEmwbE3R9vM\r\n\
rE6sRbP2S89pfrCt7iuITXPKycpUcIU0mtcT1OqxGBV2lb6RaOT2gC5pxyGaFJ+h\r\n\
A+GIbdYKO3JprPxSBoRponZJvDGEZuM3N7p3S/lRoi7G5wG5mvUmaE5RAgMBAAGj\r\n\
UDBOMB0GA1UdDgQWBBTad2QneVztIPQzRRGj6ZHKqJTv5jAfBgNVHSMEGDAWgBTa\r\n\
d2QneVztIPQzRRGj6ZHKqJTv5jAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\r\n\
A4GBAAqw1rK4NlRUCUBLhEFUQasjP7xfFqlVbE2cRy0Rs4o3KS0JwzQVBwG85xge\r\n\
REyPOFdGdhBY2P1FNRy0MDr6xr+D2ZOwxs63dG1nnAnWZg7qwoLgpZ4fESPD3PkA\r\n\
1ZgKJc2zbSQ9fCPxt2W3mdVav66c6fsb7els2W2Iz7gERJSX\r\n\
-----END CERTIFICATE-----";

#else  // ! MQTT_CLIENT_SSL_ENABLE

//#define MQTT_SERVER             "test.mosquitto.org"
//#define MQTT_SERVER_PORT        1883

//#define MQTT_SERVER             "iot.channel.aliyun.com"
#define MQTT_SERVER             "yun.miligc.com"// "10.110.20.252"//
#define MQTT_SERVER_PORT        1883


#endif // MQTT_CLIENT_SSL_ENABLE


/******************************************************
 *               Function Declarations
 ******************************************************/

static void mqtt_client_thread( mico_thread_arg_t arg );
static void messageArrived( MessageData* md );
static OSStatus mqtt_msg_publish( Client *c, const char* topic, char qos, char retained,
                                  const unsigned char* msg,
                                  uint32_t msg_len );

OSStatus user_recv_handler( void *arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/

mico_queue_t   mqtt_msg_send_queue = NULL;

Client c;  // mqtt client object
Network n;  // socket network for mqtt client

static mico_worker_thread_t  mqtt_client_worker_thread; /* Worker thread to manage send/recv events */
static mico_timed_event_t   mqtt_client_send_event;
static PMQTT_DEAL_CALLBACK mqtt_get_ver_info = NULL;
static application_config_t *stApp_config =NULL;

static unsigned char st_client_username[48]={0};
static unsigned char st_client_passwd[48]={0};
static unsigned char st_client_id[96]={0};
static unsigned char st_topic_pre[96]={0};
static unsigned char st_topic_room_pre[96] = {0};
static mqtt_connect_notify_t mqtt_conn_notify = NULL;
static unsigned char g_mqtt_conn = FALSE;

/******************************************************
 *             Custom static  Function Definitions
 ******************************************************/
static mqtt_sub_topic_t g_subtopic[4]={
		{
				"Ctrl",
				"",
				NULL,
		},
		{
				"Passthrough",
				"",
				NULL,
		},
        {
                "ManagePlatform",
                "",
                NULL,
        },
        {
                "Ctrl",
                "",
                NULL,
        },
};

void mqtt_set_connect_callback(mqtt_connect_notify_t connect_notify)
{
    mqtt_conn_notify = connect_notify;
}

void mqtt_mack_connect_state(unsigned char flag)
{
    if ((mqtt_conn_notify != NULL) && (g_mqtt_conn != flag))
    {
        mqtt_conn_notify(flag);
    }
    g_mqtt_conn = flag;
}

void mqtt_set_sub_callback(PTOPIC_RECV_DEAL_FUN_T proc)
{
	g_subtopic[0].handle = proc->topic_ctrl_recv_deal;
	g_subtopic[1].handle = proc->topic_passthrough_recv_deal;
    g_subtopic[2].handle = proc->topic_manage_recv_deal;
    g_subtopic[3].handle = proc->topic_room_ctrl_recv_deal;
}

void mqtt_local_ver_info_callback(PMQTT_DEAL_CALLBACK proc_get_ver_info)
{
    mqtt_get_ver_info = proc_get_ver_info;
}

static OSStatus mqtt_send_pub(uint8_t *topic,char qos, const unsigned char* msg,uint32_t msg_len )
{
    OSStatus err = kUnknownErr;
    p_mqtt_send_msg_t p_send_msg = NULL;
//    app_log("======App prepare to send ![%d]======", MicoGetMemoryInfo()->free_memory);
    if ((topic == NULL) || (msg == NULL) || (msg_len == 0))
    {
        return kValueErr;
    }
    if (FALSE == g_mqtt_conn)
    {
        return kConnectionErr;
    }

    /* Send queue is full, pop the oldest */
    if ( mico_rtos_is_queue_full( &mqtt_msg_send_queue ) == true ){
        mico_rtos_pop_from_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
        free( p_send_msg );
        p_send_msg = NULL;
    }
    /* Push the latest data into send queue*/
    p_send_msg = (p_mqtt_send_msg_t) calloc( 1, sizeof(mqtt_send_msg_t) );
    require_action( p_send_msg, exit, err = kNoMemoryErr );

    p_send_msg->qos = qos;
    p_send_msg->retained = 0;

    sprintf(p_send_msg->topic,"%s%s",st_topic_pre,topic);
    memcpy( p_send_msg->data, msg, msg_len );
    p_send_msg->datalen = strlen( p_send_msg->data );

    err = mico_rtos_push_to_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
    require_noerr( err, exit );

    app_log("Push mqtt public msg into send queue success! topic: %s", p_send_msg->topic);

exit:
    if( err != kNoErr && p_send_msg ) free( p_send_msg );
    return err;

}

OSStatus mqtt_pub_notify(const unsigned char* msg,uint32_t msg_len)
{
	return mqtt_send_pub("Notify",QOS0,msg,msg_len);
}

OSStatus mqtt_pub_event(const unsigned char* msg,uint32_t msg_len)
{
	return mqtt_send_pub("EventMi",QOS2,msg,msg_len);
}
OSStatus mqtt_pub_error(const unsigned char* msg,uint32_t msg_len)
{
	return mqtt_send_pub("Error",QOS0,msg,msg_len);
}
OSStatus mqtt_pub_manage(const unsigned char* msg,uint32_t msg_len)
{
    return mqtt_send_pub("ManageDevice",QOS0,msg,msg_len);
}

OSStatus mqtt_pub_ctrlecho(const unsigned char* msg,uint32_t msg_len)
{
    return mqtt_send_pub("CtrlEcho",QOS0,msg,msg_len);
}

OSStatus mqtt_pub_ver_info( void )
{
    char data[256] = {0};
    mqtt_get_ver_info(data);
    return mqtt_pub_manage(data, strlen(data));
}

OSStatus  st_init_mqtt_devinfo(void)
{
    int j;
    OSStatus ret = kNoErr;
	unsigned char temp[128],hash[16]= {0};
	md5_context  md5;

	InitMd5(&md5);

	if (strcmp(stApp_config->netdev.deviceId,"")==0 )
	{
		ret = kGeneralErr;
		return ret ;
	}
	sprintf(temp,"%s%s%s",stApp_config->netdev.modelid,stApp_config->netdev.psk, stApp_config->netdev.deviceId);


	 Md5Update(&md5, temp, (uint32_t)strlen(temp)); //MQTT_CLIENT_ID
     Md5Final(&md5, hash);
     strcpy(st_client_username,"");
     sprintf(st_client_username, "%s", stApp_config->netdev.deviceId);
     mqtt_log("deviceId len is %d", strlen(stApp_config->netdev.deviceId));
     for (j = 3; j < 6; ++j)
     {
    	 sprintf(st_client_username,"%s%02X",st_client_username,hash[j]);
     }

     strcpy(st_client_passwd,st_client_username);

     sprintf(st_client_id,"GID_0_%s@@@%s",stApp_config->netdev.modelid,stApp_config->netdev.deviceId);
     sprintf(st_topic_pre,"%s/%s/%s/%s/",stApp_config->netdev.appid, CLOUD_PROTOCOL_VER,
    		 stApp_config->netdev.modelid,stApp_config->netdev.deviceId);

     sprintf(g_subtopic[0].topic,"%s%s",st_topic_pre,g_subtopic[0].topictail);
     sprintf(g_subtopic[1].topic,"%s%s",st_topic_pre,g_subtopic[1].topictail);
     sprintf(g_subtopic[2].topic,"%s%s",st_topic_pre,g_subtopic[2].topictail);

     if ((stApp_config->netdev.room_uid[0] != 0) && (stApp_config->netdev.room_uid[0] != (char)0xff))
     {
         sprintf(st_topic_room_pre,"%s/%s/%s/%s/",stApp_config->netdev.appid, CLOUD_PROTOCOL_VER,
                 ROOM_KEY, stApp_config->netdev.room_uid);
         sprintf(g_subtopic[3].topic,"%s%s",st_topic_room_pre,g_subtopic[3].topictail);
         mqtt_log("topic3: %s", g_subtopic[3].topic);
     }

//    printf(" %s  md5: %s \n",temp,st_client_username);
//     strcpy(st_client_username,"sub");
//     strcpy(st_client_passwd,"sub");
     mqtt_log("topic0:%s, topic1: %s, topic2: %s", g_subtopic[0].topic, g_subtopic[1].topic, g_subtopic[2].topic);
    mqtt_log("mqtt username: %s  topicpre: %s clientid[%s] ",st_client_username,st_topic_pre,st_client_id);

//	mqtt_log(" MQTT [%d] %s md5[%s]",  (uint32_t)strlen(temp),temp,hash);
	return ret;
}

void mqtt_subscribe_switch_room_ctrl( char *old_room_uid, char *new_room_uid )
{
//    int rc = -1;
    if (old_room_uid == NULL || new_room_uid == NULL)
    {
        return;
    }
    if (strcmp(old_room_uid, new_room_uid))
    {
        if (old_room_uid[0] != 0)       // unsubscribe the old room topic
        {
            MQTTUnsubscribe(&c, g_subtopic[3].topic);
            memset(st_topic_room_pre, 0, sizeof(st_topic_room_pre));
            memset(g_subtopic[3].topic, 0, sizeof(g_subtopic[3].topic));
        }
        if (new_room_uid[0] != 0)       // subscribe the new room topic
        {
            sprintf(st_topic_room_pre,"%s/%s/%s/%s/",stApp_config->netdev.appid, CLOUD_PROTOCOL_VER,
                    ROOM_KEY, new_room_uid);
            sprintf(g_subtopic[3].topic,"%s%s",st_topic_room_pre,g_subtopic[3].topictail);
            mqtt_log("topic3: %s", g_subtopic[3].topic);
            if (g_mqtt_conn)
            {
                MQTTSubscribe( &c, g_subtopic[3].topic, QOS0, messageArrived );
            }
        }
    }

}

/******************************************************
 *               Function Definitions
 ******************************************************/

static OSStatus mqtt_client_release( Client *c, Network *n)
{
    OSStatus err = kNoErr;

    if ( c->isconnected ) MQTTDisconnect( c );

    n->disconnect( n );  // close connection

    if ( MQTT_SUCCESS != MQTTClientDeinit( c ) ) {
        mqtt_log("MQTTClientDeinit failed!");
        err = kDeletedErr;
    }
    return err;
}

// publish msg to mqtt server
static OSStatus mqtt_msg_publish( Client *c, const char* topic, char qos, char retained,
                                  const unsigned char* msg,
                                  uint32_t msg_len )
{
    OSStatus err = kUnknownErr;
    int ret = 0;
    MQTTMessage publishData = MQTTMessage_publishData_initializer;

    require( topic && msg_len && msg, exit);

    // upload data qos0
    publishData.qos = (enum QoS) qos;
    publishData.retained = retained;
    publishData.payload = (void*) msg;
    publishData.payloadlen = msg_len;

    ret = MQTTPublish( c, topic, &publishData );

    if ( MQTT_SUCCESS == ret ) {
        err = kNoErr;
    } else if ( MQTT_SOCKET_ERR == ret ) {
        err = kConnectionErr;
    } else {
        err = kUnknownErr;
    }

exit:
    return err;
}

void mqtt_client_thread( mico_thread_arg_t arg )
{
    OSStatus err = kUnknownErr;

    int rc = -1;
    fd_set readfds;
    struct timeval t = { 0, MQTT_YIELD_TMIE * 1000 };

    ssl_opts ssl_settings;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    p_mqtt_send_msg_t p_send_msg = NULL;
    int msg_send_event_fd = -1;
    bool no_mqtt_msg_exchange = true;

    mqtt_log("MQTT client thread started...");

    memset( &c, 0, sizeof(c) );
    memset( &n, 0, sizeof(n) );

    /* create msg send queue event fd */
    msg_send_event_fd = mico_create_event_fd( mqtt_msg_send_queue );
    require_action( msg_send_event_fd >= 0, exit, mqtt_log("ERROR: create msg send queue event fd failed!!!") );

MQTT_start:
    /* 1. create network connection */
#ifdef MQTT_CLIENT_SSL_ENABLE
    ssl_settings.ssl_enable = true;
    ssl_settings.ssl_debug_enable = false;  // ssl debug log
    ssl_settings.ssl_version = TLS_V1_2_MODE;
    ssl_settings.ca_str_len = strlen(mqtt_server_ssl_cert_str);
    ssl_settings.ca_str = mqtt_server_ssl_cert_str;
#else
    ssl_settings.ssl_enable = false;
#endif

    while( 1 ){
        char cloudhost[64] = {0};
        uint8_t paramNum = 0;
        char *param[2] = {NULL, NULL};
        char *mqttServerIp = NULL;
        uint32_t mqttServerPort = MQTT_SERVER_PORT;
        memcpy(cloudhost, stApp_config->netdev.cloudhost, sizeof(cloudhost));
        mlink_parse_space_data(cloudhost, ':', param, &paramNum);
        mqttServerIp = param[0];

        if (paramNum == 2)
        {
            mqttServerPort = atoi(param[1]);
        }
        rc = NewNetwork( &n, mqttServerIp, mqttServerPort, ssl_settings );

        if( rc == MQTT_SUCCESS ) break;
        mqtt_log("ERROR: MQTT network connection err=%d, reconnect %s after 3s...", rc, stApp_config->netdev.cloudhost);
        mico_rtos_thread_sleep( 3 );
    }

//    mqtt_log("MQTT network connection success!");

    /* 2. init mqtt client */
    //c.heartbeat_retry_max = 2;
    rc = MQTTClientInit( &c, &n, MQTT_CMD_TIMEOUT );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client init err.");

//    mqtt_log("MQTT client init success!");

    /* 3. create mqtt client connection */
    rc = st_init_mqtt_devinfo();
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client init user param  err.");

    connectData.willFlag = 0;
    connectData.MQTTVersion = 4;  // 3: 3.1, 4: v3.1.1
    connectData.clientID.cstring =st_client_id;// MQTT_CLIENT_ID;
    connectData.username.cstring = st_client_username;//MQTT_CLIENT_USERNAME;
    connectData.password.cstring = st_client_passwd;//MQTT_CLIENT_PASSWORD;
    connectData.keepAliveInterval = MQTT_CLIENT_KEEPALIVE;
    connectData.cleansession = 1;

    rc = MQTTConnect( &c, &connectData );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client connect err.");

    mqtt_log("MQTT client connect success!");

    /* 4. mqtt client subscribe */

    rc = MQTTSubscribe( &c, g_subtopic[0].topic, QOS0, messageArrived );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
    rc = MQTTSubscribe( &c, g_subtopic[1].topic, QOS0, messageArrived );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
    rc = MQTTSubscribe( &c, g_subtopic[2].topic, QOS0, messageArrived );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
    if (g_subtopic[3].topic[0] != 0)
    {
        rc = MQTTSubscribe( &c, g_subtopic[3].topic, QOS0, messageArrived );
        require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
    }
//    mqtt_log("MQTT client subscribe success! ");

#if 0
    rc = MQTTSubscribe( &c, MQTT_CLIENT_SUB_TOPIC, QOS0, messageArrived );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
    mqtt_log("MQTT client subscribe success! recv_topic=[%s].", MQTT_CLIENT_SUB_TOPIC);

    rc = MQTTSubscribe( &c, MQTT_CLIENT_SUB_TOPIC1, QOS0, messageArrived );
    require_noerr_string(rc, MQTT_reconnect, "ERROR: MQTT client subscribe err.");
     mqtt_log("MQTT client subscribe success! recv_topic=[%s].", MQTT_CLIENT_SUB_TOPIC1);
#endif

    /* Flag it connecting success*/
     mqtt_mack_connect_state(TRUE);

     mqtt_pub_ver_info();
    /* 5. client loop for recv msg && keepalive */
    while ( 1 )
    {
        no_mqtt_msg_exchange = true;
        FD_ZERO( &readfds );
        FD_SET( c.ipstack->my_socket, &readfds );
        FD_SET( msg_send_event_fd, &readfds );
        select( msg_send_event_fd + 1, &readfds, NULL, NULL, &t );

        /* recv msg from server */
        if ( FD_ISSET( c.ipstack->my_socket, &readfds ) )
        {
            rc = MQTTYield( &c, (int) MQTT_YIELD_TMIE );
            require_noerr( rc, MQTT_reconnect );
            no_mqtt_msg_exchange = false;
        }

        /* recv msg from user worker thread to be sent to server */
        if ( FD_ISSET( msg_send_event_fd, &readfds ) )
        {
            while ( mico_rtos_is_queue_empty( &mqtt_msg_send_queue ) == false )
            {
                // get msg from send queue
                mico_rtos_pop_from_queue( &mqtt_msg_send_queue, &p_send_msg, 0 );
                require_string( p_send_msg, exit, "Wrong data point");

                // send message to server
                err = mqtt_msg_publish( &c, p_send_msg->topic, p_send_msg->qos, p_send_msg->retained,
                                        p_send_msg->data,
                                        p_send_msg->datalen );

                require_noerr_string( err, MQTT_reconnect, "ERROR: MQTT publish data err" );

//                mqtt_log("MQTT publish data success! send_topic=[%s], msg=[%ld][%s].\r\n", p_send_msg->topic, p_send_msg->datalen, p_send_msg->data);
                no_mqtt_msg_exchange = false;
                free( p_send_msg );
                p_send_msg = NULL;
            }
        }

        /* if no msg exchange, we need to check ping msg to keep alive. */
        if ( no_mqtt_msg_exchange )
        {
            rc = keepalive( &c );
            require_noerr_string( rc, MQTT_reconnect, "ERROR: keepalive err" );
        }
    }

MQTT_reconnect:
//    mqtt_log("Disconnect MQTT client, and reconnect after 5s, reason: mqtt_rc = %d, err = %d", rc, err );
    mqtt_mack_connect_state(FALSE);
    mqtt_client_release( &c, &n );
    mico_rtos_thread_sleep( 5 );
    goto MQTT_start;

exit:
    mqtt_log("EXIT: MQTT client exit with err = %d.", err);
    mqtt_client_release( &c, &n);
    mico_rtos_delete_thread( NULL );
}

// callback, msg received from mqtt server
static void messageArrived( MessageData* md )
{
    OSStatus err = kUnknownErr;
    p_mqtt_recv_msg_t p_recv_msg = NULL;
    MQTTMessage* message = md->message;
    /*
     app_log("MQTT messageArrived callback: topiclen=[%d][%s],\t payloadlen=[%d][%s]",
     md->topicName->lenstring.len,
     md->topicName->lenstring.data,
     (int)message->payloadlen,
     (char*)message->payload);
     */
    mqtt_log("\t\t\t\t\t======MQTT received callback ![%d]======", MicoGetMemoryInfo()->free_memory );

    p_recv_msg = (p_mqtt_recv_msg_t) calloc( 1, sizeof(mqtt_recv_msg_t) );
    require_action( p_recv_msg, exit, err = kNoMemoryErr );

    p_recv_msg->datalen = message->payloadlen;
    p_recv_msg->qos = (char) (message->qos);
    p_recv_msg->retained = message->retained;
    strncpy( p_recv_msg->topic, md->topicName->lenstring.data, md->topicName->lenstring.len );
    memcpy( p_recv_msg->data, message->payload, message->payloadlen );

    err = mico_rtos_send_asynchronous_event( &mqtt_client_worker_thread, user_recv_handler, p_recv_msg );
    require_noerr( err, exit );

exit:
    if ( err != kNoErr ) {
        app_log("ERROR: Recv data err = %d", err);
        if( p_recv_msg ) free( p_recv_msg );
    }
    return;
}


/* Application process MQTT received data */
OSStatus user_recv_handler( void *arg )
{
    OSStatus err = kUnknownErr;
    p_mqtt_recv_msg_t p_recv_msg = arg;
    require( p_recv_msg, exit );
    if (strcmp(p_recv_msg->topic,g_subtopic[0].topic)==0){
    	if (g_subtopic[0].handle)
    		g_subtopic[0].handle(p_recv_msg->data,p_recv_msg->datalen);

    }else if (strcmp(p_recv_msg->topic,g_subtopic[1].topic)==0)
    {
    	if (g_subtopic[1].handle)
    		g_subtopic[1].handle(p_recv_msg->data,p_recv_msg->datalen);
    }else if (strcmp(p_recv_msg->topic,g_subtopic[2].topic)==0)
    {
        if (g_subtopic[2].handle)
            g_subtopic[2].handle(p_recv_msg->data,p_recv_msg->datalen);
    }else if (strcmp(p_recv_msg->topic,g_subtopic[3].topic)==0)
    {
        if (g_subtopic[3].handle)
            g_subtopic[3].handle(p_recv_msg->data,p_recv_msg->datalen);
    }else
    mqtt_log("\t\t\t\t\tuser get data success! from_topic=[%s], msg=[%ld][%s].\r\n", p_recv_msg->topic, p_recv_msg->datalen, p_recv_msg->data);
    free( p_recv_msg );

exit:
    return err;
}


OSStatus MLinkStartMQTTClient( app_context_t * const inContext )
{
    OSStatus err = kNoErr;
#ifdef MQTT_CLIENT_SSL_ENABLE
    int mqtt_thread_stack_size = 0x3000;
#else
    int mqtt_thread_stack_size = 0xc00;     // 3k
#endif
    if (inContext == NULL)
    {
        mqtt_log("Param inContext is NULL !!!");
        return err;
    }
    stApp_config = (application_config_t*)inContext->appConfig;
    if (stApp_config == NULL)
    {
        mqtt_log("stApp_config is NULL");
        return err;
    }
    else
    {
        mqtt_log("stApp_config is not NULL");
    }

    mqtt_log("ERROR, mqtt temp : %s ", stApp_config->netdev.cloudhost);

    /* create mqtt msg send queue */
    err = mico_rtos_init_queue( &mqtt_msg_send_queue, "mqtt_msg_send_queue", sizeof(p_mqtt_send_msg_t),
                                MAX_MQTT_SEND_QUEUE_SIZE );

    if ( kNoErr != err ){
        mqtt_log("ERROR: create mqtt msg send queue err=%d.", err);
        return err;
    }

    /* start mqtt client */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "mqtt_client",
                                   (mico_thread_function_t)mqtt_client_thread, mqtt_thread_stack_size, 0 );
    if ( kNoErr != err ){
        mqtt_log("ERROR, mqtt thread exit err: %d", err);
        return err;
    }

    /* Create a worker thread for user handling MQTT data event  */
    err = mico_rtos_create_worker_thread( &mqtt_client_worker_thread, MICO_APPLICATION_PRIORITY, 0x1400, 5 );
    if ( kNoErr != err ){
        mqtt_log("ERROR, matt client worker thread exit err: %d", err);
        return err;
    }

    /* Trigger a period send event */
//    mico_rtos_register_timed_event( &mqtt_client_send_event, &mqtt_client_worker_thread, user_send_handler, 2000, NULL );

    return err;
}


