/*
 * elink_protocol_chanel.c
 *
 *  Created on: 2018年7月30日
 *      Author: hxfky
 */

#include "../../../mico-os/include/common.h"
#include "../../../mico-os/include/mico_security.h"
#include "../../../mico-os/include/MiCODrivers/MiCODriverRtc.h"
#include "../../../mico-os/libraries/utilities/json_c/json_object.h"
#include "../../../mico-os/libraries/utilities/json_c/json_object_private.h"
#include "../../../mico-os/libraries/utilities/json_c/json_tokener.h"
#include "../../../mico-os/libraries/utilities/json_c/linkhash.h"
#include "../ble/bluetooth_logic.h"
#include "../flash/storage_ble_logic.h"
#include "../time/time.h"
#include "common/elink_comm.h"
#include "common/elink_crypto.h"
#include "common/elink_packet.h"
#include "elink_logic_devchanel_deal.h"
#include "mico.h"

static char Ctei_buf[16] = {0};
static char mac[13] ={0};
static char  ip[16] = {0};
static char uplinkmac[18] = {0};

extern unsigned char g_elink_devid[48];
extern unsigned char g_elink_devPin[48];
extern stELink_CommInfo *g_elink_comminfo;
extern stELink_DevInfo *g_elink_devinfo;
static char g_Aes_key[17];
static  char g_Aes_IV[17];
extern TCP_SUBDEV_RSP_HEAD_T g_elink_reslist;

static unsigned char elink_strcode[10] = { 0 };

#define IMEI_CTEI        "180004049999907"

static int st_get_login_sequence( void )
{
    int randcount = 0;
    srand( (unsigned) time( 0 ) );
    randcount = rand( );
    randcount = randcount % 65535;
    return randcount;
}

////////////////////////////////////// 以下业务包处理接口///////////////////////////////////
static unsigned int g_currSequence = -1;

static unsigned int st_get_req_sequence( void )
{
    if ( g_currSequence++ > 65535 )
        g_currSequence = 0;
    return g_currSequence;
}
/**
 *
 * @return output data
 */
char *elink_get_dev_key( void )
{
    return g_Aes_key;
}

/**
 *
 * @return output data
 */
char *elink_get_dev_iv( void )
{
    return g_Aes_IV;
}

/**
 *
 * @param 需要转换的小写字符串
 * @param 字符串长度
 */
static void upper(char *s, int length)
{
    int i = 0;

    for (i = 0; i < length; i++) {

        if (s[i] >= 'a' && s[i] <= 'z') {

            s[i] -= 'a' - 'A';

        }

    }
}

/**
 *  获取CTEI号
 * @return
 */
static char *get_ctei(void)
{

    memset( Ctei_buf, 0, sizeof(Ctei_buf) );
//    storage_read_wifictmi_obj( &Ctei_buf );
    if ( Ctei_buf[0] != 0xffffffff || Ctei_buf[1] != 0xffffffff )
    {
        sprintf(Ctei_buf,"%s",IMEI_CTEI);
        return Ctei_buf;
    }
    else
    {
        sprintf(Ctei_buf,"%s",IMEI_CTEI);
        return Ctei_buf;
    }

}

/**
 *  获取本机MAC
 * @return
 */
static char *get_mac(void)
{
    net_para_st para;

    memset(mac,0,sizeof(mac));
    micoWlanGetIPStatus(&para, Station);
    sprintf(mac, "%0c%0c%0c%0c%0c%0c%0c%0c%0c%0c%0c%0c", para.mac[0],para.mac[1],\
            para.mac[2],para.mac[3],para.mac[4],para.mac[5],para.mac[6],para.mac[7],\
            para.mac[8],para.mac[9],para.mac[10],para.mac[11]);
    upper(mac,strlen(mac));
//    elink_log("mac : %s",mac);
    return mac;
}

/**
 *  获取本机IP
 * @return
 */
static char *get_ip(void)
{
    net_para_st para;

    memset(ip,0,sizeof(ip));
    micoWlanGetIPStatus(&para, Station);
    strcpy(ip,para.ip);
//    elink_log("ip : %s",ip);
    return ip;
}

/**
 *  获取路由器MAC
 * @return
 */
static char *get_uplinkmac(void)
{

    OSStatus err = kUnknownErr;

    LinkStatusTypeDef wifi_link;
    err = micoWlanGetLinkStatus( &wifi_link );
    sprintf(uplinkmac,"%02x%02x%02x%02x%02x%02x",wifi_link.bssid[0],wifi_link.bssid[1], \
            wifi_link.bssid[2],wifi_link.bssid[3],wifi_link.bssid[4],wifi_link.bssid[5]);
    upper(uplinkmac,strlen(uplinkmac));
//    elink_log("uplinkmac : %s",uplinkmac);
    return uplinkmac;
}


/**
 *
 * @param code  包类型
 * @param data  input data
 * @param devid 设备ID
 * @param token 登录令牌
 * @return  output data
 */
char *elink_get_pack_full( char * code, char *data, char *devid, char *token )
{
    char * ciphData = NULL;
    char *jsonString = NULL;
    char *packStr = NULL;
    struct json_object *jsonObj = NULL;
    if ( code == NULL || data == NULL )
        return NULL;
    if ( strcmp( code, ELink_Code_dev_login ) == 0 )
    {
        //采用 设备PIN 加密
        memcpy( g_Aes_key, g_elink_devPin, 16 );
        memcpy( g_Aes_IV, g_elink_devPin + 16, 16 );
        ciphData = AES_CBC_Encrypt_Base64( data, strlen( data ), g_Aes_key, 16, g_Aes_IV, 16 );

    }
    else
    {
        ciphData = AES_CBC_Encrypt_Base64( data, strlen( data ), g_elink_comminfo->sessionKey, 16,
                                           g_elink_comminfo->sessionKey,
                                           16 );

    }

    if(ciphData == NULL)
    {
       return NULL;
    }

    jsonObj = json_object_new_object( );
    json_object_object_add( jsonObj, ELink_JSON_code, json_object_new_int( atoi( code ) ) );
    json_object_object_add( jsonObj, ELink_JSON_data, json_object_new_string( ciphData ) );

    if ( devid != NULL )
    {
        json_object_object_add( jsonObj, ELink_JSON_deviceId, json_object_new_string( devid ) );
    }

    if ( token != NULL )
    {
        json_object_object_add( jsonObj, ELink_JSON_token, json_object_new_string( token ) );
    }

    jsonString = json_object_to_json_string( jsonObj );
    elink_log("jsonString : %s",jsonString);

    packStr = malloc( strlen( jsonString ) + 20 );
    ELink_MSG_PACK( packStr, jsonString );

    if(ciphData)
    {
        free( ciphData );
        ciphData = NULL;
    }

    json_object_put( jsonObj );/*free memory*/
    jsonObj = NULL;
    free( jsonString );
    jsonString = NULL;
    return packStr;
}
/**
 *
 * @param outReqSequence 序列流水
 * @param json_package   打包的json数据
 * @return  output json data
 */
char* elink_get_req_data_dev_login( char *outReqSequence, char *json_package )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    json_object_object_add( dev_object, ELink_JSON_deviceId,
                            json_object_new_string( g_elink_devid ) );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );

    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add(dev_object,ELink_JSON_devMac,json_object_new_string(get_mac()));

    json_object_object_add(dev_object,ELink_JSON_devCTEI,json_object_new_string(get_ctei()));

    json_object_object_add(dev_object,ELink_JSON_devVersion,json_object_new_string(ELink_HARD_VER));

    json_object_object_add(dev_object,ELink_JSON_parentDevMac,json_object_new_string(get_uplinkmac()));

    json_object_object_add(dev_object,ELink_JSON_ip,json_object_new_string(get_ip()));

    json_object_object_add(dev_object,ELink_JSON_version,json_object_new_string(ELink_SOFT_VER));


    jsonString = json_object_to_json_string( dev_object );
    if ( json_package )
    {
        strcpy( json_package, jsonString );
        free( jsonString );
        jsonString = NULL;

    }
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    free( jsonString );
    jsonString = NULL;
    return json_package;
}
/**
 *  获取设备请求登录完整包
 * @param outReqSequence  序列流水
 * @return output data
 */
