# RCS info
# $Author: cm $
# $Locker:  $
# $Date: 2014/09/03 12:25:17 $
# $Id: cpc104_lib009.make,v 1.13 2014/09/03 12:25:17 cm Exp $
# $Revision: 1.13 $
# $State: Exp $

# 

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(CFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)); fi
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	@if [ -d $(TOOLSCFGDIR)/site_templates ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)/site_templates); fi
	$(INSTALL) $(INSTDATFLAGS) hosts.allow.tmpl $(TOOLSCFGDIR)/site_templates/hosts.allow.tmpl
	$(INSTALL) $(INSTDATFLAGS) hosts.tmpl $(TOOLSCFGDIR)/site_templates/hosts.tmpl
	$(INSTALL) $(INSTDATFLAGS) hosts.tmpl $(TOOLSCFGDIR)/comms/hosts.tmpl
	$(INSTALL) $(INSTDATFLAGS) ifcfg-eth0.0.tmpl $(TOOLSCFGDIR)/site_templates/ifcfg-eth0.0.tmpl
	$(INSTALL) $(INSTDATFLAGS) ifcfg-eth0.tmpl $(TOOLSCFGDIR)/site_templates/ifcfg-eth0.tmpl
	$(INSTALL) $(INSTDATFLAGS) ldmd.conf.tmpl $(TOOLSCFGDIR)/site_templates/ldmd.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) mps.conf.tmpl $(TOOLSCFGDIR)/site_templates/mps.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) mscf.conf.tmpl $(TOOLSCFGDIR)/site_templates/mscf.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) mscf_npa.conf.tmpl $(TOOLSCFGDIR)/site_templates/mscf_npa.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) network.tmpl $(TOOLSCFGDIR)/site_templates/network.tmpl
	$(INSTALL) $(INSTDATFLAGS) ntp.conf.tmpl $(TOOLSCFGDIR)/site_templates/ntp.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) rc.local.tmpl $(TOOLSCFGDIR)/site_templates/rc.local.tmpl
	$(INSTALL) $(INSTDATFLAGS) rssd.conf.tmpl $(TOOLSCFGDIR)/site_templates/rssd.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) tcp.conf.tmpl $(TOOLSCFGDIR)/site_templates/tcp.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) ifcfg-eth1.tmpl $(TOOLSCFGDIR)/site_templates/ifcfg-eth1.tmpl
	$(INSTALL) $(INSTDATFLAGS) ifcfg-ppp0.tmpl $(TOOLSCFGDIR)/site_templates/ifcfg-ppp0.tmpl
	$(INSTALL) $(INSTDATFLAGS) site_data.tmpl $(TOOLSCFGDIR)/site_templates/site_data.tmpl
	$(INSTALL) $(INSTDATFLAGS) hub_ios.tmpl $(TOOLSCFGDIR)/site_templates/hub_ios.tmpl
	$(INSTALL) $(INSTDATFLAGS) lan_ios.tmpl $(TOOLSCFGDIR)/site_templates/lan_ios.tmpl
	$(INSTALL) $(INSTDATFLAGS) rtr_ios.tmpl $(TOOLSCFGDIR)/site_templates/rtr_ios.tmpl
	$(INSTALL) $(INSTDATFLAGS) rtr_3640_ios.tmpl $(TOOLSCFGDIR)/site_templates/rtr_3640_ios.tmpl
	$(INSTALL) $(INSTDATFLAGS) ifcfg-eth0.1.tmpl $(TOOLSCFGDIR)/site_templates/ifcfg-eth0.1.tmpl
	$(INSTALL) $(INSTDATFLAGS) logrotate.conf.tmpl $(TOOLSCFGDIR)/site_templates/logrotate.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) netlog.tmpl $(TOOLSCFGDIR)/site_templates/netlog.tmpl
	$(INSTALL) $(INSTDATFLAGS) syslog.conf.tmpl $(TOOLSCFGDIR)/site_templates/syslog.conf.tmpl
	$(INSTALL) $(INSTDATFLAGS) syslog.tmpl $(TOOLSCFGDIR)/site_templates/syslog.tmpl
	$(INSTALL) $(INSTDATFLAGS) hub2_ios.tmpl $(TOOLSCFGDIR)/site_templates/hub2_ios.tmpl
