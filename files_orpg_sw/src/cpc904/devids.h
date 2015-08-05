/*   @(#) devids.h 99/12/23 Version 1.12   */

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
 * devids.h	- UconX xSTRa device definitions
 *
 *****************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAY-97  LMM  Use common include file for PTI500_SERIES boards
2.  15-JUL-97  LMM  Use generic define (PTI330) for 330A/330B 
3.   1-apr-98  rjp  Added support for pq370.
4.  09-jun-98  lmm  Removed VMS/USE_DIR (always use targets includes)
5.  15-dec-98  lmm  Added include of targets/pq370devs.h for pq370
                    Eliminated refs to PTI370PQ and POWERQUICC (obsolete)
6.  02-feb-99  lmm  Added support for PQ372
7.  11-may-99  lmm  Support for PQ34X, PQ37X, and MPS800
8.  11-nov-99  dp   Added support for PQ380 and PQ382
*/

#ifndef _devids_h
#define _devids_h

#ifdef PTI332				/* PTI 332 board devices */
#include "targets/pti332devs.h"
#endif

#if defined(PTI500_SERIES) || defined(PTI131F) || defined(PTI131K) || defined(PTI151F) || defined(PTI151K)
#include "targets/pti500devs.h"
#endif

#if defined(PTI131G) || defined(PTI151G)   /* PTI 131/151-G board devices */
#include "targets/pti151gdevs.h"
#endif

#if defined (PTI330) || defined (PTI330A)  /* PTI 330 board devices */
#include "targets/pti330devs.h"
#endif

#ifdef PTI340				/* PTI 340 board devices */
#include "targets/pti340devs.h"
#endif

#ifdef UCONXMPS				/* UconX MPS300/600 board devices */
#include "targets/ucmpsdevs.h"
#endif

#ifdef PTI334				/* PTI PT-SBS334 board devices */
#include "targets/pti334devs.h"
#endif

#ifdef PCI334				/* PTI PT-PCI334 board devices */
#include "targets/pci334devs.h"
#endif

/* #7 - Make sure PQ34X is defined (if PQ344 or PQ348 is defined) */
#if defined(PQ344) || defined(PQ348)
#ifndef PQ34X
#define PQ34X
#endif
#endif

/* #7 - Make sure PQ37X is defined (if PQ370 or PQ372 is defined) */
#if defined(PQ370) || defined(PQ372)
#ifndef PQ37X
#define PQ37X
#endif
#endif

/* #8 - Make sure PQ38X is defined (if PQ380 or PQ382 is defined) */
#if defined(PQ380) || defined(PQ382)
#ifndef PQ38X
#define PQ38X
#endif
#endif


/* #5,#6,#8 - PT-PCIPQ37x board devices */
#if defined(PQ34X) || defined(PQ37X) || defined(MPS800) || defined(PQ38X)
#include "targets/pqdevs.h"
#endif

#if !defined(PTI500_SERIES)
#if !defined(PTI131F)   && !defined(PTI131K)  && !defined(PTI131G)
#if !defined(PTI151F)   && !defined(PTI151K)  && !defined(PTI151G)
#if !defined(PTI330)    && !defined(PTI330A)  && !defined(PTI340)
#if !defined(PTI332)    && !defined(PTI334)   && !defined(PCI334)
#if !defined(PQ34X)     && !defined(PQ344)    && !defined(PQ348)
#if !defined(PQ37X)     && !defined(PQ370)    && !defined(PQ372)
#if !defined(PQ38X)     && !defined(PQ380)    && !defined(PQ382) /* #8 */
#if !defined(UCONXMPS)  && !defined(MPS800)   && !defined(DEVIDS_OK)
ERR: no devids provided: board type undefined
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#endif /* _devids_h */
