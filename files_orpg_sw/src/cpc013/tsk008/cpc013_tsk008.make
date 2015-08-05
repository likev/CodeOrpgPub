# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2004/01/21 20:12:16 $
# $Id: cpc013_tsk008.make,v 1.1 2004/01/21 20:12:16 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = snowaccum.mak 

include $(MAKEINC)/make.parent_bin

