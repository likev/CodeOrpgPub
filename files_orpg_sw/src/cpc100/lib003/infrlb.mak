# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:26 $
# $Id: infrlb.mak,v 1.16 2014/02/28 18:15:26 steves Exp $
# $Revision: 1.16 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# NOTE: as-delivered Intel Red Hat Linux 5.1 does not support streamio(7I)
#       ioctl(fd, I_SETSIG, ...)
#
LOCAL_DEFINES =  -DLB_NTF_SERVICE

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

LIB_CSRCS = lb_clear.c \
	lb_common.c \
	lb_list.c \
	lb_lock.c \
	lb_mark.c \
	lb_open.c \
	lb_read.c \
	lb_seek.c \
	lb_stat.c \
	lb_write.c \
	lb_notify.c \
	lb_sms.c 

LIB_TARGET = infrlb

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

libinstall::
	rm -rf $(LIBDIR)/lib$(LIB_TARGET).*

-include $(DEPENDFILE)
