
/******************************************************************

    This is a tool that generate comms device configuration file 
    for specified radar site. This is the module that reads site 
    data from CM DB and configuration files.

******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/04/29 14:25:56 $
 * $Id: gdc_read_db.c,v 1.33 2010/04/29 14:25:56 ccalvert Exp $
 * $Revision: 1.33 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "dc_shared.h"
#include "gdc_def.h"

#define TMP_BUF_SIZE 512

extern int Verbose;
char *Unfound_file = NULL;
extern int Allow_missing_data_file;

static char *Roc_sites[] = {"DOP", "FOP", "NOP", "ROP", "DRX", "NUX", "DAN1", "NFR4"};
/* static char *Admscf_sites[] = {"DAN1", "DRX1", "KVBX", "LPLA", "RKSG", "RKJK", "RODN", "DKM1", "DKM2", "FPSF", "RSHI"}; */
static char *Src_dir;
static int N_fti_sites = -1;
static Fti_site_t *Fti_sites;
static int Nmt_rt_cnt = 0;

static void Set_site_variables (Rpg_site_t *s, int chan);
static void Set_fr_variables (Rpg_site_t *s);
static int Read_hosts_template_data (char *itf, char *itf_ip);
static void Replace_by_subnet_id (char *itf_ip);
static void Set_fti_variables (Rpg_site_t *s, int chan, int host_id);
static int Read_fti_data ();
static char *Get_fti_routes (char *site);
static void Set_nmt_variables (Rpg_site_t *s, int chan);
static void Add_nmt_route (char **net, char *ip, char *wt, char *desc, 
	char *rtt,
	int ind1, int ind2, Nmt_routes_t *rt, char *label, Rpg_site_t *s);
static char *Get_netmask (int n_bits);
static int Get_sitf_ip_route (subitf_t *sitf, Rpg_site_t *s, 
				char *lip, char *route, char *route2);
static int Parse_subnet_spec (char *sub_spec, char *net, char *mask);
static int Parse_ip_spec (Rpg_site_t *s, char *ip_spec, 
					char *ip, char *mask, char *rip);
static void Set_dod_net_variables (Rpg_site_t *s, int chan, int host_id);
static void Add_unfound_file (char *fname);
static void Rm_dash_r (char *str);
static void Add_one_to_ip (char *ip);
static void Modify_ip_for_chan2 (char *ip);
static int Compare_ip (char *ip1, char *ip2, int n);
static int Get_numerical_ip (char *str_ip, int *ip);


/**********************************************************************

    Reads CM BD files and other site data files in directory "dir" to 
    set the site variables for site "site". If "site" is NULL, it reads 
    the next site. If "chan" is 2, the previous site for channel 2 is
    processed. Returns 1 on success, 0 if "site" is not found.

**********************************************************************/

int GDCR_read_db (char *dir, char *site, int chan) {
    static Rpg_site_t *last_site_info = NULL;
    static int index = 0;
    Rpg_site_t *s;
    int ret;

    Src_dir = dir;
    ret = DCS_read_site_data (dir, Verbose);	/* init site data */
    if (ret == -1) {
	fprintf (stderr, "hosts.dat not found\n");
	exit (1);
    }
    if (ret == -2) {
	Add_unfound_file ("fr_site.dat");
    }

    if (site == NULL && chan == 2) {
	if (last_site_info == NULL) {
	    fprintf (stderr, "Channel 2 site data not availabel\n");
	    exit (1);
	}
	Set_site_variables (last_site_info, chan);
	return (1);
    }

    if (site != NULL) {
	s = DCS_rpg_site_data (site);
	if (s == NULL) {
	    fprintf (stderr, "Could not find site %s\n", site);
	    if (DCS_rpg_site_data_by_ind (0) == NULL)
		fprintf (stderr, " - Site data file (hosts.dat) not found?\n");
	    return (0);
	}
    }
    else {
	s = DCS_rpg_site_data_by_ind (index);
	if (s == NULL) {
	    fprintf (stderr, "All sites are processed\n");
	    return (0);
	}
	index++;
    }
    last_site_info = s;
    Set_site_variables (s, chan);
    if (!Allow_missing_data_file && Unfound_file != NULL) {
	fprintf (stderr, "Data files not found: %s\n", Unfound_file);
	exit (1);
    }

    return (1);
}

