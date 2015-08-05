/*   @(#) xconfig.h 99/12/23 Version 1.13   */

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
 * xconfig.h	- UconX XSTRA configuration structure definition
 *
 *****************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
1.  11-SEP-96  LMM  Increased MAXPROS from 20 to 100.  For a MPS3000 server,
                    MAXPROS must be large enough to accomodate the max no. of
                    protocols over all controllers. 
2.  13-sep-96  pmt  added flag to xcon structure for system report setting
3.  23-oct-96  pmt  added ifdef for VMS client; Alpha linker complains on extern
		    even if not ref'd
4.  13-mar-97  LMM  define report frequency for non-PKPLUS systems
5.  27-aug-97  LMM  updated to note unused fields in PROTDEF
6.   4-apr-98  rjp  Add powerquicc support.
7.  11-jun-98  lmm  Removed PKPLUS ifdef (no longer required)
8.  02-jul-98  lmm  Added ramsize,nmbufs,nmclbufs,use_mclbufs
*/

#ifndef _xconfig_h
#define _xconfig_h


#define	MAXCLASS	4		/* Max data block classes */

#define	MAXMODS		50		/* Max modules per controller */
#define	MAXPROS		100		/* Max protocols per controller */
#define	MAXMODPRO	20		/* Max modules per protocol.
					   Must be an EVEN number for
					   correct alignment. */

/* 
 * Data block definition used to configure up to four different sizes of
 * data blocks and the number of blocks of each size.  Set "size" to 
 * zero for unused entries.
 */
typedef struct 
{
   int	size;		/* Size of block (multiple of 16) */
   int  num;		/* Number of blocks to build */
} DBlk;


/*
 * Module definitions, indexed by module ID
 */

typedef struct
{
   struct streamtab *modtab;
} MODDEF;


/*
 * Protocol definitions, scanned for matching name field
 */

typedef struct
{
   short	propri;
   char		name [ 12 ] ;
   char         spare [ 2 ];			/* #5 */
   int          ctlr;
   short	modlist [ MAXMODPRO +1 ];	/* terminated with -1 */
   short        unused;				/* #5 */
} PROTDEF;

/*
 * BOARD_INFO:
 *     Filled out by commfig.  This structure defines the layout of the
 *     host/controller configuration.  If a board isn't present, 
 *     btype will be set to "unused" and shmemsize will be 0.
 *
 *     The stream head on the host only cares if there are controllers or
 *     not.  Rather than check this struct, it can check for the 
 *     CTLR_PRESENT flag in cf_flags.
 *
 *     The board interface driver (UC_bif, running on the host) does care
 *     and uses this info for initialization.
 */
typedef struct
{
   unsigned int shmemsize;
   char         btype[20];  
} BOARD_INFO;

/*
 * Memory map structure, used by malloc routines
 */

typedef struct mmap {			/* Structure used for memory alloc */
	unsigned int addr;		/* Address of this chunk */
	unsigned int size;		/* Size of this chunk */
} MMap;

/*
 * Configuration structure downloaded to a fixed address (XCON_ADDR) in
 * the controller's memory.
 */
