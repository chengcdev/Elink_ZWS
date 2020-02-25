/**
******************************************************************************
* @file    MLinkDebug.h
* @author  huangxf
* @version V1.0.0
* @date    2017年5月23日
* @brief   This file provides xxx functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2017 MXCHIP Inc.
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
******************************************************************************
*/ 
#ifndef DEMOS_APPLICATION_MLINKDEMO_MLINKDEBUG_H_
#define DEMOS_APPLICATION_MLINKDEMO_MLINKDEBUG_H_

#define MLINK_DEBUG


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MLINK_DEBUG
    #define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
    #define app_log_trace() custom_log_trace("APP")
#else
    #define app_log(M, ...)
    #define app_log_trace()

#endif


#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_MLINKDEBUG_H_ */
