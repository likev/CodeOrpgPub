# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/03/03 18:19:30 $
# $Id: cpc014_tsk017.make,v 1.2 2009/03/03 18:19:30 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = DP_Precip_4bit.mak

include $(MAKEINC)/make.parent_bin

