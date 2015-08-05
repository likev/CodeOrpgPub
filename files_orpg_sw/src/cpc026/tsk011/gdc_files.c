
/******************************************************************

    gdc is a tool that generates device configuration file for
    specified site(s). This module contains file related functions.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/01/03 19:23:57 $
 * $Id: gdc_files.c,v 1.1 2011/01/03 19:23:57 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "gdc_def.h"

extern int VERBOSE;
extern char SRC_dir[];
extern char ARGV0[];

static char *Out_fname = NULL;
static FILE *Fout = NULL;
static char *Dest_dir = NULL;
static char *Link_path = NULL;
static char *Shared_dir = NULL;
static int To_install = 0;

static char *Install_path = NULL;
static int Is_link = 0;
static int File_exist = 0;

static int Run_cmd (char *cmd);
static char *Get_install_path (char *text, char *path, int size);
static int Shared_file_exist (char *path);
static int Copy_from_file (char *fname, int comment, char *ccs);


/******************************************************************

    Receives the shared variables from the main module.

******************************************************************/

void GDCF_share_vars (char *out_fname, FILE *fout, char *dest_dir, 
		char *link_path, char *shared_dir, int to_install) {
    Fout = fout;
    Out_fname = out_fname;
    Dest_dir = dest_dir;
    Link_path = link_path;
    Shared_dir = shared_dir;
    To_install = to_install;
    Is_link = 0;
    File_exist = 0;
    Install_path = NULL;
}

/******************************************************************

    Processes control variables.

******************************************************************/

void GDCF_gdc_control (char *v_name, char *value) {
    char tok[MAX_STR_SIZE];

    if (strcmp (v_name, "_gdc_install_file") == 0) {
	int off, is_link;
	off = GDCM_ftoken (value, "", 0, tok, MAX_STR_SIZE);
	is_link = 1;
	if (off <= 0 || strcmp (tok, "link") != 0) {
	    is_link = 0;
	    off = 0;
	}
	if (GDCF_set_install_path (is_link, value + off))
	    GDCP_set_done ();
    }
    else if (strcmp (v_name, "_gdc_import_from") == 0) {
	int off, binary, optional;
	char *p = value;
	binary = optional = 0;
 	off = GDCM_ftoken (p, "", 0, tok, MAX_STR_SIZE);
	if (off > 0 && strcmp (tok, "binary") == 0) {
	    binary = 1;
	    p += off;
	}
	if (GDCM_stoken (p, "", 1, tok, MAX_STR_SIZE) >= 0 &&
	    strcmp (tok, "optional") == 0)
	    optional = 1;
	if (GDCM_ftoken (p, "", 0, tok, MAX_STR_SIZE) < 0)
	    GDCP_exception_exit ("File name not found in copy instruction\n");
	if (GDCF_copy_from_file (tok, binary, GDCP_get_ccs (), optional))
	    GDCP_set_done ();
    }
    else if (strcmp (v_name, "_gdc_execute") == 0) {
	static int n_cmds = 0;
	static char *cmds = NULL;
	int i;
	char *cmd = cmds;
	for (i = 0; i < n_cmds; i++) {
	    if (strcmp (cmd, value) == 0)
		break;
	    cmd += strlen (cmd) + 1;
	}
	if (i >= n_cmds) {
	    char command[MAX_STR_SIZE], buf[MAX_STR_SIZE];
	    GDCM_strlcpy (command, value, MAX_STR_SIZE);
	    if (GDCM_stoken (command, "", 0, buf, MAX_STR_SIZE) > 0 &&
		strcmp (buf, "gdc") == 0)
		GDCM_add_dir (MISC_dirname (ARGV0, buf, MAX_STR_SIZE), 
						command, MAX_STR_SIZE);
            if (Run_cmd (command) < 0)
		GDCP_exception_exit ("Executing command failed: %s\n",
							command);
	    cmds = STR_append (cmds, value, strlen (value) + 1);
	    n_cmds++;
	}
   }
}

/******************************************************************

    Sets Install_path, File_exist and Is_link for later file install
    processing. Called when a install instruction is processed.
    Returns 1 if shared file exist or 0 otherwise.

******************************************************************/

