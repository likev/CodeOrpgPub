# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/08/06 20:01:50 $
# $Id: cpc108_tsk008.make,v 1.1 1999/08/06 20:01:50 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Inter-Task Communication files (ITCs)

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_itcs.mak
include $(MAKEINC)/make.parent_bin
