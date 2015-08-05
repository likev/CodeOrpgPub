# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 17:53:19 $
# $Id: rpgdbm.mak,v 1.7 2014/02/28 17:53:19 steves Exp $
# $Revision: 1.7 $
# $State: Exp $
# Template make description file for describing a C binary file

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# Append $(X_INCLUDES) when appropriate ...
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
# Append $(X_DEFINES) when appropriate ...
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
# Specify $(X_LIBPATH) when appropriate ...
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lorpg -linfr
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
EXTRA_LIBRARIES = 

# architecture specific system libraries.
# some system libraries, if needed.

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =


SRCS = rpgdbm_main.c rpgdbm_products.c rpgdbm_up.c

ADDITIONAL_OBJS =

TARGET = rpgdbm

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

# include $(MAKEINC)/make.cbin

# -include $(DEPENDFILE)

# The above two commented lines are the original replaced by follows

all:: $(ARCH)/$(TARGET)
$(TARGET): all

$(ARCH)/$(TARGET): rpgdbm_products.c rpg_sdqs.conf
	@if [ -d $(ARCH) ]; then set +x; \
	else (set -x; $(MKDIR) $(ARCH)); fi
	sdqs -oc ./rpg_sdqs.conf
	cp `which sdqs` $@

clean::
	$(RM) $(ARCH)/*.o $(ARCH)/$(TARGET)

install:: $(ARCH)/$(TARGET)
	@if [ -d $(BINDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(BINDIR)); fi
	$(INSTALL) $(INSTPGMFLAGS) $< $(BINDIR)/$(TARGET)

