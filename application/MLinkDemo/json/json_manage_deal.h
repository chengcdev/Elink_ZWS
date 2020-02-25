/*
 * json_manage_deal.h
 *
 *  Created on: 2017年8月1日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_MANAGE_DEAL_H_
#define DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_MANAGE_DEAL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "MLinkCommand.h"
#include "MLinkObject.h"

typedef struct{
    char uuid[32];
    int  classid;
    char objid[OBJID_SIZE];
    int  limit;
    int  page;
}MANAGE_PARAM_GET_T, *PMANAGE_PARAM_GET_T;

typedef struct {
    char devId[DEVICEID_SIZE];
    int  classid;
    char objId[OBJID_SIZE];
    int  limit;
    int  page;
}MANAGE_DEL_PARAM_T, *PMANAGE_DEL_PARAM_T;

OSStatus json_manage_unpack_get(char *data, PMANAGE_PARAM_GET_T param_struct);




#ifdef __cplusplus
}
#endif

#endif /* DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_MANAGE_DEAL_H_ */
