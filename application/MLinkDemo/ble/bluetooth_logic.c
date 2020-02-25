/*
 * bluetooth_logic.c
 *
 *  Created on: 2018年9月16日
 *      Author: Administrator
 */

#include "bluetooth_logic.h"

#include "../../../mico-os/include/common.h"
#include "../../../mico-os/include/mico_rtos.h"
#include "../../../mico-os/include/MiCODrivers/MiCODriverRtc.h"
#include "../../../mico-os/libraries/utilities/json_c/json_object.h"
#include "../flash/flash_storage_object.h"
#include "../MLinkObject.h"

#define UNLOCK_TIMER_PERIOD 5 // 等待应答超时时间 S
#define SET_TIMER_PERIOD 6    // 等待应答超时时间 S

#define MAX_RECORD_COUNT 5 // 一次最多获取十包记录
#define RECORD_CMD_LEN 18  // 一条记录命令长度

#define BLE_CMD_LEN (64)
#define MAX_PACKET_LEN 20 // 每包数据最大长度

#define SAVE_POWER_MODE 10  // 省电模式值
#define NORMAL_POWER_MODE 5 // 正常模式值


#define os_bluetooth_log(M, ...) custom_log("BLE_LOG", M, ##__VA_ARGS__)


static int errCnt = 0;
#define os_bluetooth_log_trace() custom_log_trace("UART RECV")
#define DEBG_ON  0
#define ble_return_val_if_fail(_expr_, _ret_)                               \
    if (!(_expr_))                                                          \
    {                                                                       \
        os_bluetooth_log("%s [%d] return fail \n", __FUNCTION__, __LINE__); \
        return (_ret_);                                                     \
    }


static uint8_t get_record_success = 0;
static uint8_t send_touchuan_flag = 0;
typedef struct
{
    uint8_t send_type;     //类型 0:at指令 1:透传命令
    uint8_t send_flag;     //命令分类 0:无命令 1:其他指令 2:扫描指令 3:连接、设置主从模式、恢复出厂指令
    uint16_t cmd_len;      //命令长度
    char cmd[BLE_CMD_LEN]; //命令文本
} ATcmd, *PATcmd;

struct ATCmdIndex
{
    AT_CMD_E cmd;
    unsigned char *ucpCmdPrefix;
};

static const struct ATCmdIndex ATCmdTable[] =
    {
        /*================常用指令==================*/
        {AT_TEST, "AT+"},       // 测试
        {AT_HELP, "AT+HELP"},   // 帮助查询
        {AT_VERS, "AT+VERS"},   // 软件版本查询
        {AT_NAME, "AT+NAME"},   // 查询/设置模块名称，最长允许15个字符
        {AT_RENEW, "AT+RENEW"}, // 恢复出厂设置
        {AT_RESET, "AT+RESET"}, // 重启模块
        {AT_ROLE, "AT+ROLE"},   // 查询/设置主从模式
        {AT_NOTI, "AT+NOTI"},   // 查询/设置是否把当前连接状态通知给用户
        {AT_IMME, "AT+IMME"},   // 查询/设置模块工作方式
        {AT_START, "AT+START"}, // 开始工作
        {AT_ADST, "AT+ADST"},   // 打开广播
        {AT_R, "AT+R"},         // 远端控制命令

        /*================串口指令==================*/
        {AT_BAUD, "AT+BAUD"}, // 查询/设置波特率
        {AT_FLOW, "AT+FLOW"}, // 查询/设置硬件流控
        {AT_PARI, "AT+PARI"}, // 查询/设置串口校验
        {AT_STOP, "AT+STOP"}, // 查询/设置停止位

        /*================从机指令==================*/
        {AT_ADVI, "AT+ADVI"}, // 查询/设置广播时间间隔
        {AT_POWE, "AT+POWE"}, // 查询/设置模块发射功率

        /*================主机指令==================*/
        {AT_SCAN, "AT+SCAN"},   // 搜索可连接模块
        {AT_RANG, "AT+RANG"},   // 设置扫描参数
        {AT_SHOW, "AT+SHOW"},   // 查询/设置模块在手动搜索时是否返回名字
        {AT_CON, "AT+CON"},     // 连接指定蓝牙MAC地址的从模块
        {AT_CONM,"AT+CON"},     // 连接指定的MAC地址的从模块
        {AT_CONN, "AT+CONN"},   // 连接搜索返回的模块
        {AT_CONNL, "AT+CONNL"}, // 连接最后一次连接成功的从模块
        {AT_SNAME, "AT+SNAME"}, // 扫描时显示设备名
        {AT_SRSSI, "AT+SRSSI"}, // 扫描时显示信号强度

        /*==============连接相关指令================*/
        {AT_ISCON, "AT+ISCON"},   // 查询当前模块是否处于连接状态
        {AT_DISCON, "AT+DISCON"}, // 断开连接
        {AT_DISC, "AT+DISC"},     // 断开连接
        {AT_LADDR, "AT+LADDR"},   // 断开连接
        {AT_CLEAR, "AT+CLEAR"},   // 清除模块配对信息
        {AT_RADD, "AT+RADD"},     // 查询成功连接过的远程主机地址
        {AT_SAVE, "AT+SAVE"},     // 查询/设置模块成功连接后是否保存连接地址

        /*==============安全相关====================*/
        {AT_TYPE, "AT+TYPE"}, // 查询/设置模块密码验证类型
        {AT_PWDM, "AT+PWDM"}, // 查询/设置模块登入密码
        {AT_PWD, "AT+PWD"},   // 登入设备

        /*============模块信息相关指令==============*/
        {AT_PASS, "AT+PASS"}, // 查询/设置配对密码
        {AT_UADT, "AT+UADT"}, // 查询/设置用户自定义数据
        {AT_MAC, "AT+MAC"},   // 查询本机MAC地址
        {AT_RSSI, "AT+RSSI"}, // 读取 RSSI 信号值
        {AT_TEMP, "AT+TEMP"}, // 查询模块温度

        /*===============IO监控指令================*/
        {AT_LED, "AT+LED"},   // 查询/设置LED输出状态
        {AT_PWM, "AT+PWM"},   // 查询/设置PWM输出状态
        {AT_PDIR, "AT+PDIR"}, // 查询/设置PIO口的输入输出方向
        {AT_PDAT, "AT+PDAT"}, // 查询/设置PIO口的输入输出状态

        /*=============电源管理指令================*/
        {AT_PWRM, "AT+PWRM"},   // 查询/设置模块进入自动休眠的时间
        {AT_SLEEP, "AT+SLEEP"}, // 让模块进行休眠状态
        {AT_WAKE, "AT+WAKE"},   // 将模块从休眠状态唤醒
        {AT_BATC, "AT+BATC"},   // 查询/设置电量监控开关
        {AT_BATT, "AT+BATT"},   // 查询电量信息
        { AT_STAS, "AT+STAS" },                     // 查询设备状态
};

typedef enum
{
    BLE_AT_WAR,
    BLE_AT_OK,
    BLE_AT_TUNNEL,
    BLE_AT_ERROR,
    BLE_AT_OTHER,
} BLE_AT_TYPE;

#define MAX_ATCMDLIST_SZIE 15
static ATcmd mATcmdList[MAX_ATCMDLIST_SZIE];
static uint8_t mPutIndex = 0;
static uint8_t mGetIndex = 0;

static uint8_t g_SendInitFlag = 0;
static uint8_t g_DealInitFlag = 0;
static uint8_t g_MutexFlag = 0;

static RecvPacket *g_RecvPacket = NULL;
static RecvPacket g_PacketBuf;

#define UART_BUF_SIZE (128)                 // 串口接收缓冲区大小
static char ucRecvBuf[UART_BUF_SIZE] = {0}; // 串口的接收缓冲区static uint32_t g_ucRecvLen = 0;
static uint32_t g_ucRecvLen = 0;

static uint8_t g_tunnelFlag = 0;

static unsigned char g_PrivateKey[32] = {""};
static unsigned char g_LoginPwd[16] = {""};
//static char g_lockMac1[3][32] = {{"4D5400001253"},{"4D5400001270"},{"4D4C00000725"}};//"4D4C00000844"
#if(TEST == 1)
static char Bindindex = 2;
#elif(TEST == 2)
static char Bindindex = 1;
#elif(TEST == 3)
static char Bindindex = 0;
#endif

static char g_lockMac1[3][32] = {{"4D4C00000844"},{"4D4C00000844"},{"4D4C00000844"}};//"4D4C00000844" test
static char g_lockMac[32] = { };
static char g_lockMacbak[32] = { };
static char ssiValue[11][3] = {{"23"},{"26"},{"18"},{"20"},{"22"},{"15"},{"18"},{"21"},{"33"},{"13"}};
static unsigned char g_tempUserPin[9] = {0};
static unsigned char g_subpin[9] = {0}; //子设备随机产生的PIN码
static uint8_t g_tempBroadrate = 0;
static unsigned char g_UserPinbak[9] = {0};
static uint8_t g_PwdEcho = FALSE;
static uint8_t g_IsCheck = 0;
static uint8_t g_ScanState = 0;
static uint8_t g_BleState = 0;
static uint8_t g_scanTimes = 0;
static uint8_t g_bindSate = 0;
static uint8_t g_AtCommNoEcho = 0; // AT指令没有应答数

static uint16_t g_usrid = 0;         //用户id
static uint8_t g_lockpwd[5] = {0};   //开锁密码
static uint8_t g_adminpwd[5] = {0};  //管理员密码
static mico_rtc_time_t effect_time;  //生效时间
static mico_rtc_time_t invalid_time; //失效时间
static unsigned char g_cnt = 0;      //可选次数
static uint8_t g_locktype = 0;       //开锁类型
static uint32_t g_rssiFlag = 0;
static PBLE_STATE_CALLBACK g_BleCallBackFunc = NULL;
static uint32_t g_bindtimecout = 0;  //绑定超时
static uint32_t g_systime = 0;    //it will resend if receive "Link:N" quickly after receive "Link:Y"
static uint8_t g_atDiscFlag = 0;  //it will reset ble-module if not receive "Link:N" after receive "OK+DISC"
static uint8_t g_connectFlag = 0; //it will resend if not receive "OK+CON:" after SEND "AT+CON"
static uint8_t g_linkyFlag = 0;   //it will resend if not receive "Link:Y" after receive "OK+CON:"
static uint8_t g_ble_state = 0;    // 0空闲1不空闲
static uint8_t g_fist = 0;

int ble_force_get_lock_list(uint8_t flag);

static int deal_resend(void);
static int send_touchuan_data(unsigned char cmd, unsigned char echo, char *data, unsigned int len);
static char rssiValueindex = 0;
static int ssivaluecount = 1;
static char g_linknoflag = 1;   // 断开连接的标志
static char g_finishflag = 0;  // 初始化完成标
static char g_add_del_tmp_pass = 0;
static char g_del_tmp_pass = 0;
static unsigned int IdleCnt = 0;
static unsigned int TimeoutCnt = 0;


static mico_timer_t _ble_mesh_timer;
static bool _ble_mesh_timer_initialized = false;


static char g_bind_status = 0;
/*********************************************************
 * function: elink_creat_rand_pin
 * description:   elink_get_rand_pin
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
 *********************************************************/
void elink_get_rand_pin(void)
{
    int pin;
    pin = mlink_create_rand(ELINK_BOUNDARY);
    memset(g_subpin, 0, sizeof(g_subpin));
    sprintf(g_subpin, "%0d", pin);
    os_bluetooth_log("subpinlen : %d ", strlen(g_subpin));
    if (strlen(g_subpin) < 8)
    {
        sprintf(g_subpin, "%s", DEFAULT_USERPIN);
    }
    os_bluetooth_log("g_subpin : %s", g_subpin);
}
/*********************************************************
 * function: PrintfByteHex
 * description:   打印函数
 * input:
 * output:
 * return:   NULL
 * auther:
 * other:
 *********************************************************/
static void PrintfByteHex(char *DesStr, uint8_t *data, uint32_t dataLen)
{
#if DEBG_ON
    int Count = 0;
    if (DesStr == NULL || data == NULL)
    {
        printf("%s: point is null, fail to print\r\n", __FUNCTION__);
        return;
    }
    os_bluetooth_log("%s:", DesStr);
    for (Count = 0; Count < dataLen; Count++)
    {

        printf("%02x ", *(data + Count));
    }
    printf("\n");
#endif
}

/*************************************************
 Function:     send_touchuan_mult_data
 Description:  透传多帧数据到蓝牙模块
 Input:
 1.cmd       命令码 注意应答命令高位为1
 2.echo      应答标志位 1应答 0主发
 3.data      参数数据
 4.len       参数长度
 Output:       无
 Return:       实际发送长度
 Others:
 Byte0 BYTE1   BYTE2   BYTE3   BYTE4….BYTEn-1  BYTEn
 起始位    长度  帧数  命令  参数          校验
 *************************************************/
int send_touchuan_mult_data(unsigned char cmd, unsigned char echo, unsigned int PacketNum,
                            char *data,
                            unsigned int len)
{
    int i, j, sum = 0;
    unsigned char buf[MAX_PACKET_LEN];
    //PrintfByteHex("recv data", data, len);
    if (g_MutexFlag == 1)
    {
        os_bluetooth_log(" send_touchuan_data: g_MutexFlag==1, return.\n");
        return BLE_ERROR;
    }
    g_MutexFlag = 1;

    unsigned int PacketLen = 0; // 每包数据长度
                                //    unsigned int PacketNum = 1;     // 包数
    unsigned int LastLen = 0;   // 最后一包数据 参数长度

    if (data == NULL || len == 0)
    {
        PacketNum = 1;
        LastLen = 0;
    }
    else
    {
        PacketNum = ((len % (MAX_PACKET_LEN - 5)) ? (len / (MAX_PACKET_LEN - 5) + 1) : (len / (MAX_PACKET_LEN - 5)));
        LastLen = ((len % (MAX_PACKET_LEN - 5)) ? (len % (MAX_PACKET_LEN - 5)) : (MAX_PACKET_LEN - 5));
    }

    //os_bluetooth_log("DataLen: %d  PacketNum: %d   LastLen: %d\n", len, PacketNum, LastLen);
    if (PacketNum > 0xFF)
    {
        PacketNum = 0xFF;
    }
    for (j = 0; j < PacketNum; j++)
    {
        memset(buf, 0, sizeof(buf));
        buf[0] = 0xAA;
        buf[2] = 1; //(PacketNum-1) - j;//代表有多包数据发送
        os_bluetooth_log("j: %d  buf[2]: %d\n", j, buf[2]);
        if (buf[2] == 0) // 最后一包
        {
            PacketLen = LastLen + 5;
        }
        else
        {
            PacketLen = LastLen + 5;
        }
        buf[1] = PacketLen - 2;
        if (echo == 0)
        {
            buf[3] = cmd & 0x7F;
        }
        else
        {
            buf[3] = cmd | 0x80;
        }
        if (data && (PacketLen - 5) > 0)
        {
            memcpy(buf + 4, data + (j * (MAX_PACKET_LEN - 5)), PacketLen - 5);
        }

        // 计算校验和
        sum = 0;
        for (i = 1; i < PacketLen - 1; i++)
        {
            sum += *(buf + i);
        }
        *(buf + PacketLen - 1) = sum;

        //os_bluetooth_log("port3_send data  len is %d !!\n",PacketLen);
        for (i = 0; i < PacketLen; i++)
        {
            //os_bluetooth_log("port3_send data[%d]  is %02X !!\n",i,buf[i]);
        }

        mATcmdList[mPutIndex].cmd_len = PacketLen;
        mATcmdList[mPutIndex].send_type = 1;
        memset(mATcmdList[mPutIndex].cmd, 0, sizeof(mATcmdList[mPutIndex].cmd));
        memcpy(mATcmdList[mPutIndex].cmd, buf, PacketLen);
        os_bluetooth_log("ATCmdSend to mATcmdList[%d].cmd is %s  len is  %d\n", mPutIndex, mATcmdList[mPutIndex].cmd, PacketLen);
        mATcmdList[mPutIndex].send_flag = 1;
        mPutIndex++;
        if (mPutIndex == MAX_ATCMDLIST_SZIE)
        {
            mPutIndex = 0;
        }
    }
    g_MutexFlag = 0;

    return BLE_OK;
}
/*************************************************
 Function:     creat_login_pwd
 Description:  生成登入密码
 Input:        无
 Output:       无
 Return:       密码长度
 Others:
 *************************************************/
