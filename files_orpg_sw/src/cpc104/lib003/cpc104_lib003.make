# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2012/11/19 15:54:11 $
# $Id: cpc104_lib003.make,v 1.90 2012/11/19 15:54:11 ccalvert Exp $
# $Revision: 1.90 $
# $State: Exp $

# 

DEAUDATADIR = dea

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(CFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)); fi
	@if [ -d $(CFGDIR)/$(DEAUDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)/$(DEAUDATADIR)); fi
	@if [ -d $(TOOLSDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSDATADIR)); fi
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	@if [ -d $(TOOLSCFGDIR)/$(DEAUDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)/$(DEAUDATADIR)); fi
	$(INSTALL) $(INSTDATFLAGS) rda_alarms_table $(CFGDIR)/rda_alarms_table
	$(INSTALL) $(INSTDATFLAGS) orda_alarms_table $(CFGDIR)/orda_alarms_table
	$(INSTALL) $(INSTDATFLAGS) product_attr_table $(CFGDIR)/product_attr_table
	$(INSTALL) $(INSTDATFLAGS) product_attr_table $(TOOLSDATADIR)/product_attr_table
	$(INSTALL) $(INSTDATFLAGS) data_attr_table $(CFGDIR)/data_attr_table
	$(INSTALL) $(INSTDATFLAGS) task_attr_table $(CFGDIR)/task_attr_table
	$(INSTALL) $(INSTDATFLAGS) owr.conf $(CFGDIR)/owr.conf
	$(INSTALL) $(INSTDATFLAGS) task_tables $(CFGDIR)/task_tables
	$(INSTALL) $(INSTDATFLAGS) resource_table $(CFGDIR)/resource_table
	$(INSTALL) $(INSTDATFLAGS) rpg_options $(CFGDIR)/rpg_options
	$(INSTALL) $(INSTDATFLAGS) hci_task_data $(CFGDIR)/hci_task_data
	$(INSTALL) $(INSTDATFLAGS) site_info.dea $(TOOLSCFGDIR)/site_info.dea
	$(INSTALL) $(INSTDATFLAGS) comms_info.dea $(CFGDIR)/$(DEAUDATADIR)/comms_info.dea
	$(INSTALL) $(INSTDATFLAGS) site_info.gen $(CFGDIR)/$(DEAUDATADIR)/site_info.gen
	$(INSTALL) $(INSTDATFLAGS) rda_config.def $(CFGDIR)/rda_config.def
	$(INSTALL) $(INSTDATFLAGS) prod_params.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/prod_params.dea
	$(INSTALL) $(INSTDATFLAGS) pbd.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/pbd.dea
	$(INSTALL) $(INSTDATFLAGS) vcp_sequence.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/vcp_sequence.dea
	$(INSTALL) $(INSTDATFLAGS) clutter_censor_zones.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/clutter_censor_zones.dea
	$(INSTALL) $(INSTDATFLAGS) alert_table.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/alert_table.dea
	$(INSTALL) $(INSTDATFLAGS) comms_link.conf $(TOOLSCFGDIR)/comms_link.conf
	$(INSTALL) $(INSTDATFLAGS) comms_link.conf.none $(TOOLSCFGDIR)/comms_link.conf.none
	$(INSTALL) $(INSTDATFLAGS) product_user_table $(TOOLSCFGDIR)/product_user_table
	$(INSTALL) $(INSTDATFLAGS) service_class_table $(TOOLSCFGDIR)/service_class_table
	$(INSTALL) $(INSTDATFLAGS) hci_passwords $(TOOLSCFGDIR)/hci_passwords
	$(INSTALL) $(INSTDATFLAGS) load_shed_table $(CFGDIR)/load_shed_table
	$(INSTALL) $(INSTDATFLAGS) product_generation_tables $(CFGDIR)/product_generation_tables
	$(INSTALL) $(INSTDATFLAGS) user_profiles $(TOOLSCFGDIR)/user_profiles
