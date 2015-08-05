/*   @(#) xsystem.h 99/12/23 Version 1.9   */
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

/*
Modification history:
 
Chg Date       Init Description
1.  15-JUL-97  LMM  Added clock_steering for use by SCC serial device drivers
                    Added board type to support 330A/B's 
2.  21-oct-97  pmt  Added the SNTP system clock to system structure.
3.   4-apr-98  rjp  Added PQ_SSTRUCT_ADDR and PQ_SSTRUCT_OFFSET.
4.  18-jan-99  lmm  CVDstruct address field changed from "char *" to integer
                    This was done so 64-bit hosts may correctly access the field
5.  19-may-99  lmm  Added clock_rate define for QUICC platforms (field is
                    shared with "clock_steering" defined for SCC platforms)
6.  07-jul-99  lmm  Added modem signals status in global system_struct
7.  17-dec-99  lmm  Obsoleted change #6.
*/

#ifndef _xsystem_h
#define _xsystem_h

/* Needs:	"xstypes.h" */

struct system_struct
{
    int      x_addr[2];		/* Reserved for PROM usage */
    int      sh_mem;		/* Local address of shared memory */
    int      CVDStruct;	        /* Addr of driver's CVDStruct - #4 */

    /* Pointers to global variables */
				/* Local Equiv   Type              Include  */
    char     *XConfig;		/* XCON_ADDR    XCON *            xconfig.h */
    char     *strstat;		/* STAT_ADDR    struct strstat *  strstat.h */

    int      elapsed;		/* 10 ms tick counter incremented by xSTRa  */

    /* Variables for ugdb to communicate with board over SBUS               */
    /* At the moment, only used for 334, and ignored for all other          */
    /* configurations; any insertions in this struct will require           */
    /* that ugdb be rebuilt.  Also ugdb doesn't include this header         */
    /* file -- it uses hard coded addresses.                                */

    int      gdb_to_board;      /* gdb data from sparc to board             */
    int      gdb_to_sparc;      /* gdb data from board to sparc             */

    int	     board_type;	/* =0/1 for 330A/B; or 0/1 for 151/161      */
                                /* =0/1 for PQ370/2, or 0/1 for PQ344/348   */
    int	     clock_rate;	/* clock rate (QUICC/PQUICC cards only)     */ 
#define clock_steering clock_rate /* clock steering global (SCC cards only) */
    int      utc_active;        /* notes if SNTP time has been set yet      */
    struct   utimeval ntp_time; /* Our NTP ticker in Unix epoch format      */
};


/* The slaves_struct address is constant across ALL Controllers so that the
 * Master knows where to look for the interface description.  The 
 * SSTRUCT_OFFSET is constant for all Controllers; the SSTRUCT_ADDR is based
 * on the shared memory base address for the individual controller, which may
 * be determined via the SHMEM_BASE definition in the target specific include
 * file (see target.h).
 */
/*
 * There are host drivers that support both 68k and powerpc embedded
 * platforms, so we need to define both SSTRUCT_OFFSET and PQ_SSTRUCT_OFFSET,
 * as well as SSTRUCT_ADDR and PQ_SSTRUCT_ADDR. The board code should use
 * the getsysaddr service to get the appropriate value for SSTRUCT_ADDR.
 */

#define	SSTRUCT_OFFSET	0x1200	 /* system_struct offset from shared mem base */
#define	PQ_SSTRUCT_OFFSET 0x3100 /* system_struct offset from shared mem base */

#define	SSTRUCT_ADDR	((struct system_struct *)( (int) SSTRUCT_OFFSET + \
						   (int) SHMEM_BASE ))
#define	PQ_SSTRUCT_ADDR	((struct system_struct *)( (int) PQ_SSTRUCT_OFFSET + \
						   (int) SHMEM_BASE ))

#endif /* _xsystem_h */

