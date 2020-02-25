/*
 * json_disc_deal.h
 *
 *  Created on: 2017年7月27日
 *      Author: Administrator
 */

#ifndef DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_DISC_DEAL_H_
#define DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_DISC_DEAL_H_


#ifdef __cplusplus
extern "C" {
#endif

void json_pack_req_discover(char *uuid, char *json_package);

void json_disc_pack_i_am(PNETDEVOBJ_T pnetdevobj, char *json_package);

void json_disc_pack_i_hav(PREPORT_SUBDEV_INFO_T pdeviceObj, char *json_package);

OSStatus json_disc_unpack_whois(char *data, char *uuid);


#ifdef __cplusplus
}
#endif



#endif /* DEMOS_APPLICATION_MLINKDEMO_JSON_JSON_DISC_DEAL_H_ */
