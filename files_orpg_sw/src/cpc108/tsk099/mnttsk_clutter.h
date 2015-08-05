/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/07/12 18:53:03 $
 * $Id: mnttsk_clutter.h,v 1.1 2007/07/12 18:53:03 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MNTTSK_CLUTTER
#define MNTTSK_CLUTTER
/**@#+*/ /*CcDoc Token Processing ON*/

#include <orpg.h>

/**@#-*/ /*CcDoc Token Processing OFF*/

#define STARTUP      1
#define RESTART      2
#define CLEAR        3

/*
 * Function Prototypes
 */
int MNTTSK_init_clutter(int startup_action) ;


#endif /* #ifndef MNTTSK_CLUTTER */
/**@#+*/ /*CcDoc Token Processing ON*/
