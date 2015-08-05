
# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/06/23 19:37:26 $
# $Id: cpc104_lib006.make,v 1.27 2014/06/23 19:37:26 steves Exp $
# $Revision: 1.27 $
# $State: Exp $

# 7/25/03 - Chris Calvert.  Install task DEAU files into the appropriate subdir

DEAUDATADIR = dea

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	@if [ -d $(TOOLSCFGDIR)/$(DEAUDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)/$(DEAUDATADIR)); fi
	$(INSTALL) $(INSTDATFLAGS) Archive_II.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/Archive_II.alg
	$(INSTALL) $(INSTDATFLAGS) Nonoperational.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/Nonoperational.alg
	$(INSTALL) $(INSTDATFLAGS) dpprep.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/dpprep.alg
	$(INSTALL) $(INSTDATFLAGS) bragg.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/bragg.alg
	$(INSTALL) $(INSTDATFLAGS) dp_precip.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/dp_precip.alg
	$(INSTALL) $(INSTDATFLAGS) hail.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hail.alg
	$(INSTALL) $(INSTDATFLAGS) hca.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hca.alg
	$(INSTALL) $(INSTDATFLAGS) hydromet_acc.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hydromet_acc.alg
	$(INSTALL) $(INSTDATFLAGS) hydromet_adj.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hydromet_adj.alg
	$(INSTALL) $(INSTDATFLAGS) hydromet_prep.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hydromet_prep.alg
	$(INSTALL) $(INSTDATFLAGS) hydromet_rate.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/hydromet_rate.alg
	$(INSTALL) $(INSTDATFLAGS) layer_reflectivity.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/layer_reflectivity.alg
	$(INSTALL) $(INSTDATFLAGS) mda.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/mda.alg
	$(INSTALL) $(INSTDATFLAGS) mlda.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/mlda.alg
	$(INSTALL) $(INSTDATFLAGS) mode_select.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/mode_select.alg
	$(INSTALL) $(INSTDATFLAGS) mpda.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/mpda.alg
	$(INSTALL) $(INSTDATFLAGS) prfselect.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/prfselect.alg
	$(INSTALL) $(INSTDATFLAGS) qia.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/qia.alg
	$(INSTALL) $(INSTDATFLAGS) radazvd.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/radazvd.alg
	$(INSTALL) $(INSTDATFLAGS) recclalg.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/recclalg.alg
	$(INSTALL) $(INSTDATFLAGS) saa.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/saa.alg
	$(INSTALL) $(INSTDATFLAGS) status_prod.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/status_prod.dea
	$(INSTALL) $(INSTDATFLAGS) storm_cell_component.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/storm_cell_component.alg
	$(INSTALL) $(INSTDATFLAGS) storm_cell_segment.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/storm_cell_segment.alg
	$(INSTALL) $(INSTDATFLAGS) storm_cell_track.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/storm_cell_track.alg
	$(INSTALL) $(INSTDATFLAGS) superob.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/superob.alg
	$(INSTALL) $(INSTDATFLAGS) tda.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/tda.alg
	$(INSTALL) $(INSTDATFLAGS) vad.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/vad.alg
	$(INSTALL) $(INSTDATFLAGS) vil_echo_tops.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/vil_echo_tops.alg
	$(INSTALL) $(INSTDATFLAGS) vdeal.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/vdeal.alg
	$(INSTALL) $(INSTDATFLAGS) sails.dea $(TOOLSCFGDIR)/$(DEAUDATADIR)/sails.dea
	$(INSTALL) $(INSTDATFLAGS) icing_hazard.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/icing_hazard.alg
