
/******************************************************************

    This file contains routines that reads fr_site.dat.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/01/03 19:23:57 $
 * $Id: gdc_fr_site.c,v 1.1 2011/01/03 19:23:57 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gdc_def.h"

#define TMP_BUF_SIZE 512
#define FR_SITE_SIZE 20
#define MAX_FR_SITES 64

extern char SRC_dir[];
static int Nmt_rt_cnt = 0;

#define MAX_STR_SIZE 256
#define MAX_N_RPG_SITES 256
#define MAX_N_FR_CIRCUTS 100

enum {CFS_LOAD_HUB_RPG, CFS_LOAD_HUB_MSCF, CFS_FR_CIRCUIT, CFS_LOAD_HUB_MANUAL, CFS_HUB_RPG};

#define DISC_SIZE 24
#define SITE_SIZE 16
#define LOC_SIZE 48

typedef struct {
    int dlci;
    char rsite[LOC_SIZE];
    char sitf_ip[LOC_SIZE];
    short is_brg;
    char net2[LOC_SIZE];		/* additional remote network */
    char ritf2[LOC_SIZE];		/* additional remote interface */
} subitf_t;

typedef struct {
    char site[SITE_SIZE];
    int bw;
    char slots[SITE_SIZE];
    int n_cs;
    subitf_t *sitf;
} Frc_site_t;

#define NMT_LEVELS 4
enum {NMT_FR, NMT_RR, NMT_HR};	/* values for Nmt_routes_t.flag */
typedef struct {
    char *site[NMT_LEVELS];	/* site */
    int flag[NMT_LEVELS];	/* flag */
    char *subnet[NMT_LEVELS];	/* subnet */
    char *ip[NMT_LEVELS];
    int wt[NMT_LEVELS];
} Nmt_routes_t;

typedef struct {
    char load_from[FR_SITE_SIZE];
    char remote_mscf[FR_SITE_SIZE];
    char dod_primary[FR_SITE_SIZE];
    char remote_rpg[FR_SITE_SIZE];
    char hub_collocated_rpg[FR_SITE_SIZE];
} fr_hub_site_t;

enum {SD_INIT, SD_SIBNET_ID, SD_AWIPS_IP};
			/* values for which of Get_site_data */

static fr_hub_site_t *Fr_hub_sites;
static int N_fr_hub_sites = -1;
static int N_frc_sites = 0;
static Frc_site_t *Frc_sites;

static int N_nmt_routes = 0;
static Nmt_routes_t *Nmt_routes = NULL;

static void Print_badfile_exit (char *words, char *fname, char *text);
static int Read_fr_site_data (char *dir);
static char *Get_nmt_token (FILE *fd, char type, void *buf, int buf_size);
static int Verify_nmt_data ();
static int Check_site_subnet (char *site, char *subnet);
static int Nmt_read_next_level (FILE *fd, char *fname, int level, 
				Nmt_routes_t *route, int n_secs);
static void Set_fr_variables (char *site);
static void Set_nmt_variables (char *site, FILE *fd);
static int Get_sitf_ip_route (subitf_t *sitf, char *site, 
				char *lip, char *route, char *route2);
static void Add_nmt_route (char **net, char *ip, char *wt, char *desc, 
	char *rtt,
	int ind1, int ind2, Nmt_routes_t *rt, char *label, char *site);
static int Parse_subnet_spec (char *sub_spec, char *net, char *mask);
static int Parse_ip_spec (char *subnet_id, char *ip_spec, 
					char *ip, char *mask, char *rip);
static void Modify_ip_for_chan2 (char *ip);
static char *Get_netmask (int n_bits);
static void Add_one_to_ip (char *ip);
static int Check_fr_site (int is, char *site);
static Nmt_routes_t *Get_nmt_data (char *site, int level);
static Frc_site_t *Get_frc_site_data (char *site);
static int Get_site_data (char *site, int which, char *buf);
static void Read_nmt_data (FILE *fd, char *fname);


/**********************************************************************

    Reads fr_site.dat and outputs rocrar.dat.

**********************************************************************/

void GDCF_process_nmt_data (int n_sites, char *site_names, char *input_file) {
    char fname[MAX_STR_SIZE], buf[TMP_BUF_SIZE];
    FILE *fd;
    int i, off;

    strncpy (fname, input_file, MAX_STR_SIZE);
    fname[MAX_STR_SIZE - 1] = '\0';
    GDCM_add_dir (SRC_dir, fname, MAX_STR_SIZE);
    fd = fopen (fname, "r");
    if (fd == NULL) {
	fprintf (stderr, "Open %s failed\n", fname);
	exit (1);
    }

    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];

	if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;
	if (strstr (buf, "Network Management") != NULL)
	    break;
    }
    Read_nmt_data (fd, fname);
    fclose (fd);

    strcpy (fname, "rocrar.dat");
    GDCM_add_dir (SRC_dir, fname, MAX_STR_SIZE);
    fd = fopen (fname, "w");
    if (fd == NULL) {
	fprintf (stderr, "Could not open/create file %s\n", fname);
	exit (1);
    }

    off = 0;
    for (i = 0; i < n_sites; i++) {
	char site[MAX_STR_SIZE];
	int of;
	if ((of = MISC_get_token (site_names + off, "S,", 0,
					site, MAX_STR_SIZE)) <= 0)
	    break;
	Set_nmt_variables (site, fd);
	off += of;
    }
}

/**********************************************************************

    Reads fr_site.dat and sets related global variables.

**********************************************************************/

int GDCF_read_fr_site (char *params) {
    char site[MAX_STR_SIZE], *p;
    int chan;

    if (Get_site_data (params, SD_INIT, NULL) < 0)
	GDCP_exception_exit ("Unexpected _gdc_read_file calling parameters\n");

    if (Read_fr_site_data (SRC_dir) < 0)
	GDCP_exception_exit ("fr_site.dat not found\n");

    p = GDCV_get_value ("SITE_NAME");
    if (p == NULL)
	GDCP_exception_exit ("SITE_NAME not defined (reading fr_site.dat)\n");
    strncpy (site, p, MAX_STR_SIZE);
    site[MAX_STR_SIZE - 1] = '\0';

    chan = 1;
    p = GDCV_get_value ("CHAN_NUM");
    if (p != NULL && strcmp (p, "2") == 0)
	chan = 2;

    if (Check_fr_site (CFS_LOAD_HUB_RPG, site))
	GDCV_set_variable ("HLRPG", "YES");
    if (Check_fr_site (CFS_LOAD_HUB_MSCF, site))
	GDCV_set_variable ("HLMSCF", "YES");
    if (Check_fr_site (CFS_LOAD_HUB_MANUAL, site))
	GDCV_set_variable ("HLMANUAL", "YES");

    Set_fr_variables (site);

    if (chan != 2)
	Set_nmt_variables (site, NULL);

    return (0);
}

