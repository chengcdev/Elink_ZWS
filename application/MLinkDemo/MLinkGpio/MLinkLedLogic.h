/*
 * MLinkGpio.h
 *
 *  Created on: 2017年10月16日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKLEDLOGIC_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKLEDLOGIC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "MLinkGpio.h"

#define mlink_led_setmesh_start() mlink_led_display(LED_COLOR_GREEN, LED_CTRL_FLASH)

typedef enum
{
    LED_COLOR_RED           = 1,
    LED_COLOR_GREEN         = 2,
    LED_COLOR_ORANGE        = 3
}LED_COLOR_E;

typedef enum
{
    LED_CTRL_ON                     = 1,
    LED_CTRL_SLOWLY_FLASH           = 2,
    LED_CTRL_FLASH                  = 3,
    LED_CTRL_FLASH_ONCE             = 4,
    LED_CTRL_FLASH_10S              = 5,
    LED_CTRL_ON_3S                  = 6,
    LED_CTRL_SLOWLY_FLASH_3T        = 7,
    LED_CTRL_FLASH_3T               = 8,
    LED_CTRL_LIGHT_OFF_1S            = 9,
    LED_CTRL_OFF                    = 0xff
}LED_DISP_STATE_E;

typedef struct{
    unsigned int ledOldColor;          // LED_COLOR_E
    unsigned int ledCurrColor;
    unsigned int ledOldState;          // LED_DISP_STATE_E
    unsigned int ledCurrState;
}LED_INFO_T, *PLED_INFO_T;


/*************************************************
  Function          :       mlink_led_display
  Description       :   控制灯光状态
  Input:
      DesStr:打印描述
      data:  实际需要打印的字符串
      dataLen:打印字节数
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_display(LED_COLOR_E led_color, LED_DISP_STATE_E led_state);

/*************************************************
  Function          :       mlink_led_net_communication
  Description       :   开启网络通信指示灯
  Input:
      led_color:    灯显示颜色
      ledState:     灯显示样式  （闪烁、常亮、慢闪等）
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_led_net_communication( void );

/*************************************************
  Function          :       mlink_led_submesh_communication
  Description       :   开启子设备通信指示灯
  Input:
      led_color:    灯显示颜色
      ledState:     灯显示样式  （闪烁、常亮、慢闪等）
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_led_submesh_communication( void );

/*************************************************
  Function          :       mlink_led_cloud_connected
  Description       :   灯显示已连上云端
  Input:
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_led_cloud_connected( void );

/*************************************************
  Function          :       mlink_led_easylink_start
  Description       :   开始配置网络
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_easylink_start( void );

/*************************************************
  Function          :       mlink_led_config_recv_ssid
  Description       :   配置网络接收到SSID
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_config_recv_ssid( void );

/*************************************************
  Function          :       mlink_led_network_connected
  Description       :   灯显示已连上网络
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_network_connected( void );

typedef void (*gpio_led_ctrl)(LED_COLOR_E led_color, LED_DISP_STATE_E ledState);



#ifdef __cplusplus
}
#endif


#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKLEDLOGIC_H_ */
