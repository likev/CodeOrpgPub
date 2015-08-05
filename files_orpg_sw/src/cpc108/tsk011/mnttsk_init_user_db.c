
/*************************************************************************

    Reads product user and service class tables and adds them to the product
    user database.

**************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/03/14 15:06:36 $
 * $Id: mnttsk_init_user_db.c,v 1.5 2008/03/14 15:06:36 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#include <mnttsk_pd_def.h>

static char Buf[20000];

static int Read_params (int prod_id, short *params);
static int Process_elevation_parameter (char *elev_str, short *param);
static int Read_class_table (int user_db);
static int Set_line_users (int user_db, Comms_link_t *Links, int N_links);
static int Read_user_table (int user_db);
static int Update_line_classes (LB_id_t user_db, 
				Comms_link_t *links, int n_links);


/**************************************************************************

    Initializes the product user database. "user_db" is the database file
    descritor. "Links" is the link info and "N_links" is the number of links.
    Returns 0 on success or a negative error code.

**************************************************************************/

int IUD_init_user_db (Comms_link_t *Links, int N_links) {
    int user_db, ret;
    char *name;

    /* Get the product user LB name and open it */
    name = ORPGDA_lbname (ORPGDAT_USER_PROFILES);
    if (name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_USER_PROFILES);
	return (-1);
    }
    user_db = LB_open (name, LB_WRITE, NULL);
    if (user_db < 0) {
	LE_send_msg (GL_ERROR, "LB_open %s failed (%d)", name, user_db);
	return (-1);
    }
    if (ORPGMISC_is_operational ())
	Update_line_classes (user_db, Links, N_links);
    else {
	LB_clear (user_db, LB_ALL);
    
	if ((ret = Read_class_table (user_db)) < 0 ||
	    (ret == Set_line_users (user_db, Links, N_links)) < 0 ||
	    (ret = Read_user_table (user_db)) < 0) {
	    LB_close (user_db);
	    return (ret);
	}
    }
    LB_close (user_db);
    return (0);
}

/**************************************************************************

    Update line classes in the User DB according to the adaptation DB.

**************************************************************************/

static int Update_line_classes (LB_id_t user_db, 
				Comms_link_t *links, int n_links) {
    int ret;
    Pd_user_entry u_tbl;

    LB_seek (user_db, 0, LB_FIRST, NULL);
    while ((ret = LB_read (user_db, &u_tbl, 
				sizeof (Pd_user_entry), LB_NEXT)) >= 0 || 
		ret == LB_EXPIRED || ret == LB_BUF_TOO_SMALL) {
	int i;
	if (ret > 0 || ret == LB_BUF_TOO_SMALL) {
	    if (u_tbl.up_type == UP_LINE_USER) {
		for (i = 0; i < n_links; i++) {
		    if (links[i].line_ind == u_tbl.line_ind) {
			if (links[i].user_class != u_tbl.class) {
			    u_tbl.class = links[i].user_class;
			    LB_write (user_db, (char *)&u_tbl, 
					sizeof (Pd_user_entry), 
						LB_previous_msgid (user_db));
			}
			break;
		    }
		}
	    }
	}
    }

    return (0);
}

/**************************************************************************

    Generates the "line user" entries in the product user DB.

**************************************************************************/

