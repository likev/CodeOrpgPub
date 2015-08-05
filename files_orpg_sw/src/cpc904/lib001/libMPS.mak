# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2008/01/08 23:13:38 $
# $Id: libMPS.mak,v 1.5 2008/01/08 23:13:38 aamirn Exp $
# $Revision: 1.5 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES = -I..

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ifeq ($(ARCH),lnux_x86)
ALL_DEFINES = -DSVR4 -DALLAPI -DSYSVMSG -DLINUX -D_GNU_SOURCE
else
ALL_DEFINES = -DSVR4 -DALLAPI -DSYSVMSG
endif

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =


LIB_CSRCS =  MPSapi.c MPStcp.c MPSlocal.c \
	MPSudp.c \
	MPSlist.c \
	MPSutil.c any.c equalx25.c getaddrbyid.c getconfent.c getconfintent.c \
	getintbysnid.c getnettype.c getpadbyaddr.c getpadbystr.c getpadent.c \
	getsubnetbyi.c getsubnetbyn.c getsubnetent.c getxhostbyad.c getxhostbyna.c \
	getxhostent.c x25tosnid.c ipc_error.c ipc_sysv.c nc_client.c \
	nc_error.c padtos.c snidtox25.c \
	x25tos.c  stox25.c


LIB_TARGET = MPS

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)
