# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2004/02/10 22:16:35 $
# $Id: cpc100_lib002.make,v 1.15 2004/02/10 22:16:35 jing Exp $
# $Revision: 1.15 $
# $State: Exp $

# This is the parent makefile for Event Notification Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = en_lib.mak \
               ent_lib.mak

include $(MAKEINC)/make.parent_lib

