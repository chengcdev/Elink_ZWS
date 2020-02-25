/**
 ******************************************************************************
 * @file    os_thread.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   MiCO RTOS thread control demo.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
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
 *
 ******************************************************************************
 */

#include "mico.h" 

#define os_thread_log(M, ...) custom_log("OS", M, ##__VA_ARGS__)

void thread_1( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    while ( 1 )
    {
        os_thread_log( "This is thread 1" );
        mico_thread_sleep( 2 );
    }
}

void thread_2( mico_thread_arg_t arg )
{
    UNUSED_PARAMETER( arg );
    os_thread_log( "This is thread 2" );
    mico_thread_sleep( 4 );
    /* Make with terminate state and IDLE thread will clean resources */
    mico_rtos_delete_thread( NULL );
}

int application_start( void )
{
    OSStatus err = kNoErr;
    mico_thread_t t_handler = NULL;

    /* Create a new thread */
    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "Thread 1", thread_1, 0x500, 0 );
    require_noerr_string( err, exit, "ERROR: Unable to start the thread 1." );

    while ( 1 )
    {
        /* Create a new thread, and this thread will delete its self and clean its resource */
        err = mico_rtos_create_thread( &t_handler, MICO_APPLICATION_PRIORITY, "Thread 2", thread_2, 0x500, 0 );
        require_noerr_string( err, exit, "ERROR: Unable to start the thread 2." );
        mico_rtos_thread_join( &t_handler );
    }

    exit:
    if ( err != kNoErr )
        os_thread_log( "Thread exit with err: %d", err );

    mico_rtos_delete_thread( NULL );
    return err;
}
