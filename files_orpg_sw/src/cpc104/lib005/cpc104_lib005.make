# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/03/19 20:32:50 $
# $Id: cpc104_lib005.make,v 1.14 2009/03/19 20:32:50 ccalvert Exp $
# $Revision: 1.14 $
# $State: Exp $

# 

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This makefile is an exception to the general mechanism.
# The reason being it only needs to install these shell scripts in
# INSTALLTOP directory.
install::
	@if [ -d $(INSTALLTOP) ]; then set +x; \
	else (set -x; $(MKDIR) $(INSTALLTOP)); fi
	$(INSTALL) $(INSTDATFLAGS) orpg.bashrc $(INSTALLTOP)/.bashrc
	$(INSTALL) $(INSTDATFLAGS) orpg.cshrc $(INSTALLTOP)/.cshrc
	$(INSTALL) $(INSTDATFLAGS) orpg.bash_profile $(INSTALLTOP)/.bash_profile

