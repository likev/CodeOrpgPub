
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS =  tsk001 \
	  tsk002 \
	  tsk003 \
	  tsk004 \
	  tsk005 \
	  tsk006 \

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

