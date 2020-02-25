/*
 * os_uart_logic.c
 *
 *  Created on: 2017年9月5日
 *      Author: Administrator
 *
 *  other:
 *  修改记录说明如下
 *  1. 取消串口等待应答时的重发动作。(UartSendAndRecvEx 函数中更改) 2018-09-03
 *
 */

#include "mico.h"
#include "uart_logic.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../uart/DTTable.h"
#include "../MLinkGpio/MLinkLedLogic.h"
#include "../MLinkAppDef.h"
#define os_uart_log(M, ...)  custom_log("UART LOG", M, ##__VA_ARGS__)
#define os_uart_log_trace()  custom_log_trace("UART DEBUG")

extern int mico_debug_enabled;


/******************************************************************************
 *                                Constants
 ******************************************************************************/
#define MIN(value1, value2)         ((value1<=value2) ? value1 : value2)
#define MAX(value1, value2)         ((value1>=value2) ? value1 : value2)

#define FRAME_MIN_SIZE              5

#ifdef BLE_DEVICE
#define BAUD_RATE                   9600
#else
#define BAUD_RATE                   115200
#endif

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[UART_DRIVER_BUFFER_LENGTH];
static uint8_t m_bRcvReady;
static unsigned char m_pSndBuf[512] = {0};
static unsigned char g_recvTemp[512] = {0};
static uint32_t g_recvTempLen = 0;
static uint32_t m_nSeq = 0;
static uint32_t g_mid = 0;
static QUEUE uart_send_queue;
static QUEUE uart_recv_queue;


int m_RcvWaitMs = 0;

uint8_t *m_pRcvBuf = NULL;
uint8_t g_SendUartFlag = 0;
mico_mutex_t g_waitRecvMutex;
static uint8_t uart_link_layer_comm = 1;

static discover_notify_callback_t discover_dev_notify = NULL;
static uart_recv_deal_callback zigbee_passthrough_deal_cb = NULL;
static uart_recv_deal_callback zigbee_passthrough_rsp_deal_cb = NULL;
static uart_recv_deal_callback rf433_passthrough_deal_cb = NULL;
static uart_recv_deal_callback rf433_passthrough_rsp_deal_cb = NULL;

#if (defined BLE_DEVICE)
static uart_recv_deal_callback ble_recv_deal_cb = NULL;
#endif

static uint32_t g_sendDiffRev = 0;
static uint32_t g_sendCount = 0;

