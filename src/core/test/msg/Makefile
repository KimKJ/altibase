# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


OBJ_TARGETS            = msg_objs
# SHOBJ_TARGETS          = msg_shobjs

msg_objs_MSGS          = testMsg.msg
msg_objs_DEFINES       = ACP_CFG_DL_STATIC

# msg_shobjs_MSGS        = testMsg.msg
# msg_shobjs_DEFINES     = ACP_CFG_DL_EXPORT


GENERATED_HDR_DIR  = $(CORE_DIR)/include

#SHOBJ_TARGETS                = libtestAceMsg_shobj  		\
#							   libtestAceMsgError_shobj

# Needed when testing shared object
#SHLIB_TARGETS                = libtestAceMsg_shlib			\
#                               libtestAceMsgError_shlib

#libtestAceMsg_shobj_SRCS     = libtestAceMsg.c
#libtestAceMsg_shobj_DEFINES  = ACP_CFG_DL_EXPORT

#libtestAceMsgError_shobj_SRCS     = libtestAceMsgError.c
#libtestAceMsgError_shobj_DEFINES  = ACP_CFG_DL_EXPORT

#libtestAceMsg_shlib_NAME      = testAceMsg
#libtestAceMsg_shlib_OBJS      = $(libtestAceMsg_shobj_SRCS:.c=$(SHOBJ_SUF))
#libtestAceMsgError_shlib_NAME = testAceMsgError
#libtestAceMsgError_shlib_OBJS = $(libtestAceMsgError_shobj_SRCS:.c=$(SHOBJ_SUF))

# ifeq ($(ALTI_CFG_OS),WINDOWS)
# SO_LIBS                      += core_static.lib ctest_static.lib
# else
# SO_LIBS                      += $(BUILD_DIR)/../../lib/libcore.a $(BUILD_DIR)/../../lib/libctest.a
# endif

include $(MAKEFILES_DIR)/rule.mk
