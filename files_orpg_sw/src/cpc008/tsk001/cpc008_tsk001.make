# RCS info
# $Author: port $
# $Locker:  $
# $Date: 1999/04/20 15:33:53 $
# $Id: cpc008_tsk001.make,v 1.3 1999/04/20 15:33:53 port Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = alerting.mak 

include $(MAKEINC)/make.parent_bin

