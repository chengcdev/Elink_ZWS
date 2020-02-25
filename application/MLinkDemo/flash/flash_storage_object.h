/*
 * flash_object_storage.h
 *
 *  Created on: 2017年7月28日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_STORAGE_OBJECT_H_
#define DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_STORAGE_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "MLinkObject.h"



#define DEVICE_NUM              1//32
#define SCENE_NUM               32
#define ROOM_NUM                16
#define EVENT_NUM               20      // modify the num of event from 50 to 20
#define SCHEDULE_NUM            50
#define LOOPACTION_NUM          20
#define ENDPOINT_NUM            DEVICE_NUM
#define ENDPOINTLINKOBJ_NUM     128
#define BLOCK_SIZE              4096

/*in partition MICO_PARTITION_PARAMETER_1 and MICO_PARTITION_PARAMETER_2*/
/*notice that the area of MICO_PARTITION_PARAMETER_2 is the backup of MICO_PARTITION_PARAMETER_1*/
/* flash of paramer storage */
#define FLASH_PARAM_START               BLOCK_SIZE

#define FLASH_PARAM_NETDEV_OFFSET       (FLASH_PARAM_START)
#define FLASH_PARAM_NETDEV_SIZE         (1024)

#define FLASH_PARAM_CLOUDPARAM_OFFSET   (FLASH_PARAM_NETDEV_OFFSET + FLASH_PARAM_NETDEV_SIZE)
#define FLASH_PARAM_CLOUDPARAM_SIZE     (1024)

#define FLASH_PARAM_DEVINFO_OFFSET      (FLASH_PARAM_CLOUDPARAM_OFFSET + FLASH_PARAM_CLOUDPARAM_SIZE)
#define FLASH_PARAM_DEVINFO_SIZE        (1024)


//#define FLASH_USER_DEVICE_OFFSET       (FLASH_PARAM_START + BLOCK_SIZE)
//#define FLASH_USER_DEVICE_SIZE         (BLOCK_SIZE)

#define FLASH_PARAM_TOTAL_SIZE          (BLOCK_SIZE*2)

/****in partition MICO_PARTITION_USER******/

#define FLASH_USER_FIRM_INFO            0
#define FLASH_USER_START                BLOCK_SIZE

#if 0
#define FLASH_USER_DEVICE_OFFSET       (FLASH_USER_START)
#define FLASH_USER_DEVICE_SIZE         (BLOCK_SIZE)    // 4K

#define FLASH_USER_SCENE_OFFSET        (FLASH_USER_DEVICE_OFFSET + FLASH_USER_DEVICE_SIZE)
#define FLASH_USER_SCENE_SIZE          (BLOCK_SIZE*1) //4K

#define FLASH_USER_ROOM_OFFSET          (FLASH_USER_SCENE_OFFSET + FLASH_USER_SCENE_SIZE)
#define FLASH_USER_ROOM_SIZE            (BLOCK_SIZE*4) //16K

#else

#define FLASH_USER_DEVICE_OFFSET       (FLASH_USER_START)
#define FLASH_USER_DEVICE_SIZE         (BLOCK_SIZE*2)    // 8K

#define FLASH_USER_SCENE_OFFSET        (FLASH_USER_DEVICE_OFFSET + FLASH_USER_DEVICE_SIZE)
#define FLASH_USER_SCENE_SIZE          (BLOCK_SIZE*1) //4K

#define FLASH_USER_ROOM_OFFSET          (FLASH_USER_SCENE_OFFSET + FLASH_USER_SCENE_SIZE)
#define FLASH_USER_ROOM_SIZE            (BLOCK_SIZE*3) //12K

#endif

#define FLASH_USER_ALARM_OFFSET        (FLASH_USER_ROOM_OFFSET + FLASH_USER_ROOM_SIZE)
#define FLASH_USER_ALARM_SIZE          (BLOCK_SIZE*1) //4K


