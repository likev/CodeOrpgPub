# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/03/03 18:27:24 $
# $Id: cpc023_tsk001.make,v 1.2 2009/03/03 18:27:24 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hca.mak

include $(MAKEINC)/make.parent_bin