char* elink_get_req_pack_dev_login( char *outReqSequence )
{
    char *data = NULL;
    char *devLoginJson = NULL;
    data = malloc( 1024 );
    elink_get_req_data_dev_login( outReqSequence, data );
    devLoginJson = elink_get_pack_full( ELink_Code_dev_login, data, g_elink_devid, NULL );
    if ( data )
    {
        elink_log("elink_get_req_pack_dev_login free");
        free( data );
        data = NULL;
    }
    return devLoginJson;
}
/***
 * 获取设备连接完整包
 */
char* elink_get_req_pack_dev_connect( char *token, char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
    json_object_object_add( dev_object, ELink_JSON_token, json_object_new_string( token ) );
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );
    json_object_object_add( dev_object, ELink_JSON_model,
                            json_object_new_string( ELink_DevModel ) );
    json_object_object_add( dev_object, ELink_JSON_devVersion,
                            json_object_new_string( ELink_SOFT_VER ) );



    jsonString = json_object_to_json_string( dev_object );
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    return elink_get_pack_full( ELink_Code_dev_connect, jsonString, NULL, token );
}

/***
 * 设备控制应答处理
 */
char* elink_echo_pack_dev_ctrl( char *token, char *outReqSequence, char *deviceId, char *code,
                                ELINK_RETURN_E result )
{
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    switch ( result )
    {
        case ELINK_RETURN_OK:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_OK ) );
            break;

        }
        case ELINK_INVALID_PARA:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_INVALID_PARA ) );
            break;
        }
        case ELINK_DEV_ID_NO_EXSIT:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_ID_NO_EXSIT ) );
            break;
        }
        case ELINK_DEV_PIN_ERROR:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_PIN_ERROR ) );
            break;
        }
        case ELINK_DEV_AUTH_ERR:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_AUTH_ERR ) );
            break;
        }
        case ELINK_DEV_NO_ONLINE:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_NO_ONLINE ) );
            break;
        }
        case ELINK_DEV_NO_STRCODE:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_NO_STRCODE ) );
            break;
        }
        case ELINK_DEV_UNKOWN_ERR:
            {
            json_object_object_add( dev_object, ELink_JSON_result,
                                    json_object_new_int( ELink_errInfo_UNKOWN_ERR ) );
            break;
        }

    }

    json_object_object_add( dev_object, ELink_JSON_sequence,
                            json_object_new_string( g_elink_devinfo->Resequence ) );

    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( deviceId ) );

    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    jsonString = json_object_to_json_string( dev_object );
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    elink_log("echo_pack_dev_ctrl jsonString : %s",jsonString);
    return elink_get_pack_full( code, jsonString, NULL, token );
}

/***
 * 设备心跳保持
 */
char* elink_get_req_pack_dev_heartBeat( char *token, char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_Code_heartbeat, json_object_new_string( temp ) );
    json_object_object_add( dev_object, ELink_JSON_token, json_object_new_string( token ) );
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    return elink_get_pack_full( ELink_Code_heartbeat, jsonString, NULL, token );
}
/**
 * 登录应答包处理
 */
int elink_deal_devLoginEcho( char * reqSequence, char *msg, int msgLen )
{
    int ret = -1;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;

    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_result ) )
            {
                result = json_object_get_string( val );
                if ( result != NULL && strcmp( result, ELink_JSON_result_OK ) != 0 )
                {
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
//				if (sequence !=NULL && strcmp(reqSequence,sequence)!=0)
//					return -1;
            }
            if ( !strcmp( key, ELink_JSON_sessionKey ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_comminfo->sessionKey, "%s", strTemp );
                }
                else
                {
                    ret = ELINK_INVALID_PARA;
                }

            }
            if ( !strcmp( key, ELink_JSON_tcpHost ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_comminfo->tcpHost, "%s", strTemp );
                }
                else
                {
                    ret = ELINK_INVALID_PARA;
                }

            }
            if ( !strcmp( key, ELink_JSON_token ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_comminfo->token, "%s", strTemp );
                }
                else
                {
                    ret = ELINK_INVALID_PARA;
                }

            }
            if ( !strcmp( key, ELink_JSON_udpHost ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_comminfo->udphost, "%s", strTemp );
                }

            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    return ret;
}
/**
 * 业务登录应答包处理
 */
int elink_deal_devConnectEcho( char * reqSequence, char *msg, int msgLen )
{
    int ret = ELINK_INVALID_PARA;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_result ) )
            {
                result = json_object_get_string( val );
                if ( result != NULL && strcmp( result, ELink_JSON_result_OK ) != 0 )
                {
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
//				if (sequence !=NULL && strcmp(reqSequence,sequence)!=0)
//					return -1;
            }

            if ( !strcmp( key, ELink_JSON_authInterval ) )
            {
                g_elink_comminfo->authInterval = json_object_get_int( val );
            }
            if ( !strcmp( key, ELink_JSON_heartBeat ) )
            {
                g_elink_comminfo->heartBeat = json_object_get_int( val );
            }
            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    return ret;
}
/**
 * 心跳应答包处理
 */
int elink_deal_devheartbeatEcho( char * reqSequence, char *msg, int msgLen )
{
    int ret = ELINK_INVALID_PARA;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_result ) )
            {
                result = json_object_get_string( val );
                if ( result != NULL && strcmp( result, ELink_JSON_result_OK ) != 0 )
                {
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
//				if (sequence !=NULL && strcmp(reqSequence,sequence)!=0)
//					return -1;
            }

            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    return ret;
}

/**
 * 子设备在线状态应答上报处理
 */
int elink_deal_subdev_onlineEcho( char * reqSequence, char *msg, int msgLen )
{
    int ret = ELINK_INVALID_PARA;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_result ) )
            {
                result = json_object_get_string( val );
                if ( result != NULL && strcmp( result, ELink_JSON_result_OK ) != 0 )
                {
                    elink_log("result ok err");
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
//              if (sequence !=NULL && strcmp(reqSequence,sequence)!=0)
//                  return -1;
            }
            if ( !strcmp( key, ELink_JSON_deviceId ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->devid, "%s", strTemp );
                }
                else
                {
                    elink_log("devid err");
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }
            if ( !strcmp( key, ELink_JSON_subDeviceId ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->subdevid, "%s", strTemp );
                }
                else
                {
                    elink_log("subdevid err");
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }

            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    else
    {
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_INVALID_PARA;
    }
    return ret;
}

/**
 * 子设备状态和资源应答上报处理
 */
int elink_deal_subdev_statusorresEcho( char * reqSequence, char *msg, int msgLen )
{
    int ret = ELINK_INVALID_PARA;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_result ) )
            {
                result = json_object_get_string( val );
                if ( result != NULL && strcmp( result, ELink_JSON_result_OK ) != 0 )
                {
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
//              if (sequence !=NULL && strcmp(reqSequence,sequence)!=0)
//                  return -1;
            }
            if ( !strcmp( key, ELink_JSON_deviceId ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->subdevid, "%s", strTemp );
                }
                else
                {
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }

            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    else
    {
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_INVALID_PARA;
    }
    return ret;
}
/********************************************************
 * function: json_unpack_elink_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
 *********************************************************/
