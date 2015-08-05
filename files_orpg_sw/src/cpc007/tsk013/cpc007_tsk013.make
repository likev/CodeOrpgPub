# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2006/01/03 16:53:59 $
# $Id: cpc007_tsk013.make,v 1.3 2006/01/03 16:53:59 steves Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = bref8bit.mak

include $(MAKEINC)/make.parent_bin

