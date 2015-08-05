# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2014/02/27 19:21:14 $
# $Id: libl2_static.mak,v 1.1 2014/02/27 19:21:14 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIB_TARGET = l2_full_static

SHRDLIBLD_SEARCHLIBS = $(SYS_LIBS) 

liball:: $(ARCH)/lib$(LIB_TARGET).a

libinstall:: $(ARCH)/lib$(LIB_TARGET).a
	@if [ -d $(LIBDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(LIBDIR)); fi
	$(INSTALL) $(INSTLIBFLAGS) $< $(LIBDIR)/$(<F) ;\
	$(RANLIB) $(LIBDIR)/$(<F)

$(ARCH)/lib$(LIB_TARGET).a:
	@if [ -d $(ARCH) ]; then set +x; \
	else (set -x; $(MKDIR) $(ARCH)); fi
	cd $(ARCH) ;\
	rm -f lib$(LIB_TARGET).a *.$(DBGOBJ_EXT) ;\
	ar x libl2.a ;\
	ar x $(LIBDIR)/libinfr.a ;\
	$(AR) lib$(LIB_TARGET).a *.$(DBGOBJ_EXT) ;\
	$(RANLIB) lib$(LIB_TARGET).a ;\
	rm -f *.$(DBGOBJ_EXT)

clean::
	\rm -rf $(ARCH)/lib$(LIB_TARGET).a $(ARCH)/*.dbg

