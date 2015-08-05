# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2009/03/20 20:18:22 $
# $Id: cpc104_lib004.make,v 1.25 2009/03/20 20:18:22 jing Exp $
# $Revision: 1.25 $
# $State: Exp $

# 

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(INSTALLTOP) ]; then set +x; \
	else (set -x; $(MKDIR) $(INSTALLTOP)); fi
	$(INSTALL) $(INSTDATFLAGS) rssd.conf $(INSTALLTOP)/.rssd.conf
	$(INSTALL) $(INSTDATFLAGS) rssd.conf.faa $(INSTALLTOP)/.rssd.conf.faa
	@if [ -d $(CFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)); fi
	$(INSTALL) $(INSTDATFLAGS) mscf1.conf $(CFGDIR)/mscf1.conf
	$(INSTALL) $(INSTDATFLAGS) redundant.cfg $(CFGDIR)/redundant.cfg
	$(INSTALL) $(INSTDATFLAGS) mps.x25.128 $(CFGDIR)/mps.x25.128
	$(INSTALL) $(INSTDATFLAGS) mps.lapb.128 $(CFGDIR)/mps.lapb.128
	$(INSTALL) $(INSTDATFLAGS) mps.wan.128 $(CFGDIR)/mps.wan.128
	$(INSTALL) $(INSTDATFLAGS) mps.x25.512 $(CFGDIR)/mps.x25.512 
	$(INSTALL) $(INSTDATFLAGS) mps.lapb.512 $(CFGDIR)/mps.lapb.512
	$(INSTALL) $(INSTDATFLAGS) mps.wan.512 $(CFGDIR)/mps.wan.512
	$(INSTALL) $(INSTDATFLAGS) mps.conf $(CFGDIR)/mps.conf
	$(INSTALL) $(INSTDATFLAGS) tcp.conf $(CFGDIR)/tcp.conf