OSStatus json_unpack_elink_resinfo_obj( json_object *pjson_object,
                                        PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    if ( pjson_object == NULL )
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    json_object_object_foreach( pjson_object, key, val )
    {
        if ( !strcmp( key, ELink_ResInfo_KEY_ID ) )
        {
            elink_rev_ctrl->userid = json_object_get_int( val );
        }
        else if ( !strcmp( key, ELink_ResInfo_KEY_TYPE ) )
        {
            elink_rev_ctrl->keytype = json_object_get_int( val );
        }
        else if ( !strcmp( key, ELink_ResInfo_ADMIN_PASS ) )
        {
            strTemp = json_object_get_string( val );
            if ( strTemp != NULL )
            {
                memset( &elink_rev_ctrl->adminpwd, 0, sizeof(elink_rev_ctrl->adminpwd) );
                strcpy( elink_rev_ctrl->adminpwd, strTemp );
            }
            else
            {
                elink_log("adminpwd err!");
                return ELINK_INVALID_PARA;
            }

        }
        else if ( !strcmp( key, ELink_ResInfo_TEMP_PASS ) )
        {
            strTemp = json_object_get_string( val );
            if ( strTemp != NULL )
            {
                memset( &elink_rev_ctrl->tmppwd, 0, sizeof(elink_rev_ctrl->tmppwd) );
                strcpy( elink_rev_ctrl->tmppwd, strTemp );
            }
            else
            {
                elink_log("tmppwd err!");
                return ELINK_INVALID_PARA;
            }

        }
        else if ( !strcmp( key, ELink_ResInfo_START_TIME ) )
        {
            elink_rev_ctrl->start_time = json_object_get_int( val );
        }
        else if ( !strcmp( key, ELink_ResInfo_END_TIME ) )
        {
            elink_rev_ctrl->stop_time = json_object_get_int( val );
        }
        else if ( !strcmp( key, ELink_ResInfo_LIFECYCLE ) )
        {
            elink_rev_ctrl->usecnt = json_object_get_int( val );
        }
        else if(!strcmp( key, ELink_ResInfo_MODE ))
        {
            elink_rev_ctrl->mode = json_object_get_int( val );
        }
        elink_set_sub_reslist( &elink_rev_ctrl, 1 );

    }
    return kNoErr;
}
/********************************************************
 * function: json_unpack_elink_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
 *********************************************************/
OSStatus json_unpack_elink_reserial_obj( json_object *pjson_object,
                                         PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    if ( pjson_object == NULL )
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    int ret = 0;
    json_object_object_foreach( pjson_object, key, val )
    {
        if ( !strcmp( key, ELink_JSON_ResourceName ) )
        {
            strTemp = json_object_get_string( val );
            if ( strcmp( strTemp, ELink_ResName_TEMP_KEY_LIST ) != 0 )
            {
                elink_log("paradata err!");
                return ELINK_INVALID_PARA;
            }

        }
        if ( !strcmp( key, ELink_JSON_ResourceInfo ) )
        {
            uint8_t jsonType = json_object_get_type( val );
            if ( jsonType == json_type_array )
            {
                array_list *list = NULL;
                uint8_t count = 0;
                list = json_object_get_array( val );
                for ( count = 0; count < list->length; count++ )
                {
                    ret = json_unpack_elink_resinfo_obj( list->array[count], elink_rev_ctrl );
                }
            }

        }
    }
    return kNoErr;
}
/********************************************************
 * function: json_unpack_elink_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
 *********************************************************/
OSStatus json_unpack_elink_reserials_obj( json_object *pjson_object,
                                          PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    if ( pjson_object == NULL )
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int ret = 0;
    int strLen = 0;
    json_object_object_foreach( pjson_object, key, val )
    {
        if ( !strcmp( key, ELink_JSON_SerialId ) )
        {

        }
        if ( !strcmp( key, ELink_JSON_ResourceSerial ) )
        {
            uint8_t jsonType = json_object_get_type( val );
            if ( jsonType == json_type_array )
            {
                array_list *list = NULL;
                uint8_t count = 0;
                list = json_object_get_array( val );
                for ( count = 0; count < list->length; count++ )
                {
                    ret = json_unpack_elink_reserial_obj( list->array[count], elink_rev_ctrl );
                }
            }

        }
    }
    return kNoErr;
}

/********************************************************
 * function: json_unpack_elink_ctrl_obj
 * description:
 * input:   1. pjson_object:
 * output:  1. pdevlink_obj:
 * return:
 * auther:   chenb
 * other:
 *********************************************************/
OSStatus json_unpack_elink_ctrl_obj( json_object *pjson_object,
                                     PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    if ( pjson_object == NULL )
    {
        return kGeneralErr;
    }
    unsigned char *strTemp = NULL;
    int strLen = 0;
    json_object_object_foreach( pjson_object, key, val )
    {
        if ( !strcmp( key, ELink_JSON_cmdName ) )
        {
            strTemp = json_object_get_string( val );
        }
        else if ( !strcmp( key, ELink_JSON_cmdParam ) )
        {
            if ( !strcmp( strTemp, ELink_CrtlName_OPEN_LOCK ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    memset( elink_rev_ctrl->tmppwd, 0, sizeof(elink_rev_ctrl->tmppwd) );
                    memcpy( elink_rev_ctrl->tmppwd, strTemp, strlen( strTemp ) );
                }
                else
                {
                    elink_log("tmppwd  err!");
                    free(strTemp);
                    strTemp = NULL;
                    json_object_put( pjson_object );/*free memory*/
                    return ELINK_INVALID_PARA;
                }

            }
            else
            {
                elink_rev_ctrl->open_join_net = json_object_get_int( val );
                elink_log("elink_rev_ctrl->open_join_net : %d",elink_rev_ctrl->open_join_net);
            }

        }

    }
    free(strTemp);
    strTemp = NULL;
    json_object_put( pjson_object );/*free memory*/
    return kNoErr;
}

/**
 * 子设备状态查询
 */
OSStatus elink_deal_subdev_StatusQuery( char * reqSequence, char *deviceId, char *msg, int msgLen )
{
    int ret = ELINK_RETURN_OK;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {

            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
                if ( sequence )
                {
                    sprintf( g_elink_devinfo->Resequence, "%s", sequence );
                    elink_log("g_elink_devinfo->Resequence : %s",g_elink_devinfo->Resequence);
                }
                else
                {
                    elink_log("sequence err!");
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_deviceId ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL && strcmp( deviceId, strTemp ) != 0 )
                {
                    elink_log("deviceId err!");
                    ret = ELINK_INVALID_PARA;
                }
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->subdevid, "%s", strTemp );
                }
                else
                {
                    elink_log("deviceId exsit err!");
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }

            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    else
    {
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_INVALID_PARA;
    }
    return ret;
}
/**
 * 子设备开锁控制下发
 */
int elink_deal_subdev_ctrl( char * reqSequence, char *deviceId, char *msg, int msgLen,
                            PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    int ret = ELINK_RETURN_OK;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {

            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
                if ( sequence )
                {
                    sprintf( g_elink_devinfo->Resequence, "%s", sequence );

                    elink_log("g_elink_devinfo->Resequence : %s",g_elink_devinfo->Resequence);
                }
                else
                {
                    elink_log("sequence err!");
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_deviceId ) )
            {
                strTemp = json_object_get_string( val );
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->devid, "%s", strTemp );
                }
                else
                {
                    elink_log("deviceId exsit err!");
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }
            if ( !strcmp( key, ELink_JSON_cmd ) )
            {
                uint8_t jsonType = json_object_get_type( val );
                if ( jsonType == json_type_array )
                {
                    array_list *list = NULL;
                    uint8_t count = 0;
                    list = json_object_get_array( val );
                    for ( count = 0; count < list->length; count++ )
                    {
                        ret = json_unpack_elink_ctrl_obj( list->array[count], elink_rev_ctrl );
                    }
                }

            }
            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    else
    {
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_INVALID_PARA;
    }
    return ret;
}

/**
 * 子设备添加临时秘钥下发
 */

