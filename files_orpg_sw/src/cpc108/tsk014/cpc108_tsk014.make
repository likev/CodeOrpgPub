# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2005/03/25 22:34:06 $
# $Id: cpc108_tsk014.make,v 1.1 2005/03/25 22:34:06 jing Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Product Distribution Tasks and Datastores

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_switch_orda.mak
include $(MAKEINC)/make.parent_bin
