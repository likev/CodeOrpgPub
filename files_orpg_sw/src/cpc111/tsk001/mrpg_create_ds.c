
/******************************************************************

	file: mrpg_create_ds.c

	Creates the ORPG data stores.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/09/27 16:40:31 $
 * $Id: mrpg_create_ds.c,v 1.18 2012/09/27 16:40:31 jing Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#include <mrpg.h> 
#include "mrpg_def.h"


static int Recreate_lbs = 0;
static int Recreate_all_lbs = 0;
static int Mrpg_cmd = 0;

static int Lb_created = 0;		/* at lease one of the global LBs is
					   created/recreated */

static int Get_lb_name (Mrpg_dat_entry_t *da, char *buf, int buf_size);
static int Create_lb (char *name, Mrpg_dat_entry_t *da);
static int Check_and_create_lb (char *name, Mrpg_dat_entry_t *da);
static int Check_clear_and_create_lb (char *name, Mrpg_dat_entry_t *da);
static int Get_all_lb_names (Mrpg_dat_entry_t *da, char **names);
static void Verify_permanent_files ();


/******************************************************************

    Initializes this module.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MCD_init () {

    Mrpg_cmd = MAIN_command ();
    return (0);
}

/******************************************************************

    Sets Recreate_lbs flag.
	
******************************************************************/

void MCD_recreate_lbs () {
    Recreate_lbs = 1;
}

/******************************************************************

    Sets Recreate_all_lbs flag.
	
******************************************************************/

void MCD_recreate_all_lbs (yes) {
    Recreate_all_lbs = yes;
}

/******************************************************************

    Returns Lb_created flag.
	
******************************************************************/

int MCD_is_lb_created () {
    return (Lb_created);
}

/******************************************************************

    Creates RPG data stores.

    Input:	sw - functional switch. MRPG_STARTUP or MRPG_RESTART.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MCD_create_ds (int sw) {
    Mrpg_dat_entry_t *dat, *pat;
    int n_dat, n_pat, n_lbs;
    int i, k, ret;
    char buf[128], *names, *namep;

    strcpy (buf, "Checking/creating/clearing RPG data stores - ");
    if (sw == MRPG_STARTUP)
	strcat (buf, "startup");
    else
	strcat (buf, "restart");
    LE_send_msg (LE_VL1, buf);

    Verify_permanent_files ();

    CS_cfg_name ("");

    Lb_created = 0;
    n_pat = MRD_get_PAT (&pat);
    for (i = 0; i < n_pat; i++) {
	if (pat[i].data_id < 0 || pat[i].no_create)
	    continue;
	n_lbs = Get_all_lb_names (pat + i, &names);
	if (n_lbs < 0)
	    return (-1);
	namep = names;
	for (k = 0; k < n_lbs; k++) {
	    if (Recreate_all_lbs && sw == MRPG_STARTUP)
		ret = Create_lb (namep, pat + i);
	    else if (sw == MRPG_RESTART || pat[i].persistent)
		ret = Check_and_create_lb (namep, pat + i);
	    else if (Recreate_lbs)
		ret = Create_lb (namep, pat + i);
	    else
		ret = Check_clear_and_create_lb (namep, pat + i);
	    if (ret != 0)
		return (-1);
	    namep += strlen (namep) + 1;
	}
    }
    n_dat = MRD_get_DAT (&dat);
    for (i = 0; i < n_dat; i++) {
	if (dat[i].data_id < 0 || dat[i].no_create)
	    continue;
	n_lbs = Get_all_lb_names (dat + i, &names);
	if (n_lbs < 0)
	    return (-1);
	namep = names;
	for (k = 0; k < n_lbs; k++) {
	    if (Recreate_all_lbs && sw == MRPG_STARTUP)
		ret = Create_lb (namep, dat + i);
	    else if (sw == MRPG_RESTART || dat[i].persistent)
		ret = Check_and_create_lb (namep, dat + i);
	    else if (Recreate_lbs)
		ret = Create_lb (namep, dat + i);
	    else
		ret = Check_clear_and_create_lb (namep, dat + i);
	    if (ret != 0)
		return (-1);
	    namep += strlen (namep) + 1;
	}
    }
    MCD_create_dir (NULL);		/* free resource */
			
    return (0);
}