/**********************************************************************

    Sets site variable based on site data "s" from hosts.dat for channel
    "chan".

**********************************************************************/

static void Set_site_variables (Rpg_site_t *s, int chan) {
    char b[MAX_STR_SIZE], usname[MAX_STR_SIZE];
    int i, host_id;

    strcpy (usname, s->site);
    MISC_tolower (usname);
    GDCM_set_variable ("site_name", usname);
    MISC_toupper (usname);
    GDCM_set_variable ("SITE_NAME", usname);
    GDCM_set_variable (usname, "YES");
    for (i = 0; i < sizeof (Roc_sites) / sizeof (char *); i++) {
	if (strncmp (usname, Roc_sites[i], strlen (Roc_sites[i])) == 0) {
	    GDCM_set_variable ("ROC", "YES");
	    break;
	}
    }
/*
    for (i = 0; i < sizeof (Admscf_sites) / sizeof (char *); i++) {
	if (strncmp (usname, Admscf_sites[i], strlen (Admscf_sites[i])) == 0) {
	    GDCM_set_variable ("ADMSCF", "YES");
	    break;
	}
    }
*/
    host_id = chan;
    if (strstr (usname, "OP3") != NULL)
	host_id = 3;
    else if (strstr (usname, "OP4") != NULL || strstr (usname, "NFR4") != NULL)
	host_id = 4;
    else if (strstr (usname, "KOUN") != NULL)
	host_id = 5;

    sprintf (b, "%d", s->subnet_id);
    GDCM_set_variable ("SUBNET_ID", b);
    GDCM_set_variable ("SUBNET_MASK", s->subnet_mask);
    GDCM_set_variable ("MSCF_IP_O3", b);
    GDCM_set_variable ("MSCF_IP_O4", "20");

    if (s->faa == 1) {
	GDCM_set_variable ("FAA", "YES");
	if (chan == 2) {
	    GDCM_set_variable ("FAA_CH2", "YES");
	    GDCM_set_variable ("LAN_SWITCH_HOSTNAME", "lan2");
	    GDCM_set_variable ("RPG_ROUTER_HOSTNAME", "rtr2");
	}
	else {
	    GDCM_set_variable ("FAA_CH1", "YES");
	    GDCM_set_variable ("LAN_SWITCH_HOSTNAME", "lan1");
	    GDCM_set_variable ("RPG_ROUTER_HOSTNAME", "rtr1");
	}
    }
    else {
	GDCM_set_variable ("LAN_SWITCH_HOSTNAME", "lan");
	GDCM_set_variable ("RPG_ROUTER_HOSTNAME", "rtr");
    }

    GDCM_set_variable ("RADAR_LOCATION", s->location);
    if (s->bdds == 1)
	GDCM_set_variable ("BDDS", "YES");
    if (s->nws != 0)
	GDCM_set_variable ("NWS", "YES");
    if (s->nws == 2) {
	GDCM_set_variable ("NWS_RED", "YES");
	if (chan == 2)
	    GDCM_set_variable ("NWS_CH2", "YES");
	else
	    GDCM_set_variable ("NWS_CH1", "YES");
    }
    if (DCS_check_fr_site (CFS_LOAD_HUB_RPG, usname))
	GDCM_set_variable ("HLRPG", "YES");
    if (DCS_check_fr_site (CFS_LOAD_HUB_MSCF, usname))
	GDCM_set_variable ("HLMSCF", "YES");
    if (DCS_check_fr_site (CFS_LOAD_HUB_MANUAL, usname))
	GDCM_set_variable ("HLMANUAL", "YES");

    if (s->awips_ip[0] != '\0' && strcmp (s->awips_ip, "0") != 0)
	GDCM_set_variable ("AWIPS_IP", s->awips_ip);
    else
	GDCM_set_variable ("AWIPS_IP", "10.200.200.200");
    if (s->awips_gate[0] != '\0' && strcmp (s->awips_gate, "0") != 0)
	GDCM_set_variable ("AWIPS_GATE", s->awips_gate);

    if (s->nws == 0 && s->faa == 0)
	GDCM_set_variable ("DOD", "YES");

    Set_fti_variables (s, chan, host_id);

    Set_fr_variables (s);
    Set_nmt_variables (s, chan);

    Set_dod_net_variables (s, chan, host_id);
}

