
/******************************************************************

    This is the main module for restore_adapt.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/16 21:06:33 $
 * $Id: restore_adapt.c,v 1.48 2013/04/16 21:06:33 steves Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>

#include <security/pam_appl.h>
#include <security/pam_modules.h>

#include <orpg.h>
#include <orpgadpt.h>
#include <infr.h>

#define BUF_SIZE 512
#define CMD_SIZE 256
#define	MAX_PASSWD_LENGTH 16
#define	ROOT_USER "root"

/* command line options */
static char *Src_dir = ".";
static char *Spec_channel = "";
static char *Spec_date = "";
static char *Spec_time = "";
static char *Test_install_dest = NULL;
static int From_cd = 0;
static int From_floppy = 0;
static int From_cdrom = 0;
static int Install_config = 0;
static int Interactive = 1;

static char err_msg[256] = "";

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static char *Get_stdin ();
static void Mount_media (int unmount);
static void Term_on_error (int);
static void Install_cfg_files ();
static int Install_untar_files (char *user, char *src);
static int Varifies_root ();
static int Get_root_passwd (int num_msg, const struct pam_message **msg,
                     struct pam_response **resp, void *appdata_ptr);
static int Read_password (char *buf, int buf_size);
static void Modify_file (char *file_name);
static void Run_script (char *script);
static int Change_owner (char *mrpg_hn, char *cfg_dir);


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret, chan, stat, ver;
    char mrpg_hn[ADPTU_NAME_SIZE], cfg_dir[ADPTU_NAME_SIZE];
    char site[ADPTU_NAME_SIZE], format[ADPTU_NAME_SIZE];
    static char *func = NULL, *dest = NULL;
    char *fname;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if ((ret = LE_init (argc, argv)) < 0) {
	sprintf (err_msg, "LE_init failed (%d)", ret);
	Term_on_error (1);
    }

    if (Install_config || Test_install_dest != NULL)
	Install_cfg_files ();

    if (strcasecmp (Spec_channel, "mscf") == 0) {
	sprintf (err_msg, "No adaptation data on mscf to restore");
	Term_on_error (1);
    }
    else if (strlen (Spec_channel) > 0 &&
	     strcasecmp (Spec_channel, "rpg1") != 0 &&
	     strcasecmp (Spec_channel, "rpg2") != 0) {
	sprintf (err_msg, "Unexpected RPG channel (%s) specified", Spec_channel);
	Term_on_error (1);
    }

    chan = 0;
    if (strlen (Spec_channel) == 0) {	/* try local node */
	char *node_type, *ch;
	node_type = ORPGMISC_get_site_name ("type");
	ch = ORPGMISC_get_site_name ("channel_num");
	if (node_type != NULL && ch != NULL &&
	    strncmp (node_type, "rpg", 3) == 0) {
	    if (strcmp (ch, "2") == 0)
		chan = 2;
	    else
		chan = 1;
	}
    }
    if (chan <= 0) {			/* find remote RPG node */
	if (strlen (Spec_channel) == 0 ||
	    strcasecmp (Spec_channel, "rpg1") == 0) {
	    ret = ORPGMGR_discover_host_ip ("rpga", 1, mrpg_hn, ADPTU_NAME_SIZE);
	    chan = 1;
	}
	else if (strcasecmp (Spec_channel, "rpg2") == 0) {
	    ret = ORPGMGR_discover_host_ip ("rpga", 2, mrpg_hn, ADPTU_NAME_SIZE);
	    chan = 2;
	}
	else {
	    sprintf (err_msg, 
		"Unexpected channel name (%s) from -o option", Spec_channel);
	    Term_on_error (1);
	}
	if (ret <= 0) {
	    if (ret < 0)
		sprintf (err_msg, "ORPGMGR_discover_host_ip failed (%d)", ret);
	    else 
		sprintf (err_msg, "Node rpg%d not found", chan);
	    Term_on_error (1);
	}
    }

    LE_send_msg (0, "restore_adapt on %s ...", mrpg_hn);
    if (Interactive)
	printf ("Restoring adaptation data...\n");

    if (mrpg_hn[0] == '\0')
	func = STR_gen (func, "liborpg.so,ORPGADPTU_restore_1", NULL);
    else
	func = STR_gen (func, mrpg_hn, 
			":liborpg.so,ORPGADPTU_restore_1", NULL);
    RMT_time_out (20);
    sprintf (format, "i-r ba-%d-io ba-%d-io i-i", 
				ADPTU_NAME_SIZE, ADPTU_NAME_SIZE);
    ret = RSS_rpc (func, format, &ver, site, cfg_dir, ADPTU_NAME_SIZE);
    if (ret < 0) {
	sprintf (err_msg, "RSS_rpc %s failed (%d)", func, ret);
	Term_on_error (1);
    }
    if (ver < 0) {
	sprintf (err_msg, "%s failed", func);
	Term_on_error (1);
    }
    LE_send_msg (0, "RPG site: %s; adapt version: %d", site, ver);

    if (chan == 1)
	Spec_channel = "rpg1";
    else 
	Spec_channel = "rpg2";
    LE_send_msg (0, "RPG channel: %s", Spec_channel);

    if (From_cd || From_floppy || From_cdrom)
	Mount_media (0);

    fname = ORPGADPTU_find_archive_name (Src_dir, Spec_date, Spec_time, 
	site, Spec_channel, ver / 10, NULL, NULL);
    if (fname == NULL) {
	sprintf (err_msg, "No specified adapt archive found in %s", Src_dir);
	Term_on_error (1);
    }
    LE_send_msg (0, "Restored adaptation data from %s.", fname);
    LE_send_msg (0, "The restored adaptation data will be used on the next application startup.\n");

    if (mrpg_hn[0] == '\0')
	dest = STR_gen (dest,
		cfg_dir, "/adapt/", MISC_basename (fname), ".t", NULL);
    else
	dest = STR_gen (dest, mrpg_hn, ":", 
		cfg_dir, "/adapt/", MISC_basename (fname), ".t", NULL);
    ret = RSS_copy (fname, dest);		/* copy the file over */
    if (ret < 0) {
	sprintf (err_msg, "RSS_copy from %s to %s failed", fname, dest);
	Term_on_error (1);
    }
    LE_send_msg (0, "File %s copied to %s/adapt on %s", 
						fname, cfg_dir, mrpg_hn);

    if (Change_owner (mrpg_hn, cfg_dir) < 0) {
	sprintf (err_msg, "Setting adapt write permission failed");
	Term_on_error (1);
    }

    if (mrpg_hn[0] == '\0')
	func = STR_gen (func, "liborpg.so,ORPGADPTU_restore_2", NULL);
    else
	func = STR_gen (func, mrpg_hn, 
			":liborpg.so,ORPGADPTU_restore_2", NULL);
    ret = RSS_rpc (func, "i-r s-i", &stat, MISC_basename (fname));
    if (ret < 0) {
	sprintf (err_msg, "RSS_rpc %s failed (%d)", func, ret);
	Term_on_error (1);
    }
    if (stat < 0) {
	sprintf (err_msg, "%s failed (%d)", func, stat);
	Term_on_error (1);
    }

    Mount_media (1);
    LE_send_msg (0, "restore_adapt completed");
    if (Interactive)
	printf ("restore_adapt (RPG on %s, chan %d) completed\n", mrpg_hn, chan);
    exit (0);
}

