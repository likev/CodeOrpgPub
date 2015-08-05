# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2012/07/31 15:50:43 $
# $Id: README.make,v 1.2 2012/07/31 15:50:43 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $
# copied from ORPG Web page 
# http://osserver1.nssl.noaa.gov/cm/swdoc_db/guidelines/MAKE/makefile.htm

ORPG make Description File Guidelines

  ------------------------------------------------------------------------

Table of Contents

   * 1. Introduction
   * 2. Global make Description Files
   * 3. Template make Description Files
   * 4. Example make Description Files

  ------------------------------------------------------------------------

1. Introduction

You must use the GNU implementation of the make utility when using the ORPG
make description files to build software. At this time, we maintain the GNU
make binaries in /cm/tools/bin/$ARCH (the ARCH environment variable is
described below). Be sure that your path is configured so that the GNU make
binary is preferred to any other flavors of make.

1.1 Environment Variables

The following environment variables must be defined:

  1. ARCH - set according to the OS/platform combination on which the
     software is to be built, for example:
        o lnux_x86 - Linux/x86
  2. MAKETOP - maps to the top of a "baseline" ORPG filesystem (e.g.,
     /xpit); this may be set as desired (e.g., $HOME/orpg)
  3. LOCALTOP - maps to the top of a "local" ORPG filesystem (e.g.,
     $HOME/orpg)
  4. MAKEINC - maps to the location of the global make description files,
     for example:
        o /xpit/conf
        o $HOME/orpg/conf
        o $MAKETOP/conf

The ARCH environment variable may be set in a common login-shell
initialization file, available to each member of the development team.

Note that files are installed with respect to the filesystem identified by
the MAKETOP environment variable.

1.2 Tailoring the Local Build Environment

Although the environment variables described above provide some measure of
flexibility when building software locally, the developer should realize
that make itself supports even greater flexibility via the "-e" option.

The "-e" option instructs make to give variables taken from the environment
precedence over variables from makefiles. To be fair, the GNU make
documentation notes that this approach is "not recommended practice"; but
the context of that note is the general use of make as opposed to our use of
make in conjunction with the ORPG family of make description files.

