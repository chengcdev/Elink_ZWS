/*
 * elink_logic.c
 *
 *  Created on: 2018�?�?0�?
 *      Author: hxfky
 */

#include "../../../mico-os/include/mico_security.h"
#include "common/elink_packet.h"


stELink_CommInfo *g_elink_comminfo = NULL;
stELink_DevInfo *g_elink_devinfo = NULL;
/****
 *
 * elink_init_logic 逻辑初始�?
 */
int elink_init_logic( void )
{

    if ( g_elink_comminfo == NULL )
    {
        g_elink_comminfo = malloc( sizeof(stELink_CommInfo) );
        g_elink_devinfo = malloc( sizeof(stELink_DevInfo) );
    }
    elink_logic_init_devchannel( );

}
