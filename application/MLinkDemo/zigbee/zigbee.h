
#ifndef _ML_ZIGBEE_H_
#define _ML_ZIGBEE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ATTR_DATA_MAX       64
#define SETMESH_TIMEOUT         300//30*60       // 默认进入组网超时时间为30min
#define OBSERVE_SUBDEV_MAX    20

typedef struct{
    unsigned int addr;
    char objid[8];
    char modelid[8];
    int limit;
    int page;
}DEVSET_SIP_PARAM_T,*PDEVSET_SIP_PARAM_T;


typedef struct
{
    uint16_t groupFlag :1;
    uint16_t addr :15;
    unsigned char panid;
    unsigned char reserve;
}MLINK_SUBDEV_ADDR, *PMLINK_SUBDEV_ADDR;

typedef struct{
    uint8_t subAddr;
    uint8_t devAddr[3];
    uint16_t key;
}OBSERVE_DEV_INFO, *POBSERVE_DEV_INFO;

typedef struct{
    uint32_t addr;          // it's the only attribute to identify observer
    uint32_t num;
    OBSERVE_DEV_INFO obsDevInfo[OBSERVE_SUBDEV_MAX];
}SUBNET_DEV_OBSERVER_INFO, *PSUBNET_DEV_OBSERVER_INFO;

typedef struct{
    uint32_t addr;          // it's the only attribute to identify observer
    uint32_t num;
    uint16_t keytype[64];
}SUBNET_KEYTYPE_OBSERVER_INFO, *PSUBNET_KEYTYPE_OBSERVER_INFO;

typedef int (*zigbee_logic_notify_t)(int fincId, char *data, int size);
//typedef int (*zigbee_state_deal_t)();

/********************************************************
 * function: zigbee_gateway_restore
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
void zigbee_gateway_restore(void);

/********************************************************
 * function: zigbee_logic_init
 * description:
 * input:
 * output:
 * return:   kNoErr/kGeneralErr
 * auther:
 * other:
*********************************************************/
OSStatus zigbee_logic_init(zigbee_logic_notify_t zigbee_callback);


#ifdef __cplusplus
}
#endif

#endif /* _ML_ZIGBEE_H_ */
