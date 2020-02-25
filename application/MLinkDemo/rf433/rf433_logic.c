#include "../coap/ml_coap.h"
#include "mico.h"
#include "../MLinkCommand.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../uart/uart_logic.h"
#include "MLinkAppDef.h"
#include "rf433.h"
#include "../uart/uart_packet.h"

#define os_rf433_log(M, ...) custom_log("RF433", M, ##__VA_ARGS__)

rf433_logic_notify_t rf433_funciton_distribute = NULL;
static mico_timer_t _rf433_mesh_timer;
static bool _rf433_mesh_timer_initialized = false;

/********************************************************
 * function:  rf433_logic_read_attr
 * description:
 * input:       pobj_attr
 * output:      pdev_attr_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_logic_read_attr( char *addr, POBJATTR_T pobj_attr )
{
    os_rf433_log("reading object attribute\r\n");
    if (pobj_attr == NULL)
    {
        return TELECOM_RSP_ERR;
    }
    return TELECOM_RSP_OK;
}



/********************************************************
 * function:  rf433_logic_write_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_logic_write_attr(int class_id, unsigned char *addr, PDEV_FUNCTION_OBJ_EX_T dev_attr_obj)
{
    os_rf433_log("writing object attribute\r\n");

}

/********************************************************
 * function:  zigbee_readmult_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_logic_readmult_attr()
{
    os_rf433_log("reading objects attribute\r\n");

}

/********************************************************
 * function:  zigbee_writemult_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_logic_writemult_attr()
{
    os_rf433_log("writing objects attribute\r\n");
}

/********************************************************
 * function:  rf433_logic_writegroup_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_logic_writegroup_attr()
{
    OSStatus err = kNoErr;
    os_rf433_log("writing group objects attribute\r\n");

    return err;
}

/********************************************************
 * function:  rf433_key_attr_logic_transformation
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus rf433_key_attr_logic_transformation( uint8_t state_num, PKEY_STATE_INFO pkey_state, PKEY_ATTR_T_EX pdev_attr)
{
    uint8_t count = 0;
    PKEY_STATE_INFO pKeyStateTemp = NULL;
    PKEY_ATTR_T_EX pDevAttrTemp = NULL;
    if ((pkey_state == NULL) || (pdev_attr == NULL))
    {
        return kGeneralErr;
    }
    for (count=0; count<state_num; count++)
    {
        pKeyStateTemp = pkey_state + count;
        pDevAttrTemp = pdev_attr + count;
        pDevAttrTemp->keyState.type = PROP_DATA;
        sprintf(pDevAttrTemp->keyState.key, "%d", pKeyStateTemp->keyIndex);
        sprintf(pDevAttrTemp->keyType, "%d", pKeyStateTemp->keyType);
        mlink_reverse_translate_value(pKeyStateTemp->keyState, pKeyStateTemp->stateSize, &pDevAttrTemp->keyState.valtype, pDevAttrTemp->keyState.value);
    }
    return kNoErr;
}

void rf433_setmesh_stop_timer()
{
    mico_stop_timer(&_rf433_mesh_timer);
    mico_deinit_timer( &_rf433_mesh_timer );
    _rf433_mesh_timer_initialized = false;
}


void rf433_setmesh_config_exit( void )
{
    mlink_sys_set_status(SYS_MESH_STATE, MESH_OUT);
    if (_rf433_mesh_timer_initialized == TRUE)
    {
        rf433_setmesh_stop_timer();
    }
}

/********************************************************
 * function:  zigbee_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int rf433_out_of_setmesh( uint16_t devAddr )
{
    rf433_setmesh_config_exit();
    return kNoErr;
}

/********************************************************
 * function:  rf433_mesh_timeout_handle
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void rf433_mesh_timeout_handle( void )
{
    rf433_out_of_setmesh(0xffff);
}


/********************************************************
 * function:  rf433_start_mesh_timer
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void rf433_start_mesh_timer( uint16_t timeout )
{
    if (_rf433_mesh_timer_initialized == TRUE)
    {
        if (timeout == 0)
        {
            rf433_setmesh_config_exit();
        }
        else
        {
            rf433_setmesh_stop_timer();
        }
    }

    if (timeout != 0)
    {
        mlink_sys_set_status(SYS_MESH_STATE, MESH_IN);
        mico_init_timer(&_rf433_mesh_timer, timeout*1000, rf433_mesh_timeout_handle, NULL);
        mico_start_timer(&_rf433_mesh_timer);
        _rf433_mesh_timer_initialized = true;
    }

}

/********************************************************
 * function:  zigbee_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int rf433_setmesh(uint16_t devAddr, uint16_t timeout)
{
    rf433_start_mesh_timer(timeout);
}

/********************************************************
 * function:  rf433_psth_response_deal
 * description:
 * input:        1. data:       payload data
 *               2. dataLen     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void rf433_psth_response_deal( char *data, int size )
{
    os_rf433_log("rf433 passthrough response deal !!!");

}

/********************************************************
 * function:  rf433_psth_deal
 * description:
 * input:        1. data:       payload data
 *               2. size:     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void rf433_psth_deal( PUART_RECV_PACKET recvPacket )
{
    PUART_HEAD_T pDevHead = (PUART_HEAD_T)recvPacket->data;
    PUART_SUBDEV_HEAD_T pSubDevHead = (PUART_SUBDEV_HEAD_T)(recvPacket->data+UART_SEND_MAIN_HEAD_SIZE);
    DEVICEOBJ_T devObj;
    OSStatus ret = 0;
    uint8_t payloadNum = 1;
    uint8_t *payload = recvPacket->data+UART_SEND_PTH_HEAD_SIZE;
    uint8_t payloadLen = recvPacket->size - UART_SEND_PTH_HEAD_SIZE - 1;
    uint8_t plLenCnt = 0;
    uint8_t index = 0;
    uint8_t echoData[128] = {0};
    uint8_t echoSize = 0;
    uint8_t devMac[8] = {0};
    uint32_t srcAddr = 0;
    uint8_t setmeshState = 0;
    os_rf433_log("rf433 passthrough deal !!!");
    mlink_sys_get_status(SYS_MESH_STATE, &setmeshState);


    if (setmeshState == MESH_IN)
    {
        DISC_RFDEV_INFO_T devInfo = {0};
        os_rf433_log("come in mesh !!!");
        if (pSubDevHead->u_op.op_struct.op == ML_DP_SINGLE_REPORT)
        {
            memcpy(&devInfo.addr, &pSubDevHead->daddr, 3);
            memcpy(devInfo.mid, pSubDevHead->mid, 3);
            devInfo.mid[2] &= 0x0f;
            devInfo.comm = (pSubDevHead->mid[2]>> 4) & 0x0f;
            memcpy(devInfo.mac, pSubDevHead->mid, 3);
            memcpy(devInfo.mac+3, &pSubDevHead->daddr, 3);
            main_discover_notify(&devInfo);
        }
//        return;
    }
    memset(&devObj, 0, sizeof(DEVICEOBJ_T));
    devObj.comm = TELECOM_RF433;
    memcpy(&srcAddr, &pSubDevHead->daddr, 3);
    mlink_generate_subdev_net_addr(srcAddr, devObj.addr);
    sprintf(devObj.modelid, "%02x%02x%02x", pSubDevHead->mid[2]&0x0f, pSubDevHead->mid[1], pSubDevHead->mid[0]);
    memcpy(devMac, pSubDevHead->mid, 3);
    memcpy(devMac+3, &pSubDevHead->daddr, 3);
    HexToStr(devMac, 6, devObj.mac);
    os_rf433_log("device mac : %s", devObj.mac);
    ret = storage_check_devobj(&devObj);
    os_rf433_log("ret: %d\r\nop: %d, devid is %s", ret, pSubDevHead->u_op.op_struct.op, devObj.deviceId);
    if (ret == kGeneralErr )           // The device has not been added !!!
    {
        return;
    }
    if (pSubDevHead->u_op.op_struct.batch )
    {
        // The batch has not been achieved !!!
        payloadNum = *(payload+1);
        payload ++;
    }

    for (index=0; index < payloadNum; index++)
    {
        payload = payload + plLenCnt;
        switch (pSubDevHead->u_op.op_struct.op)
        {
            case ML_DP_SINGLE_REPORT:
                {
                    KEY_STATE_INFO keyStateInfo = {0};
                    KEY_ATTR_T_EX devAttr = {0};
                    memcpy(&keyStateInfo.keyType, payload, 2);
                    memcpy(keyStateInfo.keyState, payload+2, payloadLen-2);
                    keyStateInfo.stateSize = payloadLen-2;
                    keyStateInfo.keyIndex = keyStateInfo.keyType;
                    memcpy(echoData + echoSize, &keyStateInfo.keyIndex, 2);
                    echoSize += 2;
                    rf433_key_attr_logic_transformation(1, &keyStateInfo, &devAttr);
                    main_devstate_deal(devObj.addr, devObj.deviceId, 1, &devAttr);
                }
                break;
            default:
                break;
        }
    }
    uart_send_psth_echo( recvPacket, 0, echoData, echoSize );
}

OSStatus rf433_logic_init(rf433_logic_notify_t rf433_callback)
{
	OSStatus err = kNoErr;
    uart_config_rf433_pth_deal(rf433_psth_deal, rf433_psth_response_deal);
	return err;
}

