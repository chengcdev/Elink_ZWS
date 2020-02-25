/*
 * MLinkGpio.c
 *
 *  Created on: 2017年10月16日
 *      Author: Administrator
 */

#include "mico_platform.h"
#include "MLinkGpio.h"
#include "../MLinkPublic/MLinkPublic.h"
#include "mico_system.h"
#include "../MLinkAppDef.h"

#define gpio_log(M, ...) custom_log("GPIO_LOG", M, ##__VA_ARGS__)
#define gpio_log_trace() custom_log_trace("GPIO_LOG_TRACE")


static uint8_t g_redLedState = LED_CLOSE;
static uint8_t g_greenLedState = LED_CLOSE;

//void mico_led_state_val_print( void )
//{
//    custom_log("GPIO_LOG", "red val: %d, green val: %d", g_redLedState, g_greenLedState);
//}


/*************************************************
  Function          :   mico_led_red_ctrl
  Description       :   控制红灯
  Input:
      flag:   1. 开       0.关   0xff.开关状态取反
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
OSStatus mico_led_red_ctrl( LED_CTRL_E flag )
{
//    gpio_log("g_redLedState: %d, flag: %d", g_redLedState, flag);
    if (flag == g_redLedState)
    {
        return kNoErr;
    }
    if (flag == LED_OPEN)
    {
        g_redLedState = LED_OPEN;
        return led_open(MICO_RED_LED);
    }
    else if (flag == LED_CLOSE)
    {
        g_redLedState = LED_CLOSE;
        return led_close(MICO_RED_LED);
    }
    else
    {
        g_redLedState = !g_redLedState;
        return MicoGpioOutputTrigger(MICO_RED_LED);
    }
}

/*************************************************
  Function          :   mico_led_green_ctrl
  Description       :   控制绿灯
  Input:
      flag:   1. 开       0.关
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
OSStatus mico_led_green_ctrl( unsigned char flag )
{
//    gpio_log("g_greenLedState: %d, flag: %d", g_greenLedState, flag);
    if (flag == g_greenLedState)
    {
        return kNoErr;
    }
    if (flag == LED_OPEN)
    {
        g_greenLedState = LED_OPEN;
//        gpio_log("red state is %d, green: %d", g_redLedState, g_greenLedState);
        return led_open(MICO_GREEN_LED);
    }
    else if (flag == LED_CLOSE)
    {
        g_greenLedState = LED_CLOSE;
        return led_close(MICO_GREEN_LED);
    }
    else
    {
        g_greenLedState = !g_greenLedState;
        return MicoGpioOutputTrigger(MICO_GREEN_LED);
    }
}

/*************************************************
  Function          :   mico_led_orange_ctrl
  Description       :   控制橙灯
  Input:
      flag:   1. 开       0.关
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
OSStatus mico_led_orange_ctrl( unsigned char flag )
{
//    gpio_log("g_greenLedState: %d, g_redLedState:%d,  flag: %d", g_greenLedState, g_redLedState, flag);
    if (flag != 0xff)
    {
        mico_led_green_ctrl(flag);
        mico_led_red_ctrl(flag);
    }
    else
    {
        if (g_redLedState == g_greenLedState)
        {
            mico_led_green_ctrl(flag);
            mico_led_red_ctrl(flag);
        }
        else
        {
            mico_led_green_ctrl(LED_CLOSE);
            mico_led_red_ctrl(LED_CLOSE);
        }
    }
}

/*************************************************
  Function          :   mico_led_close
  Description       :   关闭指示灯
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
OSStatus mico_led_close( void )
{
    mico_led_green_ctrl(LED_CLOSE);
    mico_led_red_ctrl(LED_CLOSE);
}

/*************************************************
  Function          :       Mlgpio_beep_on
  Description       :   开启蜂鸣器
  Input:
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_gpio_beep_on(void)
{
    beep_on(MICO_GPIO_BEEP);
}


/*************************************************
  Function          :       Mlgpio_beep_off
  Description       :   开启蜂鸣器
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_gpio_beep_off(void)
{
    beep_off(MICO_GPIO_BEEP);
}

/*************************************************
  Function          :       mlink_gpio_beep_trigger
  Description       :   开启蜂鸣器
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_gpio_beep_trigger(void)
{
    MicoGpioOutputTrigger(MICO_GPIO_BEEP);
}

/*************************************************
  Function          :       mlink_gpio_beep_start
  Description       :   开启蜂鸣器
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_gpio_beep_start(uint8_t cnt)
{
    for(int i = 0;i < cnt;i++)
    {
        mlink_gpio_beep_on();
        msleep( 500 );
        mlink_gpio_beep_off();
        msleep( 500 );
    }

}
/*************************************************
  Function          :       mlink_gpio_init
  Description       :   初始化gpio口
  Input:
      DesStr:打印描述
      data:  实际需要打印的字符串
      dataLen:打印字节数
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_gpio_init(void)
{
    MicoGpioInitialize( MICO_GREEN_LED,  OUTPUT_OPEN_DRAIN_PULL_UP);

#ifdef BLE_DEVICE
    MicoGpioInitialize(MICO_MK_WK,  OUTPUT_OPEN_DRAIN_PULL_UP);
#endif

#if SUCK_TOP_DEVICE      // 吸顶式网关 IO special configure
    MicoGpioInitialize(MICO_GPIO_ETH_RST,  OUTPUT_OPEN_DRAIN_PULL_UP);      //RST
    MicoGpioOutputLow(MICO_GPIO_ETH_RST);

    MicoGpioInitialize(MICO_GPIO_CS,  OUTPUT_OPEN_DRAIN_PULL_UP);      //CS
    MicoGpioOutputHigh(MICO_GPIO_CS);

    MicoGpioInitialize(MICO_GPIO_POWER,  OUTPUT_OPEN_DRAIN_PULL_UP);      //W5500 PWR
    MicoGpioOutputHigh(MICO_GPIO_POWER);
#else
    MicoGpioInitialize(MICO_GPIO_BEEP, OUTPUT_OPEN_DRAIN_PULL_UP);
    beep_off(MICO_GPIO_BEEP);
#endif
    led_close(MICO_GREEN_LED);
    led_close(MICO_RED_LED);

    mlink_led_logic_init();
}

