# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2006/01/03 16:55:21 $
# $Id: cpc008_tsk002.make,v 1.5 2006/01/03 16:55:21 steves Exp $
# $Revision: 1.5 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = radcdmsg.mak

include $(MAKEINC)/make.parent_bin

