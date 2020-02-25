/*
 * flash.h
 *
 *  Created on: 2017年7月26日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_H_
#define DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mico.h"
//#include "MiCODriverFlash.h"

OSStatus MicoFlashEraseEx(mico_partition_t inPartition, uint32_t off_set, uint32_t size);


OSStatus MicoFlashReadEx( mico_partition_t inPartition, volatile uint32_t* off_set, \
                          uint8_t* outBuffer, uint32_t outBufferLength);


OSStatus MicoFlashWriteEx( mico_partition_t inPartition, volatile uint32_t* off_set, \
                           uint8_t* inBuffer, uint32_t inBufferLength);

#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_FLASH_FLASH_H_ */


