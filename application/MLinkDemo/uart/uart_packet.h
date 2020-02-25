/*
 * uart_packet.h
 *
 *  Created on: 2018年6月6日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_UART_UART_PACKET_H_
#define APPLICATION_MLINKDEMO_UART_UART_PACKET_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "mico.h"

typedef struct{
    uint8_t stateLen;
    uint8_t subnet;
    uint8_t addr[3];
    uint8_t dpIndex[2];
    uint8_t data[17];
}SUBNET_STATE_NOTIFY_INFO, *PSUBNET_STATE_NOTIFY_INFO;

typedef struct{
    uint16_t sceneIndex;
    uint16_t sceneDataLen;
    uint8_t sceneData[8];
}SCENE_ADD_PARAM,*PSCENE_ADD_PARAM;

typedef struct{
    uint16_t keyIndex;
    uint16_t keyType;
    uint16_t stateSize;
    uint8_t keyState[KEY_STATE_SIZE];
}KEY_STATE_INFO, *PKEY_STATE_INFO;

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
OSStatus uart_read_attr(uint8_t *addr, uint16_t dpAddr);

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
OSStatus uart_read_attr_ex(uint8_t *addr, uint16_t dpAddr, uint8_t *value);

/********************************************************
 * function:  uart_query_attrs
 * description: query all attribute about device
 * input:       1. addr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_query_attrs( uint8_t *addr );

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
OSStatus uart_write_attr(uint8_t *addr, uint16_t dpAddr,  uint8_t *data, uint8_t len, uint32_t wait_time_ms);

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
OSStatus uart_ctrl_scene(uint16_t sceneIndex);

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
OSStatus uart_add_scene(uint8_t *addr, uint16_t keyIndex, uint8_t sceneNum, PSCENE_ADD_PARAM sceneParam);

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
OSStatus uart_get_scene(uint8_t *addr, uint16_t key_index, uint16_t scene_start, uint8_t *data);

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
OSStatus uart_del_scene(uint8_t *addr, uint16_t keyIndex, uint8_t sceneNum, uint16_t *sceneIndex);

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
OSStatus uart_clear_scene(uint8_t *addr, uint16_t keyIndex);

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
OSStatus uart_add_group(uint8_t *addr, uint16_t keyIndex, uint8_t groupNum, uint16_t *groupIndex);

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
OSStatus uart_del_group(uint8_t *addr, uint16_t keyIndex, uint8_t groupNum, uint16_t *groupIndex);

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
OSStatus uart_clear_group(uint8_t *addr, uint16_t keyIndex);

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
OSStatus uart_get_group(uint8_t *addr, uint16_t key_index, uint16_t group_start, char *data);

/********************************************************
 * function:  uart_write_group
 * description:
 * input:       1. groupIndex:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_write_group(uint16_t keyType, uint8_t *data, uint8_t dataLen);

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
OSStatus uart_read_group(uint16_t keyType);

/********************************************************
 * function:  uart_save_regedit
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_save_regedit( uint16_t devAddr );

/********************************************************
 * function:  uart_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_setmesh(uint16_t devAddr, SETMESH_DEVICE_TYPE_E devtype, uint16_t timeout);

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
int uart_setmesh_allow(uint16_t devAddr, uint16_t timeout);

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
void uart_resetmesh( uint16_t devAddr );

/********************************************************
 * function:  uart_restore_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_restore_device(uint16_t addr);

/********************************************************
 * function:  uart_reboot_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_reboot_device(uint16_t addr);

/********************************************************
 * function:  uart_clear_data
 * description:
 * input:     1. addr
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_clear_data(uint16_t addr);

/********************************************************
 * function:  uart_set_netaddr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus uart_set_netaddr( uint16_t daddr, uint8_t *mac, uint16_t *addr);

/********************************************************
 * function:  uart_discover_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_discover_device( uint16_t addr, uint8_t timeout );

/********************************************************
 * function:  uart_ctrl_dev
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void uart_ctrl_dev( uint8_t *daddr, uint8_t cmd, uint8_t *ctrlData, uint8_t ctrlLen );

/********************************************************
 * function:  uart_get_device_info
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_get_device_info( uint16_t daddr, uint8_t *echoData, uint8_t *echoLen);

/********************************************************
 * function:  uart_read_register
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_read_register( uint16_t daddr, uint16_t reg_addr, uint8_t num, uint8_t *echoData, uint8_t *echoLen);

/********************************************************
 * function:  uart_read_register
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int uart_observe_state_notify(uint8_t *addr, uint8_t notify_num, PSUBNET_STATE_NOTIFY_INFO state_info);

#ifdef __cplusplus
}
#endif


#endif /* APPLICATION_MLINKDEMO_UART_UART_PACKET_H_ */
