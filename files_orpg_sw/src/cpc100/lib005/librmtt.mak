# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:29 $
# $Id: librmtt.mak,v 1.11 2014/02/28 18:15:29 steves Exp $
# $Revision: 1.11 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DTHREADED 
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

LIB_CSRCS =	rmt_cl_register.c \
		rmt_encrypt.c \
		rmt_get_client.c \
		rmt_port_number.c \
		rmt_client.c \
		rmt_ufi.c \
		rmt_server.c \
		rmt_secu_cl.c \
		rmt_secu_sv.c \
		rmt_sock_cl.c \
		rmt_sock_sv.c \
		rmt_sv_register.c \
		rmt_msg_cl.c \
		rmt_msg_sv.c \
		rmt_sock_shared.c

LIBT_TARGET = rmtt

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

libinstall::
	rm -rf $(LIBDIR)/lib$(LIBT_TARGET).*

-include $(DEPENDFILE)