int elink_deal_subdev_add_tmp_key_list( char * reqSequence, char *deviceId, char *msg, int msgLen,
                                        PTCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl )
{
    int ret = ELINK_RETURN_OK;
    struct json_object *parse_json_object = NULL;
    char * result, *strTemp, *sequence = NULL;
    int status = json_checker_string( msg, msgLen );
    if ( !status )
    {
        elink_log("json data is illegal");
        ret = ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( msg );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {

            if ( !strcmp( key, ELink_JSON_sequence ) )
            {
                sequence = json_object_get_string( val );
                if ( sequence )
                {
                    sprintf( g_elink_devinfo->Resequence, "%s", sequence );
                    elink_log("g_elink_devinfo->Resequence : %s",g_elink_devinfo->Resequence);
                }
                else
                {
                    elink_log("sequence err!");
                    ret = ELINK_INVALID_PARA;
                }
            }
            if ( !strcmp( key, ELink_JSON_deviceId ) )
            {
                strTemp = json_object_get_string( val );
//                if ( strTemp != NULL && strcmp( deviceId, strTemp ) != 0 )
//                    return ELINK_INVALID_PARA;
                if ( strTemp != NULL )
                {
                    sprintf( g_elink_devinfo->subdevid, "%s", strTemp );
                }
                else
                {
                    ret = ELINK_DEV_ID_NO_EXSIT;
                }

            }
            if ( !strcmp( key, ELink_JSON_ResourceSerials ) )
            {
                uint8_t jsonType = json_object_get_type( val );
                if ( jsonType == json_type_array )
                {
                    array_list *list = NULL;
                    uint8_t count = 0;
                    list = json_object_get_array( val );
                    for ( count = 0; count < list->length; count++ )
                    {
                        ret = json_unpack_elink_reserials_obj( list->array[count], elink_rev_ctrl );
//                        (list->array[count], elink_rev_ctrl);
                    }
                }

            }
            if ( !strcmp( key, ELink_JSON_time ) )
            {
                g_elink_comminfo->time = json_object_get_int( val );
                //进行时间同步
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_RETURN_OK;
    }
    else
    {
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
        ret = ELINK_INVALID_PARA;
    }
    return ret;
}


/***
 * 通讯解包及分发处理
 */
int elink_parse_unpack( char *reqSequence, char *revbuf, int revlen )
{
    int ret = ELINK_RETURN_OK;
    char *temp = NULL;
    char *plainMsg = NULL;
    unsigned char *strCode, *strData = NULL;
    int plainMsgLen;
    temp = strstr( revbuf, "CTS" );
    if ( temp == NULL )
        return ret;
    mytime_struct start_time;
    mytime_struct stop_time;
    mico_rtc_time_t effect_time;
    mico_rtc_time_t invalid_time;

    plainMsg = malloc( revlen );
    memset( plainMsg, 0, revlen );
    memcpy( plainMsg, revbuf + 3, revlen - 3 );
    struct json_object *parse_json_object = NULL;
    TCP_SUBDEV_RSP_HEAD_T elink_rev_ctrl;
    int status = json_checker_string( plainMsg, strlen( plainMsg ) );
    if ( !status )
    {
        elink_log("json data is illegal");
        free( plainMsg );
        plainMsg = NULL;
        return ELINK_INVALID_PARA;
    }
    parse_json_object = json_tokener_parse( plainMsg );
    elink_set_sub_code( NULL );
    if ( parse_json_object != NULL )
    {
        json_object_object_foreach( parse_json_object, key, val )
        {
            if ( !strcmp( key, ELink_JSON_code ) )
            {
                strCode = json_object_get_string( val );
            }
            if ( !strcmp( key, ELink_JSON_data ) )
            {
                strData = json_object_get_string( val );
            }
        }
        json_object_put( parse_json_object );/*free memory*/
        parse_json_object = NULL;
    }
    memset( elink_strcode, 0, sizeof(elink_strcode) );
    strcpy( elink_strcode, strCode );
    elink_log("elink_strcode : %s",elink_strcode);

    if ( !strcmp( elink_strcode, ELink_Code_dev_login_echo ) )
    {
        //登录应答处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), elink_get_dev_key( ), 16,
                                elink_get_dev_iv( ),
                                16, plainMsg, &plainMsgLen );
        elink_log("plainMsg : %s plainMsgLen : %d",plainMsg,plainMsgLen);
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_devLoginEcho( reqSequence, plainMsg, plainMsgLen );
            if(ret != 0)
            {
                g_elink_comminfo->devConnectFd = -1;
                set_loginFlag(0);
            }
        }
        else
        {
            g_elink_comminfo->devConnectFd = -1;
            set_loginFlag(0);
        }
    }

    if ( !strcmp( elink_strcode, ELink_Code_dev_connect_echo ) )
    {
        //连接应答处理
        elink_log("g_elink_comminfo->sessionKey : %s",g_elink_comminfo->sessionKey);
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );

        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_devConnectEcho( reqSequence, plainMsg, plainMsgLen );
            elink_log(" plainMsgLen : %d ret : %d",plainMsgLen,ret);
            if ( ret == 0 )
            {
                g_elink_comminfo->devConnectFlag = 1;
                g_elink_comminfo->loginlasttime = time( 0 );
                g_elink_comminfo->temptime =  time( 0 );
                msleep( 500 );
                elink_gw_heartreport();   //取消设备资源上报
//                msleep( 500 );
//                ble_force_get_lock_list( 0 );


            }
            else
            {
                g_elink_comminfo->devConnectFd = -1;
                set_loginFlag(0);
            }

        }
        else
        {
            g_elink_comminfo->devConnectFd = -1;
            set_loginFlag(0);
        }

    }
    if ( !strcmp( elink_strcode, ELink_Code_heartbeat_echo ) )
    {
        //心跳应答处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        elink_log("plainMsgLen : %d",plainMsgLen);
        set_heatstatue(0);
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_devheartbeatEcho( reqSequence, plainMsg, plainMsgLen );

            if ( elink_get_join_net_status( ) == 1 && ret == 0)   //绑定过
            {
                g_elink_comminfo->authSucess = 1;
                elink_log("g_elink_comminfo->authSucess : %d",g_elink_comminfo->authSucess);
            }

        }


    }


    if ( !strcmp( elink_strcode, ELink_Code_StatusReport_echo ) )
    {
        //子设备状态上报处理
//        elink_log("strData : %s",strData);
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_statusorresEcho( reqSequence, plainMsg, plainMsgLen );
        }


    }
    if ( !strcmp( elink_strcode, ELink_Code_DevResChangeReq_echo ) )
    {
        //子设备资源上报处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_statusorresEcho( reqSequence, plainMsg, plainMsgLen );

        }

    }
    if ( !strcmp( elink_strcode, ELink_Code_StatusAlarm_echo ) )
    {
        //子设备告警上报处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_statusorresEcho( reqSequence, plainMsg, plainMsgLen );
        }

    }

    if ( !strcmp( elink_strcode, ELink_Code_StatusFault_echo ) )
    {
        //子设备故障应答处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_statusorresEcho( reqSequence, plainMsg, plainMsgLen );

        }

    }
    if ( !strcmp( elink_strcode, ELink_Code_EventReport_echo ) )
    {
        //子设备信息上报处理
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_statusorresEcho( reqSequence, plainMsg, plainMsgLen );
        }

    }
    if ( !strcmp( elink_strcode, ELink_Code_StatusQuery ) )
    {
        //子设备状态查询处理
        char strcodetmp[10] = { 0 };
        sprintf( strcodetmp, "%s", ELink_Code_StatusQuery );
        elink_set_sub_code( &strcodetmp );
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            ret = elink_deal_subdev_StatusQuery( reqSequence, get_devid( ), plainMsg,
                                                 plainMsgLen );
            if(strcmp( get_devid(), g_elink_devinfo->devid ) == 0)   //获取应用网关状态
            {
                ret = 0;
                elink_set_dev_errstrcode( ret );
            }
            else
            {
                if ( get_loginAuthInterval( ) == 1 )      // 授权过
                {
                   elink_set_dev_errstrcode( ret );
                   if ( ret == 0 )
                   {
                       ble_get_lockinfo( );

                       /***发送一次获取锁信息状态**/
                   }
                }
            }


        }
        else
        {
            elink_set_dev_errstrcode( ELINK_DEV_UNKOWN_ERR );
        }

    }
    if ( !strcmp( elink_strcode, ELink_Code_Ctrl ) )
    {
        //子设备控制
        char strcodetmp[10] = { 0 };
        sprintf( strcodetmp, "%s", ELink_Code_Ctrl );
        elink_set_sub_code( &strcodetmp );
        memset( &elink_rev_ctrl, 0, sizeof(TCP_SUBDEV_RSP_HEAD_T) );
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            elink_log("plainMsg : %s",plainMsg);
            ret = elink_deal_subdev_ctrl( reqSequence, get_devid( ), plainMsg, plainMsgLen,
                                          &elink_rev_ctrl );
            elink_log("ret : %d ",ret);
            elink_set_dev_errstrcode( ret );
            if ( ret == 0 )
            {

                if ( elink_rev_ctrl.open_join_net == 1 )
                {
                    ble_bind_lock_start( );

                }
                else if ( elink_rev_ctrl.adminpwd != NULL )
                {
                    if ( get_loginAuthInterval( ) == 1 )      // 授权过
                    {
                        ble_remote_pwd_unlock_start( elink_rev_ctrl.tmppwd );     //远程开锁
                    }

                }
                else if ( elink_rev_ctrl.open_join_net == 0 )
                {
                    g_elink_comminfo->authSucess = 0;  //取消入网
                    elink_set_join_net_status( 0 );
                }
            }
        }
        else
        {
            elink_set_dev_errstrcode( ELINK_DEV_UNKOWN_ERR );
        }
    }
    if ( !strcmp( elink_strcode, ELink_Code_DevResMangerReq ) )
    {
        //子设备添加临时秘钥资源列表
        char strcodetmp[10] = { 0 };
        sprintf( strcodetmp, "%s", ELink_Code_DevResMangerReq );
        elink_set_sub_code( &strcodetmp );
        memset( &elink_rev_ctrl, 0, sizeof(TCP_SUBDEV_RSP_HEAD_T) );
        AES_CBC_Decrypt_Base64( strData, strlen( strData ), g_elink_comminfo->sessionKey, 16,
                                g_elink_comminfo->sessionKey,
                                16, plainMsg, &plainMsgLen );
        if ( plainMsgLen != 0 )
        {
            elink_log("plainMsg : %s",plainMsg);
            ret = elink_deal_subdev_add_tmp_key_list( reqSequence, get_devid( ), plainMsg,
                                                      plainMsgLen,
                                                      &elink_rev_ctrl );
            if ( get_loginAuthInterval( ) == 1 )      // 授权过
            {
                elink_set_dev_errstrcode( ret );
                if ( ret == 0 )
                {

                    elink_set_dev_busystate( 1 );
                    if ( elink_rev_ctrl.mode == 1 )      // 添加临时秘钥
                    {
                        /****utc时间转换****/
                        utc_sec_2_mytime( elink_rev_ctrl.start_time, &start_time, false );
                        effect_time.year = start_time.nYear % 100;
                        effect_time.month = start_time.nMonth;
                        effect_time.date = start_time.nDay;
                        effect_time.hr = start_time.nHour;
                        effect_time.min = start_time.nMin;
                        effect_time.sec = start_time.nSec;

                        utc_sec_2_mytime( elink_rev_ctrl.stop_time, &stop_time, false );
                        invalid_time.year = stop_time.nYear % 100;
                        invalid_time.month = stop_time.nMonth;
                        invalid_time.date = stop_time.nDay;
                        invalid_time.hr = stop_time.nHour;
                        invalid_time.min = stop_time.nMin;
                        invalid_time.sec = stop_time.nSec;

                        ble_add_tmp_pwd_start( elink_rev_ctrl.userid, elink_rev_ctrl.keytype,
                                               elink_rev_ctrl.adminpwd,
                                               elink_rev_ctrl.tmppwd, &effect_time, &invalid_time,
                                               elink_rev_ctrl.usecnt );

                    }
                    if(elink_rev_ctrl.mode == 3)                        //删除临时秘钥
                    {
                        elink_log("elink_rev_ctrl.userid : %d adminpwd : %s",elink_rev_ctrl.userid,elink_rev_ctrl.adminpwd);
                        ble_del_tmp_pwd_start( elink_rev_ctrl.userid, elink_rev_ctrl.adminpwd );
                    }
                }
            }

        }

    }

    free( plainMsg );
    plainMsg = NULL;
    if(strData)
    {
        free(strData);
        strData = NULL;
    }
    if(strCode)
    {
        free(strCode);
        strCode = NULL;
    }
    json_object_put( parse_json_object );/*free memory*/
    if ( plainMsgLen == 0 )
    {
        return ELINK_INVALID_PARA;
    }

    return ret;
}

