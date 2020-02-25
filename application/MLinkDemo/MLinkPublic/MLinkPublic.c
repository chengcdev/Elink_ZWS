/*
 * MLinkPublic.c
 *
 *  Created on: 2017年10月12日
 *      Author: Administrator
 */
#include "mico.h"
#include "../MLinkObject.h"
#include "MLinkPublic.h"
#define os_public_log(M, ...) custom_log("MLINK_PUBLIC", M, ##__VA_ARGS__)

#define ABNORMAL_CTRL_SIZE           256

/********************************************************
 * function:  ConvertHexChar
 * description: convert char to hex digist
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   example: '1' -> 0x01
*********************************************************/
char ConvertHexChar(uint8_t ch)
{
    if((ch>='0')&&(ch<='9'))
        return ch-0x30;
    else if((ch>='A')&&(ch<='F'))
        return ch-'A'+10;
    else if((ch>='a')&&(ch<='f'))
        return ch-'a'+10;
    else return (-1);
}

/********************************************************
 * function:  StrToHex
 * description: convert string to hex digist
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   example: '100102' -> 0x100102  little endian
*********************************************************/
OSStatus StrToHex(char *string, uint8_t strSize, uint8_t *hex)
{
    int count = 0;
    unsigned char temp = 0;

    if ((string == NULL) || (hex == NULL) || (strSize == 0))
    {
        return FALSE;
    }
    for (count=0; count < strSize; count++)
    {
        temp = ConvertHexChar(*(string+strSize-1-count));
        if (count%2 != 0)
        {
            *(hex+count/2) += temp << 4;
        }
        else
        {
            *(hex+count/2) = temp;
        }
    }
    return TRUE;
}

/********************************************************
 * function:  StrToHexEx
 * description: convert string to hex digist
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   example: '100102' -> {0x10 0x01 0x02}
*********************************************************/
int StrToHexEx(char *string, unsigned char strSize, unsigned char *hex)
{
    int count=0;
    unsigned char temp = 0;
    int hexLen = 0;
    unsigned char *pOutputHex = hex;
    char *pInputStr = string;
    unsigned char size = strSize;
    if ((string == NULL) || (hex == NULL))
    {
        return -1;
    }
    if (strSize == 0)
    {
        return 0;
    }
    if (size%2)
    {
        *pOutputHex = ConvertHexChar(*pInputStr);
        pOutputHex++;
        pInputStr++;
        size--;
    }
    for (count=0; count < size; count++)
    {
        temp = ConvertHexChar(*(pInputStr+count));
        if (count%2 == 0)
        {
            *(pOutputHex+count/2) = temp << 4;
        }
        else
        {
            *(pOutputHex+count/2) += temp;
        }
    }
    hexLen = (strSize + 1) / 2;
    return hexLen;
}

/********************************************************
 * function:  HexStrCmp
 * description: compare hex string
 * input:
 * output:
 * return:
 * auther:
 * other:   compare string 'str1' and 'str2' ignore case
*********************************************************/
int HexStrCmp(char *str1, char *str2)
{
    char cmp1[32] = {0};
    char cmp2[32] = {0};
    unsigned char cmpLen = 0;
    cmpLen = StrToHexEx(str1, strlen(str1), cmp1);
    StrToHexEx(str2, strlen(str2), cmp2);
    return memcmp(cmp1, cmp2, cmpLen);
}

