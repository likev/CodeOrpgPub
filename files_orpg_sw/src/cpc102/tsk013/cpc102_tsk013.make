include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = asp_view.mak  asp_view_static.mak

include $(MAKEINC)/make.parent_bin

