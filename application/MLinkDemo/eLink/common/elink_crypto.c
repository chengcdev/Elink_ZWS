/*
 * elink_crypto.c
 *
 *  Created on: 2018年7月30日
 *      Author: hxfky
 */

#include "elink_crypto.h"

#include "../../../../mico-os/include/mico_security.h"
#include "base64.h"
#include "elink_comm.h"

word32 AesCbcEncryptPkcs5Padding( Aes* aes, byte* output, const byte* input, word32 sz )
{
    if ( aes == NULL || output == NULL || input == NULL || sz == 0 )
        return 0;

    word32 div = sz / 16;
    word32 sz_align = (div + 1) * 16;
    byte pad_byte = sz_align - sz;

    byte* input_align = malloc( sz_align );
    if ( input_align == NULL ) return 0;

    memcpy( input_align, input, sz );
    memset( input_align + sz, pad_byte, sz_align - sz );

    AesCbcEncrypt( aes, output, input_align, sz_align );

    free( input_align );
    return sz_align;
}


word32 AesCbcDecryptPkcs5Padding( Aes* aes, byte* output, const byte* input, word32 sz )
{
    if ( aes == NULL || output == NULL || input == NULL || sz < 16 || sz % 16 > 0 )
        return 0;

    byte* output_align = malloc( sz );
    if ( output_align == NULL ) return 0;

    AesCbcDecrypt( aes, output_align, input, sz );

    byte pad_byte = output_align[sz - 1];
    if ( pad_byte >= sz ) return 0;

    word32 act_size = sz - pad_byte;

    byte pad_cmp[16];
    memset( pad_cmp, pad_byte, 16 );
    if ( memcmp( output_align + act_size, pad_cmp, pad_byte ) )
    {
        return 0;
    }

    memcpy( output, output_align, act_size );

    free( output_align );

    return act_size;
}

unsigned char * AES_CBC_Encrypt_Base64(
                                        unsigned char *PlainText,
                                        unsigned int PlainTextLength,
                                        unsigned char *Key,
                                        unsigned int KeyLength,
                                        unsigned char *IV,
                                        unsigned int IVLength )
{

    unsigned int len,outlen;
    unsigned char *baseout;

    Aes enc;
//   elink_log("PlainText : %s Key : %s IV :%s",PlainText,Key,IV);
    unsigned char *TempData = (unsigned char *) malloc( PlainTextLength + 16 );
    if ( TempData == NULL )
    {
        elink_log("AES　Encrypt　malloc fail\n");
        free( TempData );
        TempData = NULL;
        return NULL;
    }

    AesSetKey( &enc, Key, AES_BLOCK_SIZE, IV, AES_ENCRYPTION );
    /* Encryption process */
    word32 ciphe_sz = AesCbcEncryptPkcs5Padding( &enc, TempData, PlainText,
                                                 strlen( (char *) PlainText ) );
    if ( ciphe_sz == 0 )
    {
        elink_log("Encrypt failed!\r\n");
        if(TempData)
        {
            free( TempData );
            TempData = NULL;
            return NULL;
        }

    }

    baseout = base64_encode( TempData, ciphe_sz, (int *) &outlen );
    len = strlen( baseout );
    baseout[len - 1] = 0;

    free( TempData );
    TempData = NULL;

    return baseout;

}

unsigned char *AES_CBC_Decrypt_Base64(
                                       unsigned char *CipherText,
                                       unsigned int CipherTextLength,
                                       unsigned char *Key,
                                       unsigned int KeyLength,
                                       unsigned char *IV,
                                       unsigned int IVLength,
                                       unsigned char *PlainText,
                                       unsigned int *PlainTextLength )
{
    //int i;
    unsigned int outlen;
    unsigned char * temp = NULL;
    Aes dec;
//    elink_log("CipherText : %s Key : %s IV : %s KeyLength : %d IVLength : %d ",CipherText,Key,IV,KeyLength,IVLength);
    temp = base64_decode( CipherText, CipherTextLength, (int*) &outlen );
    if ( temp == NULL )
    {
        elink_log("Decrypt_Base64 malloc fail");
        free( temp );
        temp = NULL;
        return NULL;
    }
    AesSetKey( &dec, Key, AES_BLOCK_SIZE, IV, AES_DECRYPTION );
    word32 plain_sz = AesCbcDecryptPkcs5Padding( &dec, PlainText, temp, outlen );
    *PlainTextLength = plain_sz;
    elink_log("PlainTextLength : %d",*PlainTextLength);
    PlainText[*PlainTextLength] = 0;
    free( temp );
    temp = NULL;
    return PlainText;

}

void test_ase_encrypt_base64( void )
{
    unsigned char msg[] = "012345678";
    unsigned char * chips;
    byte key[] = "0123456789abcdef"; /* align  Must be 128 bits */
    byte iv[] = "1234567890123456"; /* align  Must be 128 bits */

    byte plain[16 * 8];
    word32 plainlen;

    chips = AES_CBC_Encrypt_Base64( msg, strlen( msg ), key, AES_BLOCK_SIZE, iv, AES_BLOCK_SIZE );

    AES_CBC_Decrypt_Base64( chips, strlen( chips ), key, AES_BLOCK_SIZE, iv, AES_BLOCK_SIZE, plain,
                            &plainlen );
    elink_log("The Plain Text is: %s ",plain);

}

