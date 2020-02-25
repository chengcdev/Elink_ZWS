/**
******************************************************************************
* @file    MLinkBonjour.c
* @author  huangxf
* @version V1.0.0
* @date    2017骞�5鏈�23鏃�
* @brief   This file provides xxx functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2017  Inc.
*
******************************************************************************
*/ 

#include "MLinkAppDef.h"
#include "mico.h"

#include "platform_config.h"
#include "StringUtils.h"

void MLinkBonjourInfoUpdate( void )
{
  OSStatus err = kNoErr;
  char *temp_txt= NULL;
  char *temp_txt2;
  mico_system_status_wlan_t *wlan_status;
  DEVINFOOBJ_T devInfoObj = {0};
  NETDEVOBJ_T netDevObj = {0};

  storage_read_devinfo_obj(&devInfoObj);
  storage_read_local_devobj(&netDevObj);

  mico_system_get_status_wlan( &wlan_status );

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  temp_txt2 = __strdup_trans_dot(wlan_status->mac);
  sprintf(temp_txt, "MAC=%s.", temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  temp_txt2 = __strdup_trans_dot(MicoGetVer());
//  temp_txt2 = __strdup_trans_dot(MicoGetVer());
  sprintf(temp_txt, "%sFirmware Rev=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  temp_txt2 = __strdup_trans_dot(devInfoObj.ver);
  sprintf(temp_txt, "%sSoft Ver=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;
//  temp_txt2 = __strdup_trans_dot(HARDWARE_REVISION);
//  sprintf(temp_txt, "%sHardware Rev=%s.", temp_txt, temp_txt2);
//  free(temp_txt2);


//  sprintf(temp_txt, "%sMICO OS Rev=%s.", temp_txt, temp_txt2);
//  free(temp_txt2);

//  temp_txt2 = __strdup_trans_dot(MODEL);
  if (strcmp(netDevObj.modelname,"")==0 || ((strlen(netDevObj.modelname)>=sizeof(netDevObj.modelname))))
      temp_txt2 = __strdup_trans_dot(YL_MODELNAME);
  else
      temp_txt2 = __strdup_trans_dot(netDevObj.modelname);
  sprintf(temp_txt, "%sModel Name=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  if (strcmp(netDevObj.modelid,"")==0 || ((strlen(netDevObj.modelid)>=sizeof(netDevObj.modelid))))
      temp_txt2 = __strdup_trans_dot(MODEL_ID);
  else
      temp_txt2 = __strdup_trans_dot(netDevObj.modelid);

  sprintf(temp_txt, "%sModel ID=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;
  app_log("temp_txt: %s \n", temp_txt);


//  temp_txt2 = __strdup_trans_dot(PROTOCOL);
  temp_txt2 = __strdup_trans_dot(YL_PROTOCOL);
  sprintf(temp_txt, "%sProtocol=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

//  temp_txt2 = __strdup_trans_dot(MANUFACTURER);
  temp_txt2 = __strdup_trans_dot(YL_MANUFACTURER);
  sprintf(temp_txt, "%sManufacturer=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  app_log("temp_txt: %s \n", temp_txt);

  temp_txt2 = NULL;
  if ((strcmp(netDevObj.uuid,"") != 0)&&(strlen(netDevObj.uuid)<sizeof(netDevObj.uuid))){
      temp_txt2 = __strdup_trans_dot(netDevObj.uuid);
      sprintf(temp_txt, "%sSN=%s.", temp_txt, temp_txt2);
      free(temp_txt2);
  }

//  sprintf(temp_txt, "%sSeed=%lu.", temp_txt, mico_config->micoSystemConfig.seed);

  mdns_update_txt_record(MLINK_BONJOUR_SERVICE, Station, temp_txt);

  exit:
    if(temp_txt) free(temp_txt);
    return err;
}

OSStatus MLinkStartBonjourService( WiFi_Interface interface, app_context_t * const inContext )
{
  char *temp_txt= NULL;
  char *temp_txt2;
  OSStatus err = kNoErr;
  net_para_st para;
  mdns_init_t init;
  mico_system_status_wlan_t *wlan_status;
  mico_Context_t *mico_config = mico_system_context_get();
  mico_system_get_status_wlan( &wlan_status );
  app_log("start bonjour service!!!\n");

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  memset(&init, 0x0, sizeof(mdns_init_t));

  micoWlanGetIPStatus(&para, Station);
  app_log("Get IP status success !!!\n");

  init.service_name = MLINK_BONJOUR_SERVICE;
  app_log("modelname length: %d ", strlen(inContext->appConfig->netdev.modelname));

//app_log("modelname: %s, ", inContext->appConfig->netdev.modelname);
  if (strcmp(inContext->appConfig->netdev.modelname,"") && (strlen(inContext->appConfig->netdev.modelname)<sizeof(mico_config->micoSystemConfig.name)))
	  strcpy( mico_config->micoSystemConfig.name,inContext->appConfig->netdev.modelname);
  else
	  strcpy( mico_config->micoSystemConfig.name,YL_MODELNAME);
  custom_log("mdns"," mico_config->micoSystemConfig.name: %s \n", mico_config->micoSystemConfig.name);
  app_log("mico_config->micoSystemConfig.name: %s \n", mico_config->micoSystemConfig.name);

  /*   name#xxxxxx.local.  */
  snprintf( temp_txt, 100, "%s#%c%c%c%c%c%c.local.", mico_config->micoSystemConfig.name,
            wlan_status->mac[9],  wlan_status->mac[10], wlan_status->mac[12], wlan_status->mac[13], \
            wlan_status->mac[15], wlan_status->mac[16] );

  init.host_name = (char*)__strdup(temp_txt);
  app_log("host_name: %s \n", init.host_name);

  /*   name#xxxxxx.   */
  snprintf( temp_txt, 100, "%s#%c%c%c%c%c%c", mico_config->micoSystemConfig.name,
            wlan_status->mac[9],  wlan_status->mac[10], wlan_status->mac[12], wlan_status->mac[13], \
            wlan_status->mac[15], wlan_status->mac[16] );

  init.instance_name = (char*)__strdup(temp_txt);
  app_log("instance_name: %s \n", init.instance_name);

  init.service_port = LOCAL_PORT;//inContext->appConfig->localServerPort;

#if 1
  temp_txt2 = __strdup_trans_dot(wlan_status->mac);
  sprintf(temp_txt, "MAC=%s.", temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  temp_txt2 = __strdup_trans_dot(MicoGetVer());
//  temp_txt2 = __strdup_trans_dot(MicoGetVer());
  sprintf(temp_txt, "%sFirmware Rev=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  temp_txt2 = __strdup_trans_dot(SOFT_VER);
  sprintf(temp_txt, "%sSoft Ver=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;
//  temp_txt2 = __strdup_trans_dot(HARDWARE_REVISION);
//  sprintf(temp_txt, "%sHardware Rev=%s.", temp_txt, temp_txt2);
//  free(temp_txt2);


//  sprintf(temp_txt, "%sMICO OS Rev=%s.", temp_txt, temp_txt2);
//  free(temp_txt2);

//  temp_txt2 = __strdup_trans_dot(MODEL);
  if (strcmp(inContext->appConfig->netdev.modelname,"")==0 || ((strlen(inContext->appConfig->netdev.modelname)>=sizeof(inContext->appConfig->netdev.modelname))))
	  temp_txt2 = __strdup_trans_dot(YL_MODELNAME);
  else
	  temp_txt2 = __strdup_trans_dot(inContext->appConfig->netdev.modelname);
  sprintf(temp_txt, "%sModel Name=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

  if (strcmp(inContext->appConfig->netdev.modelid,"")==0 || ((strlen(inContext->appConfig->netdev.modelid)>=sizeof(inContext->appConfig->netdev.modelid))))
	  temp_txt2 = __strdup_trans_dot(MODEL_ID);
  else
	  temp_txt2 = __strdup_trans_dot(inContext->appConfig->netdev.modelid);

  sprintf(temp_txt, "%sModel ID=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;
  app_log("temp_txt: %s \n", temp_txt);


//  temp_txt2 = __strdup_trans_dot(PROTOCOL);
  temp_txt2 = __strdup_trans_dot(YL_PROTOCOL);
  sprintf(temp_txt, "%sProtocol=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  temp_txt2 = NULL;

//  temp_txt2 = __strdup_trans_dot(MANUFACTURER);
  temp_txt2 = __strdup_trans_dot(YL_MANUFACTURER);
  sprintf(temp_txt, "%sManufacturer=%s.", temp_txt, temp_txt2);
  free(temp_txt2);
  app_log("temp_txt: %s \n", temp_txt);

  temp_txt2 = NULL;
  if ((strcmp(inContext->appConfig->netdev.uuid,"") != 0)&&(strlen(inContext->appConfig->netdev.uuid)<sizeof(inContext->appConfig->netdev.uuid))){
	  temp_txt2 = __strdup_trans_dot(inContext->appConfig->netdev.uuid);
	  sprintf(temp_txt, "%sSN=%s.", temp_txt, temp_txt2);
	  free(temp_txt2);
  }

  app_log("Pack data success !!!\n");

//  sprintf(temp_txt, "%sSeed=%lu.", temp_txt, mico_config->micoSystemConfig.seed);
#endif

  init.txt_record = (char*)__strdup(temp_txt);
  custom_log("mdns","init.txt_record: %s \n",init.txt_record);
  custom_log("mdns","host_name: %s \n",init.host_name);
  custom_log("mdns","instance_name: %s \n",init.instance_name);
  custom_log("mdns","service_port: %d \n",init.service_port);
  custom_log("mdns","service_name: %s \n",init.service_name);

  mdns_add_record( init, interface, 1500);
//  app_log("Server is end !!!");
  free(init.host_name);
  free(init.instance_name);
  free(init.txt_record);

exit:
  if(temp_txt) free(temp_txt);
  return err;

}