/******************************************************************

    Changes owner of cfg_dir/adapt to the rpg user. Returns 0 on success
    or -1 on failure.

******************************************************************/

static int Change_owner (char *mrpg_hn, char *cfg_dir) {
    char *h, *cmd, *func, buf[256], user[128];
    int ret, fret, n, ret_value;

    h = getenv ("HOME");
    if (h == NULL ||
	MISC_get_token (h, "S/", 2, user, 128) <= 0) {
	LE_send_msg (GL_ERROR, "User name not found from HOME\n");
	return (-1);
    }

    ret_value = 0;
    cmd = STR_gen (NULL, "chown ", user, " ", cfg_dir, "/adapt", NULL);
    if (mrpg_hn[0] == '\0')
	func = STR_gen (NULL, "MISC_system_to_buffer", NULL);
    else
	func = STR_gen (NULL, mrpg_hn, ":", "MISC_system_to_buffer", NULL);
    ret = RSS_rpc (func, "i-r s-i ba-32-o i-i ia-1-o", &fret, 
				cmd, buf, 32, &n);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "RSS_rpc (%s) failed (%d)\n", func, ret);
	ret_value = -1;
    }
    if (fret != 0) {
	LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer (%s) (on %s) failed (%d)\n", 
						cmd, mrpg_hn, fret);
	ret_value = -1;
    }
    STR_free (cmd);
    STR_free (func);
    return (ret_value);
}

