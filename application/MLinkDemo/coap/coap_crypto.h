/* ml_coap_crypto.h -- ml ml_coap_crypto header file for ML CoAP stack
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef _ML_COAP_CRYPTO_H_
#define _ML_COAP_CRYPTO_H_



#ifdef __cplusplus
extern "C" {
#endif

#define ML_COAP_ENCRYPT_TYPE			"a" //aes

#define ML_COAP_AES_PUBLIC_KEY	""

/**
 *  get 鉴权 验证数据   
 *
 * @param tid	 coap 数据 id
 * @param token  
 * @param reqdata   请求的数据内容
 * @param veritydata 要鉴权校验的数据
 * @return  veritydata length.
 */
int mlcoap_get_authorized(unsigned int tid,unsigned char *token,unsigned char *reqdata,
			unsigned char *veritydata);
/**
 *  鉴权 验证数据是否合法   
 *
 * @param tid	 coap 数据 id
 * @param token  
 * @param reqdata   请求的数据内容
 * @param veritydata 要鉴权校验的数据
 * @return =1 on success or <=0 on error.
 */
int mlcoap_is_authorized(unsigned int tid,unsigned char *token,unsigned char *reqdata,
			unsigned char *veritydata);


void mlcoap_set_aes_privatekey(unsigned char *key);


unsigned char * mlcoap_data_encrypt(unsigned char *enctype,unsigned char *srcdata);


unsigned char * mlcoap_data_decrypt(unsigned char *dectype,unsigned char *srcdata);

		
#ifdef __cplusplus
}
#endif

#endif /* _ML_COAP_CRYPTO_H_ */