static int creat_login_pwd(unsigned char *pRandPwd)
{
    unsigned char val[3] = {0};
    unsigned char i, sum = 0;
    char verify[20] = {""};
    char UserPin[9];
    char *pUserPin = (char *)storage_get_present_userpin();
    memset(UserPin, 0, sizeof(UserPin));
    if (pUserPin && strlen(pUserPin) == 8)
    {
        sprintf(UserPin, "%s", pUserPin);
    }
    else
    {
        sprintf(UserPin, "%s", DEFAULT_USERPIN);
    }

    /*
     【校验值2位 规则】：
     UsePin+RandPwd合并后16个字节字符串，不进位累加求和，得到一个字节数如：0x3F,
     则校验值为："3F"。
     【生成规则】
     RandPwd1-6 +校验值（2位）
     */

    // 计算校验值
    memset(verify, 0, sizeof(verify));
    sprintf(verify, "%s%s", UserPin, pRandPwd);
    os_bluetooth_log("verift: %s\n", verify);
    sum = 0;
    for (i = 0; i < 16; i++)
    {
        sum = sum + verify[i];
    }
    HexToStr(&sum, 1, val);

    // 生成登入密码
    memset(g_LoginPwd, 0, sizeof(g_LoginPwd));
    for (i = 0; i < 6; i++)
    {
        g_LoginPwd[i] = pRandPwd[i + 1];
    }
    g_LoginPwd[6] = val[0];
    g_LoginPwd[7] = val[1];
    g_LoginPwd[8] = '\0';
    os_bluetooth_log("g_LoginPwd: %s", g_LoginPwd);
    return BLE_OK;
}

/*************************************************
 Function:     creat_private_key
 Description:  生成密钥
 Input:        无
 Output:       无
 Return:       密码长度
 Others:
 *************************************************/
static int creat_private_key(void)
{
    uint8_t i = 0;
    char publicKey[9];
    char UserPin[9];
    char *pUserPin = storage_get_present_userpin();
    memset(UserPin, 0, sizeof(UserPin));
    if (pUserPin && strlen(pUserPin) == 8)
    {
        sprintf(UserPin, "%s", pUserPin);
    }
    else
    {
        sprintf(UserPin, "%s", DEFAULT_USERPIN);
    }

    memset(publicKey, 0, sizeof(publicKey));
    sprintf(publicKey, "%s", BLE_PUBLIC_KEY);

    // 生成加密秘钥
    /*【生成规则】:
     (USER_PIN)0-3 (PUBLIC_KEY) 0-3(USER_PIN)4-7 (PUBLIC_KEY) 4-7*/
    memset(g_PrivateKey, 0, sizeof(g_PrivateKey));
    for (i = 0; i < 4; i++)
    {
        g_PrivateKey[i] = UserPin[i];
        g_PrivateKey[i + 4] = publicKey[i];
        g_PrivateKey[i + 8] = UserPin[i + 4];
        g_PrivateKey[i + 8 + 4] = publicKey[i + 4];
    }
    g_PrivateKey[16] = '\0';
    //os_bluetooth_log("creat_private_key: UserPin[%s] g_PrivateKey: %s\n", UserPin, g_PrivateKey);

    return BLE_OK;
}

/*************************************************
 Function:     tea_decrypt
 Description:  tea解密
 Input:
 1.v:        要解密的数据,长度为8字节
 2.k         解密用的key,长度为16字节
 Output:
 1.v:        解密后的数据
 Return:       无
 Others:
 *************************************************/
static void tea_decrypt(unsigned char *v, unsigned char *k)
{
    //uint32_t *v0, *v1, *k0, *k1, *k2, *k3;
    uint32_t y, z, sum = 0xC6EF3720, i;
    uint32_t delta = 0x9e3779b9;
    uint32_t a, b, c, d;
    memcpy(&y, v, 4);
    memcpy(&z, v + 4, 4);
    memcpy(&a, k, 4);
    memcpy(&b, k + 4, 4);
    memcpy(&c, k + 8, 4);
    memcpy(&d, k + 12, 4);
    //os_bluetooth_log("tea : %x %x\n", y, z);
    //os_bluetooth_log("tea : %x %x %x %x \n", a, b, c, d);

    for (i = 0; i < 32; i++)
    {
        z -= ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
        y -= ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
        sum -= delta;
    }
    memcpy(v, &y, 4);
    memcpy(v + 4, &z, 4);
}

/*************************************************
 Function:     dncrypt
 Description:  解密算法
 Input:
 1.src:      源数据,所占空间必须为8字节的倍数.解密完成后密文也存放在这
 2.size_src: 源数据大小,单位字节
 3.key:      密钥,16字节
 Output:
 1.src:      解密后的数据
 Return:       密文的字节数
 Others:
 *************************************************/
uint8_t decrypt(unsigned char *src, unsigned char size_src)
{
    uint8_t i = 0;
    uint8_t num = 0;

    // 判断长度是否为8的倍数
    if (size_src % 8 != 0)
    {
        return 0;
    }

    // 解密
    creat_private_key();
    num = size_src / 8;
    for (i = 0; i < num; i++)
    {
        tea_decrypt(src + i * 8, g_PrivateKey);
    }

    return size_src;
}

/*******************************************************
 *  函数名称：FindCmd
 *  函数功能：在AT指令表中查找目标指令
 *  入口参数：ucCmd，指令索引号；
 *  出口参数：char *，找到的该指令的前缀首地址
 *  函数说明：
 ******************************************************/
static unsigned char *FindCmd(AT_CMD_E cmd)
{
    int i;
    int ucSize = sizeof(ATCmdTable) / sizeof(ATCmdTable[0]);
    os_bluetooth_log(" FindCmd: ucSize:%d\n", ucSize);
    for (i = 0; i < ucSize; i++)
    {
        if (cmd == ATCmdTable[i].cmd)
        {
            os_bluetooth_log("FindCmd: ucpCmdPrefix:%s\n", ATCmdTable[i].ucpCmdPrefix);
            return ATCmdTable[i].ucpCmdPrefix; // 返回这个AT指令的前缀首地址
        }
    }

    return NULL;
}

/*************************************************
 Function:     ATCmdSend
 Description:  发送AT指令
 Input:
 1.cmd       指令索引号；
 2.type      指令类型；
 3.ucpPara   指令携带的参数，无参数请设置为NULL；
 Output:       无
 Return:       BLE_OK，成功；BLE_ERROR，失败
 Others:
 *************************************************/
int ATCmdSend(AT_CMD_E cmd, AT_TYPE_E type, unsigned char *ucpPara)
{
    char ucCmdStr[BLE_CMD_LEN] = {0}; // AT指令缓冲区
    unsigned char *ucpSrc = NULL;
    int len = 0;

    if (g_MutexFlag == 1)
    {
        os_bluetooth_log(" ATCmdSend: g_MutexFlag==1, return.\n");
        return BLE_ERROR;
    }
    g_MutexFlag = 1;

    ucpSrc = FindCmd(cmd); // 查找该指令，取得前缀
    if (ucpSrc == NULL)
    {
        os_bluetooth_log("ATCmdSend[%d], type[%d], not find.\n", cmd, type);
        return BLE_ERROR; // 没有找到，命令无效
    }

    memset(ucCmdStr, 0, sizeof(ucCmdStr));
    strcpy((char *)ucCmdStr, (char *)ucpSrc); // 将命令拷贝到缓冲区
    if (AT_GET == type)
    {
        sprintf((char *)ucCmdStr, "%s?", (char *)ucpSrc);
    }
    else if(AT_GETR == type)
    {
        sprintf((char *)ucCmdStr, "%s[%s?]", (char *)ucpSrc, (char *)ucpPara); // 远程查询命令，将命令拷贝到缓冲区
    }
    else if (AT_SET == type)
    {
        sprintf((char *)ucCmdStr, "%s[%s]", (char *)ucpSrc, (char *)ucpPara); // 设置命令，把要设置的参数填充到缓冲区
    }
    else
    {
        ; // 非查询非设置命令，不带问号不带参数
    }

    //将命令进队列处理
    len = strlen((char *)ucCmdStr);
    if (len > BLE_CMD_LEN)
    {
        len = BLE_CMD_LEN;
    }
    mATcmdList[mPutIndex].cmd_len = len;
    mATcmdList[mPutIndex].send_type = 0;
    memset(mATcmdList[mPutIndex].cmd, 0, sizeof(mATcmdList[mPutIndex].cmd));
    memcpy(mATcmdList[mPutIndex].cmd, ucCmdStr, len);
    if (AT_SCAN == cmd)
    {
        mATcmdList[mPutIndex].send_flag = 2;
    }
    else if ((AT_CONN == cmd) || (AT_CONNL == cmd) || (AT_ROLE == cmd) || AT_RENEW == cmd)
    {
        mATcmdList[mPutIndex].send_flag = 3;
    }
    else if (AT_CON == cmd)
    {
        mATcmdList[mPutIndex].send_flag = 5;
    }
    else
    {
        mATcmdList[mPutIndex].send_flag = 1;
    }

//    os_bluetooth_log("ATCmdSend to mATcmdList[%d].cmd[%s] len[%d] send_type[%d] send_flag[%d]\n",
//                     mPutIndex, mATcmdList[mPutIndex].cmd, len, mATcmdList[mPutIndex].send_type, mATcmdList[mPutIndex].send_flag);

    mPutIndex++;
    if (mPutIndex == MAX_ATCMDLIST_SZIE)
    {
        mPutIndex = 0;
    }

    g_MutexFlag = 0;
    return BLE_OK;
}
/*************************************************
 Function:     com_ble_init
 Description:  串口初始化
 Output:       无
 Return:       NULL
 Others:
 *************************************************/
