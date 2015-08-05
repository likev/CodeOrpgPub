# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:06:21 $
# $Id: librpg.mak,v 1.19 2014/03/07 19:06:21 steves Exp $
# $Revision: 1.19 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
LOCAL_INCLUDES =
# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# In case if there are any local defines.
LOCAL_DEFINES = -DRPG_LIBRARY

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# This list has all the includes that are needed for the compile.
# Local copies of a file are given preference over the system 
# location.
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS = 


# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
FC_LOCAL_INCLUDES = 
FPP_LOCAL_INCLUDES = ./prod_gen_msg.inc
# In case if there are any local fortran defines.
FC_LOCAL_DEFINES =
FPP_LOCAL_DEFINES = ./PROD_GEN_MSG.INC

# This list has all the includes that are needed for the compile.
# Local copies of a file are given preference over the system 
# location.
FC_ALL_INCLUDES = $(FC_STD_INCLUDES) $(FC_LOCALTOP_INCLUDES) \
						$(FC_TOP_INCLUDES) $(FC_LOCAL_INCLUDES)
FPP_ALL_INCLUDES = $(FPP_STD_INCLUDES) $(FPP_LOCALTOP_INCLUDES) \
						$(FPP_TOP_INCLUDES) $(FPP_LOCAL_INCLUDES)

# This is a list of all fortran defines.
FC_ALL_DEFINES = $(FC_STD_DEFINES) $(FC_OS_DEFINES) $(FC_LOCAL_DEFINES)
FPP_ALL_DEFINES = $(FPP_STD_DEFINES) $(FPP_OS_DEFINES) $(FPP_LOCAL_DEFINES)


FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEPENDFLAGS =

SHRDLIBLD_SEARCHLIBS = 

LIB_FSRCS =	rpg_a311.ftn \
	rpg_a312.ftn \
	rpg_a3cm.ftn \
	rpg_orpg_mem_addr.ftn \
	rpg_os32_for.ftn \
	rpg_scan_sum_for.ftn \
        rpg_task_init.ftn

LIB_CSRCS =	rpg_adaptation.c \
	rpg_adpt_data_elems.c \
	rpg_data_access.c \
	rpg_event_services.c \
	rpg_inbuf.c \
	rpg_init.c \
	rpg_itc.c \
	rpg_os32.c \
	rpg_os32_sysio.c \
	rpg_outbuf.c \
	rpg_prod_compress.c \
	rpg_prod_request.c \
	rpg_prod_support.c \
	rpg_scan_sum.c \
	rpg_timer_services.c \
	rpg_wait_act.c \
	rpg_abort_processing.c \
	rpg_legacy_prod.c \
        rpg_vcp_info.c \
	rpg_site_info_callback_fx.c

LIB_TARGET = rpg

clean::
	$(RM) $(ARCH)/*.f

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)

