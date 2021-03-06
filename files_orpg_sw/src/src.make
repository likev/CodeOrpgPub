# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2008/12/04 19:41:12 $
# $Id: src.make,v 1.56 2008/12/04 19:41:12 steves Exp $
# $Revision: 1.56 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#   Copy a few system man pages that are referenced from
#   documentation drawings
install::

# cpc001 Master System Control Function (MSCF a.k.a. HCI)
# cpc002 Product Distribution
# cpc004 Radar Data Acquisition
# cpc005 Rain Gage Data Acquisition
# cpc006 Archive III
# cpc007 Base Data Products
# cpc008 Message Processing
# cpc010 Machine Intelligent Gust Front Algorithm (MIGFA)
# cpc013 Precipitation Algorithms
# cpc014 Precipitation Products
# cpc015 Storm Cell Identification and Tracking Algorithms
# cpc016 Storm Products
# cpc017 Kinematic Algorithms
# cpc018 Kinematic Products
# cpc019 Manage Redundant
# cpc020 RMS
# cpc022 NEXRAD Turbulence Detection Algorithm (NTDA)
# cpc023 Dual-Pol Algorithms
# cpc024 Dual-Pol Products
# cpc026 Adaptation Data
# cpc100 Generic Libraries
# cpc101 ORPG Libraries
# cpc102 ORPG Tools
# cpc103 ORPG Utilities
# cpc104 ORPG Data & Configuration Files
# cpc105 ORPG Communications Managers
# cpc106 OBJECT/byte conversion library
# cpc108 Maintenance Tasks Libraries and Binaries
# cpc110 MSCF
# cpc111 Manage RPG
# cpc112 BDDS
# cpc204 ORDA
# cpc904 UconX api Library
# code_util - CODE Utilities
# CPCI26_CPC100 Adaptation data merge
# ORDA - ORDA files for MSCF    

ifeq ($(OPUP_BLD),yes)
SUBDIRS = 	cpc100 \
		cpc105 \
		cpc103
else
SUBDIRS = 	cpc002 \
		cpc004 \
		cpc005 \
		cpc006 \
		cpc007 \
		cpc008 \
		cpc013 \
		cpc014 \
		cpc015 \
		cpc016 \
		cpc017 \
		cpc018 \
		cpc019 \
		cpc020 \
		cpc023 \
		cpc024 \
		cpc100 \
		cpc101 \
		cpc001 \
		cpc026 \
		cpc102 \
		cpc103 \
		cpc104 \
		cpc105 \
		cpc108 \
		cpc110 \
		cpc111 \
		cpc112 \
		cpc904 \
		code_util \
		orda 
endif

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs
