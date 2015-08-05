/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:31 $
 * $Id: orpgcfg.h,v 1.29 2002/12/11 22:10:31 nolitam Exp $
 * $Revision: 1.29 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgcfg.h

 Description: ORPG Configuration (ORPGCFG_) public header file.

 Assumptions:

 **************************************************************************/


/**@#-*/ /*CcDoc Token Processing OFF*/
#ifndef ORPGCFG_H
#define ORPGCFG_H
/**@#+*/ /*CcDoc Token Processing ON*/

                               /** CS_INT, CS_SHORT                       */
#include <cs.h>
                               /** LB_FILE, LB_REPLACE                    */
#include <lb.h>
                               /** ORPG_NODENAME_SIZ, ORPG_PATHNAME_SIZ   */
#include <orpg_def.h>


/*
 * ASCII (CS) ORPG System Configuration File
 */
#define ORPGCFG_CS_SYSCFG_DATAID_TOK     (0 | (CS_INT))
#define ORPGCFG_CS_SYSCFG_PATH_TOK       1
#define ORPGCFG_CS_SYSCFG_PATH_SIZ	(ORPG_PATHNAME_SIZ)
#define ORPGCFG_CS_SYSCFG_COMMENT        '#'

/**@#-*/ /*CcDoc Token Processing OFF*/
/*
 * Function Prototypes
 */

#ifdef __cplusplus
extern "C"
{
#endif

const char *ORPGCFG_dataid_to_path(int data_id, const char *syscfg_fname);

#ifdef __cplusplus
}
#endif
#endif /* #ifndef ORPGCFG_H */
/**@#+*/ /*CcDoc Token Processing ON*/
