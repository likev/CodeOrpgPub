# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/12/20 23:09:17 $
# $Id: global_includes_ccdoc.mak,v 1.5 1999/12/20 23:09:17 dodson Exp $
# $Revision: 1.5 $
# $State: Exp $
#
# This is a temporary approach to building CcDoc documents.  Do not use
# this make description file as part of the baseline build (i.e., it does
# not conform to the New RPG Make Description File Guidelines).

SDFBASE=/cm/sdf_db/cpci03/cpc000

CCDOC_FLAGS=-macros -typedefs \
-rooturl $(SDFBASE)/sdf_man.htm \
-imageurl /images/ccdoc/ \
-trailer $(SDFBASE)/rpg.trailer \
-sourceurl /xpit/include/


MODULE_LIST= ORPGCFG ORPGEVT ORPGINFO ORPGLMAT ORPGLOG ORPGTASK ORPGTAT

all:: $(MODULE_LIST)

.PHONY: $(MODULE_LIST)

ORPGCFG: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgcfg.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGEVT: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgevt.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGINFO: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpginfo.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGLMAT: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpglmat.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGLOG: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpglog.h ./orpglog_sysstat.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGTASK: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgtask.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGTAT: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgtat.h
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/
