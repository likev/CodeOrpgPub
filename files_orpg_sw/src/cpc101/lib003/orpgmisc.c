/**************************************************************************

      Module:  orpgmisc.c

 Description:
        This file provides ORPG library routines for performing miscellaneous
        duties.

        Functions that are public are defined in alphabetical order at the
        top of this file and are identified by a prefix of "ORPGMISC_".

        The scope of all other routines defined within this file is
        limited to this file.  The private functions are defined in
        alphabetical order, following the definitions of the API functions.

 Interruptible System Calls:
	TBD

 Memory Allocation:
	None

 Assumptions:
	TBD

 **************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/02 15:40:29 $
 * $Id: orpgmisc.c,v 1.90 2013/07/02 15:40:29 steves Exp $
 * $Revision: 1.90 $
 * $State: Exp $
 */  

/*	System include files.						*/
#include <unistd.h>            /* getopt  */
#include <stdlib.h>            /* getenv(), malloc(), free()              */
#include <string.h>            /* strcat()                                */
#include <ctype.h>            /* toupper()                                */

#include <sys/stat.h>          /* stat()                                  */
#include <sys/types.h>         /* stat()                                  */

/*  Local include files */

#include <orpg.h>
#include <rdacnt.h>
#include <gen_stat_msg.h>
#include <itc.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

int low_bandwidth_flag=FALSE;		/*  TRUE if this app is running in low bandwidth mode */
static int initialized_deau = 0;
static int RPG_build_number = -1;
static int Adapt_version_number = 0;

static void Le_callback (char *msg, int msgsz);
static void Convert_to_upper (char *str);
static void Read_rpg_build_number ();
static void Get_system_type (char *system);
static int Read_dea (char *dea_id, char *dea_type, char **p);


/***********************************************************************

    The ORPG init function. It calls other init f(x)s. This allows
    developers to only make one call to init, instead of several
    individual calls.

    Inputs:	argc0 - argv[0]
		n_msgs - le LB size. If 0, 100 is used.
		lb_type - LB types. If 0, default LB types are used.
		instance - instance number. Must be -1 if no instance.
		no_system_log - If non-zero, no message is sent to the
				system LOG.

    Returns 0 on success or -1 on failure.

***********************************************************************/

int ORPGMISC_init (int argc, char *argv[], int n_msgs, 
		   int lb_type, int instance, int no_system_log)
{
    int rc = 0;

    rc = ORPGMISC_LE_init( argc, argv, n_msgs, lb_type, instance, no_system_log );
    if( rc < 0 ){ return rc; }
    rc = ORPGMISC_deau_init();
    if( rc < 0 ){ return rc; }

    return ( 0 );
}

/******************************************************************

    Sends each line in "msg" using an individual LE_send_msg call.
	
******************************************************************/

void ORPGMISC_send_multi_line_le (int code, char *msg) {
    char *p, *next;

    p = msg;
    while (1) {
	next = p;
	while (*next != '\0' && *next != '\n')
	    next++;
	if (*next == '\n') {
	    *next = '\0';
	    next++;
	}
	if (p == msg)
	    LE_send_msg (code, "%s", p);
	else
	    LE_send_msg (code, "    %s", p);
	p = next;
	if (*p == '\0')
	    return;
    }
}

/***********************************************************************

    Returns if ORPG is an operational execution.

***********************************************************************/

int ORPGMISC_is_operational () {
    if (getenv ("ORPG_NONOPERATIONAL") != NULL)
	return (0);
    return (1);
}

/***********************************************************************

    The ORPG deau init function. It initializes the LB name that
    DEAU functions use to get/set adaptation data. If an application
    uses DEAU_ functions, but doesn't register a callback, then this
    function must be called.

    Inputs:	none

    Returns 0 on success or exits on failure.

***********************************************************************/

int ORPGMISC_deau_init()
{
  if( !initialized_deau )
  {
    char *ac_ds_name;
    ac_ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA );
    if( ac_ds_name == NULL )
    {
      LE_send_msg( GL_ERROR, "ORPGADPT: ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA );
      exit( 1 );
    }
    DEAU_LB_name( ac_ds_name );
    initialized_deau = 1;
  }
  return 0;
}

/***********************************************************************

    The old ORPG LE init function.

***********************************************************************/

int ORPGMISC_le_init (int argc, char **argv, int instance) {

    return (ORPGMISC_LE_init (argc, argv, 100, 0, instance, 0));
}

/***********************************************************************

    The ORPG LE init function. It creates the process log LB if it does 
    not exist or has different attributes. It then calls LE_init and
    registers the RPG LE callback function for sending critical LE msgs
    to the RPG log file.

    Inputs:	argc0 - argv[0]
		n_msgs - le LB size. If 0, 100 is used.
		lb_type - LB types. If 0, default LB types are used.
		instance - instance number. Must be -1 if no instance.
		no_system_log - If non-zero, no message is sent to the
				system LOG.

    Returns 0 on success or -1 on failure.

***********************************************************************/

#define ORPGMISC_NAME_SIZE 128

