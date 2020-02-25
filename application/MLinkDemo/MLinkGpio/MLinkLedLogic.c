/*
 * MLinkLedLogic.c
 *
 *  Created on: 2018年2月11日
 *      Author: Administrator
 */

#include "../MLinkPublic/MLinkPublic.h"
#include "mico_system.h"
#include "MLinkLedLogic.h"
#include "../MLinkAppDef.h"

#define led_log(M, ...) custom_log("GPIO_LOG", M, ##__VA_ARGS__)
#define led_log_trace() custom_log_trace("GPIO_LOG_TRACE")

#define LED_NETWORK                         LED_COLOR_GREEN
#define LED_SYS_START                       LED_COLOR_GREEN
#define LED_STANDBY                         LED_COLOR_GREEN

#if SUCK_TOP_DEVICE
#define LED_ALARM                           LED_COLOR_RED
#endif

#define LED_SUBMESH                     LED_COLOR_RED

#define LEVEL_MAX       3
#define COMMON_LEVEL    1
#define COMMUNICATION_LEVEL 0
#define STATUS_PROMPT_LEVEL 0
#define SYS_LED_TRIGGER_INTERVAL 100
#define FLASH_SLOW_TIMEOUT_COUNT        (500/SYS_LED_TRIGGER_INTERVAL)      // 300ms
#define FLASH_TIMEOUT_COUNT             (100/SYS_LED_TRIGGER_INTERVAL)      // 100ms
#define TIMEOUT_COUNT_10S                  100                              // 10s
#define FLASH_SPACE_10S                     2                               // 200ms
#define TIMEOUT_COUNT_3S                  30                              // 3s
#define TIMEOUT_COUNT_1S                    10                              // 1s


static mico_timer_t _Led_timer;
static mico_mutex_t g_ledInfoMutex;
static mico_mutex_t g_oldLedInfoMutex;
static bool _Led_timer_initialized = false;

static LED_INFO_T g_ledLevel[LEVEL_MAX] = {0};
static uint8_t g_ledCurColor = 0;
static uint8_t g_ledCtrlVal = 0;
//static uint8_t g_ctrlCount = 0;


