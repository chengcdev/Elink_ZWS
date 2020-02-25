/**
******************************************************************************
* @file    platform.c
* @author  William Xu
* @version V1.0.0
* @date    05-Oct-2016
* @brief   This file provides all MICO Peripherals mapping table and platform
*          specific functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2016 MXCHIP Inc.
*
*  Permission is hereby gra nted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/

#include "common.h"
#include "mico_platform.h"

#include "platform.h"
#include "button.h"
#include "moc_api.h"
#include "../../../../application/MLinkDemo/MLinkGpio/MLinkLedLogic.h"

#ifdef USE_MiCOKit_EXT
#include "MiCOKit_EXT/micokit_ext.h"
#endif
#define MLINK_DEV_ENABLE         1
#define platform_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
extern WEAK void PlatformEasyLinkButtonClickedCallback(void);
extern WEAK void PlatformEasyLinkButtonLongPressedCallback(void);
extern void PlatformEasyLinkButtonMultClickedCallback(void *arg);

typedef void (*multi_click_dist_cb)( int arg );

/******************************************************
*               Variables Definitions
******************************************************/
const mico_gpio_init_t gpio_init[] =
{
  {MICO_GPIO_12, INPUT_PULL_UP, 0},
  {MICO_GPIO_13, INPUT_PULL_UP, 0},
  {MICO_GPIO_14, OUTPUT_OPEN_DRAIN_PULL_UP, 0},
  {MICO_GPIO_NC, 0, 0}
};

const mico_pwm_pinmap_t pwm_pinmap[] = 
{
  [MICO_PWM_1] = {.pin = MICO_GPIO_1, },
  [MICO_PWM_2] = {.pin = MICO_GPIO_2, },
  [MICO_PWM_3] = {.pin = MICO_GPIO_12,},
  [MICO_PWM_4] = {.pin = MICO_GPIO_13,},
  [MICO_PWM_5] = {.pin = MICO_GPIO_14,},
  [MICO_PWM_6] = {.pin = MICO_GPIO_7, },
};

const mico_spi_pinmap_t spi_pinmap[] =
{ 
  [MICO_SPI_1]  =
  {
    .mosi = MICO_GPIO_9,
    .miso = MICO_GPIO_7,
    .sclk = MICO_GPIO_10,
    .ssel = MICO_GPIO_8,
  },  
};

const mico_uart_pinmap_t uart_pinmap[] =
{
  [MICO_UART_1] =
  {
    .tx   = MICO_GPIO_9,
    .rx   = MICO_GPIO_10,
    .rts  = MICO_GPIO_7,
    .cts  = MICO_GPIO_8, 
  },
  [MICO_UART_2] =
  {
    .tx   = MICO_GPIO_21,
    .rx   = MICO_GPIO_22,
    .rts  = MICO_GPIO_NONE,
    .cts  = MICO_GPIO_NONE, 
  },
};

const mico_i2c_pinmap_t i2c_pinmap[] =
{
  [MICO_I2C_1] =
  {
    .sda = MICO_GPIO_8,
    .scl = MICO_GPIO_7,
  },
  [MICO_I2C_2] =
  {
    .sda = MICO_GPIO_9,
    .scl = MICO_GPIO_10,
  },  
};

const platform_peripherals_pinmap_t peripherals_pinmap = 
{
  .pwm_pinmap   = pwm_pinmap,
  .spi_pinmap   = spi_pinmap,
  .uart_pinmap  = uart_pinmap,
  .i2c_pinmap   = i2c_pinmap, 
};
#ifdef MLINK_DEV_ENABLE
#define ZIGBEE_PRESS_TIMES         3
static  gpio_led_ctrl dev_led_ctrl = NULL;
multi_click_dist_cb button_distr = NULL;
#endif
#define BUTTON_PRESS_TIMES          3
#define BUTTON_PRESS_TIMES_MAX       5

/******************************************************
*               Function Definitions
******************************************************/
void mlink_button_multi_click_distribute( int button_trigger_times )
{
//    platform_log("button trigger times is %d", button_trigger_times);
    if (button_distr)
        button_distr(button_trigger_times);
}


void init_platform( void )
{
#if defined (MOC) && (MOC == 1)
    button_init_t init;
    extern int get_last_reset_reason(void);

#if 0
    if ( get_last_reset_reason() & LAST_RST_CAUSE_WDT )
    {
       char *msg = malloc(1024);
       platform_log( "WARNING: Watchdog reset occured previously." );
       platform_log("msg return %p", msg);
       if (msg != NULL) {
            
            if (hardfault_get(msg, 1024) > 0) {
                platform_log("%s", msg);
            } else {
                platform_log("get 0");
            }
            free(msg);
       }
    }
#else
    int reason = get_last_reset_reason();
    if ( reason & LAST_RST_CAUSE_WDT )
    {
       char *msg = malloc(1024);
       platform_log( "WARNING: Watchdog reset occured previously." );
       platform_log("msg return %p", msg);
       if (msg != NULL) {
            
            if (hardfault_get(msg, 1024) > 0) {
                platform_log("%s", msg);
            } else {
                platform_log("get 0");
            }
            free(msg);
       }
    }
    else if ( reason & LAST_RST_CAUSE_VBAT )
    {
        platform_log( "WARNING: VBAT reset occured previously." );
    }
    else
    {
        platform_log("reset reason value is %d", reason);
    }
#endif

    MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
    led_close( (mico_gpio_t)MICO_SYS_LED );

    //	Initialise EasyLink buttons
    init.gpio = EasyLink_BUTTON;
    init.pressed_func = PlatformEasyLinkButtonClickedCallback;
    init.long_pressed_func = PlatformEasyLinkButtonLongPressedCallback;
    init.long_pressed_timeout = RestoreDefault_TimeOut;

    init.muti_timeout = 2000;
    init.muti_pressed_func = PlatformEasyLinkButtonMultClickedCallback;
    init.muti_button_times_ex = BUTTON_PRESS_TIMES_MAX;

    mico_system_multi_click_cb(mlink_button_multi_click_distribute);
    button_init( IOBUTTON_EASYLINK, init );

#endif
}
#ifdef MLINK_DEV_ENABLE


void MicoSysLed(bool onoff)
{
  if (onoff) {
      if (dev_led_ctrl)
          dev_led_ctrl(LED_COLOR_GREEN, LED_CTRL_FLASH_ONCE);
  } else {
      if (dev_led_ctrl)
          dev_led_ctrl(LED_COLOR_GREEN, LED_CTRL_OFF);
  }
}

#else

void MicoSysLed(bool onoff)
{
  if (onoff) {
    MicoGpioOutputLow( (mico_gpio_t)MICO_SYS_LED );
  } else {
    MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
  }
}
#endif

void MicoRfLed(bool onoff)
{
  if (onoff) {
    MicoGpioOutputLow( (mico_gpio_t)MICO_RF_LED );
  } else {
    MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
  }
}


#ifdef USE_MiCOKit_EXT
// add test mode for MiCOKit-EXT board,check Arduino_D5 pin when system startup
bool MicoExtShouldEnterTestMode(void)
{
  if( MicoGpioInputGet((mico_gpio_t)Arduino_D5)==false ){
    return true;
  }
  else{
    return false;
  }
}
#endif

#ifdef MLINK_DEV_ENABLE
void MLink_init_led_ctrl_callback(gpio_led_ctrl led_ctrl_callback)
{
    if (led_ctrl_callback)
    {
        dev_led_ctrl = led_ctrl_callback;
    }
}

void mlink_multi_click_dist_init(multi_click_dist_cb button_distribute)
{
    button_distr = button_distribute;
}

#endif