int ORPGMISC_LE_init (int argc, char *argv[], int n_msgs, 
		int lb_type, int instance, int no_system_log) {
    int retval;
    char task_name[ ORPG_TASKNAME_SIZ ];

    retval = 0;
    ORPGDA_set_event_msg_byteswap_function ();

    ORPGTAT_set_args( argc, argv );

    /* Create the log error LB. */
    LE_set_option ("LB size", n_msgs);
    LE_set_option ("LB type", lb_type);
    LE_set_option ("LE disable", 1);
    if( ORPGTAT_get_my_task_name (task_name, ORPG_TASKNAME_SIZ) >= 0 ) {
       LE_set_option ("LE name", task_name);
       LE_set_option ("label", task_name);
    }
    LE_set_option ("LE disable", 0);

    if( LE_init( argc, argv ) < 0 ){

        LE_send_msg (GL_INFO, "ORPGMISC: LE_init failed\n") ;
        retval = -1;

    }

    /* Callback function registration only necessary if messages
       can go to system status log. */
    if (!no_system_log)
	LE_set_callback (Le_callback);
    return (retval);
}

/***********************************************************************

    The RPG LE callback function. It sends critical messages to the 
    RPG system log file.

***********************************************************************/

static void Le_callback (char *msg, int msgsz) {
    static int dont_write = 0, global_error_log = 1;

    /* Controls writing of messages to system status log.  If writing
       to system status log disabled, just return.  Note:  Changed 
       "if( dont_write )" check to "if( 1 )":  Steve Smith 8/13/07. */
    if (1) {
	int ret;
	LE_message *le_msg_p;

	le_msg_p = (LE_message *)msg;
	if (!(le_msg_p->code & GL_CRIT_BIT))
	    return;

	if (global_error_log && (le_msg_p->code & GL_ERROR_BIT)) {
	    LE_set_option ("LE disable", 1);
	    if ((ret = ORPGDA_write (ORPGDAT_ERRLOG, 
				(char *)msg, msgsz, LB_ANY)) < 0) {
		LE_send_msg (0, 
"ORPGDA_write ORPGDAT_ERRLOG failed (ret %d) - RPG error log disabled", ret);
		global_error_log = 0;
	    }
	    LE_set_option ("LE disable", 0);
	}

	/* We process only GLOBAL and STATUS messages */
	if (!(le_msg_p->code & (GL_GLOBAL_BIT | GL_STATUS_BIT)))
            return;

	if ((ret = ORPGDA_write (ORPGDAT_SYSLOG, (char *)msg, msgsz, 
                                 LB_ANY)) < 0) {

	    LE_send_msg (0, 
                   "ORPGDA_write ORPGDAT_SYSLOG failed (ret %d)", ret);
	    dont_write = 1;

	}
        else {

	    ORPGDA_write (ORPGDAT_SYSLOG_SHADOW, (char *)msg, msgsz, 
                          LB_ANY );

	}

     }

}

/***************************************************************************

    Returns the RPG build number.

***************************************************************************/

int ORPGMISC_RPG_build_number () {
    if (RPG_build_number < 0)
	Read_rpg_build_number ();
    return (RPG_build_number);
}

/***************************************************************************

    Returns the RPG adaptation data version number.

***************************************************************************/

int ORPGMISC_RPG_adapt_version_number () {
    if (RPG_build_number < 0)
	Read_rpg_build_number ();
    return (Adapt_version_number);
}

/***************************************************************************

    Reads the RPG build number and the adaptation data version number.

***************************************************************************/

#define LOC_BUF_SIZE 256

static void Read_rpg_build_number () {
    char buf[LOC_BUF_SIZE], *fname;
    FILE *fl;
    int mj, mi, cnt;

    RPG_build_number = Adapt_version_number = 0;
    if (MISC_get_cfg_dir (buf, LOC_BUF_SIZE) < 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed");
	return;
    }
    fname = STR_gen (NULL, buf, "/version_rpg", NULL);
    fl = fopen (fname, "r");
    if (fl == NULL) {
	LE_send_msg (GL_INFO, "fopen %s failed", fname);
	STR_free (fname);
	return;
    }
    while (fgets (buf, LOC_BUF_SIZE, fl) != NULL) {
	char *p = buf;
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p == '#' || *p == '\0' || *p == '\n')
	    continue;
	if (sscanf (p, "%d%*c%d", &mj, &mi) != 2) {
	    LE_send_msg (GL_INFO, "RPG Build number not found in %s", fname);
	    break;
	}
	RPG_build_number = mj * 10 + mi;
	cnt = 0;
	while (*p != '\0') {
	    if (*p == ':')
		cnt++;
	    if (cnt == 4) {
		p++;
		break;
	    }
	    p++;
	}
	if (cnt != 4 ||
	    sscanf (p, "%d%*c%d", &mj, &mi) != 2) {
	    LE_send_msg (GL_INFO, 
			"RPG adapt version number not found in %s", fname);
	    break;
	}
	Adapt_version_number = mj * 10 + mi;
    }
    fclose (fl);
    STR_free (fname);
}

/***************************************************************************

    Returns the encryped string of string "str". There is no limit on the
    length of the input string. Returns NULL on failure.

***************************************************************************/

char *ORPGMISC_crypt (char *str) {
    static char *buf = NULL;
    int len, cnt;
    char b[10], *p;

    buf = STR_copy (buf, "");
    len = strlen (str);
    cnt = 0;
    while (cnt < len) {
	strncpy (b, str + cnt, 8);
	b[8] = '\0';
	cnt += 8;
	if ((p = crypt (b, "NX")) == NULL)
	    return (NULL);
	buf = STR_cat (buf, p + 2);
    }
    return (buf);
}

/***************************************************************************

    Returns the site name, node type, system type or channel number if
    "field" is respectively "site", "type", "system" or "channel_num".
    Returns NULL on failure. The site info is found in the file
    "rpg_install.info" or, if not available, the DEA DB (or file
    CFG_DIR/site_info.dea). If "field" is the empty string, all fields
    plus the info source is returned.

***************************************************************************/

