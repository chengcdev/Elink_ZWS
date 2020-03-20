/*
 * elink_logic_devchanel_deal.c
 *
 *  Created on: 2018年7月31日
 *      Author: hxfky
 */

#include "elink_logic_devchanel_deal.h"

#include "../../../mico-os/include/common.h"
#include "../../../mico-os/include/debug.h"
#include "../../../mico-os/include/mico_rtos.h"
#include "../../../mico-os/include/mico_socket.h"
#include "../../../mico-os/libraries/utilities/SocketUtils.h"
#include "../flash/flash_storage_object.h"
#include "../flash/storage_ble_logic.h"
#include "common/elink_comm.h"
#include "common/elink_packet.h"
#include "elink_logic.h"
#include "sntp.h"
#include "../ble/bluetooth_logic.h"
#define USE_MUTEX
#ifdef USE_MUTEX
#define MUTEX_LOCK(pMutex)  mico_rtos_lock_mutex(pMutex)
#define MUTEX_UNLOCK(pMutex)  mico_rtos_unlock_mutex(pMutex)
#else
#define MUTEX_LOCK(pMutex)
#define MUTEX_UNLOCK(pMutex)
#endif
static unsigned char tcp_remote_ip[16] = "192.168.12.108"; //"192.168.100.137";//"192.168.12.108"; /*remote ip address*/
static int tcp_remote_port = 9016; /*remote port*/
static mico_semaphore_t wait_sem = NULL;

static int g_tcpfd_devLogin;
static eLink_DevChannel_Param *g_DevChannelParam = NULL;
extern stELink_CommInfo *g_elink_comminfo;
extern stELink_DevInfo *g_elink_devinfo;
TCP_SUBDEV_RSP_HEAD_T g_elink_reslist;
BLEUSER_LISTRECORDLIST LockList[LOCK_LSIT_COUNT];
static uint8_t Locklistcout = 0;
static int heart_flag = 1;     //心跳开始发送标志
static QUEUE tcp_send_queue;
static mico_mutex_t g_elink_send_mutex;   // 发送互斥锁
static tcp_recv_deal_callback tcp_recieve_deal_cb = NULL;
static uint8_t elink_state = 0;
static uint8_t sub_online = 0;
static uint8_t sub_status = 0;
static ELINK_ALARM_STATE_E alarmstaus;
static uint8_t batterystatus;
static uint8_t opertionstate;
static uint8_t devbusy = 0;
static uint8_t tmpresFlag = 0;
static uint8_t errcode = 0;
static char subcode[10] = { 0 };
static uint32_t errCount = 0;
static uint32_t g_elink_online = 0;
#define BEEP
#ifdef BEEP
static mico_timer_t _beep_timer;
static bool _beep_timer_initialized = false;
#define SYS_BEEP_TRIGGER_INTERVAL 500
#endif
#define MAX_HEART_TIME      10
static uint32_t g_heattimeout = MAX_HEART_TIME;
static uint8_t g_heatflag = 0;
/********************************************************
 * function: elink_set_subonline_status
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_subonline_status( uint8_t onlie )
{
    sub_online = onlie;
    elink_log("sub_online : %d",sub_online);
}
/********************************************************
 * function: elink_get_subonline_status
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_subonline_status( void )
{
    elink_log("sub_online : %d",sub_online);
    return sub_online;
}

/********************************************************
 * function: elink_set_sub_status
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_status( uint8_t status )
{
    sub_status = status;
}
/********************************************************
 * function: elink_get_sub_status
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_sub_status( void )
{
    return sub_status;
}

/********************************************************
 * function: elink_set_sub_alarmstatus
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_alarmstatus( ELINK_ALARM_STATE_E status )
{
    alarmstaus = status;
}
/********************************************************
 * function: elink_get_sub_alarmstatus
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
ELINK_ALARM_STATE_E elink_get_sub_alarmstatus( void )
{
    return alarmstaus;
}

/********************************************************
 * function: elink_set_sub_batterystatus
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_batterystatus( uint8_t status )
{
    batterystatus = status;
}
/********************************************************
 * function: elink_get_sub_batterystatus
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_sub_batterystatus( void )
{
    return batterystatus;
}

/********************************************************
 * function: elink_set_sub_reslist
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_reslist( BLEUSER_LISTRECORDLIST *preslist, uint8_t listcout )
{
    if ( preslist == NULL )
    {
        Locklistcout = 0;
        memset( &LockList, 0, sizeof(BLEUSER_LISTRECORDLIST) * BLE_LIST_TMEPMAX_NUM );
        return;
    }
    uint8_t i;
    for ( i = 0; i < listcout; i++ )
    {

        LockList[i].usrid = preslist[i].usrid;
        LockList[i].keytype = preslist[i].keytype;
        LockList[i].mode = preslist[i].mode;

    }
    Locklistcout = listcout;
    elink_log("Locklistcout : %d",Locklistcout);

}
/********************************************************
 * function: elink_get_sub_reslist
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
BLEUSER_LISTRECORDLIST * elink_get_sub_reslist( void )
{
    return &LockList;
}
/********************************************************
 * function: elink_get_sub_reslist
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_sub_reslistcout( void )
{
    elink_log("Locklistcout : %d",Locklistcout);
    return Locklistcout;
}
/********************************************************
 * function: elink_get_sub_reslist
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
PTCP_SUBDEV_RSP_HEAD_T elink_get_sub_evenlist( void )
{

    return &g_elink_reslist;
}

/********************************************************
 * function: elink_set_sub_reslist
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_eventinfo( PTCP_SUBDEV_RSP_HEAD_T preslist )
{
    g_elink_reslist.userid = preslist->userid;
    g_elink_reslist.keytype = preslist->keytype;
    g_elink_reslist.time = preslist->time;

}

/********************************************************
 * function: elink_set_sub_operstatus
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_operstatus( uint8_t status )
{
    opertionstate = status;
}
/********************************************************
 * function: elink_get_sub_operstatus
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_sub_operstatus( void )
{
    return opertionstate;
}

/********************************************************
 * function: elink_set_sub_bind
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_bind( int bind )
{
    g_DevChannelParam->loginBind = bind;
}
/********************************************************
 * function: elink_get_sub_bind
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int elink_get_sub_bind( void )
{
    return g_DevChannelParam->loginBind;
}

/********************************************************
 * function: elink_set_sub_code
 * description:
 * input:           state_type:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_sub_code( char * code )
{

    memset( subcode, 0, sizeof(subcode) );
    strcpy( subcode, code );

}
/********************************************************
 * function: elink_get_sub_code
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
char * elink_get_sub_code( void )
{
    return (char *) &subcode;

}
/********************************************************
 * function: elink_get_join_net_status
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int elink_get_join_net_status( void )
{
    stBLELockStatus lockstate;
    stor_lockstatue_read( &lockstate );
    elink_log("lockstate.isOperSuccess : %d",lockstate.isOperSuccess);
    return lockstate.isOperSuccess;
}

/********************************************************
 * function: elink_set_join_net_status
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
int elink_set_join_net_status( unsigned char status )
{
    stBLELockStatus lockstate;
    lockstate.isOperSuccess = status;
    stor_lockstatue_write( OBJECT_UPDATE_MODIFY, 1, &lockstate );

}

/********************************************************
 * function: elink_get_dev_busystate
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_dev_busystate( void )
{

    return devbusy;
}

/********************************************************
 * function: elink_set_dev_busystate
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_dev_busystate( uint8_t status )
{
    devbusy = status;
}

/********************************************************
 * function: elink_get_dev_restmpFlag
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_dev_restmpFlag( void )
{

    return tmpresFlag;
}

/********************************************************
 * function: elink_set_dev_restmpFlag
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_dev_restmpFlag( uint8_t status )
{
    tmpresFlag = status;
}
/********************************************************
 * function: elink_get_dev_errstrcode
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
uint8_t elink_get_dev_errstrcode( void )
{

    return errcode;
}

/********************************************************
 * function: elink_set_dev_errstrcode
 * description:
 * input:           status:
 * output:          state:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_set_dev_errstrcode( ELINK_RETURN_E status )
{
    errcode = status;
    elink_log("errcode : %d",errcode);
}

/*****子设备设置subdevid*****/
void set_subdevid( char * devid )
{
    sublock_deviceid = devid;
}

/***子设备获取subdevid****/
char * get_subdevid( void )
{
    return (unsigned char *) sublock_deviceid;
}
/***主设备获取devid****/
char * get_devid(void)
{
    return (unsigned char *) g_elink_devid;
}

/*************子设备pin*********/
char * get_subpin( void )
{
    return (unsigned char *) lock_pin;
}

int get_loginAuthInterval( void )
{
    return g_elink_comminfo->authSucess;
}
int set_heatstatue(uint8_t flag)
{
    g_heatflag = flag ;
}
/********************************************************
 * function:  获取是否连接平台状态
 * description:  get_g_elink_online_state
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
 *********************************************************/
uint32_t get_g_elink_online_state(void)
{
    return g_elink_online;
}
/********************************************************
 * function:  设置是否连接平台状态
 * description:  set_g_elink_online_state
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
 *********************************************************/
