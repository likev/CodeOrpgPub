# RCS info
# $Author: eforren $
# $Locker:  $
# $Date: 2000/01/26 15:27:06 $
# $Id: cpc108_tsk015.make,v 1.1 2000/01/26 15:27:06 eforren Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =  le_pipe.mak 

include $(MAKEINC)/make.parent_bin