#define RPGINSTALLINFO_FILE "rpg_install.info"

char *ORPGMISC_get_site_name (char *field) {
    static struct {
	char site[16];			/* e.g. KTLX */
	char type[16];			/* e.g. mscf, rpga, rpgb */
	char channel_num[4];		/* e.g. 1, 2 */
	char system[8];			/* NWS NWSR FAA DODFR DODAN */
    } site_info;
    static int initialized = 0;
    static char source[64];

    if (!initialized) {		/* find the info from RPG installation */
	char *p;
	char b[256], dea_type[4];
	int ret;

	LE_set_option ("LE disable", 1);
	memset (&site_info, 0, sizeof (site_info));
	strcpy (source, "");

	ret = ORPGMISC_get_install_info ("ICAO:", b, 256);
	if (ret >= 0) {
	    strncpy (site_info.site, b, 4);
	    site_info.site[4] = '\0';
	    Convert_to_upper (site_info.site);
	    strcat (source, "-site(I)");
	}
	if (site_info.site[0] == '\0') {	/* read from DEA */
	    if (Read_dea ("site_info.rpg_name", dea_type, &p) > 0) {
		strncpy (site_info.site, p, 16);
		site_info.site[15] = '\0';
		Convert_to_upper (site_info.site);
		sprintf (source + strlen (source), "-site(%s)", dea_type);
	    }
	}
	if (site_info.site[0] == '\0')
	    return (NULL);			/* site info not found */

	if (ORPGMISC_get_install_info ("TYPE:", b, 256) >= 0) {
	    if (strcasecmp (b, "rpg") == 0 ||
		strcasecmp (b, "rpga") == 0)
		strcpy (site_info.type, "rpga");
	    else if (strcasecmp (b, "bdds") == 0 ||
		strcasecmp (b, "rpgb") == 0)
		strcpy (site_info.type, "rpgb");
	    else if (strcasecmp (b, "rpgc") == 0)
		strcpy (site_info.type, "rpgc");
	    else if (strcasecmp (b, "mscf") == 0)
		strcpy (site_info.type, "mscf");
	    else
		LE_send_msg (GL_ERROR, "Unexpected install node type %s\n", b);
	}
	if (site_info.type[0] == '\0') {
	    strcpy (site_info.type, "rpga");
	    strcat (source, "-node(D)");
	}
	else
	    strcat (source, "-node(I)");

	strcpy (site_info.channel_num, "1");
	ret = ORPGMISC_get_install_info ("CHANNEL:", b, 256);
	if (ret == 0) {
	    int ch_n;
	    if (sscanf (b, "%d", &ch_n) != 1)
		LE_send_msg (GL_ERROR, "Unexpected install CHANNEL %s\n", b);
	    else {
		if (ch_n == 2)
		    strcpy (site_info.channel_num, "2");
		else if (ch_n != 1)
		    LE_send_msg (GL_ERROR,
			"Unexpected install CHANNEL number %d\n", ch_n);
	    }
	    strcat (source, "-chan(I)");
	}
	else if (ret == -1) {			/* install does not exist */
	    if (Read_dea ("Redundant_info.channel_number", dea_type, &p) > 0) {
		if (strcmp (p, "Channel 2") == 0)
		    strcpy (site_info.channel_num, "2");
		sprintf (source + strlen (source), "-chan(%s)", dea_type);
	    }
	    else
		strcat (source, "-chan(D)");
	}
	else
	    strcat (source, "-chan(D)");

	Get_system_type (site_info.system);
	if (site_info.system[0] == '\0') {
	    if (Read_dea ("Redundant_info.redundant_type", dea_type, &p) > 0) {
		if (strcmp (p, "FAA Redundant") == 0)
		    strcpy (site_info.system, "FAA");
		else if (strcmp (p, "NWS Redundant") == 0)
		    strcpy (site_info.system, "NWSR");
		else
		    strcpy (site_info.system, "NWS");
		sprintf (source + strlen (source), "-system(%s)", dea_type);
	    }
	}
	else
	    strcat (source, "-system(I)");
	LE_set_option ("LE disable", 0);

	initialized = 1;
    }

    if (field[0] == '\0') {
	static char buf[128];
	sprintf (buf, "SITE=%s NODE=%s SYSTEM=%s", site_info.site, 
			site_info.type, site_info.system);
	if (strcmp (site_info.system, "FAA") == 0)
	    sprintf (buf + strlen (buf), " chan %s", site_info.channel_num);
	sprintf (buf + strlen (buf), " (%s)", source); 
	return (buf);
    }

    /* for backward compatibily - will be phased out */
    if (strcmp (field, "is_redundant") == 0) {
	static char is_red[16];
	if (strcmp (site_info.system, "FAA") == 0)
	    strcpy (is_red, "yes");
	else if (strcmp (site_info.system, "NWSR") == 0)
	    strcpy (is_red, "nws_yes");
	else
	    strcpy (is_red, "no");
	return (is_red);
    }

    if (strcmp (field, "type") == 0 && strlen (site_info.type) > 0)
	return (site_info.type);
    else if (strcmp (field, "site") == 0 && strlen (site_info.site) > 0)
	return (site_info.site);
    else if (strcmp (field, "channel_num") == 0 && 
				strlen (site_info.channel_num) > 0)
	return (site_info.channel_num);
    else if (strcmp (field, "system") == 0 && strlen (site_info.system) > 0)
	return (site_info.system);
    return (NULL);
}

/***************************************************************************

    Initialize DEA access and reads "dea_id". The result is returned with
    "p". Data source type is put in "dea_type".

***************************************************************************/