/***
 * 子设备鉴权
 */
char* elink_get_pack_subdev_auth( char* token, char *subdevid, char *pin, char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
    json_object_object_add( dev_object, ELink_JSON_deviceId,
                            json_object_new_string( g_elink_devid ) );
    json_object_object_add( dev_object, ELink_JSON_subDeviceId,
                            json_object_new_string( subdevid ) );
    json_object_object_add( dev_object, Elink_JSON_Pin, json_object_new_string( pin ) );
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
//    elink_log("subdev_auth jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_SubAuth, jsonString, NULL, token );
}

/**
 * 子设备解除绑定上报
 */
char* elink_get_pack_subdev_unbind( char* token, char *subdevid, char *pin, char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
    json_object_object_add( dev_object, ELink_JSON_deviceId,
                            json_object_new_string( g_elink_devid ) );
    json_object_object_add( dev_object, ELink_JSON_subDeviceId,
                            json_object_new_string( subdevid ) );
    json_object_object_add( dev_object, Elink_JSON_Pin, json_object_new_string( pin ) );
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    jsonString = json_object_to_json_string( dev_object );
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;

    return elink_get_pack_full( ELink_Code_SubunBindReport, jsonString, NULL, token );
}

/**
 * 子设备在线状态上报
 */
char* elink_get_pack_subdev_OnLineStatusReport( char* token, char *subdevid, uint8_t onlineflag,
                                                char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
    json_object_object_add( dev_object, ELink_JSON_deviceId,
                            json_object_new_string( g_elink_devid ) );
    json_object_object_add( dev_object, ELink_JSON_subDeviceId,
                            json_object_new_string( subdevid ) );
//	json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );
    if ( onlineflag > 0 )
        json_object_object_add( dev_object, ELink_JSON_OnLine,
                                json_object_new_string( ELink_JSON_online_OK ) );
    else
        json_object_object_add( dev_object, ELink_JSON_OnLine,
                                json_object_new_string( ELink_JSON_result_OK ) );

    jsonString = json_object_to_json_string( dev_object );
//    elink_log("jsonString : %s",jsonString);
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    return elink_get_pack_full( ELink_Code_SubOnlineReport, jsonString, NULL, token );
}

/**
 * 设备状态上报
 */
char* elink_get_pack_subdev_StatusReport( char* token, char *subdevid, uint8_t status, char num,
                                          char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_statusSerials_array = NULL;
    struct json_object *json_object_statusSerial_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_statusSerials_array = json_object_new_array( );
    json_object_statusSerial_array = json_object_new_array( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//	json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//	json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
    for ( count = 0; count < num; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );
        json_object_array_add( json_object_statusSerials_array, json_object_value[count] );

        json_object_object_add( json_object_value[count], Elink_JSON_StatusSerial,
                                json_object_statusSerial_array );

        json_object_serial_value = json_object_new_object( );

        json_object_object_add( json_object_serial_value, ELink_JSON_StatusName,
                                json_object_new_string( ELink_StatusName_LOCK_STATE ) );
        memset( strtrmp, 0, sizeof(strtrmp) );

        sprintf( strtrmp, "%d", status );
        json_object_object_add( json_object_serial_value, ELink_JSON_CurStatusValue,
                                json_object_new_string( strtrmp ) );
        json_object_array_add( json_object_statusSerial_array, json_object_serial_value );

    }

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/
    dev_object = NULL;
    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}
/**
 *
 * @param token  登录令牌
 * @param subdevid 子设备ID
 * @param status    上报的状态
 * @param num       上报个数
 * @param outReqSequence 序列流水
 * @return  output data
 */