int com_ble_init(void)
{
    ATCmdSend(AT_DISC, AT_CALL, NULL);                // 断开蓝牙连接
    ATCmdSend(AT_ROLE, AT_SET, (unsigned char *)"C"); // 设置蓝牙模块工作在主模式
    ATCmdSend(AT_TYPE, AT_SET, (unsigned char *)"A"); // 设置不需要密码认证身份说

    ATCmdSend(AT_NAME, AT_SET, DEFAULT_DEVNAME);          // 设置设备名
    ATCmdSend(AT_UADT, AT_SET, BLE_DEFINE_DATA);          // 设置用户自定义数据
    ATCmdSend(AT_IMME, AT_SET, (unsigned char *)"N");     // 扫描不连接,需要连接时发连接命令
    ATCmdSend(AT_SNAME, AT_SET, (unsigned char *)"Y");    // 扫描时显示设备名
    ATCmdSend(AT_RANG, AT_SET, (unsigned char *)"2,100"); // 扫描2秒返回

    ATCmdSend(AT_SRSSI, AT_SET, (unsigned char *)"Y"); // 设置扫描带信号强度

    os_bluetooth_log(" ============= ble_master_init ============ \n");
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_scan
 Description:  扫描蓝牙从模块
 Input:        无
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_scan(void)
{

    ATCmdSend(AT_SCAN, AT_CALL, NULL);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_connect
 Description:  连接指定蓝牙模块
 Input:
 flg         0 正常连接 1 使用临时pin码去连接
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_connect(char *mac)
{
    if (mac == NULL)
    {
        os_bluetooth_log("ble connect err!");
        return BLE_ERROR;
    }
    ATCmdSend(AT_CON, AT_SET, (unsigned char *)mac);
    return BLE_OK;
}


/*************************************************
 Function:     com_ble_disconnect
 Description:  断开蓝牙模块连接
 Input:        无
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_disconnect(void)
{
    ATCmdSend(AT_DISC, AT_CALL, NULL);
    return BLE_OK;
}
/*************************************************
 Function:     com_ble_get_stas
 Description:  获取蓝牙当前状态
 Input:        无
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_get_stas(void)
{
    ATCmdSend( AT_STAS, AT_GET, NULL );
    return BLE_OK;
}
/*************************************************
 Function:     com_ble_reset
 Description:  蓝牙复位
 Input:        无
 Output:       无
 Return:
 Others:
 *************************************************/
int com_ble_reset(void)
{
    ATCmdSend(AT_RESET, AT_CALL, NULL);
    g_ble_state = 1;
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_get_lockinfo
 Description:  获取锁信息
 Input:        无
 Output:       无
 Return:       成功
 Others:
 *************************************************/
int com_ble_get_lockinfo(void)
{
    char buf[2];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0;
    send_touchuan_data(CMD_BLE_GET_LOCK_INFO, 0, buf, 1);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_set_userpin
 Description:  设置用户PIN码
 Input:        无
 Output:       无
 Return:       成功
 Others:
 *************************************************/
int com_ble_set_userpin(unsigned char *userpin)
{
    if (userpin == NULL)
    {
        os_bluetooth_log("ble set userpin err !");
        return BLE_ERROR;
    }
    unsigned char buf[10];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0;
    memcpy(buf + 1, userpin, 8);

    send_touchuan_data(CMD_BLE_SET_PIN, 0, (char *)buf, 9);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_set_broadrate
 Description:  设置广播间隔频率
 Input:
 1.mode      省电模式: 1省电模式 0正常模式
 Output:       无
 Return:       成功
 Others:
 *************************************************/
int com_ble_set_broadrate(unsigned char mode)
{
    char buf[3];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0;
    if (mode == 1) // 省电模式
    {
        buf[1] = SAVE_POWER_MODE;
    }
    else
    {
        buf[1] = NORMAL_POWER_MODE;
    }
    send_touchuan_data(CMD_BLE_SET_BROADRATE, 0, buf, 2);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_set_channel
 Description:  设置蓝牙通道
 Input:
 1.mode
 Output:
 Return:
 Others:
 *************************************************/
int com_ble_set_channel(int user, unsigned int id, unsigned char type)
{
    unsigned char buf[11];
    int ret = 0;
    struct tm *currentTime;
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;
    memset(buf, 0, sizeof(buf));
    memset(buf, 0, sizeof(buf));
    buf[0] = _DEC_TO_BCD(user);
    buf[1] = _DEC_TO_BCD((unsigned char)(id / 100));
    buf[2] = _DEC_TO_BCD((unsigned char)(id % 100));
    buf[3] = type;
    buf[10] = 0;
    mico_time_get_utc_time(&utc_time);
    currentTime = localtime((const time_t *)&utc_time);
    rtc_time.sec = currentTime->tm_sec;
    rtc_time.min = currentTime->tm_min;
    rtc_time.hr = (unsigned char)currentTime->tm_hour;

    rtc_time.date = currentTime->tm_mday;
    rtc_time.weekday = currentTime->tm_wday;
    rtc_time.month = currentTime->tm_mon + 1;
    rtc_time.year = (currentTime->tm_year + 1900) % 100;
    buf[4] = _DEC_TO_BCD((unsigned char)(rtc_time.year));
    buf[5] = _DEC_TO_BCD(rtc_time.month);
    buf[6] = _DEC_TO_BCD(rtc_time.date);
    buf[7] = _DEC_TO_BCD((unsigned char)(rtc_time.hr + 8));
    buf[8] = _DEC_TO_BCD(rtc_time.min);
    buf[9] = _DEC_TO_BCD(rtc_time.sec);
    ret = send_touchuan_data(CMD_BLE_SET_LOCKCHANNEL, 0, buf, 10);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_set_factory
 Description:  蓝牙恢复出厂
 Input:
 Output:
 Return:           BLE_OK
 Others:
 *************************************************/
int com_ble_set_factory(void)
{
    char buf[2];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0;
    send_touchuan_data(CMD_BLE_SET_FACTORY, 0, buf, 1);
    return BLE_OK;
}

/*************************************************
 Function:     com_ble_unlock
 Description:  开锁
 Input:        1.设备号
 2.用户ID
 3.开锁类型
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_unlock(int user, unsigned int id, unsigned char type)
{
    char buf[11];
    struct tm *currentTime;
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;
    // 用户ID 范围0000~9999
    if (id > 10000 || id < 0)
    {
        os_bluetooth_log(" user id is wrong ! \n");
        return FALSE;
    }

    memset(buf, 0, sizeof(buf));
    buf[0] = _DEC_TO_BCD(user);
    buf[1] = _DEC_TO_BCD((unsigned char)(id / 100));
    buf[2] = _DEC_TO_BCD((unsigned char)(id % 100));
    buf[3] = 0x10 | type; // 开锁类型: 手机类型
    buf[10] = 0;
    mico_time_get_utc_time(&utc_time);
    currentTime = localtime((const time_t *)&utc_time);
    rtc_time.sec = currentTime->tm_sec;
    rtc_time.min = currentTime->tm_min;
    rtc_time.hr = (unsigned char)currentTime->tm_hour;
    rtc_time.date = currentTime->tm_mday;
    rtc_time.weekday = currentTime->tm_wday;
    rtc_time.month = currentTime->tm_mon + 1;
    rtc_time.year = (currentTime->tm_year + 1900) % 100;
    os_bluetooth_log("rtc_time.year>>[%d][%d][%d][%d][%d][%d]\n", rtc_time.year, rtc_time.month, rtc_time.date, rtc_time.hr, rtc_time.min, rtc_time.sec);
    buf[4] = _DEC_TO_BCD((unsigned char)(rtc_time.year));
    buf[5] = _DEC_TO_BCD(rtc_time.month);
    buf[6] = _DEC_TO_BCD(rtc_time.date);
    buf[7] = _DEC_TO_BCD((unsigned char)(rtc_time.hr + 8));
    buf[8] = _DEC_TO_BCD(rtc_time.min);
    buf[9] = _DEC_TO_BCD(rtc_time.sec);

    os_bluetooth_log("rtc_time.year[%x][%x][%x][%x][%x][%x]\n", buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    send_touchuan_data(CMD_BLE_UNLOCK, 0, buf, 10);

    return BLE_OK;
}

/*************************************************
 Function:     com_ble_set_time
 Description:   设置时间
 Input:        1.操作id
 2 时间
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_set_time(void)
{
    char buf[7];
    struct tm *currentTime;
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;
    memset(buf, 0, sizeof(buf));
    buf[0] = 1;
    mico_time_get_utc_time(&utc_time);
    currentTime = localtime((const time_t *)&utc_time);
    rtc_time.sec = currentTime->tm_sec;
    rtc_time.min = currentTime->tm_min;
    rtc_time.hr = (unsigned char)currentTime->tm_hour;
    rtc_time.date = currentTime->tm_mday;
    rtc_time.weekday = currentTime->tm_wday;
    rtc_time.month = currentTime->tm_mon + 1;
    rtc_time.year = (currentTime->tm_year + 1900) % 100;

    buf[1] = _DEC_TO_BCD((unsigned char)(rtc_time.year));
    buf[2] = _DEC_TO_BCD(rtc_time.month);
    buf[3] = _DEC_TO_BCD(rtc_time.date);
    buf[4] = _DEC_TO_BCD((unsigned char)(rtc_time.hr + 8));
    buf[5] = _DEC_TO_BCD(rtc_time.min);
    buf[6] = _DEC_TO_BCD(rtc_time.sec);

    PrintfByteHex("com_ble_set_time", buf, 7);
    send_touchuan_data(CMD_BLE_SET_TIME, 0, buf, 7);

    return BLE_OK;
}

/*************************************************
 Function:     com_ble_remote_unlock
 Description:  开锁
 Input:        lockpass 远程密码指针
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_remote_unlock(void)
{
    char buf[4];

    memset(buf, 0, sizeof(buf));
    /******远程开门密码******/
    strcpy(&buf[0], g_lockpwd);
    PrintfByteHex("com_ble_remote_unlock", buf, 4);
    send_touchuan_data(CMD_BLE_REMO_PASS_UNLOCK, 0, buf, 4);

    return BLE_OK;
}

/*************************************************
 Function:     com_ble_add_tmp_sec_unlock_one
 Description:  添加临时秘钥开门第一帧
 Input:        1.id 用户id
 2.type 开锁类型
 3.adminpass 管理员密码指针
 Output:       4.lockpass 开门密码指针
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_add_tmp_sec_unlock_one(void)
{
    char buf[11];

    // 用户ID 范围0000~9999
    if (g_usrid > 10000 || g_usrid < 0)
    {
        os_bluetooth_log(" user id is wrong ! \n");
        return FALSE;
    }

    memset(buf, 0, sizeof(buf));
    /*****用户id******/
    buf[0] = _DEC_TO_BCD((unsigned char)(g_usrid / 100));
    buf[1] = _DEC_TO_BCD((unsigned char)(g_usrid % 100));
    /*****开锁类型*****/
    buf[2] = g_locktype; // 开锁类型: 手机类型
    /*****开锁管理员密码****/
    strcpy(&buf[3], g_adminpwd);
    /******开门密码******/

    strcpy(&buf[7], g_lockpwd);
    PrintfByteHex("ble_add_tmp_sec_unlock_one", buf, 11);
    //    send_touchuan_data(CMD_BLE_ADD_TMP_SEC_UNLOCK, 0, buf, 11);
    send_touchuan_mult_data(CMD_BLE_ADD_TMP_SEC_UNLOCK, 0, 1, buf, 11);
    return BLE_OK;
}
/*************************************************
 Function:     com_ble_add_tmp_sec_unlock_two
 Description:  添加临时秘钥开门第二帧
 Input:        1.id 用户id
 2.effect_time 生效时间
 3.invalid_time 失效时间
 Output:
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_add_tmp_sec_unlock_two(void)
{
    char buf[15];

    // 用户ID 范围0000~9999
    if (g_usrid > 10000 || g_usrid < 0)
    {
        os_bluetooth_log(" user id is wrong ! \n");
        return FALSE;
    }

    memset(buf, 0, sizeof(buf));
    /*********用户id******/
    buf[0] = _DEC_TO_BCD((unsigned char)(g_usrid / 100));
    buf[1] = _DEC_TO_BCD((unsigned char)(g_usrid % 100));
    /****生效时间***/
    buf[2] = _DEC_TO_BCD((unsigned char)(effect_time.year));
    buf[3] = _DEC_TO_BCD(effect_time.month);
    buf[4] = _DEC_TO_BCD(effect_time.date);
    buf[5] = _DEC_TO_BCD((unsigned char)(effect_time.hr + 8));
    buf[6] = _DEC_TO_BCD(effect_time.min);
    buf[7] = _DEC_TO_BCD(effect_time.sec);
    /****失效时间****/
    buf[8] = _DEC_TO_BCD((unsigned char)(invalid_time.year));
    buf[9] = _DEC_TO_BCD(invalid_time.month);
    buf[10] = _DEC_TO_BCD(invalid_time.date);
    buf[11] = _DEC_TO_BCD((unsigned char)(invalid_time.hr + 8));
    buf[12] = _DEC_TO_BCD(invalid_time.min);
    buf[13] = _DEC_TO_BCD(invalid_time.sec);
    /******次数****/

    if (g_cnt > 15)
    {
        buf[14] = _BCD_TO_DEC((unsigned char)g_cnt);
        buf[14] = _DEC_TO_BCD(buf[14]);
    }
    else
    {
        buf[14] = _BCD_TO_DEC((unsigned char)g_cnt);
    }
    PrintfByteHex("com_ble_add_tmp_sec_unlock_two", buf, 15);
    send_touchuan_data(CMD_BLE_ADD_TMP_SEC_UNLOCK, 0, buf, 15);

    return BLE_OK;
}

/*************************************************
 Function:     com_ble_del_tmp_sec_unlock
 Description:  删除临时秘钥
 Input:        1.id 用户id
 2.adminpass 管理员密码指针
 Output:
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_del_tmp_sec_unlock(void)
{
    char buf[6];

    // 用户ID 范围0000~9999
    if (g_usrid > 10000 || g_usrid < 0)
    {
        os_bluetooth_log(" user id is wrong ! \n");
        return FALSE;
    }

    memset(buf, 0, sizeof(buf));
    /*********用户id******/
    buf[0] = _DEC_TO_BCD((unsigned char)(g_usrid / 100));
    buf[1] = _DEC_TO_BCD((unsigned char)(g_usrid % 100));
    /****管理员密码***/
    strcpy(&buf[2], g_adminpwd);
    PrintfByteHex("com_ble_del_tmp_sec_unlock", buf, 6);
    send_touchuan_data(CMD_BLE_DEL_TMP_SEC_UNLOCK, 0, buf, 6);

    return BLE_OK;
}

/*************************************************
 Function:     com_ble_get_unlock_list
 Description:  获取门锁钥匙列表
 Input:        1.flag  0x00代表强制获取  0x01有变动再获取
 Output:
 Return:       实际发送长度
 Others:
 *************************************************/
int com_ble_get_unlock_list(uint8_t mode)
{
    char buf[1];
    memset(buf, 0, sizeof(buf));
    buf[0] = mode;
    /*********更新获取******/
    send_touchuan_data(CMD_BLE_GET_LOCK_LIST, 0, buf, 1);
    return BLE_OK;
}

int ble_security_check_two(unsigned char *data)
{
    char buf[8];
    memset(buf, 0, sizeof(buf));

    /*********加密数据******/
    memcpy(buf, data, 8);
    send_touchuan_data(CMD_BLE_SECURITY_CHECK_TWO, 0, buf, 8);
    return BLE_OK;
}
/*************************************************
 Function:     send_get_record_info_cmd
 Description:  发送获取操作记录信息
 Input:
 user        设备号
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int send_get_record_info(unsigned char user)
{
    char buf[8] = {0};
    memset(buf, 0, sizeof(buf));
    buf[0] = _DEC_TO_BCD(user);
    return send_touchuan_data(CMD_BLE_GET_RECORD_INFO, 0, buf, 1);
}

/*************************************************
 Function:     send_get_record_list
 Description:  发送获取操作记录列表
 Input:
 StartNo     起始设备编号
 Num         获取条数 最大 10
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
int send_get_record_list(unsigned short StartNo, unsigned char Num)
{
    int ret;
    char buf[11] = {0};
    struct tm *currentTime;
    mico_utc_time_t utc_time;
    mico_rtc_time_t rtc_time;
    unsigned short start = StartNo;

    memset(buf, 0, sizeof(buf));
    buf[0] = 0;

    // 注意 编号这位是大端模式过去的
    buf[1] = (char)(start >> 8);
    buf[2] = (char)(start);
    if (Num > MAX_RECORD_COUNT)
    {
        buf[3] = MAX_RECORD_COUNT;
    }
    else
    {
        buf[3] = Num;
    }

    mico_time_get_utc_time(&utc_time);
    currentTime = localtime((const time_t *)&utc_time);
    rtc_time.sec = currentTime->tm_sec;
    rtc_time.min = currentTime->tm_min;
    rtc_time.hr = (unsigned char)currentTime->tm_hour;

    rtc_time.date = currentTime->tm_mday;
    rtc_time.weekday = currentTime->tm_wday;
    rtc_time.month = currentTime->tm_mon + 1;
    rtc_time.year = (currentTime->tm_year + 1900) % 100;
    buf[4] = _DEC_TO_BCD((unsigned char)(rtc_time.year));
    buf[5] = _DEC_TO_BCD(rtc_time.month);
    buf[6] = _DEC_TO_BCD(rtc_time.date);
    buf[7] = _DEC_TO_BCD((unsigned char)(rtc_time.hr + 8));
    buf[8] = _DEC_TO_BCD(rtc_time.min);
    buf[9] = _DEC_TO_BCD(rtc_time.sec);
    os_bluetooth_log("send_get_record_list: net_time[%x-%x-%x %x:%x:%x]\n", buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    ret = send_touchuan_data(CMD_BLE_GET_RECORD_LIST, 0, buf, 10);

    return ret;
}

/*************************************************
 Function:     cmd_security_check_deal
 Description:  安全验证命令处理
 Input:
 recvPacket  接收到的数据
 Output:       无
 Return:
 Others:
 *************************************************/
int cmd_security_check(RecvPacket *recvPacket)
{
    unsigned char sendAtBuf[BLE_CMD_LEN] = {""};

    if (g_IsCheck == 1)
    {
        os_bluetooth_log(" cmd_security_check: g_IsCheck is 1, return.\n");
        //return BLE_ERROR;
    }
    g_IsCheck = 1;

    // 对收到的密码进行解密
    unsigned char RecvBuf[10];
    memset(RecvBuf, 0, sizeof(RecvBuf));
    memcpy(RecvBuf, recvPacket->data, 8);
    //os_bluetooth_log("RecvBuf is [%s]\n",RecvBuf);
    //    os_bluetooth_log("RecvBuf is [%x][%x][%x][%x][%x][%x][%x][%x]\n",RecvBuf[0],RecvBuf[1],RecvBuf[2],RecvBuf[3],RecvBuf[4],RecvBuf[5],RecvBuf[6],RecvBuf[7]);
    decrypt(RecvBuf, 8);

    // 根据接收到的随机码生成登入密码、登入到远程蓝牙模块
    creat_login_pwd(RecvBuf);

    sprintf((char *)sendAtBuf, "AT+PWD[%s]", (char *)g_LoginPwd);
    //os_bluetooth_log("cmd_security_check_deal is %s\n",sendAtBuf);
    errCnt++;
    if (errCnt % 2 == 0)
    {
        errCnt = 0;
        return BLE_OK;
    }
    else
    {
        ATCmdSend(AT_R, AT_SET, sendAtBuf);
        return BLE_OK;
    }
}
/*************************************************
 Function:     ble_get_name
 Description:  获取登录成功后的从设备的名称
 Input:        无
 Output:       无
 Return:       实际发送长度
 Others:
 *************************************************/
void ble_get_name(void)
{
    unsigned char sendAtBuf[BLE_CMD_LEN] = {"AT+NAME"};
    ATCmdSend(AT_R, AT_GETR, sendAtBuf);

}

/*************************************************
 Function:     cmd_record_info
 Description:  记录信息的处理
 Input:
 recvPacket  信息包
 Output:       无
 Return:       成功/失败
 Others:
 *************************************************/
int cmd_record_info(RecvPacket *recvPacket)
{
    unsigned short UnSyncCounts = 0;
    os_bluetooth_log("g_BleState>>>>>>>>[%d]\n", g_BleState);
    // 注意记录编号总数等都是大端模式过来 需要这边进行转换

    if (!_GET_BIT_((recvPacket->data[0]), 7))
    {
        UnSyncCounts = ((unsigned short)((recvPacket->data[4]) << 8) | (unsigned short)(recvPacket->data[5]));
        os_bluetooth_log("UnSyncCounts: %d\n", UnSyncCounts);
    }

    if (UnSyncCounts >= 0)
    {
        // 必须延迟一会不然单片机处理不了 会导致这条命令无应答 添加500ms 的延迟操作

        if (g_BleState == BLE_GET_RECORD_INFO)
        {

            send_get_record_list(0, UnSyncCounts);
            g_BleState = BLE_GET_RECORD_LIST;
        }
        return BLE_OK;
    }
    else
    {
//        com_ble_get_unlock_list(1);
//        g_BleState = BLE_GET_LOCK_LIST;
    }
    return BLE_OK;
}

/*************************************************
 Function:     cmd_tmp_key_list
 Description:  临时钥匙列表的处理
 Input:
 recvPacket  信息包
 Output:       无
 Return:       成功/失败
 Others:
 *************************************************/
int cmd_tmp_key_list(RecvPacket *recvPacket)
{
    // 1B(门状态) + 2B(临时用户id)
    //        os_bluetooth_log("recvPacket->datalen: %d\n", recvPacket->datalen);
    PrintfByteHex("cmd_tmp_key_list", recvPacket->data, recvPacket->datalen);
    char tmpdata[4] = {0};
    memset(&tmpdata, 0, sizeof(tmpdata));
    memcpy(&tmpdata[0], recvPacket->data, BLE_TMP_KEY_LIST_DATA_LEN);
    if(_GET_BIT_(tmpdata[0], 7) == 0)
    {
        if (g_BleState == BLE_ADD_TMP_SEC_UNLOCK)
        {
            ble_set_adminpassword();
        }
    }
    if (g_BleCallBackFunc)
    {

        if (g_BleState == BLE_ADD_TMP_SEC_UNLOCK)
        {
            g_BleCallBackFunc(BLE_NOTI_ADD_TMP_SEC_UNLOCK, tmpdata, BLE_TMP_KEY_LIST_DATA_LEN);
        }
        else
        {
            g_BleCallBackFunc(BLE_NOTI_DEL_TMP_SEC_UNLOCK, tmpdata, BLE_TMP_KEY_LIST_DATA_LEN);
        }
    }
    g_BleState = BLE_IDLE;
    g_ScanState = 0;
    g_add_del_tmp_pass = 0;
    g_del_tmp_pass = 0;
    return BLE_OK;
}

/*************************************************
 Function:     cmd_record_list
 Description:  获取到的操作记录信息的处理
 Input:
 recvPacket  信息包
 Output:       无
 Return:       成功/失败
 Others:
 *************************************************/
void cmd_record_list(RecvPacket *recvPacket)
{
    // 1B(门锁状态) + 2B(当前编号) + 10B(记录信息)
    //os_bluetooth_log("recvPacket->datalen: %d\n", recvPacket->datalen); //参数后面的数据
    //    PrintfByteHex("cmd_record_list ", recvPacket->data, recvPacket->datalen);
    unsigned char i = 0;
    unsigned char num = recvPacket->datalen / 13;
    unsigned char RecordType = 0;
    unsigned char door_status = 0;
    BLEUSER_RECORDLIST UnlockRecord;
    memset(&UnlockRecord, 0, sizeof(BLEUSER_RECORDLIST));

    // 拆分数据送入对应存储
    for (i = 0; i < num; i++)
    {
        RecordType = recvPacket->data[i * 13 + 3]; // 记录类型
        door_status = recvPacket->data[i * 13];    // 门状态
        stBLELockStatus lockstatus;
        memset(&lockstatus, 0, sizeof(stBLELockStatus));
        lockstatus.isOperSuccess = _GET_BIT_(door_status, 7);
        os_bluetooth_log("lockstatus.isOperSuccess : %d", lockstatus.isOperSuccess);
        if (lockstatus.isOperSuccess == 0) //成功获取记录
        {
            os_bluetooth_log("RecordType: %d , total num[%d]\n", RecordType, num);
            get_record_success = 0;
            if (RecordType == BLE_RECORD_TYPE_OPEN || RecordType == BLE_RECORD_TYPE_STATUS || RecordType == BLE_RECORD_TYPE_TMPAUTH || RecordType == BLE_RECORD_TYPE_ADD || RecordType == BLE_RECORD_TYPE_DEL)
            {
                memcpy(UnlockRecord.Record[UnlockRecord.Counts], recvPacket->data + i * 13 + 3,
                       BLERECORD_LEN);
                UnlockRecord.Counts++;

                if (g_BleCallBackFunc)
                {
                    msleep(500);
                    g_BleCallBackFunc(BLE_NOTI_NEW_RECORD,
                                      UnlockRecord.Record[UnlockRecord.Counts - 1],
                                      BLERECORD_LEN);
                }
            }
        }
        else //失败发送获取门锁钥匙列表
        {
            //com_ble_get_unlock_list(1);
            ble_get_name();
           // g_BleState = BLE_GET_LOCK_LIST;
        }
    }
    if (RecordType == BLE_RECORD_TYPE_OPEN)
    {
        msleep(500);
        if (RecordType != BLE_RECORD_TYPE_ADD && RecordType != BLE_RECORD_TYPE_DEL)
        {
            g_BleCallBackFunc(BLE_NOTI_STATE, &door_status, 1);
        }

    }
}

/*************************************************
 Function:     get_record_isOperSuccess
 Description:  获取记录是否操作成功
 Input:        0 成功 1失败
 Output:       无
 Return:       成功/失败
 Others:
 ************************************************/
static uint8_t get_record_isOperSuccess(void)
{
    return get_record_success;
}

/*************************************************
 Function:     cmd_lock_list
 Description:  获取到的门锁钥匙列表
 Input:
 Output:       无
 Return:       成功/失败
 Others:
 ************************************************/
void cmd_lock_list(RecvPacket *recvPacket)
{
    // 1B(门锁状态) + 1B(钥匙总数) + 1B(钥匙个数 ) + 【用户id 2B 和 开锁类型1B】

    unsigned char i, j;
    unsigned char lock_id_total = 0;
    unsigned char toal_len = 0;
    static char num = 0;
    static char num1 = 0;
    unsigned char usr_id[800];
    toal_len = recvPacket->data[1]; //钥匙总数
    num = recvPacket->data[1] / 4;
    num1 = recvPacket->data[1] - num * 4;
    usr_id[0] = recvPacket->data[0];

    PrintfByteHex("cmd_lock_list ", recvPacket->data, recvPacket->datalen);
    if (num1 < 4 && num1 > 0)
    {
        num = num + 1;
    }
    memset(usr_id, 0, sizeof(usr_id));
    for (i = 0; i < num; i++)
   {
       toal_len = recvPacket->data[1]; //钥匙总数

       lock_id_total = recvPacket->data[2 + i * 15]; // 钥匙个数
       //        os_bluetooth_log("toal_len : %d lock_id_total ； %d",toal_len,lock_id_total);
       for (j = 0; j < lock_id_total; j++)
       {
           memcpy(&usr_id[3 * j + i * 12 + 1], &recvPacket->data[j * 3 + 3 + i * 15], 3);
       }
   }
   if (g_BleCallBackFunc)
   {
       if (g_BleState == BLE_FORCE_GET_LOCK_LIST)
       {
           g_BleCallBackFunc(BLE_NOTI_FORCE_GET_LOCK_LIST, &usr_id, toal_len * 3 + 1);
       }
       else
       {
           g_BleCallBackFunc(BLE_NOTI_GET_LOCK_LIST, &usr_id, toal_len * 3 + 1);
       }
   }
}

/*************************************************
 Function:     parse_scan_signal
 Description:  扫描命令处理
 Input:        无
 Output:       无
 Return:       成功/失败
 Others:
 *************************************************/
int parse_scan_signal(char *data, int size)
{
    char *delims = "#";
    char *delims2 = "@";
    char *result = NULL;
    char temp[50] = {0};
//    uint8_t online = 1;
    unsigned char i = 0;
    char *p = NULL;
    int bleNum = 0;
    os_bluetooth_log(" parse_scan_signal: start\n");
    if (data == NULL || size <= 0)
    {
        os_bluetooth_log(" parse_scan_signal: input argument error.\n");
        return BLE_ERROR;
    }
    bleNum = storage_get_object_num(DEVICE_OBJ_ID);
    //    for ( i = 0; i < bleNum; i++ )
    //    {
    char LockMac[32] = {0};
    int bleIndex;
    int ret = 0;
    for (bleIndex = 0; bleIndex < bleNum; bleIndex++)
    {
        ret = storage_get_ble_mac(bleIndex, LockMac);
        if (ret == kGeneralErr)
        {
            continue;
        }
        else
        {
            storage_set_present_devicePin(LockMac);
            os_bluetooth_log("LockMac[%s][%s]\n", LockMac, data);
            storage_set_present_deviceId(LockMac);
            if (strlen(LockMac) != BLU_MAC_LEN)
            {
                os_bluetooth_log(" parse_scan_signal: lockmac error.\n");
                return BLE_ERROR;
            }

            p = strstr(data, LockMac);
            os_bluetooth_log("p>>>>>>>>>>>%s",p);
            if (p)
            {
                result = strtok(p, delims);

                memset(temp, 0, sizeof(temp));
                memcpy(temp, result, strlen(result));
                os_bluetooth_log("temp is \"%s\"\n", temp);

                if (temp[13] == '1') //new record
                {
                    os_bluetooth_log("Record: lockmac[%s] has new record.\n", LockMac);
                    g_BleState = BLE_GET_RECORD_INFO;
                    com_ble_connect(LockMac);
                }
                else
                {
                    g_rssiFlag++;
                    os_bluetooth_log("g_rssiFlag : %d",g_rssiFlag);
                    if (g_rssiFlag % 4 == 0)
                    {
                        result = strtok(temp, delims2);
                        while (result != NULL)
                        {
                            os_bluetooth_log("result is \"%s\"\n", &result[0]);
                            if (result[0] == '-')
                            {
                                int rssiValue = 0;
                                rssiValue = 100 - atoi(result + 1);
                                if (rssiValue < 0)
                                {
                                    rssiValue = 0;
                                }
                                os_bluetooth_log("the signal value is %s   rssiValue = %d\r\n", result, rssiValue);

                                if (g_BleCallBackFunc)
                                {
                                    g_rssiFlag = 0;
                                    g_BleCallBackFunc(BLE_NOTI_RSSI_UPLINK,
                                                      (char *)&rssiValue,
                                                      1);
                                }
                                break;
                            }
                            else
                            {
                                result = strtok(NULL, delims2);
                            }
                        }
                    }
                    else
                    {
                        if(result)
                        {
                            free(result);
                            result = NULL;
                        }
                        if(p)
                        {
                            free(p);
                            p = NULL;
                        }
                        return BLE_OK;
                    }
                }
                if(result)
                {
                    free(result);
                    result = NULL;
                }
                if(p)
                {
                    free(p);
                    p = NULL;
                }
                return BLE_OK;
            }
            else
            {

                    if(result)
                    {
                        free(result);
                        result = NULL;
                    }
                    if(p)
                    {
                        free(p);
                        p = NULL;
                    }
             }
        }
    }
    //    }
    return BLE_ERROR;
}

/*************************************************
 Function:     parse_scan_data
 Description:  解析扫描结果数据，判断是否有绑定设备
 Input:
 1.data      蓝牙扫描到的数据
 2.datalen   数据长度
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int parse_scan_data(char *data, int size)
{
    char *p = NULL;
    if (data == NULL || size <= 0)
    {
        os_bluetooth_log(" parse_scan_data: input argument error.\n");
        return BLE_ERROR;
    }
    os_bluetooth_log(" parse_scan_data: size[%d] data:%s.\n", size, data);

    p = strstr(data, BLE_LOCK_NAME);
    if (p)
    {
        int index = strlen(data) - strlen(p) - BLU_MAC_LEN - 1;
        memset(g_lockMac, 0, sizeof(g_lockMac));
        memcpy(g_lockMac, data + index, BLU_MAC_LEN);
        os_bluetooth_log("[%s] MAC is \"%s\"\n", BLE_LOCK_NAME, g_lockMac);
        p = NULL;
        return BLE_OK;
    }


    return BLE_ERROR;
}
/*************************************************
 Function:     notify_lockstate
 Description: 通知上报门状态
 Input:       mode notiCmd state
 Output:       无
 Return:
 Others:
 *************************************************/
void notify_lockstate(int mode, uint8_t notiCmd, uint8_t state)
{

    os_bluetooth_log("mode[%x][%d]\n", mode, state);

    if(_GET_BIT_(state, 5) == 1)
    {
        elink_get_rand_pin();

       strcpy(g_tempUserPin, g_subpin);
       os_bluetooth_log("g_tempUserPin[%s]", g_tempUserPin);
       g_BleState = BLE_SET_USERPIN;
       com_ble_set_userpin(g_tempUserPin);

       if (g_BleCallBackFunc && notiCmd == BLE_NOTI_SET_USERPIN)
       {
           if (mode == 0)
           {
               g_BleCallBackFunc(notiCmd, &state, 1);
               g_BleState = BLE_IDLE;
               g_bind_status = 0;
               g_ScanState = 0;

           }
       }

    }
    else
    {
        if (g_BleCallBackFunc)
        {
            if (mode == 0)
            {
                g_BleCallBackFunc(notiCmd, &state, 1);
            }
        }
    }

}

/*************************************************
 Function:         distribute_cmd_deal
 Description:      主发命令处理
 Input:
 1.recvPacket
 Output:           无
 Return:
 Others:
 *************************************************/
int distribute_cmd_deal(RecvPacket *recvPacket)
{
    recvPacket->Cmd &= 0x7F;
    os_bluetooth_log("distribute_cmd_deal: RecCmd : 0x%02x g_BleState[%d]\n", recvPacket->Cmd, g_BleState);

    switch (recvPacket->Cmd)
    {
    case CMD_BLE_SECURITY_CHECK:
    {
        cmd_security_check(recvPacket);
    }
    break;

    default:
        os_bluetooth_log("distribute_cmd_deal: cmd is not support!!!");
        break;
    }
    return BLE_OK;
}

/*************************************************
 Function:         responsion_cmd_deal
 Description:      应答命令处理
 Input:
 1.recvPacket
 Output:           无
 Return:
 Others:
 *************************************************/
int responsion_cmd_deal(RecvPacket *recvPacket)
{
    send_touchuan_flag = 0;
    int i;
    int ret = 0;
    DEVICEOBJ_T DeviceObj;
    recvPacket->Cmd &= 0x7F;
    os_bluetooth_log("responsion_cmd_deal : 0x%02x g_BleState[%d]\n", recvPacket->Cmd, g_BleState);
    switch (recvPacket->Cmd)
    {
    case CMD_BLE_UNLOCK:
    {
        if (g_BleState == BLE_UNLOCK)
        {
            com_ble_disconnect();
        }
        notify_lockstate(0, BLE_NOTI_UNLOCK, recvPacket->data[0]);
        if (g_BleState == BLE_UNLOCK)
        {
            g_BleState = BLE_IDLE;
        }
    }
    break;
    case CMD_BLE_SET_TIME:
    {
        if (g_BleState == BLE_SET_TIME)
        {
            com_ble_disconnect();
        }
        //                notify_lockstate(0, BLE_NOTI_SET_TIME, recvPacket->data[0]);
        if (g_BleState == BLE_SET_TIME)
        {
            g_BleState = BLE_IDLE;
        }
    }
    break;
    case CMD_BLE_REMO_PASS_UNLOCK:
    {
        if (g_BleState == BLE_REMO_PASS_UNLOCK)
        {
            com_ble_disconnect();
            g_BleState = BLE_IDLE;
        }
        notify_lockstate(0, BLE_NOTI_REMO_PASS_UNLOCK, recvPacket->data[0]);
        break;
    }

    case CMD_BLE_ADD_TMP_SEC_UNLOCK:
    {
        g_del_tmp_pass = 0;
        g_add_del_tmp_pass = 0;
        int ret = cmd_tmp_key_list(recvPacket);

        if (FALSE == ret)
        {
            if (g_BleState == BLE_ADD_TMP_SEC_UNLOCK) // 防止获取记录时候开锁等操作
            {
                com_ble_disconnect(); // 断开蓝牙连接
                g_BleState = BLE_IDLE;
                g_add_del_tmp_pass = 0;
            }
        }
        break;
    }
    case CMD_BLE_DEL_TMP_SEC_UNLOCK:
    {
        g_del_tmp_pass = 0;
        g_add_del_tmp_pass = 0;
        int ret = cmd_tmp_key_list(recvPacket);
        if (FALSE == ret)
        {
            if (g_BleState == BLE_DEL_TMP_SEC_UNLOCK) // 防止获取记录时候开锁等操作
            {
                com_ble_disconnect(); // 断开蓝牙连接
                g_BleState = BLE_IDLE;
            }
        }
        break;
    }
    case CMD_BLE_SET_PIN:
        if (g_BleState == BLE_SET_USERPIN)
        {
            com_ble_disconnect();
            g_BleState = BLE_IDLE;
        }

        if (_GET_BIT_(recvPacket->data[0], 7) == 0)
        {
            memset(&DeviceObj, 0, sizeof(DEVICEOBJ_T));
            os_bluetooth_log("g_tempUserPin[%s]", g_tempUserPin);
            memcpy(DeviceObj.uuid, g_tempUserPin, 9);
            os_bluetooth_log("DeviceObj.uuid[%s]", DeviceObj.uuid);
            if(g_lockMac[0] != 0 && g_lockMac[1] != 0)
            {
                memcpy(DeviceObj.mac, g_lockMac, sizeof(DeviceObj.mac));
                os_bluetooth_log("g_lockMac : %s",g_lockMac);
            }
            else
            {
                if ( !strcmp( g_lockMacbak, g_lockMac1[Bindindex] ) )
                {
                    memcpy(DeviceObj.mac, g_lockMac1[Bindindex], sizeof(DeviceObj.mac));
                    os_bluetooth_log("g_lockMac1 : %s Binindex : %d g_lockMacbak : %s",g_lockMac1[Bindindex - 1],Bindindex,g_lockMacbak);
                }
                else
                {
                    memcpy(DeviceObj.mac, g_lockMacbak, sizeof(DeviceObj.mac));
                    os_bluetooth_log("g_lockMac1 : %s Binindex : %d g_lockMacbak : %s",g_lockMac1[Bindindex - 1],Bindindex,g_lockMacbak);

                }



            }

            storage_write_dev_obj(OBJECT_UPDATE_ADD, 1, &DeviceObj);
            //stor_set_userpin((char *)g_tempUserPin);
        }
        notify_lockstate(0, BLE_NOTI_SET_USERPIN, recvPacket->data[0]);
        break;

    case CMD_BLE_SET_BROADRATE:
        if (g_BleState == BLE_SET_BROADRATE)
        {
            com_ble_disconnect();
            //g_BleState = BLE_IDLE;
        }

        if (_GET_BIT_(recvPacket->data[0], 7) == 0)
        {
            //stor_set_powermode(g_tempBroadrate);
        }
        notify_lockstate(0, BLE_NOTI_SET_BROADRATE, recvPacket->data[0]);
        if (g_BleState == BLE_SET_BROADRATE)
        {
            g_BleState = BLE_IDLE;
        }
        break;
    case CMD_BLE_SET_LOCKCHANNEL:
        if (g_BleState == BLE_SET_LOCKCHANGNEL)
        {
            com_ble_disconnect();
        }
        notify_lockstate(0, BLE_NOTI_LOCKCHANNEL, recvPacket->data[0]);
        break;
    case CMD_BLE_SET_FACTORY:
        if (g_BleState == BLE_SET_FACTORY)
        {
            com_ble_disconnect();
            g_BleState = BLE_IDLE;
        }
        if (g_BleCallBackFunc)
        {
            int state = BLE_NOTI_ECHO_OK;
            g_BleCallBackFunc(BLE_NOTI_RESTOR_FACTORY, (char *)&state, 4);
        }
        break;

    case CMD_BLE_GET_LOCK_INFO:
//        if (g_BleState == BLE_GET_LOCKINFO)
//        {
//            com_ble_disconnect();
//            g_BleState = BLE_IDLE;
//        }
        notify_lockstate(0, BLE_NOTI_GET_LOCKINFO, recvPacket->data[0]);
//        if (g_BleState == BLE_GET_LOCKINFO)
//        {
//            g_BleState = BLE_IDLE;
//        }
        break;

    case CMD_BLE_GET_RECORD_INFO:
    {
        int ret = cmd_record_info(recvPacket);
        if (FALSE == ret)
        {
            if (g_BleState == BLE_GET_RECORD_INFO) // 防止获取记录时候开锁等操作
            {
                com_ble_disconnect(); // 断开蓝牙连接
                g_BleState = BLE_IDLE;
            }
        }
    }
    break;

    case CMD_BLE_GET_RECORD_LIST:
    {
        cmd_record_list(recvPacket);

        if (g_BleState == BLE_GET_RECORD_LIST) // 防止获取记录时候开锁等操作
        {
//            com_ble_disconnect(); // 断开蓝牙连接
            g_BleState = BLE_IDLE;
        }
        //notify_lockstate(0, BLE_NOTI_GET_LOCKINFO, recvPacket->data[0]);
    }
    break;
    case CMD_BLE_GET_LOCK_LIST:
    {
        os_bluetooth_log("CMD_BLE_GET_LOCK_LIST");
        cmd_lock_list(recvPacket);
        if (g_BleState == BLE_FORCE_GET_LOCK_LIST) // 防止获取记录时候开锁等操作
        {
            com_ble_disconnect(); // 断开蓝牙连接
            g_BleState = BLE_IDLE;

        }
        if(g_BleState == BLE_GET_LOCK_LIST)
        {
            com_ble_disconnect(); // 断开蓝牙连接
            g_BleState = BLE_IDLE;
        }
    }
    break;
    default:
        break;
    }

    return BLE_OK;
}

/*************************************************
 Function:     send_touchuan_data
 Description:  透传一帧数据到蓝牙模块
 Input:
 1.cmd       命令码 注意应答命令高位为1
 2.echo      应答标志位 1应答 0主发
 3.data      参数数据
 4.len       参数长度
 Output:       无
 Return:       实际发送长度
 Others:
 Byte0 BYTE1   BYTE2   BYTE3   BYTE4….BYTEn-1  BYTEn
 起始位    长度  帧数  命令  参数          校验
 *************************************************/
static int send_touchuan_data(unsigned char cmd, unsigned char echo, char *data, unsigned int len)
{
    if (send_touchuan_flag == 0)
    {
        send_touchuan_flag = 1; // 透传只发一次

        int i, j, sum = 0;
        unsigned char buf[MAX_PACKET_LEN];
        //PrintfByteHex("recv data", data, len);
        if (g_MutexFlag == 1)
        {
            os_bluetooth_log(" send_touchuan_data: g_MutexFlag==1, return.\n");
            return BLE_ERROR;
        }
        g_MutexFlag = 1;

        unsigned int PacketLen = 0; // 每包数据长度
        unsigned int PacketNum = 1; // 包数
        unsigned int LastLen = 0;   // 最后一包数据 参数长度

        if (data == NULL || len == 0)
        {
            PacketNum = 1;
            LastLen = 0;
        }
        else
        {
            PacketNum = ((len % (MAX_PACKET_LEN - 5)) ? (len / (MAX_PACKET_LEN - 5) + 1) : (len / (MAX_PACKET_LEN - 5)));
            LastLen = ((len % (MAX_PACKET_LEN - 5)) ? (len % (MAX_PACKET_LEN - 5)) : (MAX_PACKET_LEN - 5));
        }

        //os_bluetooth_log("DataLen: %d  PacketNum: %d   LastLen: %d\n", len, PacketNum, LastLen);
        if (PacketNum > 0xFF)
        {
            PacketNum = 0xFF;
        }
        for (j = 0; j < PacketNum; j++)
        {
            memset(buf, 0, sizeof(buf));
            buf[0] = 0xAA;
            buf[2] = (PacketNum - 1) - j;
            os_bluetooth_log("j: %d  buf[2]: %d\n", j, buf[2]);
            if (buf[2] == 0) // 最后一包
            {
                PacketLen = LastLen + 5;
            }
            else
            {
                PacketLen = MAX_PACKET_LEN;
            }
            buf[1] = PacketLen - 2;
            if (echo == 0)
            {
                buf[3] = cmd & 0x7F;
            }
            else
            {
                buf[3] = cmd | 0x80;
            }
            if (data && (PacketLen - 5) > 0)
            {
                memcpy(buf + 4, data + (j * (MAX_PACKET_LEN - 5)), PacketLen - 5);
            }

            // 计算校验和
            sum = 0;
            for (i = 1; i < PacketLen - 1; i++)
            {
                sum += *(buf + i);
            }
            *(buf + PacketLen - 1) = sum;

            //os_bluetooth_log("port3_send data  len is %d !!\n",PacketLen);
            for (i = 0; i < PacketLen; i++)
            {
                //os_bluetooth_log("port3_send data[%d]  is %02X !!\n",i,buf[i]);
            }

            mATcmdList[mPutIndex].cmd_len = PacketLen;
            mATcmdList[mPutIndex].send_type = 1;
            memset(mATcmdList[mPutIndex].cmd, 0, sizeof(mATcmdList[mPutIndex].cmd));
            memcpy(mATcmdList[mPutIndex].cmd, buf, PacketLen);
            os_bluetooth_log("ATCmdSend to mATcmdList[%d].cmd is %s  len is  %d\n", mPutIndex, mATcmdList[mPutIndex].cmd, PacketLen);
            mATcmdList[mPutIndex].send_flag = 1;
            mPutIndex++;
            if (mPutIndex == MAX_ATCMDLIST_SZIE)
            {
                mPutIndex = 0;
            }
        }
        g_MutexFlag = 0;

        return BLE_OK;
    }
    else
    {
        return BLE_ERROR;
    }
}

/*************************************************
 Function:     recv_touchuan_data
 Description:  接收到透传数据
 Input:
 1.data      数据
 Output:       无
 Return:
 Others:
 Byte0 BYTE1   BYTE2   BYTE3   BYTE4….BYTEn-1  BYTEn
 起始位    长度  帧数  命令  参数          校验
 *************************************************/
int recv_touchuan_data(unsigned char *data, unsigned int len)
{
    ble_return_val_if_fail(data, -1);

    unsigned char sum = 0;
    int i = 0;

    if (len < 5 || (data[1] + 2) != len) // 长度判断
    {
        os_bluetooth_log("recv_touchuan_data: len error: %d\n", len);
        return BLE_ERROR;
    }

    for (i = 1; i < (len - 1); i++)
    {
        sum += data[i];
    }

    if (data[len - 1] != sum) // 判断校验位是否正确
    {
        os_bluetooth_log("recv_touchuan_data:sum[%02x] !=  data[%02x]\n", sum, data[len - 1]);
        return BLE_ERROR;
    }

    RecvPacket recvpacket;
    memset(&recvpacket, 0, sizeof(recvpacket));
    recvpacket.Cmd = data[3];
    recvpacket.ErrorFlg = 0;
    recvpacket.RemainNum = data[2];
    recvpacket.datalen = len - 5;
    memcpy(recvpacket.data, data + 4, len - 5);
    if (g_RecvPacket && g_RecvPacket->Cmd == recvpacket.Cmd) // 前面已经有该命令的多包数据
    {
        //        printf("recvpacket.RemainNum: %d  g_RecvPacket->RemainNum: %d\n", recvpacket.RemainNum, g_RecvPacket->RemainNum);
        if ((recvpacket.RemainNum + 1) == g_RecvPacket->RemainNum)
        {
            memcpy(g_RecvPacket->data + g_RecvPacket->datalen, recvpacket.data,
                   recvpacket.datalen);
            g_RecvPacket->datalen += recvpacket.datalen;
            g_RecvPacket->RemainNum = recvpacket.RemainNum;
        }
        else if ((recvpacket.RemainNum + 1) < g_RecvPacket->RemainNum)
        {
            // 中间帧丢失
            printf(" recv packet error \n");
            g_RecvPacket->ErrorFlg = 1;
            g_RecvPacket->RemainNum = recvpacket.RemainNum;
        }
        else
        {
            // 可能是对方重发了该指令
            memset(g_RecvPacket, 0, sizeof(RecvPacket));
            memcpy(g_RecvPacket, &recvpacket, sizeof(RecvPacket));
            g_RecvPacket->ErrorFlg = 0;
        }
    }
    else
    {
        // 多包数据需要等最后一包再做处理
        g_RecvPacket = &g_PacketBuf;
        memset(g_RecvPacket, 0, sizeof(RecvPacket));
        memcpy(g_RecvPacket, &recvpacket, sizeof(RecvPacket));
        g_RecvPacket->ErrorFlg = 0;
    }

    // 没有后续包 则进行处理
    if (g_RecvPacket->RemainNum == 0)
    {
        // 用完后指向NULL 下次再用
        g_RecvPacket = NULL;
        if (g_PacketBuf.ErrorFlg == 0)
        {
            // 命令码最高位为1 应答 0 主发

            if (((g_PacketBuf.Cmd) & 0x80) == 0x00)
            {
                return distribute_cmd_deal(&g_PacketBuf);
            }
            else
            {
                return responsion_cmd_deal(&g_PacketBuf);
            }
        }
        else
        {
            char echo[1];
            echo[0] = 0x80;
            send_touchuan_data(g_PacketBuf.Cmd, 1, echo, 1);

            os_bluetooth_log(" recv packet error \n");
        }
    }

    return BLE_OK;
}

/*************************************************
 Function:         deal_resend
 Description:      重发处理
 Input:
 Output:           无
 Return:           BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int deal_resend(void)
{
    if (g_BleState == BLE_UNLOCK)
    {
        int ret = ble_unlock_start();
        if (BLE_OK == ret)
        {
            return BLE_OK;
        }
    }
    if(g_BleState == BLE_LOCK_BIND)
    {
        int ret = ble_bind_lock_start();
        if (BLE_OK == ret)
        {
           return BLE_OK;
        }
    }
    else if (g_BleState == BLE_REMO_PASS_UNLOCK)
    {
        int ret = ble_remote_pwd_local_unlock_start();
        if (BLE_OK == ret)
        {
            return BLE_OK;
        }
    }
    else if (g_BleState == BLE_SET_BROADRATE)
    {
        int ret = ble_set_broadrate(g_tempBroadrate);
        if (BLE_OK == ret)
        {
            return BLE_OK;
        }
    }
    else if (g_BleState == BLE_SET_LOCKCHANGNEL)
    {
        int ret = ble_set_channel();
        if (BLE_OK == ret)
        {
            return BLE_OK;
        }
    }
    else if (g_BleState == BLE_SET_USERPIN)
    {
        int ret = ble_set_userpin( (char *) g_UserPinbak );
        if (BLE_OK == ret)
        {
            return BLE_OK;
        }
    }
    else if (g_BleState == BLE_LOCK_BIND && strlen(g_lockMac) > 0)
    {
       // char *lockmac = (char *)storage_get_present_lockmac();
        com_ble_connect(g_lockMac);
        return BLE_OK;
    }
    else if (g_BleState == BLE_FORCE_GET_LOCK_LIST)
    {
        ble_force_get_lock_list(0);
        return BLE_OK;
    }
    else if (g_BleState == BLE_ADD_TMP_SEC_UNLOCK)
    {
        char *lockmac = (char *)storage_get_present_lockmac();
        g_BleState = BLE_ADD_TMP_SEC_UNLOCK;
        com_ble_connect(lockmac);
        return BLE_OK;
    }
    else if (g_BleState == BLE_DEL_TMP_SEC_UNLOCK)
    {
        char *lockmac = (char *)storage_get_present_lockmac();
        g_BleState = BLE_DEL_TMP_SEC_UNLOCK;
        com_ble_connect(lockmac);
        return BLE_OK;
    }
    return BLE_ERROR;
}

/*************************************************
 Function:         deal_login_ok
 Description:      蓝牙连接成功且密码验证通过后的处理函数
 Input:
 Output:           无
 Return:           若有处理则返回0，若无处理则返回原始长度
 Others:
 *************************************************/
int deal_login_ok(void)
{
    os_bluetooth_log(" deal_login_ok: g_BleState[%d]\n", g_BleState);
    switch (g_BleState)
    {
    case BLE_LOCK_BIND:
        if (g_BleCallBackFunc)
        {
            g_BleState = BLE_GET_LOCKINFO;
            com_ble_get_lockinfo();
            refresh_timeout(0);
        }
        break;

    case BLE_SET_USERPIN:
        os_bluetooth_log("g_tempUserPin[%s]\n", g_tempUserPin);
        com_ble_set_userpin(g_tempUserPin);
        break;
    case BLE_SET_LOCKCHANGNEL:
        com_ble_set_channel(1, 1, CMD_BLE_SET_LOCKCHANNEL);
        break;
    case BLE_SET_BROADRATE:
        com_ble_set_broadrate(g_tempBroadrate);
        break;

    case BLE_SET_FACTORY:
        com_ble_set_factory();
        break;

    case BLE_GET_LOCKINFO:
        com_ble_get_lockinfo();
        break;

    case BLE_GET_RECORD_INFO:
        send_get_record_info(0);
        break;

    case BLE_UNLOCK:
    {
        com_ble_unlock(1, 1, BLE_OPENTYPE_BLEPHONE);
        break;
    }
    case BLE_REMO_PASS_UNLOCK:
    {
        com_ble_remote_unlock();
        break;
    }
    case BLE_ADD_TMP_SEC_UNLOCK:
    {
        com_ble_add_tmp_sec_unlock_one();
        msleep(200);
        com_ble_add_tmp_sec_unlock_two();
        break;
    }
    case BLE_DEL_TMP_SEC_UNLOCK:
    {
        com_ble_del_tmp_sec_unlock();
        break;
    }
    case BLE_FORCE_GET_LOCK_LIST:
    {
        com_ble_get_unlock_list(0);
        break;
    }
    case BLE_SET_TIME:
    {
        com_ble_set_time();
        break;
    }
    }
    return BLE_OK;
}

/*************************************************
 Function:     deal_error
 Description:  蓝牙连接失败处理函数
 Input:
 mode        0 密码验证失败 1 连接失败
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int deal_error(int mode)
{

    os_bluetooth_log(" deal_error: g_BleState[%d]\n", g_BleState);
    switch (g_BleState)
    {
    case BLE_LOCK_BIND:
        if (g_BleCallBackFunc)
        {
            int state = BLE_NOTI_ECHO_ERROR;
            send_touchuan_flag = 0;
            if (mode == 0)
            {

                com_ble_disconnect();
                state = BLE_NOTI_ECHO_ERROR;
            }
            else if(mode == 1)
            {
                com_ble_disconnect();
            }

            g_BleCallBackFunc(BLE_NOTI_BIND_LOCK, (char *)&state, 1);
        }
        g_BleState = BLE_IDLE;
        break;

    case BLE_SET_USERPIN:
        if (mode == 0)
        {
            notify_lockstate(2, BLE_NOTI_SET_USERPIN, 0);
        }
        else
        {
            notify_lockstate(1, BLE_NOTI_SET_USERPIN, 0);
        }
        g_BleState = BLE_IDLE;
        send_touchuan_flag = 0;

        break;

    case BLE_SET_BROADRATE:
        if (mode == 0)
        {
            notify_lockstate(2, BLE_NOTI_SET_BROADRATE, 0);
        }
        else
        {
            notify_lockstate(1, BLE_NOTI_SET_BROADRATE, 0);
        }
        g_BleState = BLE_IDLE;
        send_touchuan_flag = 0;
        break;
    case BLE_SET_LOCKCHANGNEL:
        if (mode == 0)
        {
            notify_lockstate(2, BLE_NOTI_LOCKCHANNEL, 0);
        }
        else
        {
            notify_lockstate(1, BLE_NOTI_LOCKCHANNEL, 0);
        }
        g_BleState = BLE_IDLE;
        send_touchuan_flag = 0;
        break;
    case BLE_UNLOCK:
        if (mode == 0)
        {
            notify_lockstate(2, BLE_NOTI_UNLOCK, 0);
        }
        else
        {
            notify_lockstate(1, BLE_NOTI_UNLOCK, 0);
        }

        g_BleState = BLE_IDLE;
        send_touchuan_flag = 0;
        break;
    default:
        {

            g_BleState = BLE_IDLE;
            g_ScanState = 0;
            send_touchuan_flag = 0;
           // g_add_del_tmp_pass = 0;
         //   g_del_tmp_pass = 0;
            os_bluetooth_log("g_BleState : %d",g_BleState);
        }
        break;
    }

    return BLE_OK;
}

/*************************************************
 Function:     get_blerecv_type
 Description:  获取蓝牙接收AT指令类型
 Input:
 data
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static inline uint8_t get_blerecv_type(unsigned char data)
{
    if (data == 'W')
    {
        return BLE_AT_WAR;
    }
    else if (data == 'O')
    {
        return BLE_AT_OK;
    }
    else if (data == 0xAA)
    {
        return BLE_AT_TUNNEL;
    }
    else if (data == 'E' || data == 'F')
    {
        return BLE_AT_ERROR;
    }
    else
    {
        return BLE_AT_OTHER;
    }
}

/*************************************************
 Function:     deal_recv_war
 Description:  处理蓝牙AT指令war类型的处理
 Input:
 1. data
 2. len
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static int deal_recv_war(char *data, int len)
{
    os_bluetooth_log(" deal_recv_war: g_BleState[%d]\n", g_BleState);
    if (0 == strncmp("WAR+LINK:Y", data, 10))
    {
        os_bluetooth_log("WAR+LINK:Y !!! \n");
        g_systime = mico_rtos_get_time();
        g_linkyFlag = 0;
        g_ScanState = 0;
    }
    else if (0 == strncmp("WAR+LINK:N", data, 10))
    {
        os_bluetooth_log("WAR+LINK:N !!! g_atDiscFlag[%d]\n", g_atDiscFlag);
        g_linknoflag = 1;
        g_bindSate = 0;
        if (mico_rtos_get_time() - g_systime < 1000 && g_systime != 0)
        {
            os_bluetooth_log(" resend ble connect...... \n");
            if (deal_resend() == BLE_OK)
            {
                return BLE_OK;
            }
        }
        deal_error(1);
//        g_BleState = BLE_IDLE;
        g_ScanState = 0;
        g_scanTimes = 0;
        g_IsCheck = 0;

        g_atDiscFlag = 0;
        g_connectFlag = 0;
        g_linkyFlag = 0;
    }
    else
    {
        if (g_BleState == BLE_REMO_PASS_UNLOCK)
        {
            if (mico_rtos_get_time() - g_systime < 2000 && g_systime != 0)
            {
                if (deal_resend() == BLE_OK)
                {
                    return BLE_OK;
                }
            }
        }
    }

    return BLE_OK;
}

/*************************************************
 Function:     deal_recv_ok
 Description:  处理蓝牙AT指令OK类型的处理
 Input:
 1. data
 2. len
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static int deal_recv_ok(char *data, int len)
{
    os_bluetooth_log("data : %s len : %d",data,len);
    char * lockmac = (char *)storage_get_present_lockmac();

    if (len >= 8 && 0 == strncmp("OK+SCAN:", data, 8)) // 扫描到蓝牙设备
    {
        os_bluetooth_log("Scan Ok!  Cnt: %c, g_BleState[%d]\n", data[8], g_BleState);

        if (data[8] != '0' && data[9] == '#')
        {
            if (g_BleState == BLE_LOCK_BIND)
            {
                int ret = parse_scan_data(&data[9], len - 9);
                g_bindSate = 0;
                if (ret == BLE_OK)
                {
                    g_scanTimes = 0;
                    if(g_bindSate == 1)
                    {
                       return BLE_OK;
                    }
                    com_ble_connect(g_lockMac);
                    g_bindSate = 1;
                    g_ScanState = 0;

                }
                else
                {

                    if(g_bindSate == 1)
                    {
                        return BLE_OK;
                    }

//                    g_scanTimes++;
//                    if (g_scanTimes <= 10) /*rescan 10 times*/
//                    {
//                        com_ble_scan();
//                    }
//                    else
//                    {
//                        if (g_BleState == BLE_LOCK_BIND && g_BleCallBackFunc)
//                        {
//                            int state = BLE_NOTI_ECHO_ERROR;
//                            g_BleCallBackFunc(BLE_NOTI_BIND_LOCK, (char *)&state, 4);
//                        }
//                        g_scanTimes = 0;
//                        g_BleState = BLE_IDLE;
//                    }
                    g_scanTimes = 0;
                    g_ScanState = 0;
                   g_BleState = BLE_LOCK_BIND;
                   com_ble_connect(g_lockMac1[Bindindex]);
                   memset(g_lockMacbak,0,sizeof(g_lockMacbak));
                   memcpy(g_lockMacbak,g_lockMac1[Bindindex],sizeof(g_lockMacbak));
                   os_bluetooth_log("g_lockMacbak :%s",g_lockMacbak);
                }
            }

            else if (g_BleState == BLE_IDLE)
            {
                int ret = parse_scan_signal(&data[9], len - 9);
                if (ret == BLE_ERROR && g_BleState == BLE_IDLE)
                {
                    g_scanTimes++;
                    if (g_scanTimes <= 4) /*rescan 5 times*/
                    {
                        com_ble_scan();
                    }
                    else
                    {
                        g_scanTimes = 0;
                    }
                }
                else
                {
                    g_scanTimes = 0;
                }


            }
        }
        else
        {
            os_bluetooth_log("Scan Fail!!!\n");

            if(g_BleState == BLE_LOCK_BIND && g_bindSate == 0)
            {
//                ble_bind_lock_start();
                   g_scanTimes = 0;
                   g_bindSate = 1;
                  g_BleState = BLE_LOCK_BIND;
                  com_ble_connect(g_lockMac1[Bindindex]);
                  memset(g_lockMacbak,0,sizeof(g_lockMacbak));
                  memcpy(g_lockMacbak,g_lockMac1[Bindindex],sizeof(g_lockMacbak));
                  os_bluetooth_log("g_lockMacbak :%s",g_lockMacbak);

               // com_ble_connect(g_lockMac);

            }
            else if(g_BleState == BLE_IDLE)
            {

//                com_ble_scan();
                g_BleState = BLE_GET_RECORD_INFO;
                com_ble_connect(lockmac);
                msleep(1000);
            }

        }
        g_ScanState = 0;

    }

    // 登入远程设备成功, 可以透传数据
    else if ((len >= 8) && (0 == strncmp("OK+PWD:Y", data, 8)))
    {
        g_IsCheck = 0;
        g_PwdEcho = TRUE;
        deal_login_ok();

        os_bluetooth_log("BleState == %d \n", g_BleState);
    }
    else if (len >= 8 && 0 == strncmp("OK+PWD:N", data, 8))
    {
        g_IsCheck = 0;
        deal_error(0);
        g_PwdEcho = FALSE;
        g_BleState = BLE_IDLE;
    }
    else if (len >= 7 && 0 == strncmp("OK+DISC", data, 7))
    {
        g_BleState = BLE_IDLE;
        g_ScanState = 0;
        g_scanTimes = 0;
        g_IsCheck = 0;
        if (g_fist == 0)
        {
            g_fist = 1;
        }
        else
        {
            g_atDiscFlag = 1;
        }
    }
    else if(len >= 9 && 0 == strncmp( "OK+STAS:A", data, 9 ) || len >= 9 && 0 == strncmp( "OK+STAS:E", data, 9 ))
    {
        g_ble_state = 0;
        g_linknoflag == 1;
    }
    else if (len >= 7 && 0 == strncmp("OK+MAC:", data, 7))
    {
        //        stor_set_localmac(data+7);
        os_bluetooth_log("ucRecvBuf AT+MAC : %s\n", data);
    }
    else if (len >= 9 && ((0 == strncmp("OK+ROLE:P", data, 9)) || (0 == strncmp("OK+ROLE:C", data, 9))))
    {
        os_bluetooth_log("set role mode echo OK\n");
    }
    else if (len >= 7 && 0 == strncmp("OK+CON:", data, 7))
    {
        g_connectFlag = 0;
        g_linknoflag = 0;
        //        g_linkyFlag = 1;
        os_bluetooth_log("ucRecvBuf OK+CON : %s\n", data);
    }
    else if(len >= 8 && 0== strncmp("OK+RESET",data,9))
    {
       // g_linknoflag = 1;

        g_bindSate = 0;
        os_bluetooth_log("ucRecvBuf OK+RESET : %s\n", data);
    }
    else if(len >= 11 && 0== strncmp("OK+SRSSI:Y",data,11))
    {
        g_finishflag = 1;
        g_BleCallBackFunc( BLE_NOTE_INIT_SUCESS, (char *) &g_finishflag, 1);

    }
    else if(len >= 20 && (0 ==strncmp("OK+NAME:13_BLE_COMM",data,20)))
    {
        com_ble_get_unlock_list(1);
        os_bluetooth_log("data : %s",data);
        g_BleState = BLE_GET_LOCK_LIST;
    }
    else if(len >= 20 && (0 ==strncmp("OK+NAME:03_BLE_COMM",data,20)))
    {
        com_ble_disconnect();
        g_BleState = BLE_IDLE;
    }
    else if(len >= 20 && (0 ==strncmp("OK+NAME:23_BLE_COMM",data,20)))
    {
       com_ble_disconnect();
       g_BleState = BLE_IDLE;
    }
