/*************************************************************************

    Module: mnttsk_ls_levels.c

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/05/25 13:31:53 $
 * $Id: mnttsk_ls_levels.c,v 1.15 2004/05/25 13:31:53 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <string.h>            /* strncpy()                               */

#include "mnttsk_ls_def.h"

#include <orpgload.h>




/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */


/*
 * Static Global Variables
 */
                               /** file name for load shed table */
static char Load_shed_table_name[ORPG_PATHNAME_SIZ];

/*
 * Static Function Prototypes
 */
static  int Init_load_shed_tbl (void);


/********************************************************************

   Description:
      Controls load shed table initialization based on startup mode.

   Input:
      startup_action - start up mode (either STARTUP or RESTART)

   Outputs:

   Returns:
      Returns 0 on success, or exits with non-zero exit code on 
      error.

   Notes:

*********************************************************************/
int MNTTSK_LS_LEVELS_maint(int startup_action){

    int retval ;


    /* Assign ASCII loadshed table file name.  The macro 
       ORPGLOAD_CS_DFLT_TABLE_FNAME is defined in header file orpgload.h */
    (void) memset(Load_shed_table_name, 0, sizeof(Load_shed_table_name)) ;
    (void) strncpy(Load_shed_table_name, ORPGLOAD_CS_DFLT_TABLE_FNAME,
                   sizeof(Load_shed_table_name) - 1);

    if (startup_action == STARTUP) {

        LE_send_msg(GL_INFO, "LS LEVELS Maint. INIT FOR STARTUP DUTIES:") ;

        LE_send_msg(GL_INFO,
        "\t1. Initialize the Load Shed Categories Messages.") ;

        retval = Init_load_shed_tbl() ;
	LE_send_msg(GL_INFO, "%d = Init_load_shed_tbl()");

        if (retval < 0) {

            LE_send_msg(GL_INFO,
                        "Data ID %d: Init_load_shed_tbl() failed: %d",
                        ORPGDAT_LOAD_SHED_CAT, retval) ;
            exit(1) ;

        }
    }
    else 
        LE_send_msg(GL_INFO, "NO LS LEVELS Maint. DUTIES FOR %d",
                    startup_action) ;

    return(0) ;

/*END of MNTTSK_LS_LEVELS_maint()*/
}

