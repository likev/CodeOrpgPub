
/***********************************************************************

    Description: comm_manager common function library public header file.

***********************************************************************/

/* 
 * RCS info
 * $Author: john $
 * $Locker:  $
 * $Date: 2000/04/25 13:58:02 $
 * $Id: cmc_def.h,v 1.8 2000/04/25 13:58:02 john Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#ifndef CMC_DEF_H

#define CMC_DEF_H

#ifdef SIMPACT
    #include <cms_def.h>
#endif
#ifdef UCONX
    #include <cmu_def.h>
#endif
#ifdef CM_CISCO
    #include <cmcs_def.h>
#endif
#ifdef CM_TCP
    #include <cmt_def.h>
#endif


#endif		/* #ifndef CMC_DEF_H */

