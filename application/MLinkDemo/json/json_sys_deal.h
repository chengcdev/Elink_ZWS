/*
 * json_sys_deal.h
 *
 *  Created on: 2017年7月27日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_SYS_DEAL_H_
#define DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_SYS_DEAL_H_


#ifdef __cplusplus
extern "C" {
#endif


void json_sys_pack_setmesh(PSETMESH_T setMeshData, char *json_package);

void json_sys_pack_regnet(void);

OSStatus json_sys_unpack_setmesh(char *data, PSETMESH_T setMeshData);

void json_sys_unpack_netparam(char *data, PNET_PARAM_T netParam);

void json_sys_unpack_reg_devInfo(char *data, PDEVICEOBJ_T pregObject);




#ifdef __cplusplus
}
#endif


#endif /* DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_SYS_DEAL_H_ */
