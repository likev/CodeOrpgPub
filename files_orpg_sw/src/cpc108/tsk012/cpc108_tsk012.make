# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/08/30 20:59:49 $
# $Id: cpc108_tsk012.make,v 1.1 1999/08/30 20:59:49 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Load Shed Tasks and Datastores MNTTSK

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_loadshed.mak
include $(MAKEINC)/make.parent_bin