/******************************************************************

    Finds the LB names for data store "da". The names are returned
    with "names" in the format of a sequence of null terminated
    strings. The function returns the number of LBs on success or
    -1 on failure.
	
******************************************************************/

static int Get_all_lb_names (Mrpg_dat_entry_t *da, char **names) {
    static char *buffer = NULL;
    char name[MRPG_NAME_SIZE], *p, c, *path;
    int cnt, n_colons;

    if (buffer == NULL) {
	buffer = malloc (MHR_all_hosts (NULL) * (MRPG_NAME_SIZE * 2 + 8));
	if (buffer == NULL) {
	    LE_send_msg (GL_ERROR, "malloc failed\n");
	    return (-1);
	}
    }
    if (Get_lb_name (da, name, MRPG_NAME_SIZE) != 0)
	return (-1);

    p = name;
    n_colons = 0;
    path = name;
    while ((c = *p) != '\0' && c != '/') {
	if (c == ':') {
	    path = p + 1;
	    n_colons++;
	}
	p++;
    }
    if (n_colons == 1 || !MHR_is_distributed ()) {
	strcpy (buffer, name);
	cnt = 1;
    }
    else {		/* this data store is on multi nodes */
	int n_nodes, i;
	Node_attribute_t **nodes;
	n_nodes = MHR_get_data_hosts (da->data_id, &nodes);
	p = buffer;
	for (i = 0; i < n_nodes; i++) {
	    sprintf (p, "%s:%s", nodes[i]->hname, path);
	    p += strlen (p) + 1;
	}
	cnt = n_nodes;
    }
    *names = buffer;
    return (cnt);
}

/******************************************************************

    Checks if a data store is OK and creates it if it is not.

    Input: 	da - the RPG data store.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Check_and_create_lb (char *name, Mrpg_dat_entry_t *da) {
    int fd;

    fd = LB_open (name, LB_READ, &(da->attr));
    if (fd >= 0) {		/* LB exists and all attributes are fine */
	LB_close (fd);
	return (0);
    }
    return (Create_lb (name, da));
}

/******************************************************************

    Checks if a data store is OK and creates it if it is not. All
    messages are removed if it is OK.

    Input: 	da - the RPG data store.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Check_clear_and_create_lb (char *name, Mrpg_dat_entry_t *da) {
    int fd;

    fd = LB_open (name, LB_WRITE, &(da->attr));
    if (fd >= 0) {		/* LB exists and all attributes are fine */
	LB_clear (fd, LB_ALL);
	LB_close (fd);
	return (0);
    }
    if (fd == LB_TOO_MANY_WRITERS) {
	LE_send_msg (LE_VL3, 
              "LB %s is single writer type and used - not cleared\n", name);
	LB_close (fd);
	return (0);
    }
    return (Create_lb (name, da));
}

/******************************************************************

    Creates an RPG data store.

    Input: 	da - the RPG data store.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Create_lb (char *name, Mrpg_dat_entry_t *da) {
    int fd;

    if (Mrpg_cmd == MRPG_INIT && !(da->mrpg_init))
	return (0);

    if (da->data_id == ORPGDAT_MRPG_CMDS)
	MPC_mrpg_cmds_lb_removed ();	/* close mrpg cmd LB */

    if (MCD_create_dir (name) != 0)
	return (-1);
    LE_send_msg (LE_VL3, "    Create LB %s\n", name);
    Lb_created = 1;
    fd = LB_open (name, LB_CREATE, &(da->attr));
    if (fd < 0) {
	LE_send_msg (GL_ERROR, "Creating %s failed (ret %d)\n", name, fd);
	return (-1);
    }
    LB_close (fd);

    if (da->data_id == ORPGDAT_MRPG_CMDS)
	MPC_open_mrpg_cmds_lb ();	/* reopen mrpg cmd LB */

    return (0);
}

