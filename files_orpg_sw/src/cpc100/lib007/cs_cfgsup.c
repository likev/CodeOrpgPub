/****************************************************************
		
    Module: cs_cfgsup.c	
		
    Description: This file contains the basic functions of the 
		configuration support (CS) module. It implements
		file switching and other control functions.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/07/14 21:05:33 $
 * $Id: cs_cfgsup.c,v 1.48 2009/07/14 21:05:33 jing Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */  


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <infr.h>
#include "cs_def.h"

struct Cs_list_struct {			/* config file list item */
    cs_cfg_t cfg;			/* config struct */
    struct Cs_list_struct *next;	/* next file */
};
typedef struct Cs_list_struct Cs_cfg_list_t;

static Cs_cfg_list_t *Cfgs = NULL;	/* list of config files */

static char Default_cfg_name[CS_NAME_SIZE] = "sys_cfg";
					/* default config file name */

typedef struct {			/* for registering update event */
    EN_id_t evt;			/* event id */
    int *status;			/* caller's status variable */
    cs_cfg_t *cfg;			/* the cfg associated with. NULL 
					   indicates closed cfg file. */
} Cs_event_t;

static Cs_event_t *Evts = NULL;		/* all registered events */
static int N_evts = 0;			/* number of registered events */

#define CS_MAX_N_EVTS 16		/* MAX number of registered events */


/* local functions */
static void Get_full_cfg_name (const char *name, char *buf, int buf_size);
static int Read_from_file (char *name, char **buf);
static int Init_cfg_struct (char *name, cs_cfg_t **cfgp);
static int Init_default_cfg ();
static void Close_source ();
static void Ev_callback (EN_id_t evtcd, char *msg, int msglen, void *arg);


/********************************************************************

    Sets the system configuration file name.

********************************************************************/

void CS_set_sys_cfg (char *sys_cfg_name) {

    if (sys_cfg_name == NULL)
	strcpy (Default_cfg_name, "sys_cfg");
    else {
	strncpy (Default_cfg_name, sys_cfg_name, CS_NAME_SIZE);
	Default_cfg_name[CS_NAME_SIZE - 1] = '\0';
    }
}

/********************************************************************

    Sets the single level switch. To be phased out.

********************************************************************/

void CS_set_single_level (int yes) {

    if (yes)
	CS_control (CS_SINGLE_LEVEL);
}

/********************************************************************
			
    Description: This is the SC control function.

    Input:	action - control action flags.

********************************************************************/

int CS_control (int action) {
    int ret;
    cs_cfg_t *cfg;

    cfg = CS_get_current_cfg ();
    if (cfg == NULL) {
	Init_default_cfg ();
	cfg = CS_get_current_cfg ();
    }

    if (action & CS_COMMENT) {
	cfg->comment_char = action & 0xff;
	return (0);
    }

    ret = CS_parse_control (action);
    if (ret < 0)
       return (ret);

    if (action & CS_SINGLE_LEVEL)
	cfg->single_level = 1;

    if (action & CS_KEY_OPTIONAL)
	cfg->key_optional = 1;

    if (action & CS_KEY_REQUIRED)
	cfg->key_optional = 0;

    if (action & CS_UPDATE)
	cfg->need_cfg_read = 1;

    if (action & CS_CLOSE)
	Close_source ();

    if (action & CS_RESET) {
	cfg->line = -1;
	cfg->token = -1;
	cfg->level = 0;
	cfg->index = -1;
	cfg->start = 0;
    }

    return (0);
}

/********************************************************************
			
    Sets the current config file to "name". Returns the current full 
    config source name.

********************************************************************/

char *CS_cfg_name (const char *name) {
    cs_cfg_t *cfg;

    if (name == NULL) {
	cfg = CS_get_current_cfg ();
	if (cfg == NULL) {
	    Init_default_cfg ();
	    cfg = CS_get_current_cfg ();
	}
    }
    else {
	char full_name[CS_NAME_SIZE * 2];

	if (name[0] == '\0')
	    Get_full_cfg_name (Default_cfg_name, full_name, CS_NAME_SIZE * 2);
	else if (name[0] == '/' || name[0] == '.') {
	    strncpy (full_name, name, CS_NAME_SIZE * 2);
	    full_name[CS_NAME_SIZE * 2 - 1] = '\0';
	}
	else
	    Get_full_cfg_name (name, full_name, CS_NAME_SIZE * 2);
    
	Init_cfg_struct (full_name, &cfg);
	CS_set_current_cfg (cfg);
    }
    if (cfg == NULL)
	return (NULL);
    return (cfg->cfg_name);
}