//    MUTEX_UNLOCK(g_blue_send_mutex);
    return BLE_OK;
}

/*************************************************
 Function:     deal_recv_tunnel
 Description:  处理蓝牙AT指令tunnel类型的处理
 Input:
 1. data
 2. len
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static int deal_recv_tunnel(unsigned char *data, int len)
{

    if ((data[3] & 0x7F) == CMD_BLE_GET_RECORD_LIST && len > RECORD_CMD_LEN)
    {
        int num = len / RECORD_CMD_LEN;
        int i = 0;
        for (i = 0; i < num; i++)
        {
            recv_touchuan_data(data + i * RECORD_CMD_LEN, RECORD_CMD_LEN);
        }
    }
    else if ((data[3] & 0x7F) == CMD_BLE_SECURITY_CHECK && len > 13)
    {
        int num = len / 13;
        int i = 0;
        for (i = 0; i < num; i++)
        {
            recv_touchuan_data(data + i * 13, 13);
        }
    }
    else
    {
        //        os_bluetooth_log("deal_recv_tunnel: data[3]= %02X\n", data[3]);
        recv_touchuan_data(data, len);
    }

    return BLE_OK;
}

/*************************************************
 Function:     deal_recv_error
 Description:  处理蓝牙AT指令ERR类型的处理
 Input:
 1. data
 2. len
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static int deal_recv_error(char *data, int len)
{
    if (data[0] == 'E')
    {
        os_bluetooth_log("Echo Error!!!\n");
        g_BleState = BLE_IDLE;

        g_atDiscFlag = 0;
        g_connectFlag = 0;
        g_linkyFlag = 0;
    }
    else if (data[0] == 'F')
    {
        os_bluetooth_log("Echo Fail!!!\n");
        deal_error(1);
        g_BleState = BLE_IDLE;
        g_ScanState = 0;
        g_scanTimes = 0;
        g_IsCheck = 0;

        g_atDiscFlag = 0;
        g_connectFlag = 0;
        g_linkyFlag = 0;
    }

    return BLE_OK;
}

/*************************************************
 Function:     deal_uart_recv
 Description:  蓝牙串口接收处理函数
 Input:
 1. data
 2. len
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
static int deal_uart_recv(unsigned char *data, int len)
{
    uint8_t type = get_blerecv_type(data[0]);
    if ( data[0] == 0xaa )
    {
        PrintfByteHex( "recv data", data, len );
        os_bluetooth_log("deal_uart_recv:type[%d] \n",type);
    }
    else
    {
        os_bluetooth_log("deal_uart_recv: len[%d], data:%s type[%d] \n", len, data,type);
    }

    switch (type)
    {
    case BLE_AT_WAR:
        deal_recv_war((char *)data, len);
        break;

    case BLE_AT_OK:
        deal_recv_ok((char *)data, len);
        break;

    case BLE_AT_TUNNEL:
        deal_recv_tunnel(data, len);
        break;

    case BLE_AT_ERROR:
        deal_recv_error((char *)data, len);
        break;

    default:
        g_BleState = BLE_IDLE;
        os_bluetooth_log(" error data....\n");
        break;
    }
    return BLE_OK;
}

/*************************************************
 Function:     ble_uart_recv_callback
 Description:  蓝牙串口接收回调处理函数
 Input:
 1. recvPacket
 Output:       无
 Return:
 Others:
 *************************************************/
