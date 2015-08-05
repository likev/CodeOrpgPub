/*   @(#) target.h 99/12/23 Version 1.13   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1993 by
 
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
 * target.h	- definitions for target Controller board based on the
 *		  target definition
 *
 *****************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAY-97  LMM  Use common include file for PTI500_SERIES boards
2.  15-JUL-97  LMM  Use generic define (PTI330) for 330A/330B
3.  17-MAR-98  rjp  Added support for mps800 (Atlas DARWIN board).
4.   1-APR-98  rjp  Added support for pq370.
5.  09-JUN-98  lmm  Added include of uconx68k for common 68k defines
                    Removed VMS/USE_DIR (always use targets includes)
6.  15-dec-98  lmm  Eliminated include of pq370devs.h (done by devids.h)
                    Removed refs to POWERQUICC (obsolete) 
7.  02-feb-99  lmm  Added support for PQ372
8.  11-may-99  lmm  Added support for PQ344/348 and MPS800
9.  11-nov-99  dp   Added support for PQ380/382
*/

#ifndef _target_h
#define _target_h

#if defined(PTI500_SERIES) || defined(PTI131F) || defined(PTI131K) || defined(PTI151F) || defined(PTI151K)
#include "targets/pti500.h"
#include "targets/uconx68k.h"
#endif

#if defined (PTI131G) || defined(PTI151G)   /* PTI 131/151-G board devices */
#include "targets/pti151g.h"
#include "targets/uconx68k.h"
#endif

#ifdef	PTI332
#include "targets/pti332.h"
#include "targets/uconx68k.h"
#endif

#if defined (PTI330) || defined (PTI330A)  /* PTI 330 board devices */
#include "targets/pti330.h"
#include "targets/uconx68k.h"
#endif

#ifdef	PTI340
#include "targets/pti340.h"
#include "targets/uconx68k.h"
#endif

#ifdef	UCONXMPS
#include "targets/uconxmps.h"
#include "targets/uconx68k.h"
#include "targets/quicc.h"
#include "targets/ucmps_quicc.h"
#endif

#ifdef	PTI334
#include "targets/pti334.h"
#include "targets/uconx68k.h"
#include "targets/quicc.h"
#include "targets/pti334_quicc.h"
#endif

#ifdef	PCI334
#include "targets/pci334.h"
#include "targets/uconx68k.h"
#include "targets/quicc.h"
#include "targets/pci334_quicc.h"
#endif

/*
* #3. Add mps800 support.
*/
#ifdef MPS800	/* #6 */
#include "targets/powerquicc.h"
#include "targets/uconxppc.h"
#include "targets/mps800.h"
#endif
 
/* #8 - Make sure PQ34X is defined (if PQ344 or PQ348 is defined) */
#if defined(PQ344) || defined(PQ348)
#ifndef PQ34X
#define PQ34X
#endif
#endif

/* #8 - Make sure PQ37X is defined (if PQ370 or PQ372 is defined) */
#if defined(PQ370) || defined(PQ372)
#ifndef PQ37X
#define PQ37X
#endif
#endif

/* #9 - Make sure PQ38X is defined (if PQ380 or PQ382 is defined) */
#if defined(PQ380) || defined(PQ382)
#ifndef PQ38X
#define PQ38X
#endif
#endif

/*
* #4. Add pq370 and pq372 support.
* #8. Add cpc344 and cpc348 support.
* #9. Add cpc380 and cpc382 support.
*/
#if defined (PQ34X) || defined (PQ37X) || defined (PQ38X)
#include "targets/powerquicc.h"
#include "targets/uconxppc.h"
#endif

#endif /* _target_h */