/**********************************************************************

    Reads site data file to get field "which" for site "site" (lower 
    case). The result is stored in "buf" of size MAX_STR_SIZE. When
    which is SD_INIT, "site" is the parameters for _gdc_read_file.

**********************************************************************/

static int Get_site_data (char *site, int which, char *buf) {
    static char subnet_path[MAX_STR_SIZE] = "", awipsip_path[MAX_STR_SIZE];
    char tk[MAX_STR_SIZE];

    if (which == SD_INIT) {
	if (MISC_get_token (site, "", 0, NULL, 0) != 3 ||
	    MISC_get_token (site, "", 0, tk, MAX_STR_SIZE) <= 0 ||
	    strcmp (tk, "fr_site.dat") != 0)
	    return (-1);
	MISC_get_token (site, "", 1, tk, MAX_STR_SIZE);
	strcpy (subnet_path, tk);
	MISC_get_token (site, "", 2, tk, MAX_STR_SIZE);
	strcpy (awipsip_path, tk);
	if (strstr (subnet_path, "site_name") == NULL ||
	    strstr (awipsip_path, "site_name") == NULL)
	    return (-1);
    }
    else {
	char *pat, *p, *p1, path[MAX_STR_SIZE], *v;
	int patl, sitel;

	if (subnet_path[0] == '\0')
	    GDCP_exception_exit (
		"Site data path not initialized (reading fr_site.dat)\n");

	if (which == SD_SIBNET_ID)
	    strcpy (path, subnet_path);
	else if (which == SD_AWIPS_IP)
	    strcpy (path, awipsip_path);
	else
	    GDCP_exception_exit (
		"Unexpected calls to Get_site_data (reading fr_site.dat)\n");
	pat = "site_name";
	patl = strlen (pat);
	sitel = strlen (site);
	if (sitel > patl)
	    GDCP_exception_exit (
	      "Calling site name (%s) too long (reading fr_site.dat)\n", site);
	p = strstr (path, pat);
	strcpy (p, site);
	p1 = p + patl;
	p += sitel;
	memmove (p, p1, strlen (p1) + 1);
	v = GDCR_get_data_value (path);
	if (v == NULL) {
	    char *msg;
	    GDCR_get_error (&msg);
	    fprintf (stderr, "%s", msg);
	    GDCP_exception_exit (
		"Site data (%s) not found (reading fr_site.dat)\n", path);
	}
	strncpy (buf, v, MAX_STR_SIZE);
	buf[MAX_STR_SIZE - 1] = '\0';
    }
    return (0);
}

/**********************************************************************

    Sets frame relay site variables for "site".

**********************************************************************/

static char *Rv_trail_space (char *str) {
    if (str[strlen (str) - 1] == ' ')
	str[strlen (str) - 1] = '\0';
    return (str);
}

static void Set_fr_variables (char *site) {
    static char *sitf_ip = NULL, *sitf_route = NULL;
    static char *sitf_route2 = NULL, *rsite = NULL;
    Frc_site_t *fr;
    char dlci[MAX_STR_SIZE], subitf_id[MAX_STR_SIZE];
    char is_brg[MAX_STR_SIZE], b[MAX_STR_SIZE];
    int n_sitfs, i;

    if (Check_fr_site (CFS_HUB_RPG, site))
	GDCV_set_variable ("FR_HUB_RPG", "YES");

    if (Check_fr_site (CFS_LOAD_HUB_RPG, site))
	GDCV_set_variable ("FR_LOAD_HUB_RPG", "YES");

    GDCV_set_variable ("FRC_SITE", "NO");

    /* for frame relay circuit sites */
    if (!Check_fr_site (CFS_FR_CIRCUIT, site))
	return;

    fr = Get_frc_site_data (site);
    if (fr == NULL)
	GDCP_exception_exit (
		"Frame Relay circuit site data not found for %s\n", site);
    sprintf (b, "%d", fr->bw);
    GDCV_set_variable ("FRC_BW", b);
    GDCV_set_variable ("FRC_TSLOT", fr->slots);
    n_sitfs = fr->n_cs;
    if (n_sitfs < 0 || n_sitfs > 10)
	GDCP_exception_exit ("Too many subinterfaces for site %s\n", site);
    if (fr->n_cs == 0)
	GDCP_exception_exit (
	  "Zero subinterface is no longer allowed (FRC site %s)\n", site);
    else
	GDCV_set_variable ("FRC_SUBITF", "YES");
    GDCV_set_variable ("FRC_SITE", "YES");
    dlci[0] = '\0';
    subitf_id[0] = '\0';
    is_brg[0] = '\0';
    rsite = STR_copy (rsite, "");
    sitf_ip = STR_copy (sitf_ip, "");
    sitf_route = STR_copy (sitf_route, "");
    sitf_route2 = STR_copy (sitf_route2, "");
    for (i = 0; i < n_sitfs; i++) {
	char lip[128], route[128], route2[128];
	int is_rule_08;

	strcpy (b, fr->sitf[i].rsite);
	rsite = STR_cat (rsite, MISC_toupper (b));
	rsite = STR_cat (rsite, " ");

	sprintf (dlci + strlen (dlci), "%d ", fr->sitf[i].dlci);
	sprintf (subitf_id + strlen (subitf_id), "%d ", i + 1);
	if (fr->sitf[i].is_brg)
	    sprintf (is_brg + strlen (is_brg), "%s ", "YES");
	else
	    sprintf (is_brg + strlen (is_brg), "%s ", "NO");

	is_rule_08 = Get_sitf_ip_route (&(fr->sitf[i]), site, lip, route, route2);
	sprintf (b, "\"%s\" ", lip);
	sitf_ip = STR_cat (sitf_ip, b);
	sprintf (b, "\"%s\" ", route);
	sitf_route = STR_cat (sitf_route, b);
	sprintf (b, "\"%s\" ", route2);
	sitf_route2 = STR_cat (sitf_route2, b);

	if (i == n_sitfs - 1) {
	    Rv_trail_space (rsite);
	    Rv_trail_space (dlci);
	    Rv_trail_space (subitf_id);
	    Rv_trail_space (is_brg);
	    Rv_trail_space (sitf_ip);
	    Rv_trail_space (sitf_route);
	    Rv_trail_space (sitf_route2);
	}

	GDCV_set_variable ("FRC_RSITE", rsite);
	GDCV_set_variable ("FRC_DLCI", dlci);
	if (fr->n_cs > 0 || is_rule_08) {
	    GDCV_set_variable ("FRC_SUBITF_ID", subitf_id);
	    GDCV_set_variable ("FRC_SUBITF_BG", is_brg);
	    GDCV_set_variable ("FRC_SUBITF_IP", sitf_ip);
	    GDCV_set_variable ("FRC_SUBITF_ROUTE", sitf_route);
	    GDCV_set_variable ("FRC_SUBITF_ROUTE2", sitf_route2);
	}
    }
}

