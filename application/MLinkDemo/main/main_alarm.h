/*
 * main_alarm.h
 *
 *  Created on: 2018年1月1日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_MAIN_MAIN_ALARM_H_
#define APPLICATION_MLINKDEMO_MAIN_MAIN_ALARM_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "MLinkCommand.h"
#include "../queue/queue.h"

typedef enum{
    ALARM_MODE_HOME         = 0,        // 在家模式下只有24小时防区才触发报警
    ALARM_MODE_OUTGOING     = 1,        // 外出模式下24小时和外出警戒情况下触发报警
    ALARM_MODE_NIGHT        = 2         // 夜间模式下三者都报警
}ALARM_MODE_E;

typedef enum{
    ALARM_NONWORKING        = 0,
    ALARM_WORKING           = 1
}ALARM_WORK_E;

typedef struct{
    uint32_t delayTime;
    EVENT_REPORT_T alarmEvent;
}ALARM_INFO, *PALARM_INFO;


#ifdef __cplusplus
}
#endif


#endif /* APPLICATION_MLINKDEMO_MAIN_MAIN_ALARM_H_ */