/******************************************************************

    Creates the directory for RPG data store "da". If da is NULL,
    free all resources. If host name is empty, creates the dir on 
    all nodes.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

#define MAX_N_DIRS 128
#define CMD_BUF_SIZE 200

int MCD_create_dir (char *name) {
    static char *done_path[MAX_N_DIRS];
    static int n_done_paths = 0;
    char cmd[CMD_BUF_SIZE], *dir, *p, hname[MRPG_NAME_SIZE];
    int i, ret, all_nodes;
    char *mod_pt, mod_c;

    if (name == NULL) {
	for (i = 0; i < n_done_paths; i++)
	    free (done_path[i]);
	n_done_paths = 0;
/* printf ("free resource\n"); */
	return (0);
    }

    if (strlen (name) + 32 > CMD_BUF_SIZE) {
 	LE_send_msg (GL_ERROR, "Data store path (%s) too long\n", name);
	return (-1);
    }
    p = name + strlen (name) - 1;
    mod_pt = NULL;
    mod_c = ' ';		/* any value - to avoid compiler warning */
    while (p >= name) {		/* remove the file name part */
	if (*p == '/') {
	    mod_pt = p;
	    mod_c = *p;
	    *p = '\0';
	    break;
	}
	p--;
    }
    if (p < name) {		/* no '/' found in the path */
 	LE_send_msg (GL_ERROR, "No dir found in data store name\n", name);
	return (-1);
    }

    for (i = 0; i < n_done_paths; i++) {
	if (strcmp (done_path[i], name) == 0)
	    break;
    }
    if (i < n_done_paths) {	/* already created */
	if (mod_pt != NULL)	/* restore the string */
	    *mod_pt = mod_c;
	return (0);
    }
    if (n_done_paths >= MAX_N_DIRS)
 	LE_send_msg (GL_ERROR, "Warning - Too many data store dirs\n");
    else {			/* save the path for later check */
	if ((done_path[n_done_paths] = malloc (strlen (name) + 1)) == NULL)
 	    LE_send_msg (GL_ERROR, "Warning - malloc failed\n");
	else {
	    strcpy (done_path[n_done_paths], name);
	    n_done_paths++;
	}
    }

    dir = name;
    while (*dir != '\0') {	/* find the host name part */
	if (*dir == ':')
	    break;
	dir++;
    }
    if (*dir == ':') {		/* host name found */
	all_nodes = 0;
	strncpy (hname, name, dir - name);
	hname[dir - name] = '\0';
	dir++;
    }
    else {			/* no host name (all nodes) */
	all_nodes = 1;
	dir = name;
    }
    sprintf (cmd, "sh -c (mkdir -p %s)", dir);
    {		/* change env format of $(...) to $... */
	char *in, *out;
	int in_env = 0;
	in = out = cmd;
	while (1) {
	    if (!in_env && *in == '(' && in > cmd && in[-1] == '$') {
		in += 1;
		in_env = 1;
		continue;
	    }
	    if (in_env && *in == ')') {
		in += 1;
		in_env = 0;
		continue;
	    }
	    *out = *in;
	    if (*in == '\0')
		break;
	    out++;
	    in++;
	}
    }

    ret = 0;
    if (all_nodes) {		/* create the dir on remote nodes */
	int n_nodes;
	Node_attribute_t *nodes;

	n_nodes = MHR_all_hosts (&nodes);
	for (i = 0; i < n_nodes; i++) {
	    char *out;
	    if ((ret = MGC_system (nodes[i].hname, cmd, &out)) != 0) {
		LE_send_msg (GL_ERROR, "mkdir %s on %s failed (ret %d)\n", 
					dir, nodes[i].node, ret);
		LE_send_msg (GL_ERROR, "    - %s\n", out);
		ret = -1;
		break;
	    }
	}
    }
    else if ((ret = MGC_system (hname, cmd, NULL)) != 0) {
					/* create dir on one nodes */
	if (hname[0] != '\0')
	    LE_send_msg (GL_ERROR, "mkdir %s failed (ret %d) on %s\n", 
						dir, ret, hname);
	else
	    LE_send_msg (GL_ERROR, "mkdir %s failed (ret %d)\n", dir, ret);
	ret = -1;
    }

    if (mod_pt != NULL)		/* restore the string */
	*mod_pt = mod_c;
    return (ret);
}