/**********************************************************************

    Sets network management traffic routing variables based for "site".
    We don't do it for channel 2.

**********************************************************************/

#define MAX_ROUTES (MAX_STR_SIZE / 12)

void Set_nmt_variables (char *site, FILE *fd) {
    static char *net = NULL;
    char ip[MAX_STR_SIZE];
    char wt[MAX_STR_SIZE], desc[MAX_STR_SIZE];
    char rtt[MAX_STR_SIZE];
    Nmt_routes_t *rt;

    Nmt_rt_cnt = 0;
    net = STR_copy (net, "");
    ip[0] = '\0';
    wt[0] = '\0';
    desc[0] = '\0';
    rtt[0] = '\0';
    Get_nmt_data (NULL, 0);
    while ((rt = Get_nmt_data (site, 0)) != NULL) {
	if (rt->site[1][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 1, 1, rt, "P-2N-2IP", site);
	if (rt->site[2][0] != '\0' &&
	    rt->flag[2] != NMT_HR)
	    Add_nmt_route (&net, ip, wt, desc, rtt, 2, 1, rt, "P-3N-2IP", site);
    }

    Get_nmt_data (NULL, 0);
    while ((rt = Get_nmt_data (site, 1)) != NULL) {
	if (rt->site[2][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 2, 2, rt, "S-3N-3IP", site);
    }

    Get_nmt_data (NULL, 0);
    while ((rt = Get_nmt_data (site, 2)) != NULL) {
	if (rt->site[3][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 3, 3, rt, "T-4N-4IP", site);
    }

    if (fd != NULL) {
	int cnt, i;

	cnt = MISC_get_token (wt, "", 0, NULL, 0);
	for (i = 0; i < cnt; i++) {
	    char dd[MAX_STR_SIZE], rr[MAX_STR_SIZE], nn[MAX_STR_SIZE];
	    char ii[MAX_STR_SIZE], ww[MAX_STR_SIZE];

	    if (i == 0)
		fprintf (fd, "%s,  ", site);
	    else
		fprintf (fd, "       ");
	    if (MISC_get_token (desc, "", i, dd, MAX_STR_SIZE) <= 0 ||
		MISC_get_token (rtt, "", i, rr, MAX_STR_SIZE) <= 0 ||
		MISC_get_token (net, "Q\"", i, nn, MAX_STR_SIZE) <= 0 ||
		MISC_get_token (ip, "", i, ii, MAX_STR_SIZE) <= 0 ||
		MISC_get_token (wt, "", i, ww, MAX_STR_SIZE) <= 0) {
		fprintf (stderr, "Error found in NMT routes\n");
		exit (1);
	    }
	    fprintf (fd, "%s  %s  %s  %s  %s,", dd, rr, nn, ii, ww);
	    if (i == cnt - 1)
		fprintf (fd, "\n\n");
	    else
		fprintf (fd, "\\\n");
	}
	return;
    }

    if (net[0] != '\0')
	GDCV_set_variable ("NMTP_NET", Rv_trail_space (net));
    if (ip[0] != '\0')
	GDCV_set_variable ("NMTP_IP", Rv_trail_space (ip));
    if (wt[0] != '\0')
	GDCV_set_variable ("NMTP_WT", Rv_trail_space (wt));
    if (desc[0] != '\0')
	GDCV_set_variable ("NMTP_DESC", Rv_trail_space (desc));
    if (rtt[0] != '\0')
	GDCV_set_variable ("NMTP_DEVICE_TYPE", Rv_trail_space (rtt));

    return;
}

/******************************************************************

    Returns the FR circuit site data struct by site name "site".

******************************************************************/

static Frc_site_t *Get_frc_site_data (char *site) {
    int i;
    char buf[MAX_STR_SIZE];

    if (N_fr_hub_sites < 0)
	GDCP_exception_exit ("FR site data not initialized\n");

    strcpy (buf, site);
    MISC_tolower (buf);
    for (i = 0; i < N_frc_sites; i++) {
	if (strcmp (Frc_sites[i].site, buf) == 0)
	    return (Frc_sites + i);
    }
    return (NULL);
}

/******************************************************************

    Returns pointer to the next NMT route matches "site" in "level".
    "level" 0 is the primary sites. Returns the NULL failure.
    site = NULL resets the search.

******************************************************************/

static Nmt_routes_t *Get_nmt_data (char *site, int level) {
    static int ind = 0;
    int i;

/*
    if (N_nmt_routes <= 0) {
	fprintf (stderr, "Network Management Traffic data not initialized\n");
	exit (1);
    }
*/

    if (site == NULL) {
	ind = 0;
	return (NULL);
    }

    for (i = ind; i < N_nmt_routes; i++) {
	if (strcasecmp (Nmt_routes[i].site[level], site) == 0)
	    break;
    }
    if (i >= N_nmt_routes)
	return (NULL);
    ind = i + 1;
    return (Nmt_routes + i);
}

/******************************************************************

    Reads "fr_site.dat" in "dir" and initializes the FR data structures.
    Returns -1 if the file does not exist, or 0 otherwise.

******************************************************************/

static int Read_fr_site_data (char *dir) {
    char fname[MAX_STR_SIZE];
    FILE *fd;
    char buf[TMP_BUF_SIZE];
    fr_hub_site_t *s;

    if (N_fr_hub_sites >= 0)
	return (0);

    Fr_hub_sites = (fr_hub_site_t *)MISC_malloc 
			(MAX_FR_SITES * sizeof (fr_hub_site_t));
    N_fr_hub_sites = 0;
    GDCM_strlcpy (fname, "fr_site.dat", MAX_STR_SIZE);
    GDCM_add_dir (dir, fname, MAX_STR_SIZE);
    fd = fopen (fname, "r");
    if (fd == NULL)
	return (-1);
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];

	if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;
	if (strstr (buf, "Hub Sites") != NULL)
	    continue;
	if (strstr (buf, "Circuit Sites") != NULL)
	    break;
	if (MISC_get_token (buf, "S,Q\"", 5, tk, MAX_STR_SIZE) < 0)
	    Print_badfile_exit ("Too few tokens", fname, buf);
	if (N_fr_hub_sites >= MAX_FR_SITES) {
	    GDCP_exception_exit ("Too many FR hub sites\n");
	}
	s = Fr_hub_sites + N_fr_hub_sites;
	strncpy (s->remote_rpg, tk, FR_SITE_SIZE);
	s->remote_rpg[FR_SITE_SIZE - 1] = '\0';
	MISC_get_token (buf, "S,Q\"", 2, tk, MAX_STR_SIZE);
	strncpy (s->dod_primary, tk, FR_SITE_SIZE);
	s->dod_primary[FR_SITE_SIZE - 1] = '\0';
	MISC_get_token (buf, "S,Q\"", 1, tk, MAX_STR_SIZE);
	strncpy (s->remote_mscf, tk, FR_SITE_SIZE);
	s->remote_mscf[FR_SITE_SIZE - 1] = '\0';
	MISC_get_token (buf, "S,Q\"", 5, tk, MAX_STR_SIZE);
	strncpy (s->hub_collocated_rpg, tk, FR_SITE_SIZE);
	s->hub_collocated_rpg[FR_SITE_SIZE - 1] = '\0';
	MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE);
	strncpy (s->load_from, tk, FR_SITE_SIZE);
	s->load_from[FR_SITE_SIZE - 1] = '\0';
	N_fr_hub_sites++;
    }

    Frc_sites = (Frc_site_t *)MISC_malloc 
			(MAX_N_FR_CIRCUTS * sizeof (Frc_site_t));
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	Frc_site_t *s;
	int i, n_subis;

	if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;
	if (strstr (buf, "Network Management") != NULL)
	    break;
	if (MISC_get_token (buf, "S,Q\"", 3, tk, MAX_STR_SIZE) < 0)
	    Print_badfile_exit ("Too few tokens", fname, buf);
	if (N_frc_sites >= MAX_FR_SITES) {
	    GDCP_exception_exit ("Too many FR circuit sites\n");
	}

	s = Frc_sites + N_frc_sites;
	if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
	    strlen (tk) >= SITE_SIZE)
	    Print_badfile_exit ("Bad site name", fname, buf);
	strcpy (s->site, tk);
	if (MISC_get_token (buf, "S,Q\"", 1, tk, MAX_STR_SIZE) <= 0 ||
	    sscanf (tk, "%d", &(s->bw)) != 1)
	    Print_badfile_exit ("Bad BW", fname, buf);
	if (MISC_get_token (buf, "S,Q\"", 2, tk, MAX_STR_SIZE) <= 0 ||
	    strlen (tk) >= SITE_SIZE)
	    Print_badfile_exit ("Bad slot token", fname, buf);
	strcpy (s->slots, tk);
	if (MISC_get_token (buf, "S,Q\"", 3, tk, MAX_STR_SIZE) <= 0 ||
	    sscanf (tk, "%d", &(s->n_cs)) != 1 || s->n_cs < 0)
	    Print_badfile_exit ("Bad number of sub-itfs", fname, buf);
	n_subis = s->n_cs;
	if (n_subis == 0)
	    n_subis = 1;
	s->sitf = (subitf_t *)MISC_malloc (n_subis * sizeof (subitf_t));

	for (i = 0; i < n_subis; i++) {

	    if (fgets (buf, TMP_BUF_SIZE, fd) == NULL)
		Print_badfile_exit ("Unexpected end of file", fname, "");
	    if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
		tk[0] == '#') {
		i--;
		continue;
	    }
	    if (MISC_get_token (buf, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
		sscanf (tk, "%d", &(s->sitf[i].dlci)) != 1)
		Print_badfile_exit ("Bad DLCI", fname, buf);
	    if (MISC_get_token (buf, "S,Q\"", 1, tk, MAX_STR_SIZE) <= 0 ||
		strlen (tk) >= SITE_SIZE)
		Print_badfile_exit ("Bad remote site name", fname, buf);
	    strcpy (s->sitf[i].rsite, tk);
	    s->sitf[i].sitf_ip[0] = '\0';
	    s->sitf[i].is_brg = 0;
	    if (MISC_get_token (buf, "S,Q\"", 2, tk, LOC_SIZE) > 0) {
	        if (strcmp (tk, "brg") != 0)
		    strcpy (s->sitf[i].sitf_ip, tk);
		else
		    s->sitf[i].is_brg = 1;
	    }
	    s->sitf[i].net2[0] = '\0';
	    s->sitf[i].ritf2[0] = '\0';
	    if (MISC_get_token (buf, "S,Q\"", 3, tk, LOC_SIZE) > 0)
		strcpy (s->sitf[i].net2, tk);
	    if (MISC_get_token (buf, "S,Q\"", 4, tk, LOC_SIZE) > 0)
		strcpy (s->sitf[i].ritf2, tk);
	}
	N_frc_sites++;
    }
    Read_nmt_data (fd, fname);
    fclose (fd);

    return (0);
}