char* elink_get_pack_subdev_SignalReport( char* token, char *subdevid, uint8_t status, char num,
                                          char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_statusSerials_array = NULL;
    struct json_object *json_object_statusSerial_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_statusSerials_array = json_object_new_array( );
    json_object_statusSerial_array = json_object_new_array( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
    for ( count = 0; count < num; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );
        json_object_array_add( json_object_statusSerials_array, json_object_value[count] );

        json_object_object_add( json_object_value[count], Elink_JSON_StatusSerial,
                                json_object_statusSerial_array );

        json_object_serial_value = json_object_new_object( );

        json_object_object_add( json_object_serial_value, ELink_JSON_StatusName,
                                json_object_new_string( ELink_StatusName_SIGNAL ) );
        memset( strtrmp, 0, sizeof(strtrmp) );

        sprintf( strtrmp, "%d", status );
        json_object_object_add( json_object_serial_value, ELink_JSON_CurStatusValue,
                                json_object_new_string( strtrmp ) );
        json_object_array_add( json_object_statusSerial_array, json_object_serial_value );

    }

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/
    dev_object = NULL;
    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}
/**
 * 设备拆除状态上报
 */
char* elink_get_pack_subdev_DemolishReport( char* token, char *subdevid, uint8_t status, char num,
                                            char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_statusSerials_array = NULL;
    struct json_object *json_object_statusSerial_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_statusSerials_array = json_object_new_array( );
    json_object_statusSerial_array = json_object_new_array( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
    for ( count = 0; count < num; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );
        json_object_array_add( json_object_statusSerials_array, json_object_value[count] );

        json_object_object_add( json_object_value[count], Elink_JSON_StatusSerial,
                                json_object_statusSerial_array );

        json_object_serial_value = json_object_new_object( );

        json_object_object_add( json_object_serial_value, ELink_JSON_StatusName,
                                json_object_new_string( ELink_StatusName_DEMOLISH ) );
        memset( strtrmp, 0, sizeof(strtrmp) );

        sprintf( strtrmp, "%d", status );
        json_object_object_add( json_object_serial_value, ELink_JSON_CurStatusValue,
                                json_object_new_string( strtrmp ) );
        json_object_array_add( json_object_statusSerial_array, json_object_serial_value );

    }

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/
    dev_object = NULL;
    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}
/**
 * 设备资源上报
 */
char* elink_get_pack_subdev_ResReport( char* token, char *subdevid,
                                       BLEUSER_LISTRECORDLIST *elink_subresinfo,
                                       uint8_t num,
                                       char *outReqSequence,uint8_t status )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    int list_count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_resSerials_array = NULL;
    struct json_object *json_object_resSerial_array = NULL;
    struct json_object *json_object_resInfo_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_resvalue[num];
    struct json_object *json_object_serial_value;


   struct json_object *json_object_statusSerials_array = NULL;
   struct json_object *json_object_statusSerial_array = NULL;
   struct json_object *json_object_status_value;
   struct json_object *json_object_status_serial_value;
   dev_object = json_object_new_object( );

   sprintf( temp, "%d", st_get_req_sequence( ) );
      if ( outReqSequence )
          strcpy( outReqSequence, temp );
      json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
  //  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
      json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
   json_object_statusSerials_array = json_object_new_array( );
   json_object_statusSerial_array = json_object_new_array( );

   json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
   json_object_status_value = json_object_new_object( );

   json_object_object_add( json_object_status_value, ELink_JSON_SerialId,
                              json_object_new_string( ELink_JSON_result_OK ) );
   json_object_array_add( json_object_statusSerials_array, json_object_status_value );

   json_object_object_add( json_object_status_value, Elink_JSON_StatusSerial,
                              json_object_statusSerial_array );

   json_object_status_serial_value = json_object_new_object( );

   json_object_object_add( json_object_status_serial_value, ELink_JSON_StatusName,
                              json_object_new_string( ELink_StatusName_LOCK_STATE ) );
   memset( strtrmp, 0, sizeof(strtrmp) );

   sprintf( strtrmp, "%d", status );
   json_object_object_add( json_object_status_serial_value, ELink_JSON_CurStatusValue,
                              json_object_new_string( strtrmp ) );
   json_object_array_add( json_object_statusSerial_array, json_object_status_serial_value );

   json_object_resSerials_array = json_object_new_array( );

   json_object_resSerial_array = json_object_new_array( );
   json_object_resInfo_array = json_object_new_array( );


//	json_object_object_add(dev_object, "pin", json_object_new_string(pin));


    json_object_object_add( dev_object, ELink_JSON_ResourceSerials, json_object_resSerials_array );
    elink_log("num : %d",num);
    for ( count = 0; count < 1; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );

        json_object_array_add( json_object_resSerials_array, json_object_value[count] );

//		 json_object_object_add(json_object_value[count], ELink_JSON_ResourceSerial, json_object_resSerial_array);
        json_object_object_add( json_object_value[count], ELink_JSON_ResourceSerial,
                                json_object_resSerial_array );

        json_object_serial_value = json_object_new_object( );
        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceName,
                                json_object_new_string( ELink_ResName_KEY_LIST ) );

        json_object_array_add( json_object_resSerial_array, json_object_serial_value );

        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceInfo,
                                json_object_resInfo_array );

        for ( list_count = 0; list_count < num; list_count++ )
        {
            json_object_resvalue[list_count] = json_object_new_object( );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].usrid );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_ID,
                                    json_object_new_string( strtrmp ) );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].keytype );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_TYPE,
                                    json_object_new_string( strtrmp ) );
            json_object_array_add( json_object_resInfo_array, json_object_resvalue[list_count] );
        }

    }
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );
    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_resSerials_array );/*free memory*/
    json_object_put( json_object_resSerial_array );/*free memory*/
    json_object_put( json_object_resInfo_array );/*free memory*/
    dev_object = NULL;
    json_object_resSerials_array = NULL;
    json_object_resSerial_array = NULL;
    json_object_resInfo_array = NULL;

    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/

    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
	elink_log("jsonString : %s",jsonString);
//	return NULL;
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}
/**
 * 设备资源变化上报
 */
char* elink_get_pack_subdev_ResChangeReport( char* token, char *subdevid,
                                             BLEUSER_LISTRECORDLIST *elink_subresinfo,
                                             uint8_t num,
                                             char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    int list_count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_resSerials_array = NULL;
    struct json_object *json_object_resSerial_array = NULL;
    struct json_object *json_object_resInfo_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_resvalue[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_resSerials_array = json_object_new_array( );

    json_object_resSerial_array = json_object_new_array( );
    json_object_resInfo_array = json_object_new_array( );

    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, ELink_JSON_ResourceSerials, json_object_resSerials_array );
    elink_log("num : %d",num);
    for ( count = 0; count < 1; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );

        json_object_array_add( json_object_resSerials_array, json_object_value[count] );

//       json_object_object_add(json_object_value[count], ELink_JSON_ResourceSerial, json_object_resSerial_array);
        json_object_object_add( json_object_value[count], ELink_JSON_ResourceSerial,
                                json_object_resSerial_array );

        json_object_serial_value = json_object_new_object( );
        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceName,
                                json_object_new_string( ELink_ResName_KEY_LIST ) );

        json_object_array_add( json_object_resSerial_array, json_object_serial_value );

        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceInfo,
                                json_object_resInfo_array );

        for ( list_count = 0; list_count < num; list_count++ )
        {
            json_object_resvalue[list_count] = json_object_new_object( );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].usrid );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_ID,
                                    json_object_new_string( strtrmp ) );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].keytype );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_TYPE,
                                    json_object_new_string( strtrmp ) );
            if ( elink_subresinfo[list_count].mode == 1 )  // 新增
            {
                json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_MODE,
                                        json_object_new_string( ELink_JSON_online_OK ) );
            }
            else      // 删除
            {
                json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_MODE,
                                        json_object_new_string( ELink_JSON_del_OK ) );
            }

            json_object_array_add( json_object_resInfo_array, json_object_resvalue[list_count] );
        }

    }
    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_resSerials_array );/*free memory*/
    json_object_put( json_object_resSerial_array );/*free memory*/
    json_object_put( json_object_resInfo_array );/*free memory*/
    dev_object = NULL;
    json_object_resSerials_array = NULL;
    json_object_resSerial_array = NULL;
    json_object_resInfo_array = NULL;
    elink_log("jsonString : %s",jsonString);
//  return NULL;
    return elink_get_pack_full( ELink_Code_DevResChangeReq, jsonString, NULL, token );
}
/**
 * 设备临时资源上报
 */
