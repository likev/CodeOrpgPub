# RCS info
# $Author: priegni $
# $Locker:  $
# $Date: 2000/03/08 18:17:45 $
# $Id: cpc108_tsk017.make,v 1.1 2000/03/08 18:17:45 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: General Status Message (GSM)

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_gsm.mak
include $(MAKEINC)/make.parent_bin