int ble_uart_recv_callback(PUART_RECV_PACKET recvPacket)
{

    int i = 0;
    if ((recvPacket->size > UART_BUF_SIZE) || recvPacket->data == NULL)
    {
        os_bluetooth_log("uart recv: the len of data > 256.\n");
        return recvPacket->size;
    }
    for (i = 0; i < recvPacket->size; i++)
    {
        if (g_tunnelFlag == 0 && recvPacket->data[i] == (unsigned char)0x0A)
        {
            //蓝牙命令结束
            ucRecvBuf[g_ucRecvLen] = 0;
            g_ucRecvLen++;
            deal_uart_recv((unsigned char *)ucRecvBuf, g_ucRecvLen);

            //os_bluetooth_log("Receive: data[%d][%d]= %s\n", i,g_tunnelFlag,ucRecvBuf);
            memset(ucRecvBuf, 0, sizeof(ucRecvBuf));
            g_ucRecvLen = 0;
        }
        else if (g_tunnelFlag == 0 && recvPacket->data[i] == (unsigned char)0xAA)
        {
            //透传命令开始
            memset(ucRecvBuf, 0, sizeof(ucRecvBuf));
            g_ucRecvLen = 0;
            ucRecvBuf[g_ucRecvLen] = recvPacket->data[i];
            g_ucRecvLen++;
            g_tunnelFlag = 1;
        }
        else if (g_tunnelFlag == 1 && g_ucRecvLen >= 2 && ucRecvBuf[1] + 2 == g_ucRecvLen + 1)
        {
            //透传命令结束
            ucRecvBuf[g_ucRecvLen] = recvPacket->data[i];
            g_ucRecvLen++;
            deal_uart_recv((unsigned char *)ucRecvBuf, g_ucRecvLen);

            //os_bluetooth_log("Receive: data[%d][%d]= %02X\n", i,g_tunnelFlag,data[i]);
            memset(ucRecvBuf, 0, sizeof(ucRecvBuf));
            g_ucRecvLen = 0;
            g_tunnelFlag = 0;
        }
        else
        {
            ucRecvBuf[g_ucRecvLen] = recvPacket->data[i];
            g_ucRecvLen++;
        }
        //os_bluetooth_log("Receive: ucRecvBuf %s\n", ucRecvBuf);
    }

    return recvPacket->size;
}
/*************************************************
  Function:     BleWakeUp
  Description:  唤醒蓝牙模块
  Input:        无
  Output:       无
  Return:       无
  Others:
*************************************************/
void BleWakeUp(void)
{
    // WK引脚下降沿唤醒模块
    WK_PIN_H();
    msleep(1);
    WK_PIN_L();
    msleep(5);
}
/*************************************************
 Function:     Ble_Send_deal_thread
 Description:  串口处理函数
 Input:        无
 Output:       无
 Return:       无
 Others:
 *************************************************/
