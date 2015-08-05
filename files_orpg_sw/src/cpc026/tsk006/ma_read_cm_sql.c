
/******************************************************************

    This file contains routines that reads site info in site data
    files created from querying CM data base.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/01/08 22:31:07 $
 * $Id: ma_read_cm_sql.c,v 1.9 2010/01/08 22:31:07 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#define NEED_SITE_STRING_CONST
#define NEED_COMMS_STRING_CONST
#include "manage_adapt.h"

#define TMP_BUF_SIZE 512

static site_info_t *Df_site = NULL;	/* default site info */

static char *Get_tok_p;
static int Get_tok_err = 0;
static int Tok_ind = 0;

static int Site_cmp (void *s1p, void *s2p);
static char *Get_str_tok ();
static int Get_int_tok ();
static void Reset_get_tok (char *str);
static int Parse_site_line (char *buf, site_info_t *site);
static int Set_comms_default (char *dir, site_info_t *sites, int n_sites);
static char *my_fgets (char *buf, int size, FILE *fl);


/************************************************************************

    Reads site info from CM DB view files in "dir" and stores it in 
    "sites", a buffer of size "max_sites". It returns the number of sites 
    read on success or -1 on failure.

************************************************************************/

int RCS_read_cm_sites (char *dir, site_info_t *sites, int max_sites) {
    char fname[LOCAL_NAME_SIZE], buf[TMP_BUF_SIZE];
    int cnt, n_sites;
    FILE *fl;
    site_info_t site;

    sprintf (fname, "%s/%s", dir, "siteinfo.csv");
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "open %s failed\n", fname);
	exit (1);
    }
    cnt = 0;
    n_sites = 0;
    printf ("Reading file %s...\n", fname);
    while (my_fgets (buf, TMP_BUF_SIZE, fl) != NULL) {
	if (cnt == 0) {
	    cnt++;
	    continue;
	}
	if (Parse_site_line (buf, &site) >= 0) {
	    if (n_sites >= max_sites) {
		fprintf (stderr, "Too many sites - RCS_read_site_sql\n");
		exit (1);
	    }
	    memcpy (sites + n_sites, &site, sizeof (site_info_t));
	    n_sites++;
	}
	else {
	    fprintf (stderr, "Error found in %s\n", fname);
	    exit (1);
	}
    }

    RF_sort_verify_sites (sites, n_sites);
    fclose (fl);
    return (n_sites);
}

/************************************************************************

    Removes the ending "\r" and "\n" characters if fgets reads any thing.
    This is necessary for reading Windows text files.

************************************************************************/

static char *my_fgets (char *buf, int size, FILE *fl) {
    char *p;

    if (fgets (buf, size, fl) == NULL)
	return (NULL);
    p = buf + strlen (buf) - 1;
    while (p >= buf) {
	if (*p > 13)
	    break;
	*p = '\0';
	p--;
    }
    return (buf);
}

/************************************************************************

    Parses a line of site info in "buf" and stores it in "site". 
    "sites" of size "n_sites" is the array of all site. It returns 0 on 
    success or -1 on failure.

************************************************************************/