static int Read_dea (char *dea_id, char *dea_type, char **p) {
    static int init = 0, use_ascii = 0;;

    if (init == 0) {
	char *name;

	use_ascii = 0;
	if (DEAU_get_string_values ("site_info.rpg_name", p) <= 0 &&
	    (name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA)) != NULL)
	    DEAU_LB_name (name);
	if (DEAU_get_string_values ("site_info.rpg_name", p) <= 0) {
	    char buf[256];
	    if (MISC_get_cfg_dir (buf, 200) >= 0) {
		strcat (buf, "/site_info.dea");
		DEAU_use_attribute_file (buf, 0);
		use_ascii = 1;
	    }
	}
	init = 1;
    }
    if (use_ascii)
	strcpy (dea_type, "A");
    else
	strcpy (dea_type, "B");

    return (DEAU_get_string_values (dea_id, p));
}

/***************************************************************************

    Finds the system type by reading the site variables.

***************************************************************************/

static void Get_system_type (char *system) {
    int ret;
    char value[LOC_BUF_SIZE];

    ret = ORPGMISC_get_site_value ("FAA", value, LOC_BUF_SIZE);
    if (ret < 0 &&
	!(strstr (value, "Variable") != NULL && 
	  strstr (value, "not found") != NULL)) {
	return;
    }
    if (ret == 0 && strcmp (value, "YES") == 0) {
	strcpy (system, "FAA");
	return;
    }

    ret = ORPGMISC_get_site_value ("DOD", value, LOC_BUF_SIZE);
    if (ret == 0 && strcmp (value, "YES") == 0) {
	if (ORPGMISC_get_site_value ("ADMSCF", value, LOC_BUF_SIZE) >= 0 &&
	    strcmp (value, "YES") == 0)
	    strcpy (system, "DODAN");
	else
	    strcpy (system, "DODFR");
	return;
    }

    if (ORPGMISC_get_site_value ("NWS_RED", value, LOC_BUF_SIZE) >= 0 &&
	strcmp (value, "YES") == 0)
	strcpy (system, "NWSR");
    else
	strcpy (system, "NWS");
}

/***************************************************************************

    Finds the value of varible "variable" and returns it in buffer "value"
    of "v_size" bytes. The default site file used is "$CFG_DIR/site_data"
    if not specified with "variable" as "variable@site_file". Retuns 0 on
    success or -1 on failure in which case an error message is put in
    "value".

***************************************************************************/

int ORPGMISC_get_site_value (char *variable, char *value, int v_size) {
    FILE *fl;
    char var_file[LOC_BUF_SIZE], var[LOC_BUF_SIZE];

    if (MISC_get_token (variable, "S@", 1, var_file, LOC_BUF_SIZE) <= 0) {
	char cfgdir[LOC_BUF_SIZE];
	int ret = MISC_get_cfg_dir (cfgdir, LOC_BUF_SIZE);
	if (ret <= 0) {
	    snprintf (value, v_size, "MISC_get_cfg_dir failed");
	    return (-1);
	}
	snprintf (var_file, LOC_BUF_SIZE, "%s/site_data", cfgdir);
    }
    if (MISC_get_token (variable, "S@", 0, var, LOC_BUF_SIZE) <= 0) {
	snprintf (value, v_size, "Empty variable name");
	return (-1);
    }

    if ((fl = fopen (var_file, "r")) != NULL) {
	char buf[LOC_BUF_SIZE], comment_char;
	int started, cnt;

	comment_char = ' ';
	started = 0;
	cnt = 0;
	while (fgets (buf, LOC_BUF_SIZE, fl) != NULL) {
	    char *p, *key, tok[LOC_BUF_SIZE];

	    p = buf + MISC_char_cnt (buf, " \t");
	    if (started && *p != comment_char)
		break;
	    if (!started && cnt > 20)
		break;
	    cnt++;
	    p++;
	    p = p + MISC_char_cnt (p, " \t");
	    key = "Variables set to \"YES\"";
	    if (strncmp (p, key, strlen (key)) == 0) {
		int ind;

		started = 1;
		comment_char = *(buf + MISC_char_cnt (buf, " \t"));
		ind = 4;
		while (MISC_get_token (p, "", ind, tok, LOC_BUF_SIZE) > 0) {
		    if (strcmp (tok, var) == 0) {
			snprintf (value, v_size, "YES");
			fclose (fl);
			return (0);
		    }
		    ind++;
		}
	    }
	    else if (started &&
		     MISC_get_token (p, "", 1, tok, LOC_BUF_SIZE) > 0 &&
		     strcmp (tok, "=") == 0) {
		if (MISC_get_token (p, "", 0, tok, LOC_BUF_SIZE) > 0 &&
		    strcmp (tok, "SITE_NAME") == 0 &&
		    (strcmp (var, "SITE_NAME") == 0 ||
		     strcmp (var, "site_name") == 0)) {
		    char *v;

		    if (MISC_get_token (p, "", 5, tok, LOC_BUF_SIZE) > 0 ||
			MISC_get_token (p, "", 2, tok, LOC_BUF_SIZE) > 0) {
			v = tok;
			if (strcmp (var, "SITE_NAME") == 0) 
			    MISC_toupper (v);
		    }
		    else 
			v = "";
		    snprintf (value, v_size, "%s", v);
		    fclose (fl);
		    return (0);
		}
		else {
		    MISC_get_token (p, "", 0, tok, LOC_BUF_SIZE);
		    if (strcmp (tok, var) == 0) {
			if (MISC_get_token (p, "S=", 1, tok, LOC_BUF_SIZE) > 0)
			    snprintf (value, v_size, "%s", tok);
			else
			    snprintf (value, v_size, "%s", "");
			fclose (fl);
			return (0);
		    }
		}
	    }
	}
	if (!started) {
	    snprintf (value, v_size, "File %s does not have site info", var_file);
	    fclose (fl);
	    return (-1);
	}
	fclose (fl);
	snprintf (value, v_size, "Variable %s not found", var);
    }
    else
	snprintf (value, v_size, "File %s not found", var_file);

    return (-1);
}

