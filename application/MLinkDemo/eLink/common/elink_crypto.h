/*
 * elink_crypto.h
 *
 *  Created on: 2018年7月30日
 *      Author: hxfky
 */

#ifndef APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_CRYPTO_H_
#define APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_CRYPTO_H_

#ifdef __cplusplus
extern "C"
{
#endif

    unsigned char * AES_CBC_Encrypt_Base64(
                                            unsigned char *PlainText,
                                            unsigned int PlainTextLength,
                                            unsigned char *Key,
                                            unsigned int KeyLength,
                                            unsigned char *IV,
                                            unsigned int IVLength );

    unsigned char * AES_CBC_Decrypt_Base64(
                                            unsigned char *CipherText,
                                            unsigned int CipherTextLength,
                                            unsigned char *Key,
                                            unsigned int KeyLength,
                                            unsigned char *IV,
                                            unsigned int IVLength,
                                            unsigned char *PlainText,
                                            unsigned int *PlainTextLength );

#ifdef __cplusplus
}

#endif
#endif /* APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_CRYPTO_H_ */
