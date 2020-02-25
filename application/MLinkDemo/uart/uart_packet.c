/*
 * uart_packet.c
 *
 *  Created on: 2017年10月27日
 *      Author: Administrator
 */

#include "MLinkAppDef.h"
#include "uart_logic.h"
#include "../MLinkObject.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../zigbee/zigbee.h"
#include "uart_packet.h"

#define os_uart_packet_log(M, ...) custom_log("UART LOG", M, ##__VA_ARGS__)

#define SCENE_NUM   32
#define GROUP_NUM   32
//#define OLD_SETMESH_MODE    组网时间长度为1字节

/********************************************************
 * function:  uart_read_attr
 * description:
 * input:       1. addr
 *              2. dpAddr
 * output:      1. value
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_read_attr(uint8_t *addr, uint16_t dpAddr)
{
    int ret = 0;
    int size = sizeof(uint16_t);
    uint16_t buffer = dpAddr;

    if (addr == NULL)
    {
        return kGeneralErr;
    }
    ret = UartMlSend(addr, ML_DP_READ, (uint16_t *)&buffer, size);

    return ret;
}

/********************************************************
 * function:  uart_read_attr_ex
 * description:
 * input:       1. addr
 *              2. dpAddr
 * output:      1. value
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_read_attr_ex(uint8_t *addr, uint16_t dpAddr, uint8_t *value)
{
    int ret = 0;
    int size = sizeof(uint16_t);
    uint8_t recvLen = 0;
    uint8_t recvbuf[128] = {0};
    uint16_t buffer = dpAddr;
    uint8_t dataLen =0;
    if ((addr == NULL) || (value == NULL))
    {
        return kGeneralErr;
    }
    ret = UartMlSendEx(addr, ML_DP_READ, (uint16_t *)&buffer, size, recvbuf, 1000);

    if ( ret >= 0 )
    {
        char state = 0;

        state = *(recvbuf+UART_RSP_PTH_HEAD_SIZE-1);        // response state
        recvLen = recvbuf[2];
        if (state >= 0)         // generate value string
        {
            dataLen = recvLen - UART_RSP_PTH_HEAD_SIZE - 1;
            os_uart_packet_log("read attr rsp len is %d, dataLen: %d", recvLen, dataLen);
            memcpy(value, recvbuf+UART_RSP_PTH_HEAD_SIZE, dataLen);
        }

        ret = dataLen;
    }
    else
    {
        ret = kGeneralErr;
    }
    return ret;
}

/********************************************************
 * function:  uart_query_attrs
 * description: query all attribute about device
 * input:       1. addr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_query_attrs( uint8_t *addr )
{
    int ret = 0;
    uint8_t buffer[10] = {0};
    UART_PTH_OP u_op;
    memset(&u_op, 0, sizeof(UART_PTH_OP));
    u_op.op_struct.op = ML_DP_QUERY_ALL;
    if (addr == NULL)
    {
        return kGeneralErr;
    }
    ret = UartMlSend(addr, u_op.op, buffer, 0);

    return ret;
}


/********************************************************
 * function:  uart_write_attr
 * description:
 * input:       1. addr:        the device's address string
 *              2. datapointAddr:  the device's data point address
 *              3. data:        write attribute data
 *              4. len:         the length of data
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_write_attr(uint8_t *addr, uint16_t dpAddr,  uint8_t *data, uint8_t len, uint32_t wait_time_ms)
{
    OSStatus ret = kGeneralErr;
    uint8_t buffer[200]={0};
    if (buffer != NULL)
    {
        memcpy(buffer, &dpAddr, 2);
        memcpy(buffer+2, data, len);
        if (wait_time_ms)
        {
            uint8_t recvBuf[32] = {0};
            ret = UartMlSendEx(addr, ML_DP_WRITE, buffer, len+2, recvBuf, wait_time_ms);
        }
        else
        {
            ret = UartMlSend(addr, ML_DP_WRITE, buffer, len+2);
        }
//        free(buffer);
        if (ret != -1)
        {
            ret = kNoErr;
        }
    }
    return ret;
}

/********************************************************
 * function:  uart_ctrl_scene
 * description:
 * input:       1. addr:        the device's address string
 *              2. datapointAddr:  the device's data point address
 *              3. data:        write attribute data
 *              4. len:         the length of data
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_ctrl_scene(uint16_t sceneIndex)
{
    OSStatus ret = 0;
    uint8_t addr[3] = {0xff, 0xff, 0xff};
    uint16_t dataPointAddr = sceneIndex;
    ret = UartMlSend(addr, ML_CALL_SCENE, &dataPointAddr, 2);
    return ret;
}

/********************************************************
 * function:  uart_add_scene
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:  key
 *              3. sceneNum:    scene number
 *              4. sceneParam:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_add_scene(uint8_t *addr, uint16_t keyIndex, uint8_t sceneNum, PSCENE_ADD_PARAM sceneParam)
{
    OSStatus ret = 0;
    uint8_t buffer[200] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t count = 0;
    uint8_t pointOffset = 0;
    PSCENE_ADD_PARAM pscnParamTmp = NULL;
    uint8_t size = 0;
    uint8_t recvBuff[32] = {0};

    memcpy(buffer, &dpIndex, 2);
    buffer[2] = sceneNum;
    pointOffset = 3;
    for (count = 0; count < sceneNum; count++)
    {
        pscnParamTmp = sceneParam + count;
        memcpy(buffer+pointOffset, &pscnParamTmp->sceneIndex, 2);
        memcpy(buffer+pointOffset+2, pscnParamTmp->sceneData, pscnParamTmp->sceneDataLen);
        pointOffset = pointOffset + pscnParamTmp->sceneDataLen + 2;
    }
    size = pointOffset;
    // 需等待应答
    ret = UartMlSendEx(addr, ML_ADD_SCENE, buffer, size, recvBuff, 1000);
    if (ret == -1)
    {
        ret = kGeneralErr;
    }
    else
    {
        ret = kNoErr;
    }
    return ret;
}

/********************************************************
 * function:  uart_get_scene
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:  key
 *              3. sceneNum:    scene number
 *              4. sceneParam:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_get_scene(uint8_t *addr, uint16_t key_index, uint16_t scene_start, uint8_t *data)
{
    OSStatus ret = 0;
    uint8_t buffer[20] = {0};
    uint16_t dpIndex = key_index;
    uint16_t sceneStart = scene_start;
    uint8_t size = 0;
    uint8_t recvbuf[200] = {0};
    uint8_t recvLen = 0;
    memcpy(buffer, &dpIndex, 2);
    memcpy(buffer+2, &sceneStart, 2);

    size = 4;
    // 需等待应答
    ret = UartMlSendEx(addr, ML_GET_SCENE, buffer, size, recvbuf, 1000);
    if (ret == -1)
    {
        ret = kGeneralErr;
    }
    else
    {
        char state = 0;
        uint8_t dataLen =0;

        state = *(recvbuf+UART_RSP_PTH_HEAD_SIZE-1);        // response state
        recvLen = recvbuf[2];
        if (state >= 0)         // generate value string
        {
            dataLen = recvLen - UART_RSP_PTH_HEAD_SIZE - 1;
            memcpy(data, recvbuf+UART_RSP_PTH_HEAD_SIZE, dataLen);
        }
        ret = dataLen;
    }
    return ret;
}

/********************************************************
 * function:  uart_del_scene
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 *              3. sceneNum:
 *              4. sceneIndex: index array
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_del_scene(uint8_t *addr, uint16_t keyIndex, uint8_t sceneNum, uint16_t *sceneIndex)
{
    OSStatus ret = 0;
    uint8_t buffer[SCENE_NUM*2+4] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t count = 0;
    uint8_t pointOffset = 0;
    uint8_t size = 0;

    memcpy(buffer, &dpIndex, 2);
    buffer[2] = sceneNum;
    pointOffset = 3;
    for (count = 0; count < sceneNum; count++)
    {
        memcpy(buffer+pointOffset, sceneIndex+count, 2);
        pointOffset += 2;
    }
    size = pointOffset;

    ret = UartMlSend(addr, ML_DEL_SCENE, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_clear_scene
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_clear_scene(uint8_t *addr, uint16_t keyIndex)
{
    OSStatus ret = 0;
    uint8_t buffer[4] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t size = 0;

    memcpy(buffer, &dpIndex, 2);
    size = 2;

    ret = UartMlSend(addr, ML_CLR_SCENE, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_add_group
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 *              3. groupNum:
 *              4. groupIndex
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_add_group(uint8_t *addr, uint16_t keyIndex, uint8_t groupNum, uint16_t *groupIndex)
{
    OSStatus ret = 0;
    uint8_t buffer[GROUP_NUM*2+4] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t count = 0;
    uint8_t pointOffset = 0;
    uint8_t size = 0;

    memcpy(buffer, &dpIndex, 2);
    buffer[2] = groupNum;
    pointOffset = 3;
    for (count = 0; count < groupNum; count++)
    {
        memcpy(buffer+pointOffset, groupIndex+count, 2);
        pointOffset += 2;
    }
    size = pointOffset;

    ret = UartMlSend(addr, ML_ADD_GROUP, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_del_group
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 *              3. groupNum:
 *              4. groupIndex
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_del_group(uint8_t *addr, uint16_t keyIndex, uint8_t groupNum, uint16_t *groupIndex)
{
    OSStatus ret = 0;
    uint8_t buffer[GROUP_NUM*2+4] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t count = 0;
    uint8_t pointOffset = 0;
    uint8_t size = 0;

    memcpy(buffer, &dpIndex, 2);
    buffer[2] = groupNum;
    pointOffset = 3;
    for (count = 0; count < groupNum; count++)
    {
        memcpy(buffer+pointOffset, groupIndex+count, 2);
        pointOffset += 2;
    }
    size = pointOffset;

    ret = UartMlSend(addr, ML_DEL_GROUP, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_clear_group
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_clear_group(uint8_t *addr, uint16_t keyIndex)
{
    OSStatus ret = 0;
    uint8_t buffer[4] = {0};
    uint32_t dpIndex = keyIndex;
    uint8_t size = 0;

    memcpy(buffer, &dpIndex, 2);
    size = 2;

    ret = UartMlSend(addr, ML_CLR_GROUP, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_clear_group
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_get_group(uint8_t *addr, uint16_t key_index, uint16_t group_start, char *data)
{
    OSStatus ret = 0;
    uint8_t buffer[20] = {0};
    uint16_t dpIndex = key_index;
    uint16_t groupStart = group_start;
    uint8_t size = 0;
    uint8_t recvbuf[200] = {0};
    uint8_t recvLen = 0;

    memcpy(buffer, &dpIndex, 2);
    memcpy(buffer+2, &groupStart, 2);

    size = 4;
    // 需等待应答
    ret = UartMlSendEx(addr, ML_GET_GROUP, buffer, size, recvbuf, 1000);
    if (ret == -1)
    {
        ret = kGeneralErr;
    }
    else
    {
        char state = 0;
        uint8_t dataLen =0;

        state = *(recvbuf+UART_RSP_PTH_HEAD_SIZE-1);        // response state
        recvLen = recvbuf[2];
        if (state >= 0)         // generate value string
        {
            dataLen = recvLen - UART_RSP_PTH_HEAD_SIZE - 1;
            memcpy(data, recvbuf+UART_RSP_PTH_HEAD_SIZE, dataLen);
        }

        ret = dataLen;
    }
    return ret;
}

/********************************************************
 * function:  uart_write_group
 * description:
 * input:       1. groupIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_write_group(uint16_t keyType, uint8_t *data, uint8_t dataLen)
{
    OSStatus ret = 0;
    uint8_t addr[3] = {0xff, 0xff, 0xff};
    uint8_t buffer[32] = {0};
    uint16_t dpType = keyType;
    uint8_t size = dataLen + 2;
    memcpy(buffer, &dpType, 2);
    memcpy(buffer+2, data, dataLen);
    ret = UartMlSend(addr, ML_GROUP_WRITE, buffer, size);
    return ret;
}

/********************************************************
 * function:  uart_read_group
 * description:
 * input:       1. addr:        the device's address string
 *              2. keyIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_read_group(uint16_t keyType)
{
    OSStatus ret = 0;
    uint8_t addr[3] = {0xff, 0xff, 0xff};
    uint16_t dpType = keyType;
    ret = UartMlSend(addr, ML_GROUP_READ, &dpType, 2);
    return ret;
}

/********************************************************
 * function:  uart_save_regedit
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_save_regedit( uint16_t devAddr )
{
    uint8_t cmd = DEV_CMD_SAVE_DATA;
    uint8_t buffer[3] = {0};
    UartSend(devAddr, cmd, buffer, 0);
}

/********************************************************
 * function:  uart_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_setmesh(uint16_t devAddr, SETMESH_DEVICE_TYPE_E devtype, uint16_t timeout)
{
    uint8_t cmd = DEV_CMD_START_SETMESH;
    uint8_t buffer[3] = {0};
    uint16_t addr = 0x8000;
    int ret = 0;
    addr = devAddr;
    buffer[0] = devtype;
#if defined OLD_SETMESH_MODE
    if (timeout)
    {
        *(buffer+1) = 120;
    }
    if (devAddr == 0xffff || devAddr==0xfffe)
    {
        ret = UartSend(0x8000, cmd, buffer, 2);
    }
    else
    {
        uint8_t recvBuff[30] = {0};
        ret = UartSendAndRecv(0x8000, cmd, buffer, 2, recvBuff);
    }
#else
    *(uint16_t *)(buffer+1) = timeout;
    if (devAddr == 0xffff || devAddr==0xfffe)
    {
        ret = UartSend(addr, cmd, buffer, 3);
    }
    else
    {
        uint8_t recvBuff[30] = {0};
        ret = UartSendAndRecv(addr, cmd, buffer, 3, recvBuff);
    }
#endif

    return ret;
}

/********************************************************
 * function:  uart_setmesh_allow
 * description:
 * input:       1. devAddr
 *              2. time
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_setmesh_allow(uint16_t devAddr, uint16_t timeout)
{
    uint8_t cmd = DEV_CMD_NET_SETTING_ENABLE;
    uint8_t buffer[3] = {0};
    uint16_t addr = 0x8000;
    uint16_t allowTime = timeout;

    addr = devAddr;
#if defined OLD_SETMESH_MODE
    if (timeout)
    {
        *buffer = 120;
    }
    return UartSend(0x8000, cmd, buffer, 1);
#else
    memcpy(buffer, &allowTime, 2);
    return UartSend(addr, cmd, buffer, 2);
#endif
}

/********************************************************
 * function:  uart_resetmesh
 * description:
 * input:       1. devAddr
 *              2. time
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_resetmesh( uint16_t devAddr )
{
    uint8_t buffer[2] = {0};
    UartSend(devAddr, DEV_CMD_NET_RESET, buffer, 0);
}

/********************************************************
 * function:  uart_restore_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_restore_device(uint16_t addr)
{
    uint8_t cmd = DEV_CMD_FACTORY;
    UartSend(addr, cmd, NULL, 0);
}

/********************************************************
 * function:  uart_reboot_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_reboot_device(uint16_t addr)
{
    uint8_t cmd = DEV_CMD_REBOOT;
    UartSend(addr, cmd, NULL, 0);
}

/********************************************************
 * function:  uart_clear_data
 * description:
 * input:     1. addr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_clear_data(uint16_t addr)
{
    uint8_t cmd = DEV_CMD_CLEAR_DATA;
    UartSend(addr, cmd, NULL, 0);
}

/********************************************************
 * function:  uart_set_netaddr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_set_netaddr( uint16_t daddr, uint8_t *mac, uint16_t *addr)
{
    uint8_t cmd = DEV_CMD_SET_SHORTADDR;
    uint8_t buffer[16] = {0};
    int ret = 0;
    memcpy(buffer, addr, 2);
    memcpy(buffer+2, mac, 8);
    ret = UartSend(daddr, cmd, buffer, 10);
    uart_reboot_device(daddr);
    return ret;
}

/********************************************************
 * function:  uart_discover_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_discover_device( uint16_t addr, uint8_t timeout )
{
    uint8_t cmd = DEV_CMD_SEARCH_DEV;
    uint8_t buffer = timeout;
//    uint16_t addr = 0xFFFE;
    UartSend(addr, cmd, &buffer, sizeof(buffer));
}

/********************************************************
 * function:  uart_send_echo
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_send_echo( PUART_RECV_PACKET recvPacket, uint8_t *data, uint8_t dataSize )
{
    uint8_t echoData[128] = { 0 };
    PUART_RSP_HEAD_T pRspHead = NULL;
    PUART_HEAD_T pHead = (PUART_HEAD_T)recvPacket->data;
    pRspHead = (PUART_RSP_HEAD_T)echoData;
    pRspHead->frame_head = 0xAA;
    pRspHead->config = (pHead->config & 0x0F) + CONFIG_COM_ACK;
    pRspHead->function_code = pHead->function_code;

    if (pHead->function_code == DEV_CMD_PASSTHROUGH)
    {
        return;
    }
    else
    {
        pRspHead->frame_len = dataSize+UART_RSP_HEAD_SIZE+1;
        memcpy(echoData+UART_RSP_HEAD_SIZE, data, dataSize);
    }
    *(echoData+echoData[2]-1) = GetCrc8(echoData, echoData[2]);
    WritePort(echoData, echoData[2]);
}

/********************************************************
 * function:  uart_send_psth_echo
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_send_psth_echo( PUART_RECV_PACKET recvPacket, uint8_t stat, uint8_t *data, uint8_t dataSize )
{
    uint8_t echoData[128] = { 0 };
    PUART_HEAD_T pHead = (PUART_HEAD_T)recvPacket->data;
    PUART_SUBDEV_HEAD_T pSubHead = (PUART_SUBDEV_HEAD_T)(recvPacket->data + UART_SEND_MAIN_HEAD_SIZE);
    PUART_SEND_RSP_HEAD_T pRspHead = (PUART_SEND_RSP_HEAD_T)echoData;
    PUART_SUBDEV_RSP_HEAD_T pSubRspHead = (PUART_SUBDEV_RSP_HEAD_T)(echoData + UART_SEND_RSP_HEAD_SIZE);

    pRspHead->frame_head = 0xAA;
    pRspHead->config = (pHead->config & 0x0F) + CONFIG_COM_ACK;
    pRspHead->frame_len = dataSize+UART_SEND_RSP_PTH_HEAD_SIZE+1;
    pRspHead->function_code = pHead->function_code;
    pRspHead->destAddr = pHead->srcAddr;

    memcpy(pSubRspHead->mid, pSubHead->mid, 3);
    pSubRspHead->daddr.shortAddr = pHead->srcAddr;
    pSubRspHead->spand = pSubHead->spand;
    pSubRspHead->u_op = pSubHead->u_op;
    pSubRspHead->u_op.op_struct.batch = 0;
    pSubRspHead->u_op.op_struct.dir = 0;
    pSubRspHead->u_op.op_struct.resp = 1;
    pSubRspHead->state = stat;
    memcpy(echoData+UART_SEND_RSP_PTH_HEAD_SIZE, data, dataSize);

    *(echoData+echoData[2]-1) = GetCrc8(echoData, echoData[2]-1);
    WritePort(echoData, echoData[2]);
}



/********************************************************
 * function:  uart_ctrl_dev
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_ctrl_dev( uint8_t *daddr, uint8_t cmd, uint8_t *ctrlData, uint8_t ctrlLen )
{
    uint16_t op = cmd;
    UartMlSend(daddr, op, ctrlData, ctrlLen);
}

/********************************************************
 * function:  uart_get_device_info
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_get_device_info( uint16_t daddr, uint8_t *echoData, uint8_t *echoLen)
{
    uint16_t dest = daddr;
    uint8_t buffer[2] = {0};
    uint8_t recvBuff[64] = {0};
    int ret = 0;

    ret = UartSendAndRecv(dest, DEV_CMD_GET_DEV_STATUS, buffer, 0, recvBuff);
    if (ret != -1)
    {
        *echoLen = recvBuff[2]-UART_RSP_HEAD_SIZE-1;
        memcpy(echoData, recvBuff+UART_RSP_HEAD_SIZE, *echoLen);
    }
    return ret;

}

/********************************************************
 * function:  uart_read_register
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_read_register( uint16_t daddr, uint16_t reg_addr, uint8_t num, uint8_t *echoData, uint8_t *echoLen)
{
    uint16_t dest = daddr;
    uint8_t buffer[4] = {0};
    uint8_t recvBuff[64] = {0};
    uint16_t regAddr = reg_addr;
    int ret = 0;

    memcpy(buffer, (uint8_t *)&regAddr, 2);
    buffer[2] = num;
    ret = UartSendAndRecv(dest, DEV_CMD_READ_REG, buffer, 3, recvBuff);
    if (ret != -1)
    {
        *echoLen = recvBuff[2]-UART_RSP_HEAD_SIZE-1;
        memcpy(echoData, recvBuff+UART_RSP_HEAD_SIZE, *echoLen);
    }
    return ret;
}

/********************************************************
 * function:  uart_write_register
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_write_register( uint16_t daddr, uint16_t reg_addr, uint8_t num, uint8_t *regdata)
{
    uint16_t dest = daddr;
    uint8_t buffer[64] = {0};
    uint8_t recvBuff[64] = {0};
    uint16_t regAddr = reg_addr;
    int ret = 0;

    memcpy(buffer, (uint8_t *)&regAddr, 2);
    buffer[2] = num;
    memcpy(buffer+3, regdata, 2*num);
    ret = UartSendAndRecv(dest, DEV_CMD_WRITE_REG, buffer, 3+2*num, recvBuff);
    return ret;
}

/********************************************************
 * function:  uart_observe_state_notify
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_observe_state_notify(uint8_t *addr, uint8_t notify_num, PSUBNET_STATE_NOTIFY_INFO state_info)
{
    OSStatus ret = kGeneralErr;
    uint8_t buffer[128]={0};
    uint8_t bufferLen = 0;
    uint8_t count = 0;
    uint8_t stateInfoLen = 0;
    PSUBNET_STATE_NOTIFY_INFO pstateInfoTemp = NULL;

    // 个数1B+[状态字节数1B+子网地址1B+设备地址3B+数据点索引2B +数据点属性数据（NB）]*N
    buffer[0] = notify_num;
    bufferLen = 1;
    for (count=0; count<notify_num; count++)
    {
        pstateInfoTemp = state_info + count;
        stateInfoLen = pstateInfoTemp->stateLen+7;
        memcpy(buffer+bufferLen, pstateInfoTemp, stateInfoLen);
        bufferLen += stateInfoLen;
    }

    ret = UartMlSend(addr, ML_DP_STATE_NOTIFY, buffer, bufferLen);
    return ret;
}

/********************************************************
 * function:  uart_get_net_param
 * description:
 * input:
 * output:   1. net_param
 *           2. net_param_len
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_get_net_param( uint8_t *net_param, uint8_t *net_param_len )
{
    uint8_t buffer[4] = {0};
    uint8_t recvBuff[64] = {0};
    int ret = -1;
    if ((net_param == NULL) || (net_param_len == NULL))
    {
        return ret;
    }
    //ret = UartSendAndRecv(0x8000, DEV_CMD_GET_NET_PARAM, buffer, 0, recvBuff);
    if (ret != -1)
    {
        *net_param_len = recvBuff[2]-UART_RSP_HEAD_SIZE-1;
        memcpy(net_param, recvBuff+UART_RSP_HEAD_SIZE, *net_param_len);
    }
    return ret;
}

/********************************************************
 * function:  uart_set_net_param
 * description:
 * input:    1. net_param
 *           2. net_param_len
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_set_net_param( uint8_t *net_param, uint8_t net_param_len )
{
    uint8_t buffer[64] = {0};
    uint8_t recvBuff[64] = {0};
    uint8_t len = 0;
    int ret = -1;
    if (net_param == NULL)
    {
        return ret;
    }
    len = (net_param_len>64)?64:net_param_len;
    memcpy(buffer, net_param, len);
    //ret = UartSendAndRecv(0x8000, DEV_CMD_SET_NET_PARAM, buffer, len, recvBuff);
    if (ret != -1)
    {
        uint8_t state = 0;
        len = recvBuff[2]-UART_RSP_HEAD_SIZE-1;
        memcpy(&state, recvBuff+UART_RSP_HEAD_SIZE, 1);
        if (state)
        {
            ret = -1;
        }
    }
    return ret;
}