/***************************************************************************

    Reads the RPG installation info file "RPGINSTALLINFO_FILE" and returns
    the value after tag "tag". The value is the token after the tag. The
    value is return in "buf" of "buf_size". If "buf_size" is too small,
    the value is truncated. The returned value is always null terminated.
    Returns 0 on success, -2 if the tag is not found or -1 on failure.

***************************************************************************/

int ORPGMISC_get_install_info (char *tag, char *buf, int buf_size) {
    char name[256], line[256], tok[256];
    FILE *fl;
    int found;

    if (MISC_full_path (getenv ("CFG_DIR"), 
				RPGINSTALLINFO_FILE, name, 256) == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGMISC_get_install_info: CFG_DIR not defined");
	return (-1);
    }

    fl = fopen (name, "r");
    if (fl == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGMISC_get_install_info: Failed opening %s", name);
	return (-1);
    }
    found = 0;
    while (fgets (line, 256, fl) != NULL) {
	if (MISC_get_token (line, "", 0, tok, 256) <= 0)
	    continue;
	if (strcmp (tok, tag) == 0) {
	    found = 1;
	    break;
	}
    }
    fclose (fl);
    if (!found) {
    	LE_send_msg (GL_ERROR, 
		"ORPGMISC_get_install_info: Tag \"%s\" not found", tag);
	return (-2);
    }
    if (MISC_get_token (line, "", 1, tok, 256) <= 0)
	buf[0] = '\0';
    else {
	strncpy (buf, tok, buf_size);
	buf[buf_size - 1] = '\0';
    }
    return (0);
}
 
/***************************************************************************

    Writes the RPG installation info file "RPGINSTALLINFO_FILE" for
    setting the value after tag "tag" to "value". The value is the token
    after the tag. "tag" and "value" must be a null terminated strings.
    Returns 0 on success or -1 on failure.

***************************************************************************/

int ORPGMISC_set_install_info (char *tag, char *value) {
    char name[256], line[256], tok[256];
    FILE *fl;
    char *vbuf;
    int found;

    if (MISC_full_path (getenv ("CFG_DIR"), 
				RPGINSTALLINFO_FILE, name, 256) == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGMISC_set_install_info: CFG_DIR not defined");
	return (-1);
    }

    fl = fopen (name, "r+");
    if (fl == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGMISC_set_install_info: Failed opening %s", name);
	return (-1);
    }
    found = 0;
    vbuf = STR_reset (NULL, 512);
    while (fgets (line, 256, fl) != NULL) {
	int len, off, ret;
	char *st, *end;

	len = strlen (line);
	off = STR_size (vbuf);
	vbuf = STR_append (vbuf, line, len);
	if ((ret = MISC_get_token (line, "", 0, tok, 256)) > 0 &&
					strcmp (tok, tag) == 0) {
	    st = line + ret;
	    st += MISC_char_cnt (st, " \t");
	    end = st + MISC_char_cnt (st, "\0 \t\n");
	    if (st[-1] != ' ' && st[-1] != '\t') {
		vbuf = STR_replace (vbuf, off + (st - line), 0, " ", 1);
		st++;
		end++;
	    }
	    vbuf = STR_replace (vbuf, off + (st - line), end - st, 
						    value, strlen (value));
	    found = 1;
	}
    }
    if (!found) {
	if (*(vbuf + STR_size (vbuf) - 1) != '\n')
	    vbuf = STR_append (vbuf, "\n", 1);
	vbuf = STR_append (vbuf, tag, strlen (tag));
	vbuf = STR_append (vbuf, " ", 1);
	vbuf = STR_append (vbuf, value, strlen (value));
	vbuf = STR_append (vbuf, "\n", 1);
    }

    fseek (fl , 0, SEEK_SET);
    if (fwrite (vbuf, 1, STR_size (vbuf), fl) != STR_size (vbuf)) {
	LE_send_msg (GL_ERROR, "fwrite %s failed", name);
	fclose (fl);
	return (-1);
    }
    ftruncate (fileno (fl), STR_size (vbuf));
    fclose (fl);
    return (0);
}

/*************************************************************************

    Converts "str" to upper case.
    
**************************************************************************/

static void Convert_to_upper (char *str) {
    char *p;
    p = str;
    while (*p != '\0') {
	*p = toupper (*p);
	p++;
    }
}

/**************************************************************************
 Description: Returns TRUE if this app is running in low bandwidth mode
       Input: none
      Output: none
     Returns: none
       Notes:
 **************************************************************************/
int ORPGMISC_is_low_bandwidth()
{
	return(low_bandwidth_flag);
}


/**************************************************************************
 Description: Sets the compression for a data source if we are in low bandwidth mode
       Input: none
      Output: none
     Returns: none
       Notes:
 **************************************************************************/
void ORPGMISC_set_compression(int data_id)
{
	int lbfd; 
	if (low_bandwidth_flag)
	{
	      lbfd = ORPGDA_lbfd(data_id);
	      if (lbfd >= 0)
		  RSS_LB_compress(lbfd, TRUE);
	}
}

