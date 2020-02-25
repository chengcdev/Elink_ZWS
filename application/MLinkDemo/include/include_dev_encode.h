/*
* include_devcode.h
*
*  Created on: 2017年8月5日
*      Author: Administrator
*/

#ifndef DEMOS_APPLICATION_MLINKDEMO_INCLUDE_INCLUDE_DEV_ENCODE_H_
#define DEMOS_APPLICATION_MLINKDEMO_INCLUDE_INCLUDE_DEV_ENCODE_H_

#ifdef __cplusplus
extern "C"{
#endif


// the communication mode of device
typedef enum{
    COMM_NET_WIFI_TYPE = 0,
    COMM_RF433_TYPE,
    COMM_ZIGBEE_TYPE,
    COMM_BLE_TYPE,
}DEVICE_COMM_TYPE_E;

typedef enum{
    DEVICE_UNKNOWN                          = 0,
    DEVICE_RESERVE,
    DEVICE_INTELLIGENT_LOCK,
    DEVICE_GATEWAY,
    DEVICE_SMART_LIGHT,
    DEVICE_REMOTE_CONTROL                   = 8,

    // Lighting
    DEVICE_NON_DIMMABLE_LIGHT               = 20,
    DEVICE_DIMMABLE_LIGHT                   = 21,
    DEVCIE_RGB_LIGHT                        = 22,
    DEVICE_TABUS_AIR                        = 23,
    DEVICE_CURTAIN                          = 24,
    DEVICE_POWER                            = 25,
    DEVICE_GAS                              = 26,
    DEVICE_SCENE                            = 27,
    DEVICE_SOCKET                           = 30,

    // infrared device
    DEVICE_IR_AIR                           = 40,
    DEVICE_IR_TELEVISION                    = 41,       // 电视
    DEVICE_IR_TV_BOX                        = 42,       // 电视盒子
    DEVICE_IR_AMPLIFIER                     = 43,       // 功放
    DEVICE_IR_PROJECTOR                     = 44,       // 投影仪
    DEVICE_IR_BACKGROUND_MUSIC              = 45,

    // Alarm
    DEVICE_EMERGENCY_BUTTON                 = 61,       // 紧急按钮
    DEVICE_DOOR_CONTACT                     = 62,       // 门磁开关
    DEVICE_CURTAIN_CONTACT                  = 63,       // 窗磁开关
    DEVICE_IRPROBE                          = 64,       // 红外探头
    DEVICE_GAS_PROBE                        = 65,       // 瓦斯探头
    DEVICE_WATER_DETECTOR                   = 66,       // 水位探测
    DEVICE_SMOKE_DETECTOR                   = 67,       // 烟感探测

    // environment
    DEVICE_TEMPERATURE_SENSOR               = 81,       // 温度传感器
    DEVICE_HUMIDITY_SENSOR                  = 82,       // 湿度传感器
    DEVICE_FORMALDEHYDE_SENSOR              = 83,       // 甲醛传感器
    DEVICE_VOC_SENSOR                       = 84,       // VOC 传感器
    DEVICE_PM_SENSOR                        = 85,       // PM 传感器
    DEVICE_CO_SENSOR                        = 86,       // CO 传感器
    DEVICE_CO2_SENSOR                       = 87,       // CO2 传感器

    // third party device
    DEVICE_TP_AIR                           = 100,      // 第三方空调
    DEVICE_TP_FRESH_AIR                     = 141,      // 第三方新风
    DEVICE_TP_FLOOR_HEATING                 = 142,      // 第三方地暖
    DEVICE_TP_BK_MUSIC                      = 143,      // 第三方背景音乐

}DEVICE_TYPE_E;

typedef enum{
    BIGTYPE_SMARTHOME                       = 0,
    BIGTYPE_ALARM,
    BIGTYPE_ENVIRONMENT,
    BIGTYPE_PUBLIC_SERVICE,
    BIGTYPE_MEDIA
}DEVICE_BIG_TYPE_E;

typedef enum{
    MIDTYPE_GATEWAY                         = 0,
    MIDTYPE_HOME_ELEC,
    MIDTYPE_LIGHT_CTRL                      = 10,
    MIDTYPE_IR_ELEC,
    MIDTYPE_COMM_SWITCH,
    MIDTYPE_REMOTE_CTRL
}DEVICE_MID_TYPE_E;

typedef enum{
    GATEWAY_RF433                           = 1,
    GATEWAY_ZIGBEE                          = 2
}GATEWAY_TYPE_E;

typedef enum{
    HOME_ELEC_AIR                           = 1,
    HOME_ELEC_FRESH_AIR                     = 2,
    HOME_ELEC_FLOOR_HEATING                 = 3,
    HOME_ELEC_TV                            = 4,
    HOME_ELEC_HEATING                       = 5,
}HOME_ELEC_TYPE_E;

typedef enum{
    LIGHT_CTRL_T_LN_SWITCH                  = 0x011001,       // 2路零火开关模块
    LIGHT_CTRL_T_LN_DIMMABLE                = 0x011002,       // 2路零火调光模块
    LIGHT_CTRL_T_CURTAIN                    = 0x011003,       // 2路窗帘模块

}LIGHT_CTRL_TYPE_E;

#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_INCLUDE_INCLUDE_DEV_ENCODE_H_ */
