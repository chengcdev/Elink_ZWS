/*
 * MLinkGpio.h
 *
 *  Created on: 2017年10月16日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKGPIO_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKGPIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "mico_platform.h"

typedef enum{
    LED_CLOSE       = 0,
    LED_OPEN        = 1,
    LED_TRIGGER     = 0xff
}LED_CTRL_E;

/*************************************************
  Function          :   mico_led_red_ctrl
  Description       :   控制红灯
  Input:
      flag:   1. 开       0.关
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
OSStatus mico_led_red_ctrl( unsigned char flag );

/*************************************************
  Function          :   mico_led_green_ctrl
  Description       :   控制绿灯
  Input:
      flag:   1. 开       0.关
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
OSStatus mico_led_green_ctrl( unsigned char flag );

/*************************************************
  Function          :   mico_led_orange_ctrl
  Description       :   控制橙灯
  Input:
      flag:   1. 开       0.关
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
OSStatus mico_led_orange_ctrl( unsigned char flag );



#ifdef __cplusplus
}
#endif


#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKGPIO_MLINKGPIO_H_ */