/********************************************************************
			
    Updates the configuration info for config file "cfg". Returns 0 
    on success or a negative CS error number.

********************************************************************/

int CS_update_cfg (cs_cfg_t *cfg) {
    int err;

    if (cfg == NULL) {
	Init_default_cfg ();
	cfg = CS_get_current_cfg ();
    }
    if (!cfg->need_cfg_read)
	return (0);

    /* read the config text */
    if (cfg->cs_buf != NULL) {
	free (cfg->cs_buf);
	cfg->cs_buf = NULL;
    }
    err = Read_from_file (cfg->cfg_name, &cfg->cs_buf);
    if (err < 0) {
	CS_print_err (sprintf (CS_err_buf (), 
			"CS: open %s failed\n", cfg->cfg_name));
	return (err);
    }
    
    /* generate the key table */
    err = CS_gen_table (cfg);
    if (err < 0)
	return (err);
    cfg->need_cfg_read = 0;
    return (0);
}

/********************************************************************
			
    Initializes the default cfg file. Returns 0 on success or a
    negative error code.

********************************************************************/

static int Init_default_cfg () {
    char name[CS_NAME_SIZE * 2];
    cs_cfg_t *cfg;

    Get_full_cfg_name (Default_cfg_name, name, CS_NAME_SIZE * 2);
    Init_cfg_struct (name, &cfg);
    CS_set_current_cfg (cfg);
    return (0);
}

/********************************************************************
			
    Creates and initializes the config struct for file "name" and 
    returns it with "cfgp". Returns 0 on success or a negative error
    code.

********************************************************************/

static int Init_cfg_struct (char *name, cs_cfg_t **cfgp) {
    Cs_cfg_list_t *new_ent, *list;
    cs_cfg_t *cfg;

    list = Cfgs;
    while (list != NULL) {
	if (strcmp (list->cfg.cfg_name, name) == 0)
	    break;
	list = list->next;
    }
    if (list != NULL) {
	*cfgp = &(list->cfg);
	return (0);
    }

    new_ent = (Cs_cfg_list_t *)MISC_malloc (
			sizeof (Cs_cfg_list_t) + strlen (name) + 1);
    cfg = &(new_ent->cfg);
    cfg->cfg_name = (char *)new_ent + sizeof (Cs_cfg_list_t);
    strcpy (cfg->cfg_name, name);
    cfg->cs_buf = NULL;
    cfg->cfg_list = NULL;
    cfg->n_cfgs = 0;
    cfg->line = -1;
    cfg->token = -1;
    cfg->level = 0;
    cfg->index = -1;
    cfg->start = 0;
    cfg->key_optional = 0;
    cfg->single_level = 0;
    cfg->comment_char = '#';
    cfg->need_cfg_read = 1;
    new_ent->next = NULL;
    if (Cfgs == NULL)
	Cfgs = new_ent;
    else {
	Cs_cfg_list_t *list;
	list = Cfgs;
	while (list->next != NULL)
	    list = list->next;
	list->next = new_ent;
    }
    *cfgp = &(new_ent->cfg);
    return (0);
}

/********************************************************************
			
    Constructs and returns, with "buf" of size "buf_size", the full 
    path of config file "name".

********************************************************************/

static void Get_full_cfg_name (const char *name, char *buf, int buf_size) {
    int ret;

    ret = MISC_get_cfg_dir (buf, buf_size);
    if (ret < 0)
	strcpy (buf, ".");
    if (strlen (buf) + strlen (name) + 2 >= (unsigned int)buf_size)
	return;
    strcat (buf, "/");
    strcat (buf, name);
    return;
}

/********************************************************************
			
    Reads the entire file named "name" into a buffer and returns the 
    buffer with "buf". Returns the length of the config text or a 
    negative error code.

********************************************************************/

static int Read_from_file (char *name, char **buf) {
    char *cs_buf = NULL;	/* buffer holding the config text */
    int size, fd;

    /* open the file */
    fd = MISC_open (name, O_RDONLY, 0);
    if (fd < 0) {
	CS_print_err (sprintf (CS_err_buf (), "CS: open %s failed\n", name));
	return (CS_OPEN_ERR);
    }

    /* find message size */
    size = lseek (fd, 0, SEEK_END);
    if (size < 0) {
	CS_print_err (sprintf (CS_err_buf (), "CS: lseek %s failed\n", name));
	close (fd);
	return (CS_SEEK_ERR);
    }
    lseek (fd, 0, SEEK_SET);

    /* malloc buffer */
    cs_buf = (char *)malloc (size + 1);
    if (cs_buf == NULL) {
	CS_print_err (sprintf (CS_err_buf (), "CS: malloc failed\n"));
	close (fd);
	return (CS_MALLOC_ERR);
    }

    /* read the file */
    if (MISC_read (fd, cs_buf, size) != size) {
	CS_print_err (sprintf (CS_err_buf (), "CS: read %s failed\n", name));
	free (cs_buf);
	close (fd);
	return (CS_READ_ERR);
    }
    cs_buf[size] = '\0';
    close (fd);

    *buf = cs_buf;
    return (size);
}

