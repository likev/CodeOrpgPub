# RCS info
# $Author: port $
# $Locker:  $
# $Date: 1999/04/20 20:46:31 $
# $Id: cpc016_tsk003.make,v 1.4 1999/04/20 20:46:31 port Exp $
# $Revision: 1.4 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = stmtrprd.mak 

include $(MAKEINC)/make.parent_bin

