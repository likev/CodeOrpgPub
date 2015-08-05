# RCS info
# $Author: priegni $
# $Locker:  $
# $Date: 1999/10/18 14:19:37 $
# $Id: cpc102_tsk046.make,v 1.1 1999/10/18 14:19:37 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = test_faa.mak

include $(MAKEINC)/make.parent_bin