/******************************************************************

    Installs configuration files to mscf. This function does not 
    return when running on HUB loading mscf.

******************************************************************/

static void Install_cfg_files () {
    char *node, *dir;
    static char *src = NULL;
    int err;
    struct stat st;

    node = ORPGMISC_get_site_name ("type");
    if (node == NULL) {
	sprintf (err_msg, "ORPGMISC_get_site_name - site info not found");
	Term_on_error (1);
    }

    if (From_cd || From_floppy || From_cdrom)
	Mount_media (0);
    else if (Test_install_dest == NULL) {
	sprintf (err_msg, "Media type not specified");
	Term_on_error (1);
    }

    dir = ORPGADPTU_install_dir (node, NULL, -1, 0);
    if (dir == NULL) {
	sprintf (err_msg, "ORPGADPTU_install_dir failed");
	Term_on_error (1);
    }

    src = STR_gen (src, Src_dir, "/", dir, NULL);
    if (stat (src, &st) != 0 || !S_ISDIR (st.st_mode)) {
	sprintf (err_msg, "Dir %s not found in %s", src, Src_dir);
	Term_on_error (1);
    }

    LE_send_msg (0, "Installing files from %s", src);
    err = 0;
    if (Install_untar_files ("user", src) != 0)
	err = 1;
    if (!err && Install_untar_files ("root", src) != 0)
	err = 1;
    if (err)
	Term_on_error (1);

    Mount_media (1);
    exit (err);
}

/******************************************************************

    Installs the files in subdirectory "user" from "src". Return 0
    on success or -1 is any error is detected.
	
******************************************************************/