/**********************************************************************

    Sets FTI site variable based on site data "s" from hosts.dat.
    FAA_INT, FAA_NET and FAA_SUBNET are not used for processing the 
    generic configuration files.

**********************************************************************/

static void Set_fti_variables (Rpg_site_t *s, int chan, int host_id) {
    char *fti_routes, itf_ip[MAX_STR_SIZE], b[MAX_STR_SIZE];

    GDCM_set_variable ("FAA_NET", s->faa_net);
    GDCM_set_variable ("FTI_YES", "NO");
    if (s->faa_net[0] == '\0' || strcmp (s->faa_net, "0") == 0)
	return;

    GDCM_set_variable ("FTI_YES", "YES");
    sprintf (b, "%d", s->faa_subnet);
    GDCM_set_variable ("FAA_SUBNET", b);
    sprintf (b, "%d", s->faa_subnet_mask);
    GDCM_set_variable ("FAA_SUBNET_MASK", b);

    /* deduced FTI variables */
    sprintf (b, "rpg%d-eth1", chan);
    GDCM_set_variable ("FAA_INT", b);
    sprintf (b, "%s%d", GDCM_get_value ("FAA_NET"), 
				    s->faa_subnet + host_id);
    GDCM_set_variable ("FAA_IP", b);
    sprintf (b, "%s%d", GDCM_get_value ("FAA_NET"), s->faa_subnet);
    GDCM_set_variable ("WARP_NET", b);
    sprintf (b, "%s%d", GDCM_get_value ("FAA_NET"), 
			    255 + s->faa_subnet - s->faa_subnet_mask - 1);
    GDCM_set_variable ("FAA_GATEWAY", b);
    if ((fti_routes = Get_fti_routes (s->site)) != NULL &&
	fti_routes[0] != '\0')
	GDCM_set_variable ("FTI_RT", fti_routes);

    if (Read_hosts_template_data (GDCM_get_value ("FAA_INT"), itf_ip) != 0) {
	Replace_by_subnet_id (itf_ip);
	GDCM_set_variable ("FTI_LOCAL_IP", itf_ip);
    }
}

/**********************************************************************

    Sets frame relay site variable based on site data "s" from hosts.dat.

**********************************************************************/

static void Set_fr_variables (Rpg_site_t *s) {
    static char *sitf_ip = NULL, *sitf_route = NULL;
    static char *sitf_route2 = NULL, *rsite = NULL;
    Frc_site_t *fr;
    char dlci[MAX_STR_SIZE], subitf_id[MAX_STR_SIZE];
    char is_brg[MAX_STR_SIZE], b[MAX_STR_SIZE];
    int n_sitfs, i;

    if (DCS_check_fr_site (CFS_HUB_RPG, s->site))
	GDCM_set_variable ("FR_HUB_RPG", "YES");

    if (DCS_check_fr_site (CFS_LOAD_HUB_RPG, s->site))
	GDCM_set_variable ("FR_LOAD_HUB_RPG", "YES");

    GDCM_set_variable ("FRC_SITE", "NO");

    /* for frame relay circuit sites */
    if (!DCS_check_fr_site (CFS_FR_CIRCUIT, s->site))
	return;

    fr = DCS_frc_site_data (s->site);
    if (fr == NULL) {
	fprintf (stderr, 
	    "Frame Relay circuit site data not found for %s\n", s->site);
	exit (1);
    }
    sprintf (b, "%d", fr->bw);
    GDCM_set_variable ("FRC_BW", b);
    GDCM_set_variable ("FRC_TSLOT", fr->slots);
    n_sitfs = fr->n_cs;
    if (n_sitfs < 0 || n_sitfs > 10) {
	fprintf (stderr, "Too many subinterfaces for site %s\n", s->site);
	exit (1);
    }
    if (fr->n_cs == 0) {
	fprintf (stderr, 
	    "Zero subinterface is no longer allowed (FRC site %s)\n", s->site);
	exit (1);
    }
    else
	GDCM_set_variable ("FRC_SUBITF", "YES");
    GDCM_set_variable ("FRC_SITE", "YES");
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

	is_rule_08 = Get_sitf_ip_route (&(fr->sitf[i]), s, lip, route, route2);
	sprintf (b, "\"%s\" ", lip);
	sitf_ip = STR_cat (sitf_ip, b);
	sprintf (b, "\"%s\" ", route);
	sitf_route = STR_cat (sitf_route, b);
	sprintf (b, "\"%s\" ", route2);
	sitf_route2 = STR_cat (sitf_route2, b);

	GDCM_set_variable ("FRC_RSITE", rsite);
	GDCM_set_variable ("FRC_DLCI", dlci);
	if (fr->n_cs > 0 || is_rule_08) {
	    GDCM_set_variable ("FRC_SUBITF_ID", subitf_id);
	    GDCM_set_variable ("FRC_SUBITF_BG", is_brg);
	    GDCM_set_variable ("FRC_SUBITF_IP", sitf_ip);
	    GDCM_set_variable ("FRC_SUBITF_ROUTE", sitf_route);
	    GDCM_set_variable ("FRC_SUBITF_ROUTE2", sitf_route2);
	}
    }
}