/********************************************************************
			
    Closes the current cfg file and sets the current cfg file to the
    default or the first if default is not used.

********************************************************************/

static void Close_source () {
    cs_cfg_t *cfg;
    Cs_cfg_list_t *list, *prev;
    char full_name[CS_NAME_SIZE * 2];
    int i;

    cfg = CS_get_current_cfg ();
    if (cfg == NULL)
	return;

    for (i = 0; i < N_evts; i++) {	/* remove from Evts */
	if (Evts[i].cfg == cfg)
	    Evts[i].cfg = NULL;
    }

    if (cfg->cs_buf != NULL)
	free (cfg->cs_buf);
    if (cfg->cfg_list != NULL)
	free (cfg->cfg_list);

    list = Cfgs;
    prev = NULL;
    while (list != NULL) {
	if (&(list->cfg) == cfg) {
	    if (prev == NULL)
		Cfgs = list->next;
	    else
		prev->next = list->next;
	    free (list);
	    break;
	}
	prev = list;
	list = list->next;
    }

    /* set the new default */
    Get_full_cfg_name (Default_cfg_name, full_name, CS_NAME_SIZE * 2);
    list = Cfgs;
    while (list != NULL) {
	if (strcmp (list->cfg.cfg_name, full_name) == 0)
	    break;
	list = list->next;
    }
    if (list == NULL) {			/* no default found */
	if (Cfgs == NULL)
	    CS_set_current_cfg (NULL);
	else
	    CS_set_current_cfg (&(Cfgs->cfg));	/* use the first */
    }
    else
	CS_set_current_cfg (&(list->cfg));	/* use the default */
}

/********************************************************************
			
    Registers a configuration change event "cfg_ev" and, by option, a 
    user status variable "status". Returns 0 on success or a negative 
    error code. Identical registration is ignored.

********************************************************************/

int CS_event (int cfg_ev, int *status) {
    int i, free_ind;
    cs_cfg_t *cfg;

    if (Evts == NULL) {
	Evts = (Cs_event_t *)malloc (CS_MAX_N_EVTS * sizeof (Cs_event_t));
	if (Evts == NULL)
	    return (CS_MALLOC_ERR);
    }

    for (i = 0; i < N_evts; i++) {
	if (Evts[i].evt == (EN_id_t)cfg_ev && Evts[i].status == status)
	    return (0);
    }
    for (free_ind = 0; free_ind < N_evts; free_ind++) {
	if (Evts[free_ind].cfg == NULL)
	    break;
    }
    if (free_ind >= N_evts && N_evts >= CS_MAX_N_EVTS)
	return (CS_TOO_MANY_EVENTS);

    cfg = CS_get_current_cfg ();
    if (cfg == NULL) {
	Init_default_cfg ();
	cfg = CS_get_current_cfg ();
    }

    for (i = 0; i < N_evts; i++) {
	if (Evts[i].evt == (EN_id_t)cfg_ev)
	    break;
    }
    if (i >= N_evts) {		/* new event number */
	int ret;

	if ((ret = EN_register (cfg_ev, Ev_callback)) < 0) {
	    CS_print_err (sprintf (CS_err_buf (), 
			"CS: EN_register failed (ret %d)\n", ret));
	    return (CS_EV_REGISTER_ERR);
	}
    }
    Evts[free_ind].evt = cfg_ev;
    Evts[free_ind].status = status;
    Evts[free_ind].cfg = cfg;
    if (free_ind >= N_evts)
	N_evts = free_ind + 1;

    return (0);
}

/********************************************************************
			
    The callback function of the configuration update events. 

********************************************************************/

static void Ev_callback (EN_id_t evtcd, char *msg, int msglen, void *arg) {
    int i;

    /* match the event and set configuration info update flag */
    for (i = 0; i < N_evts; i++) {
	if (Evts[i].evt == evtcd) {
	    if (Evts[i].cfg == NULL)
		continue;
	    Evts[i].cfg->need_cfg_read = 1;
	    if (Evts[i].status != NULL)
		*(Evts[i].status) = CS_UPDATED;
	}
    }
    return;
}