int GDCF_set_install_path (int is_link, char *inst) {
    static char path[MAX_STR_SIZE];

    if (GDCP_is_include ())
	GDCP_exception_exit ("Install instruction cannot be in variable definition file\n");

    Is_link = is_link;
    if (VERBOSE && To_install)
	printf ("    Process install path %s\n", inst);
    if (Install_path == NULL)
	Install_path = Get_install_path (inst, path, MAX_STR_SIZE);
    if (Install_path && Is_link)
	File_exist = Shared_file_exist (Install_path);
    if (File_exist) {
	if (VERBOSE && To_install)
	    printf ("    Shared file %s exits - not created\n", 
				MISC_basename (Install_path));
	return (1);
    }
    return (0);
}

/******************************************************************

    Returns 1 if the file in "path" exists in the shared dir and
    it is a regular file and its size > 0. Otherwise, 0 is returned.

******************************************************************/

#include <time.h>

static int Shared_file_exist (char *path) {
    char *name;
    struct stat st;
    int ret;

    if (Link_path[0] == '\0')
	return (0);

    name = STR_gen (NULL, Shared_dir, "/", MISC_basename (path), NULL);
    ret = stat (name, &st);
    STR_free (name);
    if (ret < 0 ||
	!S_ISREG (st.st_mode) ||
	st.st_size <= 0)
	return (0);
    if (time (NULL) > st.st_ctime + 300)
	return (0);
    return (1);
}

/******************************************************************

    Copies the contents of "fname" to the output file. If "binary" is 
    true, binary copy is performed. If "optional" is true, the copy 
    is optional. "ccs" is the control chars. Returns 1 on success or
    0 if the file is not found.

******************************************************************/

int GDCF_copy_from_file (char *fname, int binary, char *ccs, int optional) {
    char *ev, name[MAX_STR_SIZE];
    int ret;

    if (GDCP_is_include ())
	GDCP_exception_exit ("Copy instruction cannot be in variable definition file\n");

    if (VERBOSE)
	printf ("    Process copy from %s %d %d\n", fname, binary, optional);
    if ((ev = GDCP_evaluate_variables (fname)) != NULL) {
	GDCM_strlcpy (name, ev, MAX_STR_SIZE);
	free (ev);
    }
    else
	GDCM_strlcpy (name, fname, MAX_STR_SIZE);
    if (!binary)
	ret = Copy_from_file (name, 1, ccs);
    else
	ret = Copy_from_file (name, 0, ccs);
    if (ret)
	return (1);
    if (!optional)
	GDCP_exception_exit ("Could not find import file %s\n", name);
 
    return (0);
}

/******************************************************************

    Copies the contents of "fname" to the output file. Returns 1 on
    success of 0 if the file is not found. If "comment" is true, a 
    message is written to the file before copying. "ccs" is the 
    control chars.

******************************************************************/

static int Copy_from_file (char *fname, int comment, char *ccs) {
    FILE *fd;
    char name[MAX_STR_SIZE];

    if (To_install && Install_path == NULL)
	return (1);			/* not necessary */

    GDCM_strlcpy (name, fname, MAX_STR_SIZE);
    GDCM_add_dir (SRC_dir, name, MAX_STR_SIZE);
				/* add src dir to input file name */
    fd = fopen (name, "r");
    if (fd == NULL)
	return (0);

    if (VERBOSE)
	printf ("    Copy from %s\n", name);
    if (comment)
	fprintf (Fout, "%c The following is copied from %s\n", 
						ccs[COM_C], name);
    while (1) {
	char buf[1024];
	int cnt;
	cnt = fread (buf, 1, 1024, fd);
	if (cnt <= 0)
	    break;
	if (fwrite (buf, 1, cnt, Fout) != cnt) {
	    fprintf (stderr, "Failed in writing output file %s\n", Out_fname);
	    exit (1);
	}
    }
    fclose (fd);
    return (1);
}

/**************************************************************************

    Copies the output file to the install file specified in the template 
    file.

**************************************************************************/