/********************************************************
 * function: mlink_string_convert_md5
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus mlink_string_convert_md5(unsigned char *input, int input_len, char *output, int *output_len)
{
    if (( input == NULL )||( output == NULL )||( output_len == NULL ))
    {
        return kGeneralErr;
    }
    md5_context  md5;
    InitMd5(&md5);
    Md5Update(&md5, input, (uint32_t)input_len);
    Md5Final(&md5, output);
    *output_len = 16;
    return kNoErr;
}

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
void HexToStr(unsigned char *pbSrc, int nLen, char *pbDest)
{
    char    ddl,ddh;
    int i;

    for (i=0; i<nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        pbDest[(nLen-1)*2 - i*2] = ddh;
        pbDest[(nLen-1)*2 - i*2+1] = ddl;
    }

    pbDest[nLen*2] = '\0';
}

/********************************************************
 * function: HexToStr
 * description:
 * input:       pbSrc
 *              nLen:    pbSrc contain number of bytes
 * output:      pbDest:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:   example:  {0x10 0x01 0x02} -> "100102"
*********************************************************/
void HexToStrEx(unsigned char *pbSrc, int nLen, char *pbDest)
{
    char    ddl,ddh;
    int i;

    for (i=0; i<nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;
        if (ddh > 57) ddh = ddh + 7;
        if (ddl > 57) ddl = ddl + 7;
        pbDest[i*2] = ddh;
        pbDest[i*2+1] = ddl;
    }

    pbDest[nLen*2] = '\0';
}

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
char *mlink_strtok(char *str, char *delim)
{
    static char *temp = NULL;
    char *ch = NULL;
    char *retStr = NULL;
    if (str != NULL)
    {
        temp = str;
    }
    if (temp == NULL)
    {
        return NULL;
    }
    ch = temp;
    while ((*ch != *delim)&&(*ch != '\0'))
    {
        if (*ch == (char)0xff)
        {
            return NULL;
        }
        ch++;
    }
    if (*ch == *delim)
    {
        retStr = temp;
        temp = ch+1;
        *ch = '\0';
    }
    else if (*ch == '\0')
    {
        retStr = temp;
        temp = NULL;
    }
    return retStr;
}

