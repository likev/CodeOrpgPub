# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2005/04/06 14:25:05 $
# $Id: cpc102_tsk053.make,v 1.2 2005/04/06 14:25:05 jing Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = print_dea.mak \
	       edit_dea.mak

include $(MAKEINC)/make.parent_bin

