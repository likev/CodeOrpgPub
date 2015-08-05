# RCS info
# $Author: jeffs $
# $Locker:  $
# $Date: 2014/03/11 18:07:55 $
# $Id: libinfr.mak,v 1.10 2014/03/11 18:07:55 jeffs Exp $
# $Revision: 1.10 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#
# Build the "infrastructure" library is straightforward, if somewhat involved.
# Note that we EXTRACT static library (libinfr.a) objects from the daughter
# static libraries (e.g., libmisc.a).  We COPY dynamic library (libinfr.s?)
# objects from the daughter dynamic library object-file subdirectories
# (e.g., ../lib004/$(ARCH)).
#
# Complete the following three steps when adding a new daughter library to
# the infrastructure library:
#
# 1. Other_libs variable - add the daughter static library PATHNAME to this
#    list.  For example:
#
#		../lib003/$(ARCH)/liblb.a \
#
# 2. libinfr.a target - add the daught static library PATHNAME to this
#    target (or targets: see NOTE below).  For example:
#
#	ar x ../../lib004/$(ARCH)/libmisc.a ;\
#
#    NOTE: we go back two directory levels because we'll be working in the
#    architecture subdirectory at this time
#
# 3. libinfr.$(LIB_EXT) target - add the lines necessary to copy the
#    daughter dynamic library shared object files.  For example:
#
#	objects=`ar t ../lib015/$(ARCH)/libcmp.a | sed "s/\.dbg/\.shr/"`; \
#	for i in $$objects; do \
#	   cp ../lib015/$(ARCH)/$$i $(ARCH) ; \
#	done 
#

# This is for the general infrastructure library "libinfr.a"
# The order of the following libraries is important to
# prevent unresolved symbols in the links of various executables.

Other_Libs =	../lib006/$(ARCH)/librss.a \
		../lib005/$(ARCH)/librmt.a \
		../lib002/$(ARCH)/liben.a \
		../lib003/$(ARCH)/libinfrlb.a \
		../lib007/$(ARCH)/libsupp.a \
		../lib010/$(ARCH)/libmalrm.a \
		../lib004/$(ARCH)/libmisc.a  \
		../lib016/$(ARCH)/libnet.a \
		../lib019/$(ARCH)/libsmia.a

LIB_TARGET = infr

SHRDLIBLD_SEARCHLIBS = $(SYS_LIBS) 

liball:: $(ARCH)/lib$(LIB_TARGET).$(LIB_EXT)
liball:: $(ARCH)/lib$(LIB_TARGET).a

# shared library ...
libinstall:: $(ARCH)/lib$(LIB_TARGET).$(LIB_EXT)
	@if [ -d $(LIBDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(LIBDIR)); fi
	$(INSTALL) $(INSTSHLIBFLAGS) $< $(LIBDIR)/$(<F)

libinstall:: $(ARCH)/lib$(LIB_TARGET).a
	@if [ -d $(LIBDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(LIBDIR)); fi
	$(INSTALL) $(INSTLIBFLAGS) $< $(LIBDIR)/$(<F) ;\
	$(RANLIB) $(LIBDIR)/$(<F)

# note that we _assume_ no identically-named object files exist
# among the library directories
# NOTE: debug object files (.dbg) are used to build the static libs

$(ARCH)/libinfr.a: $(Other_Libs)
	@if [ -d $(ARCH) ]; then set +x; \
	else (set -x; $(MKDIR) $(ARCH)); fi
	cd $(ARCH) ;\
	rm -f lib$(LIB_TARGET).a *.$(DBGOBJ_EXT) ;\
	ar x ../../lib006/$(ARCH)/librss.a ;\
	ar x ../../lib005/$(ARCH)/librmt.a ;\
	ar x ../../lib002/$(ARCH)/liben.a ;\
	ar x ../../lib003/$(ARCH)/libinfrlb.a ;\
	ar x ../../lib007/$(ARCH)/libsupp.a ;\
	ar x ../../lib010/$(ARCH)/libmalrm.a ;\
	ar x ../../lib004/$(ARCH)/libmisc.a ;\
	ar x ../../lib016/$(ARCH)/libnet.a ;\
	ar x ../../lib019/$(ARCH)/libsmia.a ;\
	$(AR) libinfr.a *.$(DBGOBJ_EXT) ;\
	$(RANLIB) libinfr.a

# ASSUMPTION: there is a one-to-one correspondence between every object
# file in the static libraries (.a) and the shared libraries (.$(LIB_EXT))
#
# For each of the libraries ...
#    use the static version of the library to identify the object files
#    use "sed" and "cp" commands to retrieve the corresponding shared
#       object files
# Then use "ld" to construct the shared library using the object files
#    sitting in the directory ...
#
# NOTE the double-dollar-signs required to access shell variables
# REFER to Chapter 4 Commands of "Managing Projects with make"
# by Andrew Oram and Steve Talbott (O'Reilly & Associates, Inc.)
#

$(ARCH)/libinfr.$(LIB_EXT): $(Other_Libs)
	@if [ -d $(ARCH) ]; then set +x; \
	else (set -x; $(MKDIR) $(ARCH)); fi
	rm -f $(ARCH)/*.shr $(ARCH)/lib$(LIB_TARGET).$(LIB_EXT)
	objects=`ar t ../lib006/$(ARCH)/librss.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib006/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib005/$(ARCH)/librmt.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib005/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib002/$(ARCH)/liben.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib002/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib003/$(ARCH)/libinfrlb.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib003/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib007/$(ARCH)/libsupp.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib007/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib010/$(ARCH)/libmalrm.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib010/$(ARCH)/$$i $(ARCH) ; \
	done
	objects=`ar t ../lib004/$(ARCH)/libmisc.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib004/$(ARCH)/$$i $(ARCH) ; \
	done 
	objects=`ar t ../lib016/$(ARCH)/libnet.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib016/$(ARCH)/$$i $(ARCH) ; \
	done 
	objects=`ar t ../lib019/$(ARCH)/libsmia.a | sed "s/\.dbg/\.shr/"`; \
	for i in $$objects; do \
	   cp ../lib019/$(ARCH)/$$i $(ARCH) ; \
	done 
	cd $(ARCH) ;\
	objects=`ls *.shr`; gcc -o libinfr.$(LIB_EXT) $(SHRDLIBLDFLAGS) $$objects $(SHRDLIBLD_SEARCHLIBS)

clean::
	\rm -rf $(ARCH)/*.sh? $(ARCH)/*.db? $(ARCH)/*.so $(ARCH)/*.a

