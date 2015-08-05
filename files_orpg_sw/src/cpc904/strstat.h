/*   @(#) strstat.h 99/12/23 Version 1.3   */
/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
 
                            Copyright (c) 1997 by
 
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
   notice and should not  be  considered  as  a  commitment by UconX
   Corporation.
                          UconX Corporation
                        San Diego, California
 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAR-97  lmm  Added support for memory stats
2.  15-DEC-97  mpb omment out text proceeding 'endif'.
*/

#ifndef	_xstra_strstat_
#define	_xstra_strstat_

/*
 * Streams Statistics header file.  This file
 * defines the counters which are maintained for statistics gathering
 * under Streams.
 */

typedef struct 
{
	int use;			/* current item usage count */
	int total;			/* total item usage count */
	int max;			/* maximum item usage count */
	int fail;			/* count of allocation failures */
} alcdat;

struct  strstat 
{
	alcdat stream;			/* stream allocation data */
	alcdat queue;			/* queue allocation data */
	alcdat mblock;			/* message block allocation data */
	alcdat dblock;			/* aggregate data block allocation */
	alcdat dblk[MAXCLASS];		/* data block class allocation */
	alcdat stack;			/* stack allocation data */
	alcdat memory;			/* main memory allocation data - #1 */
};

/* in the following macro, x is assumed to be of type alcdat */
#define BUMPUP(X)	{(X).use++;  (X).total++;\
	(X).max=((X).use>(X).max?(X).use:(X).max); }
#define BUMPDOWN(X) ((X).use--)


/* per-module statistics structure */
struct module_stat {
	long ms_pcnt;			/* count of calls to put proc */
	long ms_scnt;			/* count of calls to service proc */
	long ms_ocnt;			/* count of calls to open proc */
	long ms_ccnt;			/* count of calls to close proc */
	long ms_acnt;			/* count of calls to admin proc */
	char *ms_xptr;			/* pointer to private statistics */
	short ms_xsize;			/* length of private statistics buf */
};



#define	STAT_STREAM	(((struct strstat *)STAT_ADDR)->stream)
#define	STAT_QUEUE	(((struct strstat *)STAT_ADDR)->queue)
#define	STAT_MBLOCK	(((struct strstat *)STAT_ADDR)->mblock)
#define	STAT_DBLOCK	(((struct strstat *)STAT_ADDR)->dblock)
#define	STAT_DBLK	(((struct strstat *)STAT_ADDR)->dblk)
#define	STAT_STACK	(((struct strstat *)STAT_ADDR)->stack)
#define	STAT_MEM	(((struct strstat *)STAT_ADDR)->memory)	/* #1 */

#endif	/* _xstra_strstat_  */  /* #2 */