static int Install_untar_files (char *user, char *src) {
    static char *name = NULL;
    char buf[BUF_SIZE], cmd[CMD_SIZE], *dest;
    int ret, err, n_bytes;
    struct stat st;
    mode_t mode;

    err = 0;
    mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH | 0111;
    if (strcmp (user, "user") == 0)
	mode |= S_IWGRP | S_IWOTH;
    if (Test_install_dest == NULL) {
	dest = "/";
	if (strcmp (user, "user") == 0) {
	    char *h = getenv ("HOME");
	    if (h == NULL) {
		sprintf (err_msg, "HOME undefined");
		return (-1);
	    }
	    dest = h;
	}
    }
    else {
	static char *d_buf = NULL;
	d_buf = STR_gen (d_buf, Test_install_dest, "/", "root", NULL);
	if (strcmp (user, "user") == 0)
	    d_buf = STR_gen (d_buf, Test_install_dest, "/", "user", NULL);
	dest = d_buf;
    }
    name = STR_gen (name, src, "/", user, NULL);
    if (stat (name, &st) == 0 &&
	S_ISDIR (st.st_mode)) {
	int ind;
	char tk[256], t[256];

	if (strcmp (user, "root") == 0 && Test_install_dest == NULL &&
	    getuid () != 0) {
	    if (Varifies_root () != 0) {
		sprintf (err_msg, "Root login failed");
		return (-1);
	    }
	    if (setuid (0) != 0) {
		sprintf (err_msg, "Setuid to root failed");
		return (-1);
	    }
	}
	if (MISC_mkdir (dest) != 0) {
	    sprintf (err_msg, "mkdir %s failed", dest);
	    return (-1);
	}
	sprintf (cmd, "find %s -name \".*\" -type f", name);
	LE_send_msg (0, "find cmd: %s", cmd);
	ret = MISC_system_to_buffer (cmd, buf, BUF_SIZE, &n_bytes);
	if (ret != 0) {
	    if (n_bytes > 0)
		LE_send_msg (0, "%ms", buf);
	    sprintf (err_msg, "Installing %s files failed (find)", user);
	    return (-1);
	}
	if (n_bytes > 0)
	    sprintf (cmd, "sh -c \"set -o pipefail; cp -vrfL %s/* %s/.[!.]* %s | awk -F[\134\140\134\047] \047{print $4}\047\"", name, name, dest);
	else
	    sprintf (cmd, "sh -c \"set -o pipefail; cp -vrfL %s/* %s | awk -F[\134\140\134\047] \047{print $4}\047\"", name, dest);
	LE_send_msg (0, "copy cmd: %s", cmd);
	ret = MISC_system_to_buffer (cmd, buf, BUF_SIZE, &n_bytes);
	if (ret != 0) {
	    if (n_bytes > 0)
		LE_send_msg (0, "%ms", buf);
	    sprintf (err_msg, "Installing %s files failed (cp)", user);
	    return (-1);
	}

	ind = 0;
	while (MISC_get_token (buf, "S\n", ind, tk, 256) > 0) {
	    int i, is_tar, is_gz, is_modify, is_script;
	    struct stat fst;

	    LE_send_msg (0, "post process: %s", tk);
	    is_tar = is_gz = is_modify = is_script = i = 0;
	    while (MISC_get_token (tk, "S.", i, t, 256) > 0) {
		if (strcmp (t, "tar") == 0)
		    is_tar = 1;
		if (strcmp (t, "gz") == 0)
		    is_gz = 1;
		if (strcmp (t, "modify") == 0)
		    is_modify = 1;
		if (strcmp (t, "script") == 0)
		    is_script = 1;
		i++;
	    }
	    stat (tk, &fst);
	    LE_send_msg (0, "chmod %s %o", tk, mode & fst.st_mode);
	    chmod (tk, mode & fst.st_mode);
	    if (is_script)
		Run_script (tk);
	    else if (is_modify)
		Modify_file (tk);
	    else if (is_tar || is_gz) {
		char b[BUF_SIZE], bb[BUF_SIZE];
		if (is_tar && is_gz) {
		    sprintf (cmd, "tar zxvf %s -C %s", 
			    tk, MISC_dirname (tk, b, BUF_SIZE));
		}
		else if (is_tar)
		    sprintf (cmd, "tar xvf %s -C %s", 
			    tk, MISC_dirname (tk, b, BUF_SIZE));
		else 
		    sprintf (cmd, "gunzip %s", tk);
		LE_send_msg (0, "run cmd: %s", cmd);
		ret = MISC_system_to_buffer (cmd, bb, BUF_SIZE, &n_bytes);
		if (ret != 0) {
		    if (n_bytes > 0)
			LE_send_msg (0, "%ms", bb);
		    sprintf (err_msg, "tar or gunzip failed");
		    err = -1;
		}
		else {
		    if (is_tar) {
			static char *n = NULL;
			i = 0;
			while (MISC_get_token (bb, "S\n", i, t, 256) > 0) {
			    n = STR_gen (n, MISC_dirname (tk, b, BUF_SIZE), 
								"/", t, NULL);
			    stat (tk, &fst);
			    LE_send_msg (0, "chmod %s %o", n, 
						mode & fst.st_mode);
			    chmod (n, mode & fst.st_mode);
			    i++;
			}
			unlink (tk);
		    }
		    LE_send_msg (0, "File %s installed and expanded", tk);
		}
	    }
	    else
		LE_send_msg (0, "File %s installed", tk);
	    ind++;
	}
	LE_send_msg (0, "%s files in %s installed", user, src);
    }
    else
	LE_send_msg (0, "No %s files to install in %s", user, src);
    return (err);
}

/**************************************************************************

    "file_name" is assumed to be "f_name.modify". This function modifies
    file f_name using lines in "file_name". In "file_name", each line is
    either a comment line started with the first non-empty char of "#"
    or a line of two tokens separated by ",". A token containing space 
    must be quoted by """. If the first token is empty, the second token
    is inserted at the end of file. Otherwise, if the second token is
    empty, the lines matching the first token is deleted. Otherwise, lines
    matching the first token are replaced by the second token. The file 
    mode does not change after the modification. Before inserting a line,
    any existing such lines are removed.

**************************************************************************/

