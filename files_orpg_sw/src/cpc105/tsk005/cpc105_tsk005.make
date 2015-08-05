# RCS info
# $Author: john $
# $Locker:  $
# $Date: 1998/05/18 19:27:28 $
# $Id: cpc105_tsk005.make,v 1.1 1998/05/18 19:27:28 john Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = cm_tcp.mak

include $(MAKEINC)/make.parent_bin

