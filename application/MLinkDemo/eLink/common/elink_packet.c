/*
 * elink_packet.c
 *
 *  Created on: 2018年8月14日
 *      Author: hxfky
 */

#include "elink_packet.h"

static QUEUE g_queue_send;
static QUEUE g_queue_recv;

static void elink_init_packet( void )
{
    init_queue( &g_queue_send, sizeof(stElinkPacket) );
    init_queue( &g_queue_recv, sizeof(stElinkPacket) );
}

static void elink_push_node( QUEUE *queue, stElinkPacket *packet )
{
    int ret = 0;
    ret = push_node( queue, packet );
    return ret;
}

