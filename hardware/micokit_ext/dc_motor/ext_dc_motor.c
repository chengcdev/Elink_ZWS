/**
 ******************************************************************************
 * @file    ext_dc_motor.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   DC motor control demo.
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
 **/

#include "mico.h"
#include "MiCOKit_EXT/micokit_ext.h"

#define ext_dc_motor_log(M, ...) custom_log("EXT", M, ##__VA_ARGS__)

int application_start( void )
{
  OSStatus err = kNoErr;
  
  /*init DC Motor*/ 
  err = dc_motor_init();
  require_noerr_string( err, exit, "ERROR: Unable to Init DC motor" );
  
  while(1)
  {    
    ext_dc_motor_log("start");
    dc_motor_set(1);
    mico_thread_msleep(1500);
    dc_motor_set(0);
    mico_thread_msleep(4500);
  }
exit:
  return err; 
}



