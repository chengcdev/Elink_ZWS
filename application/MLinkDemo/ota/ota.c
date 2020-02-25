/**
 ******************************************************************************
 * @file    ota.c
 * @author  QQ ding
 * @version V1.0.0
 * @date    219-Oct-2016
 * @brief   Firmware update example
 ******************************************************************************
 *
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

#include "mico.h"
#include "ota_server.h"
#include "ota.h"

#define ota_log(M, ...) custom_log("OTA", M, ##__VA_ARGS__)

ota_result_deal ota_succ_deal;
ota_result_deal ota_fail_deal;

/********************************************************
 * function: ota_server_status_handler
 * description: deal ota result
 * input:   1. state:
 *          2. progress
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
static void ota_server_status_handler(OTA_STATE_E state, float progress)
{
    switch ( state )
    {
        case OTA_LOADING:
            ota_log("ota server is loading, progress %.2f%%", progress);
            break;
        case OTA_SUCCE:
            if (ota_succ_deal)
            {
                ota_succ_deal();
            }
//            mqtt_pub_manage();
            ota_log("ota server daemons success");
            mico_thread_sleep(3);
            break;
        case OTA_FAIL:
            if (ota_fail_deal)
            {
                ota_fail_deal();
            }
            ota_log("ota server daemons failed");
            break;
        default:
            break;
    }
}

/********************************************************
 * function: ota_server_start_ex
 * description: start to upgrade with ota
 * input:   1. url:
 *          2. md5
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
void ota_server_start_ex(char *url, char *md5)
{
    ota_server_start(url, md5, ota_server_status_handler);
}

/********************************************************
 * function: ota_server_init
 * description:
 * input:   1. ota_succ:   callback param
 *          2. ota_fail:   callback param
 * output:
 * return:
 * auther:
 * other:
*********************************************************/
void ota_server_init( ota_result_deal ota_succ, ota_result_deal ota_fail )
{
    ota_succ_deal = ota_succ;
    ota_fail_deal = ota_fail;
}