/*************************************************
  Function          :       mlink_get_local_id
  Description       :
  Input:
      1. devId:    it points to string
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
void mlink_get_local_id(char *devid)
{
    NETDEVOBJ_T netDevObj;

    if (devid == NULL)
    {
        return kGeneralErr;
    }
    storage_read_local_devobj(&netDevObj);
    strcpy(devid, netDevObj.uuid);
}

/*************************************************
  Function          :       mlink_get_local_id
  Description       :
  Input:
      1. devId:    it points to string
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
void mlink_get_local_addr(char *panid)
{
    NETDEVOBJ_T netDevObj;

    if (panid == NULL)
    {
        return kGeneralErr;
    }
    storage_read_local_devobj(&netDevObj);
    strcpy(panid, netDevObj.addr);
}

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
uint32_t mlink_get_total_pages( uint32_t total_size, uint32_t page_size )
{
    uint32_t totalPages = (total_size + page_size - 1) / page_size;
    return totalPages;
}

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
uint32_t mlink_get_curpage_size( uint32_t total_size, uint32_t page_size, uint32_t current_page )
{
    uint32_t fullNum = current_page*page_size;
    uint32_t currentSize = 0;
    if (fullNum <= total_size)
    {
        currentSize = page_size;
    }
    else
    {
        currentSize = total_size%page_size;
    }
    return currentSize;
}


/*************************************************
  Function          :       mlink_generate_endpoint_id
  Description       :   generate endpoint id
  Input:
      1. dobjId:    represent object id
      2. devId:    it points to string
      3. key: point to string
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_generate_endpoint_id(OBJECT_ID_E objId, char *devId, char *key, char *endpointId)
{
    if (endpointId == NULL || devId == NULL)
    {
        return kGeneralErr;
    }
    sprintf(endpointId, "%d_%s", objId, devId);
    if (key != NULL)
    {
        sprintf(endpointId, "%s_%s", endpointId, key);
    }

    return kNoErr;
}

/*************************************************
  Function          :       mlink_generate_endpointscene_id
  Description       :   generate endpoint id
  Input:
      1. dobjId:    represent object id
      2. devId:    it points to string
      3. key: point to string
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_generate_endpointscene_id(OBJECT_ID_E objId, char *key, char *endpointId)
{
    if (endpointId == NULL)
    {
        return kGeneralErr;
    }
    sprintf(endpointId, "%d", objId);
    if (key != NULL)
    {
        sprintf(endpointId, "%s_%s", endpointId, key);
    }
    return kNoErr;
}

/*************************************************
  Function          :       mlink_parse_endpointid
  Description       :   generate endpoint id
  Input:
      1. dobjId:    represent object id
      2. devId:    it points to string
      3. key: point to string
  Output:     1. endpointId:
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_endpointid(char *endpoint_id, PENDPOINT_ELE_T pendpoint_ele)
{
    char endpointId[ENDPOINTID_SIZE] = {0};
    char *elementPoint = NULL;
    char delim = '_';

    if (endpoint_id == NULL || pendpoint_ele == NULL)
    {
        return kGeneralErr;
    }
    memset(pendpoint_ele, 0, sizeof(ENDPOINT_ELE_T));
    strcpy(endpointId, endpoint_id);
    elementPoint = STRTOK(endpointId, &delim);
    pendpoint_ele->classId = (uint16_t)atoi(elementPoint);
    elementPoint = STRTOK(NULL, &delim);
    strcpy(pendpoint_ele->devid, elementPoint);
    elementPoint = STRTOK(NULL, &delim);
    if (elementPoint != NULL)
    {
        strcpy(pendpoint_ele->key, elementPoint);
    }
//    os_public_log("Parse endpoint. \r\nclassid: %d, key: %s, devid: %s", pendpoint_ele->classId, pendpoint_ele->key, pendpoint_ele->devid);
    return kNoErr;
}

/*************************************************
  Function          :       mlink_parse_subdev_net_addr
  Description       :   generate endpoint id
  Input:      1. netAddr:    represent object id
              2. comm:   zigbee or rf433
  Output            :
              1. daddr:  contain panid and short address
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_subdev_net_addr( uint32_t comm, char *netAddr, uint8_t *daddr )
{
    uint8_t len = strlen(netAddr);
    uint8_t count = 0;
    char panidStr[4] = {0};
    char shortAddrStr[8] = {0};


    if (netAddr == NULL || daddr == NULL)
    {
        return kGeneralErr;
    }

    for (count=0; count < len; count++)
    {
        if (*(netAddr+count) == '.')
        {
            break;
        }
    }
    memcpy(panidStr, netAddr, count);
    memcpy(shortAddrStr, netAddr+count+1, strlen(netAddr+count+1));
    if (comm == TELECOM_ZIGBEE)
    {
        uint8_t panid = 0;
        uint16_t shortaddr = 0;
        StrToHex(panidStr, strlen(panidStr), &panid);
        StrToHex(shortAddrStr, strlen(shortAddrStr), (uint8_t *)&shortaddr);
        memcpy(daddr, &shortaddr, 2);
        daddr[2] = panid;
    }
    else
    {
        uint32_t shortaddr = 0;
        StrToHex(shortAddrStr, strlen(shortAddrStr), (uint8_t *)&shortaddr);
        memcpy(daddr, (uint8_t *)&shortaddr, 3);
    }

    return kNoErr;
}

/*************************************************
  Function          :       mlink_generate_subdev_net_addr
  Description       :   generate device net address
  Input:      1. addr
  Output            :
              1. net_addr
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_generate_subdev_net_addr( uint32_t addr, char *net_addr )
{
    NETDEVOBJ_T netDevObj = {0};

    if ( net_addr == NULL )
    {
        return kGeneralErr;
    }

    storage_read_local_devobj(&netDevObj);
    sprintf(net_addr, "%s.%06X", netDevObj.addr, addr);

    return kNoErr;
}

/*************************************************
  Function          :       mlink_generate_subdevid
  Description       :   generate subdevice id
  Input:
  Output            :
              1. devid:
  Return            :
  Others            :

*************************************************/
OSStatus mlink_generate_subdevid(char *devid)
{
    char netDevId[UUID_SIZE] = {0};
    int num = 0;
    mlink_get_local_id(netDevId);
    num = storage_get_object_num(DEVICE_OBJ_ID);
    sprintf(devid, "%s-%d", netDevId, num+1);

    return kNoErr;
}