/**************************************************************************
 Description: Read the command-line options
       Input: argc,argv
       	      -l remote_flag
      Output: none
     Returns: 1 if successful; 0 if successful, but do not run program, and
     		-1 if there is an error
       Notes:
 **************************************************************************/
int ORPGMISC_read_options(int argc,char **argv)
{
   int input;
   int retval = 1;

   /*
    * Establish some defaults that may be overridden ...
    */
   low_bandwidth_flag = 0;

   while ((input = getopt(argc,argv,"lh")) != -1) 
   {
      switch(input) 
      {
         case 'h':
		retval = 0;
		fprintf(stderr,"options:\n");
		fprintf(stderr,"-h this help message\n");
		fprintf(stderr,"-l low bandwidth mode\n");
		break ;
         case 'l':
	    	low_bandwidth_flag = TRUE;
      		break ;
      }
   }
   return(retval);
/*END of ORPGMISC_Read_options()*/
}

/**************************************************************************
 Description: Return a flag that indicates whether or not RPG Status is
              matches the specified status value
       Input: RPG Status value
      Output: none
     Returns: '1' if current RPG Status matches argument
              '0' if current RPG Status does not match argument
       Notes:

              Convenience macros have been provided.

 **************************************************************************/
unsigned char
ORPGMISC_is_rpg_status(int check_status)
{
    int retval ;
    unsigned int rpg_status ;
    char *status_string_p ;

    retval = ORPGINFO_statefl_rpg_status(ORPGINFO_STATEFL_GET,
                                         ORPGINFO_STATEFL_RPG_STATUS_CUR,
                                         &rpg_status, &status_string_p) ;
    if (retval < 0) {
        LE_send_msg(GL_ERROR,
        "ORPGINFO_statefl_rpg_status(_GET, _RPG_STATUS_CUR failed: %d",
                    retval) ;
        return(0) ;
    }

    /*
     * It would have been simpler to use the ORPGINFO_STATEFL_RPGSTAT_
     * macros, but I chose not to reference those macros in orpgmisc.h ...
     */
    if (check_status == ORPGMISC_IS_RPG_STATUS_OPER) {
        if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_OPERATE) {
            return(1) ;
        }
        else {
            return(0) ;
        }
    }
    else if (check_status == ORPGMISC_IS_RPG_STATUS_RESTART) {
        if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_RESTART) {
            return(1) ;
        }
        else {
            return(0) ;
        }
    }
    else if (check_status == ORPGMISC_IS_RPG_STATUS_STANDBY) {
        if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_STANDBY) {
            return(1) ;
        }
        else {
            return(0) ;
        }
    }
    else if (check_status == ORPGMISC_IS_RPG_STATUS_TEST) {
        if (rpg_status == ORPGINFO_STATEFL_RPGSTAT_TEST) {
            return(1) ;
        }
        else {
            return(0) ;
        }
    }
    else {
        LE_send_msg(GL_ERROR,"Unrecognized check_status %d", check_status) ;
        return(0) ;
    }


/*END of ORPGMISC_is_rpg_status()*/
}


/*	The following function returns the ORPG endian type.		*/

int ORPGMISC_system_endian_type ()
{
	int	status;
	int	endian;
	
	/*  Initialize the RPG Endian Value data source */
	ORPGMISC_set_compression(ORPGDAT_RPG_INFO);

	status = ORPGDA_read (ORPGDAT_RPG_INFO,
			(char *) &endian,
			sizeof (endian),
			ORPGINFO_ENDIANVALUE_MSGID);

	if (status <= 0) {

	    LE_send_msg (GL_ORPGDA(status),
		"ORPGDA_read(_RPG_INFO, _ENDIANVALUE_MSGID) failed: %d\n",
		status);

	    return (-1);

	}

	return endian;
}

/*	The following function returns the endian type of local system	*/

int ORPGMISC_local_endian_type ()
{
	int	num;
	char	*buf;

	buf = (char *) &num;
	num = 1;

	if ((int) buf [0] == 0) {

	    return ORPG_BIG_ENDIAN;

	} else {

	    return ORPG_LITTLE_ENDIAN;

	}
}

/*	The following module swaps the bytes/words in the specified	*
 *	ORPG data store.  The data store names should match those	*
 *	used to identify the item in ORPGDA_read operations.		*/

int ORPGMISC_change_endian_type ( char	*buf, int type )
{
	int	i;
	char	ctemp;
	short	stemp;
	int	status;
	short	*word;
	A3cd97	*wind;

	switch (type) {

	    case A3CD97 :	/* Environmental wind data (itc.h)	*/

/*	    First swap bytes since all members are not char	*/

		for (i=1;i<sizeof (A3cd97);i=i+2) {

		    ctemp     = buf [i];
		    buf [i]   = buf [i-1];
		    buf [i-1] = ctemp;

		}

/*		Next, swap halfwords for all int and float members	*/

		word = (short *) buf;

		for (i=1;i<=(NPARMS*LEN_EWTAB);i=i+2) {

		    stemp      = word [i];
		    word [i]   = word [i-1];
		    word [i-1] = stemp;

		}

/*		The last item is the time (int). Swap halfwords.	*/

		wind = (A3cd97 *) buf;
		word = (short *) wind->sound_time;
		stemp    = word [1];
		word [1] = word [0];
		word [0] = stemp;
		status = 0;

		break;

	    default :

		LE_send_msg (GL_ERROR,
			"ORPGMISC_change_endian_type: Byte swapping not supported for type %d\n",
			type);
		status = 1;
		break;

	}

	return status;
}