char* elink_get_pack_subdev_ResTmpChangeReport( char* token, char *subdevid,
                                          BLEUSER_LISTRECORDLIST *elink_subresinfo,
                                          uint8_t num,
                                          uint8_t restmpflag,
                                          char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    int list_count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_resSerials_array = NULL;
    struct json_object *json_object_resSerial_array = NULL;
    struct json_object *json_object_resInfo_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_resvalue[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_resSerials_array = json_object_new_array( );

    json_object_resSerial_array = json_object_new_array( );
    json_object_resInfo_array = json_object_new_array( );

    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, ELink_JSON_ResourceSerials, json_object_resSerials_array );
    elink_log("num : %d",num);
    for ( count = 0; count < 1; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );

        json_object_array_add( json_object_resSerials_array, json_object_value[count] );

//       json_object_object_add(json_object_value[count], ELink_JSON_ResourceSerial, json_object_resSerial_array);
        json_object_object_add( json_object_value[count], ELink_JSON_ResourceSerial,
                                json_object_resSerial_array );

        json_object_serial_value = json_object_new_object( );
        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceName,
                                json_object_new_string( ELink_ResName_TEMP_KEY_LIST ) );

        json_object_array_add( json_object_resSerial_array, json_object_serial_value );

        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceInfo,
                                json_object_resInfo_array );

        for ( list_count = 0; list_count < num; list_count++ )
        {
            if ( num == 0 && elink_subresinfo == NULL)
            {
                break;
            }
            json_object_resvalue[list_count] = json_object_new_object( );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].usrid );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_ID,
                                    json_object_new_string( strtrmp ) );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].keytype );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_TYPE,
                                    json_object_new_string( strtrmp ) );

            if ( restmpflag == 1 )
            {
                json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_MODE,
                                        json_object_new_string( ELink_JSON_online_OK ) );
            }
            else
            {
                json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_MODE,
                                        json_object_new_string( ELink_JSON_del_OK ) );
            }

            json_object_array_add( json_object_resInfo_array, json_object_resvalue[list_count] );
        }

    }
    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_resSerials_array );/*free memory*/
    json_object_put( json_object_resSerial_array );/*free memory*/
    json_object_put( json_object_resInfo_array );/*free memory*/
    dev_object = NULL;
    json_object_resSerials_array = NULL;
    json_object_resSerial_array = NULL;
    json_object_resInfo_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_DevResChangeReq, jsonString, NULL, token );
}
/**
 * 设备临时资源属性上报
 */
char* elink_get_pack_subdev_ResTmpReport( char* token, char *subdevid,
                                          BLEUSER_LISTRECORDLIST *elink_subresinfo,
                                          uint8_t num,
                                          uint8_t restmpflag,
                                          char *outReqSequence,uint8_t status )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    int list_count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_resSerials_array = NULL;
    struct json_object *json_object_resSerial_array = NULL;
    struct json_object *json_object_resInfo_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_resvalue[num];
    struct json_object *json_object_serial_value;

    struct json_object *json_object_statusSerials_array = NULL;
    struct json_object *json_object_statusSerial_array = NULL;
    struct json_object *json_object_status_value;
    struct json_object *json_object_status_serial_value;

    dev_object = json_object_new_object( );
    json_object_resSerials_array = json_object_new_array( );

    json_object_resSerial_array = json_object_new_array( );
    json_object_resInfo_array = json_object_new_array( );

    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );



    json_object_statusSerials_array = json_object_new_array( );
    json_object_statusSerial_array = json_object_new_array( );

   json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
   json_object_status_value = json_object_new_object( );

   json_object_object_add( json_object_status_value, ELink_JSON_SerialId,
                              json_object_new_string( ELink_JSON_result_OK ) );
   json_object_array_add( json_object_statusSerials_array, json_object_status_value );

   json_object_object_add( json_object_status_value, Elink_JSON_StatusSerial,
                              json_object_statusSerial_array );

   json_object_status_serial_value = json_object_new_object( );

   json_object_object_add( json_object_status_serial_value, ELink_JSON_StatusName,
                              json_object_new_string( ELink_StatusName_LOCK_STATE ) );
   memset( strtrmp, 0, sizeof(strtrmp) );

   sprintf( strtrmp, "%d", status );
   json_object_object_add( json_object_status_serial_value, ELink_JSON_CurStatusValue,
                              json_object_new_string( strtrmp ) );
   json_object_array_add( json_object_statusSerial_array, json_object_status_serial_value );

    elink_log("num : %d",num);
    json_object_object_add( dev_object, ELink_JSON_ResourceSerials, json_object_resSerials_array );
    for ( count = 0; count < 1; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );

        json_object_array_add( json_object_resSerials_array, json_object_value[count] );

//       json_object_object_add(json_object_value[count], ELink_JSON_ResourceSerial, json_object_resSerial_array);
        json_object_object_add( json_object_value[count], ELink_JSON_ResourceSerial,
                                json_object_resSerial_array );

        json_object_serial_value = json_object_new_object( );
        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceName,
                                json_object_new_string( ELink_ResName_TEMP_KEY_LIST ) );

        json_object_array_add( json_object_resSerial_array, json_object_serial_value );

        json_object_object_add( json_object_serial_value, ELink_JSON_ResourceInfo,
                                json_object_resInfo_array );

        for ( list_count = 0; list_count < num; list_count++ )
        {
            if ( num == 0 && elink_subresinfo == NULL)
            {
                break;
            }
            json_object_resvalue[list_count] = json_object_new_object( );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].usrid );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_ID,
                                    json_object_new_string( strtrmp ) );
            memset( strtrmp, 0, sizeof(strtrmp) );
            sprintf( strtrmp, "%d", elink_subresinfo[list_count].keytype );
            json_object_object_add( json_object_resvalue[list_count], ELink_ResInfo_KEY_TYPE,
                                    json_object_new_string( strtrmp ) );


            json_object_array_add( json_object_resInfo_array, json_object_resvalue[list_count] );
        }

    }
    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_resSerials_array );/*free memory*/
    json_object_put( json_object_resSerial_array );/*free memory*/
    json_object_put( json_object_resInfo_array );/*free memory*/
    dev_object = NULL;
    json_object_resSerials_array = NULL;
    json_object_resSerial_array = NULL;
    json_object_resInfo_array = NULL;

    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/

    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}

/**
 * 设备攻击报警
 */
char* elink_get_pack_subdev_AtkReport( char* token, char *subdevid,
                                         ELINK_ALARM_STATE_E alarmstate,
                                         uint8_t battery, char num,
                                         char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtemp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_event_array = NULL;
    struct json_object *json_object_eventinfo_array = NULL;
    struct json_object *json_object_event_value;
    struct json_object *json_object_eventinfo_value;

    dev_object = json_object_new_object( );
    json_object_event_array = json_object_new_array( );
    json_object_eventinfo_array = json_object_new_array( );
    json_object_event_value = json_object_new_object( );
    json_object_eventinfo_value = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, ELink_JSON_Event, json_object_event_array );

    json_object_object_add( json_object_event_value, ELink_JSON_EventId,
                            json_object_new_string( ELink_JSON_online_OK ) );

    json_object_object_add( json_object_event_value, ELink_JSON_SerialId,
                            json_object_new_string( ELink_JSON_result_OK ) );

    json_object_object_add( json_object_event_value, ELink_JSON_EventName,
                            json_object_new_string( ELink_EventName_LOCK_ALARM ) );

    json_object_array_add( json_object_event_array, json_object_event_value );

    json_object_object_add( json_object_event_value, ELink_JSON_EventInfo,
                            json_object_eventinfo_array );

    json_object_object_add( json_object_eventinfo_value, ELink_EventInfo_ALARM_TYPE,
                            json_object_new_string( ELink_AlarmInfo_PASS_ATK ) );

    json_object_object_add( json_object_eventinfo_value, ELink_EventInfo_TIME,
                            json_object_new_int( time( 0 )  ) );

    json_object_array_add( json_object_eventinfo_array, json_object_eventinfo_value );

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_event_array );/*free memory*/
    json_object_put( json_object_eventinfo_array );/*free memory*/
    dev_object = NULL;
    json_object_event_array = NULL;
    json_object_eventinfo_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_EventReport, jsonString, NULL, token );
}

