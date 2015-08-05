# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/07/08 21:13:00 $
# $Id: liborpg.mak,v 1.80 2014/07/08 21:13:00 steves Exp $
# $Revision: 1.80 $
# $State: Exp $

# The make description file for the New RPG Library (liborpg)

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = -I$(MAKETOP)/src/cpc001

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# Append X_INCLUDES to the list of includes if you want to 
# use include files for X and motif.
#ifdef 
X_INCLUDED = /usr/X11R6/include
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(X_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(XOPEN_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = -g $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

LOCAL_LIBRARIES = -linfr -lcrypt
SHRDLIBLD_SEARCHLIBS = -lcrypt -lm

LIB_CSRCS =	orpgcfg.c \
		orpgcmi.c \
		orpgcmp.c \
		orpgmgr.c \
		orpgda.c \
                orpgtask.c \
		orpginfo.c \
		orpginfo_statefl.c \
		orpginfo_statefl_shared.c \
		orpgload.c \
		orpgtat.c \
		orpgalt.c \
		orpgdbm.c \
		orpgmisc.c \
		orpgumc.c \
		orpgumc_rda.c \
		orpgdat_api.c \
		orpgpat.c \
		orpgpgt.c \
		orpgrda.c \
		orpgvst.c \
		orpgnbc.c \
                orpgbdr.c \
                orpggst_product_info.c \
                orpggst_rda_info.c \
		orpggst_rpg_info.c \
		orpgred.c \
		orpgedlock.c \
		orpgsmi.c \
		orpgrat.c \
		orpgrda_adpt.c \
		orpgvcp.c \
		orpgprq.c \
		orpgadpt.c \
		orpgsite.c \
                orpgsum_scan_sum.c \
		orpg_adptu.c \
		orpgccz.c \
		orpggdr.c \
		orpgsails.c

		
LIB_TARGET = orpg

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH) orpgsmi.c
DEPENDFLAGS = -f $(DEPENDFILE)

orpgsmi.c: orpgsmi.h
	smipp -o "-I. -I$(MAKETOP)/lib/include -I$(MAKETOP)/include $(LOCAL_INCLUDES) -DCALLED_FROM_SMIPP" orpgsmi.h

liball::
	makedepend -I. -I$(MAKETOP)/lib/include -I$(MAKETOP)/include -I/usr/X11R6/include $(LOCAL_INCLUDES) -o.c orpgsmi.h

clean::
	rm -f orpgsmi.c

include $(MAKEINC)/make.cflib

-include ./makedepend.$(ARCH)