/******************************************************************************

   Description:
      Converts volume scan sequence number to volume scan number.  Volume
      scan sequence number is monotonically increasing whereas volume
      scan number is in the range 1 - MAX_SCAN_SUM_VOLS.

   Inputs:
      vol_seq_num - volume scan sequence number.

   Outputs:
      
   Returns:
      Returns the volume scan number.

******************************************************************************/
int ORPGMISC_vol_scan_num( unsigned int vol_seq_num ){

   int vol_scan_num;
 
   vol_scan_num = vol_seq_num % MAX_SCAN_SUM_VOLS;
   if( vol_scan_num == 0 && vol_seq_num != 0 )
      vol_scan_num = MAX_SCAN_SUM_VOLS;

   return( vol_scan_num );

/* End of ORPGMISC_vol_scan_num() */ 
}

/******************************************************************************

   Description:
      Converts volume scan number to volume scan sequence number.  Volume
      scan sequence number is monotonically increasing whereas volume
      scan number is in the range 1 - MAX_SCAN_SUM_VOLS.

   Inputs:
      vol_quotient - volume scan quotient. 
      vol_scan_num - volume scan number.

   Outputs:
      
   Returns:
      Returns the volume scan sequence number.

******************************************************************************/
unsigned int ORPGMISC_vol_seq_num( int vol_quotient, int vol_scan_num ){

   unsigned int vol_seq_num;
 
   vol_seq_num = vol_quotient*MAX_SCAN_SUM_VOLS + 
                 (vol_scan_num % MAX_SCAN_SUM_VOLS);

   return( vol_seq_num );

/* End of ORPGMISC_vol_seq_num() */ 
}

#define MAX_REQ_PER_PROD           2
#define MIN_NUM_PRODUCTS          200

/***********************************************************************

   Description:
      This function estimates the maximum number of products which could
      be generated per volume scan, any volume scan.

   Returns:
      The maximum number of products on success or MIN_NUM_PRODUCTS on
      error.

   Notes:
      The following assumptions are made:

         1) If volume-based product, only 1 product per volume unless
            the product has customizing data.  In this case, up to 
            MAX_REQ_PER_PROD can be generated.  (MAX_REQ_PER_PROD 
            is a compromise between how many could be generated,
            usually 10 at time, and how many are likely to be 
            generated.)

         2) If elevation-based product, if only one product dependent
            parameter, assume ECUTMAX number of products per volume.
            If customizing data, ECUTMAX*MAX_REQ_PER_PROD can be 
            generated.

         3) All other product types, assume 1 product per volume.

***********************************************************************/
int ORPGMISC_max_products(){

   int num_prods = 0;
   int error = 0;
   int count = 0;
   int indx, type, prod_id, num_params;
   int pcode, warehoused;
   

   /* Determine the number of products in the product attribute table. */
   num_prods = ORPGPAT_num_tbl_items();
   if( num_prods <= 0 ){

      LE_send_msg( GL_ERROR, "ORPGPAT_num_tbl_items() Failed (%d)\n",
                   num_prods );
      return( MIN_NUM_PRODUCTS );

   }

   /* Determine the number of products which can be generated during the 
      course of a volume scan.  */
   for( indx = 0; indx < num_prods; indx++ ){

      prod_id = ORPGPAT_get_prod_id( indx );
      if( prod_id <= ORPGPAT_ERROR ){

         LE_send_msg( GL_ERROR, "ORPGPAT_get_prod_id(%d) Returned %d\n",
                      indx, prod_id );

         error = 1;
         break;
      }

      /* If product doesn't have a product code and the product is not
         warehoused, do not include in the count. */
      pcode = ORPGPAT_get_code( prod_id );
      warehoused = ORPGPAT_get_warehoused( prod_id );
      if( (pcode <= 0) && (warehoused <= 0) )
         continue;
    
      type = ORPGPAT_get_type( prod_id );
      if( type < 0 ){

         LE_send_msg( GL_ERROR, "ORPGPAT_get_type(%d) Returned %d\n",
                      indx, type );
         error = 1;
         break;

      }

      switch( type ){

         case TYPE_VOLUME:
            num_params = ORPGPAT_get_num_parameters( prod_id );
            if( num_params <= 0 )
               count++;
            else
               count += MAX_REQ_PER_PROD;

            break;

         case TYPE_ELEVATION:
            num_params = ORPGPAT_get_num_parameters( prod_id );
            if( num_params <= 1 )
               count += ECUT_UNIQ_MAX;
            else
               count += (ECUT_UNIQ_MAX*MAX_REQ_PER_PROD);

            break;

         default:
            count++;
            break;

      /* End of "switch" */
      }

   /* End of "for" loop */
   }

   if( error )
      return( MIN_NUM_PRODUCTS );

   return( count );

}

/****************************************************************
                                                                                                                   
   Description:
      Packs 4 bytes pointed to by "value" into 2 unsigned shorts.
      "value" can be of any type.  The address where the 4 bytes
      starting at "value" will be stored starts @ "loc".  
                                                                                                                   
      The Most Significant 2 bytes (MSW)  of value are stored at 
      the byte addressed by "loc", the Least Significant 2 bytes 
      (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where to store value. 
      value - pointer to data value.
                                                                                                                   
   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.
                                                                                                                   
   Notes:

****************************************************************/
int ORPGMISC_pack_ushorts_with_value( void *loc, void *value ){

   unsigned int   fw_value = *((unsigned int *) value);
   unsigned short hw_value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   hw_value = (unsigned short) (fw_value >> 16) & 0xffff;
   *msw = hw_value;

   hw_value = (unsigned short) (fw_value & 0xffff);
   *lsw = hw_value;

   return 0;

/* End of ORPGMISC_pack_ushorts_with_value() */
}
 
