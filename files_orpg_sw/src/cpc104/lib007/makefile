
# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/01/24 19:24:50 $
# $Id: cpc104_lib007.make,v 1.8 2013/01/24 19:24:50 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

# 7/25/03 - Chris Calvert.  Install task VCP files into the appropriate subdir

VCPDATADIR = vcp

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(CFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)); fi
	@if [ -d $(CFGDIR)/$(VCPDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)/$(VCPDATADIR)); fi
	$(INSTALL) $(INSTDATFLAGS) vcp_11 $(CFGDIR)/$(VCPDATADIR)/vcp_11
	$(INSTALL) $(INSTDATFLAGS) vcp_12 $(CFGDIR)/$(VCPDATADIR)/vcp_12
	$(INSTALL) $(INSTDATFLAGS) vcp_21 $(CFGDIR)/$(VCPDATADIR)/vcp_21
	$(INSTALL) $(INSTDATFLAGS) vcp_121 $(CFGDIR)/$(VCPDATADIR)/vcp_121
	$(INSTALL) $(INSTDATFLAGS) vcp_31 $(CFGDIR)/$(VCPDATADIR)/vcp_31
	$(INSTALL) $(INSTDATFLAGS) vcp_32 $(CFGDIR)/$(VCPDATADIR)/vcp_32
	$(INSTALL) $(INSTDATFLAGS) vcp_211 $(CFGDIR)/$(VCPDATADIR)/vcp_211
	$(INSTALL) $(INSTDATFLAGS) vcp_212 $(CFGDIR)/$(VCPDATADIR)/vcp_212
	$(INSTALL) $(INSTDATFLAGS) vcp_221 $(CFGDIR)/$(VCPDATADIR)/vcp_221
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_11 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_11
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_12 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_12
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_21 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_21
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_121 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_121
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_31 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_31
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_32 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_32
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_211 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_211
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_212 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_212
	$(INSTALL) $(INSTDATFLAGS) KLGX_vcp_221 $(CFGDIR)/$(VCPDATADIR)/KLGX_vcp_221