The developer may wish to create an alias from, say, omake (i.e., "ORPG
make"), to "make -e". This alias would then be used by the developer when
building the ORPG software locally.

We should eventually develop an exhaustive list of ORPG make variables that
also describes their use within ORPG software builds. In the meantime, the
developer may find these variables in the global (or "configuration") make
description files.

The following illustrates overriding the ORPG make variables EXTRA_LDOPTIONS
(used when building C binaries) and FC_EXTRA_LDOPTIONS (for Fortran
binaries) to link against LOCALTOP libraries in preference to MAKETOP
libraries. This would be useful when the developer wants to maintain a
minimum of local libraries (e.g., the majority of libraries could be found
in /xpit/lib/$ARCH).

setenv EXTRA_LDOPTIONS "-s -L$LOCALTOP/lib/$ARCH -L$MAKETOP/lib/$ARCH"
setenv FC_EXTRA_LDOPTIONS "-s -L$LOCALTOP/lib/$ARCH -L$MAKETOP/lib/$ARCH"

An obvious disadvantage of this approach is that any changes made to the
global make description files will not be propagated into the developer's
environment. The burden is on the developer to ensure the validity of the
make variables defined in his environment.

An alternative is to set both MAKETOP and LOCALTOP to local directories.

Finally, the developer has the freedom of tailoring the ORPG configuration
make description files and/or the local make description files.

1.3 GNU Makefile Conditionals

Although the use of architecture conditionals in local make description
files is deprecated, such conditionals may be used as necessary (the vast
majority of architecture-specific descriptions should appear only in the
global make description files).

The following illustrates the use of conditionals (an obvious weakness is
that the local make description file may need to be updated whenever a new
architecture is added).

ifeq ($(ARCH), lnux_x86)
(Linux x86-specific description)
endif

1.4 X Descriptions

The following X descriptions are defined in the various
architecture-specific global make description files:

   * X_DEFINES (append to ALL_DEFINES)
   * X_INCLUDES (append to ALL_INCLUDES)
   * X_LIBPATH (use to specify LIBPATH)

These descriptions may be added to the local make description file. If these
descriptions are not appropriate for a given binary, then the local make
description file must be tailored to support the binary (by, for example,
using architecture-specific conditionals as described above).

1.5 System Descriptions

The following "system" descriptions are defined in the various
architecture-specific global make description files:

   * SYS_DEFINES (append to ALL_DEFINES)
   * SYS_INCLUDES (append to ALL_INCLUDES)
   * SYS_LIBPATH (may use to specify LIBPATH)
   * SYS_LIBS (may use to specify libraries)

These descriptions may be added to the local make description file. If these
descriptions are not appropriate for a given binary, then the local make
description file must be tailored to support the binary (by, for example,
using architecture-specific conditionals as described above).

These descriptions are not required for typical ORPG software items (i.e.,
they appear primarily in infrastructure software make description files).

2. Global make Description Files

The global make description files are located in the directory identified by
the MAKEINC environment variable. In xpit, these files are found in the
/xpit/conf subdirectory.

2.1 make.common

This global make description file should be included in all ORPG make
description files:

include $(MAKEINC)/make.common

Refer to the xpit version of make.common.

2.2 make.$(ARCH)

The make.$(ARCH) global This make description file is typically included at
the top of an ORPG make description file (immediately following inclusion of
the make.common file):

include $(MAKEINC)/make.$(ARCH)

Refer to the various versions of this file:

   * make.lnux_x86

2.3 Single-target make Description Files

Developers are urged to provide separate make description files for every
binary file and/or library.

Using these files should result in shorter make description files. Using
these files should also support the addition of future targets (e.g.,
Insure++ binaries).

2.3.1 make.cbin

Reference make.cbin in a make description file that describes how a single C
binary file is to be generated. Other than the typical flags, the critical
make variables include:

* SRCS - set to the list of C source files
* ADDITIONAL_OBJS - set to "shared" objects (retrieved from outside the current source-file directory)
* TARGET - set to the name of the binary file

2.3.2 make.fbin

Reference make.fbin in a make description file that describes how a single
Fortran or C/Fortran (C objects linked into Fortran binary) binary file is
to be generated. Other than the typical flags, the critical make variables
include:

* CSRCS - set to the list of C source files
* FSRCS - set to the list of Fortran source files (.ftn)
* ADDITIONAL_OBJS - set to "shared" objects (retrieved from outside the current source-file directory)
* TARGET - set to the name of the binary file

The ADDITIONAL_OBJS variable supports, for example, the "sharing" of
cpc-level object files.

2.3.3 make.cflib

Reference make.cflib in a make description file that describes how a single
C, Fortran or C/Fortran library is to be generated. Other than the typical
flags, the critical make variables include:

* LIB_CSRCS - set to the list of C source files
* LIB_FSRCS - set to the list of Fortran source files (.ftn)
* LIB_TARGET - set to the name of the binary file

2.3.4 make.parent_*

For our purposes, a "parent" make description file resides at the task or
library level and is used to reference one or more children make description
files.

2.3.4.1 make.parent_bin

Reference make.parent_bin in a make description file that uses one or more
"child" make description files to generate binary files.

The BINMAKEFILES make variable must be set prior to inclusion of this file,
for example:

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = lb_create.mak \
        lb_info.mak \
        lb_rm.mak \
        lb_rep.mak

include $(MAKEINC)/make.parent_bin

2.3.4.2 make.parent_lib

Reference make.parent_lib in a make description file that uses one or more
"child" make description files to generate libraries.

The LIBMAKEFILES make variable must be set prior to inclusion of this file,
for example:

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = librss.mak \
        librsscl.mak \
        librsssv.mak

include $(MAKEINC)/make.parent_lib

2.3.5 make.subdirs

Reference make.subdirs in a high-level (e.g., src- or cpc-level) make
description file that references one or more subdirectories.

The CURRENT_DIR and SUBDIRS make variables must be set prior to inclusion of
this file, for example:

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

CURRENT_DIR = .
SUBDIRS =       lib002 \
                lib003 \
                lib004 \
                lib005 \
                lib006 \
                lib010 \
                lib012 \
                tsk001
include $(MAKEINC)/make.subdirs

2.3.6 make.cfg

Reference make.cfg in a make description file that describes how a
configuration file target is determined. The critical variable for this file
include

* CFGDIR - set to the target installation directory.
* INSTDATFLAGS - set to the flags used with INSTALL.

2.3.7 make.lb

Reference make.lb in a make description file that describes how a linear
buffer file target is determined. The critical variable for this file
include

* DATADIR - set to the target installation directory.
* INSTDATFLAGS - set to the flags used with INSTALL.

2.3.7 make.script

Reference make.script in a make description file that describes how a script
target is determined. The critical variable for this file include

* SCRIPTDIR - set to the target installation directory.
* INSTBINFLAGS - set to the flags used with INSTALL.

3. Template make Description Files

The templates makefile.general and makefile.fortran are provided for
reference.

3.1 C Binary File

template.cbin provides a template for writing a make description file that
describes generating a single C binary file.

3.2 Fortran or C/Fortran Binary File

template.fbin provides a template for writing a make description file that
describes generating a single Fortran binary binary file.

3.3 C/Fortran Binary File

template.cfbin provides a template for writing a make description file that
describes generating a single C/Fortran binary file. (A C/Fortran binary
file is a Fortran binary file into which one or more C object files are
linked).

3.4 C Library

template.clib provides a template for writing a make description file that
describes generating a single C library.

3.5 C/Fortran Library

template.cflib provides a template for writing a make description file that
describes generating a single C/Fortran library.

3.6 Fortran Library

template.flib provides a template for writing a make description file that
describes generating a single Fortran library.

3.7 Binary/Library Parent Makefile

template.binlibparent provides a template for writing a parent make
description file that references one or more binary and/or library children
makefiles.

3.8 CPC-Level Parent Makefile

template.cpc provides a template for writing a CPC-level make description
file that references one or more subdirectory makefiles.

4. Example make Description Files

The following makefiles exemplify the various kinds of makefiles we have in
the ORPG environment.


      makefile                    description

      base.make                   base-level makefile
      src.make                    src-level makefile
      cpc100.make                 cpc-level makefile
      cpc014.make                 cpc-level makefile w/shared objects
      cpc014_shared.mak           cpc-level makefile to generate shared objects
      cpc100_lib002.makeparent    makefile (library & binary)
      en_lib.mak                  C library makefile
      en_daemon.mak               C binary makefile
      cpc101_lib001.make          C/Fortran library makefile
      cpc101_lib002.make          Fortran library makefile
      cpc013_tsk001.make          Fortran binary makefile
      cpc014_tsk006.make          Fortran binary makefile w/additional objects
      cpc013_tsk003.make          C/Fortran binary makefile
      cpc014_tsk007.make          C/Fortran binary makefile w/additional objects