/*************************************************
  Function          :       mlink_convert_utc_to_rtc
  Description       :
  Input:
  Output            :
  Return            :
  Others            :
*************************************************/
OSStatus mlink_convert_utc_to_rtc( mico_utc_time_t *utc_time, mico_rtc_time_t *rtc_time)
{
    struct tm *     currentTime;

    currentTime = localtime( (const time_t *)utc_time );
    rtc_time->sec = currentTime->tm_sec;
    rtc_time->min = currentTime->tm_min;
    rtc_time->hr = currentTime->tm_hour;

    rtc_time->date = currentTime->tm_mday;
    rtc_time->weekday = currentTime->tm_wday;
    rtc_time->month = currentTime->tm_mon + 1;
    rtc_time->year = (currentTime->tm_year + 1900) % 100;
    os_public_log("%d.%d.%d %d:%d:%d", rtc_time->year, rtc_time->month, rtc_time->date, rtc_time->hr, rtc_time->min, rtc_time->sec);
    return true;
}

/*************************************************
  Function          :       MLink_set_system_time
  Description       :
  Input:
  Output            :
  Return            :
  Others            :
*************************************************/
OSStatus MLink_set_system_time(mico_utc_time_t *utc_time)
{
    mico_rtc_time_t rtc_time;

    mlink_convert_utc_to_rtc(utc_time, &rtc_time);
    MicoRtcSetTime( &rtc_time );
}

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
  示例： content = “1|2|3” 分割符为space = ‘|’  那么 data_array的数据分别为 1、2、3; data_num为 3
*************************************************/
OSStatus mlink_parse_space_data(char *content, char space, char **data_array, uint8_t *data_num)
{
    char delim = space;
    char *dataTemp = NULL;
    uint8_t count = 0;
    if ((content == NULL)||(data_array == NULL)||(data_num==NULL))
    {
        return kGeneralErr;
    }
    do{
        if (count == 0)
        {
            dataTemp = STRTOK(content, &delim);
        }
        else
        {
            dataTemp = STRTOK(NULL, &delim);
        }
        if (dataTemp){
            *(data_array+count) =dataTemp;
        }

        count++;
    }while (dataTemp);
    *data_num = count-1;
    return kNoErr;
}


/*************************************************
  Function          :       mlink_parse_ctrl_content
  Description       :   generate endpoint id
  Input:
      1. ctrl_content:    represent object id
  Output:     1. ctrl_array:
              2. array_num
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_ctrl_content(char *ctrl_content, char **ctrl_array, uint8_t *array_num)
{
    return mlink_parse_space_data(ctrl_content, ',', ctrl_array, array_num);
}

/*************************************************
  Function          :       mlink_parse_ctrl
  Description       :   generate endpoint id
  Input:
      1. ctrl:    represent object id
  Output:     1. param_array:
              2. param_num
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_ctrl(char *ctrl, char **param_array, uint8_t *param_num)
{
    return mlink_parse_space_data(ctrl, '|', param_array, param_num);
}

/*************************************************
  Function          :       mlink_parse_state_array_content
  Description       :   generate endpoint id
  Input:
      1. ctrl:    represent object id
  Output:     1. param_array:
              2. param_num
  Return            :
  Others            :       无

*************************************************/
OSStatus mlink_parse_state_array_content(char *content, char **state_array, uint8_t *state_num)
{
    return mlink_parse_space_data(content, ',', state_array, state_num);
}

/*************************************************
  Function          :       mlink_parse_state_content
  Description       :   generate endpoint id
  Input:
      1. content:
  Output:     1. member:   状态成员指针数组
              2. member_num:     状态成员数量
  Return            :
  Others            :       无
*************************************************/
OSStatus mlink_parse_state_content(char *content, char **member, uint8_t *member_num)
{
    return mlink_parse_space_data(content, '|', member, member_num);
}

