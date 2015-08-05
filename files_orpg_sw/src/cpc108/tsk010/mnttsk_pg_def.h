/**************************************************************************

      Module: mnttsk_pg_def.h

  Created by: Arlis Dodson

 Description: Product Generation MNTTSK header file

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2000/08/28 16:16:57 $
 * $Id: mnttsk_pg_def.h,v 1.5 2000/08/28 16:16:57 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MNTTSK_PG_DEF_H
#define MNTTSK_PG_DEF_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <orpg.h>

/**@#-*/ /*CcDoc Token Processing OFF*/

#define STARTUP      1
#define RESTART      2
#define CLEAR        3

/*
 * Function Prototypes
 */
int MNTTSK_PG_RT_REQUEST_maint(int startup_action) ;


#endif /* #ifndef MNTTSK_PG_DEF_H */
/**@#+*/ /*CcDoc Token Processing ON*/
