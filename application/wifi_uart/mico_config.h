/**
 ******************************************************************************
 * @file    mico_config.h
 * @author  William Xu
 * @version V1.0.0
 * @date    12-Aug-2015
 * @brief   This file provide constant definition and type declaration for MICO
 *          system running.
 ******************************************************************************
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
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

#pragma once

#define APP_INFO   "SPP(wlan<>uart) Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_SPP_2_6"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20140606"
#define PROTOCOL            "com.mxchip.spp"

#define CONFIG_MODE_TRIGGER_AUTO                (1) /**< Enter wlan config mode if no available wlan. */
#define CONFIG_MODE_TRIGGER_EASYLINK_BTN        (2) /**< Enter wlan config mode by Easylink button. */

/************************************************************************
 * Application thread stack size */
#define MICO_DEFAULT_APPLICATION_STACK_SIZE         (4096)

/************************************************************************
 * Enable wlan connection, start easylink configuration if no wlan settings are existed */
#define MICO_WLAN_CONNECTION_ENABLE

#define MICO_WLAN_CONFIG_MODE CONFIG_MODE_EASYLINK

#define MICO_WLAN_CONFIG_MODE_TRIGGER  CONFIG_MODE_TRIGGER_AUTO

//#define AIRKISS_DISCOVERY_ENABLE
#define AIRKISS_APP_ID			"gh_420af5d2de71"
#define AIRKISS_DEVICE_ID		"EF83A7650F6EFE0CE9285E02951CB7959A5CAC0B5E56FBA47FA2CC8513DEBC52"

#define EasyLink_TimeOut                60000 /**< EasyLink timeout 60 seconds. */

#define EasyLink_ConnectWlan_Timeout    20000 /**< Connect to wlan after configured by easylink.
                                                   Restart easylink after timeout: 20 seconds. */

/************************************************************************
 * Device enter MFG mode if MICO settings are erased. */
//#define MFG_MODE_AUTO

/************************************************************************
 * Command line interface */
#define MICO_CLI_ENABLE  

/************************************************************************
 * Start a system monitor daemon, application can register some monitor  
 * points, If one of these points is not executed in a predefined period, 
 * a watchdog reset will occur. */
#define MICO_SYSTEM_MONITOR_ENABLE

/************************************************************************
 * Add service _easylink._tcp._local. for discovery */
#define MICO_SYSTEM_DISCOVERY_ENABLE   
   
/************************************************************************
 * MiCO TCP server used for configuration and ota. */
#define MICO_CONFIG_SERVER_ENABLE
#define MICO_CONFIG_SERVER_PORT    8000
#define MICO_CONFIG_SERVER_REPORT_SYSTEM_DATA