void set_g_elink_online_state(uint32_t state)
{
     g_elink_online = state;
}
void set_loginFlag(uint8_t flag)
{
    g_DevChannelParam->loginFlag = flag;
}
/********************************************************
 * function:  tcp_send_queue_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
 *********************************************************/
OSStatus tcp_send_queue_init( QUEUE *queue )
{
    return init_queue( queue, sizeof(TCP_FRAME_DATA_T) );
}

/*************************************************
 Function:     tcp_push_node
 Description:  网络发送入队
 Input:
 queue       队列指针
 data        欲入队的数据指针
 Output:
 Return:
 成功      AU_SUCCESS
 失败      AU_FAILURE
 *************************************************/
int tcp_push_node( QUEUE *queue, PTCP_FRAME_DATA_T tcp_data )
{
    int ret = 0;
    ret = push_node( queue, (void*) tcp_data );
    return ret;
}

/*************************************************
 Function:     tcp_pop_node
 Description:  网络发送数据出队
 Input:
 queue       队列指针
 Output:
 Return:
 成功      数据的指针
 失败      NULL
 *************************************************/
PTCP_FRAME_DATA_T tcp_pop_node( QUEUE *queue )
{
    return (PTCP_FRAME_DATA_T) pop_node( queue );
}
/********************************************************
 * function: elink_send_sys_mutex_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus elink_send_sys_mutex_init( void )
{
    mico_rtos_init_mutex(&g_elink_send_mutex);
}
void _get_host_devLogin( unsigned char *host, unsigned char * addr, int * port )
{
//	char delim = ':';
    if ( host == NULL || addr == NULL || port == NULL )
        return;

    unsigned char *q;
    int len;
    int tempPort = 0;
    q = host;
    len = strlen( host );
    while ( len && *q != ':' )
    {
        ++q;
        --len;
    }

    if ( len != 0 )
    {
        memcpy( addr, host, q - host );
        *(addr + (q - host)) = '\0';
    }
    ++q, --len;
    while ( len && *q != '\0' && isdigit( *q ) )
    {
        tempPort = tempPort * 10 + *q - '0';
        ++q;
        --len;
    }
    *port = tempPort;

}

static int st_connect_tcp_server( unsigned char *tcphost, int port )
{
    elink_log("tcphost :%s port : %d ",tcphost,port);
    struct sockaddr_in addr;
    OSStatus err;
    int tcp_fd = -1;
    tcp_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( IsValidSocket( tcp_fd ) < 0 )
        return -1;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr( tcphost );
    addr.sin_port = htons( port );
    err = connect( tcp_fd, (struct sockaddr *) &addr, sizeof(addr) );
    if ( err != kNoErr )
    {
        elink_log( "Connected to TCP server: ip=%s  port=%d  error !", tcphost,port );

        return -1;
    }
    elink_log( "Connected to TCP server: ip=%s  port=%d  success !", tcphost,port );

    return tcp_fd;
}
#define TCP_REV_BUFFER_LEN_MAX	1024
/**
 * 登录服务处理
 */
