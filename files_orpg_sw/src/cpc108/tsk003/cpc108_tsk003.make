# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2002/12/04 16:45:36 $
# $Id: cpc108_tsk003.make,v 1.2 2002/12/04 16:45:36 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

# Maintenance Task: RDA Alarms Table

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_rda_alarms_tbl.mak
include $(MAKEINC)/make.parent_bin
