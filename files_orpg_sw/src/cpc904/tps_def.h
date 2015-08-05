/*   @(#) tps_def.h 99/12/23 Version 1.5   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1991 by
|
|              +++    +++                           +++     +++
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
|              +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
|              +++    ++++++      +++++ ++++++++++ +++ +++++   
|              +++    ++++++      +++++ ++++++++++ +++  +++    
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++ +++++   
|              +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
|              +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|               ++++++++                             +++    +++
|                ++++++         Corporation         ++++    ++++
|                 ++++   All the right connections  +++      +++
|
|
|      This software is furnished  under  a  license and may be used and
|      copied only  in  accordance  with  the  terms of such license and
|      with the inclusion of the above copyright notice.   This software
|      or any other copies thereof may not be provided or otherwise made
|      available to any other person.   No title to and ownership of the
|      program is hereby transferred.
|
|      The information  in  this  software  is subject to change without
|      notice  and  should  not be considered as a commitment by UconX
|      Corporation.
|  
|      UconX Corporation
|      San Diego, California
|                                               
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAY-97  LMM  Use common include file for PTI500_SERIES boards
2.  14-MAY-97  LMM  Include 151g defines
3.  15-JUL-97  LMM  Use generic define (PTI330) for 330A/330B
*/

#ifndef	_tps_scc_h
#define	_tps_scc_h

#if defined(PTI500_SERIES) || defined(PTI131F) || defined(PTI131K) || defined(PTI151F) || defined(PTI151K)
#include "targets/pti500_scc.h"
#endif	PTI500_SERIES

#ifdef  PTI151G
#include "targets/pti151g_scc.h"
#endif	PTI151G

#if defined (PTI330) || defined (PTI330A)  /* PTI 330 board devices */
#include "targets/pti330_scc.h"
#endif	PTI330A

#ifdef	PTI332
#include "targets/pti332_scc.h"
#endif	PTI332

#ifdef	PTI340
#include "targets/pti340_iusc.h"
#endif	PTI340

#endif	_tps_scc_h
