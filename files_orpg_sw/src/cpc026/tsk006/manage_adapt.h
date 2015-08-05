
/*************************************************************************

    Manage adaptation data tools shared header file.

**************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/01/08 22:33:47 $
 * $Id: manage_adapt.h,v 1.4 2010/01/08 22:33:47 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef MANAGE_ADAPT_H
#define MANAGE_ADAPT_H

#define LOCAL_NAME_SIZE 256
#define SHORT_NAME_SIZE 8
#define PSWD_SIZE 16
#define N_MLOS 4
#define CAT_SIZE 4
#define MAX_N_LINKS 100

#ifdef NEED_SITE_STRING_CONST
static char *Wx_mode[] = {"Clear Air", "Precipitation", ""};
static char *Yes_no[] = {"No", "Yes", ""};
static char *Redun_t[] = {"No Redundancy", "FAA Redundant", "NWS Redundant", ""};
static char *Channel[] = {"Channel 1", "Channel 2", ""};
static char *Mlos_t[] = {"None", "RPG NON DIV", "RPG DIV", "RDA NON DIV", "RDA DIV", "RPT DIV", "RPT ND/DIV", "RPG ND/DIV", ""};
#endif

#ifdef NEED_COMMS_STRING_CONST
static char *Ln_type[] = {"Dedic", "D-in", "WAN", ""};
static char *Cm_name[] = {"cm_atlas", "cm_uconx", "cm_tcp", ""};
#endif

typedef struct {
    char name[SHORT_NAME_SIZE];
    int lat;
    int lon;
    int elev;
    int rpg_id;
    int wx_mode;
    int vcp_a;
    int vcp_b;
    int mlos;
    int rms;
    int bdds;
    int orda;
    int enable_sr;
    int a_3;
    char node[SHORT_NAME_SIZE];
    int p_code;
    int redun_t;
    int channel;
    int n_mlos;
    int mlos_t[N_MLOS];
    char roc_pwd[PSWD_SIZE];
    char agency_pwd[PSWD_SIZE];
    char urc_pwd[PSWD_SIZE];

    int n_links;
    int psn[MAX_N_LINKS];
    int cmn[MAX_N_LINKS];
    int devn[MAX_N_LINKS];
    int portn[MAX_N_LINKS];
    int type[MAX_N_LINKS];
    int baud[MAX_N_LINKS];
    int cm_name[MAX_N_LINKS];
    int psize[MAX_N_LINKS];
    int npvc[MAX_N_LINKS];
    int lstate[MAX_N_LINKS];
    int ntf[MAX_N_LINKS];
    int class[MAX_N_LINKS];
    int tout[MAX_N_LINKS];
    char pswd[MAX_N_LINKS][SHORT_NAME_SIZE];
    int rda_link;

    char cat[CAT_SIZE];
    int src;

} site_info_t;

#define SRC_PREV_BUILD 0x1	/* for site_info_t.src */
#define SRC_CM_DB 0x2		/* for site_info_t.src */

#define UNAME_SIZE 32
#define UDESC_SIZE 64
#define MAX_N_USERS 1000

typedef struct {
    int id;
    int dial;
    char name[UNAME_SIZE];
    char desc[UDESC_SIZE];
    char pswd[UNAME_SIZE];
    char site[SHORT_NAME_SIZE];
    char node[SHORT_NAME_SIZE];
} User_t;

int RF_read_site_master_dir (char *d_name, site_info_t *sites, int buf_size);
int WF_create_site_adapt (site_info_t *s, char *work_dir);
int WF_tar_compress_site_adapt (site_info_t *s, char *work_dir);
int WF_read_template_files (char *dir);
int RCS_read_cm_sites (char *dir, site_info_t *sites, int max_sites);
int RF_get_enum (char **enums, char *str);
int RF_sort_verify_sites (site_info_t *sites, int n_sites);
int RF_read_comms_link_conf (char *dir, site_info_t *site);

int MAIN_is_site_selected (site_info_t *s);
int MAIN_read_user_ids ();
int MAIN_no_cat_in_creation ();
char *MAIN_get_version ();
site_info_t *RCS_search_site (char *sname, char *node,
				site_info_t *sites, int n_sites);
int RCS_read_cm_hci_pswd (char *dir, site_info_t *sites, int n_sites);
int RCS_read_cm_comms (char *dir, site_info_t *sites, int n_sites);
int RCS_read_cm_users (char *dir, char *name, User_t *users, int max_users);
int WF_create_user_table (char *odir, char *name, User_t *users, int n_users);


#endif /* #ifndef MANAGE_ADAPT_H */