/*************************************************
  Function          :       mlink_parse_state_value
  Description       :
  Input:
      1. content:
  Output:     1. value:
              2. num:
  Return            :
  Others            :       无
*************************************************/
OSStatus mlink_parse_state_value(char *content, char **value, uint8_t *num)
{
    return mlink_parse_space_data(content, ':', value, num);
}


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
OSStatus mlink_value_wildcard_replace( char *str, char *str1, char *replace_str)
{
    uint8_t len = 0;
    uint8_t stepCount = 0;
    uint8_t stepLen = 2;        // “FF” 为字节数为2
    uint8_t stepNum = 0;
    uint8_t replaceLen = 0;
    char *pTemp = str;
    if ((str == NULL)||(replace_str == NULL)||(str1 == NULL))
    {
        return kGeneralErr;
    }
    replaceLen = strlen(replace_str);
    len = strlen(str);
    stepNum = (len - (replaceLen - stepLen))/stepLen; // 总需检查的次数

    // 循环匹配每移动一个步长即两个字节的匹配
    for (stepCount = 0; stepCount<stepNum; stepCount++)
    {
        pTemp = str + stepCount*stepLen;

        if(!memcmp(str1, pTemp, replaceLen))
        {
            memcpy(pTemp, replace_str, replaceLen);
            stepCount += replaceLen/2;
            continue;
        }
    }
    return kNoErr;
}

/***************************************************************
  Function          :       mlink_value_cmp
  Description       :   比较字符串是否匹配
  Input:
      1. compare1:      输入值
      2. compare2:      条件值( 可能包含通配字符串 )
      3. wildcard: 通配字符串
      4. wildcard1: 特殊匹配符
  Output:     1. str3:  str1中与通配字符串相对应的字符串
              2. str4:  最终通配字符格式
              3. str5: 特殊匹配符对应的
  Return  :
  Others  :  该函数只支持连续存在通配字符串的情况
****************************************************************/
OSStatus mlink_value_cmp(char *compare1, char *compare2, char *wildcard, char *wildcard1, char *str3, char *str4, char *str5)
{
    uint8_t len1 = 0;
    uint8_t len2 = 0;
    uint8_t stepCount = 0;
    uint8_t stepLen = 2;
    uint8_t wcLen = 0;
    uint8_t start = 0xFF;
    uint8_t len = 0;        // 使用通配字符串的数据字符串总长度
    uint8_t stepNum = 0;
    int ret = 0;
    char *pTemp = NULL;
    char *str1 = compare1;
    char *str2 = compare2;
    int count = 0;

    if (str1==NULL || str2==NULL)
    {
        os_public_log("str1=NULL or str2==NULL !!!");
        return kGeneralErr;
    }
    ret = strcmp(str1, str2);
    if (ret == 0)       // 字符串相等则立刻返回
    {
//        os_public_log("str1=str2 !!!");
        return ret;
    }

    // 如果通配字符串为空指针，则直接比较两个字符串
    if ((wildcard == NULL) || (str3 == NULL) || (str4 == NULL))
    {
        os_public_log("(wildcard == NULL) || (str3 == NULL) || (str4 == NULL)");
        return ret;
    }
    else    // 若不为空，则只要是遇到通配字符串, 则在比较的时候不考虑通配符所在位置的值是否一致
    {

        len1 = strlen(str1);
        len2 = strlen(str2);
        if (len1 < len2)
        {
//            os_public_log("len1 != len2");
            return kGeneralErr;
        }
        else if (len1 > len2)
        {
            len1 = len2;
        }

        wcLen = strlen(wildcard);
        stepNum = len1/stepLen;
        // 循环匹配每移动一个步长即两个字节的匹配
        for (stepCount = 0; stepCount < stepNum; stepCount++)
        {
            pTemp = str2 + stepCount * stepLen;
//                os_public_log("wildcard:%s, pTemp:%s", wildcard, pTemp);
            if(!memcmp(wildcard, pTemp, wcLen))
            {
                if (0xFF == start)
                {
                    start = pTemp-str2;
//                        os_public_log("start:%d", start);
                }
                len += wcLen;
            }
            else
            {
                if (0xFF != start)
                {
                    break;
                }
            }
        }
        if (start == 0xFF)          // input value isn't match conditon
        {
//                os_public_log("can't match!! ret = %d", ret);
            return ret;
        }
        memcpy(str4, str2+start, len);
        memcpy(str2+start, str1+start, len);
            os_public_log("str1:%s, str2:%s, str4:%s", str1, str2, str4);
        // 替换FF为实际值后进行比较
        for (count = 0; count < len1; count++)
        {
            if (*(str1+count)==*(str2+count) || (*(str2+count)=='-') || (*(str2+count)==*wildcard1))
            {
                if (*(str2+count)==*wildcard1)
                {
                    *str5 = *(str1+count);
                    count++;
                    *(str5+1) = *(str1+count);
                }
            }
            else
            {
                break;
            }
        }
        if (count >= len1)
        {
            memcpy(str3, str1+start, len);
            ret = 0;
        }
        return ret;
    }
}

