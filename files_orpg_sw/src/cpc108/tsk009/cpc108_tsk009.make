# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/08/10 20:40:39 $
# $Id: cpc108_tsk009.make,v 1.1 1999/08/10 20:40:39 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Manage RPG Tasks and Information

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_mngrpg.mak
include $(MAKEINC)/make.parent_bin
