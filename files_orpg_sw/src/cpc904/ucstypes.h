/*   @(#) ucstypes.h 99/12/23 Version 1.5   */
/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1992 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 
 
       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
   
       UconX Corporation
       San Diego, California

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/******************************************************************************
 *
 * ucstypes.h	- Standard type definitions for the UconX Server platform
 *
 * These type definitions are intended to be used when compiling code which
 * will run on the server platform.
 *
 *****************************************************************************/

#ifndef _ucstypes_h
#define _ucstypes_h


#include "xstypes.h"			/* Get special internal definitions */

#if !defined(_U_CHAR)			/* common types between SysxV & BSD */
#define _U_CHAR
typedef	unsigned char	u_char;		/* (Needed for 4.2 driver ports) */
typedef	unsigned short	u_short;	/* (Needed for 4.2 driver ports) */
typedef	unsigned int	u_int;		/* (Needed for 4.2 driver ports) */
typedef	unsigned long	u_long;		/* (Needed for 4.2 driver ports) */
typedef	long		daddr_t;	/* <disk address> type */
typedef	char *		caddr_t;	/* ?<core address> type */
typedef	unsigned short	ushort;
typedef	unsigned char	unchar;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;
typedef	unsigned long	ino_t;		/* <inode> type */
typedef	long		time_t;		/* <time> type */

/* Modified to conform to standard. used to be short */
typedef	int		dev_t;		/* <device number> type */
typedef	long		off_t;		/* ?<offset> type */
typedef	unsigned short	uid_t;
typedef	unsigned short	gid_t;
#endif /* _U_CHAR */

/* SysV basic types not used in BSD */
typedef	short		cnt_t;		/* ?<count> type */
typedef	int		label_t[12];	/* sync with NJBREGS in pcb.h */
typedef	unsigned long	paddr_t;	/* <physical address> type */
typedef	int		key_t;		/* IPC key type */
typedef	unsigned char	use_t;		/* use count for swap.  */
typedef	short		sysid_t;
typedef	short		index_t;
typedef	short		lock_t;		/* lock work for busy wait */
typedef	unsigned int	size_t;		/* len param for string funcs */

#ifndef NULL
#define	NULL	0
#endif

#define	minor(dev)	(dev & 0xff)

#endif /* _ucstypes_h */
