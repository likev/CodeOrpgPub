
/******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive. This is the module that 
    searches for volume files.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 15:11:52 $
 * $Id: pa_files.c,v 1.8 2014/10/03 15:11:52 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <infr.h> 
#include "pa_def.h" 

#define MESSAGE_SIZE 2432		/* NCDC message size */
#define FILE_HEADER_SIZE 24		/* NCDC file header size */
#define BUFFER_SIZE 500000		/* 100 uncompressed radials */

#define MAX_SEARCH_DEPTH 3	/* Maximum directory search depth */
static int Search_depth;	/* Current directory search depth */

static Ap_vol_file_t *Vol_files;	/* volume file list */
static int N_vol_files = 0;		/* size of array Vol_files */
static void *Vol_file_tblid = NULL;	/* Vol_files table ID */


/* local functions */
static void Search_volume_files (char *d_name);
static void Process_a_file_name (char *dir, char *name, int file_size);
static void Sort_file_names (int n, Ap_vol_file_t *vf);
static int Is_less_than (Ap_vol_file_t *left, Ap_vol_file_t *right);
static void Complete_dir_path (char *dir_name);
static void Add_slash (char *path);
static void Add_pwd_path (char *path);
static void Add_default_path (char *path);
static int Find_compression_type (char *name);
static void Add_a_new_vol_file (char *prefix, char *dir, char *name, 
		int compress_type, int content_type, time_t ftime, 
		int file_size, int selected);
static void Get_list_file_path (char *dir_name);
int PAF_get_listed_files (char *dir_name, 
			char *list_file_name, Ap_vol_file_t **Vol_files);


/******************************************************************

    Searches the volume file directory "dir_name" to get all 
    volume file names. The list of volume file found are returned
    with "vol_files". The return value is the number of volume files
    found.

******************************************************************/

int PAF_search_volume_files (char *d_name, Ap_vol_file_t **vol_files) {
    int i;

    for (i = 0; i < N_vol_files; i++)
	free (Vol_files[i].path);
    if (Vol_file_tblid != NULL)
	MISC_free_table (Vol_file_tblid);
    N_vol_files = 0;
    Vol_file_tblid = NULL;

    Complete_dir_path (d_name);
    Search_depth = 0;
    Search_volume_files (d_name);
    Sort_file_names (N_vol_files, Vol_files);
    *vol_files = Vol_files;
    return (N_vol_files);
}

/******************************************************************

    Searches the volume file directory "dir_name" to get all 
    volume file names.

******************************************************************/

static void Search_volume_files (char *d_name) {
    DIR *dir;
    struct dirent *dp;

    Search_depth++;
    if (Search_depth > MAX_SEARCH_DEPTH)
	return;

    dir = opendir (d_name);
    if (dir == NULL) {
	fprintf (stderr, "opendir (%s) failed, errno %d\n", d_name, errno);
	return;
    }

    while ((dp = readdir (dir)) != NULL) {
	int ret, file_size;
	struct stat st;

	if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0)
	    continue;

	ret = stat (PAF_get_full_path (d_name, dp->d_name), &st);
	if (ret < 0) 
	    continue;
	file_size = st.st_size;

	if (S_ISDIR (st.st_mode)) {	/* a directory */
	    char full_name[LOCAL_NAME_SIZE + 4];
	    strcpy (full_name, PAF_get_full_path (d_name, dp->d_name));
	    strcat (full_name, "/");
	    Search_volume_files (full_name);
	}
	else if (S_ISREG (st.st_mode))
	    Process_a_file_name (d_name, dp->d_name, file_size);
    }
    closedir (dir);
    Search_depth--;
    return;
}

/************************************************************************

    Checks if file "name" in directory "dir" is a volume file. Adds this 
    to the volume file list if it is.

************************************************************************/

static void Process_a_file_name (char *dir, char *fname, int file_size) {
    char *cpt, prefix[LOCAL_NAME_SIZE], buf[32], *name;
    int y, mon, d, h, m, s, i;
    int len, compress_type, content_type;
    time_t ftime;

    name = fname;		/* name for parsing */
    cpt = name;			/* find the first _ */
    while (*cpt != '\0' && *cpt != '_')
	cpt++;
    if (*cpt != '_') {		/* no _ in name */
	if (strlen (name) == 25 && strcmp (name + 21, ".raw") == 0)
	    cpt = name;		/* .raw format */
	else
	    return;
    }
    else if (cpt - name == 4 && MISC_char_cnt (cpt + 1, "\0_") == 8 && 
					strlen (name) == 18) {
	cpt = buf;
	memcpy (cpt, name, 4);
	cpt += 4;
	memcpy (cpt, name + 5, 8);
	cpt += 8;
	memcpy (cpt, name + 14, 4);
	cpt += 4;
	strcpy (cpt, "00V03.raw");
	cpt = buf;
	name = cpt;	/* converted name (.raw format) */
    }

    if (sscanf (cpt + 1, "%d%*c%d%*c%d%*c%d%*c%d%*c%d", 
					&y, &mon, &d, &h, &m, &s) == 6) {
	len = cpt - name;
	strncpy (prefix, name, len);
	prefix[len] = '\0';
	content_type = AP_LOCAL_TYPE;
    }
    else if (sscanf (cpt + 1, "%4d%2d%2d%*c%2d%2d%2d", 
					&y, &mon, &d, &h, &m, &s) == 6) {
	len = cpt - name;
	strncpy (prefix, name, len);
	prefix[len] = '\0';
	content_type = AP_LOCAL_TYPE;
    }
    else if (cpt - name >= 8 &&
	sscanf (cpt - 8, "%4d%2d%2d%*c%2d%2d%2d", 
					&y, &mon, &d, &h, &m, &s) == 6) {
	len = cpt - name - 8;
	strncpy (prefix, name, len);
	prefix[len] = '\0';
	content_type = AP_NCDC_TYPE;
    }
    else if (cpt == name && sscanf (cpt + 4, "%4d%2d%2d%2d%2d%2d", 
					&y, &mon, &d, &h, &m, &s) == 6) {
	len = 4;
	strncpy (prefix, name, len);
	prefix[len] = '\0';
	content_type = AP_LDM_TYPE;
    }
    else 
	return;

    ftime = 0;
    unix_time (&ftime, &y, &mon, &d, &h, &m, &s);

    for (i = 0; i < N_vol_files; i++) {
	if (Vol_files[i].time == ftime &&
		strcmp (Vol_files[i].prefix, prefix) == 0)
	return;			/* a duplicated file */
    }

    if ((compress_type = Find_compression_type (name)) < 0)
	return;

    Add_a_new_vol_file (prefix, dir, fname, compress_type, 
				content_type, ftime, file_size, 0);
}

/************************************************************************

    Adds a new entry to the volume file list.

************************************************************************/

static void Add_a_new_vol_file (char *prefix, char *dir, char *name, 
		int compress_type, int content_type, time_t ftime, 
		int file_size, int selected) {
    char *cpt;
    Ap_vol_file_t *new_vf;
    int new_ind;

    if (Vol_file_tblid == NULL) {
	Vol_file_tblid = MISC_open_table (sizeof (Ap_vol_file_t), 32, 
			    0, &N_vol_files, (char **)&Vol_files);
	if (Vol_file_tblid == NULL) {		/* malloc failed */
	    fprintf (stderr, "MISC_open_table failed\n");
	    exit (1);
	}
    }

    cpt = malloc (strlen (prefix) + 
				strlen (PAF_get_full_path (dir, name)) + 2);
    new_vf = (Ap_vol_file_t *)MISC_table_new_entry (Vol_file_tblid, &new_ind);
    if (new_vf == NULL || cpt == NULL) {		/* malloc failed */
	fprintf (stderr, "malloc failed\n");
	exit (1);
    }
    new_vf->path = cpt;
    strcpy (new_vf->path, PAF_get_full_path (dir, name));
    new_vf->name = new_vf->path + strlen (dir);
    cpt += strlen (new_vf->path) + 1;
    new_vf->prefix = cpt;
    strcpy (new_vf->prefix, prefix);
    new_vf->compress_type = compress_type;
    new_vf->content_type = content_type;
    new_vf->time = ftime;
    new_vf->size = file_size;
    new_vf->selected = selected;
    new_vf->session = -1;
}

/************************************************************************

    Returns the compression type of file "name". Returns -1 on failure.

************************************************************************/

static int Find_compression_type (char *name) {
    int len;

    len = strlen (name);
    if (len >= 4 && strcmp (name + len - 4, ".bz2") == 0)
	return (AP_COMP_BZ2);
    else if (len >= 2 && strcmp (name + len - 2, ".Z") == 0)
	return (AP_COMP_Z);
    else if (len >= 3 && strcmp (name + len - 3, ".gz") == 0)
	return (AP_COMP_GZ);
    else if (len >= 4 && strcmp (name + len - 4, ".raw") == 0)
	return (AP_COMP_BZ2);
    else
	return (-1);
}

/************************************************************************

    Returns the full path of directory "dir" and file name "name".

************************************************************************/

char *PAF_get_full_path (char *dir, char *name) {
    static char buffer[LOCAL_NAME_SIZE] = "";

    if (strlen (dir) + strlen (name) >= LOCAL_NAME_SIZE) {
	fprintf (stderr, "Full path %s%s too long\n", dir, name);
	exit (1);
    }
    sprintf (buffer, "%s%s", dir, name);
    return (buffer);
}

/************************************************************************

    Completes the data directory path from possibly incomplete 
    "path". The output is in "path". Refer to
    play_a2.1 for how the directory path is formed. Dir_name is
    always terminated with "/". Note that AR2_DIR may not be a full
    path. Dir_name should contain, when this is called, the contents 
    of the -d option or the path of the first line of the list file
    in case of -p option.

************************************************************************/

static void Complete_dir_path (char *path) {

    Add_default_path (path);
    Add_pwd_path (path);
    Add_slash (path);
}

/************************************************************************

    Adds the default path in front of "path" if necessary.

************************************************************************/

static void Add_default_path (char *path) {
    char tmp[LOCAL_NAME_SIZE], *cpt;

    if (path[0] == '/' || strcmp (path, ".") == 0 || 
			strncmp (path, "./", 2) == 0)	/* nothing to do */
       return;

    if ((cpt = getenv ("AR2_DIR")) != NULL) {
	if (strlen (cpt) + strlen (path) + 1 >= LOCAL_NAME_SIZE) {
	    fprintf (stderr, "Path too long\n");
	    exit (1);
	}
	strcpy (tmp, path);
	strcpy (path, cpt);
	Add_slash (path);
	strcat (path, tmp);
    }
    else
	Add_pwd_path (path);
}

/************************************************************************

    Adds the PWD path in front of "path" if necessary.

************************************************************************/

static void Add_pwd_path (char *path) {
    static char pwd[LOCAL_NAME_SIZE] = "";
    char tmp[LOCAL_NAME_SIZE], *cpt;

    if (path[0] == '/')	/* nothing to do */
       return;
    cpt = path;
    if (*cpt == '.') {
	cpt++;
	if (*cpt == '/')
	    cpt++;
	else if (*cpt != '\0')		/* not pwd */
	    cpt = path;
    }
    strcpy (tmp, cpt);
    if (pwd[0] == '\0') {
	if (getcwd (pwd, LOCAL_NAME_SIZE) == NULL) { /* get current dir */
	    fprintf (stderr, "getcwd failed (errno %d)\n", errno);
	    exit (1);
	}
	Add_slash (pwd);
    }
    if (strlen (pwd) + strlen (tmp) >= LOCAL_NAME_SIZE) {
	fprintf (stderr, "Path too long\n");
	exit (1);
    }
    strcpy (path, pwd);
    strcat (path, tmp);
}

/********************************************************************
			
    Appends "/" to "path" if the last character is not "/".

********************************************************************/

static void Add_slash (char *path) {

    if (path[strlen (path) - 1] != '/') {
	if (strlen (path) + 1 >= LOCAL_NAME_SIZE) {
	    fprintf (stderr, "Path too long\n");
	    exit (1);
	}
	strcat (path, "/");
    }
}

/********************************************************************
			
    Opens and reads the play list file "list_file_name" with the
    -d option "dir_name" and creates the volume file list. Returns
    the list with "Vol_files". The return value is the number of 
    volume files.

********************************************************************/

int PAF_get_listed_files (char *dir_name, 
			char *list_file_name, Ap_vol_file_t **vol_files) {
    FILE *fl;
    int cnt;
    char buffer[256];

    fl = fopen (list_file_name, "r");
    if (fl == NULL) {
	fprintf (stderr, "fopen %s failed (errno %d)\n", 
					    list_file_name, errno);
	exit (1);
    }

    cnt = 0;
    while (fgets (buffer, 256, fl) != NULL) {
	char *tok, *fname, dir[LOCAL_NAME_SIZE * 2], *cpt;
	struct stat st;
	int line_used, ret, compress_type;

	if ((tok = strtok (buffer, " \n\t")) == NULL)
	    continue;
	line_used = 0;

	if (cnt == 0) {
	    if (strcmp (tok, "PATH") == 0) {
		Get_list_file_path (dir_name);
		line_used = 1;
	    }
	    Complete_dir_path (dir_name);
	}
	cnt++;
	if (line_used)
	    continue;

	if (tok[0] == '/')
	    fname = tok;
	else
	    fname = PAF_get_full_path (dir_name, tok);
	ret = stat (fname, &st);
	if (ret < 0)
	    continue;
	if (!S_ISREG (st.st_mode)) {
	    printf ("File %s is not a regular file\n", fname);
	    continue;
	}

	if ((compress_type = Find_compression_type (fname)) < 0) {
	    printf ("File (%s) compression type unknown\n", fname);
	    continue;
	}
	strcpy (dir, fname);
	cpt = dir + strlen (dir) - 1;
	while (cpt >= dir) {
	    if (*cpt == '/') {
		cpt[1] = '\0';
		break;
	    }
	    cpt--;
	}
	Add_a_new_vol_file ("", dir, fname + (cpt - dir + 1), 
		compress_type, AP_NCDC_TYPE, 0, st.st_size, 1);
    }

    *vol_files = Vol_files;
    return (N_vol_files);
}

/********************************************************************
			
    Gets the PATH from the play list file.

********************************************************************/

static void Get_list_file_path (char *dir_name) {
    char *tok;

    if ((tok = strtok (NULL, " \n\t")) != NULL &&
	strcmp (tok, "=") == 0 && 
	(tok = strtok (NULL, " \n\t")) != NULL) {

	if (strlen (tok) >= LOCAL_NAME_SIZE) {
	    fprintf (stderr, "PATH too long in play list file\n");
	    exit (1);
	}
	strcpy (dir_name, tok);
    }
}

/********************************************************************
			
    The Heapsort algorithm from "Numerical recipes in C". Refer to 
    the book. It sorts array "vf" of size "n" into ascent order.

********************************************************************/

static void Sort_file_names (int n, Ap_vol_file_t *vf) {
    int l, j, ir, i;
    Ap_vol_file_t tvf;				/* type dependent */

    if (n <= 1)
	return;
    vf--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    tvf = vf[--l];
	else {
	    tvf = vf[ir];
	    vf[ir] = vf[1];
	    if (--ir == 1) {
		vf[1] = tvf;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && Is_less_than (vf + j, vf + j + 1))
						/* type dependent */
		++j;
	    if (Is_less_than (&tvf, vf + j)) {	/* type dependent */
		vf[i] = vf[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	vf[i] = tvf;
    }
}

/********************************************************************
			
    Volume file name comparison function sorting the file names. "left"
    and "right" are the two name to compare. Returns 1 if "left" should
    be in front of "right" or 0 otherwise.

********************************************************************/

static int Is_less_than (Ap_vol_file_t *left, Ap_vol_file_t *right) {

    int c;

    c = strcmp (left->prefix, right->prefix);
    if (c < 0)
	return (1);
    else if (c == 0)
	return (left->time < right->time);
    else
	return (0);
}

