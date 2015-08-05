/**************************************************************************

      Module: mnttsk_ls_def.h

  Created by: Arlis Dodson

 Description: Load Shed Maintenance Duties MNTTSK header file

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2000/11/21 22:07:44 $
 * $Id: mnttsk_ls_def.h,v 1.5 2000/11/21 22:07:44 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MNTTSK_LS_DEF_H
#define MNTTSK_LS_DEF_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <orpg.h>

/**@#-*/ /*CcDoc Token Processing OFF*/

#define STARTUP      1
#define RESTART      2

/*
 * Function Prototypes
 */
int MNTTSK_LS_CAT_maint(int startup_action) ;
int MNTTSK_LS_LEVELS_maint(int startup_action) ;


#endif /* #ifndef MNTTSK_LS_DEF_H */
/**@#+*/ /*CcDoc Token Processing ON*/
