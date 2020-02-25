/**
 ******************************************************************************
 * @file    ext_light_sensor.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   light sensor control demo.
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
#include "micokit_ext.h"

#define ext_light_sensor_log(M, ...) custom_log("EXT", M, ##__VA_ARGS__)

int application_start( void )
{
  OSStatus err = kNoErr;
  uint16_t light_sensor_data = 0;
  
  /*init Light sensor*/ 
  err = light_sensor_init();
  require_noerr_action( err, exit, ext_light_sensor_log("ERROR: Unable to Init light sensor") );
  
  while(1)
  {
    err = light_sensor_read(&light_sensor_data);
    require_noerr_action( err, exit, ext_light_sensor_log("ERROR: Can't light sensor read data") );
    ext_light_sensor_log("light date: %d", light_sensor_data);
    mico_thread_sleep(1);
  }
exit:
  return err; 
}



