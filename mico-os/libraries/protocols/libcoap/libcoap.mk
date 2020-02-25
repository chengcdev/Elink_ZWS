#
#  PROPRIETARY SOURCE CODE
#  Copyright (c) 2017  Inc.
#
#  The contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of  Corporation.
#

NAME := Lib_coap

GLOBAL_INCLUDES := .

$(NAME)_SOURCES := src/pdu.c \
					src/net.c \
					src/debug_coap.c \
					src/encode.c \
					src/uri.c \
					src/coap_list.c \
					src/resource.c \
					src/hashkey.c \
					src/str.c \
					src/option.c \
					src/async.c \
					src/subscribe.c \
					src/block.c 

$(NAME)_INCLUDES := include \

#GLOBAL_INCLUDES := ./ \
#					./include
					