# RCS info 
# $Author: steves $ 
# $Locker:  $ 
# $Date: 2006/01/04 15:25:08 $ 
# $Id: cpc014_tsk011.make,v 1.3 2006/01/04 15:25:08 steves Exp $ 
# $Revision: 1.3 $ 
# $State: Exp $ 
#####################################
##
##	task011 makefile 
##

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = user_sel_LRM.mak

include $(MAKEINC)/make.parent_bin