/******************************************************************

    Gets the full path of a data store.

    Inputs:	da - the RPG data store.
		buf_size - size of "buf".

    Output:	buf - buffer for returning the full path.

    Returns 0 on success or -1 on failure.

******************************************************************/

static int Get_lb_name (Mrpg_dat_entry_t *da, char *buf, int buf_size) {
    CS_parse_control (CS_NO_ENV_EXP);
    if (CS_entry ((char *)da->data_id, 1 | CS_INT_KEY, buf_size, buf) <= 0) {
	LE_send_msg (GL_ERROR, "Data store %d not found in system config", 
					da->data_id);
	CS_parse_control (CS_YES_ENV_EXP);
	return (-1);
    }
    CS_parse_control (CS_YES_ENV_EXP);
    return (0);
}

/***********************************************************************

    Checks listed permanent files for corruption. If a file is determined
    to be corrupted, the file is removed. We assume, for the moment, the
    permanent files must be on the mrpg node.

***********************************************************************/

static void Verify_permanent_files () {
    int n_p_files, i;
    char *pf, *pf_names;

    n_p_files = MRT_get_perm_file_names (&pf_names);
    pf = pf_names;
    for (i = 0; i < n_p_files; i++) {
	if (strstr (pf, "mrpg.log") == NULL)	/* we cannot remove mrpg.log */
	    MCD_verify_permanent_file (pf);
	pf += strlen (pf) + 1;
    }
}

/***********************************************************************

    Checks file "fname" for corruption. The file is assumed to be in
    $ORPGDIR. If the file exists and is determined to be corrupted, the 
    file is removed.

***********************************************************************/

void MCD_verify_permanent_file (char *fname) {
    static char *name = NULL, *dir = NULL;
    int bad, fd, ret;
    char *buf;

    if (dir == NULL)
	dir = getenv ("ORPGDIR");
    if (dir == NULL) {
	LE_send_msg (GL_ERROR, 
	"Environ variable ORPGDIR not defined (Verify_permanent_files)");
	return;
    }

    name = STR_copy (name, dir);
    if (dir[strlen (dir) - 1] != '/')
	name = STR_cat (name, "/");
    name = STR_cat (name, fname);
    bad = 0;
    LE_send_msg (LE_VL2, "Checking permanent file %s", name);

    /* Verify header and reset the free space table */
    if ((!bad && (ret = LB_fix_byte_order (name)) < 0) ||
	(!bad && (ret = LB_fix_byte_order (name)) < 0)) {
	if (ret != LB_OPEN_FAILED)
	    LE_send_msg (GL_ERROR, 
		    "LB_fix_byte_order failed - %s removed", name);
	bad = 1;
    }

    /* test go through all messages */
    fd = -1;
    if (!bad && (fd = LB_open (name, LB_READ, NULL)) < 0) {
	if (fd != LB_OPEN_FAILED)
	    LE_send_msg (GL_ERROR, 
		    "LB_open %s failed (%d) - removed", name, fd);
	bad = 1;
    }
    if (!bad && (ret = LB_seek (fd, 0, LB_FIRST, NULL)) < 0) {
	LE_send_msg (GL_ERROR, 
		    "LB_seek %s failed (%d) - removed", name, ret);
	bad = 1;
    }
    while (!bad) {
	ret = LB_read (fd, &buf, LB_ALLOC_BUF, LB_NEXT);
	if (ret > 0)
	    free (buf);
	if (ret == LB_TO_COME)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
		    "LB_read %s failed (%d) - removed", name, ret);
	    bad = 1;
	}
    }
    if (fd >= 0)
	LB_close (fd);

    if (bad && (ret = LB_remove (name)) < 0 && ret != LB_NOT_EXIST)
	LE_send_msg (GL_ERROR, "LB_remove %s failed", name);

}
