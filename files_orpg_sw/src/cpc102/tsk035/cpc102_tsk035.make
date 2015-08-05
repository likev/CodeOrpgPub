# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2005/08/03 21:27:54 $
# $Id: cpc102_tsk035.make,v 1.1 2005/08/03 21:27:54 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Hydrometeorological Tasks

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = create_hydro_files.mak
include $(MAKEINC)/make.parent_bin