static void Modify_file (char *file_name) {
    char *f_name, buf[BUF_SIZE], *cmd, *tmp_fname, *sed_cmd_file, *p;
    FILE *fl, *sed_cmd_fl;
    int lcnt, ccnt;

    f_name = STR_copy (NULL, file_name);
    p = strstr (f_name, ".modify");
    if (p == NULL)		/* should never happen */
	return;
    *p = '\0';
    tmp_fname = "/tmp/restore_adapt.tmp";
    sed_cmd_file = "/tmp/restore_adapt.sed_cmd";

    fl = fopen (file_name, "r");
    if (fl == NULL)		/* should never happen */
	return;
    sed_cmd_fl = fopen (sed_cmd_file, "w");
    if (sed_cmd_fl == NULL) {
	sprintf (err_msg, "Failed in creating tmp file %s", sed_cmd_file);
	Term_on_error (1);
    }
    cmd = STR_copy (NULL, "sh -c \"rm ");
    cmd = STR_cat (cmd, tmp_fname);
    cmd = STR_cat (cmd, "; sed -f ");
    cmd = STR_cat (cmd, sed_cmd_file);
    cmd = STR_cat (cmd, " ");
    lcnt = ccnt = 0;
    while (fgets (buf, BUF_SIZE, fl) != NULL) {
	char str1[BUF_SIZE], str2[BUF_SIZE], c, *tmp;
	lcnt++;
	c = buf[MISC_char_cnt (buf, " \t\n")];
	if (c == '#' || c == '\0')		/* comment line */
	    continue;
	if (MISC_get_token (buf, "S,Q\"", 0, str1, BUF_SIZE) <= 0 ||
	    MISC_get_token (buf, "S,Q\"", 1, str2, BUF_SIZE) <= 0 ||
	    (str1[0] == '\0' && str2[0] == '\0')) {
	    LE_send_msg (0, "Unexpected line (%d) in %s - Not processed", 
							lcnt, file_name);
	    continue;
	}
	if (str1[0] == '\0') {		/* inserting */
	    tmp = STR_gen (NULL, "grep \"", str2, "\" ", f_name, NULL);
	    if (MISC_system_to_buffer (tmp, NULL, 0, NULL)) {
		STR_free (tmp);
		tmp = STR_gen (NULL, "$a\\", str2, "\n", NULL);
	    }
	    else
		tmp = STR_copy (tmp, "");	/* already exists */
	}
	else if (str2[0] == '\0')	/* deleting */
	    tmp = STR_gen (NULL, "/", str1, "/d\n", NULL);
	else				/* replacing */
	    tmp = STR_gen (NULL, "/", str1, "/c\\", str2, "\n", NULL);
	if (tmp[0] != '\0')
	    fwrite (tmp, 1, strlen (tmp), sed_cmd_fl);
	STR_free (tmp);
	ccnt++;
    }
    fclose (fl);
    fclose (sed_cmd_fl);

    if (ccnt > 0) {
	char bb[BUF_SIZE];
	int n_bytes, ret;
	struct stat fst;

	stat (f_name, &fst);
	sprintf (buf, "%s > %s; cp -f %s %s\"", 
				f_name, tmp_fname, tmp_fname, f_name);
	cmd = STR_cat (cmd, buf);
	ret = MISC_system_to_buffer (cmd, bb, BUF_SIZE, &n_bytes);
	if (ret != 0) {
	    int l;
	    if (n_bytes > 0)
		LE_send_msg (0, "%ms", bb);
	    sprintf (err_msg, "Failed (%d) in modifying %s", ret, f_name);
	    l = strlen (cmd);
	    if (l > 200)
		cmd[200] = '\0';
	    LE_send_msg (0, "    cmd: %s", cmd);
	    Term_on_error (1);
	}
	chmod (f_name, fst.st_mode);
	LE_send_msg (0, "%d lines edited in %s", ccnt, f_name);
    }
    STR_free (cmd);
    STR_free (f_name);
}

/**************************************************************************

    Runs "script".

**************************************************************************/