int GDCF_install_file () {
    char *p, path[MAX_STR_SIZE], dest[MAX_STR_SIZE];
    char new_name[MAX_STR_SIZE];
    char buf[MAX_STR_SIZE * 2 + 128], tok[MAX_STR_SIZE];
    int nt, is_gz, is_tar, len, path_levels;

    p = Install_path;
    if (p == NULL) {
	unlink (Out_fname);
	if (VERBOSE)
	    printf ("Install not needed for this node and site\n");
	return (0);
    }

    if (strlen (p) + 32 > MAX_STR_SIZE)
	GDCP_exception_exit ("Install file name (%s) too long\n", p);
    GDCM_strlcpy (path, p, MAX_STR_SIZE);
    is_gz = is_tar = 0;
    if (Link_path[0] == '\0' || !Is_link) {
	nt = MISC_get_token (path, "S.", 0, NULL, 0);
	if (nt > 1 && GDCM_stoken 
				(path, "S.", nt - 1, tok, MAX_STR_SIZE) > 0) {
	    if (strcmp (tok, "gz") == 0) {
		if (nt > 2 && 
		    GDCM_stoken 
				(path, "S.", nt - 2, tok, MAX_STR_SIZE) > 0 &&
		    strcmp (tok, "tar") == 0)
		    is_tar = 1;
		is_gz = 1;
	    }
	    else if (strcmp (tok, "tar") == 0)
		is_tar = 1;
	}
    }

    len = 0;
    if (is_gz)
	len += 3;
    if (is_tar)
	len += 4;
    path[strlen (path) - len] = '\0';

    if (is_tar) {
	char tmp[MAX_STR_SIZE];
	int i;
	if (rename (Out_fname, MISC_basename (path)) < 0)
	    GDCP_exception_exit ("rename (%s %s) failed\n", 
				Out_fname, MISC_basename (path));
	strcpy (tmp, path);
	i = 0;
	while (path[i] != '\0') {
	    if (path[i] == ':')
		path[i] = '_';
	    i++;
	}
	GDCM_strlcat (path, ".tar", MAX_STR_SIZE);
	GDCM_strlcpy (new_name, MISC_basename (path), MAX_STR_SIZE);
	sprintf (buf, "tar cf %s %s", new_name, MISC_basename (tmp));
	if (Run_cmd (buf) != 0)
	    exit (1);
	unlink (MISC_basename (tmp));
    }
    else
	strcpy (new_name, Out_fname);

    if (path[0] == '/') {
	GDCM_strlcpy (dest, "root/", MAX_STR_SIZE);
	GDCM_strlcat (dest, path + 1, MAX_STR_SIZE);
    }
    else if (path[0] == '.' && path[1] == '/') {
	GDCM_strlcpy (dest, path + 2, MAX_STR_SIZE);
    }
    else {
	GDCM_strlcpy (dest, "user/", MAX_STR_SIZE);
	GDCM_strlcat (dest, path, MAX_STR_SIZE);
    }
    path_levels = MISC_get_token (path, "S/", 0, NULL, 0) - 1;
    GDCM_add_dir (Dest_dir, dest, MAX_STR_SIZE);

    if (MISC_mkdir (MISC_dirname (dest, buf, MAX_STR_SIZE)))
	GDCP_exception_exit ("MISC_mkdir (%s) failed\n", buf);
    if (Link_path[0] == '\0' || !Is_link) {
	if (rename (new_name, dest) < 0)
	    GDCP_exception_exit ("Failed in installing (rename) (%s)\n", dest);
    }
    else {
	int i;
	char *name = NULL;
	name = STR_copy (name, "");
	for (i = 0; i < path_levels; i++) 
	    name = STR_cat (name, "../");
	name = STR_cat (name, Link_path);
	name = STR_cat (name, "/");
	name = STR_cat (name, MISC_basename (dest));
	unlink (dest);
	if (symlink (name, dest) < 0)
	    GDCP_exception_exit (
			"Failed in installing (symlink) link (%s)\n", dest);
	STR_free (name);
	name = NULL;
	if (!File_exist) {
	    name = STR_gen (name, Shared_dir, "/", MISC_basename (dest), NULL);
	    if (MISC_mkdir (MISC_dirname (name, buf, MAX_STR_SIZE)))
		GDCP_exception_exit ("MISC_mkdir (%s) failed\n", buf);
	    if (rename (new_name, name) < 0)
		GDCP_exception_exit (
			"Failed in installing shared (%s)\n", name);
	    STR_free (name);
	}
    }

    if (is_gz) {
	sprintf (buf, "gzip -f %s", dest);
	if (Run_cmd (buf) != 0)
	    exit (1);
	GDCM_strlcat (dest, ".gz", MAX_STR_SIZE);
    }

    printf ("Created %s\n", dest);

    return (1);
}

/******************************************************************

    Reads content of the file "name". The content is returned with
    "buf_p" which is null-terminated. The caller must free the pointer.
    It returns the size of the file or -1 on failure.

******************************************************************/

