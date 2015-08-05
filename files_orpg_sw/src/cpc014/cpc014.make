# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/05/14 16:17:16 $
# $Id: cpc014.make,v 1.13 2008/05/14 16:17:16 ccalvert Exp $
# $Revision: 1.13 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This is the parent makefile for CPC014 precipitation Products

# Following builds two shared object files ... i.e., two object files
# linked-in by two or more tasks within this cpc ...
install::
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	$(MAKE) $(MFLAGS) -f cpc014_shared.mak all

all::
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	$(MAKE) $(MFLAGS) -f cpc014_shared.mak all

debugall::
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	$(MAKE) $(MFLAGS) -f cpc014_shared.mak debugall

clean::
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	$(RM) ./$(ARCH)/*.f ./$(ARCH)/*.o ./$(ARCH)/*.$(DBGOBJ_EXT)

#   tsk001 Create Echo Tops Product
#   tsk003 Create Hybrid Scan Reflectivity Product
#   tsk004 Create Vertically Integrated Liquid Water Product
#   tsk005 Create Layer Composite Reflectivity Product
#   tsk006 Create Precipitation Products
#   tsk007 Create User-Selectable Precipitation Products
#   tsk008 Create Layer Composite Reflectivity Polar Grid AP Removed
#   tsk009 Create Layer Composite Reflectivity Product AP Removed
#   tsk011 Create User-Selectable Layer Composite Reflectivity
#   lib001 Library for Snow Algorithm Product Tasks (tsk013, tsk014)  
#   tsk013 Create Snow Algorithm Product  
#   tsk014 Create Snow Algorithm Users Product  
#   lib002 Library for Dual-Pol Product Tasks
#   tsk016 Create Dual-Pol 8-bit Products
#   tsk017 Create Dual-Pol 4-bit Products

SUBDIRS =	tsk001 \
		tsk003 \
		tsk004 \
		tsk005 \
		tsk006 \
		tsk007 \
		tsk008 \
		tsk009 \
		tsk011 \
		lib001 \
		tsk013 \
		tsk014 \
		lib002 \
		tsk016 \
		tsk017

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs

