/*
 * queue.h
 *
 *  Created on: 2018年1月30日
 *      Author: Administrator
 */

#ifndef APPLICATION_MLINKDEMO_QUEUE_QUEUE_H_
#define APPLICATION_MLINKDEMO_QUEUE_QUEUE_H_


#ifdef __cplusplus
extern "C"
{
#endif


#include "mico.h"
#define pthread_mutex_t   mico_mutex_t
#define pthread_mutex_init(a, b) mico_rtos_init_mutex(a)
#define pthread_mutex_lock mico_rtos_lock_mutex
#define pthread_mutex_unlock mico_rtos_unlock_mutex
#define QUEUE_COUNT_DEBUG       1
typedef struct tagQUEUE_NODE
{
    struct tagQUEUE_NODE* prev;
    struct tagQUEUE_NODE* next;
    void *data;
}QUEUE_NODE,*pQUEUE_NODE;

typedef struct tagQUEUE
{
    pQUEUE_NODE head;
    pQUEUE_NODE end;
    int queue_data_size;
#if QUEUE_COUNT_DEBUG
    int count;
#endif
    pthread_mutex_t mutex;
}QUEUE;

/*************************************************
  Function:     init_queue
  Description:  初始化队列
  Input:
    queue       队列指针
    data_size   数据节点大小
  Output:
  Return:
    成功返回0
*************************************************/
int init_queue(QUEUE *queue,int data_size);

/*************************************************
  Function:     push_node
  Description:  数据入队
  Input:
    queue       队列指针
    data        欲入队的数据指针
  Output:
  Return:
    成功      AU_SUCCESS
    失败      AU_FAILURE
*************************************************/
int push_node(QUEUE *queue,void* data);

/*************************************************
  Function:     pop_node
  Description:  数据出队
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
void* pop_node(QUEUE *queue);

/*************************************************
  Function:     pop_node
  Description:  数据出队
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
int del_node(QUEUE *queue, pQUEUE_NODE node);

/*************************************************
  Function:     empty_node
  Description:  清空队列
  Input:
    queue       队列指针
  Output:
  Return:
    成功      AU_SUCCESS
*************************************************/
int empty_node(QUEUE *queue);

#ifdef __cplusplus
}
#endif

#endif /* APPLICATION_MLINKDEMO_QUEUE_QUEUE_H_ */
