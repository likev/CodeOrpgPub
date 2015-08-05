# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2000/09/14 19:18:43 $
# $Id: liborpg_ccdoc.mak,v 1.10 2000/09/14 19:18:43 jing Exp $
# $Revision: 1.10 $
# $State: Exp $

# This is the make description file for building the New RPG Library
# (liborpg) SDF CcDoc webpages.

CCDOC_FLAGS=-nolocals -notypedefs \
-rooturl /cm/sdf_db/cpci03/cpc101/lib003/sdf_man.htm \
-imageurl /images/ccdoc/ \
-trailer /cm/sdf_db/cpci03/cpc101/lib003/liborpg.trailer \
-sourceurl /xpit/src/cpc101/lib003/

SDFBASE=/cm/sdf_db/cpci03/cpc101/lib003

MODULE_LIST= ORPGCFG ORPGINFO ORPGLMAT ORPGLOG ORPGMISC \
             ORPGPROD ORPGTASK ORPGTAT

all:: $(MODULE_LIST)

.PHONY: $(MODULE_LIST)

ORPGCFG: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgcfg.c
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGINFO: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpginfo.c ./orpginfo_statefl.c ./orpginfo_statefl_shared.c
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGMISC: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgmisc.c
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGPROD: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgprod.c
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGTASK: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgtask.c 
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/

ORPGTAT: $(PHONEY_TARGET)
	ccdoc -ctf $@.ctf -log $@.log -pkg $@ ./orpgtat.c
	ccdoc $(CCDOC_FLAGS) -ctf $@.ctf -html $(SDFBASE)/$@/
