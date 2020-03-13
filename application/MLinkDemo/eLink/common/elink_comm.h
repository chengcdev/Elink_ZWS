/*
 * elink_comm.h
 *
 *  Created on: 2018年7月30日
 *      Author: hxfky
 */

#ifndef APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_COMM_H_
#define APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_COMM_H_

#include "mico.h"

#define ELink_SOFT_VER	"1.1.1.1"
#define ELink_HARD_VER  "1.0.0.1"
#define ELink_DevModel "MK-S06"
#if 1
//#define ELink_LOGIN_TCPHOST     "223.255.253.3:9012"
#define ELink_LOGIN_TCPHOST		"180.100.133.131:9017"//"smarthome.thirdcloud.ott4china.com:9012"//"180.100.133.131:9017"
#else
#define ELink_LOGIN_TCPHOST		"192.168.12.110:9017"//"192.168.100.137:9017"//"192.168.12.108:9017"
#endif
#define ELink_LOGIN_TCPDOMAIN	"smarthome.test.ott4china.com:9017"

#define elink_log(M, ...) custom_log("eLink", M, ##__VA_ARGS__)

#endif /* APPLICATION_MLINKDEMO_ELINK_COMMON_ELINK_COMM_H_ */
