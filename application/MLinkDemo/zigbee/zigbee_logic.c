#include "mico.h"
#include "zigbee.h"
#include "../MLinkCommand.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "../uart/uart_logic.h"
#include "../uart/uart_packet.h"
#include "../uart/DTTable.h"
#include "MLinkAppDef.h"
#include "../main/main_server_disc_msg.h"

//#include "../main/main_server_ctrl.h"

#define os_zigbee_log(M, ...) custom_log("ZIGBEE", M, ##__VA_ARGS__)

#define ATTR_TYPE_TIME      5
#define PANID_REG_ADDR      1
static mico_timer_t _zigbee_mesh_timer;
static bool _zigbee_mesh_timer_initialized = false;

static mico_timer_t _zigbee_query_timer;
static bool _zigbee_query_timer_initialized = false;

static QUEUE obs_lnk_list_dev;           // The list records all devices which observe this device and the observers observe the attribute "address" .
static QUEUE obs_lnk_list_keytype;      // The list records all devices which observe this device and the observers observe the attribute "keytype" .

int zigbee_device_allow_setmesh(uint16_t devAddr, uint16_t timeout);

/*************************************************
  Function          :       zigbee_uart_psth_distribute
  Description       :   distribute the passthrough data
  Input:
      uart_recv_data:  uart data
      recv_len:           data length
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
//OSStatus zigbee_uart_psth_distribute(unsigned char *psth_data, uint8_t psth_len)
//{
//
//    PUART_SUBDEV_HEAD_T psubdev_head = (PUART_SUBDEV_HEAD_T)(psth_data);
//    PSTH_OP_T opStruct;
//    memset(&opStruct, 0, sizeof(PSTH_OP_T));
//    memcpy(&opStruct, psubdev_head->op, 2);
//
//    if ((opStruct.dir != 0) || (zigbee_funciton_distribute == NULL))
//    {
//        return FALSE;
//    }
//    os_zigbee_log("zigbee_uart_psth_distribute");
//    if (opStruct.batch == 0)
//    {
//        if (opStruct.resp != 0)
//        {
//            if (psubdev_head->state < 0)
//            {
//
//            }
//            zigbee_funciton_distribute(opStruct.op, psth_data, psth_len);
//        }
//        else
//        {
//            zigbee_funciton_distribute(opStruct.op, psth_data, psth_len);
//        }
//    }
//    else
//    {
//
//    }
//}

/********************************************************
 * function:  zigbee_observer_list_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:   init observer linked list
*********************************************************/
void zigbee_observer_list_init(void)
{
    init_queue(&obs_lnk_list_dev, sizeof(SUBNET_DEV_OBSERVER_INFO));
    init_queue(&obs_lnk_list_keytype, sizeof(SUBNET_KEYTYPE_OBSERVER_INFO));
}