/*************************************************
  Function          :       mlink_set_led_info
  Description       :
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
static void mlink_set_led_info( LED_COLOR_E led_color, LED_DISP_STATE_E disp_val, uint8_t level )
{
    mico_rtos_lock_mutex(&g_ledInfoMutex);
    g_ledLevel[level].ledCurrState = disp_val;
    g_ledLevel[level].ledCurrColor = led_color;
//    led_log("mlink_set_led_info: [level: %d, led_color: %d, led_state: %d]", level, led_color, disp_val);
    mico_rtos_unlock_mutex(&g_ledInfoMutex);
}

/*************************************************
  Function          :       mlink_get_led_info
  Description       :
  Input:
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
static void mlink_get_led_info( uint8_t level, uint32_t *led_color, uint32_t *disp_val )
{
    mico_rtos_lock_mutex(&g_ledInfoMutex);
    *led_color = g_ledLevel[level].ledCurrColor;
    *disp_val = g_ledLevel[level].ledCurrState;
    mico_rtos_unlock_mutex(&g_ledInfoMutex);
}

/*************************************************
  Function          :       mlink_set_old_led_info
  Description       :   ctrl the led displayed
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
static void mlink_set_old_led_info( LED_COLOR_E led_color, LED_DISP_STATE_E disp_val, uint8_t level )
{
    mico_rtos_lock_mutex(&g_oldLedInfoMutex);
    g_ledLevel[level].ledOldColor = led_color;
    g_ledLevel[level].ledOldState = disp_val;
    led_log("mlink_set_old_led_info: [level: %d, old_color: %d, old_state: %d]", level, led_color, disp_val);
    mico_rtos_unlock_mutex(&g_oldLedInfoMutex);
}

/*************************************************
  Function          :       mlink_get_old_led_info
  Description       :   ctrl the led displayed
  Input:
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
static void mlink_get_old_led_info( uint8_t level, uint32_t *led_color, uint32_t *disp_val )
{
    mico_rtos_lock_mutex(&g_oldLedInfoMutex);
    *led_color = g_ledLevel[level].ledOldColor;
    *disp_val = g_ledLevel[level].ledOldState;
    mico_rtos_unlock_mutex(&g_oldLedInfoMutex);
}

/*************************************************
  Function          :       mlink_led_display_ctrl
  Description       :   ctrl the led displayed
  Input:
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
static void mlink_led_display_ctrl( LED_COLOR_E led_color, LED_CTRL_E ctrl_val )
{
    int ret = 0;
//    led_log("led color: %d, ctrl_val: %d", led_color, ctrl_val);
    switch (led_color)
    {
        case LED_COLOR_RED:
            ret = mico_led_red_ctrl(ctrl_val);
            mico_led_green_ctrl(LED_CLOSE);
            break;
        case LED_COLOR_GREEN:
            ret = mico_led_green_ctrl(ctrl_val);
            mico_led_red_ctrl(LED_CLOSE);
            break;
        case LED_COLOR_ORANGE:
            ret = mico_led_orange_ctrl(ctrl_val);
            break;
        default:
            break;
    }
    if (ret == kNoErr)
    {
        if (ctrl_val != LED_TRIGGER)
        {
            g_ledCtrlVal = ctrl_val;
        }
        else
        {
            g_ledCtrlVal = !g_ledCtrlVal;
        }
        g_ledCurColor = led_color;
    }

}

static void mlink_led_get_color_and_ctrl(uint8_t *color, uint8_t *ctrl_val)
{
    *color = g_ledCurColor;
    *ctrl_val = g_ledCtrlVal;
}

/*************************************************
  Function          :       _led_Timeout_handler
  Description       :   led灯超时处理
  Input:
      DesStr:打印描述
      data:  实际需要打印的字符串
      dataLen:打印字节数
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
static void _led_Timeout_handler( void* arg )
{
    uint8_t levelCount = 0;
    uint32_t colorTemp = 0;
    uint32_t stateTemp = 0;
    static uint32_t sTimeoutCount = 0;
    static uint8_t timeCount = 0;
    sTimeoutCount++;
    for (levelCount=0; levelCount<LEVEL_MAX; levelCount++)
    {
        mlink_get_led_info(levelCount, &colorTemp, &stateTemp);
        if ( stateTemp != 0 )
        {
            uint8_t indicatorColor = 0;
            uint8_t indicatorOnOffState = 0;
            mlink_led_get_color_and_ctrl(&indicatorColor, &indicatorOnOffState);
//            led_log("Timeout_handler: [levelCount: %d, ledOldState: %d, ledCurrState: %d, color: %d]", levelCount, g_ledLevel[levelCount].ledOldState, stateTemp, g_ledLevel[levelCount].ledCurrColor);
            switch(stateTemp)
            {
                case LED_CTRL_ON:
                    if ((indicatorOnOffState != LED_OPEN)||(colorTemp != indicatorColor))
                    {
                        led_log("color: %d, ctrlType: %d", colorTemp, stateTemp);
                        mlink_led_display_ctrl(colorTemp, LED_OPEN);
                    }
                    break;
                case LED_CTRL_OFF:
                    if ((indicatorOnOffState != LED_CLOSE)||(colorTemp != indicatorColor))
                    {
                        mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                    }
                    break;
                case LED_CTRL_FLASH_10S:              // 间隔10s闪烁一次
                {
                    static uint32_t timeOutCount = 0;
                    static uint8_t openTimeCount = 0;
                    if (timeOutCount%TIMEOUT_COUNT_10S == 0)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_OPEN);
                        openTimeCount = 0;
                        led_log("LED_CTRL_FLASH_10S  OPEN");
                    }
                    else if ( openTimeCount == FLASH_SPACE_10S )
                    {
                        mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                        led_log("LED_CTRL_FLASH_10S  CLOSE");
                    }
                    if (openTimeCount != 0xff)
                    {
                        openTimeCount ++;
                    }
                    timeOutCount ++;
                }
                    break;
                case LED_CTRL_FLASH_ONCE:    // 100ms
                {
                    static uint8_t switchTimes = 0;
                    static uint8_t colorRecord = 0;
                    static uint8_t onOffRecord = 0;
//                    led_log("switchTimes is %d", switchTimes);
                    if (switchTimes == 0)
                    {
                        colorRecord = indicatorColor;
                        onOffRecord = indicatorOnOffState;
                        if (indicatorOnOffState == LED_OPEN)
                        {
                            mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                        }
                        else
                        {
                            switchTimes++;
                            mlink_led_display_ctrl(colorTemp, LED_OPEN);
                        }
                    }
                    else if (switchTimes == 1)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_OPEN);
                    }
                    else if (switchTimes == 2)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                    }
                    else if (switchTimes == 3)
                    {
                        mlink_led_display_ctrl(colorRecord, onOffRecord);
                        switchTimes = 0;
                        mlink_set_led_info(0, 0, levelCount);
                    }
                    switchTimes ++;
                }
                    break;
                case LED_CTRL_FLASH_3T:
                {
                    static uint8_t switchTimes = 0;
                    mlink_led_display_ctrl(colorTemp, LED_TRIGGER);
                    if (switchTimes++ == 5)
                    {
                        switchTimes = 0;
                        mlink_set_led_info(0, 0, levelCount);
                        break;
                    }
                }
                    break;
                case LED_CTRL_FLASH:     // 100ms
                    mlink_led_display_ctrl(colorTemp, LED_TRIGGER);
                    break;
                case LED_CTRL_SLOWLY_FLASH:  // 500ms
                    if (sTimeoutCount%FLASH_SLOW_TIMEOUT_COUNT == 0)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_TRIGGER);
                    }
                    break;
                case LED_CTRL_ON_3S:
                {
                    if (timeCount == 0)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_OPEN);
                    }
                    else if (timeCount == TIMEOUT_COUNT_3S)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                        timeCount = 0;
                        mlink_set_led_info(0, 0, levelCount);
                        break;
                    }
                    timeCount++;
                    break;
                }
                case LED_CTRL_SLOWLY_FLASH_3T:      // flash three times
                {
                    static uint8_t switchTimes = 0;
                    if (timeCount%FLASH_SLOW_TIMEOUT_COUNT == 0)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_TRIGGER);
                        if (switchTimes ++ == 5)
                        {
                            switchTimes = 0;
                            timeCount = 0;
                            mlink_set_led_info(0, 0, levelCount);
                            break;
                        }
                    }
                    timeCount++;
                    break;
                }
                case LED_CTRL_LIGHT_OFF_1S:
                {
                    if (timeCount == 0)
                    {
                        mlink_led_display_ctrl(colorTemp, LED_CLOSE);
                    }
                    else if (timeCount == TIMEOUT_COUNT_1S)
                    {
                        timeCount = 0;
                        mlink_set_led_info(0, 0, levelCount);
                        break;
                    }
                    break;
                }
                default:
                    break;
            }

//            if (levelCount+1<LEVEL_MAX)
//            {
//                if (g_ledLevel[levelCount+1].ledCurrState != 0)
//                {
//                    if (g_ledLevel[levelCount+1].ledOldState == g_ledLevel[levelCount+1].ledCurrState)
//                    {
//                        mlink_set_old_led_info(0, 0, levelCount+1);
//                    }
//                }
//            }
            break;
        }
    }
}

/*************************************************
  Function          :       mlink_led_display
  Description       :   灯光显示控制
  Input:
      led_color:    灯显示颜色
      ledState:     灯显示样式  （闪烁、常亮、慢闪等）
  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_led_display(LED_COLOR_E led_color, LED_DISP_STATE_E led_state)
{
    led_log("mlink_led_display: led_color: %d, led_state: %d", led_color, led_state);
    mlink_set_led_info(led_color, led_state, COMMON_LEVEL);
}

///*************************************************
//  Function          :       mlink_led_display_roll_back
//  Description       :   使某一等级的灯光状态恢复到原先的状态
//  Input:
//      level:
//  Output            :       无
//  Return            :
//  Others            :       无
//
//*************************************************/
//void mlink_led_display_roll_back( uint8_t level )
//{
//    uint32_t ledColor = 0;
//    uint32_t dispVal = 0;
//    mlink_get_old_led_info(level, &ledColor, &dispVal);
//    mlink_set_old_led_info(0, 0, level);
//    mlink_set_led_info(ledColor, dispVal, level);
//}

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
void mlink_led_net_communication( void )
{
    led_log("mlink_led_net_communication");

//    mlink_set_led_info(LED_NETWORK, LED_CTRL_FLASH_ONCE, COMMUNICATION_LEVEL);
}

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
void mlink_led_submesh_communication( void )
{
    led_log("mlink_led_submesh_communication");

//    mlink_set_led_info(LED_SUBMESH, LED_CTRL_FLASH_ONCE, COMMUNICATION_LEVEL);
}

