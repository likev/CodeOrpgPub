# RCS info
# $Author: cm $
# $Locker:  $
# $Date: 2000/03/09 15:51:51 $
# $Id: cpc904_tsk002.make,v 1.1 2000/03/09 15:51:51 cm Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = dnetd.mak

include $(MAKEINC)/make.parent_bin

