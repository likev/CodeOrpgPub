# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/10/07 16:35:48 $
# $Id: cpc102_tsk022.make,v 1.4 2011/10/07 16:35:48 ccalvert Exp $
# $Revision: 1.4 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#install::
#	@if [ -d $(TOOLSCFGEXTDIR) ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGEXTDIR)); fi
#	$(INSTALL) $(INSTDATFLAGS) product_attr_table.gauge_radar $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) product_generation_tables.gauge_radar $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_attr_table.gauge_radar $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_tables.gauge_radar $(TOOLSCFGEXTDIR)
#	@if [ -d $(TOOLSCFGDIR)/dea ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGDIR)/dea); fi
#	$(INSTALL) $(INSTDATFLAGS) gauge_radar.alg $(TOOLSCFGDIR)/dea
#	@if [ -d $(TOOLSMAINDIR)/man/cat1 ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSMAINDIR)/man/cat1); fi
#	$(INSTALL) $(INSTDATFLAGS) gauge_radar.1 $(TOOLSMAINDIR)/man/cat1

BINMAKEFILES = gauge_radar.mak

include $(MAKEINC)/make.parent_bin
