/* ml_logic_service.h -- ml logic_service header file for ML CoAP stack
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef _ML_LOGIC_SERVICE_H_
#define _ML_LOGIC_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	MLCOAP_RET_OK			=0x45,      // 2.05
	MLCOAP_RET_BADREQ		=0x80,      // 4.00
	MLCOAP_RET_SERVER_ERR	=0xa0,      // 5.00
}MLCoAP_RET_E;

typedef int (*PCOAP_BIZ_DEAL_CALLBACK)(char * data, unsigned char len);
typedef int (*PCOAP_BIZ_RESP_CALLBACK)(char * data, unsigned char len);

int mlcoap_service_init();

void mlcoap_service_exit(void);


#ifdef __cplusplus
}
#endif

#endif /* _ML_LOGIC_SERVICE_H_ */