typedef struct XCon 
{
   int	nqueue;		/* Total # of Queue pairs */
   int	nstream;	/* Total # of Streams */
   int	nclass;		/* Number of classes defined */
   DBlk dblk[MAXCLASS];	/* Data block definitions */
   int	nmuxlink;	/* Total # of lower Streams for MUX drivers */
   int	nstrpush;	/* Maximum # of modules per Stream */
   int	strlofrac;	/* Low-priority alloc failure lvl (% already alloc'd) */
   int	strmedfrac;	/* Med-priority alloc failure lvl (% already alloc'd) */
   int	slice_ticks;	/* Number of system ticks per time slice */
   int	stack_size;	/* Size of each module's QUEUE stack */
   int	nstacks;	/* Total number of active stacks */
   int	npri;		/* Number of queue scheduling priorities */
   int	ntimer;		/* Total number of active timers */
   int  ntoken;		/* Number of token entries in system resource table */
   int  ramsize;	/* Ram size */					/* #8 */
   int  nmbufs;		/* TCP message bufs (Server only) */		/* #8 */
   int  nmclbufs;	/* TCP cluster bufs (Server only) */ 		/* #8 */
   int  use_mclbufs;	/* Use cluster bufs for TCP rcv (0=no,1=yes */  /* #8 */
   MODDEF moddefs [ MAXMODS ];		/* Module definitions */
   PROTDEF protdefs [ MAXPROS ];	/* Protocol definitions */
   MMap *mmap;		/* Map of main memory (excluding shared region) */
   MMap *smap;		/* Map of shared memory (==mmap if no shared mem) */

			/* set up by commfig util */
   int  cf_flags;	/* Configuration flags: set by commfig util */
   int  l_cid;		/* Controller ID for this board, set by commfig util */
   char l_btype[20];	/* Type names for this board, set by commfig util */
   BOARD_INFO boards[20];
   unsigned int rep_freq;/* System Report flag; 0=off, >0 sec */
} XCON;

#ifndef VMS		/* pmt #3 */
extern XCON xconfig;
#endif

#if defined(UCONXPPC)   /* #6 */
#define	XCON_ADDR  ((XCON *)(PQ_SSTRUCT_ADDR->XConfig))
#define	STAT_ADDR  ((struct strstat *)(PQ_SSTRUCT_ADDR->strstat))
#else
#define	XCON_ADDR  ((XCON *)(SSTRUCT_ADDR->XConfig))
#define	STAT_ADDR  ((struct strstat *)(SSTRUCT_ADDR->strstat))
#endif

#define	NQUEUE		(((XCON *)XCON_ADDR)->nqueue)
#define	NSTREAM		(((XCON *)XCON_ADDR)->nstream)
#define NCLASS		(((XCON *)XCON_ADDR)->nclass)
#define	DBLK		(((XCON *)XCON_ADDR)->dblk)
#define	NMUXLINK	(((XCON *)XCON_ADDR)->nmuxlink)
#define	NSTRPUSH	(((XCON *)XCON_ADDR)->nstrpush)
#define	STRLOFRAC	(((XCON *)XCON_ADDR)->strlofrac)
#define	STRMEDFRAC	(((XCON *)XCON_ADDR)->strmedfrac)
#define	SLICE_TICKS	(((XCON *)XCON_ADDR)->slice_ticks)
#define	STACK_SIZE	(((XCON *)XCON_ADDR)->stack_size)
#define	NSTACKS		(((XCON *)XCON_ADDR)->nstacks)
#define	NPRI		(((XCON *)XCON_ADDR)->npri)
#define	NTIMER		(((XCON *)XCON_ADDR)->ntimer)
#define NTOKEN		(((XCON *)XCON_ADDR)->ntoken)
#define	MODULES		(((XCON *)XCON_ADDR)->moddefs)
#define	PROTOCOLS	(((XCON *)XCON_ADDR)->protdefs)


 /*
  * values for the cf_flags field
  */

#define	ENABLE_GDB_CONS	0x1	/* Enables GDB debugger (instead of PTBug or
				 * other native ROM debugger) for I/O through
				 * the console port.
				 */
#define	ENABLE_GDB_BUS	0x2	/* Enables GDB debugger for I/O through the
				 * backplane bus.
				 */
#define	ENABLE_GDB	(ENABLE_GDB_CONS|ENABLE_GDB_BUS)

#define	ENABLE_RESET	0x4	/* Enables system reset on failure */

#define	ENABLE_NATIVE	0x8	/* Enables native ROM debugger (co-exists with
				 * GDB as best as possible if ENABLE_GDB is
				 * also specified).
				 */

#define CTLR_PRESENT    0x100   /* Host board has at least one ctlr.
				 * Quick check for TCP stream head.
				 */

#endif /* _xconfig_h */