/*************************************************
  Function          :       mlink_translate_value
  Description       :   mlink协议控制值转sub协议控制数据
  Input:

  Output:     1. str3:
  Return            :
  Others            :
*************************************************/
OSStatus mlink_translate_value(int val_type, char *val, uint8_t *data, uint8_t *size)
{
    if (( val == NULL ) ||( data == NULL )||( size == NULL ) || ( *val == 0 ))
    {
        *size = 0;
        return kGeneralErr;
    }
    switch (val_type)
    {
        case VALTYPE_CHAR:
        {
            *data = (uint8_t)atoi(val);
            *size = 1;
        }
            break;
        case VALTYPE_SHORT:
        {
            uint16_t value = 0;
            value = (uint16_t)atoi(val);
            memcpy(data, &value, 2);
            *size = 2;
        }
            break;
        case VALTYPE_INT:
        {
            uint32_t value = (uint32_t)atoi(val);
            memcpy(data, &value, 4);
            *size = 4;
        }
            break;
        case VALTYPE_MULTPARAM:
        {
            char delim = '|';
            char *byte[32] = {0};
            uint8_t count = 0;
            mlink_parse_space_data(val, delim, byte, size);
            for (count=0; count<size; count++)
            {
                *(data+count) = (uint8_t)atoi(byte[count]);
            }
        }
            break;
        case VALTYPE_HEX_STR:
        {
            *size = StrToHexEx(val, strlen(val), data);
        }
            break;
        case VALTYPE_DATA_STREAM:
        {

        }
            break;
        default:
            break;
    }
    return kNoErr;
}

/*************************************************
  Function          :       mlink_reverse_translate_value
  Description       :   sub协议控制数据转mlink协议控制值
  Input:        1.  data:
                2.  size:
  Output:     1. val_type:
              2. value:
  Return            :
  Others            :
*************************************************/
OSStatus mlink_reverse_translate_value( uint8_t *data, uint8_t size, int *val_type, char *value )
{
    if (( value == NULL ) ||( data == NULL )||( val_type == NULL ))
    {
        return kGeneralErr;
    }

    switch (size)
    {
        case 1:
        {
            sprintf(value, "%d", *data);
            *val_type = VALTYPE_CHAR;
        }
            break;
        case 2:
        {
            sprintf(value, "%d", *(uint16_t *)data);
            *val_type = VALTYPE_SHORT;
        }
            break;
        case 3:
        case 4:
        {
            sprintf(value, "%d", *(uint32_t *)data);
            *val_type = VALTYPE_INT;
        }
            break;
        default:
        {
            HexToStrEx(data, size, value);
            *val_type = VALTYPE_HEX_STR;
        }
            break;
    }
    return kNoErr;
}

/*************************************************
  Function          :       mlink_create_rand
  Description       :   生成一个有随机种子产生的随机数
  Input:            boundary 设定随机数的取值范围
  Output:
  Return            :
  Others            :
*************************************************/
int mlink_create_rand( unsigned int boundary )
{
    static unsigned char seedflag = 0;
    unsigned int randtimer = 0;
    if (0 == seedflag)  // there is no rand seed
    {
        time_t time;
        mico_time_get_utc_time((mico_utc_time_t *)&time);
        srand(time);
        seedflag = 1;
    }
    randtimer = rand()%boundary;
    return randtimer;
}

