/*
 * Copyright 2015, Marvell International Ltd.
 * All Rights Reserved.
 */

/*! \file system-work-queue.h
 * \brief System Work Queue
 *
 * Work queues are software entities provided as a service to the
 * application layer to simplify the task of programming new
 * features. For more information on this, please refer \ref work-queue.h.
 * If various modules need to use this feature, there will be too many work
 * queues in the system, which is not desirable. Hence, a common system queue
 * has been defined which can be used by all the modules. It runs at a priority
 * \ref DEFAULT_WORKER_PRIO and offers stack as per CONFIG_SYS_WQ_STACK config
 * option
 */

#ifndef __SYSTEM_WORK_QUEUE_H__
#define __SYSTEM_WORK_QUEUE_H__

#include <work-queue.h>

/** Initialize the system work queue
 *
 * This functions initializes the system work queue.
 * This call is mandatory before a work queue can be used.
 *
 * \return WM_SUCCESS System Work queue initialization successfully.
 * \return -WM_E_INVAL if invalid arguments given.
 * \return -WM_E_NOMEM if heap allocation failed.
 * \return -WM_FAIL for any other failure.
 */
int sys_work_queue_init();
/** Gives the System Work Queue Handle
 *
 * This API returns a system work queue handle if its initialized using
 * sys_work_queue_init(). Calling this API multiple times does not cause any
 * inadvertent behavior. Jobs can then be added to or removed from this queue
 * using work_enqueue() and work_dequeue(). This API is safe to call from
 * an interrupt handler as well.
 *
 * \return wq_handle_t System Work queue handle if work queue is initialized
 * successfully.
 * \return 0 If work queue is not initialized.
 */
wq_handle_t sys_work_queue_get_handle(void);

#endif /* __SYSTEM_WORK_QUEUE_H__ */