int Ble_Send_deal_thread(void *argv)
{
    static int delay_flag = 0; // 1:延迟500ms 2:无延迟
    static unsigned int scanTimeout = 0;
    static unsigned int linkNTimeout = 0;
    static unsigned int conTimeout = 0;
    static unsigned int linkYTimeout = 0;

    os_bluetooth_log("Ble_Send_deal_thread: thread start!!!");
    while (1)
    {
        //解决扫描时应答数据错误导致无法退出扫描状态问题
        if (g_ScanState == 1)
        {
            unsigned int val = mico_rtos_get_time() - scanTimeout;
            if (val > 10000)
            {
                g_ScanState = 0;
                g_scanTimes = 0;
                g_MutexFlag = 0;
                g_bindSate = 0;
                g_BleState = BLE_IDLE;
                com_ble_reset();
                os_bluetooth_log(" scan timeout, reset blestate to idle.\n");
            }
        }
        else
        {
            scanTimeout = mico_rtos_get_time();
        }

        if (g_atDiscFlag == 1)
        {
            linkNTimeout++;
            //            os_bluetooth_log(" wait \"Link:N\" after receive \"OK+DISC\", timeout[%d].\n", linkNTimeout);
            msleep(100);

            if ( linkNTimeout > 30 )
            {
                linkNTimeout = 0;
                g_atDiscFlag = 0;
                com_ble_reset();

            }
            continue;
        }
        else
        {
            linkNTimeout = 0;
        }

        if (g_connectFlag == 1)
        {
            conTimeout++;
            os_bluetooth_log(" wait \"OK+CON:\" after send \"AT+CON\", timeout[%d].\n", conTimeout);
            msleep(100);

            if (conTimeout > 30)
            {
                conTimeout = 0;
                //                deal_resend();
                g_connectFlag = 0;
            }
            continue;
        }
        else
        {
            conTimeout = 0;
        }

        if (g_linkyFlag == 1)
        {
            linkYTimeout++;
            os_bluetooth_log(" wait \"LINK:Y\" after receive \"OK+CON:\", timeout[%d].\n", linkYTimeout);
            msleep(100);
            if ( linkYTimeout > 30 )
            {
                linkYTimeout = 0;
//                deal_resend();

                g_linkyFlag = 0;
            }
            continue;
        }
        else
        {
            linkYTimeout = 0;
        }

        delay_flag = 0;
        if (g_ScanState == 0 && g_MutexFlag == 0)
        {
//            os_bluetooth_log("mATcmdList[%d].cmd is %d, mPutIndex[%d]\n",mGetIndex,mATcmdList[mGetIndex].send_flag, mPutIndex);
            if (mATcmdList[mGetIndex].send_flag != 0)
            {
                delay_flag = 1;
                os_bluetooth_log("g_ScanState:%d g_MutexFlag : %d send_flag : %d send_type : %d ",\
                                                    g_ScanState,g_MutexFlag,mATcmdList[mGetIndex].send_flag,mATcmdList[mGetIndex].send_type);
                if (mATcmdList[mGetIndex].send_type != 1)
                {
                    //WK_PIN_L();
                    BleWakeUp();
                    msleep(100);
                    os_bluetooth_log("WK_PIN_L>>>>>>>>>>>>>>>>>>>>");
                    //                    os_bluetooth_log("send to uart0 mATcmdList[%d].cmd is %s len is %d time[%d]\n", \
//                            mGetIndex,mATcmdList[mGetIndex].cmd,mATcmdList[mGetIndex].cmd_len, mico_rtos_get_time());

                    if(g_linknoflag == 1)
                    {

                    }
                    WritePort(mATcmdList[mGetIndex].cmd, mATcmdList[mGetIndex].cmd_len);
                    g_AtCommNoEcho++;
                    if (mATcmdList[mGetIndex].send_flag == 2)
                    {
                        g_ScanState = 1;
                        scanTimeout = mico_rtos_get_time();
                    }

                    else if (mATcmdList[mGetIndex].send_flag == 3)
                    {
                        delay_flag = 1;
                        msleep(500);
                    }
                    else if (mATcmdList[mGetIndex].send_flag == 5)
                    {
                        delay_flag = 1;
                        g_connectFlag = 1;
                    }
                    mATcmdList[mGetIndex].send_flag = 0;
                    mGetIndex++;
                    if (mGetIndex == MAX_ATCMDLIST_SZIE)
                    {
                        mGetIndex = 0;
                    }
                    msleep(100);
                    WK_PIN_H();
                }
                else
                {
                    WK_PIN_H();
                    os_bluetooth_log("WK_PIN_H>>>>>>>>>>>>>>>>>>>>");
                    msleep(500);

                    //PrintfByteHex("send data", mATcmdList[mGetIndex].cmd, mATcmdList[mGetIndex].cmd_len);
                    //os_bluetooth_log("send to uart0 mATcmdList[%d].cmd is %s len is %d time[%d]\n",\
                        //  mGetIndex,mATcmdList[mGetIndex].cmd,mATcmdList[mGetIndex].cmd_len, mico_rtos_get_time());
                    WritePort(mATcmdList[mGetIndex].cmd, mATcmdList[mGetIndex].cmd_len);
                    g_AtCommNoEcho++;
                    mATcmdList[mGetIndex].send_flag = 0;
                    mGetIndex++;
                    if (mGetIndex == MAX_ATCMDLIST_SZIE)
                    {
                        mGetIndex = 0;
                    }
                }
            }
        }

        if (delay_flag == 0)
        {
            msleep(20);
        }
    }
}

