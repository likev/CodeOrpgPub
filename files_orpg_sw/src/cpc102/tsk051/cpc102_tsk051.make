# RCS info
# $Author: dan $
# $Locker:  $
# $Date: 2000/01/25 21:34:26 $
# $Id: cpc102_tsk051.make,v 1.1 2000/01/25 21:34:26 dan Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = rms_read_msg.mak

include $(MAKEINC)/make.parent_bin

