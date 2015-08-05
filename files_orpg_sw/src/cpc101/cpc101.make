# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/06/13 21:15:21 $
# $Id: cpc101.make,v 1.10 2013/06/13 21:15:21 steves Exp $
# $Revision: 1.10 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# ORPG libraries

# lib001 librpg.   * Legacy RPG Support Library
# lib002 librpgcm. * Legacy RPG Common Modules (CM) Library
# lib003 liborpg.  * ORPG Library
# lib004 librpgc.  * RPG Support Library (C/C++ interface)
# lib005 libaprcom.* AP-Removed Common Modules
# lib006 liborpgsun* Library for determining Sun location
# lib006 snmp      * snmp library
# lib007 xlibadaptcomblk     * adaptation <-> common block library
# tsk001 update_alg_data
# tsk008 owr (one-way replicator for satellite comms)

SUBDIRS =	lib001 \
		lib002 \
		lib003 \
		lib004 \
		lib005 \
		lib006 \
		lib007 \
		tsk001 \
		tsk008

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

