/*
 * flash.c
 * create a new flash operating like erase or read or write.
 *  Created on: 2017年7月6日
 *      Author: chenb
 */

#include "mico.h"

#define BLOCK_SIZE        4096
#define os_flash_log(M, ...) //custom_log("FLASH_OPERATE_DEBUG", M, ##__VA_ARGS__)

/******************************************************************
 * @function MicoFlashEraseEx
 * @decription:  erase the flash
 * @param inPartition
 * @param off_set
 * @param size
 * @return kNoErr/kGeneralErr
 ******************************************************************
 */

OSStatus MicoFlashEraseEx(mico_partition_t inPartition, uint32_t off_set, uint32_t size)
{
    uint32_t EraseSize = size;
    uint32_t block_start = off_set/BLOCK_SIZE;
    uint32_t block_end = (off_set+size)/BLOCK_SIZE;
    uint32_t EraseOffSet = block_start*BLOCK_SIZE;
    char *blockTemp = NULL;

    if (EraseSize == 0)
    {
//        debug("The size of erasing is zero. it's illegal !!!");
        return kGeneralErr;
    }
    else
    {
        EraseSize = BLOCK_SIZE;
    }


    blockTemp = (char*)malloc(BLOCK_SIZE);
    if (blockTemp == NULL)
    {
//        debug("malloc fail. Cann't erase data!!!");
        return kGeneralErr;
    }

    if (block_start == block_end)
    {
        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        memset(blockTemp+off_set%BLOCK_SIZE, 0xff, size);
        MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
    }
    else if (block_start < block_end)
    {

        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        memset(blockTemp+off_set%BLOCK_SIZE, 0xff, BLOCK_SIZE-off_set%BLOCK_SIZE);
        MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        if ((block_end -block_start) > 1)
        {
            uint32_t eraseMiddleAreaSize =  BLOCK_SIZE*(block_end -block_start-1);
            MicoFlashErase(inPartition, EraseOffSet, eraseMiddleAreaSize);
            EraseOffSet = EraseOffSet + eraseMiddleAreaSize;
        }
        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        memset(blockTemp, 0xff, (off_set+size)%BLOCK_SIZE);
        MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
    }

    if (blockTemp != NULL)
    {
        free(blockTemp);
        blockTemp = NULL;
    }
    return kNoErr;
}

/******************************************************************
 * @function MicoFlashReadEx
 * @decription:  erase the flash
 * @param inPartition
 * @param off_set
 * @param outBuffer
 * @param inBufferLength
 * @return kNoErr/kGeneralErr
 ******************************************************************
 */
OSStatus MicoFlashReadEx( mico_partition_t inPartition, volatile uint32_t* off_set, \
                          uint8_t* outBuffer, uint32_t outBufferLength)
{
    return MicoFlashRead(inPartition, off_set, outBuffer, outBufferLength);
}


/******************************************************************
 * @function MicoFlashWriteEx
 * @decription:  write the flash
 * @param inPartition
 * @param off_set
 * @param inBuffer
 * @param inBufferLength
 * @return kNoErr/kGeneralErr
 ******************************************************************
 */
static char *stblockTemp=NULL;
OSStatus MicoFlashWriteEx( mico_partition_t inPartition, volatile uint32_t* off_set, \
                          uint8_t* inBuffer, uint32_t inBufferLength)
{
    uint32_t block_start = (*off_set)/BLOCK_SIZE;
    uint32_t block_end = ((*off_set)+inBufferLength)/BLOCK_SIZE;
    uint32_t EraseOffSet = block_start*BLOCK_SIZE;
    char *blockTemp = NULL;
    uint32_t offsetTemp = *off_set;
    OSStatus ret = kNoErr;
    if ((off_set == NULL) || (NULL == inBuffer))
    {
        return kGeneralErr;
    }
    if (stblockTemp==NULL){
        stblockTemp =(char*)malloc(BLOCK_SIZE);
    }
    blockTemp = stblockTemp;

//        blockTemp = (char*)malloc(BLOCK_SIZE);

    require_action( blockTemp, exit, ret = kNoMemoryErr );
//    if (blockTemp == NULL)
//    {
//        return kGeneralErr;
//    }
    memset(blockTemp, 0, BLOCK_SIZE);
    printf("block_start=%d, block_end=%d, EraseOffSet=%d, off_set = %d\r\n", block_start, block_end, EraseOffSet, *off_set);
    if (block_start == block_end)
    {
        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        memcpy(blockTemp+(*off_set)%BLOCK_SIZE, inBuffer, inBufferLength);
        ret = MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        require_noerr(ret, exit);

        ret = MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        require_noerr(ret, exit);
    }
    else
    {
        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        memcpy(blockTemp+(*off_set)%BLOCK_SIZE, inBuffer, BLOCK_SIZE-(*off_set)%BLOCK_SIZE);
        ret= MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        require_noerr(ret, exit);

        ret =MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        require_noerr(ret, exit);

        MicoFlashRead(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        EraseOffSet -= BLOCK_SIZE;
        memcpy(blockTemp, inBuffer+BLOCK_SIZE-(*off_set)%BLOCK_SIZE, (*off_set+inBufferLength)%BLOCK_SIZE);

        ret = MicoFlashErase(inPartition, EraseOffSet, BLOCK_SIZE);
        require_noerr(ret, exit);
        ret = MicoFlashWrite(inPartition, &EraseOffSet, blockTemp, BLOCK_SIZE);
        require_noerr(ret, exit);
    }
    if (ret == kNoErr)
    {
        *off_set = offsetTemp+inBufferLength;
    }


//    if (blockTemp != NULL)
//    {
//        free(blockTemp);
//        blockTemp = NULL;
//    }
    msleep(10);
    return kNoErr;
exit:
    return ret;
}


