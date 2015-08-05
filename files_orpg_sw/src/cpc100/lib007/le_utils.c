
/**************************************************************************

    Various Log Error (LE) utility routines.

 **************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/04/15 15:31:52 $
 * $Id: le_utils.c,v 1.22 2009/04/15 15:31:52 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */  

/*
 * System Include Files/Local Include Files
 */
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <infr.h>
#include <le_def.h>

static int Instance = -1;
static char *Label = NULL;
static char *Le_name = NULL;
static int Lb_type = UNDEFINED_LB_TYPE;
static int Lb_size = UNDEFINED_LB_SIZE;
static int Use_plain_file = 0;
static int Msg_cnt = 0;			/* message count in plain log file */
static FILE *Plain_fh = NULL;		/* plain log file handler */
static int Plain_fd = -1;		/* plain log file descriptor */

/* Static Function Prototypes */
static int Rm_trailing_slash (char *str) ;


/********************************************************************
			
    Sets LE options. Returns 0 on success or a negative error code.

********************************************************************/

int LE_set_option (const char *option_name, ...) {
    va_list args;
    int ret;

    va_start (args, option_name);

    ret = 0;
    if (strcmp (option_name, "instance") == 0) {
	Instance = va_arg (args, int);
    }
    else if (strcmp (option_name, "label") == 0) {
	if (Label == NULL) {
	    char *p = va_arg (args, char *);
	    Label = STR_copy (Label, p);
	}
	else
	    ret = LE_OPT_ALREADY_DEFINED;
    }
    else if (strcmp (option_name, "LE name") == 0) {
	if (Le_name == NULL) {
	    char *p = va_arg (args, char *);
	    Le_name = STR_copy (Le_name, p);
	}
	else
	    ret = LE_OPT_ALREADY_DEFINED;
    }
    else if (strcmp (option_name, "LB type") == 0) {
	if (Lb_type == UNDEFINED_LB_TYPE)
	    Lb_type = va_arg (args, int);
	else
	    ret = LE_OPT_ALREADY_DEFINED;
    }
    else if (strcmp (option_name, "LB size") == 0) {
	if (Lb_size == UNDEFINED_LB_SIZE)
	    Lb_size = va_arg (args, int);
	else
	    ret = LE_OPT_ALREADY_DEFINED;
    }
    else if (strcmp (option_name, "verbose level") == 0) {
	int level = va_arg (args, int);
	ret = LE_local_vl (level);
    }
    else if (strcmp (option_name, "LE disable") == 0) {
	int yes = va_arg (args, int);
	ret = LE_set_disable (yes);
    }
    else if (strcmp (option_name, "set foreground") == 0) {
	LE_set_foreground ();
    }
    else if (strcmp (option_name, "also stderr") == 0) {
	int i = va_arg (args, int);
	LE_also_print_stderr (i);
    }
    else if (strcmp (option_name, "output fd") == 0) {
	void *fl = va_arg (args, void *);
	LE_set_output_fd (fl);
    }
    else if (strcmp (option_name, "no source info") == 0) {
	int mask = va_arg (args, unsigned int);
	LE_set_print_src_mask (mask);
    }
    else if (strcmp (option_name, "use plain log file") == 0) {
	Use_plain_file = 1;
    }
    else
	ret = LE_BAD_ARGUMENT;

    va_end (args);
    return (ret);
}

/********************************************************************

    Sets several LE attributes. Returns 0. To be phased out.

********************************************************************/

int LE_create_lb (char *argv0, int n_msgs, int lb_type, int instance) {

    if (Le_name == NULL)
	Le_name = STR_copy (Le_name, MISC_string_basename (argv0));
    if (Lb_size == UNDEFINED_LB_SIZE && 
			n_msgs > 0 && n_msgs != DEFAULT_LB_SIZE)
	Lb_size = n_msgs;
    if (Lb_type == UNDEFINED_LB_TYPE && 
			lb_type != 0 && lb_type != DEFAULT_LB_TYPE)
	Lb_type = lb_type;
    Instance = instance;
    return (0);
}

/********************************************************************

    Checks the existance of the LE LB and verifies its attributes. 
    If any attribute is different from user specified (not default),
    The LB is removed. Then the LB is created according to the user
    specification or the default. Returns LB fd on success or a negative
    error code.

********************************************************************/

int LE_check_and_open_lb (char *arg0, int *use_plain_filep) {
    char lelb_path[LE_NAME_LEN];
    int lb_size, lb_type, ret, fd, need_verify;
    LB_attr attr;

    *use_plain_filep = Use_plain_file;
    if (Le_name == NULL && arg0 != NULL)
	Le_name = STR_copy (Le_name, MISC_string_basename (arg0));
    if (Le_name == NULL)
	return (LE_LB_NAME_UNDEFINED);

    ret = LE_filepath (Le_name, Instance, lelb_path, LE_NAME_LEN);
    if (ret < 0)
	return (ret);

    if (Use_plain_file) {
	char buf[256], *p;
	if ((Plain_fh = MISC_fopen (lelb_path, "r+")) == NULL &&
	    (Plain_fh = MISC_fopen (lelb_path, "w+")) == NULL)
	    return (LE_LB_OPEN_CREATE_FAILED);
	LE_file_lock (1);
	fseek (Plain_fh, 0, 0);
	if ((p = fgets (buf, 256, Plain_fh)) == NULL ||
	    (p != NULL && strcmp (buf, LE_LOG_LABEL) != 0)) {
	    fseek (Plain_fh, 0, 0);	/* init label */
	    fprintf (Plain_fh, "%s\n", LE_LOG_LABEL);
	    fflush (Plain_fh);
	    fseek (Plain_fh, -1, SEEK_CUR);
	}
	Msg_cnt = 0;
	while ((p = fgets (buf, 256, Plain_fh)) != NULL) {
	    Msg_cnt++;
	    if (buf[0] == '\n')
		break;
	    if (strcmp (buf, LE_LOG_LABEL) == 0) {
		Msg_cnt = 0;	/* corrupted file or non-formated file */
		break;
	    }
	}
	if (p == NULL)		/* neither line "\n" nor lable line found */
	    Msg_cnt = 0;
	if (Msg_cnt == 0) {
	    fseek (Plain_fh, 0, 0);
	    fgets (buf, 256, Plain_fh);	/* by pass the label line */
	    fprintf (Plain_fh, "\n");
	    fflush (Plain_fh);
	}
	LE_file_lock (0);
	Plain_fd = fileno (Plain_fh);
	return (Plain_fd);
    }

    fd = LB_open (lelb_path, LB_WRITE, NULL);
    if (fd == LB_TOO_MANY_WRITERS)
	return (LE_DUPLI_INSTANCE);
    if (fd < 0 && fd != LB_OPEN_FAILED)
	return (fd);

    need_verify = 0;
    if (Lb_type != UNDEFINED_LB_TYPE || Lb_size != UNDEFINED_LB_SIZE)
	need_verify = 1;
    lb_size = Lb_size;
    if (lb_size == UNDEFINED_LB_SIZE)
	lb_size = DEFAULT_LB_SIZE;
    lb_type = Lb_type;
    if (lb_type == UNDEFINED_LB_TYPE)
	lb_type = DEFAULT_LB_TYPE;
    if (fd >= 0) {
	LB_status status;

	if (!need_verify)
	    return (fd);
	status.attr = &attr;		/* verify the LB */
	status.n_check = 0;
	if (LB_stat (fd, &status) == 0 &&
	    (attr.types & lb_type) == lb_type && attr.maxn_msgs == lb_size)
	    return (fd);
	LB_close (fd);
	LB_remove (lelb_path);
    }

    /* create the LB */
    {				/* try to make the LE dir */
	char *p = lelb_path + strlen (lelb_path) - 1;
	while (p > lelb_path && *p != '/')
	    p--;
	if (p > lelb_path) {
	    *p = '\0';
	    MISC_mkdir (lelb_path);
	    *p = '/';
	}
    }
    attr.maxn_msgs = lb_size;
    attr.msg_size = 0;
    attr.types = lb_type;
    attr.remark[0] = '\0';
    attr.mode = 0664;
    attr.tag_size = 0;
    fd = LB_open (lelb_path, LB_CREATE, &attr);
    if (fd < 0)
	return (LE_LB_OPEN_CREATE_FAILED);
    return (fd);
}

/********************************************************************
			
    Returns the plain file handler. Repositions the file pointer.
    For lem to work, we must update the old \n after all other updates.

********************************************************************/

FILE *LE_get_plain_file_fh () {

    if (Plain_fh != NULL) {
	char c;
	int maxn_msgs = Lb_size;
	if (maxn_msgs == UNDEFINED_LB_SIZE)
	    maxn_msgs = DEFAULT_LB_SIZE;

	LE_file_lock (1);

	/* move the write point if anybody else has written to the file */
	fflush (Plain_fh);
	if (lseek (Plain_fd, -1, SEEK_CUR) == (off_t)-1 ||
	    read (Plain_fd, &c, 1) != 1 || c != '\n') {
	    int cnt = 0;
	    while (1) {		/* move to write point - next empty line */
		char *p, buf[256];
		p = fgets (buf, 256, Plain_fh);
		Msg_cnt++;
		if (p == NULL) {
		    fseek (Plain_fh, strlen (LE_LOG_LABEL), SEEK_SET);
		    cnt++;
		    if (cnt >= 2)	/* the write point not found */
			break;
		    Msg_cnt = 0;
		    continue;
		}
		if (buf[0] == '\n') {	/* the write point found */
		    Msg_cnt--;
		    break;
		}
	    }
	    fflush (Plain_fh);
	}

	if (Msg_cnt > maxn_msgs) {
	    int opos, npos;
	    opos = ftell (Plain_fh);
	    fseek (Plain_fh, 0, 0);
	    fprintf (Plain_fh, "%s\n", LE_LOG_LABEL);
	    fflush (Plain_fh);
	    npos = ftell (Plain_fh);
	    while (ftruncate (Plain_fd, opos - 1) < 0 && errno == EINTR);
	    fseek (Plain_fh, npos, SEEK_SET);
	    Msg_cnt = 0;
	}
	Msg_cnt++;
    }
    return (Plain_fh);
}

/********************************************************************
			
    Locks/unlocks the plain log file for exclusive access.

********************************************************************/

void LE_file_lock (int lock) {
    int err;
    struct flock fl;		/* structure used by fcntl */

    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 1;
    if (lock)
	fl.l_type = F_WRLCK;
    else
	fl.l_type = F_UNLCK;
    while ((err = fcntl (Plain_fd, F_SETLKW, &fl)) == -1 && errno == EINTR);
}

/********************************************************************
			
    Returns Instance.

********************************************************************/

int LE_get_instance () {
    return (Instance);
}

/********************************************************************
			
    Returns Label.

********************************************************************/

char *LE_get_label () {
    return (Label);
}

/********************************************************************
			
    Accepts the optional instance number. To be phased out.

********************************************************************/

void LE_instance (int instance_number) {

    Instance = instance_number;
    return;
}

/********************************************************************

    Gets the Log Error (LE) directory path. The LE directory path is 
    read from the LE_DIR_EVENT environment variable and placed in the 
    caller-provided storage. Returns the length of the path on success
    or a negetive error code.

********************************************************************/

int LE_dirpath (char *buf, size_t buf_size) {
    char *env ;

    if ((env = getenv ("LE_DIR_EVENT")) != NULL) {
	char *colon_p ;
	int len;

        colon_p = strstr (env, ":") ;
        if (colon_p != NULL)
	    len = colon_p - env;
	else
	    len = strlen (env);
	if (len >= (int)buf_size)
	    return (LE_BUF_TOO_SMALL);
	memcpy (buf, env, len);
	buf[len] = '\0';
	len = Rm_trailing_slash (buf);
	return (len);
    }
    else
        return (LE_ENV_NOT_DEF);
}


/*************************************************************************

    Gets the LE filename for task "name" and "instance". The 
    caller provides "buf" of size "bufsz" for returning the file name.

    Returns 0 on success or -1 on failure.

*************************************************************************/

int LE_filename (const char *name, int instance, char *buf, size_t bufsz) {
    char instance_string[24] ;

    if (instance < 0)
	instance_string[0] = '\0';
    else
	sprintf (instance_string, ".%d", instance);

    if (strlen (name) + strlen (instance_string) + 1 + 
				strlen (LE_FILENAME_EXT) < bufsz) {
        sprintf (buf, "%s%s.%s", name, instance_string, LE_FILENAME_EXT);
	return (0);
    }
    else
	return (LE_BUF_TOO_SMALL);
}


/*************************************************************************

    Gets the LE file full path for task "name" and "instance". The 
    caller provides "buf" of size "bufsz" for returning the file name.
    Returns 0 on success or -1 on failure.

*************************************************************************/

int LE_filepath (const char *name, int instance, char *buf, size_t bufsz) {
    int retval ;

    retval = LE_dirpath (buf, bufsz);
    if (retval < 0)
        return (retval);

    if (strlen (buf) + strlen ("/") < bufsz)
        strcat (buf, "/");
    else
        return(LE_BUF_TOO_SMALL);

    retval = LE_filename (name, instance,
                         buf + strlen (buf), bufsz - strlen (buf));
    if (retval < 0)
        return (retval);
    return (0) ;
}

/************************************************************************

    Removes trailing '\' in string "str" and returns the length of the 
    new string.

************************************************************************/

static int Rm_trailing_slash (char *str) {
    char *pt;
    int len;

    len = strlen (str);
    pt = str + len - 1;
    while (pt > str && *pt == '/') {
        *pt = '\0';
        pt--;
        len--;
    }
    return (len);
}