static void Read_nmt_data (FILE *fd, char *fname) {

    while (1) {			/* read Network Management Traffic routes */
	Nmt_routes_t route;
	int i;

	memset (&route, 0, sizeof (Nmt_routes_t));
	for (i = 0; i < 4; i++) {
	    route.site[i] = "";
	    route.ip[i] = "";
	}
	if (Nmt_read_next_level (fd, fname, 0, &route, 1) < 0)
	    break;
    }

    Verify_nmt_data ();
}

/******************************************************************

    Reads the level of "level" of NMT specification for primary site 
    "route->site[0]". The number of routes for this level is "n_secs".
    The file name "fname" and its handle is "fd".

******************************************************************/

static int Nmt_read_next_level (FILE *fd, char *fname, int level, 
					Nmt_routes_t *route, int n_secs) {
    static char pre_read_site[MAX_STR_SIZE] = "";
    int i, done;

    done = 0;
    for (i = 0; i < n_secs; i++) {
	char c, tk[MAX_STR_SIZE];
	char site[MAX_STR_SIZE], flag[MAX_STR_SIZE], ip[MAX_STR_SIZE];
	int wt, n_ters;
	char buf[MAX_STR_SIZE], subnet[MAX_STR_SIZE];

	strcpy (site, pre_read_site);
	if ((pre_read_site[0] == '\0' && 
		Get_nmt_token (fd, 'c', site, MAX_STR_SIZE) == NULL) ||
	    Get_nmt_token (fd, 'c', flag, MAX_STR_SIZE) == NULL ||
	    Get_nmt_token (fd, 'c', subnet, MAX_STR_SIZE) == NULL ||
	    (level >= 1 &&
		Get_nmt_token (fd, 'c', ip, MAX_STR_SIZE) == NULL) ||
	    (level >= 1 &&
		Get_nmt_token (fd, 'i', &wt, 0) == NULL)) {
	    if (route->site[0][0] != '\0')
		sprintf (buf, "level %d, primary site %s", level, route->site[0]);
	    else
		sprintf (buf, "level %d, near token %s", level, site);
	    Print_badfile_exit ("Bad NMT spec", fname, buf);
	}
	route->site[level] = STR_copy (NULL, site);
	route->subnet[level] = STR_copy (NULL, subnet);
	if (strcmp (flag, "(FR)") == 0)
	    route->flag[level] = NMT_FR;
	else if (strcmp (flag, "(RR)") == 0)
	    route->flag[level] = NMT_RR;
	else if (strcmp (flag, "(HR)") == 0)
	    route->flag[level] = NMT_HR;
	else {
	    sprintf (buf, "level %d, primary site %s", level, route->site[0]);
	    Print_badfile_exit ("Bad flag spec", fname, buf);
	}
	if (level >= 1) {
	    route->ip[level] = STR_copy (NULL, ip);
	    route->wt[level] = wt;
	}
	tk[0] = '\0';
	if (Get_nmt_token (fd, 'c', tk, MAX_STR_SIZE) == NULL)
	    done = 1;
	if (sscanf (tk, "%d%c", &n_ters, &c) == 1) {
	    pre_read_site[0] = '\0';
	    if (Nmt_read_next_level (fd, fname, level + 1, route, n_ters) < 0)
		return (-1);
	}
	else {
	    Nmt_routes = (Nmt_routes_t *)STR_append ((char *)Nmt_routes, 
			    (char *)route, sizeof (Nmt_routes_t));
	    N_nmt_routes++;
	    strcpy (pre_read_site, tk);
	}
    }
    route->site[level] = "";
    route->subnet[level] = "";
    route->ip[level] = "";
    route->wt[level] = 0;
    if (done)
	return (-1);
    return (0);
}

