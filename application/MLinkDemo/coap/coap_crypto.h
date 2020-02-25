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
 *  get ��Ȩ ��֤����   
 *
 * @param tid	 coap ���� id
 * @param token  
 * @param reqdata   �������������
 * @param veritydata Ҫ��ȨУ�������
 * @return  veritydata length.
 */
int mlcoap_get_authorized(unsigned int tid,unsigned char *token,unsigned char *reqdata,
			unsigned char *veritydata);
/**
 *  ��Ȩ ��֤�����Ƿ�Ϸ�   
 *
 * @param tid	 coap ���� id
 * @param token  
 * @param reqdata   �������������
 * @param veritydata Ҫ��ȨУ�������
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

