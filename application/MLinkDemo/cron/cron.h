/*
 * cron.h
 *
 *  Created on: 2018年6月13日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_CRON_CRON_H_
#define APPLICATION_MLINKDEMO_CRON_CRON_H_


#ifdef __cplusplus
extern "C"
{
#endif

// schedule
typedef int (*func_schedule_task)(char *id, void *task_info, unsigned int info_len);


/********************************************************
 * function:    cron_start_service
 * description: start to run cron parsing service
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int cron_start_service( void );

/********************************************************
 * function:    cron_monitor
 * description: monitor cron value
 * input:
 * output:
 * return:
 * auther:   chenb
 * other:
*********************************************************/
int cron_monitor( void );

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_MLINKDEMO_CRON_CRON_H_ */