static uint16_t crc16_tab[256]=
{
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/*************************************************
  Function          :       PrintfByteHex
  Description       :   打印一串字符各字节16进制值
  Input:
      DesStr:打印描述
      data:  实际需要打印的字符串
      dataLen:打印字节数
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void PrintfByteHex(char *DesStr, uint8_t *data, uint32_t dataLen)
{
    if (mico_debug_enabled)
    {
        int Count = 0;
        os_uart_log("");
        if (DesStr==NULL || data==NULL)
        {
    //        printf("%s: point is null, fail to print\r\n", __FUNCTION__);
            return;
        }
        printf("%s:\r\n", DesStr);
        for (Count=0; Count<dataLen; Count++)
        {
            printf("%02x ", *(data+Count));
        }
        printf("\n");
    }
}


/********************************************************************
函数名称:CRC-16 Process
函数原型:INT16U make_crc16(INT8U *msgaddr,INT8U datalen)
函数功能:进行CRC校验和产生CRC代码.这个函数只影响全局变量crc16.
校验字放在字符串最后,低8位在前高8位在后.
msgaddr  : 进行CRC16校验的据块的首地址
datalen  : 进行CRC16校验的据块的个数

  CRC-ITU的计算算法如下：
  a.寄存器组初始化为全"1"(0xFFFF)。
  b.寄存器组向右移动一个字节。
  c.刚移出的那个字节与数据字节进行异或运算，得出一个指向值表的索引。
  d.索引所指的表值与寄存器组做异或运算。
  f.数据指针加1，如果数据没有全部处理完，则重复步骤b。
  g.寄存器组取反，得到CRC，附加在数据之后(这一步可省略)。
********************************************************************/
uint16_t MakeCrc16(uint8_t *msgaddr,uint8_t datalen)
{
    uint8_t temp_data;                        //临时变量
    uint16_t crc16;
    crc16=0xffff;                           //初始化
    while(datalen--)
    {
        temp_data=crc16^*msgaddr++;
        crc16>>=8;
        crc16^=crc16_tab[temp_data];
    }
    return crc16;
}

/********************************************************
 * function:  GetCrc8
 * description:
 * input:       1. pBuffer: the data you want to make crc
 * output:      2. len: the length to pBuffer and the length must less the 255
 * return:
 * auther:   chenb
 * other:
*********************************************************/
uint8_t GetCrc8( uint8_t* pBuffer, uint8_t len)
{
    int i;
    uint8_t sum = 0;
    for (i = 0; i < pBuffer[2]-1 ; i++) {
        sum += pBuffer[i];
    }
    return sum;
}


/********************************************************
 * function:  uart_send_queue_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_queue_init( QUEUE *queue )
{
    return init_queue(queue, sizeof(UART_FRAME_DATA_T));
}

/*************************************************
  Function:     uart_push_node
  Description:  串口发送入队
  Input:
    queue       队列指针
    data        欲入队的数据指针
  Output:
  Return:
    成功      AU_SUCCESS
    失败      AU_FAILURE
*************************************************/
int uart_push_node( QUEUE *queue, PUART_FRAME_DATA_T uart_data )
{
    int ret = 0;
    ret = push_node(queue,(void*)uart_data);
    return ret;
}

/*************************************************
  Function:     uart_pop_node
  Description:  串口发送数据出队
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
PUART_FRAME_DATA_T uart_pop_node(QUEUE *queue)
{
    return (PUART_FRAME_DATA_T)pop_node(queue);
}

/********************************************************
 * function:  WritePort
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus WritePort(uint8_t *buf,int dwCharToWrite)
{
    OSStatus ret = 0;
#if 0
    ret = MicoUartSend(UART_MLINK_PIC, buf, dwCharToWrite);
    os_uart_log("data len is %d", dwCharToWrite);
    PrintfByteHex("send data", buf, dwCharToWrite);
    if (m_pSndBuf != NULL) {
        memcpy(m_pSndBuf, buf, dwCharToWrite);
    }
#else
    UART_FRAME_DATA_T sendData = {0};
    memcpy(sendData.data, buf, dwCharToWrite);
    sendData.dataSize = dwCharToWrite;

#ifdef BLE_DEVICE
    ret = MicoUartSend(UART_MLINK_PIC, sendData.data, sendData.dataSize);
//    os_uart_log("uart send err: %d", ret);
#else
//    PrintfByteHex("send data", sendData.data, sendData.dataSize);
    uart_push_node(&uart_send_queue, &sendData);
#endif
#endif
    return ret;
}


/********************************************************
 * function:  LOBYTE
 * description:
 * input:       1. value
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
uint8_t LOBYTE(uint16_t value)
{
    uint16_t loByteValue = 0;
    uint8_t lowByte = 0;
    loByteValue = value&0x00ff;
    lowByte = (uint8_t)loByteValue;
    return lowByte;
}

/********************************************************
 * function:  HIBYTE
 * description:
 * input:       1. value
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
uint8_t HIBYTE(uint16_t value)
{
    uint16_t hiByteValue = 0;
    uint8_t highByte = 0;
    hiByteValue = (value&0xff00) >>8;
    highByte = (uint8_t)hiByteValue;
    return highByte;
}


/********************************************************
 * function:  GetMID
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int GetMID(uint32_t *mid)
{
    *mid = g_mid;
    return TRUE;
}

/********************************************************
 * function:  SetMID
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void SetMID(uint8_t *mid)
{
    memcpy(&g_mid, mid, 3);
}

void set_recv_flag(uint8_t flag)
{
    m_bRcvReady = flag;
}

uint8_t get_recv_flag()
{
    return m_bRcvReady;
}

/********************************************************
 * function:  recv_buffer_set
 * description:
 * input:       1. value
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void recv_buffer_set( uint8_t* recvbuff )
{
    m_pRcvBuf = recvbuff;
}

/********************************************************
 * function:  UartSend
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus UartSend(uint16_t dest, uint8_t cmd, uint8_t* pBuffer, int len)
{
    uint8_t buffer[TP_FRAME_MAX_SIZE];
    OSStatus ret = 0;

    buffer[0] = 0xaa;
    buffer[1] = 0x20|(++m_nSeq&0x0f);
    buffer[2] = len+7;
    buffer[3] = cmd;
    buffer[4] = LOBYTE(dest);
    buffer[5] = HIBYTE(dest);
    if (pBuffer != NULL)
    {
        memcpy(buffer+6, pBuffer, len);
    }
    buffer[6+len] = GetCrc8(buffer, buffer[2]);

    ret = WritePort(buffer, buffer[2]);
    return ret;

}

/********************************************************
 * function:  UartSendNoAck
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void UartSendNoAck(uint16_t dest, uint8_t cmd, uint8_t* pBuffer, int len)
{
    uint8_t buffer[TP_FRAME_MAX_SIZE];

    buffer[0] = 0xaa;
    buffer[1] = (++m_nSeq&0x0f);
    buffer[2] = len+7;
    buffer[3] = cmd;
    buffer[4] = LOBYTE(dest);
    buffer[5] = HIBYTE(dest);
    memcpy(buffer+6, pBuffer, len);
    buffer[6+len] = GetCrc8(buffer, buffer[2]);
    WritePort(buffer, buffer[2]);
}

/********************************************************
 * function:  UartSendAndRecv
 * description: send
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int UartSendAndRecv(uint16_t dest, uint8_t cmd, uint8_t* pBuffer, int len, uint8_t* pRecv )
{
    mico_rtos_lock_mutex(&g_waitRecvMutex);
    int retry = 0;
    m_RcvWaitMs = 2000;
    recv_buffer_set(pRecv);
    set_recv_flag(TRUE);
    UartSend(dest, cmd, pBuffer, len);
    while(1) {
        mico_thread_msleep(20);
        m_RcvWaitMs -= 20;
        if (m_RcvWaitMs <= 0) {
            if (retry-- > 0) {
                m_RcvWaitMs = 3000;
                m_nSeq--;
                UartSend(dest, cmd, pBuffer, len);
            }else {
                set_recv_flag(FALSE);
                mico_rtos_unlock_mutex(&g_waitRecvMutex);
                return -1;
            }
        }
        if ( FALSE == get_recv_flag()) {
            mico_rtos_unlock_mutex(&g_waitRecvMutex);
            return pRecv[2];
        }
    }
    set_recv_flag(FALSE);
    mico_rtos_unlock_mutex(&g_waitRecvMutex);
    return -1;
}

/********************************************************
 * function:  UartSendAndRecvEx
 * description: send
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int UartSendAndRecvEx(uint16_t dest, uint8_t cmd, uint8_t* pBuffer, int len, uint8_t* pRecv, uint16_t wait_time)
{
    mico_rtos_lock_mutex(&g_waitRecvMutex);
    int retry = 0;          // 取消重发
    m_RcvWaitMs = wait_time;
    recv_buffer_set(pRecv);
    UartSendNoAck(dest, cmd, pBuffer, len);
    set_recv_flag(TRUE);
    while(1) {
        mico_thread_msleep(20);
        m_RcvWaitMs -= 20;
        if (m_RcvWaitMs <= 0) {
            if (retry-- > 0) {
                m_RcvWaitMs = wait_time;
                m_nSeq--;
                UartSendNoAck(dest, cmd, pBuffer, len);
            }else {
                set_recv_flag(FALSE);
                mico_rtos_unlock_mutex(&g_waitRecvMutex);
                return -1;
            }
        }
        if ( FALSE == get_recv_flag()) {
            mico_rtos_unlock_mutex(&g_waitRecvMutex);
            return pRecv[2];
        }
    }
    set_recv_flag(FALSE);
    mico_rtos_unlock_mutex(&g_waitRecvMutex);
    return -1;
}

/********************************************************
 * function:  UartMlSend
 * description:
 * input:       1. addr: total: 3 byte.  panid(1B) + addr(2B)
 *              2. op:
 *              3. buffer: payload data
 *              4. size:   the length of payload
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus UartMlSend(unsigned char * addr, uint16_t op, char* buffer, int size)
{
    uint8_t buffer2[256]= {0};
    uint32_t MID;
    uint16_t dest = 0;

    size = MIN(size, ML_PARAM_MAX_SIZE);
    if (GetMID(&MID)) {
        memcpy(buffer2, &MID, 3);
        memcpy(buffer2+3, addr, 3);
        buffer2[6] = 0;
        memcpy(buffer2+7, &op, 2);
        memcpy(buffer2+9, buffer, size);
        memcpy(&dest, buffer2+3, 2);
        UartSendNoAck(dest, DEV_CMD_PASSTHROUGH, buffer2, 9+size);
        return 0;
    }else {
        os_uart_log("无效的设备类型");
        return -1;
    }
}

/********************************************************
 * function:  UartMlSend
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus UartMlSendEx(unsigned char *addr, uint16_t op, char* buffer, int size, uint8_t* pRecv, uint16_t wait_time )
{
    uint8_t buffer2[256]= {0};
    uint32_t MID;
    uint16_t dest = 0;
    int ret = 0;

    size = MIN(size, ML_PARAM_MAX_SIZE);
    if (GetMID(&MID)) {
        memcpy(buffer2, &MID, 3);
        memcpy(buffer2+3, addr, 3);
        buffer2[6] = 0;
        memcpy(buffer2+7, &op, 2);
        memcpy(buffer2+9, buffer, size);
        memcpy(&dest, buffer2+3, 2);
        ret = UartSendAndRecvEx(dest, DEV_CMD_PASSTHROUGH, buffer2, 9+size, pRecv, wait_time);
    }else {
//        Log("无效的设备类型");
        ret = -1;
    }
    return ret;
}

///*************************************************
//  Function          :      UartSendResponse
//  Description       :   distribute the passthrough data
//  Input:
//      uart_recv_data:  uart data
//      recv_len:           data length
//  Output            :       无
//  Return            :
//  Others            :       无
//
//*************************************************/
//OSStatus UartSendResponse(uint8_t *data, uint8_t size)
//{
//    uint8_t index = 0;
//    uint32_t ret = 0;
//    for (index = 0; index<4; index++)
//    {
//        sendData[4] += sendData[index];
//    }
//    ret = WritePort(sendData, sendData[2]);
//}

/*************************************************
  Function          :       uart_recv_resp
  Description       :   recive uart data and deal with it
  Input:
      uart_recv_data:  uart data
      recv_len:           data length
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void uart_recv_resp(uint8_t *uart_recv_data, uint16_t recv_len)
{
    uint8_t cmd = *(uart_recv_data + 3);            //
    UART_RECV_PACKET recvPacket = {0};
    recvPacket.data = uart_recv_data;
    recvPacket.size = recv_len;
//    os_uart_log("Uart recive cmd is 0x%02x", cmd);
    switch (cmd)
    {
        case DEV_CMD_CLEAR_GROUP_ADDR:
            break;
        case DEV_CMD_DEL_GROUP_ADDR:
            break;
        case DEV_CMD_GET_DEV_STATUS:
            break;
        case DEV_CMD_NET_RESET:
            break;
        case DEV_CMD_NET_SETTING_ENABLE:
            break;
        case DEV_CMD_QUERY_GROUP_ADDR:
            break;
        case DEV_CMD_READ_REG:
            break;
        case DEV_CMD_SAVE_DATA:
            break;
        case DEV_CMD_SEARCH_DEV:
            if (discover_dev_notify != NULL)
            {
                DISC_DEV_INFO_T subDevInfo;
                uint8_t dataLen = recv_len - UART_RSP_HEAD_SIZE - 1;
                memcpy(&subDevInfo, uart_recv_data+UART_RSP_HEAD_SIZE, dataLen);
                discover_dev_notify(&subDevInfo);
            }
            break;
        case DEV_CMD_START_SETMESH:
            break;
        case DEV_CMD_WRITE_REG:
            break;
        case DEV_CMD_PASSTHROUGH:
            {
                uint8_t deviceType = (*(uart_recv_data+UART_RSP_HEAD_SIZE)&0xf0)>>4;
                if (deviceType == TELECOM_RF433)
                {
                    if (rf433_passthrough_rsp_deal_cb)
                    {
                        rf433_passthrough_rsp_deal_cb(&recvPacket);
                    }
                }
                else if (deviceType == TELECOM_ZIGBEE)
                {
                    if (zigbee_passthrough_rsp_deal_cb)
                    {
                        zigbee_passthrough_rsp_deal_cb(&recvPacket);
                    }
                }
            }
            break;
        default:
            break;
    }
}


/*************************************************
  Function          :       uart_recv_distribute
  Description       :   recive uart data and deal with it
  Input:
      uart_recv_data:  uart data
      recv_len:           data length
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void uart_recv_distribute(uint8_t *uart_recv_data, uint16_t recv_len)
{
    uint8_t cmd = *(uart_recv_data + 3);            //
    UART_RECV_PACKET recvPacket = {0};
    recvPacket.data = uart_recv_data;
    recvPacket.size = recv_len;
//    os_uart_log("Uart recive cmd is 0x%02x", cmd);

    switch (cmd)
    {
        case DEV_CMD_GET_DEV_STATUS:
            break;
        case DEV_CMD_NET_RESET:
            break;
        case DEV_CMD_PASSTHROUGH:
            {
                PUART_SUBDEV_HEAD_T sub_head = (PUART_SUBDEV_HEAD_T)(uart_recv_data+UART_SEND_HEAD_SIZE);
                uint8_t deviceType = 0;
                deviceType = (sub_head->mid[2]&0xf0)>>4;
                if (deviceType == TELECOM_RF433)
                {

                    if (sub_head->u_op.op_struct.resp == 0)     // send frame
                    {
                        if (rf433_passthrough_deal_cb)
                        {
                            rf433_passthrough_deal_cb(&recvPacket);
                        }
                    }
                }
                else if (deviceType == TELECOM_ZIGBEE)
                {
                    if (sub_head->u_op.op_struct.resp == 0)
                    {
                        if (zigbee_passthrough_deal_cb)
                        {
                            zigbee_passthrough_deal_cb(&recvPacket);
                        }
                    }
                }
            }
            break;

        default:
            break;
    }
}


/*************************************************
  Function          :       uart_recv_deal
  Description       :   recive uart data and deal with it
  Input:
      uart_recv_data:  uart data
      recv_len:           data length
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
int uart_recv_deal( uint32_t argc )
{
    PUART_FRAME_DATA_T precvData = NULL;
    PUART_HEAD_T puart_head = NULL;
    os_uart_log("start recv deal !!!");
    while(1)
    {
        if (precvData != NULL)
        {
            free(precvData);
        }
        precvData = uart_pop_node(&uart_recv_queue);
        if (precvData == NULL)
        {
            msleep(10);
            continue;
        }

        mlink_led_submesh_communication();
        puart_head = (PUART_HEAD_T)(precvData->data);


        if ((puart_head->config & 0xc0) == (uint8_t)CONFIG_COM_ACK)
        {
            uart_recv_resp(precvData->data, precvData->dataSize);
        }
        else if ((puart_head->config & 0xc0) == (uint8_t)CONFIG_DIR_SEND)
        {
            uart_recv_distribute(precvData->data, precvData->dataSize);
        }
        else if (((puart_head->config & 0xc0) == (uint8_t)CONFIG_SEND_ACK) || ((puart_head->config & 0xc0) == (uint8_t)CONFIG_SEND_RSP_ACK))
        {
            uart_link_layer_comm = 1;
            g_sendDiffRev++;
        }
    }
}

/********************************************************
 * function:  CheckRecv
 * description:
 * input:       1. value
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
bool CheckIsWaitEcho( uint8_t* pBuffer )
{
    if (m_bRcvReady &&((pBuffer[1]&0xc0) == 0x40)&&(pBuffer[3] == m_pSndBuf[3]) && ((pBuffer[1]&0x0f) == (m_pSndBuf[1]&0x0f))) {
        if (pBuffer[3] == 0x31)
        {
            uint16_t opCmd = 0;
            memcpy(&opCmd, &pBuffer[11], 2);
            opCmd = opCmd & 0x0fff;
            if (memcmp(&opCmd, &m_pSndBuf[13], 2))
            {
                os_uart_log("cmd is not we want");
                return FALSE;
            }
        }

        if (m_pRcvBuf != NULL)
        {
            memcpy(m_pRcvBuf, pBuffer, pBuffer[2]);
        }
        m_bRcvReady = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/********************************************************
 * function:  CheckRecv
 * description:
 * input:       1. pBuffer
 *              2. bufflen
 * output:      1. frame
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus CheckRecv( uint8_t* pBuffer, uint8_t *bufflen, uint8_t *frame)
{
    uint8_t verifyValue = 0;
    uint8_t ret = FALSE;
    if ((*(pBuffer+2)==*bufflen) && (*pBuffer==(uint8_t)0xaa))
    {
        for (int count=0; count<*bufflen-1; count++)
        {
            verifyValue = verifyValue+*(pBuffer+count);
        }
        if (verifyValue == *(pBuffer+*bufflen-1))
        {
            ret = TRUE;
            memcpy(frame, pBuffer, *bufflen);
            memset(pBuffer, 0, *bufflen);
            *bufflen = 0;
        }
        else
        {
            os_uart_log("verify value is err!!!");
            PrintfByteHex("verify data", pBuffer, *bufflen);
            memset(pBuffer, 0, *bufflen);
            *bufflen = 0;
        }
    }
    else if (*(pBuffer+2)<=*bufflen)
    {
//        char data[512] = {0};
        int count=0;
        uint8_t len = *(pBuffer+2);

        if ((*pBuffer) == 0xaa && (len >= FRAME_MIN_SIZE))
        {
            for (count=0; count<*(pBuffer+2)-1; count++)
            {
                verifyValue = verifyValue+*(pBuffer+count);
            }
            if (verifyValue == *(pBuffer+len-1))
            {
                ret = TRUE;
                memcpy(frame, pBuffer, len);
//                memcpy(data, pBuffer+len, *bufflen-len);
//                memset(pBuffer, 0, *bufflen);
//                memcpy(pBuffer, data, *bufflen-len);
                memcpy(pBuffer, pBuffer+len, *bufflen-len);
                *bufflen -= len;
                ret = TRUE;
            }
            else
            {
                PrintfByteHex("verify data", pBuffer, len);
                os_uart_log("verify value is err!!! verifyValue %d != %d, len:%d", verifyValue, *(pBuffer+len-1), bufflen);
                ret = FALSE;
            }
        }
        else
        {
            ret = FALSE;
        }
        if (ret == FALSE)
        {
            for (count = 1; count<*bufflen; count++)
            {
                if (*(pBuffer+count) == (uint8_t)0xaa)
                {
                    break;
                }
            }
            if (count >= *bufflen)
            {
                memset(pBuffer, 0, *bufflen);
                *bufflen = 0;
            }
            else
            {
                memcpy(pBuffer, pBuffer+count, *bufflen-count);
//                memcpy(data, pBuffer+count, *bufflen-count);
//                memset(pBuffer, 0, *bufflen);
//                memcpy(pBuffer, data, *bufflen-count);
                *bufflen -= count;
            }
        }
    }
    else if (*(pBuffer+2)>*bufflen)
    {
        if (*pBuffer!=(uint8_t)0xaa)
        {
            os_uart_log("*pBuffer != 0xaa!!!");
            memset(pBuffer, 0, *bufflen);
            *bufflen = 0;
        }
        ret = FALSE;
    }

    return ret;
}

//void uartRecv_thread(system_context_t* sys_context)
//{
//    int recvlen;
//    uint8_t *inDataBuffer;
//
//    inDataBuffer = malloc(UartRecvBufferLen);
//    require(inDataBuffer, exit);
//
//    while(1)
//    {
//        recvlen = _uart_get_one_packet(inDataBuffer, UartRecvBufferLen);
//    //  uart_recv_log("recvlen = %d",recvlen);
//        if(recvlen <= 0)
//            continue;
//    //  uart_recv_log("---------len = %d",recvlen);
//     //   if(sum_check_recv(inDataBuffer,recvlen))
//      //   continue;
//        if(connectServerFlag == 1)
//           {
//                if(uart_data_filter(inDataBuffer,recvlen) ==1 )
//                    {
//                    uart_recv_log("uart_data_filter is ok!!");
//                    continue;
//                    }
//
//           }
//        device_cmd_process(inDataBuffer, recvlen);
//    }
//
//exit:
//    if(inDataBuffer) free(inDataBuffer);
//}

/* Packet format: BB 00 CMD(2B) Status(2B) datalen(2B) data(x) checksum(2B)
* copy to buf, return len = datalen+10
*/

#if ((SUCK_TOP_DEVICE) || (defined BLE_DEVICE))
size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen)
{
  int datalen;

  while(1) {
    if( MicoUartRecv( UART_MLINK_PIC, inBuf, inBufLen, UART_RECV_TIMEOUT) == kNoErr){
      return inBufLen;
    }
   else{
     datalen = MicoUartGetLengthInBuffer( UART_MLINK_PIC );
     if(datalen){
       MicoUartRecv(UART_MLINK_PIC, inBuf, datalen, UART_RECV_TIMEOUT);
       return datalen;
     }
//     mico_thread_msleep(100);       // mico_thread_msleep 100 ms
   }
  }
}
#else
size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen)
{
    OSStatus err = kNoErr;
    int datalen = 0;
    uint8_t *p = NULL;

    while(1)
    {
        p = inBuf;
        err = MicoUartRecv(UART_MLINK_PIC, p, 1, MICO_WAIT_FOREVER);
        require_noerr(err, exit);
        require(*p == 0xAA, exit);
        p++;

        err = MicoUartRecv(UART_MLINK_PIC, p, 1, 200);
        require_noerr(err, exit);
        p++;

        err = MicoUartRecv(UART_MLINK_PIC, p, 1, 300);
        require_noerr(err, exit);
        datalen = *p;
        p++;

        err = MicoUartRecv(UART_MLINK_PIC, p, datalen-3, 100);
        require_noerr(err, exit);
        p+= datalen ;

    //    uart_recv_log("--------datalen = %d",datalen + 5);
        return datalen;
    }

exit:
    os_uart_log("ERROR: %02x, datalen %d", *p, datalen);
    printf("error : recive data err\r\n");
    return -1;

}

