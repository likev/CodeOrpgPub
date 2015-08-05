# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/02/24 23:20:03 $
# $Id: cpc104_lib002.make,v 1.13 2005/02/24 23:20:03 ccalvert Exp $
# $Revision: 1.13 $
# $State: Exp $

# This is the parent make description file for Task Data Files
# The data directory needs to be made here for the build_install_files script

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(TOOLSDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSDATADIR)); fi
	@if [ -d $(DATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(DATADIR)); fi
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	$(INSTALL) $(INSTDATFLAGS) ktlx.map $(TOOLSDATADIR)/ktlx.map
	$(INSTALL) $(INSTDATFLAGS) klwx.map $(TOOLSDATADIR)/klwx.map
	$(INSTALL) $(INSTDATFLAGS) change_radar.dat $(TOOLSCFGDIR)/change_radar.dat