/*************************************************
  Function          :       mlink_led_upgrade
  Description       :   指示升级状态
  Input:
  Output            :       无
  Return            :
  Others            :       绿灯常亮

*************************************************/
void mlink_led_upgrade( void )
{
    led_log("mlink_led_upgrade");
    mlink_led_display(LED_NETWORK, LED_CTRL_ON);
}

/*************************************************
  Function          :       mlink_led_mesh_join
  Description       :   RF433设备加入组网
  Input:
  Output            :       无
  Return            :
  Others            :       绿灯灭

*************************************************/
void mlink_led_mesh_join( void )
{
    mlink_set_led_info(LED_NETWORK, LED_CTRL_LIGHT_OFF_1S, STATUS_PROMPT_LEVEL);
}

/*************************************************
  Function          :       mlink_led_cloud_connected
  Description       :   灯显示已连上云端
  Input:
  Output            :       无
  Return            :
  Others            :       常亮3秒
*************************************************/
void mlink_led_cloud_connected( void )
{
    /* Green light slowly flash*/
    led_log("mlink_led_cloud_connected");
    mlink_set_led_info(LED_NETWORK, LED_CTRL_FLASH_ONCE, STATUS_PROMPT_LEVEL);
}

/*************************************************
  Function          :       mlink_led_network_connected
  Description       :   灯显示已连上网络
  Input:
  Output            :       无
  Return            :
  Others            :       绿灯闪烁3次
*************************************************/
void mlink_led_network_connected( void )
{
    /* open orange led */
    led_log("mlink_led_network_connected");
        mlink_set_led_info(LED_NETWORK, LED_CTRL_SLOWLY_FLASH_3T, STATUS_PROMPT_LEVEL);
}

