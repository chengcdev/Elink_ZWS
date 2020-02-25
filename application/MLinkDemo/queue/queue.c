/*
 * queue.c
 *
 *  Created on: 2018年1月30日
 *      Author: Administrator
 */

#include "queue.h"

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
int init_queue(QUEUE *queue,int data_size)
{
    queue->head = NULL;
    queue->end = NULL;
    queue->queue_data_size = data_size;
    pthread_mutex_init(&(queue->mutex), NULL);

    return true;
}

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
int push_node(QUEUE *queue,void* data)
{
    pQUEUE_NODE node;
    node = (pQUEUE_NODE)malloc(sizeof(QUEUE_NODE));
    if (node == NULL)
    {
        return false;
    }
    node->data = malloc(queue->queue_data_size);
    if (node->data == NULL)
    {
        return false;
    }
#if QUEUE_COUNT_DEBUG
    queue->count++;     // chenb
#endif
    memset(node->data, 0, queue->queue_data_size);
    memcpy(node->data,data,queue->queue_data_size);
    node->next = NULL;

    pthread_mutex_lock(&(queue->mutex));

    if (queue->head == NULL)
    {
        queue->head = node;
        queue->end = node;
        node->prev = NULL;
    }else
    {
        queue->end->next = node;
        node->prev = queue->end;
        queue->end = node;
    }

    pthread_mutex_unlock(&(queue->mutex));

    return true;
}

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
void* pop_node(QUEUE *queue)
{
    pQUEUE_NODE node;
    void *data;
    pthread_mutex_lock(&(queue->mutex));
    if (queue->head == 0)
    {
        data = NULL;
    }else
    {
        node = queue->head;
        queue->head = node->next;
        if (queue->head == NULL)
        {
            queue->end = NULL;
        }
        data = node->data;
#if QUEUE_COUNT_DEBUG
        queue->count--;     // chenb
#endif
        free(node);
    }
    pthread_mutex_unlock(&(queue->mutex));

    return data;
}

/*************************************************
  Function:     del_node
  Description:  数据出队
  Input:
    queue       队列指针
  Output:
  Return:
    成功      数据的指针
    失败      NULL
*************************************************/
int del_node(QUEUE *queue, pQUEUE_NODE node)
{
    pQUEUE_NODE nodeTemp;
    void *data;
    pthread_mutex_lock(&(queue->mutex));
    if (queue->head == 0)
    {
        return kNoErr;
    }else
    {
        if (queue->head == node)
        {
            queue->head = node->next;
            queue->head->prev = NULL;
            if (queue->end == node)
            {
                queue->end = NULL;
            }
        }
        else if (queue->end == node)
        {
            queue->end = node->prev;
            queue->end->next = NULL;
        }
        else
        {
            pQUEUE_NODE nodeNetxTemp;
            nodeTemp = node->prev;
            nodeNetxTemp = node->next;
            nodeTemp->next = node->next;
            nodeNetxTemp->prev = node->prev;
        }

        if (node->data != NULL)
        {
            free(node->data);
            node->data = NULL;
        }
        free(node);
#if QUEUE_COUNT_DEBUG
        if (queue->count > 0)
        {
            queue->count --;
        }
#endif
    }
    pthread_mutex_unlock(&(queue->mutex));

    return data;
}

/*************************************************
  Function:     empty_node
  Description:  清空队列
  Input:
    queue       队列指针
  Output:
  Return:
    成功      AU_SUCCESS
*************************************************/
int empty_node(QUEUE *queue)
{
    pQUEUE_NODE node;
    pthread_mutex_lock(&(queue->mutex));
    while (queue->head)
    {
        node = queue->head;
        queue->head = node->next;
        free(node->data);
        free(node);
        node = queue->head;
    }
    pthread_mutex_unlock(&(queue->mutex));
    return true;
}