/*************************************************
 Function:     ble_scan_thread
 Description:  蓝牙扫描线程
 Input:        无
 Output:       无
 Return:       无
 Others:
 在空闲状态下模块一直在扫描蓝牙锁,查看是否有新记录
 *************************************************/
int ble_scan_thread(void *argv)
{
    os_bluetooth_log("ble_scan_thread !!!");
#define MAX_SCAN_TIMEOUT 16
    static unsigned int BlestateCnt = 0;
    int bleIndex = 0;
    int bleNum = 0;
    storage_get_ble_mac(bleIndex, g_lockMac);
    os_bluetooth_log("g_lockMac[%s]\n",g_lockMac);
    while (1)
    {
        msleep(1000);

        bleNum = storage_get_object_num(DEVICE_OBJ_ID);
        for (bleIndex = 0; bleIndex < bleNum; bleIndex++)
        {
//            if (strlen(g_lockMac) != BLU_MAC_LEN)
//            {
//                IdleCnt = 0;
//                continue;
//            }
            if (g_BleState == BLE_IDLE && g_ScanState == 0 && get_loginAuthInterval() == 1 )
            {
                if (++IdleCnt >= MAX_SCAN_TIMEOUT)
                {

                    IdleCnt = 0;
                    com_ble_scan();
                    ssivaluecount++;
                    os_bluetooth_log("IdleCnt: %d ssivaluecount : %d\n", IdleCnt,ssivaluecount);

                }
                if(++BlestateCnt >= 8)
                {
                    com_ble_get_stas();


                    BlestateCnt = 0;
                    g_bindSate = 0;
                    rssiValueindex++;
                   if(rssiValueindex == 10)
                   {
                       rssiValueindex = 0;
                   }

                }
            }

            else
            {
                IdleCnt = 0;
                BlestateCnt = 0;
            }
        }
    }
}

void refresh_timeout(uint8_t flag)
{
    TimeoutCnt = flag;
}
/*************************************************
 Function:     ble_timeout_thread
 Description:  蓝牙操作超时处理
 Input:        无
 Output:       无
 Return:       无
 Others:
 *************************************************/
int ble_timeout_thread(void *argv)
{
    os_bluetooth_log("ble_timeout_thread !!!");
#define MAX_DEAL_TIMEOUT 8 // 8秒超时

    unsigned char lockbuf[1] = {0};
    int state = 0;
    static char online = 1;
    static int Bind_Timeout = 0;
    while (1)
    {
        msleep(1000);

        if(g_BleState == BLE_IDLE)
        {
            if(g_bind_status == 1 && (storage_get_mac_sucess() != 0))
            {
                if (++Bind_Timeout >= 10)
                {
                    os_bluetooth_log("Bind_Timeout : %d",Bind_Timeout);
                    g_bind_status = 0;
                    Bind_Timeout = 0;
                    ble_bind_lock_start();

                }
            }

        }


        if(ssivaluecount % 8 == 0 && elink_get_join_net_status() == 1)
        {
            ssivaluecount = 1;
            if (g_BleCallBackFunc)
            {
               g_BleCallBackFunc(BLE_NOTI_RSSI_UPLINK,
                                 ssiValue[rssiValueindex],
                                1);
            }
        }


        if (g_BleState == BLE_UNLOCK)
        {
            if (++TimeoutCnt >= MAX_DEAL_TIMEOUT)
            {
                os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
                TimeoutCnt = 0;
                send_touchuan_flag = 0;
                g_BleState = BLE_IDLE;
                lockbuf[0] = 0x80; // 失败
                com_ble_disconnect();
                g_BleCallBackFunc(BLE_NOTI_UNLOCK, (char *)lockbuf, 1);
            }
        }
        else if (g_BleState == BLE_LOCK_BIND)
        {

            if(++TimeoutCnt >= MAX_DEAL_TIMEOUT + 12)
            {
                send_touchuan_flag = 0;
                g_BleState = BLE_IDLE;
                state = BLE_NOTI_ECHO_ERROR;
                com_ble_disconnect();
                g_BleCallBackFunc(BLE_NOTI_BIND_LOCK, (char *)&state, 1);
            }

        }
        else if (g_BleState == BLE_SET_USERPIN)
        {
            if (++TimeoutCnt >= MAX_DEAL_TIMEOUT + 10)
            {
                os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
                TimeoutCnt = 0;
                send_touchuan_flag = 0;
                lockbuf[0] = 0x80;
                g_BleState = BLE_IDLE;
                com_ble_disconnect();
                g_BleCallBackFunc(BLE_NOTI_SET_USERPIN, (char *)lockbuf, 2);
            }
        }
        else if (g_BleState == BLE_GET_LOCKINFO)
        {
            if (++TimeoutCnt >= MAX_DEAL_TIMEOUT)
            {
                os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
                TimeoutCnt = 0;
                lockbuf[0] = 0x80;
                send_touchuan_flag = 0;
                g_BleState = BLE_IDLE;
                com_ble_disconnect();

                g_BleCallBackFunc(BLE_NOTI_ERROR, (char *)lockbuf, 2);
            }
        }

        else if (g_BleState == BLE_REMO_PASS_UNLOCK)
        {
            if (++TimeoutCnt >= MAX_DEAL_TIMEOUT)
            {
                os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
                TimeoutCnt = 0;
                lockbuf[0] = 0x80;
                send_touchuan_flag = 0;
                g_BleState = BLE_IDLE;
                g_BleCallBackFunc(BLE_NOTI_ERROR, (char *)lockbuf, 2);
            }
        }
        else if (g_add_del_tmp_pass == 1 )
        {
            if (++TimeoutCnt >= MAX_DEAL_TIMEOUT)
            {
                os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
                TimeoutCnt = 0;
                g_linknoflag = 1;
                lockbuf[0] = 0x00;
                send_touchuan_flag = 0;
                g_BleState = BLE_IDLE;
                g_add_del_tmp_pass = 0;
                ble_add_tmp_pwd();

            }
        }
        else if (g_del_tmp_pass == 1 )
        {
          if (++TimeoutCnt >= MAX_DEAL_TIMEOUT)
          {
              os_bluetooth_log("TimeoutCnt: %d\n", TimeoutCnt);
              TimeoutCnt = 0;
              lockbuf[0] = 0x00;
              send_touchuan_flag = 0;
              g_linknoflag = 1;
              g_BleState = BLE_IDLE;
              g_add_del_tmp_pass = 0;
              g_del_tmp_pass = 0;
              ble_del_tmp_pwd();

          }
        }
        else
        {
            TimeoutCnt = 0;
        }
    }
}

