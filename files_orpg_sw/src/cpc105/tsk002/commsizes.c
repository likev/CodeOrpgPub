/*   @(#) commsizes.c 99/11/02 Version 1.14   */
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
2.  02-MAY-97  LMM  Corrected calculation of memory reqd for HIF stream head 
3.  23-JUL-97  LMM  Added support for DECUX
4.  28-aug-97  LMM  Check only 1st 6 chars for PTI330
5.  14-OCT-97  LMM  New stream head sizing
6.                  Need prototype for strncmp() to compile on NT 
7.  27-FEB-98  LMM  Adjustments for BIF memory
8.  02-JUL-98  LMM  TCP mbufs and cluster bufs now configurable.  Use new 
                    ramsize field in config struct to determine total memsize
9.  02-DEC-98  LMM  Account for PQ370 memory management descriptors
10. 22-JAN-99  LMM  Display memory size
*/

/* #3 - if DECUX, need to define 32-bit pointers here */
#ifdef DECUX
#pragma pointer_size save
#pragma pointer_size short
#endif
#include "xstopts.h"
#include "ucstypes.h"
#include "xstra.h"
#include "xstrmgr.h"
#include "xconfig.h"
#include "commupdate.h"

typedef struct malloc_hdr
{
    bit32  size;    /* size in bytes including this header */
    MMap   *map;    /* address of map allocated from */
} MHDR;
#ifdef DECUX
#pragma pointer_size restore
#endif

/* MS compiler needs the prototypes for these functions, but we cannot include
   string.h and stdio.h. */
#ifdef WINNT
int   __cdecl printf(const char *, ...);
int   __cdecl strcmp(const char *, const char *);
int   __cdecl strncmp(const char *, const char *, int);  /* #6 */
#endif /* WINNT */

extern CPARMS cp;

void display_memory_usage()
{
   int i, totblks, dblkmem, dbsize, poolsize;
   int freemem, codemem;
   int tcpmem, drvmem, totmem;
   int sysmem = 0;

   /* determine memory used for code/data/reserved areas */
   freemem = cp.freemem;
   codemem = cp.ramsize-freemem;	/* #8 */
 
   /* determine memory used for system structs */
   sysmem += (cp.npri + 1) * sizeof (SCHED_ELEM) + sizeof (MHDR);
   sysmem += cp.ntoken * sizeof (TOKEN) + sizeof (MHDR);
   sysmem += cp.ntimer * sizeof (TI_MSG) + sizeof (MHDR);
   sysmem += cp.nstream * sizeof (BC_MSG) + sizeof (MHDR);
   sysmem += cp.nqueue * sizeof (queue_t) + sizeof (MHDR);
   sysmem += cp.nstacks * cp.stacksize + sizeof (MHDR);

   /* #9 - account for PQ descriptor memory */
   if (!strncmp(cp.btype, "PQ370", 5))
      sysmem += PQ_MM_DESCMEM;

   /* adjust memory remaining after system structs */ 
   freemem = freemem-sysmem;

   /* determine memory used for data blocks and message blocks */
   totblks = 0;
   dblkmem = 0;
   for (i=0; i<cp.nclass; i++)
   {
      dbsize = ( cp.dblk[i].size + 15 ) & 0xfffffff0;
      totblks += cp.dblk[i].num;
      poolsize = cp.dblk[i].num *( sizeof ( dblk_t ) + dbsize ) + sizeof (MHDR);
      dblkmem += poolsize;
   }
   poolsize = totblks * sizeof ( mblk_t ) + sizeof (MHDR);
   dblkmem += poolsize;
   freemem = freemem-dblkmem;

   /* Driver and TCP memory */
   drvmem = 0;

   /* Stream head memory (non-TCP) and TCP */
   drvmem += cp.nstream * sizeof (SMDATA) + sizeof (MHDR);

   /* Memory for multiplexor structs */
   drvmem += cp.nmuxlink * sizeof(struct linkblk);
   tcpmem = 0;
   if (cp.cid == HOST_CONTROLLER)
   {
      /* #8 -  space for mbufs and mclbufs is configurable */ 
      tcpmem = (cp.nmbufs+1)*TCP_MBSIZE + (cp.nmclbufs+1) *TCP_MCLBSIZE 
                                        + TCP_UDPBUFS;
      if ( cp.nboards )
      {
         /* bif driver memory */
         drvmem += cp.nstream * BIF_STREAMMEM;
         drvmem += cp.nboards * BIF_MPSDEVMEM;
         drvmem += ( cp.n330s * BIF_DESC330 ) + BIF_MISCMEM;	/* #7 */
      }
   }
   else
   {
      /* HIF driver stream head */

      /* #5 - All stream manager info has been merged into SMDATA 
              (already has been accounted for above */

      /* if not 330A, add memory for descriptors */
      if (strncmp(cp.btype, "PTI330", 6))		/* #4 */
         drvmem += HIF_DESCMEM;
   }

   /* Adjust free memory for TCP and Driver use */
   freemem = freemem-tcpmem;
   freemem = freemem-drvmem;

   /* Determine total memory used */
   totmem = codemem + sysmem + dblkmem + tcpmem + drvmem;

   /* Output summary */
   printf ("\nMEMORY SIZE:				%7d\n", cp.ramsize);
   printf ("\nCode/data/reserved areas:	%7d\n", codemem);
   printf ("System memory:			%7d\n", sysmem);
   printf ("Data/Message blocks:		%7d\n", dblkmem);
   printf ("Drivers/Stream Head:		%7d\n", drvmem);
   if ( tcpmem )
      printf ("TCP memory:			%7d\n", tcpmem);
   printf ("TOTAL USED				%7d\n", totmem);

   if ( freemem > 0 )
      printf ("\nEstimated memory available		%7d\n", freemem); 
   else
   {
      freemem = -freemem;
      printf ("\n**** WARNING ****. Memory size exceeded by	%7d bytes\n\n", 
          freemem); 
   }
}
  
