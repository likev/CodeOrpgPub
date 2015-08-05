# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/06/03 15:19:54 $
# $Id: cpc102_tsk060.make,v 1.5 2014/06/03 15:19:54 steves Exp $
# $Revision: 1.5 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	@if [ -d $(DATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(DATADIR)); fi
	$(INSTALL) $(INSTDATFLAGS) alert_req.msg $(TOOLSCFGDIR)/alert_req.msg
	$(INSTALL) $(INSTDATFLAGS) bias_table.msg $(TOOLSCFGDIR)/bias_table.msg
	$(INSTALL) $(INSTDATFLAGS) one_time_req.msg $(TOOLSCFGDIR)/one_time_req.msg
	$(INSTALL) $(INSTDATFLAGS) awips_rps.msg $(TOOLSCFGDIR)/awips_rps.msg
	$(INSTALL) $(INSTDATFLAGS) rpccds_rps.msg $(TOOLSCFGDIR)/rpccds_rps.msg

BINMAKEFILES = nbtcp.mak

include $(MAKEINC)/make.parent_bin

