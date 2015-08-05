/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:27 $
 * $Id: hci_user_profile_functions.c,v 1.9 2009/02/27 22:26:27 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_user_profile_functions.c				*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate the User info message in  the	*
 *		      PROD_INFO linear buffer.				*
 *									*
 ************************************************************************/
 
/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_user_info.h>

static int Rec_ind = 0;
static void *qr = NULL;			/* query results */


/************************************************************************

    Returns the next record in the current SQL search result "qr" or NULL
    on failure.

 ************************************************************************/

void *hci_get_next_user_rec () {
    void *rec;
    int ret;

    if (qr == NULL)
	return (NULL);
    if (SDQ_get_query_error (qr) != 0)
	return (NULL);
    if ((ret = SDQ_get_record (qr, Rec_ind, (char **)&rec)) < 0) {
	HCI_LE_error( "SDQ_get_record (user DB) failed (%d)", ret);
	return (NULL);
    }
    Rec_ind++;
    return (rec);
}

/************************************************************************

    Performs an SQL search of "sql_text" in the product user database. 
    Returns the number of records found or 0 on failure.

 ************************************************************************/

int hci_search_users (char *sql_text) {
    static char *lb_name = NULL;
    int ret;

    if (lb_name == NULL) {
	char *name = ORPGDA_lbname (ORPGDAT_USER_PROFILES);
	if (name == NULL) {
	    HCI_LE_error( "ORPGDA_lbname (ORPGDAT_USER_PROFILES) failed" );
	    return (0);
	}
	lb_name = STR_copy (lb_name, name);
	HCI_LE_log( "User DB: %s", lb_name );
    }

    if (qr != NULL) {
	free (qr);
	qr = NULL;
	Rec_ind = 0;
    }
    ret = SDQ_select (lb_name, sql_text, (void *)&qr);
    if (ret == CSS_SERVER_DOWN) {
	HCI_LE_log( "Start rpgdbm ..." );
	ORPGMGR_start_rpgdbm (lb_name);
	ret = SDQ_select (lb_name, sql_text, (void *)&qr);
    }
    else if (ret == CSS_LOST_CONN) {
	HCI_LE_log( "CSS connection lost, will retry..." );
	ret = SDQ_select (lb_name, sql_text, (void *)&qr);
    }
    if (ret < 0) {
	HCI_LE_error( "SDQ_select (%s) failed, ret %d", sql_text, ret );
	return (0);
    }
    return (ret);
}