static void Run_script (char *script) {
    char bb[BUF_SIZE];
    int n_bytes, ret;
    struct stat fst;

    stat (script, &fst);
    chmod (script, fst.st_mode | 0111);
    ret = MISC_system_to_buffer (script, bb, BUF_SIZE, &n_bytes);
    chmod (script, fst.st_mode);
    if (n_bytes > 0) {
	LE_send_msg (0, "Script output:");
	LE_send_msg (0, "%ms", bb);
    }
    if (ret != 0) {
	sprintf (err_msg, "Failed in running %s", script);
	Term_on_error (1);
    }
    LE_send_msg (0, "Script %s executed", script);
}

/**************************************************************************

    Terminates this process on error.

**************************************************************************/

static void Term_on_error (int err) {
    Mount_media (1);
    if (strlen (err_msg) > 0) {
	LE_send_msg (0, "%s", err_msg);
	printf ("%s\n", err_msg);
    }
    printf ("restore_adapt failed - See restore_adapt log (lem restore_adapt)\n");
    exit (err);
}

/**************************************************************************

    Mounts the media and resets "Src_dir". It umounts the media if "unmount"
    is non-zero.

**************************************************************************/

static void Mount_media (int unmount) {
    static int media_mounted = 0;
    char *device, cmd[CMD_SIZE], buf[BUF_SIZE];
    int ret, n_bytes;

    if ((unmount && !media_mounted) || (!unmount && media_mounted))
	return;

    if (From_cd)
	device = "cd";
    else if (From_cdrom)
	device = "cdrom";
    else
	device = "floppy";
    if (Interactive && !unmount) {
	printf ("--->Restoring from %s\n", device);
	printf ("--->Insert the adaptation backup %s into the drive\n", device);
	printf ("--->Hit return when ready\n");
	Get_stdin ();
	printf ("--->Restoring adaptation data from %s\n", device);
    }

    if (unmount) {
	LE_send_msg (0, "Unmount %s ...", device);
	sprintf (cmd, "medcp -u -l restore_adapt %s", device);
    }
    else {
	LE_send_msg (0, "Mount %s ...", device);
	sprintf (cmd, "medcp -mp -l restore_adapt %s", device);
    }
    n_bytes = 0;
    buf[0] = '\0';
    ret = ( MISC_system_to_buffer (cmd, buf, BUF_SIZE, &n_bytes) >> 8 );
    if (n_bytes > 0)
	LE_send_msg (0, "%ms", buf);
    if (ret != 0) {
	sprintf (err_msg, "\"%s\" failed (%d)\n", cmd, ret);
	Term_on_error (ret);
    }
    if (unmount)
	media_mounted = 0;
    else {
	char b[128];
	media_mounted = 1;
	MISC_get_token (buf, "", 0, b, 128);
	Src_dir = STR_copy (NULL, b);
    }
}

/*******************************************************************

    Reads standard input up to 256 characters. It discards any 
    remaining characters in the buffer before reading.

*******************************************************************/

static char *Get_stdin () {
    static char buf[256];

    fseek (stdin, 0, 2);
    buf[0] = '\0';
    fgets (buf, 256, stdin);
    return (buf);
}

static int Read_password (char *buf, int size) {

    struct termios attr;
    int n, term_set;

    lseek (STDIN_FILENO, 0, SEEK_END);
    term_set = 1;
    if (tcgetattr (STDIN_FILENO, &attr) != 0) {
	LE_send_msg (0, "tcgetattr failed - we don't disable echo");
	term_set = 0;
    }
    if (term_set) {
	printf ("Password: ");
	fflush (stdout);
	attr.c_lflag &= ~(ECHO);
	sigset (SIGINT, SIG_HOLD);
	if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &attr) != 0) {
	    sprintf (err_msg, "tcsetattr TCSAFLUSH failed\n");
	    Term_on_error (1);
	}
    }
    else {		/* runs as the coprocessor */
	printf ("Password: \n");
	fflush (stdout);
    }
    n = read (STDIN_FILENO, buf, size);
    buf[n - 1] = '\0';
    if (n > 0 && buf[n - 1] == '\n')
 	buf[n - 1] = '\0';
    if (term_set) {
	attr.c_lflag |= ECHO;
	if (tcsetattr (STDIN_FILENO, TCSANOW, &attr) != 0) {
	    sprintf (err_msg, "tcsetattr TCSANOW failed\n");
	    Term_on_error (1);
	}
	sigset (SIGINT, SIG_DFL);
    }
    return (n);
}