/********************************************************
 * function:  zigbee_look_up_observer_about_dev
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
PSUBNET_DEV_OBSERVER_INFO zigbee_look_up_observer_about_dev( uint32_t addr )
{
    pQUEUE_NODE pObserverInfoNode = NULL;
    PSUBNET_DEV_OBSERVER_INFO pObserverInfo = NULL;
    pObserverInfoNode = obs_lnk_list_dev.head;

    int count = 0;
    while (pObserverInfoNode != NULL)
    {
        count ++;
        pObserverInfo = (PSUBNET_DEV_OBSERVER_INFO)pObserverInfoNode->data;
        if (pObserverInfo->addr == addr)
        {
            os_zigbee_log("observer is exist, its address is %d", addr);
            break;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }

    os_zigbee_log("observer num is %d, observer addr is %d", count, addr);

    if (pObserverInfoNode != NULL)
    {
        return pObserverInfo;
    }
    else
    {
        return NULL;
    }
}

/********************************************************
 * function:  zigbee_update_observer_dev_info
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_update_observer_dev_info(PSUBNET_DEV_OBSERVER_INFO pobserver_info)
{
    PSUBNET_DEV_OBSERVER_INFO pObserverInfo = NULL;

    pObserverInfo = zigbee_look_up_observer_about_dev(pobserver_info->addr);  // look up observer
    if (pObserverInfo == NULL)  // if we can't find observer, add this observer to linked list
    {
        push_node(&obs_lnk_list_dev, pobserver_info);
    }else  // if we have found it, update the info which observer wants to know
    {
        memcpy(pObserverInfo, pobserver_info, sizeof(SUBNET_DEV_OBSERVER_INFO));
    }
}

/********************************************************
 * function:  zigbee_del_observer_dev_info
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_del_observer_dev_info(PSUBNET_DEV_OBSERVER_INFO pobserver_info)
{
    pQUEUE_NODE pObserverInfoNode = NULL;
    PSUBNET_DEV_OBSERVER_INFO pObserverInfo = NULL;
    pObserverInfoNode = obs_lnk_list_dev.head;
    while (pObserverInfoNode != NULL)
    {
        pObserverInfo = (PSUBNET_DEV_OBSERVER_INFO)pObserverInfoNode->data;
        if (pObserverInfo->addr == pobserver_info->addr)
        {
            del_node(&obs_lnk_list_dev, pObserverInfoNode);
            break;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }
}

/********************************************************
 * function:  zigbee_look_up_observer_keytype
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
PSUBNET_KEYTYPE_OBSERVER_INFO zigbee_look_up_observer_keytype( uint32_t addr )
{
    pQUEUE_NODE pObserverInfoNode = NULL;
    PSUBNET_KEYTYPE_OBSERVER_INFO pObserverInfo = NULL;
    pObserverInfoNode = obs_lnk_list_keytype.head;
    while (pObserverInfoNode != NULL)
    {
        pObserverInfo = (PSUBNET_KEYTYPE_OBSERVER_INFO)pObserverInfoNode->data;
        if (pObserverInfo->addr == addr)
        {
            break;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }
    if (pObserverInfoNode != NULL)
    {
        return pObserverInfo;
    }
    else
    {
        return NULL;
    }
}


/********************************************************
 * function:  zigbee_update_observer_keytype_info
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_update_observer_keytype_info(PSUBNET_KEYTYPE_OBSERVER_INFO pobserver_info)
{
    PSUBNET_KEYTYPE_OBSERVER_INFO pObserverInfo = NULL;

    pObserverInfo = zigbee_look_up_observer_keytype(pobserver_info->addr);  // look up observer
    if (pObserverInfo == NULL)  // if we can't find observer, add this observer to linked list
    {
        push_node(&obs_lnk_list_keytype, pobserver_info);
    }else  // if we have found it, update the info which observer wants to know
    {
        memcpy(pObserverInfo, pobserver_info, sizeof(SUBNET_KEYTYPE_OBSERVER_INFO));
    }
}

/********************************************************
 * function:  zigbee_del_observer_keytype_info
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_del_observer_keytype_info(PSUBNET_KEYTYPE_OBSERVER_INFO pobserver_info)
{
    pQUEUE_NODE pObserverInfoNode = NULL;
    PSUBNET_KEYTYPE_OBSERVER_INFO pObserverInfo = NULL;
    pObserverInfoNode = obs_lnk_list_keytype.head;
    while (pObserverInfoNode != NULL)
    {
        pObserverInfo = (PSUBNET_KEYTYPE_OBSERVER_INFO)pObserverInfoNode->data;
        if (pObserverInfo->addr == pobserver_info->addr)
        {
            del_node(&obs_lnk_list_keytype, pObserverInfoNode);
            break;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }
}

/********************************************************
 * function:  zigbee_observe_dev_state_notify
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_observe_dev_state_notify(uint8_t subnet, uint8_t *addr, unsigned char state_num, PKEY_STATE_INFO pstate_info)
{
    uint8_t count = 0;
    uint8_t notifyNum = 0;
    uint32_t observerAddr = 0;
    PKEY_STATE_INFO pstateInfoTemp = NULL;
    SUBNET_STATE_NOTIFY_INFO notifyInfo[5] = {0};
    PSUBNET_DEV_OBSERVER_INFO pObserverInfo = NULL;
    pQUEUE_NODE pObserverInfoNode = NULL;

    // look up device's observer if there are any devcie observe it
    pObserverInfoNode = obs_lnk_list_dev.head;
    while(pObserverInfoNode)
    {
        // have someone observer
        pObserverInfo = (PSUBNET_DEV_OBSERVER_INFO)pObserverInfoNode->data;
        os_zigbee_log("pObserverInfo->num is %d", pObserverInfo->num);
        for (count=0; count<pObserverInfo->num; count++)        // Traversal the observed device
        {
            if (subnet == pObserverInfo->obsDevInfo[count].subAddr)       //
            {
                if (0 == memcmp(pObserverInfo->obsDevInfo[count].devAddr, addr, 3))  // judge that there is a observed device
                {
                    uint8_t stateIndex = 0;
                    // check if device is match
                    for (stateIndex= 0; stateIndex<state_num; stateIndex++)
                    {
                        pstateInfoTemp = pstate_info + stateIndex;
                        if (pstateInfoTemp->keyIndex == pObserverInfo->obsDevInfo[count].key)
                        {
                            notifyInfo[notifyNum].subnet = pObserverInfo->obsDevInfo[count].subAddr;
                            notifyInfo[notifyNum].stateLen = pstateInfoTemp->stateSize;
                            memcpy(notifyInfo[notifyNum].addr, addr, 3);
                            memcpy(notifyInfo[notifyNum].dpIndex, &pstateInfoTemp->keyIndex, 2);
                            if (pstateInfoTemp->stateSize <= sizeof(notifyInfo[notifyNum].data))
                            {
                                memcpy(notifyInfo[notifyNum].data, pstateInfoTemp->keyState, pstateInfoTemp->stateSize);
                                notifyNum ++;
                            }
                        }
                        if (notifyNum == 5)
                        {
                            observerAddr = pObserverInfo->addr;
                            uart_observe_state_notify((uint8_t *)&observerAddr, notifyNum, notifyInfo);
                            notifyNum = 0;
                        }
                    }
                }
            }
        }
        if (notifyNum)
        {
            observerAddr = pObserverInfo->addr;
            os_zigbee_log("zigbee notify");
            uart_observe_state_notify((uint8_t *)&observerAddr, notifyNum, notifyInfo);
            notifyNum = 0;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }
}

/********************************************************
 * function:  zigbee_observe_keytype_state_notify
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_observe_keytype_state_notify(uint8_t subnet, uint8_t *addr, unsigned char state_num, PKEY_STATE_INFO pstate_info)
{
    uint8_t count = 0;
    uint8_t notifyNum = 0;
    uint32_t observerAddr = 0;
    PKEY_STATE_INFO pstateInfoTemp = NULL;
    SUBNET_STATE_NOTIFY_INFO notifyInfo[5] = {0};
    PSUBNET_KEYTYPE_OBSERVER_INFO pObserverInfo = NULL;
    pQUEUE_NODE pObserverInfoNode = NULL;

    // look up device's observer if there are any devcie observe it
    pObserverInfoNode = obs_lnk_list_keytype.head;
    while(pObserverInfoNode)
    {
        // have observer infomation
        pObserverInfo = (PSUBNET_KEYTYPE_OBSERVER_INFO)pObserverInfoNode->data;
        os_zigbee_log("pObserverInfo->num is %d", pObserverInfo->num);
        for (count=0; count<pObserverInfo->num; count++)        // Traversal the observed device
        {
            uint8_t stateIndex = 0;
            // check if device is match which's state is change
            for (stateIndex= 0; stateIndex<state_num; stateIndex++)
            {
                pstateInfoTemp = pstate_info + stateIndex;
                if (pstateInfoTemp->keyType == pObserverInfo->keytype[count])
                {
                    notifyInfo[notifyNum].subnet = subnet;
                    notifyInfo[notifyNum].stateLen = pstateInfoTemp->stateSize;
                    memcpy(notifyInfo[notifyNum].addr, addr, 3);
                    memcpy(notifyInfo[notifyNum].dpIndex, &pstateInfoTemp->keyIndex, 2);
                    if (pstateInfoTemp->stateSize <= sizeof(notifyInfo[notifyNum].data))
                    {
                        memcpy(notifyInfo[notifyNum].data, pstateInfoTemp->keyState, pstateInfoTemp->stateSize);
                        notifyNum ++;
                    }
                }
                if (notifyNum == 5)
                {
                    uart_observe_state_notify((uint8_t *)&observerAddr, notifyNum, notifyInfo);
                    notifyNum = 0;
                }
            }

        }
        if (notifyNum)
        {
            uart_observe_state_notify((uint8_t *)&observerAddr, notifyNum, notifyInfo);
            notifyNum = 0;
        }
        pObserverInfoNode = pObserverInfoNode->next;
    }
}

/********************************************************
 * function:  zigbee_observe_state_notify
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_observe_state_notify(uint8_t subnet, uint8_t *addr, unsigned char state_num, PKEY_STATE_INFO pstate_info)
{
    zigbee_observe_dev_state_notify(subnet, addr, state_num, pstate_info);
    zigbee_observe_keytype_state_notify(subnet, addr, state_num, pstate_info);
}

/********************************************************
 * function:  zigbee_statechange_notify
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_statechange_notify(char *net_address, uint8_t state_num, PSTATE_DATA_T pstate_info)
{
    unsigned char addr[4] = {0};
    unsigned char subnet = 0;
    unsigned int devAddr = 0;
    unsigned char stateIndex = 0;
    unsigned char strLen = 0;
    PSTATE_DATA_T pstateTemp = NULL;
    KEY_STATE_INFO stateInfo[ENDPOINTLIST_NUM] = {0};
    mlink_parse_subdev_net_addr(TELECOM_ZIGBEE, net_address, addr);
    subnet = addr[2];
    memcpy((uint8_t *)&devAddr, addr, 2);


    for (stateIndex=0; stateIndex<state_num; stateIndex++)
    {
        pstateTemp = pstate_info + stateIndex;
        strLen = strlen(pstateTemp->value);
        stateInfo[stateIndex].keyIndex = atoi(pstateTemp->key);
        stateInfo[stateIndex].keyType = atoi(pstateTemp->keyType);
        stateInfo[stateIndex].stateSize = (strLen+1)/2;
        StrToHexEx(pstateTemp->value, strLen, stateInfo[stateIndex].keyState);
    }
    zigbee_observe_state_notify(subnet, (uint8_t *)&devAddr, state_num, stateInfo);
}

/********************************************************
 * function:  zigbee_set_mid
 * description:
 * input:       modelid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_set_mid( char *modelid )
{
    char midStr[7] = {0};
    uint8_t mid[3] = {0};
    uint8_t size = 0;
    if ((modelid == NULL) || (*modelid == 0))
    {
        return;
    }
    memcpy(midStr, modelid, 6);
    midStr[0] = '2';
    size = (strlen(midStr)<=6)?strlen(midStr):6;
    StrToHex(midStr, size, mid);
    SetMID(mid);
}


/********************************************************
 * function:  zigbee_get_dev_info
 * description:
 * input:       1. daddr
 * output:      1.soft_ver
 *              2. hardware
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_get_dev_info(uint16_t daddr, char *soft_ver, char *hardware)
{
    int ret = kGeneralErr;

    char echoData[128] = {0};
    uint8_t echoLen = 0;
    ret = uart_get_device_info(daddr, echoData, &echoLen);
    if (ret != -1)
    {
        if (hardware)
        {
            sprintf(hardware, "%02d.%02d", echoData[7], echoData[6]);
        }
        if (soft_ver)
        {
            sprintf(soft_ver, "%02d.%02d", echoData[9], echoData[8]);
        }
    }
    return ret;

}


/********************************************************
 * function:  zigbee_read_attr
 * description:
 * input:       pobj_attr
 * output:      pdev_attr_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_read_attr(char *modelid, char *addr, char *key, PDEV_FUNCTION_OBJ_EX_T pattr_obj)
{
    os_zigbee_log("reading object attribute\r\n");
    int ret = 0;
    uint16_t dpAddr = 0;
    uint8_t dpAttrId = PROP_DATA;
    uint8_t value[ATTR_DATA_MAX] = {0};
    if ((addr == NULL) || (key == NULL) || (pattr_obj == NULL) ||(modelid == NULL))
    {
        return TELECOM_RSP_ERR;
    }
    if (key != NULL)
    {
        zigbee_set_mid(modelid);
        dpAddr = atoi(key);
        dpAddr = (dpAddr<<4) + dpAttrId;           // datapoint attribute id is 3
        ret = uart_read_attr_ex(addr, dpAddr, value);
        if ( ret != kGeneralErr )
        {
            mlink_reverse_translate_value(value, (uint8_t)ret, &pattr_obj->valtype, pattr_obj->value);
        }
        strcpy(pattr_obj->key, key);
        pattr_obj->type = PROP_DATA;
    }

    return TELECOM_RSP_OK;


}

/********************************************************
 * function:  zigbee_read_attr
 * description:
 * input:       pobj_attr
 * output:      pdev_attr_obj
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_read_attrs( char *modelid, uint8_t *addr )
{
    os_zigbee_log("reading object attribute\r\n");

    if (( addr == NULL ) || (modelid == NULL))
    {
        return TELECOM_RSP_ERR;
    }
    zigbee_set_mid(modelid);
    uart_query_attrs(addr);
    return TELECOM_RSP_OK;

}

/********************************************************
 * function:  zigbee_write_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_write_attr(int class_id, char *modelid, unsigned char *addr, PDEV_FUNCTION_OBJ_EX_T dev_attr_obj, uint32_t wait_time_ms)
{
    OSStatus ret = 0;

    if ((addr == NULL)|| (modelid == NULL))
    {
        return TELECOM_RSP_ERR;
    }
    uint8_t buffer[128] = {0};
    uint16_t dataPointAddr = 0;
    uint16_t keycode = 0;
    uint8_t size = 0;
    zigbee_set_mid(modelid);
    if (class_id == SCENE_OBJ_ID)
    {
        uint16_t sceneIndex = 0;
        sceneIndex = *((uint16_t *)addr);
        uart_ctrl_scene(sceneIndex);
        return TELECOM_RSP_OK;
    }

    if (dev_attr_obj == NULL)
    {
        return TELECOM_RSP_ERR;
    }

    keycode = (uint16_t)atoi(dev_attr_obj->key);
    dataPointAddr = (keycode<<4) + dev_attr_obj->type;

    ret = mlink_translate_value(dev_attr_obj->valtype, dev_attr_obj->value, buffer, &size);

    if (ret != kGeneralErr)
    {
        ret = uart_write_attr(addr, dataPointAddr, buffer, size, wait_time_ms);
    }
    return ret;
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
OSStatus zigbee_readmult_attr()
{
    os_zigbee_log("reading objects attribute\r\n");
    return kNoErr;
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
OSStatus zigbee_writemult_attr()
{
    os_zigbee_log("writing objects attribute\r\n");
    return kNoErr;
}

/********************************************************
 * function:  zigbee_writegroup_attr
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_writegroup_attr()
{
    os_zigbee_log("writing group objects attribute\r\n");
    return kNoErr;
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
int zigbee_get_node_type(uint16_t devAddr, uint16_t *type)
{
    uint8_t buffer[32] = {0};
    uint8_t buffSize = 0;
    int ret = 0;
    if (type == NULL)
    {
        return kGeneralErr;
    }
    ret = uart_read_register(devAddr, 0, 1, buffer, &buffSize);
    if (buffSize != 0)
    {
        memcpy((uint8_t *)type, buffer+3, 2);
    }
    else
    {
        os_zigbee_log("uart_read_register ret is %d", ret );
    }
    return kNoErr;
}

/********************************************************
 * function:  zigbee_get_node_panid
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_get_node_panid(uint16_t devAddr, uint16_t *panid)
{
    uint8_t buffer[32] = {0};
    uint8_t buffSize = 0;
    int ret = 0;
    if (panid == NULL)
    {
        return kGeneralErr;
    }
    ret = uart_read_register(devAddr, 0, 1, buffer, &buffSize);
    if (buffSize != 0)
    {
        memcpy((uint8_t *)panid, buffer+2, 2);
    }
    else
    {
        os_zigbee_log("uart_read_register ret is %d", ret );
    }
    return kNoErr;
}

static void zigbee_setmesh_stop_timer()
{
    mico_stop_timer(&_zigbee_mesh_timer);
    mico_deinit_timer( &_zigbee_mesh_timer );
    _zigbee_mesh_timer_initialized = false;
}

static void zigbee_setmesh_config_exit( void )
{
    mlink_sys_set_status(SYS_MESH_STATE, MESH_OUT);
    if (_zigbee_mesh_timer_initialized == TRUE)
    {
        zigbee_setmesh_stop_timer();
    }
}


/********************************************************
 * function:  zigbee_out_of_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_out_of_setmesh( uint16_t devAddr )
{
    int ret = 0;
    ret = uart_setmesh_allow(devAddr, 0);
    if ((devAddr == 0xffff) || (devAddr == 0x8000))
    {
        zigbee_setmesh_config_exit();
    }
    return ret;
}


/********************************************************
 * function:  zigbee_mesh_timeout_handle
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
static void zigbee_mesh_timeout_handle( void *arg )
{
    zigbee_out_of_setmesh(0xffff);
}


/********************************************************
 * function:  zigbee_start_mesh_timer
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
static void zigbee_start_mesh_timer( uint16_t timeout )
{
    if (_zigbee_mesh_timer_initialized == TRUE)
    {
        if (timeout == 0)
        {
            zigbee_setmesh_config_exit();
        }
        else
        {
            zigbee_setmesh_stop_timer();
        }
    }

    if (timeout != 0)
    {
        mlink_sys_set_status(SYS_MESH_STATE, MESH_IN);
        mico_init_timer(&_zigbee_mesh_timer, timeout*1000, zigbee_mesh_timeout_handle, NULL);
        mico_start_timer(&_zigbee_mesh_timer);
        _zigbee_mesh_timer_initialized = true;
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
int zigbee_setmesh_start_coordinate(uint16_t devAddr, uint16_t timeout)
{
    int ret = 0;
    ret = uart_setmesh(devAddr, SETMESH_COORDINATOR, timeout);

    if (devAddr == 0xffff || devAddr == 0x8000)
    {
        zigbee_start_mesh_timer(timeout);
    }

//    mlink_gpio_beep_on();
    return ret;
}


/********************************************************
 * function:  zigbee_device_allow_setmesh
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_device_allow_setmesh(uint16_t devAddr, uint16_t timeout)
{
    int ret = 0;
    ret = uart_setmesh_allow(devAddr, timeout);
    if (devAddr == 0xffff || devAddr == 0x8000 )
    {
        zigbee_start_mesh_timer(timeout);
    }
    return ret;
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
int zigbee_setmesh(uint16_t devAddr, uint16_t timeout)
{
    os_zigbee_log("start to set mesh\r\n");
    uint8_t echoData[64] = {0};
    uint8_t echoLen = 0;
    int ret = 0;
    uint16_t shortAddr = 0;

    if (devAddr == 0xffff)
    {
        ret = uart_get_device_info(0x8000, echoData, &echoLen);
    }
    else
    {
        ret = uart_get_device_info(devAddr, echoData, &echoLen);
    }
    if (ret == kGeneralErr)
    {
        return ret;
    }

    shortAddr = *(uint16_t *)(echoData+4);
    os_zigbee_log("mesh state is %d, shortAddr: %x", echoData[0], shortAddr);
    switch(echoData[0])
    {
        case SUB_NET_STATE_HAND_UP:
            zigbee_setmesh_start_coordinate(devAddr, timeout);
            break;
        case SUB_NET_STATE_ROUTE:
        case SUB_NET_STATE_CODNT:
        case SUB_NET_STATE_ORPHAN:
        {
            os_zigbee_log("Allow to add to the mesh!!! state is %d, timeout: %d", echoData[0], timeout);
            ret = zigbee_device_allow_setmesh(devAddr, timeout);
        }
            break;

        case SUB_NET_STATE_SEARCH:                  // 路由器组网中 需启动协调器
        case SUB_NET_STATE_INITIAL:             // 组网中
        case SUB_NET_STATE_JOINT:
        case SUB_NET_STATE_RESTART_JOINT:
        case SUB_NET_STATE_TERMINAL_UNAUTHORIZE:
        case SUB_NET_STATE_CODNT_STARTING:
            break;
        case SUB_NET_STATE_TERMINAL:            // 组网成功  只需允许加网
            break;
        default:
            ret = zigbee_device_allow_setmesh(devAddr, timeout);
            break;
    }
    return ret;
}


/********************************************************
 * function:  zigbee_restore_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_resetmesh(uint16_t addr)
{
    uart_resetmesh(addr);
}

void zigbee_cleardata(uint16_t addr)
{
    uart_clear_data(addr);
}

/********************************************************
 * function:  zigbee_restore_device
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_restore_device(uint16_t addr)
{
    uart_restore_device(addr);
}

/********************************************************
 * function:  zigbee_reboot_device
 * description:
 * input:        1. addr: it's address about the device which you want to reset
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_reboot_device(uint16_t addr)
{
    uart_reboot_device(addr);
}

/********************************************************
 * function:  zigbee_set_addr
 * description:
 * input:        1. daddr: The old addr of device
 *               2. mac: the mac of device
 *               3. addr: the new addr of device
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_set_addr( uint16_t daddr, uint8_t *mac, uint16_t *addr )
{
    int ret = 0;
    ret = uart_set_netaddr(daddr, mac, addr);
    return ret;
}

/********************************************************
 * function:  zigbee_discover_device
 * description: discover all devices
 * input:        1. timeout
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_discover_device( uint8_t timeout )
{
    uart_discover_device(0xFFFE, timeout);
}

/********************************************************
 * function:  zigbee_discover_device
 * description: discover all devices
 * input:        1. timeout
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_add_scene( uint8_t *daddr, char *modelid, char *scene_id, PDEV_FUNCTION_OBJ_T dev_attr)
{
    if ((daddr == NULL)||(scene_id == NULL)||(dev_attr == NULL))
    {
        return kGeneralErr;
    }
    SCENE_ADD_PARAM sceneParam = {0};
    uint16_t keyCode = (uint16_t)atoi(dev_attr->key);
    int ret = 0;
    zigbee_set_mid(modelid);
    sceneParam.sceneIndex = (uint16_t)atoi(scene_id);
    mlink_translate_value(dev_attr->valtype, dev_attr->value, sceneParam.sceneData, &sceneParam.sceneDataLen);
    ret = uart_add_scene(daddr, keyCode, 1, &sceneParam);
    return ret;
}

/********************************************************
 * function:  zigbee_del_scene
 * description: discover all devices
 * input:        1. timeout
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_del_scene( uint8_t *daddr, char *modelid, char *scene_id, char *key)
{
    if ((daddr == NULL)||(scene_id == NULL)||(key == NULL))
    {
        return kGeneralErr;
    }
    uint16_t sceneIndex = 0;
    uint16_t keyCode = (uint16_t)atoi(key);
    int ret = 0;
    zigbee_set_mid(modelid);
    sceneIndex = (uint16_t)atoi(scene_id);
    ret = uart_del_scene(daddr, keyCode, 1, &sceneIndex);
    return ret;
}

/********************************************************
 * function:  zigbee_clear_scene
 * description: discover all devices
 * input:        1. timeout
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_clear_scene( uint8_t *daddr, char *modelid, char *key)
{
    if ((daddr == NULL)||(key == NULL))
    {
        return kGeneralErr;
    }
    uint16_t keyCode = (uint16_t)atoi(key);
    int ret = 0;
    zigbee_set_mid(modelid);
    ret = uart_clear_scene(daddr, keyCode);
    return ret;
}

/********************************************************
 * function:  zigbee_get_scene
 * description: discover all devices
 * input:        1. daddr
 *               2. modelid
 *               3. pattr_obj
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_get_scene( PDEVSET_SIP_PARAM_T psip_param, PDEV_FUNCTION_OBJ_T pattr_obj, char *page_info)
{
    if ((psip_param == NULL)||(pattr_obj == NULL)||(page_info == NULL))
    {
        return kGeneralErr;
    }

    uint16_t dpIndex = (uint16_t)atoi(pattr_obj->key);
    uint8_t echoData[128] = {0};
    int  len = 0;
    uint16_t startIndex = 0;
    if (psip_param->limit == 0)
    {
        psip_param->limit = 10;
    }
    if (psip_param->page == 0)
    {
        psip_param->page = 1;
    }
    startIndex = (psip_param->page-1)*psip_param->limit;
    zigbee_set_mid(psip_param->modelid);
    len = uart_get_scene(&psip_param->addr, dpIndex, startIndex, echoData);
    if (len > 0)
    {
        uint16_t lenPerScene = 0;        // 情景地址长度+情景数据长度
        uint16_t totalScene = 0;
        uint8_t sceneNum = 0;
        uint16_t reportNum = 0;
        uint8_t maxPages = 0;
        memcpy( &totalScene, echoData+2, 2 );
        sceneNum = *(echoData + 6);
        if (sceneNum != 0)
        {
            lenPerScene = (len - 7)/sceneNum;
            reportNum = (sceneNum > psip_param->limit)?psip_param->limit:sceneNum;
            maxPages = mlink_get_total_pages(totalScene, psip_param->limit);
            sprintf(page_info, "[%d|%d|%d|%d]", totalScene, psip_param->limit, maxPages, psip_param->page);
            len = reportNum*lenPerScene;
        }
        else
        {
            sprintf(page_info, "[%d|%d|%d|%d]", totalScene, psip_param->limit, maxPages, psip_param->page);
            len = len -7;
        }
        memset(pattr_obj->value, 0, sizeof(pattr_obj->value));
        HexToStrEx(echoData+7, len, pattr_obj->value);
        pattr_obj->type = 7;
        pattr_obj->valtype = 50;
    }
    return kNoErr;
}


/********************************************************
 * function:  zigbee_add_group
 * description: discover all devices
 * input:        1. daddr
 *               2. modelid
 *               3. group_id
 *               4. key
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_add_group( uint8_t *daddr, char *modelid, char *group_id, char *key)
{
    if ((daddr == NULL)||(group_id == NULL)||(key == NULL))
    {
        return kGeneralErr;
    }
    int ret = 0;
    uint16_t keyCode = (uint16_t)atoi(key);
    uint16_t groupId = (uint16_t)atoi(group_id);
    zigbee_set_mid(modelid);
    ret = uart_add_group(daddr, keyCode, 1, &groupId);
    return ret;
}

/********************************************************
 * function:  zigbee_del_group
 * description: discover all devices
 * input:        1. daddr
 *               2. modelid
 *               3. group_id
 *               4. key
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_del_group( uint8_t *daddr, char *modelid, char *group_id, char *key)
{
    if ((daddr == NULL)||(group_id == NULL)||(key == NULL))
    {
        return kGeneralErr;
    }
    uint16_t groupId = (uint16_t)atoi(group_id);
    uint16_t keyCode = (uint16_t)atoi(key);
    int ret = 0;
    zigbee_set_mid(modelid);
    ret = uart_del_group(daddr, keyCode, 1, &groupId);
    return ret;
}

/********************************************************
 * function:  zigbee_clear_group
 * description: discover all devices
 * input:       1. daddr
 *              2. modelid
 *              3. key
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_clear_group( uint8_t *daddr, char *modelid, char *key)
{
    if ((daddr == NULL)||(key == NULL))
    {
        return kGeneralErr;
    }
    uint16_t keyCode = (uint16_t)atoi(key);
    int ret = 0;
    zigbee_set_mid(modelid);
    ret = uart_clear_group(daddr, keyCode);
    return ret;
}

/********************************************************
 * function:  zigbee_get_scene
 * description: discover all devices
 * input:        1. daddr
 *               2. modelid
 *               3. pattr_obj
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_get_group( PDEVSET_SIP_PARAM_T psip_param, PDEV_FUNCTION_OBJ_T pattr_obj, char *page_info)
{
    if ((psip_param == NULL)||(pattr_obj == NULL)||(page_info == NULL))
    {
        return kGeneralErr;
    }

    uint16_t dpIndex = (uint16_t)atoi(pattr_obj->key);
    uint8_t echoData[128] = {0};
    int  len = 0;
    uint16_t startIndex = 0;
    if (psip_param->limit == 0)
    {
        psip_param->limit = 10;
    }
    if (psip_param->page == 0)
    {
        psip_param->page = 1;
    }
    startIndex = (psip_param->page-1)*psip_param->limit;
    zigbee_set_mid(psip_param->modelid);
    len = uart_get_group(&psip_param->addr, dpIndex, startIndex, echoData);
    if (len > 0)
    {
        uint16_t lenPerGroup = 2;        // 组地址长度
        uint16_t totalGroup = 0;
        uint8_t groupNum = 0;
        uint16_t reportNum = 0;
        uint8_t maxPages = 0;
        memcpy( &totalGroup, echoData+2, 2 );
        groupNum = *(echoData + 6);
        reportNum = (groupNum > psip_param->limit)?psip_param->limit:groupNum;
        maxPages = totalGroup/psip_param->limit + ((totalGroup%psip_param->limit)?1:0);
        sprintf(page_info, "[%d|%d|%d|%d]", totalGroup, psip_param->limit, maxPages, psip_param->page);
        len = reportNum*lenPerGroup;

        memset(pattr_obj->value, 0, sizeof(pattr_obj->value));
        HexToStrEx(echoData+7, len, pattr_obj->value);
        pattr_obj->type = 7;
        pattr_obj->valtype = 50;
    }
    return kNoErr;
}


/********************************************************
 * function:  zigbee_set_panid
 * description:
 * input:        1. panid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_set_panid( uint16_t panid)
{
    uint16_t regdata = panid;
    int ret = 0;
    ret = uart_write_register(0x8000, PANID_REG_ADDR, 1, (uint8_t *)&regdata);
    return ret;
}

/********************************************************
 * function:  zigbee_get_panid
 * description:
 * input:        1. panid
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_get_panid( uint16_t *panid)
{
    int ret = 0;
    uint8_t len = 0;
    uint8_t data[16] = {0};
    ret = uart_read_register(0x8000, PANID_REG_ADDR, 1, data, &len);
    if (ret != -1)
    {
        memcpy(panid, data+3, 2);
        os_zigbee_log("datalen is %d, panid is: %d", len, *panid);
    }
    return ret;
}


/********************************************************
 * function:  zigbee_set_net_param
 * description:
 * input:        1. net_param
 *               2. valtype
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_set_net_param( char *net_param, uint8_t valtype)
{
    uint8_t netParam[32] = {0};
    uint8_t size = 0;
    int ret = 0;

    if (net_param == NULL)
    {
        return kGeneralErr;
    }
    mlink_translate_value(valtype, net_param, netParam, &size);
    ret = uart_set_net_param(netParam, size);
    return ret;
}

/********************************************************
 * function:  zigbee_get_net_param
 * description:
 * input:
 * output:  1. net_param
 *          2. valtype
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_get_net_param( char *net_param, uint8_t *valtype)
{
    uint8_t netParam[32] = {0};
    uint8_t size = 0;
    int ret = 0;

    if (net_param == NULL)
    {
        return kGeneralErr;
    }
    ret = uart_get_net_param(netParam, &size);
    if ((ret != -1) && (size != 0))
    {
        mlink_reverse_translate_value(netParam, size, valtype, net_param);
    }
    else
    {
        *net_param = 0;
    }
    return ret;
}

/********************************************************
 * function:  zigbee_key_attr_logic_transformation
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_key_attr_logic_transformation( uint8_t state_num, PKEY_STATE_INFO pkey_state, PKEY_ATTR_T_EX pdev_attr)
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
        if (pKeyStateTemp->keyType == 604)
        {
            HexToStrEx(pKeyStateTemp->keyState, pKeyStateTemp->stateSize, pDevAttrTemp->keyState.value);
            pDevAttrTemp->keyState.valtype = VALTYPE_HEX_STR;
        }
        else
        {
            mlink_reverse_translate_value(pKeyStateTemp->keyState, pKeyStateTemp->stateSize, &pDevAttrTemp->keyState.valtype, pDevAttrTemp->keyState.value);
        }
    }
    return kNoErr;
}

/********************************************************
 * function:  zigbee_psth_response_deal
 * description:
 * input:        1. data:       payload data
 *               2. dataLen     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_psth_response_deal( PUART_RECV_PACKET recvPacket )
{
#if 0
    PUART_HEAD_T pDevHead = (PUART_HEAD_T)recvPacket->data;
    PUART_SUBDEV_RSP_HEAD_T pSubDevHead = (PUART_SUBDEV_RSP_HEAD_T)(recvPacket->data+sizeof(UART_RSP_HEAD_T));
    uint8_t *payload = recvPacket->data + UART_RSP_PTH_HEAD_SIZE;
    uint8_t payloadLen = recvPacket->size - UART_RSP_PTH_HEAD_SIZE - 1;
    DEVICEOBJ_T devObj;
    OSStatus ret = 0;

    memset(&devObj, 0, sizeof(DEVICEOBJ_T));
    mlink_generate_subdev_net_addr(pDevHead->srcAddr, devObj.addr);
    sprintf(devObj.modelid, "%02x%02x%02x", pSubDevHead->mid[2]&0x0f, pSubDevHead->mid[1], pSubDevHead->mid[0]);
    ret = storage_check_devobj(devObj.addr, &devObj);

    switch (pSubDevHead->u_op.op_struct.op)
    {
        case ML_DP_READ:
        {

        }
            break;

        default:
            break;
    }
#endif
}

/********************************************************
 * function:  zigbee_write_forworded_deal
 * description:
 * input:        1. payload:       payload data
 *               2. payload_len     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_write_forworded_deal(unsigned char *payload, unsigned char payload_len)
{
    uint8_t subaddr = 0;
    uint32_t addr = 0;
    uint16_t key = 0;
    uint8_t attrId = 0;
    uint8_t *writeData = NULL;
    uint8_t writeDataLen = 0;

    // (subnet address)1B+(device address)3B+(data point index)2B+(attribute id)1B+(data)NB
    subaddr = *payload;                     // get subnet address
    memcpy(&addr, payload+1, 3);            // get addr
    memcpy(&key, payload+4, 2);             // get key value
    attrId = *(payload+6);                  // get attribute id
    writeData = payload + 7;                // get ctrl data
    writeDataLen = payload_len-7;           // get ctrl data length

    // if subnet is zero we will send data to zigbee network in local
    if (subaddr == 0)
    {
        uint16_t dpAddr = 0;
        dpAddr = (key<<4) + attrId;
        uart_write_attr((uint8_t *)&addr, dpAddr, writeData, writeDataLen, 1000);
    }
    else //when subnet isn't equal zero, send write command to other zigbee network with milink protocol
    {
        DEVATTR_WRITE_T attrData = {0};
        DEV_FUNCTION_OBJ_EX_T funcObj = {0};
        char netPanid[4] = {0};

        attrData.classID = DEVICE_OBJ_ID;
        attrData.num = 1;
        sprintf(netPanid, "%02X", subaddr);
        sprintf(attrData.netaddr, "%02X.%06X", subaddr, addr);
        attrData.opobj = &funcObj;
        sprintf(attrData.opobj->key, "%d", key);
        attrData.opobj->type = attrId;
        attrData.opobj->valtype = VALTYPE_HEX_STR;
        HexToStrEx(writeData, writeDataLen, attrData.opobj->value);
        mlcoap_client_ctrl_write(netPanid, &attrData);
    }
}

/********************************************************
 * function:  zigbee_psth_response_deal
 * description:
 * input:        1. dp_addr:       payload data
 * output:       1. echo:
 *               2. echo_size:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int zigbee_read_distribute(uint16_t dp_addr, uint8_t *echo, uint8_t *echo_size)
{
    int ret = 0;
    if (echo == NULL || echo_size == NULL)
    {
        return kGeneralErr;
    }
    os_zigbee_log("read dp addr is %d", dp_addr);
    switch (dp_addr)
    {
        case ATTR_TYPE_TIME:
        {
            mico_utc_time_t utc_time;
            mico_time_get_utc_time(&utc_time);
            utc_time += 3600*8;
            memcpy(echo, (uint8_t *)&utc_time, 4);
            *echo_size = 4;
        }
            break;
        default:
            break;
    }
    ret = kNoErr;
    return ret;
}

/********************************************************
 * function:  zigbee_observe_dev_deal
 * description:
 * input:        1. data:       payload data
 *               2. dataLen     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_observe_dev_deal(PSUBNET_DEV_OBSERVER_INFO pobserver_info)
{
    char addr[4] = {0};
    uint32_t panid = 0;
    uint8_t count = 0;
    uint32_t devAddr = 0;
    DEVICEOBJ_T devObj = {0};
    int ret = 0;
    uint8_t endpointCount = 0;

    if (pobserver_info)
    {
        PSUBNET_STATE_NOTIFY_INFO pnotifyInfo = malloc(OBSERVE_SUBDEV_MAX*sizeof(SUBNET_STATE_NOTIFY_INFO));
        uint32_t devIndex = 0;
        PDEV_ENDPOINT_STATUS_T pdevEndpointTemp = NULL;
        PENDPOINT_STATUS_T pEndpointStatusTemp = NULL;
        uint8_t notifyNum = 0;
        PSUBNET_STATE_NOTIFY_INFO pnotifyInfoTemp = NULL;
        if ( pobserver_info->num > OBSERVE_SUBDEV_MAX )
        {
            pobserver_info->num =  OBSERVE_SUBDEV_MAX;
        }
        mlink_get_local_addr(addr);
        StrToHex(addr, strlen(addr), (uint8_t *)&panid);
        for (count=0; count < pobserver_info->num; count++)
        {
            if (pobserver_info->obsDevInfo[count].subAddr == 0 || pobserver_info->obsDevInfo[count].subAddr == panid)
            {
                // deal device state in this gateway.
                memcpy((uint8_t *)&devAddr, pobserver_info->obsDevInfo[count].devAddr, 3);
                memcpy(((uint8_t *)&devAddr +2), &panid, 1);
                os_zigbee_log("devaddr: %x", devAddr);
                ret = storage_get_dev_by_addr((uint8_t *)&devAddr, &devObj);
                if (ret == kNoErr)
                {
                    pdevEndpointTemp = storage_get_endpoints_status(devObj.deviceId);
                    if (pdevEndpointTemp)
                    {
                        for (endpointCount=0; endpointCount < pdevEndpointTemp->endpointNum; endpointCount++)
                        {
                            pEndpointStatusTemp = pdevEndpointTemp->endpointStatus + endpointCount;
                            os_zigbee_log("key: %s ==? %d", pEndpointStatusTemp->keyAttr.keyState.key, pobserver_info->obsDevInfo[count].key);
                            if (atoi(pEndpointStatusTemp->keyAttr.keyState.key) == pobserver_info->obsDevInfo[count].key)
                            {
                                // find this key
                                pnotifyInfoTemp = pnotifyInfo + notifyNum;
                                memcpy(pnotifyInfoTemp->dpIndex, (uint8_t *)&pobserver_info->obsDevInfo[count].key, 2);
                                memcpy(pnotifyInfoTemp->addr, pobserver_info->obsDevInfo[count].devAddr, 3);
                                pnotifyInfoTemp->subnet = 0;        // the panid is 0, if the device is in this gateway
                                mlink_translate_value(pEndpointStatusTemp->keyAttr.keyState.valtype, pEndpointStatusTemp->keyAttr.keyState.value, pnotifyInfoTemp->data, (uint8_t *)&pnotifyInfoTemp->stateLen);
                                notifyNum++;
                                break;
                            }
//                            pobserver_info->obsDevInfo[count].key
                        }
                    }
                }
                else
                {
                    os_zigbee_log("can't find this device");
                }
            }
            else        // device is not in local zigbee network
            {
                SYN_STATUS_INFO_T info = {0};
                uint32_t addr = 0;
                memcpy((uint8_t *)&addr, pobserver_info->obsDevInfo[count].devAddr, 3);
                sprintf(info.addr, "%02X.%06X", pobserver_info->obsDevInfo[count].subAddr, addr);
                sprintf(info.key, "%d", pobserver_info->obsDevInfo[count].key);

                main_synstatus_deal(pobserver_info->obsDevInfo[count].subAddr, &info);
                os_zigbee_log("panid is %d", pobserver_info->obsDevInfo[count].subAddr);
            }
        }

        uart_observe_state_notify((uint8_t *)&pobserver_info->addr, notifyNum, pnotifyInfo);
        if (pnotifyInfo)
        {
            free(pnotifyInfo);
        }
    }
}

/********************************************************
 * function:  zigbee_psth_response_deal
 * description:
 * input:        1. data:       payload data
 *               2. dataLen     payload length
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_psth_deal( PUART_RECV_PACKET recvPacket )
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
    uint8_t setmeshState = 0;
    uint8_t isCheckOk = 0;

    memset(&devObj, 0, sizeof(DEVICEOBJ_T));
    mlink_generate_subdev_net_addr(pDevHead->srcAddr, devObj.addr);
    sprintf(devObj.modelid, "%02x%02x%02x", (pSubDevHead->mid[2]&0x0f), pSubDevHead->mid[1], pSubDevHead->mid[0]);
    ret = storage_check_devobj(&devObj);
//    os_zigbee_log("shortAddr: %s\r\n modelid is %s, op: %d, devid is %s", devObj.addr, devObj.modelid, pSubDevHead->u_op.op_struct.op, devObj.deviceId);


    mlink_sys_get_status(SYS_MESH_STATE, &setmeshState);
    if (setmeshState == MESH_IN)
    {
        uart_discover_device(pDevHead->srcAddr, 9);
    }

    if (ret == kGeneralErr)           // The device has not been added !!!
    {
        isCheckOk = 0;
        os_zigbee_log("Can't find the device !!!");
//        return;
    }
    else
    {
        isCheckOk = 1;
    }


    if ( pSubDevHead->u_op.op_struct.batch )
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
            case ML_DP_DEV_REPORT:
                {
                    uint8_t stateNum = *payload;
                    uint8_t count = 0;
                    uint8_t pos = 1;
                    uint32_t addr = pDevHead->srcAddr;
                    PKEY_STATE_INFO pKeyState = (PKEY_STATE_INFO)malloc(stateNum * sizeof(KEY_STATE_INFO));
                    PKEY_ATTR_T_EX pDevAttr = (PKEY_ATTR_T_EX)malloc(stateNum * sizeof(KEY_ATTR_T_EX));
                    echoData[0] = echoData[0]+stateNum;         // 批量命令时使用
                    if (index == 0)
                    {
                        echoSize = 1;
                    }

                    memset(pKeyState, 0, stateNum * sizeof(KEY_STATE_INFO));
                    for (count = 0; count < stateNum; count ++)
                    {
                        PKEY_STATE_INFO pKeyStateTemp = pKeyState + count;
                        memcpy(&pKeyStateTemp->keyIndex, payload+pos, 2);
                        memcpy(&pKeyStateTemp->keyType,  payload+pos+2, 2);
                        pKeyStateTemp->stateSize = *(payload+pos+4);
                        memcpy(pKeyStateTemp->keyState, payload+pos+5, *(payload+pos+4));
                        memcpy(echoData + echoSize, &pKeyStateTemp->keyIndex, 2);
                        echoSize += 2;
                        pos = pos + 5 + *(payload+pos+4);
                        if (pKeyStateTemp->keyType == 10)
                        {
                            pKeyStateTemp->keyState[0] = 1;
                        }
                    }
                    uart_send_psth_echo( recvPacket, 0, echoData, echoSize );
                    ret = zigbee_key_attr_logic_transformation(stateNum, pKeyState, pDevAttr);
                    if (ret != kGeneralErr)
                    {
                        if (isCheckOk )//edit by 2018.3.7
                        {
                            main_devstate_deal(devObj.addr, devObj.deviceId, stateNum, pDevAttr);
                        }
                        else        // 未找到设备的时候上报设备状态携带设备地址
                        {
                            main_statechange_report_ex(devObj.addr, stateNum, pDevAttr);
                        }
                    }
                    zigbee_observe_state_notify(0, (uint8_t *)&addr, stateNum, pKeyState);
                    free(pKeyState);
                    free(pDevAttr);
                }
                break;
            case ML_DP_SINGLE_REPORT:
                {
                    KEY_STATE_INFO keyStateInfo = {0};
                    KEY_ATTR_T_EX devAttr = {0};
                    uint32_t addr = pDevHead->srcAddr;
                    memcpy(&keyStateInfo.keyType, payload, 2);
                    memcpy(keyStateInfo.keyState, payload+2, payloadLen-2);
                    keyStateInfo.stateSize = payloadLen-2;
                    keyStateInfo.keyIndex = keyStateInfo.keyType;
                    memcpy(echoData + echoSize, &keyStateInfo.keyIndex, 2);
                    echoSize += 2;
                    uart_send_psth_echo( recvPacket, 0, echoData, echoSize );
                    ret = zigbee_key_attr_logic_transformation(1, &keyStateInfo, &devAttr);
                    if (ret != kGeneralErr)
                    {
                        if (isCheckOk )//edit by 2018.3.7
                        {
                            main_devstate_deal(devObj.addr, devObj.deviceId, 1, &devAttr);
                        }
                        else        // 未找到设备的时候上报设备状态携带设备地址
                        {
                            main_statechange_report_ex(devObj.addr, 1, &devAttr);
                        }
                    }
                    zigbee_observe_state_notify(0, (uint8_t *)&addr, 1, &keyStateInfo);
                }
                break;
            case ML_CALL_SCENE:
            {
                uint16_t sceneIndex = 0;
                char sceneId[10] = {0};
                memcpy(&sceneIndex, payload, 2);
                sprintf(sceneId, "%d", sceneIndex);
                main_scene_ctrl_deal(sceneId);
            }
                break;
            case ML_DP_WRITE_FORWORDED:
            {
                //parse the payload and deal data
                zigbee_write_forworded_deal(payload, payloadLen);
            }
                break;
            case ML_DP_STATE_OBSERVE_DEV:
            {
                SUBNET_DEV_OBSERVER_INFO observerInfo ={0};
                uint8_t count = 0;
                uint8_t pos = 0;
                observerInfo.addr = pDevHead->srcAddr;
                observerInfo.num = *payload;
                pos = 1;
                for (count=0; count<observerInfo.num; count++)
                {
                    observerInfo.obsDevInfo[count].subAddr = *(payload+pos);
                    pos++;
                    memcpy(observerInfo.obsDevInfo[count].devAddr, payload+pos, 3);
                    pos += 3;
                    memcpy(&observerInfo.obsDevInfo[count].key, payload+pos, 2);
                    pos += 2;
                }
                zigbee_update_observer_dev_info(&observerInfo);
                zigbee_observe_dev_deal(&observerInfo);
            }
                break;
            case ML_DP_STATE_OBSERVE_TYPE:
            {
                SUBNET_KEYTYPE_OBSERVER_INFO observerInfo ={0};
                observerInfo.addr = pDevHead->srcAddr;
                observerInfo.num = *payload;
                memcpy(observerInfo.keytype, payload+1, 2*observerInfo.num);
                zigbee_update_observer_keytype_info(&observerInfo);
            }
                break;
            case ML_DP_READ:
            {
                uint16_t dpAddr = 0;
                memcpy((uint8_t *)&dpAddr, payload, 2);
                dpAddr = dpAddr >> 4;
                zigbee_read_distribute(dpAddr, echoData, &echoSize);
                // get echo size
                // get echo data
                uart_send_psth_echo( recvPacket, echoSize, echoData, echoSize );
            }
                break;
            default:
                break;
        }
    }
}

/********************************************************
 * function:  zigbee_devstatus_query
 * description:
 * input:     1. index:  If index equal 0xff, we will query all
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_devstatus_query( uint16_t addr, char *modelid )
{
    if (addr == 0xffff)
    {
        uint8_t count = 0;
        uint32_t devNum = storage_get_object_num(DEVICE_OBJ_ID);
        DEVICEOBJ_T devObj = {0};
        uint32_t daddr = 0;
        for (count = 0; count < devNum; count++)
        {
            storage_read_dev_obj_batch(1, count, &devObj);
            mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, (uint8_t *)&daddr);
            zigbee_set_mid(devObj.modelid);
            uart_query_attrs(&daddr);
        }
    }
    else
    {
        uint8_t daddr[3] = {0};
        uint16_t queryAdd = addr;
        memcpy(daddr, (uint8_t *)&queryAdd, 2);

        if (modelid != NULL)
        {
            zigbee_set_mid(modelid);
        }
        uart_query_attrs(daddr);
    }
    return TRUE;
}

void zigbee_query_timeout_handle( void *arg )
{
    static int8_t count = -3;
    uint32_t devNum = storage_get_device_num();
    DEVICEOBJ_T devObj = {0};
    uint32_t daddr = 0;
    if (++count<0)
        return;
    if (count<devNum)
    {
        storage_read_dev_obj_batch(1, count, &devObj);
        mlink_parse_subdev_net_addr(devObj.comm, devObj.addr, (uint8_t *)&daddr);
        zigbee_devstatus_query(daddr, devObj.modelid);
    }
    else
    {
        count = 0;
        mico_stop_timer(&_zigbee_query_timer);
        mico_deinit_timer( &_zigbee_query_timer );
        _zigbee_query_timer_initialized = false;
        os_zigbee_log("quit query on timer");
    }


}

/********************************************************
 * function:  zigbee_devstatus_query
 * description:
 * input:     1. index:  If index equal 0xff, we will query all
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_devstatus_query_all( void )
{
    if (_zigbee_query_timer_initialized != true)
    {
        mico_init_timer(&_zigbee_query_timer, 1000, zigbee_query_timeout_handle, NULL);
        mico_start_timer(&_zigbee_query_timer);
        _zigbee_query_timer_initialized = true;
    }
}

/********************************************************
 * function:  zigbee_module_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
void zigbee_module_init(void)
{
    uint8_t count = 0;
    uint16_t type = DEV_NODE_UNCONFIG;
    while(type == DEV_NODE_UNCONFIG)        // 如果查询不到设备节点类型则继续查询直到查满三次退出查询
    {
        mico_thread_msleep(100);
        if (++count > 3)
        {
            break;
        }
        zigbee_get_node_type(0x8000, &type);
        if ((type == DEV_NODE_ROUTER) || (type == DEV_NODE_TERMINAL))  // node type is route or terminal
        {
            zigbee_setmesh_start_coordinate(0x8000, 0);
        }
    }

}

/********************************************************
 * function: zigbee_gateway_restore
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void zigbee_gateway_restore(void)
{
    // restore zigbee module and the local zigbee module address is 0x8000
    zigbee_restore_device(0x8000);
    mico_thread_sleep(1);
}

/********************************************************
 * function:  app_main_deal_coap_init
 * description:
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
OSStatus zigbee_logic_init(zigbee_logic_notify_t zigbee_callback)
{
    OSStatus err = kNoErr;
    os_zigbee_log("zigbee_logic_init");

    zigbee_module_init();
    zigbee_observer_list_init();
    uart_config_zigbee_pth_deal(zigbee_psth_deal, zigbee_psth_response_deal);
    zigbee_devstatus_query_all();
//    MicoSystemReboot();

    return err;
}