static int Set_line_users (int user_db, Comms_link_t *Links, int N_links) {
    int i;

    for (i = 0; i < N_links; i++) {
	Pd_user_entry *u_tbl;
	int ret;

	u_tbl = (Pd_user_entry *)(Buf);
	memset (u_tbl, 0, sizeof (Pd_user_entry));
	u_tbl->up_type = UP_LINE_USER;
	u_tbl->line_ind = Links[i].line_ind;
	u_tbl->class = Links[i].user_class;
	u_tbl->defined |= UP_DEFINED_CLASS;
	u_tbl->entry_size = ALIGNED_SIZE (sizeof (Pd_user_entry));
/*
printf ("  line_ind %d  up_type %d  class %d  defined %d  max_connect_time %d  entry_size %d\n", u_tbl->line_ind, u_tbl->up_type, u_tbl->class, u_tbl->defined, u_tbl->max_connect_time, u_tbl->entry_size);
*/
	ret = LB_write (user_db, (char *)u_tbl, u_tbl->entry_size, LB_ANY);
	if (ret != u_tbl->entry_size) {
	    LE_send_msg (GL_INFO,  
		    "LB_write product user DB failed (ret %d)", ret);
	    return (-1);
	}
    }
    return (0);
}

/**************************************************************************

    Reads the product user table and adds them to the product user database.
    The LB fd of the database is "user_db". Returns 0 on success or a 
    negative error code on failure.

**************************************************************************/

static int Read_user_table (int user_db) {
    int u_cnt;
    char strtmp[NAME_SIZE], *vb;
    int err;
    
    CS_cfg_name ("product_user_table");
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);
    LE_send_msg (GL_INFO, 
		"Reading user table (%s) ...\n", CS_cfg_name (NULL));

    u_cnt = 0;
    err = 0;
    vb = NULL;
    while (1) {
	int i, ret;
	Pd_user_entry *u_tbl;
	short *s;

	ret = CS_entry (CS_NEXT_LINE, 0, NAME_SIZE, strtmp);
	if (ret == CS_FORMAT_ERROR)
	    err = 1;
	if (ret < 0)
	    break;

	u_tbl = (Pd_user_entry *)Buf;
	memset (u_tbl, 0, sizeof (Pd_user_entry));

	if (CS_entry (CS_THIS_LINE, CS_SHORT, 0, 
					(void *)&(u_tbl->user_id)) < 0 ||
	    CS_entry (CS_THIS_LINE, 1, 12, u_tbl->user_name) <= 0 ||
	    CS_entry (CS_THIS_LINE, 2, NAME_SIZE, strtmp) <= 0) {
	    CS_report ("Unexpected user line");
	    err = 1;
	    break;
	}

	u_tbl->entry_size = ALIGNED_SIZE (sizeof (Pd_user_entry));
	vb = STR_append (vb, (char *)&(u_tbl->user_id), sizeof (short));
	s = (short *)vb;
	for (i = 0; i < u_cnt; i++) {
	    if (s[i] == u_tbl->user_id) {
		LE_send_msg (GL_ERROR, 
			"Duplicated user ID (%d) found", u_tbl->user_id);
		err = 1;
		break;
	    }
	}
	if (err)
	    break;
	u_cnt++;

/*
	u_tbl->up_type = UP_DEDICATED_USER;
	ret = LB_write (user_db, (char *)u_tbl, u_tbl->entry_size, LB_ANY);
	if (ret != u_tbl->entry_size) {
	    LE_send_msg (GL_INFO,  
		    "LB_write product user DB failed (ret %d)", ret);
	    err = 1;
	    break;
	}
*/

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry (CS_THIS_LINE, 3, 12, u_tbl->user_password) <= 0)
	    u_tbl->user_password[0] = '\0';
	CS_control (CS_KEY_REQUIRED);
	u_tbl->up_type = UP_DIAL_USER;
	u_tbl->class = 2;
	u_tbl->defined |= UP_DEFINED_CLASS;

/*
printf ("  user_id %d  user_name %s up_type %d  class %d  defined %d  entry_size %d  user_password %s\n", u_tbl->user_id, u_tbl->user_name, u_tbl->up_type, u_tbl->class, u_tbl->defined, u_tbl->entry_size, u_tbl->user_password);
*/
	ret = LB_write (user_db, (char *)u_tbl, u_tbl->entry_size, LB_ANY);
	if (ret != u_tbl->entry_size) {
	    LE_send_msg (GL_INFO,  
		    "LB_write product user DB failed (ret %d)", ret);
	    err = 1;
	    break;
	}
    }
    if (vb != NULL)
	STR_free (vb);
    if (err)
	return (-1);
    return (0);
}