static int Parse_site_line (char *buf, site_info_t *site) {
    char *t, *sname;
    int i;

    Reset_get_tok (buf);

    memset (site, 0, sizeof (site_info_t));
    strncpy (site->name, Get_str_tok (), SHORT_NAME_SIZE);
    site->name[SHORT_NAME_SIZE - 1] = '\0';
    sname = site->name;
    site->lat = Get_int_tok ();
    site->lon = Get_int_tok ();
    site->elev = Get_int_tok ();
    site->rpg_id = Get_int_tok ();
    if ((site->wx_mode = RF_get_enum (Wx_mode, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad Wx_mode (site %s)\n", sname);
	return (-1);
    }
    site->vcp_a = Get_int_tok ();
    site->vcp_b = Get_int_tok ();
    if ((site->mlos = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad has_mlos (site %s)\n", sname);
	return (-1);
    }
    if ((site->rms = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad has_rms (site %s)\n", sname);
	return (-1);
    }
    if ((site->bdds = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad has_bdds (site %s)\n", sname);
	return (-1);
    }
    if ((site->a_3 = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad has_archive_III (site %s)\n", sname);
	return (-1);
    }
    strncpy (site->node, Get_str_tok (), SHORT_NAME_SIZE);
    site->node[SHORT_NAME_SIZE - 1] = '\0';
    site->p_code = Get_int_tok ();
    if ((site->redun_t = RF_get_enum (Redun_t, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad redundant_type (site %s)\n", sname);
	return (-1);
    }
    if ((site->channel = RF_get_enum (Channel, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad channel_number (site %s)\n", sname);
	return (-1);
    }
    site->n_mlos = Get_int_tok ();
    t = strtok (Get_str_tok (), ",");
    for (i = 0; i < N_MLOS; i++) {
	if (t == NULL || (site->mlos_t[i] = RF_get_enum (Mlos_t, t)) < 0) {
	    fprintf (stderr, "Bad MLOS type (site %s)\n", sname);
	    return (-1);
	}
	t = strtok (NULL, ",");
    }
    if ((site->orda = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad orda (site %s)\n", sname);
	return (-1);
    }
    if ((site->enable_sr = RF_get_enum (Yes_no, Get_str_tok ())) < 0) {
	fprintf (stderr, "Bad enable_sr (site %s)\n", sname);
	return (-1);
    }

    if (Get_tok_err) {
	fprintf (stderr, "site line not correct (site %s)\n", sname);
	return (-1);
    }
    return (0);
}

/************************************************************************

    Reads user info from CM DB view file "name" in "dir" and stores it in 
    "users", a buffer of size "max_users". It returns the number of users 
    read on success or -1 on failure.

************************************************************************/

int RCS_read_cm_users (char *dir, char *name, User_t *users, int max_users) {
    char fname[LOCAL_NAME_SIZE], buf[TMP_BUF_SIZE];
    int n_users, cnt;
    FILE *fl;

    sprintf (fname, "%s/%s", dir, name);
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "open %s failed\n", fname);
	exit (1);
    }
    cnt = 0;
    n_users = 0;
    printf ("Reading file %s...\n", fname);
    while (my_fgets (buf, TMP_BUF_SIZE, fl) != NULL) {
	char uname[UNAME_SIZE], desc[UDESC_SIZE], pswd[UNAME_SIZE];
	int id, i;

	cnt++;
	if (cnt == 1)
	    continue;

	Reset_get_tok (buf);
	id = Get_int_tok ();
	strncpy (uname, Get_str_tok (), UNAME_SIZE);
	uname[UNAME_SIZE - 1] = '\0';
	strncpy (pswd, Get_str_tok (), UNAME_SIZE);
	pswd[UNAME_SIZE - 1] = '\0';
	strncpy (desc, Get_str_tok (), UDESC_SIZE);
	desc[UDESC_SIZE - 1] = '\0';
	if (Get_tok_err) {
	    fprintf (stderr, "User error in file %s (line %s)\n", fname, buf);
	    exit (1);
	}
	for (i = 0; i < n_users; i++) {
	    if (users[i].id == id) {
		if (strcmp (users[i].name, uname) != 0 ||
		    strcmp (users[i].desc, desc) != 0 ||
		    strcmp (users[i].pswd, pswd) != 0) {
		    fprintf (stderr, "Duplicated user id (%d in %s)\n", 
								id, fname);
		    exit (1);
		}
		break;
	    }
	}
	if (i < n_users)
	    continue;
	if (n_users >= max_users) {
	    fprintf (stderr, "Too many users read from %s\n", fname);
	    exit (1);
	}
	users[n_users].id = id;
	strcpy (users[n_users].name, uname);
	strcpy (users[n_users].desc, desc);
	strcpy (users[n_users].pswd, pswd);
	n_users++;
    }
    printf ("%d users read\n", n_users);

    fclose (fl);
    return (n_users);
}

/************************************************************************

    Reads hci passwords from CM DB view files in "dir" and stores it in 
    the corresponding site in array "sites" of all sites. "n_sites" is
    the total number of sites. It returns 0 on success or -1 on failure.

************************************************************************/

int RCS_read_cm_hci_pswd (char *dir, site_info_t *sites, int n_sites) {
    char fname[LOCAL_NAME_SIZE], buf[TMP_BUF_SIZE];
    int cnt;
    FILE *fl;

    sprintf (fname, "%s/%s", dir, "hcipassword.csv");
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "open %s failed\n", fname);
	exit (1);
    }
    cnt = 0;
    printf ("Reading file %s...\n", fname);
    while (my_fgets (buf, TMP_BUF_SIZE, fl) != NULL) {
	site_info_t *s, *s1;

	cnt++;
	if (cnt == 1)
	    continue;

	Reset_get_tok (buf);

	s = RCS_search_site (Get_str_tok (), "RPG1", sites, n_sites);
	if (s == NULL)
	    continue;
	strncpy (s->agency_pwd, Get_str_tok (), PSWD_SIZE);
	s->agency_pwd[PSWD_SIZE - 1] = '\0';
	strncpy (s->roc_pwd, Get_str_tok (), PSWD_SIZE);
	s->roc_pwd[PSWD_SIZE - 1] = '\0';
	strncpy (s->urc_pwd, Get_str_tok (), PSWD_SIZE);
	s->urc_pwd[PSWD_SIZE - 1] = '\0';
	if (Get_tok_err) {
	    fprintf (stderr, "HCI password error (site %s)\n", s->name);
	    exit (1);
	}
	if ((s1 = RCS_search_site (s->name, "RPG2", sites, n_sites)) != NULL) {
	    strcpy (s1->agency_pwd, s->agency_pwd);
	    strcpy (s1->roc_pwd, s->roc_pwd);
	    strcpy (s1->urc_pwd, s->urc_pwd);
	}
    }
    printf ("HCI password read for %d sites\n", cnt - 1);

    fclose (fl);
    return (0);
}

/************************************************************************

    Reads comms data from CM DB view files in "dir" and stores them in 
    the corresponding site in array "sites" of all sites. "n_sites" is
    the total number of sites. It returns 0 on success or -1 on failure.

************************************************************************/

int RCS_read_cm_comms (char *dir, site_info_t *sites, int n_sites) {
    char fname[LOCAL_NAME_SIZE], buf[TMP_BUF_SIZE], *not_used;
    int cnt;
    FILE *fl;

    Set_comms_default (dir, sites, n_sites);

    sprintf (fname, "%s/%s", dir, "commslink.csv");
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "open %s failed\n", fname);
	exit (1);
    }
    cnt = 0;
    printf ("Reading file %s...\n", fname);
    while (my_fgets (buf, TMP_BUF_SIZE, fl) != NULL) {
	site_info_t *s, *s1;
	int link;

	cnt++;
	if (cnt == 1)
	    continue;

	Reset_get_tok (buf);

	s = RCS_search_site (Get_str_tok (), "RPG1", sites, n_sites);
	if (s == NULL)
	    continue;
	link = Get_int_tok ();
	if (link < 0 || link >= s->n_links)
	    continue;
	if ((s->type[link] = RF_get_enum (Ln_type, Get_str_tok ())) < 0) {
	    fprintf (stderr, "Bad line type (site %s)\n", s->name);
	    exit (1);
	}
	s->psize[link] = Get_int_tok ();
	s->baud[link] = Get_int_tok ();
	s->class[link] = Get_int_tok ();
	strncpy (s->pswd[link], Get_str_tok (), SHORT_NAME_SIZE);
	s->pswd[link][SHORT_NAME_SIZE - 1] = '\0';
	s->tout[link] = Get_int_tok ();
	if (Get_tok_err) {
	    fprintf (stderr, 
		"Comms data error (site %s, line %d)\n", s->name, link);
	    exit (1);
	}
	if ((s1 = RCS_search_site (s->name, "RPG2", sites, n_sites)) != NULL) {
	    s1->type[link] = s->type[link];
	    s1->psize[link] = s->psize[link];
	    s1->baud[link] = s->baud[link];
	    s1->class[link] = s->class[link];
	    s1->tout[link] = s->tout[link];
	    strcpy (s1->pswd[link], s->pswd[link]);
	    cnt++;
	}
	cnt++;
    }
    printf ("Comms read for %d site-links\n", cnt - 1);

    fclose (fl);
    not_used = (char *)Cm_name;		/* To suppress a compiler warning */
    return (0);
}

/************************************************************************

    Sets the Get_tok service to start to parse string "str".

************************************************************************/

static void Reset_get_tok (char *str) {
    Get_tok_p = str;
    Get_tok_err = 0;
    Tok_ind = 0;
}

/************************************************************************

    Returns the next integer token.

************************************************************************/

static int Get_int_tok () {
    char *p, c;
    int v;

    p = Get_str_tok ();
    if (sscanf (p, "%d%c", &v, &c) != 1) {
	Get_tok_err = 1;
	return (0);
    }
    return (v);
}

/************************************************************************

    Returns the next token in "Get_tok_p" as a NULL terminated string.

************************************************************************/

static char *Get_str_tok () {
    static char buf[TMP_BUF_SIZE];

    if (MISC_get_token (Get_tok_p, "S,Q\"", Tok_ind, buf, TMP_BUF_SIZE) <= 0) {
	buf[0] = '\0';
	fprintf (stderr, 
		"MISC_get_token (%d) failed: %s\n", Tok_ind, Get_tok_p);
    }
    else
	Tok_ind++;

    return (buf);
}

/************************************************************************

    Searches for site by site name and node name in site array "sites" of 
    size "n_sites". Returns the array element if found or NULL if not found.

************************************************************************/

site_info_t *RCS_search_site (char *sname, char *node,
				site_info_t *sites, int n_sites) {
    int ind;
    site_info_t ent;

    strncpy (ent.name, sname, SHORT_NAME_SIZE);
    ent.name[SHORT_NAME_SIZE - 1] = '\0';
    strncpy (ent.node, node, SHORT_NAME_SIZE);
    ent.node[SHORT_NAME_SIZE - 1] = '\0';
    if (MISC_bsearch (&ent, sites, n_sites, sizeof (site_info_t), 
				Site_cmp, &ind) == 0)
	return (NULL);
    return (sites + ind);
}

/************************************************************************

    Comparison function for site name search.

************************************************************************/

static int Site_cmp (void *s1p, void *s2p) {
    site_info_t *s1, *s2;
    int ret;

    s1 = (site_info_t *)s1p;
    s2 = (site_info_t *)s2p;
    ret = strcmp (s1->name, s2->name);
    if (ret != 0)
	return (ret);
    return (strcmp (s1->node, s2->node));
}

/********************************************************************

    Sets comms data to default from RPG template comms_link.conf for
    all "n_sites" sites in "sites".
	
********************************************************************/

static int Set_comms_default (char *dir, site_info_t *sites, int n_sites) {
    int i, comms_size;

    if (Df_site == NULL)
	Df_site = MISC_malloc (sizeof (site_info_t));
    RF_read_comms_link_conf (dir, Df_site);

    comms_size = Df_site->cat - (char *)&(Df_site->n_links);
    for (i = 0; i < n_sites; i++) {
	site_info_t *s = sites + i;
	memcpy ((char *)&(s->n_links), 
				(char *)&(Df_site->n_links), comms_size);
    }
    return (0);
}
