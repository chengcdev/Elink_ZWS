/*
 * MLinkPublic.h
 *
 *  Created on: 2017年7月13日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKPUBLIC_MLINKPUBLIC_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKPUBLIC_MLINKPUBLIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "mico.h"

#define STRTOK( str, space )                 mlink_strtok( str, space )//strtok( str, space )//mlink_strtok( str, space )
#define DISCOVER_TIMEOUT_DEFAULT            10

#define  kDuplicate                         kDuplicateErr
#define  edLinkDealOk                         1
#define  edLinkNotFoundOutput                 2
#define  edLinkNotFound                       3

/***************************************************************/
typedef enum
{
    MODULE_PUBLIC         = 0x00,     // about net
    MODULE_COAP           = 0x10,     // module coap
    MODULE_RF433          = 0x20,     // module rf433
    MODULE_ZIGBEE         = 0x30,     // module zigbee
    MODULE_CLOUD          = 0x40,     // module cloud
}MODULE_TYPE_E;

typedef enum
{
    TELECOM_NET_WIFI        = 0,
    TELECOM_RF433           = 1,
    TELECOM_ZIGBEE          = 2,
    TELECOM_BLE             = 3
}TELECOM_TYPE_E;

typedef enum{
    TELECOM_RSP_OK                           = kNoErr,
    TELECOM_RSP_EXIST                        = kDuplicateErr,
    TELECOM_RSP_NO_OBJECT                    = kNotFoundErr,
    TELECOM_RSP_ERR                          = kGeneralErr
}TELECOM_RSP_STATUS_E;


//typedef enum{
//   FUNCTION_SETMESH_ID                     = 0x00,
//   FUNCTION_SETNET_ID                      = 0x01,
//   FUNCTION_FACTORY                        = 0x02
//}FUNCTION_ID_E;

typedef enum{
    VALTYPE_CHAR            = 0,
    VALTYPE_SHORT           = 2,
    VALTYPE_INT             = 4,
    VALTYPE_MULTPARAM       = 21,       // 多参数数值类型
    VALTYPE_HEX_STR         = 50,       // HEX值类型字符串如 “1E2AB0”
    VALTYPE_DATA_STREAM     = 51,       // 数据流类型
    VALTYPE_TEXT            = 60,       // 值类型为纯文本
    VALTYPE_JSON            = 61        // 值类型为json文本格式对象
}VALTYPE_E;


typedef enum{
    MASTER_ENDPOINT         = 1,
    SUBDEV_ENDPOINT         = 2,
    DEVICE_ENDPOINT         = 3,
    SCENE_ENDPOINT          = 4,
    AREA_ENDPOINT           = 5
}ENDPOINT_TYPE_E;

//typedef enum{
//    STATE_BOOT_UP               = 1,
//    STATE_EASY_LINK             = 2,
//    STATE_NET_UNCONNECT         = 3,
//    STATE_NET_CONNECT           = 4,
//    STATE_CLOUD_CONNECT         = 5,
//    STATE_COMMUNICATION         = 6,
//    STATE_FAULT                 = 7
//}STATE_INDICATE_E;

/********************************************************
 * function:  StrToHex
 * description: convert string to hex digist
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   example: '100102' -> 0x100102  little endian
*********************************************************/
OSStatus StrToHex( char *string, uint8_t strSize, uint8_t *hex );

/********************************************************
 * function:  StrToHex
 * description: convert string to hex digist
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   example: '100102' -> {0x10 0x01 0x02}
*********************************************************/
int StrToHexEx(char *string, unsigned char strSize, unsigned char *hex);

/********************************************************
 * function: HexToStr
 * description:
 * input:       pbSrc
 *              nLen:    pbSrc contain number of bytes
 * output:      pbDest:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   example:  {0x100102} -> '100102'
*********************************************************/
void HexToStr(unsigned char *pbSrc, int nLen, char *pbDest);

/********************************************************
 * function: HexToStr
 * description:
 * input:       pbSrc
 *              nLen:    pbSrc contain number of bytes
 * output:      pbDest:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   example:  {0x10 0x01 0x02} -> '100102'
*********************************************************/
void HexToStrEx(unsigned char *pbSrc, int nLen, char *pbDest);

/*************************************************
  Function          :       mlink_get_local_id
  Description       :
  Input:
      1. str:
      2. delim:
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
char *mlink_strtok(char *str, char *delim);

/*************************************************
  Function          :       mlink_parse_ctrl
  Description       :   解析特定分隔符的一组数据
  Input:
      1. content:   需要解析的数据
      2. space:    分隔符
  Output:     1. data_array: 指向被分隔开的各个数据
              2. data_num:  被分隔开的数据个数
  Return            :
  Others            :     注：将会改变传入数据content的内容
*************************************************/
OSStatus mlink_parse_space_data(char *content, char space, char **data_array, uint8_t *data_num);

/*************************************************
  Function          :       mlink_value_wildcard_replace
  Description       :   在str中查找str1, 并用replace_str替换
  Input:
      1. str:
      2. str1:
      3. replace_str:
  Output:
  Return            :
  Others            :

*************************************************/
OSStatus mlink_value_wildcard_replace( char *str, char *str1, char *replace_str);

/*************************************************
  Function          :       mlink_parse_subdev_net_addr
  Description       :   generate endpoint id
  Input:      1. netAddr:    represent object id
  Output            :
              1. daddr:  contain panid and short address
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_subdev_net_addr( uint32_t comm, char *netAddr, uint8_t *daddr );


/*************************************************
  Function          :       mlink_generate_subdevid
  Description       :   generate subdevice id
  Input:
  Output            :
              1. devid:
  Return            :
  Others            :

*************************************************/
OSStatus mlink_generate_subdevid(char *devid);

/*************************************************
  Function          :       mlink_get_total_pages
  Description       :   获取总页数
  Input:
      1. total_size:    总记录数
      2. page_size： 一页最大加载记录数量
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
uint32_t mlink_get_total_pages( uint32_t total_size, uint32_t page_size );

/*************************************************
  Function          :       mlink_get_curpage_size
  Description       :   获取当前页总记录数
  Input:
      1. total_size:    总记录数
      2. page_size： 一页最大加载记录数量
      2. current_page： 当前页（从 1 开始）
  Output:     1. :
  Return            :
  Others            :       无

*************************************************/
uint32_t mlink_get_curpage_size( uint32_t total_size, uint32_t page_size, uint32_t current_page );

/*************************************************
  Function          :       mlink_create_rand
  Description       :   生成一个有随机种子产生的随机数
  Input:            boundary 设定随机数的取值范围
  Output:
  Return            :
  Others            :
*************************************************/
int mlink_create_rand( unsigned int boundary );

#ifdef __cplusplus
}
#endif

#endif