/**
 * 设备告警上报
 */
char* elink_get_pack_subdev_AlarmReport( char* token, char *subdevid,
                                         ELINK_ALARM_STATE_E alarmstate,
                                         uint8_t battery, char num,
                                         char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtrmp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_statusSerials_array = NULL;
    struct json_object *json_object_statusSerial_array = NULL;
    struct json_object *json_object_value[num];
    struct json_object *json_object_serial_value;

    dev_object = json_object_new_object( );
    json_object_statusSerials_array = json_object_new_array( );
    json_object_statusSerial_array = json_object_new_array( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, Elink_JSON_StatusSerials, json_object_statusSerials_array );
    for ( count = 0; count < num; count++ )
    {
        json_object_value[count] = json_object_new_object( );

        json_object_object_add( json_object_value[count], ELink_JSON_SerialId,
                                json_object_new_string( ELink_JSON_result_OK ) );
        json_object_array_add( json_object_statusSerials_array, json_object_value[count] );

        json_object_object_add( json_object_value[count], Elink_JSON_StatusSerial,
                                json_object_statusSerial_array );

        json_object_serial_value = json_object_new_object( );

        json_object_object_add( json_object_serial_value, ELink_JSON_StatusName,
                                            json_object_new_string( ELink_StatusName_BATTERY_WARN ) );

        memset( strtrmp, 0, sizeof(strtrmp) );

        sprintf( strtrmp, "%d", 1 );
        json_object_object_add( json_object_serial_value, ELink_JSON_CurStatusValue,
                                json_object_new_string( strtrmp ) );
        json_object_array_add( json_object_statusSerial_array, json_object_serial_value );

    }

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_statusSerials_array );/*free memory*/
    json_object_put( json_object_statusSerial_array );/*free memory*/
    dev_object = NULL;
    json_object_statusSerials_array = NULL;
    json_object_statusSerial_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}

/**
 * 设备信息上报
 */
char* elink_get_pack_subdev_EventReport( char* token, char *subdevid,
                                         PTCP_SUBDEV_RSP_HEAD_T evnetinfo,
                                         char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtemp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;
    struct json_object *json_object_event_array = NULL;
    struct json_object *json_object_eventinfo_array = NULL;
    struct json_object *json_object_event_value;
    struct json_object *json_object_eventinfo_value;

    dev_object = json_object_new_object( );
    json_object_event_array = json_object_new_array( );
    json_object_eventinfo_array = json_object_new_array( );
    json_object_event_value = json_object_new_object( );
    json_object_eventinfo_value = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//	json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//	json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, ELink_JSON_Event, json_object_event_array );

    json_object_object_add( json_object_event_value, ELink_JSON_EventId,
                            json_object_new_string( ELink_JSON_online_OK ) );

    json_object_object_add( json_object_event_value, ELink_JSON_SerialId,
                            json_object_new_string( ELink_JSON_result_OK ) );

    json_object_object_add( json_object_event_value, ELink_JSON_EventName,
                            json_object_new_string( ELink_EventName_LOCK_OPEN ) );

    json_object_array_add( json_object_event_array, json_object_event_value );

    json_object_object_add( json_object_event_value, ELink_JSON_EventInfo,
                            json_object_eventinfo_array );
    memset( strtemp, 0, sizeof(strtemp) );
    sprintf( strtemp, "%d", evnetinfo->userid );
    json_object_object_add( json_object_eventinfo_value, ELink_ResInfo_KEY_ID,
                            json_object_new_string( strtemp ) );
    memset( strtemp, 0, sizeof(strtemp) );
    sprintf( strtemp, "%d", evnetinfo->keytype );

    json_object_object_add( json_object_eventinfo_value, ELink_ResInfo_KEY_TYPE,
                            json_object_new_string( strtemp ) );

    json_object_object_add( json_object_eventinfo_value, ELink_EventInfo_TIME,
                            json_object_new_int( evnetinfo->time ) );

    json_object_array_add( json_object_eventinfo_array, json_object_eventinfo_value );

    jsonString = json_object_to_json_string( dev_object );

    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_event_array );/*free memory*/
    json_object_put( json_object_eventinfo_array );/*free memory*/
    dev_object = NULL;
    json_object_event_array = NULL;
    json_object_eventinfo_array = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_EventReport, jsonString, NULL, token );
}

/**
 * 设备故障信息上报
 */
char* elink_get_pack_subdev_errReport( char* token, char *subdevid, ELINK_RETURN_E errstate,
                                       char num,
                                       char *outReqSequence )
{
    unsigned char temp[32];
    unsigned char strtemp[10];
    int count = 0;
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    struct json_object *json_object_serial_value = NULL;

    dev_object = json_object_new_object( );
    json_object_serial_value = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//  json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );

    json_object_object_add( dev_object, ELink_JSON_Error, json_object_serial_value );

    json_object_object_add( json_object_serial_value, ELink_JSON_SerialId,
                            json_object_new_int( ELink_errInfo_OK ) );
    switch ( errstate )
    {
        case ELINK_INVALID_PARA:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_INVALID_PARA ) );
            break;
        }
        case ELINK_DEV_ID_NO_EXSIT:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_ID_NO_EXSIT ) );
            break;
        }
        case ELINK_DEV_PIN_ERROR:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_PIN_ERROR ) );
            break;
        }
        case ELINK_DEV_AUTH_ERR:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_AUTH_ERR ) );
            break;
        }
        case ELINK_DEV_NO_ONLINE:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_NO_ONLINE ) );
            break;
        }
        case ELINK_DEV_NO_STRCODE:
            {
            json_object_object_add( json_object_serial_value, ELink_JSON_ErrorCode,
                                    json_object_new_int( ELink_errInfo_NO_STRCODE ) );
            break;
        }
        default:
            break;
    }

    json_object_object_add( json_object_serial_value, ELink_JSON_ErrorInfo,
                            json_object_new_string( ELink_JSON_online_OK ) );
    json_object_object_add( json_object_serial_value, ELink_JSON_ErrorTime,
                            json_object_new_int( time( 0 ) ) );

    jsonString = json_object_to_json_string( dev_object );
    json_object_put( dev_object );/*free memory*/
    json_object_put( json_object_serial_value );/*free memory*/
    dev_object = NULL;
    json_object_serial_value = NULL;
    elink_log("jsonString : %s",jsonString);
    return elink_get_pack_full( ELink_Code_StatusFault, jsonString, NULL, token );
}
/**
 * 子设备执行结果上报
 */
char* elink_get_pack_subdev_OperastionStatusReport( char* token, char *subdevid, uint8_t operstion,
                                                    char *outReqSequence )
{
    unsigned char temp[32];
    char *jsonString = NULL;
    struct json_object *dev_object = NULL;

    dev_object = json_object_new_object( );
    sprintf( temp, "%d", st_get_req_sequence( ) );
    if ( outReqSequence )
        strcpy( outReqSequence, temp );
    json_object_object_add( dev_object, ELink_JSON_sequence, json_object_new_string( temp ) );
//    json_object_object_add(dev_object, ELink_JSON_deviceId, json_object_new_string(g_elink_devid));
    json_object_object_add( dev_object, ELink_JSON_deviceId, json_object_new_string( subdevid ) );
//  json_object_object_add(dev_object, "pin", json_object_new_string(pin));
    json_object_object_add( dev_object, ELink_JSON_time, json_object_new_int( time( 0 ) ) );
    if ( operstion > 0 )
        json_object_object_add( dev_object, ELink_JSON_OnLine,
                                json_object_new_string( ELink_JSON_online_OK ) );
    else
        json_object_object_add( dev_object, ELink_JSON_OnLine,
                                json_object_new_string( ELink_JSON_result_OK ) );

    jsonString = json_object_to_json_string( dev_object );
    json_object_put( dev_object );/*free memory*/
    dev_object = NULL;
    return elink_get_pack_full( ELink_Code_StatusReport, jsonString, NULL, token );
}

//////////////////////////////////////////////////////////////////////////////////////////