#endif



/********************************************************
 * function:  uartRecv_thread
 * description:  uart recvice thread
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uartRecv_thread(uint32_t arg)
{
    int recvlen;
    UART_FRAME_DATA_T recvData = {0};
    uint8_t *recv_frame_buffer = NULL;
    recv_frame_buffer = malloc(TP_FRAME_MAX_SIZE*2);        // avoid overflow
    require(recv_frame_buffer, exit);

    static uint32_t dataLen = 0;

    while(1)
    {
        recvlen = _uart_get_one_packet(recv_frame_buffer, UART_ONE_PACKAGE_LENGTH);
        if (recvlen <= 0){
            continue;
        }

        PrintfByteHex("recive uart data ", recv_frame_buffer, recvlen);
        memcpy(g_recvTemp+g_recvTempLen, recv_frame_buffer, recvlen);
        g_recvTempLen += recvlen;
        dataLen += recvlen;
        os_uart_log("data len: %d, recv len: %d, dataLen: %d", g_recvTempLen, recvlen, dataLen);
        while(g_recvTemp[2]<=g_recvTempLen && (g_recvTempLen >= FRAME_MIN_SIZE))
        {
            if (TRUE != CheckRecv(g_recvTemp, (uint8_t *)&g_recvTempLen, recv_frame_buffer))
            {
                continue;
            }
            if (CheckIsWaitEcho(recv_frame_buffer))
            {
                continue;
            }
            recvData.dataSize = *(recv_frame_buffer+2);
            memcpy(recvData.data, recv_frame_buffer, recvData.dataSize);
            uart_push_node(&uart_recv_queue, &recvData);
        }
    }

    exit:
    if(recv_frame_buffer) free(recv_frame_buffer);
    mico_rtos_delete_thread(NULL);

}

/********************************************************
 * function:  uartSend_thread
 * description:  uart recvice thread
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uartSend_thread(uint32_t arg)
{
    PUART_FRAME_DATA_T psendData = NULL;
    int ret = 0;
    uint8_t flag = 0;

    while(1)
    {
        uint8_t count = 0;
        flag = 0;
#ifndef BLE_DEVICE
        msleep(50);
#endif
        if (psendData != NULL)
        {
            free(psendData);
        }
        psendData = uart_pop_node(&uart_send_queue);

        if(psendData == NULL)
        {
#ifndef BLE_DEVICE
            msleep(20);
#else
            msleep(5);
#endif
            continue;
        }
//        os_uart_log("uart_pop_node: uart_send_queue count:%d ",uart_send_queue.count);
//        os_uart_log("psendData->data %s\n",psendData->data);
        ret = MicoUartSend(UART_MLINK_PIC, psendData->data, psendData->dataSize);

#ifndef BLE_DEVICE
        if (ret == kNoErr)
        {
            g_sendCount++;
            uart_link_layer_comm = 0;
            if (m_pSndBuf != NULL) {
                memcpy(m_pSndBuf, psendData->data, psendData->dataSize);
            }
            PrintfByteHex("uart send", psendData->data, psendData->dataSize);
        }

        while (uart_link_layer_comm == 0)
        {
            if (++count == 20)      // 等待链路层的应答超时时长为200ms
            {
//                os_uart_log("waiting timeout 500ms");
                if (flag != 0)
                {
                    uart_link_layer_comm = 1;
                }
                else
                {
                    flag = !flag;
                    g_sendCount++;
                    MicoUartSend(UART_MLINK_PIC, psendData->data, psendData->dataSize);
                    count = 10;
                    printf("\r\nlose frame number is %d, send: %d, revc: %d\r\n", g_sendCount-g_sendDiffRev, g_sendCount, g_sendDiffRev);
                }
            }
//            os_uart_log("send uart waiting timeout [%d] ",count);
            msleep(10);
        }
#endif

    }
    mico_rtos_delete_thread(NULL);
}

/********************************************************
 * function:
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_config_zigbee_pth_deal( uart_recv_deal_callback zigbee_pth_deal_cb,  uart_recv_deal_callback zigbee_pth_rsp_deal_cb)
{
    os_uart_log("uart_config_deal_function");
    if (zigbee_pth_deal_cb != NULL)
    {
        zigbee_passthrough_deal_cb = zigbee_pth_deal_cb;
        zigbee_passthrough_rsp_deal_cb = zigbee_pth_rsp_deal_cb;
    }
    return TRUE;
}

/********************************************************
 * function:
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_config_rf433_pth_deal( uart_recv_deal_callback rf433_pth_deal_cb, uart_recv_deal_callback rf433_pth_rsp_deal_cb )
{
    os_uart_log("uart_config_deal_function");
    if (rf433_pth_deal_cb != NULL)
    {
        rf433_passthrough_deal_cb = rf433_pth_deal_cb;
        rf433_passthrough_rsp_deal_cb = rf433_pth_rsp_deal_cb;
    }
    return TRUE;
}


/********************************************************
 * function:  uart_discover_notify_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_discover_notify_init( discover_notify_callback_t disc_notify )
{
    OSStatus err = kNoErr;
    if (disc_notify != NULL)
    {
        discover_dev_notify = disc_notify;
    }

    return err;
}

#if defined BLE_DEVICE

#define FRAME_MAX_SIZE TP_FRAME_MAX_SIZE
/********************************************************
 * function:   uart_config_ble_recv_cb
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_config_ble_recv_cb( uart_recv_deal_callback ble_callback )
{
    ble_recv_deal_cb = ble_callback;
}

/********************************************************
 * function:  bleRecv_thread
 * description:  ble recvice thread
 * input:
 * output:
 * return:
 * auther:   chengc
 * other:
*********************************************************/
void bleRecv_thread(uint32_t arg)
{
    int recvlen;
    char *recv_frame_buffer = NULL;
    UART_RECV_PACKET recvPacket;
    recv_frame_buffer = malloc(FRAME_MAX_SIZE*2);        // avoid overflow
    require(recv_frame_buffer, exit);
    while(1)
    {
        recvlen = _uart_get_one_packet(recv_frame_buffer, UART_ONE_PACKAGE_LENGTH);
        if (recvlen <= 0)
          continue;

        if (ble_recv_deal_cb)
        {
            recvPacket.size = recvlen;
            recvPacket.data = recv_frame_buffer;

            ble_recv_deal_cb(&recvPacket);
        }
//        ble_uart_recv_callback(recv_frame_buffer,recvlen);
        // uart_push_node(&uart_recv_queue, &recvData);

    }

    exit:
    if(recv_frame_buffer) free(recv_frame_buffer);
    mico_rtos_delete_thread(NULL);
}

#endif


/********************************************************
 * function:  os_uart_logic_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_logic_init( void )
{
    OSStatus err = kNoErr;

    /*UART receive thread*/
    mico_uart_config_t uart_config;

    uart_config.baud_rate = BAUD_RATE;
    uart_config.data_width = DATA_WIDTH_8BIT;
    uart_config.parity = NO_PARITY;
    uart_config.stop_bits = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;
    uart_config.flags = UART_WAKEUP_DISABLE;

    uart_queue_init(&uart_send_queue);
    uart_queue_init(&uart_recv_queue);
    mico_rtos_init_mutex(&g_waitRecvMutex);
    ring_buffer_init( (ring_buffer_t *) &rx_buffer, (uint8_t *) rx_data, UART_DRIVER_BUFFER_LENGTH );
    MicoUartInitialize( UART_MLINK_PIC, &uart_config, (ring_buffer_t *) &rx_buffer );

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "UART Recv", bleRecv_thread,
                                   0x2000, 0 );
    require_noerr_action( err, exit, os_uart_log("ERROR: Unable to start the uart recv thread.") );
    exit:

    return err;
}


