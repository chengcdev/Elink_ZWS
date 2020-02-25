/* ml_logic_client.h -- ml logic_service header file for ML CoAP stack
 *
 * Copyright (C) 2017--2018
 *
 * This file is part of the ml CoAP library libcoap. Please see
 * README for terms of use. 
 */

#ifndef _ML_LOGIC_CLIENT_H_
#define _ML_LOGIC_CLIENT_H_

#include "config.h"
#include "ml_coap.h"
#include "resource.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
	MLCoAP_GET		=1,
	MLCoAP_POST		=2,
	MLCoAP_PUT		=3,
	ML_CoAP_DELETE	=4,
	
}MLCOAP_METHOD_E;


void mlcoap_set_token(char *arg);
str mlcoap_get_token(void);


		
#ifdef __cplusplus
}
#endif

#endif /* _ML_LOGIC_CLIENT_H_ */

