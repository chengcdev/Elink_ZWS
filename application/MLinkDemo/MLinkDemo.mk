#
#  UNPUBLISHED PROPRIETARY SOURCE CODE
#  Copyright (c) 2017  Inc.
#

NAME := App_MLinkDemo

$(NAME)_SOURCES := MLinkAppEntrance.c \
                   MLinkBonjour.c \
                   main/main_logic.c \
                   main/main_server_disc_msg.c \
                   main/main_server_ctrl.c \
                   main/main_server_manage.c \
                   main/main_server_sys.c \
                   main/main_alarm.c \
                   mqtt/mqtt_client.c \
                   ota/ota.c \

# sys param
$(NAME)_SOURCES += MLinkGwSys/MLinkSysStatus.c \


# json deal
$(NAME)_SOURCES += json/json_public.c \
                   json/json_checker.c \
                   json/json_sys_deal.c \
                   json/json_disc_deal.c \
                   json/json_ctrl_deal.c \
                   json/json_manage_deal.c \
                   json/json_msg_deal.c

# coap logic
$(NAME)_SOURCES += coap/coap_server_deal.c \
				   coap/coap_service.c \
				   coap/coap_crypto.c \
				   coap/coap_client_api.c \
				   coap/coap_logic.c \

# cloud module
$(NAME)_SOURCES += cloud/cloud_logic.c \

# rf433 module
#$(NAME)_SOURCES += rf433/rf433_logic.c \

# zigbee module
#$(NAME)_SOURCES += zigbee/zigbee_logic.c \

#ble module
$(NAME)_SOURCES += ble/bluetooth_logic.c \
				   flash/storage_ble_logic.c \

# storage module 
$(NAME)_SOURCES += flash/flash.c \
                   flash/flash_storage_object.c \
                   flash/flash_storage_distribute.c \

# ntp module
$(NAME)_SOURCES += sntp_client/sntp_client.c \

# uart module
$(NAME)_SOURCES += uart/uart_logic.c \
				   uart/uart_packet.c \
				   uart/DTTable.c \

# MLinkPublic
$(NAME)_SOURCES += MLinkPublic/MLinkPublic.c \

# MLinkGpio
$(NAME)_SOURCES += MLinkGpio/MLinkGpio.c \
			     = MLinkGpio/MLinkLedLogic.c \

# cron
#$(NAME)_SOURCES += cron/cron.c \

# eLink module
$(NAME)_SOURCES += eLink/common/elink_crypto.c \
					eLink/elink_protocol_chanel.c \
					eLink/elink_logic_devchanel_deal.c \
				   eLink/elink_logic.c \
				   
# eLink time	
$(NAME)_SOURCES += time/time.c 	
	   			   
# queue
$(NAME)_SOURCES += queue/queue.c 



$(NAME)_INCLUDES := ../../mico-os/libraries/protocols/libcoap/include \
					coap \
					json


                   
$(NAME)_COMPONENTS := protocols/mqtt

$(NAME)_COMPONENTS += protocols/libcoap

$(NAME)_COMPONENTS += daemons/ota_server

$(NAME)_COMPONENTS += protocols/SNTP

$(NAME)_COMPONENTS += utilities/base64


#DEFS = -DTEST_ADD -DTEST_SUB=1 
#(NAME)_CFLAGS += $(DEFS)
                   