/**************************************************************************

    Reads the product service class table and puts the class definitions 
    to the product user database. The LB fd of the database is "user_db". 
    Returns 0 on success or a negative error code on failure.

**************************************************************************/

static int Read_class_table (int user_db) {

    int u_cnt, total_size, n;
    char strtmp[NAME_SIZE];
    int err, ret;
    
/*    CS_cfg_name ("user_profiles"); */
    CS_cfg_name ("service_class_table");
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);
    LE_send_msg (GL_INFO, 
		"Reading service class table (%s) ...\n", CS_cfg_name (NULL));

    u_cnt = total_size = 0;
    err = 1;
    while (1) {
	int size, itmp;
	Pd_user_entry *u_tbl;

	if (CS_entry (CS_NEXT_LINE, 0, NAME_SIZE, strtmp) < 0) {
	    err = 0;
	    break;
	}
	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    CS_report ("unexpected line");
	    break;
	}

	u_tbl = (Pd_user_entry *)(Buf + total_size);
	size = sizeof (Pd_user_entry);
	size = ALIGNED_SIZE (size);
	memset (u_tbl, 0, sizeof (Pd_user_entry));

	if (strcmp (strtmp, "Dial_user") == 0)
	    u_tbl->up_type = UP_DIAL_USER;
	else if (strcmp (strtmp, "Dedicated_user") == 0)
	    u_tbl->up_type = UP_DEDICATED_USER;
	else if (strcmp (strtmp, "Class") == 0)
	    u_tbl->up_type = UP_CLASS;
	else if (strcmp (strtmp,"Line_user") == 0)
	    u_tbl->up_type = UP_LINE_USER;
	else {
	    CS_report ("unknown block name");
	    break;
	}

	/* A Class definition must have a class field defined */
	if (u_tbl->up_type == UP_CLASS) {
	    if (CS_entry ("class", 1 | CS_CHAR, 0, 
					(void *)&(u_tbl->class)) <= 0)
		break;
	    u_tbl->defined |= UP_DEFINED_CLASS;
	}

	/* A Line User definition must have a line index */
	if (u_tbl->up_type == UP_LINE_USER) {
	    if (CS_entry ("line_ind", 1 | CS_CHAR, 0, 
					(void *)&(u_tbl->line_ind)) <= 0)
		break;
	}

	/* A Dedicated User definition must have user ID and name fields */
	if (u_tbl->up_type == UP_DEDICATED_USER) {
	    if (CS_entry ("user_id", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->user_id)) <= 0 ||
		CS_entry ("user_name", 1, 12, u_tbl->user_name) <= 0)
		break;
	}

	/* A Dial User definition must have a user ID, name and password */
	if (u_tbl->up_type == UP_DIAL_USER) {
	    if (CS_entry ("user_password", 1, PASSWORD_LEN, 
					u_tbl->user_password) <= 0 ||
	        CS_entry ("user_id", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->user_id)) <= 0 ||
		CS_entry ("user_name", 1, 12, u_tbl->user_name) <= 0)
		break;
	}

	/* Read optional items */
	u_tbl->defined = 0;
	u_tbl->cntl = 0;
	CS_control (CS_KEY_OPTIONAL);

	/* For a Class type record, the class_name and line_ind fields
	   are optional. The default to NULL and CLASS_ALL, respectively */
	if (u_tbl->up_type == UP_CLASS) {
	    CS_entry ("class_name", 1, 12, u_tbl->user_name);
	    CS_entry ("line_ind", 1 | CS_CHAR, 0,
				(void *)&(u_tbl->line_ind));
	}

	/* For a Line User type record the port password field is optional 
	   (i.e., this is appropriate for dial-in lines). */
	if (u_tbl->up_type == UP_LINE_USER) {
	    CS_entry ("port_password", 1, 12, u_tbl->user_password);
	    CS_entry ("name", 1, 12, u_tbl->user_name);
	}

	/* For all Users other than CLASS the class field is optional. If it 
	   is defined, this class definition supercedes the line class 
	   definiton for the line the user is connected to. */
	if ((u_tbl->up_type != UP_CLASS) &&
	    CS_entry ("class", 1 | CS_CHAR, 0, 
					(void *)&(u_tbl->class)) > 0)
	    u_tbl->defined |= UP_DEFINED_CLASS;

	/* Check to see if any of the lists are defined. */
	if (CS_entry ("pms_len", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->pms_len)) > 0)
	    u_tbl->defined |= UP_DEFINED_PMS;
	if (CS_entry ("dd_len", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->dd_len)) > 0)
	    u_tbl->defined |= UP_DEFINED_DD;
	if (CS_entry ("map_len", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->map_len)) > 0)
	    u_tbl->defined |= UP_DEFINED_MAP;

	if (CS_entry ("max_connect_time", 1 | CS_SHORT, 0, 
				(void *)&(u_tbl->max_connect_time)) > 0)
	    u_tbl->defined |= UP_DEFINED_MAX_CONNECT_TIME;
	if (CS_entry ("n_req_prods", 1 | CS_SHORT, 0, 
					(void *)&(u_tbl->n_req_prods)) > 0)
	    u_tbl->defined |= UP_DEFINED_N_REQ_PRODS;
	if (CS_entry ("wait_time_for_rps", 1 | CS_SHORT, 0, 
				(void *)&(u_tbl->wait_time_for_rps)) > 0)
	    u_tbl->defined |= UP_DEFINED_WAIT_TIME_FOR_RPS;
	if (CS_entry ("distri_method", 1 | CS_SHORT, 0, 
				(void *)&(u_tbl->distri_method)) > 0)
	    u_tbl->defined |= UP_DEFINED_DISTRI_METHOD;

	if (CS_entry ("UP_CD_OVERRIDE", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_OVERRIDE;
	    if (itmp)
		u_tbl->cntl |= UP_CD_OVERRIDE;
	}
	if (CS_entry ("UP_CD_APUP_STATUS", 
			1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_APUP_STATUS;
	    if (itmp)
		u_tbl->cntl |= UP_CD_APUP_STATUS;
	}
	if (CS_entry ("UP_CD_RPGOP", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_RPGOP;
	    if (itmp)
		u_tbl->cntl |= UP_CD_RPGOP;
	}
	if (CS_entry ("UP_CD_ALERTS", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_ALERTS;
	    if (itmp)
		u_tbl->cntl |= UP_CD_ALERTS;
	}
	if (CS_entry ("UP_CD_COMM_LOAD_SHED", 
			1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_COMM_LOAD_SHED;
	    if (itmp)
		u_tbl->cntl |= UP_CD_COMM_LOAD_SHED;
	}
	if (CS_entry ("UP_CD_STATUS", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_STATUS;
	    if (itmp)
		u_tbl->cntl |= UP_CD_STATUS;
	}
	if (CS_entry ("UP_CD_MAPS", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_MAPS;
	    if (itmp)
		u_tbl->cntl |= UP_CD_MAPS;
	}
	if (CS_entry ("UP_CD_PROD_GEN_LIST", 
			1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_PROD_GEN_LIST;
	    if (itmp)
		u_tbl->cntl |= UP_CD_PROD_GEN_LIST;
	}
	if (CS_entry ("UP_CD_PROD_DISTRI_LIST", 
			1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_PROD_DISTRI_LIST;
	    if (itmp)
		u_tbl->cntl |= UP_CD_PROD_DISTRI_LIST;
	}
	if (CS_entry ("UP_CD_RCM", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_RCM;
	    if (itmp)
		u_tbl->cntl |= UP_CD_RCM;
	}
	if (CS_entry ("UP_CD_DAC", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_DAC;
	    if (itmp)
		u_tbl->cntl |= UP_CD_DAC;
	}
	if (CS_entry ("UP_CD_MULTI_SRC", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_MULTI_SRC;
	    if (itmp)
		u_tbl->cntl |= UP_CD_MULTI_SRC;
	}
	if (CS_entry ("UP_CD_AAPM", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_AAPM;
	    if (itmp)
		u_tbl->cntl |= UP_CD_AAPM;
	}
	if (CS_entry ("UP_CD_IMM_DISCON", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_IMM_DISCON;
	    if (itmp)
		u_tbl->cntl |= UP_CD_IMM_DISCON;
	}
	if (CS_entry ("UP_CD_NO_SCHEDULE", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_NO_SCHEDULE;
	    if (itmp)
		u_tbl->cntl |= UP_CD_NO_SCHEDULE;
	}
	if (CS_entry ("UP_CD_FREE_TEXTS", 1 | CS_INT, 0, (void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_FREE_TEXTS;
	    if (itmp)
		u_tbl->cntl |= UP_CD_FREE_TEXTS;
	}
	if (CS_entry ("UP_CD_RESTRICTED_RCM", 1 | CS_INT, 0, 
						(void *)&itmp) > 0) {
	    u_tbl->defined |= UP_CD_RESTRICTED_RCM;
	    if (itmp)
		u_tbl->cntl |= UP_CD_RESTRICTED_RCM;
	}
	CS_control (CS_KEY_REQUIRED);

	if (u_tbl->pms_len > 0) {	/* read the permission table */
	    Pd_pms_entry *item;
	    int cnt;

	    CS_entry ("pms_len", 0, 0, NULL);
	    if (CS_level (CS_DOWN_LEVEL) < 0) {
		LE_send_msg (GL_INFO,  "prod distri permission list missing");
		break;
	    }

	    cnt = 0;
	    u_tbl->pms_list = size;
	    item = (Pd_pms_entry *)(Buf + total_size + size);
	    while (CS_entry (CS_THIS_LINE, CS_SHORT, 0,
				(void *)&(item[cnt].prod_id)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 1 | CS_BYTE, 0,
				(void *)&(item[cnt].wx_modes)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 2 | CS_BYTE, 0,
				(void *)&(item[cnt].types)) > 0) {

		/* We need to convert the product code to product id. */
		if (item[cnt].prod_id > 0) {
		    item[cnt].prod_id = ORPGPAT_get_prod_id_from_code (
					(int) item[cnt].prod_id);
		    if (item[cnt].prod_id < 0)
			LE_send_msg (GL_INFO,
			"Unable to convert product code (%d) to product id", 
						item[cnt].prod_id);
		    else
			cnt++;
		}
		else
		    cnt++;
	    	if (CS_entry (CS_NEXT_LINE, 0, 0, NULL) < 0)
		    break;
	    }
	    if (cnt != u_tbl->pms_len) {
		CS_report ("bad prod distri permission list");
		break;
	    }
	    size += cnt * sizeof (Pd_pms_entry);
	    size = ALIGNED_SIZE (size);

	    CS_level (CS_UP_LEVEL);
	}

	if (u_tbl->dd_len > 0) {	/* read the default distri table */
	    Pd_prod_item *item;
	    int cnt;

	    CS_entry ("dd_len", 0, 0, NULL);
	    if (CS_level (CS_DOWN_LEVEL) < 0) {
		LE_send_msg (GL_INFO | 14,  "default distri list missing");
		break;
	    }

	    cnt = 0;
	    u_tbl->dd_list = size;
	    item = (Pd_prod_item *)(Buf + total_size + size);
	    while (CS_entry (CS_THIS_LINE, CS_SHORT, 0,
				(void *)&(item[cnt].prod_id)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 1 | CS_BYTE, 0,
				(void *)&(item[cnt].wx_modes)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 2 | CS_BYTE, 0,
				(void *)&(item[cnt].period)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 3 | CS_BYTE, 0,
				(void *)&(item[cnt].number)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 4 | CS_BYTE, 0,
				(void *)&(item[cnt].map_requested)) > 0 &&
	    	   CS_entry (CS_THIS_LINE, 5 | CS_BYTE, 0,
				(void *)&(item[cnt].priority)) > 0) {

		/* We need to convert the input product code to product id. */
		if (item[cnt].prod_id > 0) {
		    item[cnt].prod_id = ORPGPAT_get_prod_id_from_code (
					item[cnt].prod_id);
		    if (item[cnt].prod_id < 0)
			LE_send_msg (GL_INFO,
			"Unable to convert product code (%d) to product id",
						item[cnt].prod_id);
		    else {
			if (Read_params ((int) item[cnt].prod_id, 
						item[cnt].params) == 0)
			    cnt++;
			else
			    CS_report ("Read_params failed");
		    }
		}
		else
		    cnt++;
	    	if (CS_entry (CS_NEXT_LINE, 0, 0, NULL) < 0)
		    break;
	    }
	    if (cnt != u_tbl->dd_len) {
		CS_report ("bad default distri list");
		break;
	    }
	    size += cnt * sizeof (Pd_prod_item);
	    size = ALIGNED_SIZE (size);

	    CS_level (CS_UP_LEVEL);
	}

	CS_control (CS_KEY_OPTIONAL);
	if (u_tbl->map_len > 0) {	/* read the map table */
	    prod_id_t *item;
	    int cnt;

	    cnt = 0;
	    u_tbl->map_list = size;
	    item = (prod_id_t *)(Buf + total_size + size);
	    while (CS_entry ("map_ids", (cnt + 1) | CS_SHORT, 0,
					(void *)&(item[cnt])) > 0)
		cnt++;
	    if (cnt != u_tbl->map_len) {
		CS_report ("bad map list");
		break;
	    }
	    size += cnt * sizeof (prod_id_t);
	    size = ALIGNED_SIZE (size);
	}
	CS_control (CS_KEY_REQUIRED);
	u_cnt++;
	u_tbl->entry_size = size;
	total_size += size;
/*
if (u_tbl->up_type == UP_LINE_USER)
printf ("  line_ind %d  up_type %d  class %d  defined %d  max_connect_time %d  entry_size %d  user_password %s\n", u_tbl->line_ind, u_tbl->up_type, u_tbl->class, u_tbl->defined, u_tbl->max_connect_time, u_tbl->entry_size, u_tbl->user_password);

if (u_tbl->up_type == UP_DIAL_USER || u_tbl->up_type == UP_DEDICATED_USER)
printf ("	%5d	%20s	\"description\"	%12s\n", u_tbl->user_id, u_tbl->user_name, u_tbl->user_password);
*/

	/* Write a user profile DB record */
	ret = LB_write (user_db, (char *)u_tbl, size, LB_ANY);
	if (ret != size) {
	    LE_send_msg (GL_INFO,  
		    "LB_write product user DB failed (ret %d)", ret);
	    err = 1;
	    break;
	}
	
	/* Go to the next record definition. */
	CS_level (CS_UP_LEVEL);
    }

    if (!err &&
	(n = CS_entry (NULL, CS_UNREAD_KEYS, NAME_SIZE, strtmp)) > 0) {
	LE_send_msg (GL_INFO, "Warning: %d keys in file %s never used: %s", 
					n, CS_cfg_name (NULL), strtmp);
	err = 1;
    }
    if (err)
	return (-1);
    return (0);
}

/**************************************************************************

    Reads and parses the six product parameters.

**************************************************************************/

#define TBUF_SIZE 16

static int Read_params (int prod_id, short *params) {
    char tmp[TBUF_SIZE];
    int elev_ind, i;

    /* If this product is elevation-based, save the parameter index. */
    elev_ind = ORPGPAT_elevation_based (prod_id);

    for (i = 0; i < 6; i++) {
	if (CS_entry (CS_THIS_LINE, i + 6, TBUF_SIZE, (void *)tmp) > 0) {
	    int v;

	    if (strcmp (tmp, "UNU") == 0)
		params[i] = PARAM_UNUSED;
	    else if (strcmp (tmp, "ANY") == 0)
		params[i] = PARAM_ANY_VALUE;
	    else if (strcmp (tmp, "ALG") == 0)
		params[i] = PARAM_ALG_SET;
	    else if (strcmp (tmp, "ALL") == 0)
		params[i] = PARAM_ALL_VALUES;
	    else if (strcmp (tmp, "EXS") == 0)
		params[i] = PARAM_ALL_EXISTING;
	    else {
		/* If this product is elevation-based, then the elevation 
		   parameter has special format. If product not elevtion-based,
		   elev_ind is negative value. */
		if (elev_ind == i) {
		    if (Process_elevation_parameter (tmp, &params[i]) < 0)
			return (-1); 
		}
		else { 
		    if (sscanf (tmp, "%d", &v) == 1)
			params[i] = v;
		    else {
			LE_send_msg (GL_ERROR, "!!!!!!sscanf Failed\n");
			return (-1);
		    }
		}
	    }
	}
	else {
	    LE_send_msg (GL_ERROR, "!!!!!!CS_entry Failed\n"); 
	    return (-1);
	}
    }
    return (0);
}

/**************************************************************************

    Processes the elevation parameter.

**************************************************************************/

static int Process_elevation_parameter (char *elev_str, short *param) {
    int j, value;
    char *bits;

    /* Check if elevation parameter is in special format. */
    j = 0;
    while (elev_str[j] != '-') {
	j++;
	if (j >= TBUF_SIZE)
	    break;
    }
		   
    /* The elevation parameter is defined in special format. */
    if (j < TBUF_SIZE) {
	char tmp[TBUF_SIZE];
    
	/* Copy "elev_str" to temporary buffer. */
	strcpy (tmp, elev_str);
    
	/* Validate the elevation/cut field.  If not valid, return error. */
	if ((j >= (TBUF_SIZE-2)) || (sscanf (&tmp[j + 1], "%d", &value) != 1)
		   || (*param < 0)) {
	    LE_send_msg (GL_ERROR, 
			"!!!!!!sscanf of Elevation Parameter Failed\n");
	    return (-1);
	}
    
	*param = (short)value;
    
	/* Test the "bit" values ... set parameter accordingly. */
	tmp[j] = '\0';
	if ((bits = strstr (tmp, "00")) != NULL)
	    return 0;
    
	else if ((bits = strstr (tmp, "01" )) != NULL) {
	    *param |= ORPGPRQ_LOWER_ELEVATIONS;
	    return 0;
	} 
	else if ((bits = strstr (tmp, "10")) != NULL) {
	    *param = ORPGPRQ_ALL_ELEVATIONS;
	    return 0;
	} 
	else if ((bits = strstr (tmp, "11")) != NULL) {
	    *param |= ORPGPRQ_LOWER_CUTS;
	    return 0;
	} 
	else {
	   LE_send_msg( GL_ERROR, 
		"!!!!!!Unknown Bits Set For Elevation Parameter\n" );
	   return (-1);
	}
    }
    else {
	if (sscanf (elev_str, "%d", &value) == 1) {
	    *param = (short) value;
	    return 0;
	}
	else {
	    LE_send_msg( GL_ERROR, 
			"!!!!!!sscanf for Elevation Parameter Failed\n" ); 
	    return (-1);
	}
    }
    
    return 0;
}
