/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:12:45 $
 * $Id: orpgcmi.c,v 1.13 2002/12/11 21:12:45 nolitam Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

/****************************************************************
		
    Module: orpgcmi.c	
		
    Description: This file contains the CMI (comm_manager 
		interface) module of liborpg.a.

*****************************************************************/



/* System include files */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Local include files */

#include <prod_distri_info.h>
#include <orpg.h>
#include <infr.h>

typedef struct {
    short line_ind;
    short req_ind;
    short resp_ind;
} Orpgcmi;

static int N_lines = -1;
static Orpgcmi *list;

static int Wb_req_id = -1;
static int Wb_resp_id = -1;


/* local functions */
static void Initialize ();


/*************************************************************************

    Description: This function returns the data store ID for sending
		requests to the communication link "line_ind".

    Return:	returns the data store ID on success or -1 on failure.

*************************************************************************/

int ORPGCMI_request (int line_ind)
{
    int i;

    if (N_lines < 0)
	Initialize ();

    for (i = 0; i < N_lines; i++) {
	if (line_ind == list[i].line_ind)
	    break;
    }

    if (i < N_lines)
	return (ORPGDAT_CM_REQUEST + list[i].req_ind);
    else
	return (-1);
}

/*************************************************************************

    Description: This function returns the data store ID for receiving
		responses from the communication link "line_ind".

    Return:	returns the data store ID on success or -1 on failure.

*************************************************************************/

int ORPGCMI_response (int line_ind)
{
    int i;

    if (N_lines < 0)
	Initialize ();

    for (i = 0; i < N_lines; i++) {
	if (line_ind == list[i].line_ind)
	    break;
    }

    if (i < N_lines)
	return (ORPGDAT_CM_RESPONSE + list[i].resp_ind);
    else
	return (-1);
}

/*************************************************************************

    Description: This function returns the data store ID for sending
		requests to the RDA communication link.

    Return:	returns the data store ID on success or -1 on failure.

*************************************************************************/

int ORPGCMI_rda_request ()
{

    if (N_lines < 0)
	Initialize ();

    return (Wb_req_id);
}

/*************************************************************************

    Description: This function returns the data store ID for reading
		responses from the RDA communication link.

    Return:	returns the data store ID on success or -1 on failure.

*************************************************************************/

int ORPGCMI_rda_response ()
{

    if (N_lines < 0)
	Initialize ();

    return (Wb_resp_id);
}

/*************************************************************************

    Description: This function looks up the request and response LB data
		IDs.

*************************************************************************/

#ifndef NAME_SIZE
#define NAME_SIZE 256
#endif

static void Initialize ()
{
    int lind, nlinks;
    int rda_link, len;
    char *buf;
    Pd_distri_info *p_tbl;
    Pd_line_entry *l_tbl;

    N_lines = 0;
    CS_cfg_name ("");

    if (CS_entry ("RDA_link", 1 | CS_INT, 0, (void *)&rda_link) < 0) {
	LE_send_msg (GL_INPUT, "ORPGCMI: RDA_link not found");
	return;
    }

    if ((len = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, 
			LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID)) < 0) {
	LE_send_msg (GL_INPUT, "ORPGCMI: ORPGDA_read ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID failed (ret %d)", len);
	return;
    }

    p_tbl = (Pd_distri_info *)buf;
    if (len < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list < (int)sizeof (Pd_distri_info) ||
	p_tbl->line_list + 
		p_tbl->n_lines * (int)sizeof (Pd_line_entry) > len) {
	LE_send_msg (GL_INPUT, "ORPGCMI: error in PD_LINE_INFO_MSG_ID message");
	free (buf);
	return;
    }
    l_tbl = (Pd_line_entry *)(buf + p_tbl->line_list);

    nlinks = p_tbl->n_lines;
    list = (Orpgcmi *)malloc (nlinks * sizeof (Orpgcmi));
    if (list == NULL) {
	LE_send_msg (0, "ORPGCMI: malloc failed in orpgcmi lib");
	free (buf);
	return;
    }
    Wb_req_id = Wb_resp_id = -1;
    for (lind = 0; lind < nlinks; lind++) {

	list[lind].req_ind = l_tbl[lind].cm_ind;
	list[lind].resp_ind = lind;
	list[lind].line_ind = lind;

	if (lind == rda_link) {
	    Wb_req_id = ORPGDAT_CM_REQUEST + l_tbl[lind].cm_ind;
	    Wb_resp_id = ORPGDAT_CM_RESPONSE + lind;
	}
    }
    CS_cfg_name ("");
    if (lind == nlinks)
	N_lines = nlinks;

    free (buf);
    return;
}