static int st_login_tcphost_deal( char *revBuf )
{
    int ret = -1;
    int len;

    TCP_FRAME_DATA_T sendData = { 0 };
    OSStatus err;
    //2. 发送登录数据
    if ( g_DevChannelParam->loginFlag == 1 )
        return 0;
    //1.连接服务器
    if ( g_DevChannelParam->loginTcpFd < 0 )
    {
        _get_host_devLogin( (unsigned char *) ELink_LOGIN_TCPHOST, tcp_remote_ip,
                            &tcp_remote_port );
        if(tcp_remote_port == 0)
        {
            goto exit;
        }
        g_DevChannelParam->loginTcpFd = st_connect_tcp_server( tcp_remote_ip, tcp_remote_port );
        if ( g_DevChannelParam->loginTcpFd < 0 )
            return -1;
        g_DevChannelParam->loginlasttime = time( 0 );
    }

    ret = 0;
    int interval = 0;
    if ( g_DevChannelParam->loginFailCount > 3 )
    {
        interval = g_DevChannelParam->loginAuthTimeoutInterval;
        goto exit;
    }
    else
    {
        interval = g_DevChannelParam->loginAuthInterval;
    }
    elink_log("time> : %d  interval : %d",time( 0 ) - g_DevChannelParam->loginlasttime,interval);
//    if ( time( 0 ) - g_DevChannelParam->loginlasttime > interval
//         || g_DevChannelParam->loginlasttime == 0 )
   if(1)
    {
       mico_rtos_thread_sleep(1);

        MUTEX_LOCK(&g_elink_send_mutex);
        char *buf = NULL;
        buf = (char *) elink_get_req_pack_dev_login( g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        len = strlen( buf );
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        memcpy( sendData.data, buf, len );
        sendData.TcpFd = g_DevChannelParam->loginTcpFd;
        sendData.dataSize = len;
        tcp_push_node( &tcp_send_queue, &sendData );
        g_DevChannelParam->loginFailCount++;
        elink_log("g_DevChannelParam->loginFailCount : %d",g_DevChannelParam->loginFailCount);
        g_DevChannelParam->loginlasttime = time( 0 );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    //接收TCP 数据
    struct timeval t;
    fd_set readfds;
    t.tv_sec = 1;
    t.tv_usec = 0;
    FD_ZERO( &readfds );
    FD_SET( g_DevChannelParam->loginTcpFd, &readfds );

    memset( revBuf, 0, TCP_REV_BUFFER_LEN_MAX );
    require_action( select( g_DevChannelParam->loginTcpFd + 1, &readfds, NULL, NULL, &t ) >= 0,
                    exit,
                    err = kConnectionErr );
    /* recv wlan data, and send back */
    if ( FD_ISSET( g_DevChannelParam->loginTcpFd, &readfds ) )
    {
        len = recv( g_DevChannelParam->loginTcpFd, revBuf, TCP_REV_BUFFER_LEN_MAX, 0 );
        require_action( len >= 0, exit, err = kConnectionErr );

        if ( len == 0 )
        {
            elink_log( "TCP Login Client is disconnected, fd: %d", g_DevChannelParam->loginTcpFd );
            goto exit;
        }
//        elink_log("rev tcp login server data[%d]: %s ",len,revBuf);
        int result;
        result = elink_parse_unpack( g_DevChannelParam->sequence, revBuf, len );
        elink_log("result : %d",result);
        if ( result == 0 )
        {
            errCount = 0;
            g_DevChannelParam->loginFlag = 1;
            g_DevChannelParam->loginFailCount = 0;
            g_elink_comminfo->devConnectFlag = 0;
            g_elink_comminfo->devConnectFd = -1;

        }
        else
        {
            errCount++;
            if ( errCount % 10 == 0 )
            {
                errCount = 0;
                goto exit;
            }

        }
    }
    return ret;
    exit:
    SocketClose( &g_DevChannelParam->loginTcpFd );
    g_DevChannelParam->loginTcpFd = -1;
    g_DevChannelParam->loginFlag = 0;
//    free(revBuf);
//    revBuf = NULL;
    elink_log("g_DevChannelParam->loginTcpFd : %d g_DevChannelParam->loginFlag: %d",g_DevChannelParam->loginTcpFd,g_DevChannelParam->loginFlag);
    return -1;
}

/***
 * 设备连接处理
 */
static int st_dev_connect_deal( char *revBuf )
{
    int ret = 0;
    int len;

    char strcode[10] = { 0 };
    OSStatus err;
    TCP_FRAME_DATA_T sendData = { 0 };

    if ( g_elink_comminfo->devConnectFd < 0 )
    {
        //连接TCPhost
        unsigned char host[32];
        int port;
        _get_host_devLogin( g_elink_comminfo->tcpHost, host, &port );

        g_elink_comminfo->devConnectFd = st_connect_tcp_server( host, port );
        if ( g_elink_comminfo->devConnectFd < 0 )
            return -1;

    }

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 )
    {
        //发送请求心跳包


        g_elink_comminfo->temptime++;
        heart_flag = (g_elink_comminfo->temptime - g_elink_comminfo->loginlasttime) % g_elink_comminfo->heartBeat;
//        elink_log("timetmp [%d] g_elink_comminfo->loginlasttime[%d] g_elink_comminfo->heartBeat[%d]",\
//                  g_elink_comminfo->temptime,g_elink_comminfo->loginlasttime,g_elink_comminfo->heartBeat);
        if ( heart_flag == 0  )
        {
            mico_rtos_thread_sleep(1);
            MUTEX_LOCK(&g_elink_send_mutex);
            char *buf = NULL;
            g_heatflag = 1;
            elink_log("st_dev_connect_deal devConnectFlag %d ",g_elink_comminfo->devConnectFlag);
            buf = (char *) elink_get_req_pack_dev_heartBeat( g_elink_comminfo->token,
                                                             g_DevChannelParam->sequence );
            if(buf == NULL)
            {
                MUTEX_UNLOCK(&g_elink_send_mutex);
                return -1;
            }
            memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
            len = strlen( buf );
            memcpy( sendData.data, buf, len );
            sendData.dataSize = len;
            sendData.TcpFd = tcp_fd;
            tcp_push_node( &tcp_send_queue, &sendData );
            if(buf)
            {
                free( buf );
                buf = NULL;
            }

            MUTEX_UNLOCK(&g_elink_send_mutex);
        }
    } else
    {
        //发送连接包
        mico_rtos_thread_sleep(1);
       MUTEX_LOCK(&g_elink_send_mutex);
       char *buf = NULL;
        buf = (char *) elink_get_req_pack_dev_connect( g_elink_comminfo->token,
                                                       g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
//		elink_log("sendData.data : %s ",sendData.data);
        elink_log("g_DevChannelParam->loginTcpFd : %d",g_DevChannelParam->loginTcpFd);
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }

        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    //接收TCP 数据

    struct timeval t;
    fd_set readfds;
    t.tv_sec = 1;
    t.tv_usec = 0;
    FD_ZERO( &readfds );
    FD_SET( tcp_fd, &readfds );

    memset( revBuf, 0, TCP_REV_BUFFER_LEN_MAX );
    require_action( select( tcp_fd + 1, &readfds, NULL, NULL, &t ) >= 0, exit,
                    err = kConnectionErr );
    /* recv  data, and send back */
    if ( FD_ISSET( tcp_fd, &readfds ) )
    {
        len = recv( tcp_fd, revBuf, TCP_REV_BUFFER_LEN_MAX, 0 );
        require_action( len >= 0, exit, err = kConnectionErr );


        elink_log("st_dev_connect_deal[%d]: %s ",len,revBuf);
        if(len == 0)
        {
            return 0;
        }
        int result = elink_parse_unpack( g_DevChannelParam->sequence, revBuf, len );

        elink_log("result : %d",result);
        g_elink_online ++;
        set_g_elink_online_state(g_elink_online);
        if ( result == 0 )
        {
//        	g_elink_comminfo->devConnectFlag=1;
//        	g_elink_comminfo->loginlasttime =time(0);
            errCount = 0;
            elink_set_dev_errstrcode( ELINK_RETURN_OK );
            elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );

        }
        else
        {
            errCount++;
            if ( errCount % 5 == 0 )
            {
                errCount = 0;
                goto exit;
            }
            elink_set_dev_busystate( 1 );
            elink_sub_ctrlechoreport( elink_get_sub_code( ), elink_get_dev_errstrcode( ) );

        }
    }
    return ret;
    exit:
    SocketClose( &tcp_fd );
    g_elink_comminfo->devConnectFd = -1;
//    g_elink_comminfo->devConnectFlag = 0;
    g_DevChannelParam->loginFlag = 0;
//    g_DevChannelParam->loginTcpFd = -1;
//    free(revBuf);
//    revBuf = NULL;
	elink_log("g_elink_comminfo->devConnectFd : %d",g_elink_comminfo->devConnectFd);
	return -1;
}
/*when client connected wlan success,create socket*/

/***
 * 子设备鉴权处理
 */
static int st_subdev_authority_deal( char *revBuf, char *subdevid, char *pin )
{

    int ret = 0;
    int len;
    char *buf = NULL;
    uint8_t get_state = 0;
    TCP_FRAME_DATA_T sendData = { 0 };
    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 )
    {
        //子设备鉴权发送
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_auth( g_elink_comminfo->token, subdevid, pin,
                                                   g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            elink_log("st_subdev_authority_deal err!");
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备在线上报
 */
static int st_subdev_online_deal( char *revBuf, char *subdevid, uint8_t online )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备在线状态上报
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_OnLineStatusReport( g_elink_comminfo->token, subdevid,
                                                                 online,
                                                                 g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备状态上报
 */
static int st_subdev_status_deal( char *revBuf, char *subdevid, uint8_t status, uint8_t num )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备状态上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_StatusReport( g_elink_comminfo->token, subdevid,
                                                           status,
                                                           num,
                                                           g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备信号强度上报
 */
static int st_subdev_signal_deal( char *revBuf, char *subdevid, uint8_t status, uint8_t num )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备状态上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_SignalReport( g_elink_comminfo->token, subdevid,
                                                           status,
                                                           num,
                                                           g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备拆除状态上报
 */
static int st_subdev_demolish_deal( char *revBuf, char *subdevid, uint8_t status, uint8_t num )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备状态上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_DemolishReport( g_elink_comminfo->token, subdevid,
                                                             status,
                                                             num,
                                                             g_DevChannelParam->sequence );
        if(buf == NULL)
        {
           MUTEX_UNLOCK(&g_elink_send_mutex);
           return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备资源上报
 */
static int st_subdev_res_deal( char *revBuf, char *subdevid,
                               BLEUSER_LISTRECORDLIST *g_elink_subeventinfo,
                               uint8_t num ,uint8_t status)
{
    int ret = 0;
    int len;
    char *buf = NULL;
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备资源上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_ResReport( g_elink_comminfo->token, subdevid,
                                                        g_elink_subeventinfo,
                                                        num,
                                                        g_DevChannelParam->sequence ,status);
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        len = strlen( buf );
        len = send( tcp_fd, buf, len, 0 );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;

}
/***
 * 子设备资源变化上报处理
 */
static int st_subdev_reschange_deal( char *revBuf, char *subdevid,
                                     BLEUSER_LISTRECORDLIST *g_elink_subeventinfo,
                                     uint8_t num )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备资源上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_ResChangeReport( g_elink_comminfo->token, subdevid,
                                                              g_elink_subeventinfo,
                                                              num,
                                                              g_DevChannelParam->sequence );
        if(buf == NULL)
        {
           MUTEX_UNLOCK(&g_elink_send_mutex);
           return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        elink_log("sendData.dataSize : %d",len);
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备临时资源上报
 */
static int st_subdev_reschangetmp_deal( char *revBuf, char *subdevid,
                                  BLEUSER_LISTRECORDLIST *g_elink_subeventinfo,
                                  uint8_t num,
                                  uint8_t restmpflag )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备资源上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_ResTmpChangeReport( g_elink_comminfo->token, subdevid,
                                                           g_elink_subeventinfo,
                                                           num, restmpflag,
                                                           g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            elink_log("ResTmpChangeReport error !");
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        len = strlen( buf );
        len = send( tcp_fd, buf, len, 0 );
        elink_log("tcp_fd : %d len : %d",tcp_fd,len);
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;

}
/***
 * 子设备临时资源属性上报
 */
static int st_subdev_restmpreport_deal( char *revBuf, char *subdevid,
                                  BLEUSER_LISTRECORDLIST *g_elink_subeventinfo,
                                  uint8_t num,
                                  uint8_t restmpflag,uint8_t status )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备资源上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_ResTmpReport( g_elink_comminfo->token, subdevid,
                                                           g_elink_subeventinfo,
                                                           num, restmpflag,
                                                           g_DevChannelParam->sequence,status );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;

}

/***
 * 子设备告警上报
 */
static int st_subdev_alarm_deal( char *revBuf, char *subdevid, ELINK_ALARM_STATE_E alarmstate,
                                 uint8_t batterystatus,
                                 char num )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    elink_log("st_dev_connect_deal devConnectFlag %d ",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备告警上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        if(alarmstate == ELINK_ALARM_BATTERY_WARN_STATE)
        {
            buf = (char *) elink_get_pack_subdev_AlarmReport( g_elink_comminfo->token, subdevid,
                                                              alarmstate,
                                                              batterystatus,num,
                                                              g_DevChannelParam->sequence);
        }
        else
        {
            buf = (char *) elink_get_pack_subdev_AtkReport( g_elink_comminfo->token, subdevid,
                                                                          alarmstate,
                                                                          batterystatus,num,
                                                                          g_DevChannelParam->sequence);
        }

        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}

/***
 * 子设备开锁信息上报
 */
static int st_subdev_eventinfo_deal( char *revBuf, char *subdevid,
                                     PTCP_SUBDEV_RSP_HEAD_T g_elink_subeventinfo )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };

    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 && g_elink_comminfo->authSucess == 1 )
    {
        //子设备告警上报处理
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_pack_subdev_EventReport( g_elink_comminfo->token, subdevid,
                                                          g_elink_subeventinfo,
                                                          g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    return ret;
}


/***
 * 网关心跳请求上报
 */
static int st_gw_heart_deal( )
{
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };

    elink_log("st_dev_connect_deal devConnectFlag %d",g_elink_comminfo->devConnectFlag);

    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( g_elink_comminfo->devConnectFlag == 1 )
    {
        MUTEX_LOCK(&g_elink_send_mutex);
        buf = (char *) elink_get_req_pack_dev_heartBeat( g_elink_comminfo->token,
                                                         g_DevChannelParam->sequence );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return -1;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
}
/*************************************************
 Function:     elink_sub_ctrlechoreport
 Description:  子设备控制应答处理
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_ctrlechoreport( char *code, ELINK_RETURN_E strcode )
{
    if ( code == NULL )
    {
        return;
    }
    int ret = 0;
    int len;
    char *buf = NULL;
    TCP_FRAME_DATA_T sendData = { 0 };
    int tcp_fd = g_elink_comminfo->devConnectFd;
    if ( !strcmp( code, ELink_Code_Ctrl ) )
    {
        //发送子设备控制应答
        MUTEX_LOCK(&g_elink_send_mutex);
        sprintf( code, "%s", ELink_Code_Ctrl_echo );
        buf = (char *) elink_echo_pack_dev_ctrl( g_elink_comminfo->token,
                                                 g_DevChannelParam->sequence,
                                                 get_devid( ), code,
                                                 strcode );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return ;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
//      elink_log("sendData.data : %s ",sendData.data);
        elink_log("g_DevChannelParam->loginTcpFd : %d",sendData.TcpFd);
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    else if ( !strcmp( code, ELink_Code_StatusQuery ) )
    {
        //发送子设备查询状态应答
        MUTEX_LOCK(&g_elink_send_mutex);
        sprintf( code, "%s", ELink_Code_StatusQuery_echo );

        if(strcmp( get_devid, g_elink_devinfo->devid ) == 0 )  //网关状态
        {
            buf = (char *) elink_echo_pack_dev_ctrl( g_elink_comminfo->token,
                                                             g_DevChannelParam->sequence,
                                                             get_devid( ), code,
                                                             strcode );
        }
        else
        {
            buf = (char *) elink_echo_pack_dev_ctrl( g_elink_comminfo->token,
                                                             g_DevChannelParam->sequence,
                                                             get_devid( ), code,
                                                             strcode );
        }
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return ;
        }
        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
//      elink_log("sendData.data : %s ",sendData.data);
        elink_log("g_DevChannelParam->loginTcpFd : %d",sendData.TcpFd);
        tcp_push_node( &tcp_send_queue, &sendData );
        if(buf)
        {
            free( buf );
            buf = NULL;
        }
        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    else if ( !strcmp( code, ELink_Code_DevResMangerReq ) )
    {
        //发送子设备添加资源
        MUTEX_LOCK(&g_elink_send_mutex);
        sprintf( code, "%s", ELink_Code_DevResMangerReq_echo );
        buf = (char *) elink_echo_pack_dev_ctrl( g_elink_comminfo->token,
                                                 g_DevChannelParam->sequence,
                                                 get_devid( ), code,
                                                 strcode );
        if(buf == NULL)
        {
            MUTEX_UNLOCK(&g_elink_send_mutex);
            return ;
        }

        memset( &sendData, 0, sizeof(TCP_FRAME_DATA_T) );
        len = strlen( buf );
        memcpy( sendData.data, buf, len );
        sendData.dataSize = len;
        sendData.TcpFd = tcp_fd;
        tcp_push_node( &tcp_send_queue, &sendData );

        if(buf)
        {
            free( buf );
            buf = NULL;
        }

        elink_set_dev_busystate( 0 );
        MUTEX_UNLOCK(&g_elink_send_mutex);
    }
    else
    {
        if(buf)
        {
           free( buf );
           buf = NULL;
        }
    }
}
/*************************************************
 Function:     elink_sub_resreport
 Description:  子设备资源上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_resreport( void )
{
   st_subdev_res_deal( NULL, get_devid( ), elink_get_sub_reslist( ),
                              elink_get_sub_reslistcout( ),elink_get_sub_status( ) );
}

/*************************************************
 Function:     elink_sub_reschangereport
 Description:  子设备资源变化上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_reschangereport( void )
{
    st_subdev_reschange_deal( NULL, get_devid( ), elink_get_sub_reslist( ),
                                    elink_get_sub_reslistcout( ) );
}
/*************************************************
 Function:     elink_sub_restmpchangereport
 Description:  子设备临时资源变化上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_restmpchangereport( void )
{
    st_subdev_reschangetmp_deal( NULL, get_devid( ), elink_get_sub_reslist( ),
                                 elink_get_sub_reslistcout( ),
                                 elink_get_dev_restmpFlag( ) );
}
/*************************************************
 Function:     elink_sub_restmpreport
 Description:  子设备临时资源属性上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_restmpreport( void )
{
    st_subdev_restmpreport_deal( NULL, get_devid( ), elink_get_sub_reslist( ),
                                 elink_get_sub_reslistcout( ),
                                 elink_get_dev_restmpFlag( ),elink_get_sub_status( ) );
}
/*************************************************
 Function:     elink_sub_statusreport
 Description:  子设备状态上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_statusreport( void )
{
    st_subdev_status_deal( NULL, get_devid( ), elink_get_sub_status( ), 1 );
}

/*************************************************
 Function:     elink_sub_signalreport
 Description:  子设备信号强度上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_signalreport( void )
{
    st_subdev_signal_deal( NULL, get_devid( ), elink_get_sub_status( ), 1 );
}
/*************************************************
 Function:     elink_sub_demolishreport
 Description:  子设备拆除状态上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_demolishreport( void )
{
    st_subdev_demolish_deal( NULL, get_devid( ), elink_get_sub_status( ), 1 );
}


/*************************************************
 Function:     elink_sub_alarmreport
 Description:  子设备告警上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_alarmreport( void )
{

    st_subdev_alarm_deal( NULL, get_devid( ), elink_get_sub_alarmstatus( ),
                                elink_get_sub_batterystatus( ),
                                1 );

}
/*************************************************
 Function:     elink_sub_eventreport
 Description:  子设备信息上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_sub_eventreport( void )
{
    uint8_t ret = 0;
    ret = st_subdev_eventinfo_deal( NULL, get_devid( ), elink_get_sub_evenlist( ) );
}
/*************************************************
 Function:     elink_gw_heartreport
 Description:  网关心跳上报
 Input:
 queue
 Output:
 Return:
 *************************************************/
void elink_gw_heartreport( void )
{
    uint8_t ret = 0;
    g_heattimeout = MAX_HEART_TIME;
    g_heatflag = 1;
    ret = st_gw_heart_deal( );
}


void tcp_client_thread( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    OSStatus err;
    int tcp_fd = -1;
    char *buf = NULL;
    buf = (char*) malloc( TCP_REV_BUFFER_LEN_MAX );
    require_action( buf, exit, err = kNoMemoryErr );

    while ( 1 )
    {
        //1.连接登录服务器
        if(get_sntp() ==  1 && ble_init_finish() == 1)
        {
            if(g_heatflag == 1)
            {
                 g_heattimeout--;
                 if(g_heattimeout == 0)
                 {
                     g_elink_comminfo->devConnectFd = -1;
                     g_DevChannelParam->loginFlag = 0;
                 }
            }

            if ( st_login_tcphost_deal( buf ) < 0 )
            {
                mico_rtos_thread_sleep( 3 );
                continue;
            }

            if ( g_DevChannelParam->loginFlag != 1 )
                    continue;

            if ( g_DevChannelParam->loginTcpFd >= 0 )
            {
                //1.1关闭登录
                elink_log( "=====1.1 close tcp dev login server connected ==== " );
                SocketClose( &g_DevChannelParam->loginTcpFd );
                g_DevChannelParam->loginTcpFd = -1;
            }

            //2.1 发起设备连接到登录返回的TCP host 业务服务器
            if ( st_dev_connect_deal( buf ) != 0 )
            {
                mico_rtos_thread_sleep( 3 );
                continue;
            }


        }

        msleep( 100 );
    }
    exit:
    if ( err != kNoErr ) elink_log( "TCP client thread exit with err: %d", err );
    if ( buf != NULL ) free( buf );
    SocketClose( &tcp_fd );
    mico_rtos_delete_thread( NULL );
}


/********************************************************
 * function:  tcp_send_thread
 * description:  tcp send data thread
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
 *********************************************************/

void tcp_send_thread( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    int len = 0;
    PTCP_FRAME_DATA_T psendData = NULL;

    while ( 1 )
    {
        if ( psendData != NULL )
        {
            free( psendData );
            psendData = NULL;
        }

        psendData = tcp_pop_node( &tcp_send_queue );

        if ( psendData == NULL )
        {
            msleep( 50 );
            continue;

        }

        if ( psendData != NULL )
        {
            len = send( psendData->TcpFd, psendData->data, psendData->dataSize, 0 );
            if(len <= 0)
            {
               SocketClose( &psendData->TcpFd );
               g_elink_comminfo->devConnectFd = -1;
               g_DevChannelParam->loginFlag = 0;
            }
        }

        if(psendData)
        {
            free( psendData );
            psendData = NULL;
        }
    }
}

OSStatus elink_logic_init_devchannel( )
{
    OSStatus err = kNoErr;
    g_DevChannelParam = malloc( sizeof(eLink_DevChannel_Param) );
    g_DevChannelParam->loginFailCount = 0;
    g_DevChannelParam->loginAuthInterval = 30;     //60;
    g_DevChannelParam->loginAuthTimeoutInterval = 30*10;     //60*10;
    g_DevChannelParam->loginFlag = 0;
    g_DevChannelParam->loginTcpFd = -1;

    /*****初始化发送队列****/
    tcp_send_queue_init( &tcp_send_queue );
    elink_send_sys_mutex_init();
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP_client_devLogin",
                                   tcp_client_thread,
                                   0x1500, 0 );

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "TCP_client_send",
                                   tcp_send_thread,
                                   0x800, 0 );

    return err;

}

