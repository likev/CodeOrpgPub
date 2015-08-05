/**************************************************************************

      Module: mnttsk_mngrpg_def.h

  Created by: Arlis Dodson

 Description: Manage RPG MNTTSK header file

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2000/11/28 23:07:25 $
 * $Id: mnttsk_mngrpg_def.h,v 1.9 2000/11/28 23:07:25 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef MNTTSK_MNGRPG_DEF_H
#define MNTTSK_MNGRPG_DEF_H
/**@#+*/ /*CcDoc Token Processing ON*/

#include <orpg.h>

/** System Log Message Macros */
enum {MNTTSK_MNGRPG_SYSLOG_INIT = 1} ;

#define MNTTSK_MNGRPG_SYSLOG_INIT_TXT "SYSLOG file initialized ..."

/**@#-*/ /*CcDoc Token Processing OFF*/

#define STARTUP          1
#define RESTART          2
#define CLEAR_SYSLOG     3
#define CLEAR_STATEFILE  4
#define CLEAR_ALL        5

/*
 * Function Prototypes
 */
int MNTTSK_MNGRPG_CRITDS_maint(int maint_type) ;
enum {MNTTSK_MNGRPG_RPGINFO_INIT_ALL_MSGS=0,
      MNTTSK_MNGRPG_RPGINFO_INIT_STATEFL_MSG,
      MNTTSK_MNGRPG_RPGINFO_INIT_NODELIST_MSG,
      MNTTSK_MNGRPG_RPGINFO_INIT_ENDIANVALUE_MSG,
      MNTTSK_MNGRPG_RPGINFO_INIT_RPGCMD_MSGS} ;
int MNTTSK_MNGRPG_RPGINFO_init(int action) ;
int MNTTSK_MNGRPG_init_lmat();

#endif /* #ifndef MNTTSK_MNGRPG_DEF_H */
/**@#+*/ /*CcDoc Token Processing ON*/