/**********************************************************************

    Evaluates the local subinterface IP and mask, the remote IP, the 
    remote subnet and subnet mask, and the second remote subnet and subnet 
    mask for subinterface "sitf" of FRC site "s".

**********************************************************************/

static int Get_sitf_ip_route (subitf_t *sitf, Rpg_site_t *s, 
				char *lip, char *route, char *route2) {
    char ip[MAX_STR_SIZE], mask[MAX_STR_SIZE], rip[MAX_STR_SIZE];
    char rnet[MAX_STR_SIZE], rnet_mask[MAX_STR_SIZE];
    char rip2[MAX_STR_SIZE], rnet2[MAX_STR_SIZE];
    char rnet_mask2[MAX_STR_SIZE], b[MAX_STR_SIZE];

    if (strcmp (sitf->sitf_ip, "rule_08") == 0) {	/* The 2008 rule */
	char *pt, b[32];
	Rpg_site_t *rem;
	int oct3, oct4;

	if (MISC_get_token (sitf->rsite, "S.Ci", 0, &oct3, 0) >= 0 &&
	    MISC_get_token (sitf->rsite, "S.Ci", 1, &oct4, 0) >= 0) {
	    rem = s;
	}
	else {
	    rem = DCS_rpg_site_data (sitf->rsite);
	    oct3 = rem->subnet_id;
	    oct4 = 35;
	}

	sprintf (ip, "172.21.%d.101", s->subnet_id);
	sprintf (rip, "172.21.%d.100", s->subnet_id);

	strcpy (b, rem->awips_ip);
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
	GDCM_set_variable ("MSCF_IP_O3", b);
	sprintf (b, "%d", oct4);
	GDCM_set_variable ("MSCF_IP_O4", b);
	return (1);
    }

    strcpy (mask, "255.255.255.128");	/* default interface mask */
    ip[0] = '\0';
    if (strcmp (sitf->rsite, "smg") == 0) {
	sprintf (ip, "172.20.%d.100", s->subnet_id);
	sprintf (rip, "172.20.%d.101", s->subnet_id);
	sprintf (rnet, "165.92.220.0");
	sprintf (rnet_mask, "255.255.255.0");
    }
    else if (strcmp (sitf->rsite, "roc") == 0) {
	strcpy (mask, "255.255.255.252");	/* interface mask */
	sprintf (ip, "172.21.%d.201", s->subnet_id);
	sprintf (rip, "172.21.%d.202", s->subnet_id);
	sprintf (rnet, "172.25.187.0");
	sprintf (rnet_mask, "255.255.255.128");
    }
    else if (MISC_get_token (sitf->rsite, "S.", 0, NULL, 0) == 4) {
	if (Parse_subnet_spec (sitf->rsite, rnet, rnet_mask) < 0) {
	    fprintf (stderr, 
	      "Bad sub-net spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->rsite, sitf->dlci, s->site);
	    exit (1);
	}
    }
    else {
	char *pt;
	Rpg_site_t *rem = DCS_rpg_site_data (sitf->rsite);
	if (rem == NULL) {
	    fprintf (stderr, 
	      "Invalid remote RPG site (%s) used by FR circuit site %s (dlci %d)\n",
				    sitf->rsite, s->site, sitf->dlci);
	    exit (1);
	}
	sprintf (ip, "172.20.%d.100", rem->subnet_id);
	sprintf (rip, "172.20.%d.101", rem->subnet_id);
	strcpy (b, rem->awips_ip);
	pt = b + strlen (b) - 1;
	while (pt > b && *pt != '.')
	    pt--;
	*pt = '\0';
	sprintf (rnet, "%s.0", b); 
	strcpy (rnet_mask, "255.255.255.0"); 
    }

    if (sitf->sitf_ip[0] != '\0') {
	if (Parse_ip_spec (s, sitf->sitf_ip, ip, mask, rip) < 0) {
	    fprintf (stderr, 
	      "Bad sub-itf IP spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->sitf_ip, sitf->dlci, s->site);
	    exit (1);
	}
    }
    if (ip[0] == '\0') {
	fprintf (stderr, 
	      "Sub-itf IP not specified (dlci %d, FR circuit site %s)\n",
				    sitf->dlci, s->site);
	exit (1);
    }

    /* second route */
    rnet2[0] = '\0';
    if (sitf->net2[0] != '\0') {
	char not_used[MAX_STR_SIZE];
	if (Parse_subnet_spec (sitf->net2, rnet2, rnet_mask2) < 0) {
	    fprintf (stderr, 
	      "Bad sub-net spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->net2, sitf->dlci, s->site);
	    exit (1);
	}
	if (Parse_ip_spec (s, sitf->ritf2, not_used, not_used, rip2) < 0) {
	    fprintf (stderr, 
	      "Bad sub-itf IP spec (%s, dlci %d, FR circuit site %s)\n",
				    sitf->ritf2, sitf->dlci, s->site);
	    exit (1);
	}
    }

    if (strcmp (sitf->rsite, "roc") != 0)
	Modify_ip_for_chan2 (ip);
/*
    if (GDCM_get_value ("FAA_CH2") != NULL) {
	fprintf (stderr, 
	      "sub-itf spec not allowed for channel 2 (dlci %d, FR circuit site %s)\n",
				    sitf->dlci, s->site);
	exit (1);
    }
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

static int Parse_ip_spec (Rpg_site_t *s, char *ip_spec, 
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
	i2 = s->subnet_id;
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

    Sets network management traffic routing variables based on site 
    data "s" from hosts.dat. We don't do it for channel 2.

**********************************************************************/

#define MAX_ROUTES (MAX_STR_SIZE / 12)

void Set_nmt_variables (Rpg_site_t *s, int chan) {
    static char *net = NULL;
    char ip[MAX_STR_SIZE];
    char wt[MAX_STR_SIZE], desc[MAX_STR_SIZE];
    char rtt[MAX_STR_SIZE];
    Nmt_routes_t *rt;

    if (chan == 2)
	return;

    Nmt_rt_cnt = 0;
    net = STR_copy (net, "");
    ip[0] = '\0';
    wt[0] = '\0';
    desc[0] = '\0';
    rtt[0] = '\0';
    DCS_get_nmt_data (NULL, 0);
    while ((rt = DCS_get_nmt_data (s->site, 0)) != NULL) {
	if (rt->site[1][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 1, 1, rt, "P-2N-2IP", s);
	if (rt->site[2][0] != '\0' &&
	    rt->flag[2] != NMT_HR)
	    Add_nmt_route (&net, ip, wt, desc, rtt, 2, 1, rt, "P-3N-2IP", s);
    }

    DCS_get_nmt_data (NULL, 0);
    while ((rt = DCS_get_nmt_data (s->site, 1)) != NULL) {
	if (rt->site[2][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 2, 2, rt, "S-3N-3IP", s);
    }

    DCS_get_nmt_data (NULL, 0);
    while ((rt = DCS_get_nmt_data (s->site, 2)) != NULL) {
	if (rt->site[3][0] != '\0')
	    Add_nmt_route (&net, ip, wt, desc, rtt, 3, 3, rt, "T-4N-4IP", s);
    }

    if (net[0] != '\0')
	GDCM_set_variable ("NMTP_NET", net);
    if (ip[0] != '\0')
	GDCM_set_variable ("NMTP_IP", ip);
    if (wt[0] != '\0')
	GDCM_set_variable ("NMTP_WT", wt);
    if (desc[0] != '\0')
	GDCM_set_variable ("NMTP_DESC", desc);
    if (rtt[0] != '\0')
	GDCM_set_variable ("NMTP_DEVICE_TYPE", rtt);

    return;
}

/**********************************************************************

    Adds a new NMT route if it is not already added.

**********************************************************************/

static void Add_nmt_route (char **net, char *ip, char *wt, char *desc,
	char *rtt,
	int ind1, int ind2, Nmt_routes_t *rt, char *label, Rpg_site_t *s) {
    static struct {
	char *subnet;
	char *ip;
	int wt;
    } set[MAX_ROUTES];
    int i;
    char snet[MAX_STR_SIZE];

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
    if (Nmt_rt_cnt >= MAX_ROUTES) {
	fprintf (stderr, "Too many NMT routes for %s\n", s->site);
	exit (1);
    }
    set[Nmt_rt_cnt].subnet = MISC_malloc (strlen (snet) + 1);
    strcpy (set[Nmt_rt_cnt].subnet, snet);
    set[Nmt_rt_cnt].ip = rt->ip[ind2];
    set[Nmt_rt_cnt].wt = rt->wt[ind2];
    Nmt_rt_cnt++;
}

/******************************************************************

    Looks for host name "itf" in file "hosts.tmpl" in "Src_dir" and
    returns the IP address with "itf_ip".

******************************************************************/

static int Read_hosts_template_data (char *itf, char *itf_ip) {
    char fname[MAX_STR_SIZE];
    FILE *fd;
    char buf[TMP_BUF_SIZE];

    itf_ip[0] = '\0';
    strcpy (fname, "hosts.tmpl");
    DCS_add_dir (Src_dir, fname);
    fd = fopen (fname, "r");
    if (fd == NULL) {
	strcpy (fname, "../site_templates/hosts.tmpl");
	DCS_add_dir (Src_dir, fname);
	fd = fopen (fname, "r");
    }
    if (fd == NULL) {
	Add_unfound_file (fname);
	return (0);
    }
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	int ind;

	if (MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;

	ind = 1;
	while (MISC_get_token (buf, "", ind, tk, MAX_STR_SIZE) > 0) {
	    if (strcmp (itf, tk) == 0) {
		MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE);
		DCS_str_cat (itf_ip, tk);
		goto done;
	    }
	    ind++;
	}
    }

done:

    fclose (fd);
    if (itf_ip[0] == '\0') {
	fprintf (stderr, "IP address for %s not found in %s\n", itf, fname);
	exit (1);
    }
    return (1);
}

/******************************************************************

    Replaces the third octat in "itf_ip" by SUBNET_ID.

******************************************************************/

static void Replace_by_subnet_id (char *itf_ip) {
    char *p, *p1, *p2, buf[MAX_STR_SIZE];
    int cnt;

    p = itf_ip;
    p1 = p2 = NULL;
    cnt = 0;
    while (*p != '\0') {
	if (*p == '.') {
	    cnt++;
	    if (cnt == 2)
		p1 = p;
	    if (cnt == 3)
		p2 = p;
	}
	p++;
    }
    if (cnt < 3 || strlen (itf_ip) + 32 > MAX_STR_SIZE) {
	fprintf (stderr, "Bad IP (%s) returned from hosts.tmpl\n", itf_ip);
	exit (1);
    }
    *p1 = '\0';
    *p2 = '\0';
    sprintf (buf, "%s.%s.%s", itf_ip, GDCM_get_value ("SUBNET_ID"), p2 + 1);
    strcpy (itf_ip, buf);
}

/******************************************************************

    Returns the FTI routes for site "site".

******************************************************************/

static char *Get_fti_routes (char *site) {
    int i;
    char buf[MAX_STR_SIZE];

    if (N_fti_sites < 0)
	Read_fti_data ();

    strcpy (buf, site);
    MISC_tolower (buf);
    for (i = 0; i < N_fti_sites; i++) {
	if (strcmp (Fti_sites[i].site, buf) == 0)
	    return (Fti_sites[i].routes);
    }
    return (NULL);
}

/******************************************************************

    Reads "fipr.dat" in "dir" and initializes the FTI site data 
    structures.

******************************************************************/

static int Read_fti_data () {
    char fname[MAX_STR_SIZE];
    FILE *fd;
    char buf[TMP_BUF_SIZE];
    Fti_site_t *s;

    if (N_fti_sites >= 0)
	return (0);

    Fti_sites = (Fti_site_t *)DCS_malloc 
			(MAX_N_RPG_SITES * sizeof (Fti_site_t));
    N_fti_sites = 0;
    strcpy (fname, "fipr.dat");
    DCS_add_dir (Src_dir, fname);
    fd = fopen (fname, "r");
    if (fd == NULL) {
	Add_unfound_file (fname);
	return (0);
    }
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE];
	int ind;

	if (MISC_get_token (buf, "S,", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;
	if (N_fti_sites >= MAX_N_RPG_SITES) {
	    fprintf (stderr, "Too many FTI sites\n");
	    exit (1);
	}

	s = Fti_sites + N_fti_sites;
	s->site = (char *)DCS_malloc (strlen (buf) + 1);
	MISC_get_token (buf, "S,", 0, tk, MAX_STR_SIZE);
	strcpy (s->site, tk);
	s->routes = s->site + strlen (tk) + 1;
	s->routes[0] = '\0';
	ind = 1;
	while (MISC_get_token (buf, "S,", ind, tk, MAX_STR_SIZE) > 0) {
	    strcat (s->routes, tk);
	    strcat (s->routes, " ");
	    ind++;
	}
	N_fti_sites++;
    }

    fclose (fd);
    return (0);
}

/**********************************************************************

    Sets DOD NET site variable based on site data "s" from hosts.dat.

**********************************************************************/

static void Set_dod_net_variables (Rpg_site_t *s, int chan, int host_id) {
    char fname[MAX_STR_SIZE];
    FILE *fd;
    char buf[TMP_BUF_SIZE], ip[TMP_BUF_SIZE], *p;
    char subnet[32], mscf_subnet[32], type[32], mscf_ip[32];

    strcpy (fname, "dod_net.dat");
    DCS_add_dir (Src_dir, fname);
    fd = fopen (fname, "r");
    if (fd == NULL) {
	strcpy (fname, "dod_nat.dat");		/* try a different name */
	DCS_add_dir (Src_dir, fname);
	fd = fopen (fname, "r");
    }
    if (fd == NULL) {
	Add_unfound_file (fname);
	return;
    }
    GDCM_set_variable ("DODN_YES", "NO");
    type[0] = '\0';
    while (fgets (buf, TMP_BUF_SIZE, fd) != NULL) {
	char tk[MAX_STR_SIZE], *v_name[10];
	int i, err;

	if (MISC_get_token (buf, "S,", 0, tk, MAX_STR_SIZE) <= 0 ||
	    tk[0] == '#')
	    continue;

	if (strcasecmp (tk, s->site) != 0)
	    continue;

	v_name[0] = "DODN_SUBNET";
	v_name[1] = "DODN_SNM";
	v_name[2] = "DODN_NAT_IP";
	v_name[3] = "DODN_GW";
	v_name[4] = "DDMSCF_TYPE";
	v_name[5] = "DDMSCF_SUBNET";
	v_name[6] = "DDMSCF_SNM";
	v_name[7] = "DDMSCF_NAT_IP";
	v_name[8] = "DDMSCF_GW";
	err = 0;
	subnet[31] = mscf_subnet[31] = type[31] = mscf_ip[31] = '\0';
	for (i = 0; i <= 8; i++) {
	    if (MISC_get_token (buf, "S,", i + 1, tk, MAX_STR_SIZE) <= 0) {
		if (i == 4)
		    break;
		fprintf (stderr, "Could not find %s for site %s in %s\n",
					v_name[i], s->site, fname);
		exit (1);
	    }
	    if (i == 4) {
		if (strlen (tk) == 0)
		    break;
		strncpy (type, tk, 31);
		if (strcmp (type, "ANALOG") == 0) {
		    char b[32];
		    GDCM_set_variable ("ADMSCF", "YES");
		    sprintf (b, "%d", s->subnet_id);
		    GDCM_set_variable ("MSCF_IP_O3", b);
		    GDCM_set_variable ("MSCF_IP_O4", "20");
		}
	    }
	    else if (MISC_get_token (tk, "S.", 0, NULL, 0) != 4) {
		if (i > 4 && strcmp (type, "ANALOG") == 0)
		    break;
		fprintf (stderr, "Unexpected %s (%s) for site %s in %s\n",
					v_name[i], tk, s->site, fname);
		exit (1);
	    }
	    else {
		if (i == 0)
		    strncpy (subnet, tk, 31);
		else if (i == 5) {
		    strncpy (mscf_subnet, tk, 31);
		    err += Compare_ip (tk, subnet, 2);
		}
		else if (i == 1 || i == 6)
		    err += Compare_ip (tk, "255.255.255", 3);
		else if (i >= 2 && i <= 3)
		    err += Compare_ip (tk, subnet, 3);
		else if (i >= 7 && i <= 8)
		    err += Compare_ip (tk, mscf_subnet, 3);
	    }
	    if (i == 7)
		strncpy (mscf_ip, tk, 31);
	    if (i == 3 || i == 8)
		Rm_dash_r (tk);
	    if (chan == 2 && strcmp (v_name[i], "DODN_NAT_IP") == 0)
		Add_one_to_ip (tk);
	    if (i <= 4 || strcmp (type, "NETWORK") == 0 || 
				strcmp (type, "DOD_NETWORK") == 0)
		GDCM_set_variable (v_name[i], tk);
	}
	if (err) {
	    fprintf (stderr, 
		"Warning: Data may be incorrect for site %s in %s\n",
						s->site, fname);
	}
	GDCM_set_variable ("DODN_YES", "YES");
	break;
    }
    fclose (fd);

    if ((p = GDCM_get_value ("MSCF_IP_O3")) != NULL) {
	sprintf (buf, "%d.%d.%s", 172, 25, p);
	GDCM_set_variable ("MSCF_SUBNET", buf);
    }

    if (strcmp (type, "NETWORK") == 0 || strcmp (type, "DOD_NETWORK") == 0) {
	int nip[4];
	if (Get_numerical_ip (mscf_ip, nip) < 0) {
	    fprintf (stderr, 
		"mscf subnet (%s) is not a valid IP for site %s in %s\n",
						mscf_subnet, s->site, fname);
	    exit (1);
	}
	sprintf (buf, "%d", nip[2]);
	GDCM_set_variable ("MSCF_IP_O3", buf);
	sprintf (buf, "%d", nip[3]);
	GDCM_set_variable ("MSCF_IP_O4", buf);
	sprintf (buf, "%d.%d.%d", nip[0], nip[1], nip[2]);
	GDCM_set_variable ("MSCF_SUBNET", buf);
    }

    sprintf (buf, "rpga%d-dod", chan);
    if (Read_hosts_template_data (buf, ip) != 0) {
	Replace_by_subnet_id (ip);
	GDCM_set_variable ("DODN_LOCAL_IP", ip);
    }

    return;
}

/**********************************************************************

    Coverts string IP to numerical IP.

**********************************************************************/

static int Get_numerical_ip (char *str_ip, int *ip) {

    if (MISC_get_token (str_ip, "S.Ci", 0, ip, 0) <= 0 ||
	MISC_get_token (str_ip, "S.Ci", 1, ip + 1, 0) <= 0 ||
	MISC_get_token (str_ip, "S.Ci", 2, ip + 2, 0) <= 0 ||
	MISC_get_token (str_ip, "S.Ci", 3, ip + 3, 0) <= 0)
	return (-1);
    return (0);
}

/**********************************************************************

    Compares the first "n" octets of "ip1" and "ip2". Returns 0 if they
    are the same or 1 if not.

**********************************************************************/

static int Compare_ip (char *ip1, char *ip2, int n) {
    int o1, o2, i;

    for (i = 0; i < n; i++) {
	if (MISC_get_token (ip1, "S.Ci", i, &o1, 0) <= 0 ||
	    MISC_get_token (ip2, "S.Ci", i, &o2, 0) <= 0 ||
	    o1 != o2)
	    return (1);
    }
    return (0);
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

    Modifies "ip" for FAA channel 2 by adding add 1 to the last octet.

**********************************************************************/

static void Modify_ip_for_chan2 (char *ip) {

    if (GDCM_get_value ("FAA_CH2") == NULL)
	return;
    Add_one_to_ip (ip);
}

/**********************************************************************

    Add "fname" to the list of unfound data files.

**********************************************************************/

static void Add_unfound_file (char *fname) {

    if (Unfound_file != NULL && strstr (Unfound_file, fname) != NULL)
	return;
    if (Unfound_file != NULL) {
	Unfound_file = STR_cat (Unfound_file, " ");
	Unfound_file = STR_cat (Unfound_file, fname);
    }
    else
	Unfound_file = STR_copy (Unfound_file, fname);
}

/**********************************************************************

    Removes trailing "\r" in string "str".

**********************************************************************/

static void Rm_dash_r (char *str) {
    int len;

    len = strlen (str);
    if (len > 0 && str[len - 1] == '\r')
	str[len - 1] = '\0';
}