/**************************************************************************

   Description: 

      Parses ASCII loadshed table and set elements of binary loadshed
      table via ORPGLOAD family of functions.

   Inputs:

   Outputs:

   Returns:	
      It returns the number of entries of the table on success or -1
      on failure.

   Notes:

**************************************************************************/
static int Init_load_shed_tbl (void){

	int	cnt;
	int	total_size;
	int	err;
	load_shed_threshold_t	thr;
	load_shed_current_t	cur;

	cnt = total_size = err = 0;

	LE_send_msg(GL_INFO,
		"% s= Load_shed_table_name",Load_shed_table_name);
	LE_send_msg (GL_INFO,"Load_shed_table_name = %s\n",
		Load_shed_table_name);
	CS_cfg_name (Load_shed_table_name);
	CS_control  (CS_COMMENT | '#');

	if (CS_entry ("Load_shed_threshold_categories", 0, 0, NULL) < 0 ||
	    CS_level (CS_DOWN_LEVEL) < 0) {

	    LE_send_msg (GL_INFO,"Unable to find Load_shed_threshold_categories in %s\n",
		Load_shed_table_name);

	    return (-1);

	}

	thr.prod_dist_warn     = 0;
	thr.prod_dist_alarm    = 0;
	cur.prod_dist          = 0;
	cur.prod_dist_state    = 0;
	thr.prod_storage_warn  = 0;
	thr.prod_storage_alarm = 0;
	cur.prod_storage       = 0;
	cur.prod_storage_state = 0;
	thr.input_buf_warn     = 0;
	thr.input_buf_alarm    = 0;
	cur.input_buf          = 0;
	cur.input_buf_state    = 0;
	thr.rda_radial_warn    = 0;
	thr.rda_radial_alarm   = 0;
	cur.rda_radial         = 0;
	cur.rda_radial_state   = 0;
	thr.rpg_radial_warn    = 0;
	thr.rpg_radial_alarm   = 0;
	cur.rpg_radial         = 0;
	cur.rpg_radial_state   = 0;
	thr.wb_user_warn       = 0;
	thr.wb_user_alarm      = 0;
	cur.wb_user            = 0;
	cur.wb_user_state      = 0;

	do {

	    int	warn;
	    int	alarm;
	    char	name [16];

	    if (CS_level (CS_DOWN_LEVEL) < 0) {

		continue;

	    }

	    if (CS_entry ("warn",   1 | CS_INT, 0, 
					(void *)&(warn))  <= 0 ||
		CS_entry ("alarm",  1 | CS_INT, 0, 
					(void *)&(alarm)) <= 0 ||
		CS_entry ("name",   1,          16,
					(void *)(name))   < 0) {

		err = 1;
		break;

	    }

	    LE_send_msg(GL_INFO,
	    	"Found %s load shed category data",
			name);

	    if (!strncmp (name,"Distribution",12)) {

		thr.prod_dist_warn  = (unsigned char) warn;
		thr.prod_dist_alarm = (unsigned char) alarm;

	    } else if (!strncmp (name,"Storage",7)) {

		thr.prod_storage_warn  = (unsigned char) warn;
		thr.prod_storage_alarm = (unsigned char) alarm;

	    } else if (!strncmp (name,"Input Buffer",12)) {

		thr.input_buf_warn   = (unsigned char) warn;
		thr.input_buf_alarm  = (unsigned char) alarm;
		thr.rda_radial_warn  = (unsigned char) warn;
		thr.rda_radial_alarm = (unsigned char) alarm;
		thr.rpg_radial_warn  = (unsigned char) warn;
		thr.rpg_radial_alarm = (unsigned char) alarm;

	    } else if (!strncmp (name,"Wideband User",13)) {

		thr.wb_user_warn  = (unsigned char) warn;
		thr.wb_user_alarm = (unsigned char) alarm;

	    }

	    cnt++;

	    CS_level (CS_UP_LEVEL);

	} while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

	LE_send_msg (GL_INFO,"err = %d\n", err);

	CS_cfg_name ("");

	{
	    int	i;
	    int	status;
	    int	warn;
	    int	alarm;
	    char	*buf;

/*	    Get destination message information - if it exists and	*
 *	    is of non-zero length, we will not update.			*/

	    status = ORPGDA_read (ORPGDAT_LOAD_SHED_CAT,
				  &buf,
				  LB_ALLOC_BUF,
				  LOAD_SHED_THRESHOLD_MSG_ID);

	    if (status <= 0) {

/*	        Issue an LB write for both messages to initialize them	*
 *	        (should be all 0).					*/

		status = ORPGLOAD_write (LOAD_SHED_THRESHOLD_MSG_ID);

		LE_send_msg(GL_INFO,
	    		"%d = ORPGLOAD_write (LOAD_SHED_THRESHOLD_MSG_ID)",
			status);

		status = ORPGLOAD_write (LOAD_SHED_THRESHOLD_BASELINE_MSG_ID);

		LE_send_msg(GL_INFO,
	    		"%d = ORPGLOAD_write (LOAD_SHED_THRESHOLD_BASELINE_MSG_ID)",
			status);

		status = ORPGLOAD_write (LOAD_SHED_CURRENT_MSG_ID);

		LE_send_msg(GL_INFO,
	    		"%d = ORPGLOAD_write (LOAD_SHED_CURRENT_MSG_ID)",
			status);

/*	        Begin setting all of the warning and alarm threshold	*
 *	        values using the ORPGLOAD set of library functions.	*/

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.prod_dist_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Distribution warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,WARNING)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.prod_dist_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Distribution alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,ALARM)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.prod_storage_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Storage warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,WARNING)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.prod_storage_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Storage alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,ALARM)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.input_buf_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Input Buffer warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,WARNING)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.input_buf_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Input Buffer alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,ALARM)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.rda_radial_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set RDA Radial warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,WARNING)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.rda_radial_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set RDA Radial alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,ALARM)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.rpg_radial_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set RPG Radial warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,WARNING)",
			status);

	 	}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.rpg_radial_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set RPG Radial alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,ALARM)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,
					LOAD_SHED_WARNING_THRESHOLD,
					(int) thr.wb_user_warn);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Wideband User warning field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,WARNING)",
			status);

		}

		status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,
					LOAD_SHED_ALARM_THRESHOLD,
					(int) thr.wb_user_alarm);

		if (status != 0) {

		    LE_send_msg (GL_INFO, "Unable to set Wideband User alarm field in LSC table");
		    LE_send_msg(GL_INFO,
	    	    "%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,ALARM)",
			status);

		}

/*		Write out the threshold information.			*
 *		NOTE:  We are assuming that if the threshold message	*
 *		didn't exist then the baseline threshold message didn't	*
 *		exists and needs initializing.				*/


		status = ORPGLOAD_write (LOAD_SHED_THRESHOLD_MSG_ID);

		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_write (LOAD_SHED_THRESHOLD_MSG_ID)",
			status);

		status = ORPGLOAD_write (LOAD_SHED_THRESHOLD_BASELINE_MSG_ID);

		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_write (LOAD_SHED_THRESHOLD_BASELINE_MSG_ID)",
			status);

		for (i=LOAD_SHED_CATEGORY_PROD_DIST;i<=LOAD_SHED_CATEGORY_WB_USER;i++) {

		    status = ORPGLOAD_get_data (i,
					   LOAD_SHED_WARNING_THRESHOLD,
					   &warn);
		    status = ORPGLOAD_get_data (i,
					   LOAD_SHED_ALARM_THRESHOLD,
					   &alarm);
		    LE_send_msg (GL_INFO,"[%d] - (%d,%d)\n", i, warn, alarm);

		}

	    } else {

		free (buf);

	    }

/*	    Write the load shed current data to an LB. (initially all	*
 *	    values 0.							*/

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.prod_dist);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set Distribution current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,CURRENT)",
			status);

	    }

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.prod_storage);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set Storage current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_STORAGE,CURRENT)",
			status);

	    }

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.input_buf);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set Input Buffer current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_INPUT_BUF,CURRENT)",
			status);

	    }

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.rda_radial);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set RDA Radial current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RDA_RADIAL,CURRENT)",
			status);

	    }

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.rpg_radial);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set RPG Radial current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_RPG_RADIAL,CURRENT)",
			status);

	    }

	    status = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,
					LOAD_SHED_CURRENT_VALUE,
					(int) cur.wb_user);

	    if (status != 0) {

		LE_send_msg (GL_INFO, "Unable to set Wideband User current field in LSC table");
		LE_send_msg(GL_INFO,
	    	"%d = ORPGLOAD_set_data (LOAD_SHED_CATEGORY_WB_USER,CURRENT)",
			status);

	    }
	}

	return (cnt);

}