static void Print_badfile_exit (char *words, char *fname, char *text) {
    GDCP_exception_exit ("%s in file %s at \"%s\"\n", words, fname, text);
}

/******************************************************************

    Gets the next token in the "Network Management" section of the 
    FR site info. The token is returned in "buf" of size "buf_size" 
    in type "type". The function returns "buf" on success or NULL
    on failure. If fd = NULL, the content of the current line is copied
    to "buf".

******************************************************************/

#define LINE_SIZE 256

static char *Get_nmt_token (FILE *fd, char type, void *buf, int buf_size) {
    static char line[LINE_SIZE] = "";
    static int token = -1;

    if (fd == NULL) {
	strncpy (buf, line, buf_size);
	((char *)buf)[buf_size - 1] = '\0';
	return (buf);
    }
    while (1) {
	int ret;
	char format[16];
	if (token < 0) {
	    char tk[MAX_STR_SIZE];
	    if (fgets (line, LINE_SIZE, fd) == NULL)
		return (NULL);
	    if (MISC_get_token (line, "S,Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
		tk[0] == '#')
		continue;
	    token = 0;
	}
	sprintf (format, "S,Q\"C%c", type);
	ret = MISC_get_token (line, format, token, buf, buf_size);
	if (ret == 0) {
	    token = -1;
	    continue;
	}
	if (ret < 0)
	    return (NULL);
	token++;
	return (buf);
    }
}

/******************************************************************

    Prints varification info for Network Management Traffic data.

******************************************************************/

static int Verify_nmt_data () {
    int i, cnt;
    char *bad_sites;

return (0);	/* no longer used */

/*
    printf ("N_nmt_routes %d\n", N_nmt_routes);
    for (i = 0; i < N_nmt_routes; i++) {
	Nmt_routes_t *r = Nmt_routes + i;
	printf ("    %s %d %s  %s %d %s %s %d  %s %d %s %s %d  %s %d %s %s %d\n", 
		r->site[0], r->flag[0], r->subnet[0], 
		r->site[1], r->flag[1], r->subnet[1], r->ip[1], r->wt[1],
		r->site[2], r->flag[2], r->subnet[2], r->ip[2], r->wt[2],
		r->site[3], r->flag[3], r->subnet[3], r->ip[3], r->wt[3]);
    }
*/

    bad_sites = NULL;
    cnt = 0;
    for (i = 0; i < N_nmt_routes; i++) {
	int k;
	Nmt_routes_t *r = Nmt_routes + i;
	for (k = 0; k < 4; k++) {
	    if (r->site[k][0] != '\0' &&
		strcmp (r->site[k], "roc") != 0 &&
		strcmp (r->site[k], "smg") != 0 &&
		Check_site_subnet (r->site[k], r->subnet[k]) < 0) {
		bad_sites = STR_cat (bad_sites, r->site[k]);
		bad_sites = STR_cat (bad_sites, " ");
		cnt++;
	    }
	}
    }
/*
    if (cnt > 0) {
	printf ("Sites of Bad subnet: %s\n", bad_sites);
    }
*/
    STR_free (bad_sites);

    return (0);
}

static int Check_site_subnet (char *site, char *subnet) {
    int snet;
    char subnet_id[MAX_STR_SIZE];

    if (MISC_get_token (subnet, "S.", 0, NULL, 0) == 2) {
	if (MISC_get_token (subnet, "S.Ci", 0, &snet, 0) <= 0 ||
	    MISC_get_token (subnet, "S.Ci", 1, &snet, 0) <= 0) {
	    printf ("Warning: Bad NMT subnet ID %s (%s)\n", subnet, site);
	    return (-1);
	}
	return (0);
    }
    if (MISC_get_token (subnet, "Ci", 0, &snet, 0) <= 0) {
	printf ("Warning: Bad NMT subnet ID %s (%s)\n", subnet, site);
	return (-1);
    }

    Get_site_data (site, SD_SIBNET_ID, subnet_id);
    if (atoi (subnet_id) != snet) {
	printf ("Warning: subnet ID mismatch %s (host data) %d (NMT) (%s)\n", 
					subnet_id, snet, site);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Checks if "site" is of the type as specified by "is".

******************************************************************/

static int Check_fr_site (int is, char *site) {
    int ret;
    char lsite[MAX_STR_SIZE];

    if (N_fr_hub_sites < 0)
	GDCP_exception_exit ("fr hb sites not initialized\n");

    strcpy (lsite, site);
    MISC_tolower (lsite);
    ret = 0;
    switch (is) {
	int i;
	case CFS_LOAD_HUB_RPG:
	case CFS_LOAD_HUB_MSCF:
	case CFS_LOAD_HUB_MANUAL:
	    for (i = 0; i < N_fr_hub_sites; i++) {
		if (strstr (Fr_hub_sites[i].load_from, lsite) != NULL)
		    break;
	    }
	    if (i < N_fr_hub_sites) {
		if (strstr (Fr_hub_sites[i].load_from, "mscf") != NULL) {
		    if (is == CFS_LOAD_HUB_MSCF)
			ret = 1;
		}
		else if (strstr (Fr_hub_sites[i].load_from, "manual") != NULL) {
		    if (is == CFS_LOAD_HUB_MANUAL)
			ret = 1;
		}
		else {
		    if (is == CFS_LOAD_HUB_RPG)
			ret = 1;
		}
	    }
	    break;
	case CFS_HUB_RPG:
	    for (i = 0; i < N_fr_hub_sites; i++) {
		if ((strstr (Fr_hub_sites[i].load_from, lsite) != NULL &&
			(strstr (Fr_hub_sites[i].load_from, "/") == NULL ||
			strstr (Fr_hub_sites[i].load_from, "rpg") != NULL)) ||
		    strstr (Fr_hub_sites[i].hub_collocated_rpg, lsite) != NULL)
		    break;
	    }
	    if (i < N_fr_hub_sites)
		ret = 1;
	    break;
	case CFS_FR_CIRCUIT:
	    ret = 1;
	    if (Get_frc_site_data (lsite) == NULL)
		ret = 0;
	    break;
	default:
	    break;
    }
    return (ret);
}

/**********************************************************************

    Evaluates the local subinterface IP and mask, the remote IP, the 
    remote subnet and subnet mask, and the second remote subnet and subnet 
    mask for subinterface "sitf" of FRC site "s".

**********************************************************************/

static int Get_sitf_ip_route (subitf_t *sitf, char *site, 
				char *lip, char *route, char *route2) {
    char ip[MAX_STR_SIZE], mask[MAX_STR_SIZE], rip[MAX_STR_SIZE];
    char rnet[MAX_STR_SIZE], rnet_mask[MAX_STR_SIZE];
    char rip2[MAX_STR_SIZE], rnet2[MAX_STR_SIZE];
    char rnet_mask2[MAX_STR_SIZE], b[MAX_STR_SIZE], subnet_id[MAX_STR_SIZE];

    Get_site_data (site, SD_SIBNET_ID, subnet_id);
    if (strcmp (sitf->sitf_ip, "rule_08") == 0) {	/* The 2008 rule */
	char *pt, b[32], rem_subnet[MAX_STR_SIZE], rem_awipsip[MAX_STR_SIZE];
	int oct3, oct4;

	if (MISC_get_token (sitf->rsite, "S.Ci", 0, &oct3, 0) >= 0 &&
	    MISC_get_token (sitf->rsite, "S.Ci", 1, &oct4, 0) >= 0) {
	    Get_site_data (site, SD_AWIPS_IP, rem_awipsip);
	}
	else {
	    Get_site_data (sitf->rsite, SD_SIBNET_ID, rem_subnet);
	    Get_site_data (sitf->rsite, SD_AWIPS_IP, rem_awipsip);
	    oct3 = atoi (rem_subnet);
	    oct4 = 35;
	}

	sprintf (ip, "172.21.%s.101", subnet_id);
	sprintf (rip, "172.21.%s.100", subnet_id);

	strcpy (b, rem_awipsip);
	pt = b + strlen (b) - 1;
	while (pt > b && *pt != '.')
	    pt--;
	*pt = '\0';
	sprintf (rnet, "%s.0", b); 

	Modify_ip_for_chan2 (ip);
	route2[0] = '\0';
        sprintf (lip, "%s 255.255.255.248", ip);
	if (MISC_get_token (rnet, "S.", 0, NULL, 0) == 4)
	    sprintf (route, "%s 255.255.255.0 %s", rnet, rip);
	else
	    route[0] = '\0';
/*	sprintf (route2, "172.25.%d.0 255.255.255.128 %s", oct3, rip); */
	sprintf (b, "%d", oct3);
	GDCV_set_variable ("MSCF_IP_O3", b);
	sprintf (b, "%d", oct4);
	GDCV_set_variable ("MSCF_IP_O4", b);
	return (1);
    }

    strcpy (mask, "255.255.255.128");	/* default interface mask */
    ip[0] = '\0';
    if (strcmp (sitf->rsite, "smg") == 0) {
	sprintf (ip, "172.20.%s.100", subnet_id);
	sprintf (rip, "172.20.%s.101", subnet_id);
	sprintf (rnet, "165.92.220.0");
	sprintf (rnet_mask, "255.255.255.0");
    }
    else if (strcmp (sitf->rsite, "roc") == 0) {
	strcpy (mask, "255.255.255.252");	/* interface mask */
	sprintf (ip, "172.21.%s.201", subnet_id);
	sprintf (rip, "172.21.%s.202", subnet_id);
	sprintf (rnet, "172.25.187.0");
	sprintf (rnet_mask, "255.255.255.128");
    }
    else if (MISC_get_token (sitf->rsite, "S.", 0, NULL, 0) == 4) {
	if (Parse_subnet_spec (sitf->rsite, rnet, rnet_mask) < 0)
	    GDCP_exception_exit (
		"Bad sub-net spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->rsite, sitf->dlci, site);
    }
    else {
	char *pt, rem_subnet[MAX_STR_SIZE], rem_awipsip[MAX_STR_SIZE];
	if (Get_site_data (sitf->rsite, SD_SIBNET_ID, rem_subnet) < 0 ||
	    Get_site_data (sitf->rsite, SD_AWIPS_IP, rem_awipsip) < 0)
	    GDCP_exception_exit (
		"Invalid remote RPG site (%s) used by FR circuit site %s (dlci %d)\n",
				    sitf->rsite, site, sitf->dlci);
	sprintf (ip, "172.20.%s.100", rem_subnet);
	sprintf (rip, "172.20.%s.101", rem_subnet);
	strcpy (b, rem_awipsip);
	pt = b + strlen (b) - 1;
	while (pt > b && *pt != '.')
	    pt--;
	*pt = '\0';
	sprintf (rnet, "%s.0", b); 
	strcpy (rnet_mask, "255.255.255.0"); 
    }

    if (sitf->sitf_ip[0] != '\0') {
	if (Parse_ip_spec (subnet_id, sitf->sitf_ip, ip, mask, rip) < 0) {
	    GDCP_exception_exit (
		"Bad sub-itf IP spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->sitf_ip, sitf->dlci, site);
	}
    }
    if (ip[0] == '\0')
	GDCP_exception_exit (
		"Sub-itf IP not specified (dlci %d, FR circuit site %s)\n",
				    sitf->dlci, site);

    /* second route */
    rnet2[0] = '\0';
    if (sitf->net2[0] != '\0') {
	char not_used[MAX_STR_SIZE];
	if (Parse_subnet_spec (sitf->net2, rnet2, rnet_mask2) < 0)
	    GDCP_exception_exit (
		"Bad sub-net spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->net2, sitf->dlci, site);
	if (Parse_ip_spec (subnet_id, sitf->ritf2, not_used, not_used, rip2) < 0)
	    GDCP_exception_exit (
		"Bad sub-itf IP spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->ritf2, sitf->dlci, site);
    }

    if (strcmp (sitf->rsite, "roc") != 0)
	Modify_ip_for_chan2 (ip);
/*
    if (GDCV_get_value ("FAA_CH2") != NULL)
	GDCP_exception_exit (
	  "sub-itf spec not allowed for channel 2 (dlci %d, FR circuit site %s)\n",
				    sitf->dlci, site);
*/
    sprintf (lip, "%s %s", ip, mask);
    sprintf (route, "%s %s %s", rnet, rnet_mask, rip);
    if (rnet2[0] != '\0')
	sprintf (route2, "%s %s %s", rnet2, rnet_mask2, rip2);
    else
	route2[0] = '\0';

    return (0);
}

/**********************************************************************

    Parses FRC subnetwork specification "sub_spec" (field rsite or net2
    of subitf_t, as read from fr_site.dat, column 2 or 4, of
    subinterface spec). The subnet and mask are returned in "net" and
    "mask". Returns 0 on success or -1 on failure.

**********************************************************************/

static int Parse_subnet_spec (char *sub_spec, char *net, char *mask) {
    int n_bits, i0, i1, i2, i3;
    char ip[128];

    if (MISC_get_token (sub_spec, "S/Ci", 1, &n_bits, 0) <= 0)
	n_bits = 24;
    strcpy (mask, Get_netmask (n_bits));
    MISC_get_token (sub_spec, "S/", 0, ip, 128);
    if (MISC_get_token (ip, "S.Ci", 0, &i0, 0) <= 0 ||
	MISC_get_token (ip, "S.Ci", 1, &i1, 0) <= 0 ||
	MISC_get_token (ip, "S.Ci", 2, &i2, 0) <= 0 ||
	MISC_get_token (ip, "S.Ci", 3, &i3, 0) <= 0)
	return (-1);
    sprintf (net, "%d.%d.%d.%d", i0, i1, i2, i3);
    return (0);
}

/**********************************************************************

    Parses FRC subinterface IP specification "ip_spec" (field sitf_ip
    or ritf2 of subitf_t, as read from fr_site.dat, column 3 or 5, of
    subinterface spec). The local interface IP and mask are returned in
    "ip" and "mask". The remote interface IP is returned in "rip". 
    Returns 0 on success or -1 on failure.

**********************************************************************/

static int Parse_ip_spec (char *subnet_id, char *ip_spec, 
					char *ip, char *mask, char *rip) {
    char ips[128], tk[MAX_STR_SIZE];
    int n_bits, i0, i1, i2, i3, i4;

    if (ip_spec[0] == '\0')
	return (-1);
    MISC_get_token (ip_spec, "S/", 0, ips, 128);
    if (MISC_get_token (ips, "S.Ci", 0, &i0, 0) <= 0 ||
	MISC_get_token (ips, "S.Ci", 1, &i1, 0) <= 0)
	return (-1);

    if (MISC_get_token (ips, "S.Ci", 2, &i2, 0) <= 0) {
	if (MISC_get_token (ips, "S.", 2, tk, MAX_STR_SIZE) <= 0 ||
	    strcmp (tk, "subnet") != 0)
	    return (-1);
	i2 = atoi (subnet_id);
    }

    if (MISC_get_token (ips, "S.", 0, NULL, 0) == 3) {
	sprintf (ip, "%d.%d.%d.100", i0, i1, i2);
	sprintf (rip, "%d.%d.%d.101", i0, i1, i2);
    }
    else if (MISC_get_token (ips, "S.", 0, NULL, 0) == 4) {
	MISC_get_token (ips, "S.", 3, tk, MAX_STR_SIZE);
	if (MISC_get_token (tk, "S-", 0, NULL, 0) == 2) {
	    if (MISC_get_token (tk, "S-Ci", 0, &i3, 0) <= 0 ||
		MISC_get_token (tk, "S-Ci", 1, &i4, 0) <= 0)
		return (-1);
	    sprintf (ip, "%d.%d.%d.%d", i0, i1, i2, i3 - i4);
	    sprintf (rip, "%d.%d.%d.%d", i0, i1, i2, i3);
	}
	else if (MISC_get_token (tk, "S+", 0, NULL, 0) == 2) {
	    if (MISC_get_token (tk, "S+Ci", 0, &i3, 0) <= 0 ||
		MISC_get_token (tk, "S+Ci", 1, &i4, 0) <= 0)
		return (-1);
	    sprintf (ip, "%d.%d.%d.%d", i0, i1, i2, i3 + i4);
	    sprintf (rip, "%d.%d.%d.%d", i0, i1, i2, i3);
	}
	else
	    return (-1);
    }
    else
	return (-1);
    n_bits = 25;
    if (MISC_get_token (ip_spec, "S/", 0, NULL, 0) == 2) {
	if (MISC_get_token (ip_spec, "S/Ci", 1, &n_bits, 0) <= 0)
	    return (-1);
    }
    strcpy (mask, Get_netmask (n_bits));
    return (0);
}

/**********************************************************************

    Modifies "ip" for FAA channel 2 by adding add 1 to the last octet.

**********************************************************************/

static void Modify_ip_for_chan2 (char *ip) {

    if (GDCV_get_value ("FAA_CH2") == NULL)
	return;
    Add_one_to_ip (ip);
}

/**********************************************************************

    Constucts and returns the text form of netmask in terms of the 
    "n_bits" format netmask.

**********************************************************************/

static char *Get_netmask (int n_bits) {
    static char buf[64];
    int byte_ind, bit_ind, bytes[4], i;

    for (i = 0; i < 4; i++)
	bytes[i] = 0;

    if (n_bits > 32)
	n_bits = 32;
    if (n_bits < 0)
	n_bits = 0;
    byte_ind = 0;
    bit_ind = 7;
    for (i = 0; i < n_bits; i++) {
	bytes[byte_ind] |= 1 << bit_ind;
	if (bit_ind > 0)
	    bit_ind--;
	else {
	    bit_ind = 7;
	    byte_ind++;
	}
    }
    sprintf (buf, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return (buf);
}

/**********************************************************************

    Add 1 to the last octet of the ip address "ip".

**********************************************************************/

static void Add_one_to_ip (char *ip) {
    int o0, o1, o2, o3;

    if (MISC_get_token (ip, "S.Ci", 0, &o0, 0) > 0 &&
	MISC_get_token (ip, "S.Ci", 1, &o1, 0) > 0 &&
	MISC_get_token (ip, "S.Ci", 2, &o2, 0) > 0 &&
	MISC_get_token (ip, "S.Ci", 3, &o3, 0) > 0) {
	sprintf (ip, "%d.%d.%d.%d", o0, o1, o2, o3 + 1);
    }
}

/**********************************************************************

    Adds a new NMT route if it is not already added.

**********************************************************************/

static void Add_nmt_route (char **net, char *ip, char *wt, char *desc,
	char *rtt,
	int ind1, int ind2, Nmt_routes_t *rt, char *label, char *site) {
    struct set_struct {
	char *subnet;
	char *ip;
	int wt;
    };
    static struct set_struct set[MAX_ROUTES];
    static int init = 0;
    int i;
    char snet[MAX_STR_SIZE];

    if (!init) {
	for (i = 0; i < MAX_ROUTES; i++)
	    set[i].subnet = NULL;
	init = 1;
    }

    if (MISC_get_token (rt->subnet[ind1], "S.", 0, NULL, 0) == 2)
	sprintf (snet, "\"172.25.%s 255.255.255.255\" ", rt->subnet[ind1]);
    else
	sprintf (snet, "\"172.25.%s.0 255.255.255.128\" ", rt->subnet[ind1]);

    for (i = 0; i < Nmt_rt_cnt; i++) {
	if (strcmp (snet, set[i].subnet) == 0 &&
	    strcmp (rt->ip[ind2], set[i].ip) == 0 &&
	    rt->wt[ind2] == set[i].wt)
	    return;
    }

    *net = STR_cat (*net, snet);

    sprintf (ip + strlen (ip), "%s ", rt->ip[ind2]);
    sprintf (wt + strlen (wt), "%d ", rt->wt[ind2]);
    sprintf (desc + strlen (desc), "%s ", label);
    if (strncmp (label, "P-", 2) == 0)
	i = 0;
    else if (strncmp (label, "S-", 2) == 0)
	i = 1;
    else
	i = 2;
    if (rt->flag[i] == NMT_FR)
	sprintf (rtt + strlen (rtt), "FR ");
    else
	sprintf (rtt + strlen (rtt), "RTR ");
    if (Nmt_rt_cnt >= MAX_ROUTES)
	GDCP_exception_exit ("Too many NMT routes for %s\n", site);
    if (set[Nmt_rt_cnt].subnet != NULL)
	free (set[Nmt_rt_cnt].subnet);
    set[Nmt_rt_cnt].subnet = MISC_malloc (strlen (snet) + 1);
    strcpy (set[Nmt_rt_cnt].subnet, snet);
    set[Nmt_rt_cnt].ip = rt->ip[ind2];
    set[Nmt_rt_cnt].wt = rt->wt[ind2];
    Nmt_rt_cnt++;
}

