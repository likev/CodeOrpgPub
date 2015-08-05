# RCS info
# $Author: 
# $Locker:  $
# $Date: 2011/05/22 17:38:38 $
# $Id: cpc026.make,v 1.18 2011/05/22 17:38:38 cmn Exp $
# $Revision: 1.18 $
# $State: Exp $

all::
	rm -f ./tsk011/misc_string.c
	cp ../cpc100/lib004/misc_string.c ./tsk011/misc_string.c

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This is the parent makefile for CPC026 Adaptation Data


SUBDIRS =	tsk001 \
		tsk002 \
		tsk005 \
		tsk006 \
		tsk007 \
		tsk009 \
		tsk010 \
		tsk011

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs
