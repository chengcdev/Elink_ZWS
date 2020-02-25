/*
 * ml_logic_main.h
 *
 *  Created on: 2017年7月12日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_MAIN_MAIN_LOGIC_H_
#define DEMOS_APPLICATION_MLINKDEMO_MAIN_MAIN_LOGIC_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "main_server_ctrl.h"
#include "main_server_manage.h"
#include "main_server_sys.h"
#include "main_server_disc_msg.h"
#define COMMOND_RESP_DATA_SIZE      256

#define USE_MUTEX

#ifdef USE_MUTEX
#define MUTEX_LOCK(pMutex)  mico_rtos_lock_mutex(pMutex)
#define MUTEX_UNLOCK(pMutex)  mico_rtos_unlock_mutex(pMutex)
#else
#define MUTEX_LOCK(pMutex)
#define MUTEX_UNLOCK(pMutex)
#endif

typedef enum{
    NETTRANS_MQTT               = 0,
    NETTRANS_COAP               = 1
}NETTRANS_TYPE_E;

#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_MAIN_MAIN_LOGIC_H_ */
