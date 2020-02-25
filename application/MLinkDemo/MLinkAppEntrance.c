/**
******************************************************************************
* @file    MLinkAppEntrace.c

* @author  huangxf
* @version V1.0.0
* @date    2017年5月23日
* @brief   This file provides xxx functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2017  Inc.
*
******************************************************************************
*/ 

#include "coap/ml_coap.h"
#include "MLinkAppDef.h"
#include "mico.h"
#include "MLinkGpio/MLinkGpio.h"
#include "StringUtils.h"
#ifdef BLE_DEVICE
#include "../ble/bluetooth_logic.h"
#endif
#define USER_PARAM_TOTAL_SIZE (4096*3)
#define USER_PARTITION_SIZE    (0x3a000)

static mico_semaphore_t wait_sem = NULL;
static uint8_t is_restore_default = 0;
extern void zigbee_gateway_restore(void);
extern void main_multi_click_distribute( int button_times );
extern int main_ble_callback_proc( BLE_NOTIFY_E state, char * data, int size );
uint8_t userDataRestoreDefaultCheck( void )
{
    DEVINFOOBJ_T devVerInfo = {0};
    storage_read_devinfo_obj(&devVerInfo);
    if (devVerInfo.ver[0] == (char)0xff || devVerInfo.ver[0] == 0)
    {
        storage_init_local_ver_info();
    }
    else if (0 > strcmp(devVerInfo.ver, SOFT_VER))
    {
        storage_init_local_ver_info();
    }
    return is_restore_default;
}

void appRestoreDefault( void )
{
    is_restore_default = 1;
}
/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback( void * const user_config_data, uint32_t size )
{
    app_log("appRestoreDefault_callback: size[%d]\n",size);
    appRestoreDefault();
//    application_config_t* appConfig = user_config_data;

// we will restore through this function
    UNUSED_PARAMETER( size );

}


void userDataRestoreDefault( app_context_t * const user_config_data )
{
    application_config_t* appConfig = user_config_data;

    if (appConfig->netdev.uuid[0] == (char)0xff)
    {
        // 恢复出厂版本信息应该保持不变

        mico_Context_t* mainContext=  mico_system_context_get();
        storage_factory_flash();
        storage_init_local_dev_info(&appConfig->netdev);
        storage_init_local_ver_info( );
        storage_init_event_obj();
        storage_init_scene_obj();
        memset(&appConfig->alarmRecord, 0, sizeof(ALARM_RECORD_T));
        memset(&appConfig->netdevEndpoint, 0, sizeof(LOCAL_ENDPOINT_T));
        mico_system_context_update(mainContext);
    }
}

/* EasyLink callback: Notify wlan configuration type */
USED void mico_system_delegate_config_success( mico_config_source_t source )
{
    app_log( "Configured by %d", source );
    mlink_led_standby();
}

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    app_log("WiFiEvent status is %d", status);
    mlink_sys_set_status(SYS_WIFI_STATE, status);
//    app_context->appStatus.run_status.wlanStatus = status
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            mico_rtos_set_semaphore( &wait_sem );
            mlink_led_standby();

            break;
        case NOTIFY_STATION_DOWN:
        case NOTIFY_AP_UP:
            break;
        case NOTIFY_AP_DOWN:
            break;
    }
}

/*************************************************
  Function          :       mico_system_delegate_config_will_start
  Description       :
  Input:
  Output            :
  Return            :
  Others            :

*************************************************/
USED void mico_system_delegate_config_will_start( void )
{
    /*Led trigger*/
    mlink_sys_set_status(SYS_EASYLINK_STATE, RESTART_EASYLINK);

    return;
}

USED void mico_system_delegate_config_will_stop( void )
{
    app_log("mico_system_delegate_config_will_stop");

    mlink_sys_set_status(SYS_EASYLINK_STATE, EXIT_EASYLINK);

  return;
}

USED void mico_system_delegate_config_recv_ssid ( char *ssid, char *key )
{
  UNUSED_PARAMETER(ssid);
  UNUSED_PARAMETER(key);

  mlink_led_config_recv_ssid();
  return;
}

static void init_appconfig_user_data(application_config_t *appConfig)
{
//    memset(appConfig->maindev.devid,0,sizeof(appConfig->maindev.devid));
//    appConfig->maindev.devtype =0;
#if 0
    mico_system_status_wlan_t *wlan_status;
    mico_system_get_status_wlan( &wlan_status );

app_log("init_appconfig_user_data: mac[%s]\n",appConfig->maindev.mac);

	if (strcmp(appConfig->maindev.mac,"")==0 )
		snprintf( appConfig->maindev.mac, 32, "%c%c%c%c%c%c",
              wlan_status->mac[9],  wlan_status->mac[10], wlan_status->mac[12], wlan_status->mac[13], \
              wlan_status->mac[15], wlan_status->mac[16] );
	if (strcmp(appConfig->maindev.modelid,"")==0 )
		strcpy(appConfig->maindev.modelid,MODEL_ID);
	if (strcmp(appConfig->maindev.name,"")==0 ){
		strcpy(appConfig->maindev.name,DEV_MODEL_NAME);
		strcpy(appConfig->maindev.uuid,appConfig->maindev.mac);
	}
	app_log("init_appconfig_user_data: mac[%s]\n",appConfig->maindev.mac);
#endif
}

int application_start( void )
{
    app_log_trace();
    OSStatus err = kNoErr;

    mico_rtos_init_semaphore( &wait_sem, 1 );

    app_context_t* app_context = NULL;
    mico_Context_t* mico_context;

    /*init gpio*/
    mlink_gpio_init();


    /* Create application context */
    app_context = (app_context_t *) calloc( 1, sizeof(app_context_t) );
    require_action( app_context, exit, err = kNoMemoryErr );

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler,
                                       NULL );
    require_noerr( err, exit );

    /* Create mico system context and read application's config data from flash */
    mico_context = mico_system_context_init( sizeof(application_config_t) );
    app_context->appConfig = mico_system_context_get_user_data( mico_context );

    memset(&app_context->appStatus, 0, sizeof(current_app_status_t));
    mlink_sys_status_start();
    mlink_multi_click_dist_init(main_multi_click_distribute);

    /* mico system initialize */
    err = mico_system_init( mico_context );

    // restore user data
    if (userDataRestoreDefaultCheck())
    {
        userDataRestoreDefault(app_context->appConfig);
    }
    require_noerr( err, exit );
#ifdef  BLE_DEVICE
    main_init_subdev_info( );
    err = ble_logic_init( main_ble_callback_proc );
#endif
#ifndef BLE_DEVICE
    err = main_uart_comm_init(app_context);
#endif

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );

    init_appconfig_user_data(app_context->appConfig);

    /* Initialize service discovery */
    err = MLinkStartBonjourService( Station, app_context );
    require_noerr( err, exit );

    /* Initialize main logic */
    err = main_logic_init(app_context);

    require_noerr( err, exit );

exit:
    if ( wait_sem != NULL )
        mico_rtos_deinit_semaphore( &wait_sem );
    mico_rtos_delete_thread( NULL );
    return err;
}

