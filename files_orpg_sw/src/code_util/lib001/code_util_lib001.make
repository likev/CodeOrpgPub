# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/05/15 18:14:52 $
# $Id: code_util_lib001.make,v 1.2 2009/05/15 18:14:52 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

## CVG 9 moved files from tsk001/map to lib001

install:: 
	@if [ -d $(MAKETOP)/tools ]; then set +x; \
	else (set -x; $(MKDIR) $(MAKETOP)/tools); fi
	@if [ -d $(MAKETOP)/tools/cvg_map ]; then set +x; \
	else (set -x; $(MKDIR) $(MAKETOP)/tools/cvg_map); fi
	$(INSTALL) $(INSTDATFLAGS) us_map.dat.bz2 $(MAKETOP)/tools/cvg_map/us_map.dat.bz2
	$(INSTALL) $(INSTDATFLAGS) ak_map.dat.bz2 $(MAKETOP)/tools/cvg_map/ak_map.dat.bz2
	$(INSTALL) $(INSTDATFLAGS) hi_map.dat.bz2 $(MAKETOP)/tools/cvg_map/hi_map.dat.bz2