/****************************************************************

   Description:
      Unpacks the data value @ loc.  The unpacked value will be 
      stored at "value". 

      The Most Significant 2 bytes (MSW) of the packed value are
      stored at the byte addressed by "loc", the Least Significant
      2 bytes (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where packed value is stored.
      value - address to received the packed value.
                                                                                                                   
   Output:
      value - holds the unpacked value.

   Returns:
      Always returns 0.

   Notes:

****************************************************************/
int ORPGMISC_unpack_value_from_ushorts( void *loc, void *value ){

   unsigned int *fw_value = (unsigned int *) value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   *fw_value = 
      (unsigned int) (0xffff0000 & ((*msw) << 16)) | ((*lsw) & 0xffff);

   return 0;

/* End of ORPGMISC_unpack_value_from_ushorts() */
}

/***********************************************************************

    Returns version of Level-II data.

    Input: NONE
    Return:
      LDM_VERSION_UNKN - Unknown
      LDM_VERSION_0 - No LDM
      LDM_VERSION_1 - Code: 0XX000011NXXXXX
      LDM_VERSION_2 - Code: 12X000011NXXXXX
      LDM_VERSION_3 - Code: 111101011YYXXXX
      LDM_VERSION_4 - Code: 112000011NNNXXX
      LDM_VERSION_5 - Code: 14X001111NYY11Y
      LDM_VERSION_6 - Code: 133101111YYY11Y
      LDM_VERSION_7 - Code: 134001111NYY11Y

      Code: ABCDEFGHIJKLMNO is deciphered below where:

        A - Message code
            0 - Message 1 (pre-ORDA)
            1 - Message 31

        B - Super-Res at RDA
            0 - disabled
            1 - SR pre-Build 12.0 enabled
            2 - SR pre-Build 12.0 disabled
            3 - SR Build 12.0+ enabled
            4 - SR Build 12.0+ disabled
            X - Not applicable (pre-Super-Res)

        C - Super-Res for LDM transmission
            1 - SR pre-Build 12.0 enabled
            2 - SR pre-Build 12.0 disabled
            3 - SR Build 12.0+ enabled
            4 - SR Build 12.0+ disabled
            X - Not applicable (pre-Super-Res)

        D - Azimuthal resolution on split cuts
            0 - 1.0 degrees
            1 - 0.5 degrees

        E - Azimuthal resolution on batch cuts
            0 - 1.0 degrees
            1 - 0.5 degrees

        F - Range resolution of Surveillance on split cuts
            0 - 1000 meters
            1 -  250 meters

        G - Range resolution of Surveillance on batch cuts
            0 - 1000 meters
            1 -  250 meters

        H - Range resolution of Doppler on split cuts
            0 - 1000 meters
            1 -  250 meters

        I - Range resolution of Doppler on batch cuts
            0 - 1000 meters
            1 -  250 meters

        J - Surveillance data included on Doppler split cuts
            N - No
            Y - Yes

        K - Doppler data to 300km
            N - No
            Y - Yes
            X - Not applicable

        L - Dual-Pol data included
            N - No
            Y - Yes
            X - Not applicable

        M - Range resolution of Dual-Pol on split cuts
            0 - 1000 meters
            1 -  250 meters
            X - Not applicable

        N - Range resolution of Dual-Pol on batch cuts
            0 - 1000 meters
            1 -  250 meters
            X - Not applicable

        O - Dual-Pol data to 300km
            N - No
            Y - Yes
            X - Not applicable

***********************************************************************/

int ORPGMISC_get_LDM_version () {

  int return_code = LDM_VERSION_UNKN;
  float rda_build_num = ORPGRDA_get_status( RS_RDA_BUILD_NUM )/100.0;
  double value;

  /* Ignore LDM_VERSION_0 - Everyone has LDM now */
  /* Ignore LDM_VERSION_1 - Everyone has an ORDA now */


  /* RDA Build number could be scaled by 100 (11.2 and later) or
     10 (11.1 and earlier). As a simple check, divide by 100. If
     the number is less than 2, the RDA Build number is scaled by
     10. */

  if( rda_build_num  < 2.0 )
  {
    /* RDA Build number is scaled by 10 */
    rda_build_num = ORPGRDA_get_status( RS_RDA_BUILD_NUM )/10.0;
  }

  if( ORPGRDA_get_status( RS_SUPER_RES ) == SR_DISABLED )
  {
    /* Super-Res disabled at the RDA. */
    if( rda_build_num < 12.0 ){ return_code = LDM_VERSION_2; }
    else{ return_code = LDM_VERSION_5; }
  }
  else
  {
    if( DEAU_get_values( "alg.Archive_II.version", &value, 1 ) > 0 )
    {
      if( rda_build_num < 12.0 )
      {
        if( (int) value == LDM_VERSION_4 || (int) value == LDM_VERSION_7 )
        {
          /* Super-Res is enabled at the RDA, but disabled
             for level-II transmission. Build 12+ versions
             4 and 7 correspond to version 4 pre-Build 12. */
          return_code = LDM_VERSION_4;
        }
        else if( (int) value == LDM_VERSION_6 )
        {
          /* Super-Res is enabled at the RDA and for level-II
             transmission. Build 12+ version 6 corresponds to
             version 3 pre-Build 12. */
          return_code = LDM_VERSION_3;
        }
      }
      else
      {
        return_code = (int) value;
      }
    }
  }

  return return_code;
}