/*************************************************
  Function          :       mlink_led_network_unconnected
  Description       :   灯显示未连上网络
  Input:
  Output            :       无
  Return            :
  Others            :       绿灯慢闪三次
*************************************************/
void mlink_led_network_unconnected( void )
{
    /* open orange led */
    led_log("mlink_led_network_unconnected");
    mlink_set_led_info(LED_NETWORK, LED_CTRL_FLASH_3T, STATUS_PROMPT_LEVEL);
}

/*************************************************
  Function          :       mlink_led_wireless_networking
  Description       :   灯显示多种无线方式进入组网
  Input:
  Output            :       无
  Return            :
  Others            :       包括wifi和zigbee组网
*************************************************/
void mlink_led_wireless_networking()
{
    led_log("mlink_led_wireless_networking");
//    mlink_led_display(LED_COLOR_GREEN, LED_CTRL_FLASH);
}

/*************************************************
  Function          :       mlink_led_easylink_start
  Description       :   开始配置网络
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_easylink_start( void )
{
    /* open orange led */
    led_log("mlink_led_easylink_start");
    mlink_led_display(LED_NETWORK, LED_CTRL_FLASH);
}

/*************************************************
  Function          :       mlink_led_bindlock_start
  Description       :   开始绑定指纹锁
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_bindlock_start( void )
{
    /* open orange led */
    led_log("mlink_led_bindlock_start");
    mlink_led_display(LED_NETWORK, LED_CTRL_SLOWLY_FLASH);
}
///*************************************************
//  Function          :       mlink_led_net_config_stop
//  Description       :   停止配置网络
//  Input:
//  Output            :       无
//  Return            :
//  Others            :       无
//*************************************************/
//void mlink_led_net_config_stop( void )
//{
//    /* open orange led */
//    led_log("mlink_led_net_config_stop");
//    mlink_led_display_roll_back( COMMON_LEVEL );
//}

/*************************************************
  Function          :       mlink_led_config_recv_ssid
  Description       :   配置网络接收到SSID
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_config_recv_ssid( void )
{
    /* open orange led */
    led_log("mlink_led_config_recv_ssid");
//    mlink_led_display(LED_NETWORK, LED_CTRL_SLOWLY_FLASH);
}

/*************************************************
  Function          :       mlink_led_standby
  Description       :   灯显示设备待机状态
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_standby( void )
{
    /* close led */
    mlink_led_display(LED_STANDBY, LED_CTRL_OFF);
}

#if SUCK_TOP_DEVICE
/*************************************************
  Function          :       mlink_led_alarm
  Description       :   安防报警指示灯
  Input:
  Output            :       无
  Return            :
  Others            :       无
*************************************************/
void mlink_led_alarm( void )
{
    /* close led */
    mlink_led_display(LED_ALARM, LED_CTRL_SLOWLY_FLASH);
}
#endif

/*************************************************
  Function          :       mlink_led_logic_init
  Description       :
  Input:

  Output            :       无
  Return            :
  Others            :       无

*************************************************/
void mlink_led_logic_init(void)
{
    if(_Led_timer_initialized == true)
    {
      mico_stop_timer(&_Led_timer);
      mico_deinit_timer( &_Led_timer );
      _Led_timer_initialized = false;
    }

    mico_init_timer(&_Led_timer, SYS_LED_TRIGGER_INTERVAL, _led_Timeout_handler, NULL);
    mico_start_timer(&_Led_timer);
    _Led_timer_initialized = true;

    mico_rtos_init_mutex(&g_ledInfoMutex);
    mico_rtos_init_mutex(&g_oldLedInfoMutex);
    MLink_init_led_ctrl_callback(mlink_led_display);
}