/*******************************************************************

    Asks the user to type in the root password and verified it. Returns
    0 on success or -1 on failure.

*******************************************************************/

static int Varifies_root () {
    static struct pam_conv conv = {Get_root_passwd, NULL};
    pam_handle_t *pamh = NULL;
    int retval;

    retval = pam_start ("passwd", "root", &conv, &pamh);

    if (retval == PAM_SUCCESS) {
	retval = pam_authenticate (pamh, 0);
	pam_end (pamh, 0);
	if (retval == PAM_SUCCESS)
	    return (0);
	else {
	    LE_send_msg (0, "AUTHENTICATION FAILED: Invalid password");
	    return (-1);
	}
    }
    else {
	LE_send_msg (0, "AUTHENTICATION FAILED: Pam module error");
	return (-1);
    }
}

/**************************************************************************

    Prompt user for root password. This function is called by the Linux-PAM 
    module.

***************************************************************************/

static int Get_root_passwd (int num_msg, const struct pam_message **msg,
                     struct pam_response **resp, void *appdata_ptr) {
    int count;
    struct pam_response *r;
    char *passwd = (char *)MISC_malloc (MAX_PASSWD_LENGTH);
  
    r = (struct pam_response *) calloc (num_msg, sizeof (struct pam_response));
    for (count = 0; count < num_msg; count++) {
	switch (msg[count]->msg_style) {

	    case PAM_PROMPT_ECHO_OFF:
		Read_password (passwd, MAX_PASSWD_LENGTH);
		break;
	    default:
		strcpy (passwd, "bad password");
		break;
	}
	r[count].resp_retcode = 0;
	r[count].resp = passwd;
    }
    *resp = r;
    return PAM_SUCCESS;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "D:o:d:t:s:H:I:ricfnh?")) != EOF) {
	switch (c) {

	    case 'D':
		Src_dir = optarg;
		break;

	    case 'o':
		Spec_channel = optarg;
		break;

	    case 'd':
		Spec_date = optarg;
		break;

	    case 't':
		Spec_time = optarg;
		break;

	    case 'I':
		Test_install_dest = optarg;
		break;

	    case 'c':
		From_cd = 1;
		break;

	    case 'f':
		From_floppy = 1;
		break;

	    case 'r':
		From_cdrom = 1;
		break;

	    case 'i':
		Install_config = 1;
		break;

	    case 'n':
		Interactive = 0;
		break;

	    case 's':
	    case 'H':
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
    Restores the RPG adaptation data by searching an archive file in the \n\
    specified local directory, or on the local CD or floppy, and putting it\n\
    in $CFG_DIR/adapt on the specified mrpg host. All existing archive files\n\
    in $CFG_DIR/adapt and $CFG_DIR/adapt/installed are removed. This command\n\
    can be invoked on any of the connected nodes (mscf, rpg1 etc).\n\
    Options:\n\
	-D dir (Specifies the directory where the RPG adapt archive files are\n\
	   stored. Default: The current directory.)\n\
	-c (Restore adapt from CD)\n\
	-f (Restore adapt from floppy)\n\
	-r (Restore adapt from CDROM)\n\
	-o channel (rpg1 or rpg2. Default: The RPG channel on the local host.\n\
	   If the local host is not an RPG node, rpg1 is assumed.)\n\
	-d date (The date, in format mm/dd/yyyy, of the archive to restore.\n\
		 Default: Todays date.)\n\
	-t time (The time, in format hh:mm:ss, of the archive to restore.\n\
		 Default: The current time.)\n\
	   Note: If date is specified but time is not, the latest archive of\n\
		 the specified date is restored. Otherwise, the archive\n\
		 closest to the specified time is restored.\n\
	-n (Non-interactive mode)\n\
	-i (Installs site configuration files)\n\
	-I dest (Test install site configuration files from \"dir\" to \"dest\")\n\
	-h (Print usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}


