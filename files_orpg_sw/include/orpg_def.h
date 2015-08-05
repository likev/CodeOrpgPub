/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:28 $
 * $Id: orpg_def.h,v 1.2 2002/12/11 22:10:28 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpg_def.h

 Description: Global Open Systems Radar Product Generator (ORPG) header
              file.
       Notes: All ORPG-unique constants defined in this file begin with the
              prefix ORPG_.
 **************************************************************************/


#ifndef ORPG_DEF_H
#define ORPG_DEF_H

/*
 * Global ORPG Constant Definitions/Macro Definitions/Type Definitions
 */
#ifdef SUNOS
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE
#endif
#endif

#ifdef HPUX
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#endif

#if (defined(LINUX) || defined(SUNOS))
#define ORPG_HAS_PROC_FS
#else
#undef ORPG_HAS_PROC_FS
#endif


/**
  * New RPG Endian Value
  */
enum {ORPG_BIG_ENDIAN=1,
      ORPG_LITTLE_ENDIAN=2} ;

/*
 * This is the length of "simple" node names (i.e., *not* Fully Qualified
 * Domain Names (FQDNs)) ...
 *
 *     e.g., "rpglan12", *not* "rpglan12.nssl.noaa.gov"
 *
 */
#define ORPG_NODENAME_LEN	16
#define ORPG_NODENAME_SIZ	((ORPG_NODENAME_LEN) + 1)

#define ORPG_PROGNAME_LEN	128
#define ORPG_PROGNAME_SIZ	((ORPG_PROGNAME_LEN) + 1)
                               /* e.g., for usage messages                */ 
#define ORPG_TASKNAME_LEN	32
#define ORPG_TASKNAME_SIZ	((ORPG_TASKNAME_LEN) + 1)
                               /* Task Attribute Table task name          */ 

#define ORPG_PATHNAME_LEN	1024
#define ORPG_PATHNAME_SIZ	((ORPG_PATHNAME_LEN) + 1)
                               /* define this in terms of MAXPATHLEN???   */

#define ORPG_HOSTNAME_LEN	256
#define ORPG_HOSTNAME_SIZ	((ORPG_HOSTNAME_LEN) + 1)
                               /* Fully Qualified Domain Names            */ 
                               /* define this in terms of MAXHOSTNAMELEN??*/
/*
 * Environment variables ...
 */
#define ORPG_CFGDIR_ENVVAR "CFG_DIR"
#define ORPG_CFGSRC_ENVVAR "CFG_SRC"
#define ORPG_DATADIR_ENVVAR "ORPGDIR"
#define ORPG_DELIVERABLE_ENVVAR "ORPG_DELIVERABLE"
#define ORPG_LE_ENVVAR "LE_DIR_EVENT"


/* date/time conversion macros */
#define RPG_JULIAN_DATE(unix_seconds) (((unix_seconds) / 86400) + 1)
#define RPG_TIME_IN_SECONDS(unix_seconds) ((unix_seconds) % 86400)
#define UNIX_SECONDS_FROM_RPG_DATE_TIME(date,ms) \
				(((date) - 1) * 86400 + (ms) / 1000)

#endif /*DO NOT REMOVE!*/
