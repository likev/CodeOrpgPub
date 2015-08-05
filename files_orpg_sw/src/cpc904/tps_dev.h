/*   @(#) tps_dev.h 99/12/23 Version 1.4   */
/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1991 by
 
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

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

   SCC Devices for the TPS template protocols

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/*
Modification history:
 
Chg Date       Init Description
1.  07-MAY-97  LMM  Use common include file for PTI500_SERIES boards
2.  15-JUL-97  LMM  Use generic define (PTI330) for 330A/330B
*/

#ifndef _tps_dev_h_
#define _tps_dev_h_

#include "devids.h"

#if defined(PTI500_SERIES) || defined(PTI131F) || defined(PTI131K) || defined(PTI151F) || defined(PTI151K)
#include "targets/pti500_dev.h"
#endif  PTI500_SERIES

#ifdef	PTI151G
#include "targets/pti151g_dev.h"
#endif	PTI151G

#if defined (PTI330) || defined (PTI330A)  /* PTI 330 board devices */
#include "targets/pti330_dev.h"
#endif	PTI330A

#ifdef	PTI332
#include "targets/pti332_dev.h"
#endif	PTI332

#ifdef	PTI340
#include "targets/pti340_dev.h"
#endif	PTI340

#ifdef  PTI334
#include "targets/pti334_quicc.h"
#endif  PTI334

#ifdef  UCONXMPS
#include "targets/ucmps_quicc.h"
#endif  UCONXMPS

#endif /* _tps_dev_h */
