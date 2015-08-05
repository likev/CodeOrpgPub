/**************************************************************************

      Module: iti_def.h

 Description: Data required by one or more of the Initialize Task
              Information (ITI) source files.

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:30 $
 * $Id: iti_def.h,v 1.4 2005/12/27 16:41:30 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef ITI_DEF_H
#define ITI_DEF_H

/*
 * System Include Files/Local Include Files
 */
#include <orpg.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define STARTUP     1
#define RESTART     2
#define CLEAR       0

/*
 * Function Prototypes:
 */
int ITI_ATTR_init_table( const char *ttcf_fname, char **dir_entry );
int ITI_ATTR_write_directory_record() ;

#endif /* #ifndef ITI_DEF_H */
