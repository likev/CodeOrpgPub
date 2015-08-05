/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:12:44 $
 * $Id: orpgcfg.c,v 1.16 2002/12/11 21:12:44 nolitam Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/**************************************************************************

      Module:  orpgcfg.c

 Description:
	This file provides ORPG library configuration routines.

	Functions that are public are defined in alphabetical order at
	the top of this file and are identified by a prefix of
	"ORPGCFG_".

	The scope of all other routines defined within this file is
	limited to this file.  The private functions are defined in
	alphabetical order, following the definitions of the API functions.

 Interruptible System Calls:
	TBD

 Memory Allocation:
	None

 Assumptions:
	TBD

 **************************************************************************/



#include <stdio.h>
#include <stdlib.h>            /* malloc()                                */
#include <string.h>            /* strncpy()                               */

#include <infr.h>
#include <orpg.h>
#include <orpgcfg.h>


/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */

/*
 * Static Global Variables
 */

/**************************************************************************
   Description: 
      Given datastore Data ID, return the corresponding datastore
      path (null-terminated string).

   Input: 
      datastore Data ID
      pointer to system configuration filename

   Output: 

   Returns: 
      upon success, a pointer to the character string representation
      of the datastore pathname; otherwise, a pointer to an empty
      string.

   Notes: 
      CS_entry guarantees that the pathname will be null-terminated

**************************************************************************/
const char * ORPGCFG_dataid_to_path(int data_id, const char *syscfg_fname){

    static char *empty_path = "" ;
    static char path[ORPG_PATHNAME_SIZ] ;
                               /* pathname corresponding to data ID ...   */
    char *save_cfg_name ;      /* CS cfg name upon entering this routine  */
    char *tmp_char_p ;

    /*
     * Save a copy of the current CS configuration name, as we'll
     * need to restore that when we leave this function ...
     *
     * CS_cfg_name() returns a NULL-terminated string ...
     */
    tmp_char_p = CS_cfg_name(NULL) ;
    if (strlen(tmp_char_p) > 0) {
        save_cfg_name = malloc(strlen(tmp_char_p) + 1) ;    
        if (save_cfg_name != NULL) {
            (void) strncpy(save_cfg_name,
                           (const char *) tmp_char_p,
                           strlen(tmp_char_p)+1) ;
        }
        else {
            return(empty_path) ;
        }
    }
    else {
        save_cfg_name = malloc(strlen("") + 1) ;    
        if (save_cfg_name != NULL) {
            (void) strncpy(save_cfg_name,
                           (const char *) "",
                           strlen("")+1) ;
        }
        else {
            return(empty_path) ;
        }
    }

    if ((syscfg_fname == NULL)
                      ||
        (strlen(syscfg_fname) == 0)) {
        (void) CS_cfg_name("") ;
    }
    else {
        (void) CS_cfg_name(syscfg_fname) ;
    }

    CS_control(CS_COMMENT | ORPGCFG_CS_SYSCFG_COMMENT) ;

    path[0] = '\0' ;

    /*
     * Using an integer key is a little different ...
     */
    if (CS_entry((char *) data_id,
                 CS_INT_KEY | ORPGCFG_CS_SYSCFG_PATH_TOK,
                 ORPG_PATHNAME_SIZ,
                 (void *) path) <= 0) {
         (void) CS_cfg_name(save_cfg_name) ;
         free(save_cfg_name) ;
         return(empty_path) ;
    }

    (void) CS_cfg_name(save_cfg_name) ;
    free(save_cfg_name) ;


    return((const char *) path) ;


/*END of ORPGCFG_dataid_to_path()*/
}
