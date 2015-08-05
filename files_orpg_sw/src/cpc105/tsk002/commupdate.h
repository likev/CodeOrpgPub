/*   @(#) commupdate.h 99/11/02 Version 1.8   */
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
1.  17-APR-97  LMM  Initial version
2.  14-OCT-97  LMM  Updated to reflect new SMDATA struc (HIF_SMDEXTMEM obsolete)
3.  27-FEB-98  LMM  Updates for adjustments to descriptor size
4.  02-JUL-98  LMM  Memory for TCP mbufs and mclbufs is now configurable
5.  02-DEC-98  LMM  Account for PQ370 memory management descriptors
6.  22-JAN-99  LMM  Eliminated report frequency from CPARMS struct
*/

#define HOST_CONTROLLER 15 
#define MAX_MAPSIZE     1024	/* needed only to read in memory map */
#define MAXCTLRS        15	

#define TCP_MBSIZE	256	/* size of message bufs */
#define TCP_MCLBSIZE	2048	/* size of cluster bufs */
#define TCP_UDPBUFS	8192	/* udp bufs for TCP */

/* Board Interface Driver memory (only used in MPS3000 with at least
   one controller card in addition to the host card */

#define	BIF_STREAMMEM	64	/* Memory for each stream */
#define	BIF_MPSDEVMEM	160	/* Memory for each controller */ 
#define	BIF_DESC330	132000  /* #3 - descriptors for each 330 card */
#define	BIF_MISCMEM	8192    /* #3 - misc memory for bif */

#define HIF_DESCMEM  	132000	/* Descriptor memory */ 
#define PQ_MM_DESCMEM  	350000	/* #5 - Memory for PQ Memory Mgmt descriptors */ 
typedef struct cp {
   int npri;		/* number of priorities */
   int ntoken;		/* number of tokens */
   int ntimer;		/* number of timers */
   int nstream;		/* number of streams */
   int nqueue;		/* number of queues */
   int nmuxlink;	/* number of mux links */
   int nstacks;		/* number of stacks */
   int stacksize;	/* stack size */
   int freemem;		/* free memory available */
   int ramsize;		/* size of ram */			/* #4 */
   int nmbufs;		/* number of mbufs (server only) */	/* #4 */
   int nmclbufs;	/* number of mclbufs (server only) */ 	/* #4 */
   int cid;		/* controller id */
   int nboards;		/* number of boards (MPS3000 only) */
   int n330s;		/* number of 330's (MPS3000 only) */
   char btype[20]; 	/* board type string */
   int nclass;		/* number of data block classes */
   struct {
      int size;		/* size of dblk */
      int num;		/* number of dblks */
   } dblk [MAXCLASS];
} CPARMS;

void display_memory_usage ( );