/*************************************************
 Function:     ble_bind_lock_start
 Description:  开始绑定蓝牙锁
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_bind_lock_start(void)
{
    os_bluetooth_log(" ble_bind_start");
    g_BleState = BLE_LOCK_BIND;
    com_ble_scan();
    g_scanTimes = 1;
    g_bind_status = 1;
    g_bindtimecout = mico_rtos_get_time();
    return BLE_OK;
}

/*************************************************
 Function:     ble_bind_lock_end
 Description:  结束绑定蓝牙锁
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_bind_lock_end(void)
{
    if (g_BleState == BLE_LOCK_BIND)
    {
        g_BleState = BLE_IDLE;
        //g_ScanState = 0;
        g_scanTimes = 0;
        g_IsCheck = 0;
    }
    return BLE_OK;
}

/*************************************************
 Function:     ble_bind_lock_bymac
 Description:  绑定锁，已知蓝牙mac码
 Input:
 1.blemac    蓝牙mac码
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_bind_lock_bymac(char *blemac)
{
    if (blemac == NULL || strlen(blemac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_bind_lock_bymac: blemac error.\n");
        return BLE_ERROR;
    }
    os_bluetooth_log(" ble_bind_lock_bymac\n");
    g_BleState = BLE_LOCK_BIND;
    memset(g_lockMac, 0, sizeof(g_lockMac));
    memcpy(g_lockMac, blemac, strlen(blemac));
    com_ble_connect(blemac);

    return BLE_OK;
}

/*************************************************
 Function:     ble_set_userpin
 Description:  设置蓝牙用户pin码
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:       蓝牙正在连接时可直接发透传数据，否则需进行蓝牙连接
 *************************************************/
int ble_set_userpin(char *pUserpin)
{
    char *lockmac = storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_set_userpin, lockmac error.\n");
        return BLE_ERROR;
    }
    g_BleState = BLE_SET_USERPIN;

    // 保存需要变更的 pin码到临时缓冲区中
    memset(g_tempUserPin, 0, sizeof(g_tempUserPin));
    memcpy(g_tempUserPin, pUserpin, BLU_PWD_LEN);
    memcpy( g_UserPinbak,pUserpin,BLU_PWD_LEN);
    os_bluetooth_log("g_tempUserPin>>>>>[%s]\n", g_tempUserPin);
    com_ble_connect(lockmac);
    return BLE_OK;
}

/*************************************************
 Function:     ble_set_broadrate
 Description:  设置蓝牙电源模式(广播频率)
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:       蓝牙正在连接时可直接发透传数据，否则需进行蓝牙连接
 *************************************************/
int ble_set_broadrate(unsigned char mode)
{
#ifdef BLE_DEVICE
    char *lockmac = storage_get_present_lockmac();
#endif
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_set_broadrate, lockmac error.\n");
        return BLE_ERROR;
    }
    g_BleState = BLE_SET_BROADRATE;
    g_tempBroadrate = mode;
    com_ble_connect(lockmac);
    return BLE_OK;
}

/*************************************************
 Function:     ble_unlock_start
 Description:  开锁
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:       蓝牙正在连接时可直接发透传数据，否则需进行蓝牙连接
 *************************************************/
int ble_unlock_start(void)
{
    char *lockmac = storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    if (g_BleState == BLE_GET_RECORD_INFO)
    {
        com_ble_disconnect();
    }
    g_BleState = BLE_UNLOCK;

    os_bluetooth_log(" ble_unlock_start: start timer.[%d]\n", mico_rtos_get_time());
    com_ble_connect(lockmac);
    return BLE_OK;
}

/*************************************************
 Function:     ble_set_channel
 Description:
 Input:
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_set_channel(void)
{
#ifdef BLE_DEVICE
    char *lockmac = storage_get_present_lockmac();
#endif

    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    g_BleState = BLE_SET_LOCKCHANGNEL;
    com_ble_connect(lockmac);
    return BLE_OK;
}
/**
 * 设置管理员密码
 */
void ble_set_adminpassword(void)
{
    OSStatus ret;
    stBLELockStatus lockstate;
    stor_lockstatue_read(&lockstate);
    if ( !strcmp( lockstate.g_adminpwd, g_adminpwd ) )
    {
        os_bluetooth_log("lockstate.admin : [%x][%x][%x][%x]",lockstate.g_adminpwd[0],lockstate.g_adminpwd[1],\
                             lockstate.g_adminpwd[2],lockstate.g_adminpwd[3]);
    }
    else
    {
        memcpy(lockstate.g_adminpwd,g_adminpwd,sizeof(g_adminpwd));
        os_bluetooth_log("lockstate.admin : [%x][%x][%x][%x]",lockstate.g_adminpwd[0],lockstate.g_adminpwd[1],\
                        lockstate.g_adminpwd[2],lockstate.g_adminpwd[3]);
        ret = stor_lockstatue_write( OBJECT_UPDATE_ADD, 1, &lockstate );
    }



}
/***
 *  获取管理员密码
 */
void ble_get_adminpassword(void)
{
    stBLELockStatus lockstate;
    stor_lockstatue_read(&lockstate);
    memcpy(g_adminpwd,lockstate.g_adminpwd,sizeof(g_adminpwd));
    os_bluetooth_log("g_adminpwd : [%x][%x][%x][%x]",g_adminpwd[0],g_adminpwd[1],\
                 g_adminpwd[2],g_adminpwd[3]);
}
int ble_add_tmp_pwd(void)
{
    char *lockmac = storage_get_present_lockmac();
    IdleCnt = 1;
    g_add_del_tmp_pass = 1;
    refresh_timeout(0);
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
       os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
       return BLE_ERROR;
    }
    g_BleState = BLE_ADD_TMP_SEC_UNLOCK;
    if(g_linknoflag == 1)
    {
        g_connectFlag = 0;
        g_linkyFlag = 0;
        com_ble_connect(lockmac);
    }
}
/***********stor_lockstatue_read**************************************
 Function:     ble_add_tmp_pwd_start
 Description:
 Input:        添加临时秘钥开门
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_add_tmp_pwd_start(uint16_t id, uint8_t locktype, uint8_t *adminpwd, uint8_t *pwd,
                          mico_rtc_time_t *eff_time,
                          mico_rtc_time_t *no_eff_time, uint8_t cnt)
{
    g_add_del_tmp_pass = 1;
    IdleCnt = 1;
    char *lockmac = storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    g_usrid = id;
    g_locktype = locktype;
    memset(g_adminpwd, 0, sizeof(g_adminpwd));
    StrToHexEx(adminpwd, strlen(adminpwd), g_adminpwd);
    PrintfByteHex("adminpwd", g_adminpwd, 4);
    memset(g_lockpwd, 0, sizeof(g_lockpwd));
    StrToHexEx(pwd, strlen(pwd), g_lockpwd);
    PrintfByteHex("g_lockpwd", g_lockpwd, 4);
    //ble_set_adminpassword();         // set admin
    effect_time.year = eff_time->year;
    effect_time.month = eff_time->month;
    effect_time.date = eff_time->date;
    effect_time.hr = eff_time->hr;
    effect_time.min = eff_time->min;
    effect_time.sec = eff_time->sec;

    invalid_time.year = no_eff_time->year;
    invalid_time.month = no_eff_time->month;
    invalid_time.date = no_eff_time->date;
    invalid_time.hr = no_eff_time->hr;
    invalid_time.min = no_eff_time->min;
    invalid_time.sec = no_eff_time->sec;

    g_cnt = cnt;
    g_BleState = BLE_ADD_TMP_SEC_UNLOCK;
    if(g_linknoflag == 1)
    {
        g_connectFlag = 0;
        g_linkyFlag = 0;
        com_ble_connect(lockmac);
    }
    else if(g_IsCheck == 0)
    {
        com_ble_add_tmp_sec_unlock_one();
        msleep(200);
        com_ble_add_tmp_sec_unlock_two();
    }

    return BLE_OK;

}
/***
 *
 * @删除临时秘钥
 */
int ble_del_tmp_pwd(void)
{
    IdleCnt = 1;
    g_del_tmp_pass = 1;
    refresh_timeout(0);
    char *lockmac = (char *)storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
       os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
       return BLE_ERROR;
    }
    g_BleState = BLE_DEL_TMP_SEC_UNLOCK;
    if(g_linknoflag == 1)
    {

        g_connectFlag = 0;
        g_linkyFlag = 0;
        com_ble_connect(lockmac);
    }
    else if(g_PwdEcho == TRUE)
    {
        com_ble_del_tmp_sec_unlock();
    }
}

/*************************************************
 Function:     ble_del_tmp_pwd_start
 Description:
 Input:        删除临时秘钥
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_del_tmp_pwd_start(uint16_t id, uint8_t *adminpwd)
{
    g_del_tmp_pass = 1;
    IdleCnt = 1;
    char *lockmac = storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    id = id /100;
    g_usrid = id;
    ble_get_adminpassword();      // get admin
    g_BleState = BLE_DEL_TMP_SEC_UNLOCK;
    if(g_linknoflag == 1)
    {

        g_connectFlag = 0;
        g_linkyFlag = 0;
        com_ble_connect(lockmac);
    }
    else if(g_IsCheck == 0)
    {
        com_ble_del_tmp_sec_unlock();
    }


    return BLE_OK;

}

/*************************************************
 Function:     ble_remote_pwd_unlock_start
 Description:
 Input:        远程密码开锁
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_remote_pwd_unlock_start(uint8_t *password)
{
    char *lockmac = (char *)storage_get_present_lockmac();
      if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
      {
          os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
          return BLE_ERROR;
      }
      memset(g_lockpwd, 0, sizeof(g_lockpwd));
      StrToHexEx(password, strlen(password), g_lockpwd);
      g_BleState = BLE_REMO_PASS_UNLOCK;
      com_ble_connect(lockmac);
      return BLE_OK;

}

/*************************************************
 Function:     ble_remote_pwd_local_unlock_start
 Description:
 Input:        远程密码开锁
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_remote_pwd_local_unlock_start(void)
{
    if(g_linknoflag == 1)
    {
          char *lockmac = (char *)storage_get_present_lockmac();
          if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
          {
              os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
              return BLE_ERROR;
          }
          g_BleState = BLE_REMO_PASS_UNLOCK;
          com_ble_connect(lockmac);
          return BLE_OK;
      }
    return BLE_OK;
}
/*************************************************
 Function:     ble_force_get_lock_list
 Description:
 Input:        强制获取获取门锁列表
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_force_get_lock_list(uint8_t flag)
{

    char *lockmac = (char *)storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    if (flag == 0)
    {

        g_BleState = BLE_FORCE_GET_LOCK_LIST;
    }
    else
    {
        g_BleState = BLE_GET_LOCK_LIST;
    }

    com_ble_connect(lockmac);
    return BLE_OK;
}
/*************************************************
 Function:     ble_set_time
 Description:
 Input:        设置时间同步
 Output:
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_set_time(void)
{
    char *lockmac = (char *)storage_get_present_lockmac();
    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_unlock_start, lockmac error.\n");
        return BLE_ERROR;
    }
    g_BleState = BLE_SET_TIME;
    com_ble_connect(lockmac);
    return BLE_OK;
}

/*************************************************
 Function:     ble_set_factory
 Description:
 Input:            无
 Output:           无
 Return:           BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_set_factory(void)
{
    char *lockmac = (char *)storage_get_present_lockmac();

    if (lockmac == NULL || strlen(lockmac) != BLU_MAC_LEN)
    {
        os_bluetooth_log(" ble_set_factory, lockmac error.\n");
        return BLE_ERROR;
    }
    g_BleState = BLE_SET_FACTORY;

    com_ble_connect(lockmac);
    return BLE_OK;
}

/*************************************************
 Function:     ble_get_lockinfo
 Description:  获取锁状态
 Input:        无
 Output:       无
 Return:       BLE_OK/BLE_ERROR
 Others:
 *************************************************/
int ble_get_lockinfo(void)
{
#ifdef BLE_DEVICE
    char *lockmac = storage_get_present_lockmac();
#endif
    if (lockmac != NULL && strlen(lockmac) == BLU_MAC_LEN)
    {
        g_BleState = BLE_GET_LOCKINFO;
        com_ble_connect(lockmac);
    }
    return BLE_OK;
}

/***
 * ble initfinish
 * @return
 */
int ble_init_finish(void)
{
    return g_finishflag;
}

/********************************************************
 * function:  ble_mesh_timeout_handle
 * description:
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
void ble_mesh_timeout_handle( void )
{
    ble_setmesh_config_exit();
    g_bind_status = 0;
}
/********************************************************
 * function:  ble_start_mesh_timer
 * description:
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
void ble_start_mesh_timer( uint16_t timeout )
{
    if (_ble_mesh_timer_initialized == TRUE)
    {
        if (timeout == 0)
        {
            ble_setmesh_config_exit();
        }
        else
        {
            ble_setmesh_stop_timer();
        }
    }

    if (timeout != 0)
    {
        mico_init_timer(&_ble_mesh_timer, timeout*1000, ble_mesh_timeout_handle, NULL);
        mico_start_timer(&_ble_mesh_timer);
        _ble_mesh_timer_initialized = true;
    }

}
void ble_setmesh_stop_timer()
{
    mico_stop_timer(&_ble_mesh_timer);
    mico_deinit_timer( &_ble_mesh_timer );
    _ble_mesh_timer_initialized = false;
}


void ble_setmesh_config_exit( void )
{
    if (_ble_mesh_timer_initialized == TRUE)
    {
        ble_setmesh_stop_timer();
    }
}
/********************************************************
 * function:  ble_logic_init
 * description:  ble logic init
 * input:       pBleCallback
 * output:
 * return:
 * auther:   chengc
 * other:
 *********************************************************/
OSStatus ble_logic_init(PBLE_STATE_CALLBACK pBleCallback)
{
    OSStatus err = kNoErr;

    /*UART receive thread*/
    uart_logic_init();
    uart_config_ble_recv_cb(ble_uart_recv_callback);
    g_BleState = BLE_IDLE;

    if (pBleCallback != NULL)
    {
        g_BleCallBackFunc = pBleCallback;
    }

    if (!g_DealInitFlag)
    {
        g_DealInitFlag = 1;

        err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART scan",
                                      ble_scan_thread,
                                      0x1000,
                                      0);
        os_bluetooth_log("UART Recv err: %d", err);
    }

    if (!g_SendInitFlag)
        g_SendInitFlag = 1;

    memset(mATcmdList, 0, sizeof(mATcmdList));
    mGetIndex = 0;
    mPutIndex = 0;
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Send Queue Deal",
                                  Ble_Send_deal_thread,
                                  0x800,
                                  0);

    if (!g_SendInitFlag)
        g_SendInitFlag = 1;

    mGetIndex = 0;
    mPutIndex = 0;
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "timeout  Deal",
                                  ble_timeout_thread,
                                  0x1500,
                                  0);
    com_ble_init();
    err = storage_set_present_lockpin();

    os_bluetooth_log(" ======== ble_logic_init ========= \n");
    return BLE_OK;
}