#define FLASH_USER_EVENT_OFFSET         (FLASH_USER_ALARM_OFFSET + FLASH_USER_ALARM_SIZE)
#define FLASH_USER_EVENT_SIZE           (BLOCK_SIZE) //4K

#define FLASH_USER_SCHEDULE_OFFSET      (FLASH_USER_EVENT_OFFSET + FLASH_USER_EVENT_SIZE)
#define FLASH_USER_SCHEDULE_SIZE        (BLOCK_SIZE) //4K

#define FLASH_USER_LOOPACTION_OFFSET    (FLASH_USER_SCHEDULE_OFFSET + FLASH_USER_SCHEDULE_SIZE)
#define FLASH_USER_LOOPACTION_SIZE      (BLOCK_SIZE) //4K

#define FLASH_USER_ENDPOINT_OFFSET      (FLASH_USER_LOOPACTION_OFFSET + FLASH_USER_LOOPACTION_SIZE)
#define FLASH_USER_ENDPOINT_SIZE        (BLOCK_SIZE*15) // 60K

#define FLASH_USER_ENDPOINTLINK_OFFSET  (FLASH_USER_ENDPOINT_OFFSET + FLASH_USER_ENDPOINT_SIZE)
#define FLASH_USER_ENDPOINTLINK_SIZE    (BLOCK_SIZE*12) // 48k

#define FLASH_USER_LINKAGE_INPUT_OFFSET  (FLASH_USER_ENDPOINTLINK_OFFSET + FLASH_USER_ENDPOINTLINK_SIZE)
#define FLASH_USER_LINKAGE_INPUT_SIZE     (BLOCK_SIZE*10) //40K

#define FLASH_USER_LINKAGE_OUTPUT_OFFSET  (FLASH_USER_LINKAGE_INPUT_OFFSET + FLASH_USER_LINKAGE_INPUT_SIZE)
#define FLASH_USER_LINKAGE_OUTPUT_SIZE    (BLOCK_SIZE*10) //40K

#define FLASH_USER_TOTAL_SIZE           (FLASH_USER_LINKAGE_OUTPUT_OFFSET+FLASH_USER_LINKAGE_OUTPUT_SIZE-FLASH_USER_START)



typedef enum{
    OBJECT_UPDATE_ADD,
    OBJECT_UPDATE_MODIFY,
    OBJECT_UPDATE_DEL
}OBJECT_UPDATE_TYPE_E;

typedef enum{
    FLASH_OPERATE_OK,
    FLASH_OBJECT_EXIST,
    FLASH_NO_OBJECT,
    FLASH_OPERATE_FAIL
}FLASH_STORAGE_STATUS_E;

OSStatus storage_check_local_obj(void);

OSStatus storage_write_local_obj(PNETDEVOBJ_T pNetDevObj);

OSStatus storage_read_local_devobj(PNETDEVOBJ_T pNetDevObj);



/******************************** USER STORAGE MAP ************************************
    |=============================================================================|
    | OBJECT                                   | NUM       | USED      | TOTAL    |
    |=============================================================================|
    | version infomation                       |           |           | 4KB      |
    | device                                   | 32        | 144Byte   | 8KB      |
    | scene                                    | 32        | 64Byte    | 4KB      |
    | room area                                | 32        |           | 12KB     |
    | alarm info/system backups block          | 32        |           | 4KB      |
    | event                                    |           | 200Byte   | 4KB      |
    | schedule                                 |           | 92Byte    | 4KB      |
    | loopaciton/special scene                 |           | 156Byte   | 4KB      |
    | endpoint                                 | 32        | 2068Byte  | 60KB     |
    | endpoint link                            |           | 200Byte   | 48KB     |
    | linkage input                            | 32        | 1104Byte  | 40KB     |
    | linkage output                           | 32        | 1108Byte  | 40KB     |
    |=============================================================================|
    | TOTAL (bytes)                            |           | 232KB     | 232KB    |
    |=============================================================================|
**************************************************************************************/



#ifdef __cplusplus
}
#endif



#endif /* DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_STORAGE_OBJECT_H_ */
