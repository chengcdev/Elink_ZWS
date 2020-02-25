#include "coap_service.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include "coap_client.h"
#include "coap_crypto.h"
#include "ml_coap.h"
#include "coap.h"

static coap_service_data_deal_t g_service_callback = 0;

/********************************************************
 * function: mlcoap_service_sys_callback
 * description:
 * input:   1. reqPacket:  point to 'ml_coap_reqPacket_t' about
 * the recive data
 *          2. ackPacket: the member of data point null. we need to malloc
 * memory in this function if we want to response ack.
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_sys_callback(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket)
{
    int ret =0;
    int datalen;
    unsigned char *reqData;

//    debug("rev callback id[%d] [%s] t[%s] enc[%s] [%s] \n ",
//        reqPacket->id, reqPacket->srcaddr, reqPacket->token, reqPacket->encrypt,reqPacket->verify);

    if (reqPacket->data==NULL){
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        ret =-1;
        return ret;
    }


#if IS_USE_MLCOAP_Authorized

    ret = mlcoap_is_authorized(reqPacket->id,reqPacket->token,reqPacket->data,reqPacket->verify);
    if (ret <=0)
    {
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        return (-2);
    }

#endif

#if IS_USE_MLCOAP_ENCRYPT
    reqData = mlcoap_data_decrypt(reqPacket->encrypt,reqPacket->data);
    if (reqData )
    debug("rev callback dec data: %s \n",reqData);

#else
	reqData = reqPacket->data;
	datalen = reqPacket->length;
#endif 

    //logic_sys_distribute(reqData, reqPacket->length, g_service_logic_callback, ackPacket);
	if (g_service_callback)
	    ret = g_service_callback(ML_COAP_URITYPE_SYS, reqData, datalen, reqPacket->srcaddr, ackPacket);
//	if (ackPacket){
//
//		ackPacket->data =malloc(1400);
//		memset(ackPacket->data, 0, 1200);
//		memcpy(ackPacket->data,reqData, reqPacket->length);
//		//memcpy(ackPacket->data,jsondata,strlen(jsondata));
//        //ackPacket->datalen = strlen(jsondata);
//		ackPacket->datalen = reqPacket->length;
//
//
//
////
////		ml_coap_notify_obs(ML_COAP_URITYPE_DISCOVER,jsondata,strlen(jsondata));//reqPacket->data,strlen(reqPacket->data));
////
////		ml_coap_notify_obs(3,jsondata,strlen(jsondata));//reqPacket->data,strlen(reqPacket->data));
////		ml_coap_notify_discover(reqPacket->data);
//	}
	//json_sys_parse(reqPacket->data, reqPacket->length);

	return ret;
}

/********************************************************
 * function: mlcoap_service_manager_callback
 * description:
 * input:   1. reqPacket
 *          2. ackPacket
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_manager_callback(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket)
{
    int ret =0;
    unsigned char *reqData;

//    debug("rev callback id[%d] [%s] t[%s] enc[%s] [%s] \n ",
//        reqPacket->id, reqPacket->srcaddr, reqPacket->token, reqPacket->encrypt,reqPacket->verify);

    if (reqPacket->data==NULL){
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        ret =-1;
        return ret;
    }

    printf("mlcoap_service_manager_callback");
#if IS_USE_MLCOAP_Authorized

    ret = mlcoap_is_authorized(reqPacket->id,reqPacket->token,reqPacket->data,reqPacket->verify);
    if (ret <=0)
    {
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        return (-2);
    }

#endif

#if IS_USE_MLCOAP_ENCRYPT
    reqData = mlcoap_data_decrypt(reqPacket->encrypt,reqPacket->data);
    if (reqData )
    debug("rev callback dec data: %s \n",reqData);

#else
    reqData = reqPacket->data;
#endif

    //logic_sys_distribute(reqData, reqPacket->length, g_service_logic_callback, ackPacket);
    if (g_service_callback)
        ret= g_service_callback(ML_COAP_URITYPE_MANAGER, reqData, reqPacket->length, reqPacket->srcaddr, ackPacket);

    //json_sys_parse(reqPacket->data, reqPacket->length);

    return ret;
}

/********************************************************
 * function: mlcoap_service_eventmsg_callback
 * description:
 * input:   1. reqPacket
 *          2. ackPacket
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_eventmsg_callback(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket)
{
    int ret =0;
//    int datalen;
    unsigned char *reqData;

    debug("rev callback id[%d] [%s] t[%s] enc[%s] [%s] \n ",
        reqPacket->id, reqPacket->srcaddr, reqPacket->token, reqPacket->encrypt,reqPacket->verify);

//    if (reqPacket->data==NULL){
//        ackPacket->echoValue =MLCOAP_RET_OK;
//        return ret;
//    }

#if IS_USE_MLCOAP_Authorized

    ret = mlcoap_is_authorized(reqPacket->id,reqPacket->token,reqPacket->data,reqPacket->verify);
    if (ret <=0)
    {
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        return (-2);
    }

#endif

#if IS_USE_MLCOAP_ENCRYPT
    reqData = mlcoap_data_decrypt(reqPacket->encrypt,reqPacket->data);
    if (reqData )
    debug("rev callback dec data: %s \n",reqData);

#else
    reqData = reqPacket->data;
#endif
    //logic_sys_distribute(reqData, reqPacket->length, g_service_logic_callback, ackPacket);
    if (g_service_callback)
        ret =  g_service_callback(ML_COAP_URITYPE_MSG, reqData, reqPacket->length, reqPacket->srcaddr, ackPacket);
//    if (ackPacket){
//
//        ackPacket->data =malloc(1400);
//        memset(ackPacket->data, 0, 1200);
//        memcpy(ackPacket->data,reqData, reqPacket->length);
//        //memcpy(ackPacket->data,jsondata,strlen(jsondata));
//        //ackPacket->datalen = strlen(jsondata);
//        ackPacket->datalen = reqPacket->length;
//
//
//
////
////      ml_coap_notify_obs(ML_COAP_URITYPE_DISCOVER,jsondata,strlen(jsondata));//reqPacket->data,strlen(reqPacket->data));
////
////      ml_coap_notify_obs(3,jsondata,strlen(jsondata));//reqPacket->data,strlen(reqPacket->data));
////      ml_coap_notify_discover(reqPacket->data);
//    }
    //json_sys_parse(reqPacket->data, reqPacket->length);

    return ret;
}

/********************************************************
 * function: mlcoap_service_ctrl_callback
 * description:
 * input:   1. reqPacket
 *          2. ackPacket
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_ctrl_callback(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket)
{
    int ret =0;
//    int datalen;
    unsigned char *reqData;

//    debug("rev callback id[%d] [%s] t[%s] enc[%s] [%s] \n ",
//        reqPacket->id, reqPacket->srcaddr, reqPacket->token, reqPacket->encrypt,reqPacket->verify);

    if (reqPacket->data==NULL){
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        ret =-1;
        return ret;
    }


#if IS_USE_MLCOAP_Authorized

    ret = mlcoap_is_authorized(reqPacket->id,reqPacket->token,reqPacket->data,reqPacket->verify);
    if (ret <=0)
    {
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        return (-2);
    }

#endif

#if IS_USE_MLCOAP_ENCRYPT
    reqData = mlcoap_data_decrypt(reqPacket->encrypt,reqPacket->data);
    if (reqData )
    debug("rev callback dec data: %s \n",reqData);

#else
    reqData = reqPacket->data;
#endif

    //logic_sys_distribute(reqData, reqPacket->length, g_service_logic_callback, ackPacket);
    if (g_service_callback)
        ret = g_service_callback(ML_COAP_URITYPE_CTRL, reqData, reqPacket->length, reqPacket->srcaddr, ackPacket);
//    if (ackPacket){
//
//        ackPacket->data =malloc(1400);
//        memset(ackPacket->data, 0, 1200);
//        memcpy(ackPacket->data,reqData, reqPacket->length);
//        ackPacket->datalen = reqPacket->length;
//    }

    return ret;
}

/********************************************************
 * function: mlcoap_service_manager_callback
 * description:
 * input:   1. reqPacket
 *          2. ackPacket
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_discover_callback(ml_coap_reqPacket_t *reqPacket,ml_coap_ackPacket_t *ackPacket)
{

    int ret =0;
    int datalen;
    unsigned char *reqData;

//    debug("rev callback id[%d] [%s] t[%s] enc[%s] [%s] \n ",
//        reqPacket->id, reqPacket->srcaddr, reqPacket->token, reqPacket->encrypt,reqPacket->verify);


    if (reqPacket->data==NULL){
        ackPacket->echoValue =MLCOAP_RET_OK;
//        ret = -1;
        return ret;
    }


#if IS_USE_MLCOAP_Authorized

    ret = mlcoap_is_authorized(reqPacket->id,reqPacket->token,reqPacket->data,reqPacket->verify);
    if (ret <=0)
    {
        ackPacket->echoValue =MLCOAP_RET_BADREQ;
        return (-2);
    }

#endif

#if IS_USE_MLCOAP_ENCRYPT
    reqData = mlcoap_data_decrypt(reqPacket->encrypt,reqPacket->data);
    if (reqData )
    debug("rev callback dec data: %s \n",reqData);

#else
    reqData = reqPacket->data;
    datalen = reqPacket->length;
#endif
    //logic_sys_distribute(reqData, reqPacket->length, g_service_logic_callback, ackPacket);
    if (g_service_callback)
        ret =  g_service_callback(ML_COAP_URITYPE_DISCOVER, reqData, datalen, reqPacket->srcaddr, ackPacket);

    return ret;

}

/********************************************************
 * function: mlcoap_service_init
 * description:
 * input:
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
*********************************************************/
int mlcoap_service_init(void)
{
	int ret=0;
	ret =ml_coap_init();
	
	ml_coap_reg_handler(ML_COAP_URITYPE_SYS, mlcoap_service_sys_callback);
	
	ml_coap_reg_handler(ML_COAP_URITYPE_MANAGER, mlcoap_service_manager_callback);

	ml_coap_reg_handler(ML_COAP_URITYPE_CTRL, mlcoap_service_ctrl_callback);

	ml_coap_reg_handler(ML_COAP_URITYPE_MSG, mlcoap_service_eventmsg_callback);

	ml_coap_reg_handler(ML_COAP_URITYPE_DISCOVER, mlcoap_service_discover_callback);

	return ret;
}


/*************************************************************
 * function: mlcoap_service_exit
 * description:
 * input:
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
**************************************************************/
void mlcoap_service_exit(void)
{
	ml_exit_coap();
}

/*************************************************************
 * function: mlcoap_init_sys_callback
 * description:
 * input:
 * output:
 * return:   TRUE/FALSE
 * auther:
 * other:
**************************************************************/
void mlcoap_init_service_callback(coap_service_data_deal_t service_callback)
{
    g_service_callback = service_callback;
}




