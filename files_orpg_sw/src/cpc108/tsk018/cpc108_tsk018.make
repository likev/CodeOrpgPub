# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2000/11/02 20:55:19 $
# $Id: cpc108_tsk018.make,v 1.1 2000/11/02 20:55:19 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: RDA ALARMS MNTTSK code

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_rda_alarms.mak
include $(MAKEINC)/make.parent_bin