int GDCF_read_file (char *name, char **buf_p) {
    FILE *fd;
    char *buf, *p, *op;
    int size;

    fd = fopen (name, "r");
    if (fd == NULL) {
	fprintf (stderr, "Could not open file %s\n", name);
	exit (1);
    }
    if (fseek (fd, 0, SEEK_END) < 0 ||
	(size = ftell (fd)) < 0 ||
	fseek (fd, 0, SEEK_SET) < 0) {
	fprintf (stderr, "Failed in finding file (%s) size\n", name);
	exit (1);
    }

    buf = MISC_malloc (size + 1);
    if (fread (buf, 1, size, fd) != size) {
	fprintf (stderr, "read (file %s, %d bytes) failed\n", name, size);
	exit (1);
    }
    buf[size] = '\0';
    *buf_p = buf;
    fclose (fd);

    /* preprocess the file */
    p = buf;
    op = p;
    while (*p != '\0') {
	if (*p == 13) {		/* discard \r */
	    p++;
	    continue;
	}
	if (*p == '\\' && p[1] == '\n') {	/* continuing line */
	    p += 2;
	    continue;
	}
	*op = *p;
	op++;
	p++;
    }
    *op = '\0';
    return (op - buf);
}

/******************************************************************

    Runs command "cmd". Returns 0 on success or -1 on failure.

******************************************************************/

#define OUT_BUF_SIZE 20480

static int Run_cmd (char *cmd) {
    int ret;
    char buf[OUT_BUF_SIZE];

    if (VERBOSE)
	printf ("    RUN: %s\n", cmd);
    ret = MISC_system_to_buffer (cmd, buf, OUT_BUF_SIZE, NULL);
    if (VERBOSE && strlen (buf) > 0)
	printf ("%s\n", buf);
    if (ret != 0) {
	fprintf (stderr, 
		"Failed in MISC_system_to_buffer (%d) %s\n", ret, cmd);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Parses the install statement line of "text" in the template 
    configuration file. Returns the install path if the node and site 
    category match, or NULL otherwise. If an syntax error is found, this
    terminates the program. "path" is the caller provided buffer of "size"
    bytes.

**************************************************************************/

static char *Get_install_path (char *text, char *path, int size) {
    char section[MAX_STR_SIZE], *p;
    int off, ind;

    off = GDCM_ftoken (text, "", 0, section, MAX_STR_SIZE);
    if (off <= 0)
	goto err;
    GDCM_strlcpy (path, section, size);
    if ((p = GDCP_evaluate_variables (path)) != NULL) {
	GDCM_strlcpy (path, p, size);
	free (p);
    }
    p = text + off;

    ind = 0;
    while (GDCM_ftoken (p, "S;", ind, section, MAX_STR_SIZE) > 0) {
	int node_check, site_check, node_ok, site_ok, i;
	char tok[MAX_STR_SIZE], *v;

	i = 0;
	node_check = 0;
	site_check = 0;
	node_ok = 1;
	site_ok = 1;
	while (GDCM_ftoken (section, "", i++, tok, MAX_STR_SIZE) > 0) {
	    if (strcmp (tok, "on") == 0) {
		if (node_check || site_check)
		    goto err;
		node_check = 1;
		node_ok = 0;
		continue;
	    }
	    if (strcmp (tok, "of") == 0) {
		if (site_check || node_check == 1)
		    goto err;
		node_check = 0;
		site_check = 1;
		continue;
	    }
	    if (node_check) {
		if ((v = GDCV_get_value ("NODE_NAME")) != NULL) {
		    if (strcmp (v, tok) == 0)
			node_ok = 1;
		}
		else if (To_install)
		    GDCP_exception_exit ("Variable NODE_NAME not defined\n");
		node_check++;
	    }
	    if (site_check) {
		if ((v = GDCV_get_value (tok)) == NULL ||
		    strcmp (v, "YES") != 0)
		    site_ok = 0;
		site_check++;
	    }
	    if (!node_check && !site_check)
		goto err;
	}
	if (node_check == 1 || site_check == 1)
	    goto err;
	if (node_ok && site_ok)
	    return (path);
	ind++;
    }
    if (ind == 0)
	return (path);
    return (NULL);

err:
    if (strlen (text) > 128)
	strcpy (text + 128 - 4, "...");
    GDCP_exception_exit ("Unexpected install spec: %s\n", text);
    return (NULL);
}
