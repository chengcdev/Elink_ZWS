/* ml_coap_crypto.c --ML CoAP data authorized and encrypto deal inferfaces
 *
 * Copyright (C) 2017--2018 
 *
 * 
 * README for terms of use. 
 */

#include "coap_crypto.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>


static unsigned char privatekey[32]={0};


/**
 *  get 鉴权 验证数据
 *
 * @param tid    coap 数据 id
 * @param token  
 * @param reqdata   请求的数据内容
 * @param veritydata 要鉴权校验的数据
 * @return  veritydata length.
 */
int mlcoap_get_authorized(unsigned int tid,unsigned char *token,unsigned char *reqdata,
            unsigned char *veritydata)
{
    int len=0;
    if (token==NULL)
        return 0;
    len = strlen(token);
    memset(veritydata,0,len+1);
    memcpy(veritydata,token,len);

    return len;
}


/**
 *  鉴权 验证数据是否合法
 *
 * @param tid    coap 数据 id
 * @param token  
 * @param reqdata   请求的数据内容
 * @param veritydata 要鉴权校验的数据
 * @return =1 on success or <=0 on error.
 */
int mlcoap_is_authorized(unsigned int tid,unsigned char *token,unsigned char *reqdata,
            unsigned char *veritydata)
{
    int ret =1;

    return ret;
}

void mlcoap_set_aes_privatekey(unsigned char *key)
{

}

unsigned char * mlcoap_data_encrypt(unsigned char *enctype,unsigned char *srcdata)
{
    return srcdata;
}

unsigned char * mlcoap_data_decrypt(unsigned char *dectype,unsigned char *srcdata)
{
    return srcdata;
}
