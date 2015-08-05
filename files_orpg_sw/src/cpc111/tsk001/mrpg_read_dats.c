
/******************************************************************

	file: mrpg_read_dats.c

	Reads in data and comms configuration tables: DAT, PAT and
	comms_link.conf.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/07/14 20:10:59 $
 * $Id: mrpg_read_dats.c,v 1.23 2011/07/14 20:10:59 jing Exp $
 * $Revision: 1.23 $
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
#include <fcntl.h>

#include <orpg.h> 
#include <infr.h> 
#include "mrpg_def.h"

#define TMP_BUF_SIZE 128

static Mrpg_dat_entry_t *Dat;		/* Data attribute table */
static int N_dat = 0;			/* size of Data attribute table */
static void *Dat_tbl;			/* Data attribute table id */

static Mrpg_dat_entry_t *Pat;		/* Product attribute table */
static int N_pat = 0;			/* size of Product attribute table */
static void *Pat_tbl;			/* product attribute table id */

static Mrpg_comms_link_t *Cms;		/* comms links table */
static int N_cms = 0;			/* size of comms links table */

static int RDA_link;			/* RDA comms link number */
static int Read_extension_table = 0;

static char *Data_table_basename = "data_attr_table";
static char *Product_table_basename = "product_attr_table";
static char Cfg_extensions[MRPG_NAME_SIZE] = "extensions";
static char Data_table_name[MRPG_NAME_SIZE];
static char Product_table_name[MRPG_NAME_SIZE];
static char Comms_config_name[MRPG_NAME_SIZE] = "comms_link.conf";


static void Set_lb_types (int *types, char *tmp);
static void Get_default_lb_attr (LB_attr *attr);
static int Read_DAT ();
static int Read_PAT ();
static int Read_comms_link_table ();
static int Process_DAT ();
static int Read_datfile (char *filename);
static int Read_patfile (char *filename);
static int Read_de_comms_info ();
static int Get_de_value (char *de_id, int is_str_type, void *values);
static void Free_product_entry (int ind);
static void Free_data_entry (int ind);
static int Check_shared_file (Mrpg_dat_entry_t *d, Mrpg_dat_entry_t *dn);
static int Close_cs_file (int ret);


/******************************************************************

    Initializes this module.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MRD_init () {
    int len, i, k;
    char dir[MRPG_NAME_SIZE], tmp[MRPG_NAME_SIZE];

    len = MISC_get_cfg_dir (dir, MRPG_NAME_SIZE);
    if (len <= 0) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir not found\n");
	return (-1);
    }
    if (len + strlen (Cfg_extensions) + 3 > MRPG_NAME_SIZE ||
	len + strlen (Data_table_basename) + 3 > MRPG_NAME_SIZE ||
	len + strlen (Product_table_basename) + 3 > MRPG_NAME_SIZE ||
	len + strlen (Comms_config_name) + 3 > MRPG_NAME_SIZE) {
	LE_send_msg (GL_ERROR, "ORPG cfg dir too long\n");
	return (-1);
    }
    strcpy (tmp, dir);
    strcat (tmp, "/");
    strcat (tmp, Cfg_extensions);
    strcpy (Cfg_extensions, tmp);
    if (MRD_get_table_file_name (Data_table_name, dir, 
			Data_table_basename, tmp, MRPG_NAME_SIZE) < 0 ||
	MRD_get_table_file_name (Product_table_name, dir, 
			Product_table_basename, tmp, MRPG_NAME_SIZE) < 0)
	return (-1);
    strcpy (tmp, dir);
    strcat (tmp, "/");
    strcat (tmp, Comms_config_name);
    strcpy (Comms_config_name, tmp);

    if (Read_DAT () < 0) {
	LE_send_msg (GL_ERROR, "Failed in reading data attributes\n");
	return (-1);
    }
    if (Read_PAT () < 0) {
	LE_send_msg (GL_ERROR, "Failed in reading product attributes\n");
	return (-1);
    }

    /* remove duplicated data store specs */
    for (i = 0; i < N_dat; i++) {
	int data_id;
	Mrpg_dat_entry_t *d;

	d = Dat + i;
	data_id = d->data_id;
	for (k = 0; k < N_pat; k++) {
	    if (Pat[k].data_id == data_id)
		break;
	}
	if (k < N_pat && d->data_id > 0)
	    d->data_id = -d->data_id;
    }

    /* Check shared file by multiple data items */
    for (i = 0; i < N_pat; i++) {
	for (k = i + 1; k < N_pat; k++) {
	    if (Check_shared_file (Pat + i, Pat + k) < 0)
		return (-1);
	}
	for (k = 0; k < N_dat; k++) {
	    if (Check_shared_file (Pat + i, Dat + k) < 0)
		return (-1);
	}
    }
    for (i = 0; i < N_dat; i++) {
	for (k = i + 1; k < N_dat; k++) {
	    if (Check_shared_file (Dat + i, Dat + k) < 0)
		return (-1);
	}
    }

    /* sets up default number of messages */
    for (i = 0; i < N_pat; i++) {
	if (Pat[i].attr.maxn_msgs == 0)
	     Pat[i].attr.maxn_msgs = 10;
    }
    for (i = 0; i < N_dat; i++) {
	if (Dat[i].attr.maxn_msgs == 0)
	     Dat[i].attr.maxn_msgs = 40;
    }

    return (0);
}

/**********************************************************************

    Data stores may share the same physical file. In this case the file
    does not need to be created multiple times. The attributes for these
    data stores must be consistent. This function checks any inconsitency
    and sets the no_create flag. Returns 0 on success or -1 on failure.
	
**********************************************************************/

#define CHECK_ATTR(fld,emsg) if (at->fld != def->fld) {\
		if (atn->fld != def->fld && atn->fld != at->fld)\
		    err = emsg;\
	    }\
	    else\
		at->fld = atn->fld;

static int Check_shared_file (Mrpg_dat_entry_t *d, Mrpg_dat_entry_t *dn) {
    static LB_attr *def = NULL, def_buf;
    char *err;
    LB_attr *at, *atn;

    if (def == NULL) {
	def = &def_buf;
	memset (&(def->remark), 0, LB_REMARK_LENGTH);
	Get_default_lb_attr (def);
    }

    if (d->no_create)
	return (0);

    if (strcmp (dn->path, d->path) != 0)
	return (0);
    err = NULL;
    if (dn->persistent != d->persistent)
	err = "persistent";
    if (dn->mrpg_init != d->mrpg_init)
	err = "mrpg_init";
    if (dn->compr_code != d->compr_code)
	err = "compr_code";
    if (dn->wp_size != d->wp_size)
	err = "wp_size";
    if (dn->wp != d->wp)
	err = "wp";
    at = &(d->attr);
    atn = &(dn->attr);
    CHECK_ATTR (mode, "LB mode");
    CHECK_ATTR (msg_size, "msg_size");
    CHECK_ATTR (maxn_msgs, "maxn_msgs");
    CHECK_ATTR (types, "LB type");
    CHECK_ATTR (tag_size, "LB tag_size");
    if (err != NULL) {
	LE_send_msg (GL_ERROR, 
	    "Inconsistent %s for data stores %d and %d", 
				    err, d->data_id, dn->data_id);
	return (-1);
    }
    dn->no_create = 1;
    return (0);
}

/******************************************************************

    Reads comms configuration info. Returns 0 on success or -1 on 
    failure.
	
******************************************************************/

int MRD_read_comms_config () {

    if (Read_comms_link_table () < 0) {
	LE_send_msg (GL_ERROR, "Failed in reading comms configuration\n");
	return (-1);
    }
    if (Process_DAT () < 0)
	return (-1);
    return (0);
}

/******************************************************************

    Returns DAT in "datp".

    Returns the size of DAT.
	
******************************************************************/

int MRD_get_DAT (Mrpg_dat_entry_t **datp) {
    *datp = Dat;
    return (N_dat);
}

/******************************************************************

    Returns PAT in "patp".

    Returns the size of PAT.
	
******************************************************************/

int MRD_get_PAT (Mrpg_dat_entry_t **patp) {
    *patp = Pat;
    return (N_pat);
}

/******************************************************************

    Returns comms link configuration in "cmsp".

    Returns the size of the comms link table.
	
******************************************************************/

int MRD_get_CMT (Mrpg_comms_link_t **cmsp) {
    *cmsp = Cms;
    return (N_cms);
}

/******************************************************************

    Returns RDA link number.
	
******************************************************************/

int MRD_get_RDA_link () {
    return (RDA_link);
}

/******************************************************************

    Reads the DAT.

    Returns the number of DAT entries on success or -1 on failure.
	
******************************************************************/

static int Read_DAT () {
    char ext_name[MRPG_NAME_SIZE], *call_name;

    LE_send_msg (LE_VL1, "Reading data table");

    Dat_tbl = MISC_open_table (sizeof (Mrpg_dat_entry_t), 64, 
			    0, &N_dat, (char **)&Dat);
    if (Dat_tbl == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }

    LE_send_msg (LE_VL2, "    Reading data table file %s", Data_table_name);
    Read_extension_table = 0;
    if (Read_datfile (Data_table_name) < 0)	/* read main data table */
	return (-1);

    /* read extended data tables */
    Read_extension_table = 1;
    call_name = Cfg_extensions;
    while (MRD_get_next_file_name (call_name, 
			Data_table_basename, ext_name, MRPG_NAME_SIZE) == 0) {
	LE_send_msg (LE_VL2, "    Reading data table file %s", ext_name);
	if (Read_datfile (ext_name) < 0)
	    return (-1);
	call_name = NULL;
    }

    if (N_dat == 0)
	LE_send_msg (LE_VL1, "No data table entry found");
    CS_cfg_name ("");

    return (N_dat);
}

static int Close_cs_file (int ret) {

    CS_control (CS_CLOSE);
    CS_cfg_name ("");
    return (ret);
}

/******************************************************************

    Reads data table file "filename".

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_datfile (char *filename) {
    char *line;
    char tmp[TMP_BUF_SIZE];
    Mrpg_dat_entry_t dat, *new;
    int ret;

    CS_cfg_name (filename);
    CS_control (CS_COMMENT | '#');

    CS_control (CS_KEY_OPTIONAL);
    ret = CS_entry ("Datastore_attr_tbl", 0, TMP_BUF_SIZE, tmp);
    CS_control (CS_KEY_REQUIRED);
    if (ret > 0 && CS_level (CS_DOWN_LEVEL) < 0)
	return (Close_cs_file (-1));

    line = CS_THIS_LINE;
    while (1) {
	int ret, tag_size, nra_size, lb_attr_found, i, did, stream;

	CS_control (CS_KEY_OPTIONAL);
	ret = CS_entry (line, 0, TMP_BUF_SIZE, tmp);
	CS_control (CS_KEY_REQUIRED);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;
	if (ret == CS_OPEN_ERR) {
	    LE_send_msg (GL_ERROR, "Failed in opening file %s", filename);
	    return (Close_cs_file (-1));
	}
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "CS_entry CS_NEXT_LINE failed (%d)", ret);
	    return (Close_cs_file (-1));
	}
	if (strcmp (tmp, "Datastore") != 0) {
	    LE_send_msg (GL_ERROR, "Unexpected key %d", tmp);
	    return (Close_cs_file (-1));
	}
	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    CS_report ("Empty Datastore section");
	    return (Close_cs_file (-1));
	}

	if (CS_entry ("data_id", 1, TMP_BUF_SIZE, tmp) <= 0)
	    return (Close_cs_file (-1));
	if (sscanf (tmp, "%d%*c%d", &stream, &did) == 2)
	    dat.data_id = ORPGDA_STREAM_DATA_ID (stream, did);
	else if (sscanf (tmp, "%d", &did) == 1)
	    dat.data_id = did;
	else {
	    CS_report ("Bad data_id specified");
	    return (Close_cs_file (-1));
	}

	for (i = 0; i < N_dat; i++) {	/* check Duplicated data ID */
	    if (Dat[i].data_id == dat.data_id) {
		if (Dat[i].is_extension) {
		    LE_send_msg (GL_ERROR, 
			"Duplicated data ID (%d) found in %s", 
					dat.data_id, filename);
		    return (Close_cs_file (-1));
		}
		else {
		    Free_data_entry (i);
		    break;
		}
	    }
	}

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry ("path", 1, TMP_BUF_SIZE, tmp) <= 0)
	    tmp[0] = '\0';
	if ((dat.path = malloc (strlen (tmp) + 1)) == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return (Close_cs_file (-1));
	}
	strcpy (dat.path, tmp);

	memset (&(dat.attr.remark), 0, LB_REMARK_LENGTH);
	Get_default_lb_attr (&(dat.attr));
	tag_size = dat.attr.tag_size & 0xff;
	nra_size = dat.attr.tag_size >> NRA_SIZE_SHIFT;
	dat.no_create = 0;
	lb_attr_found = 0;
	if (CS_entry ("Lb_attr", 0, TMP_BUF_SIZE, tmp) > 0) {
	    int i;

	    if (CS_level (CS_DOWN_LEVEL) < 0)
		break;
	    
	    CS_entry ("remark", 1, LB_REMARK_LENGTH,
				(void *)&(dat.attr.remark));
	    CS_entry ("msg_size", 1 | CS_INT, 0,
				(void *)&(dat.attr.msg_size));
	    CS_entry ("maxn_msgs", 1 | CS_INT, 0,
				(void *)&(dat.attr.maxn_msgs));
	    CS_entry ("tag_size", 1 | CS_INT, 0, (void *)&tag_size);
	    CS_entry ("nra_size", 1 | CS_INT, 0, (void *)&nra_size);
	    if (CS_entry ("mode", 1 | CS_INT, 0, (void *)&i) >= 0) {
		dat.attr.mode = (i % 10) + ((i / 10) % 10) * 8 + 
					((i / 100) % 10) * 64;
	    }
	    if (CS_entry ("types", 1, TMP_BUF_SIZE, tmp) > 0)
		Set_lb_types (&(dat.attr.types), tmp);

	    CS_level (CS_UP_LEVEL);
	    lb_attr_found = 1;
	}
	dat.attr.tag_size = tag_size | (nra_size << NRA_SIZE_SHIFT);

	dat.persistent = 0;
	dat.mrpg_init = 0;
	dat.is_extension = Read_extension_table;
	if (CS_entry ("persistent", 0, TMP_BUF_SIZE, tmp) > 0)
	    dat.persistent = 1;
	if (CS_entry ("mrpg_init", 0, TMP_BUF_SIZE, tmp) > 0)
	    dat.mrpg_init = 1;

	dat.compr_code = 0;
	if (CS_entry ("compression", 0, TMP_BUF_SIZE, tmp) > 0) {
	    if (CS_entry ("compression", 1 | CS_INT, 0, (char *)&i) > 0)
		dat.compr_code = i;
	    else {
		CS_report ("Compression code not found");
		return (Close_cs_file (-1));
	    }
	}

	/* read write permission table */
	dat.wp_size = 0;
	dat.wp = NULL;
	if (CS_entry ("write_permission", 0, TMP_BUF_SIZE, tmp) > 0) {
	    int i, cnt;
	    char *l;
	    Mrpg_wp_item *wp;

	    if (CS_level (CS_DOWN_LEVEL) < 0) {
		CS_report ("Empty write_permission section");
		return (Close_cs_file (-1));
	    }

	    cnt = 0;
	    l = CS_THIS_LINE;
	    while (1) {
		if (CS_entry (l, 0, TMP_BUF_SIZE, tmp) < 0) {
		    if (cnt == 0) {
			CS_report ("Empty write permission table");
			return (Close_cs_file (-1));
		    }
		    break;
		}

		i = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, i, TMP_BUF_SIZE, tmp) < 0) {
			if (i == 1) {
			    CS_report ("No task name found in write permission specification");
			    return (Close_cs_file (-1));
			}
			break;
		    }
		    cnt++;
		    i++;
		}
		l = CS_NEXT_LINE;
	    }

	    wp = (Mrpg_wp_item *)malloc (cnt * sizeof (Mrpg_wp_item));
	    if (wp == NULL) {
		LE_send_msg (GL_ERROR, "Malloc failed");
		return (Close_cs_file (-1));
	    }

	    CS_level (CS_UP_LEVEL);
	    CS_level (CS_DOWN_LEVEL);
	    cnt = 0;
	    l = CS_THIS_LINE;
	    while (1) {
		LB_id_t msg_id;

		if (CS_entry (l, 0, TMP_BUF_SIZE, tmp) < 0)
		    break;

		if (strcmp (tmp, "*") == 0)
		    msg_id = LB_ANY;
		else if (sscanf (tmp, "%d", &msg_id) != 1) {
		    CS_report (
			"Bad message ID for write permission specification");
		    return (Close_cs_file (-1));
		}

		i = 1;
		while (1) {
		    if (CS_entry (CS_THIS_LINE, i, TMP_BUF_SIZE, tmp) < 0)
			break;
		    strncpy (wp[cnt].name, tmp, MRPG_WP_NAME_SIZE);
		    wp[cnt].name[MRPG_WP_NAME_SIZE - 1] = '\0';
		    wp[cnt].msg_id = msg_id;
		    cnt++;
		    i++;
		}
		l = CS_NEXT_LINE;
	    }

	    CS_level (CS_UP_LEVEL);
	    dat.wp_size = cnt;
	    dat.wp = wp;
	}

	CS_control (CS_KEY_REQUIRED);
	CS_level (CS_UP_LEVEL);
	line = CS_NEXT_LINE;
	if (!lb_attr_found)	/* we only take LB data stores */
	    dat.data_id = -dat.data_id;

	new = MISC_table_new_entry (Dat_tbl, NULL);
	if (new == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return (Close_cs_file (-1));
	}
	memcpy (new, &dat, sizeof (Mrpg_dat_entry_t));
    }

    return (Close_cs_file (0));
}

/******************************************************************

    Reads the PAT.

    Returns the number of PAT entries on success or -1 on failure.
	
******************************************************************/

static int Read_PAT () {
    char ext_name[MRPG_NAME_SIZE], *call_name;

    LE_send_msg (LE_VL1, "Reading product table");

    Pat_tbl = MISC_open_table (sizeof (Mrpg_dat_entry_t), 64, 
			    0, &N_pat, (char **)&Pat);
    if (Pat_tbl == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }

    LE_send_msg (LE_VL2, 
		"    Reading product table file %s", Product_table_name);
    Read_extension_table = 0;
    if (Read_patfile (Product_table_name) < 0)	/* read main product table */
	return (-1);

    /* read extended product tables */
    Read_extension_table = 1;
    call_name = Cfg_extensions;
    while (MRD_get_next_file_name (call_name, 
		Product_table_basename, ext_name, MRPG_NAME_SIZE) == 0) {
	LE_send_msg (LE_VL2, "    Reading product table file %s", ext_name);
	if (Read_patfile (ext_name) < 0)
	    return (-1);
	call_name = NULL;
    }

    if (N_pat == 0)
	LE_send_msg (LE_VL1, "No product table entry found");
    CS_cfg_name ("");

    return (N_pat);
}

/******************************************************************

    Reads product table file "filename".

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_patfile (char *filename) {
    char *line;
    char tmp[TMP_BUF_SIZE];
    Mrpg_dat_entry_t pat, *new;
    int ret;

    CS_cfg_name (filename);
    CS_control (CS_COMMENT | '#');

    CS_control (CS_KEY_OPTIONAL);
    ret = CS_entry ("Prod_attr_table", 0, TMP_BUF_SIZE, tmp);
    CS_control (CS_KEY_REQUIRED);
    if (ret > 0 && CS_level (CS_DOWN_LEVEL) < 0)
	return (Close_cs_file (-1));

    line = CS_THIS_LINE;
    while (1) {
	int ret, i;

	CS_control (CS_KEY_OPTIONAL);
	ret = CS_entry (line, 0, TMP_BUF_SIZE, tmp);
	CS_control (CS_KEY_REQUIRED);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND)
	    break;
	if (ret == CS_OPEN_ERR) {
	    LE_send_msg (GL_ERROR, "Failed in opening file %s", filename);
	    return (Close_cs_file (-1));
	}
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "CS_entry CS_NEXT_LINE failed (%d)", ret);
	    return (Close_cs_file (-1));
	}
	if (strcmp (tmp, "Product") != 0) {
	    LE_send_msg (GL_ERROR, "Unexpected key %d", tmp);
	    return (Close_cs_file (-1));
	}
	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    CS_report ("Empty Product section");
	    return (Close_cs_file (-1));
	}

	if (CS_entry ("prod_id", 1 | CS_INT, 0,
				(void *)&(pat.data_id)) <= 0)
	    return (Close_cs_file (-1));

	for (i = 0; i < N_pat; i++) {	/* check Duplicated product ID */
	    if (Pat[i].data_id == pat.data_id) {
		if (Pat[i].is_extension) {
		    LE_send_msg (GL_ERROR, 
			"Duplicated product ID (%d) found in %s", 
					pat.data_id, filename);
		    return (Close_cs_file (-1));
		}
		else {
		    Free_product_entry (i);
		    break;
		}
	    }
	}

	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry ("path", 1, TMP_BUF_SIZE, tmp) <= 0)
	    tmp[0] = '\0';
	if ((pat.path = malloc (strlen (tmp) + 1)) == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return (Close_cs_file (-1));
	}
	strcpy (pat.path, tmp);
	pat.no_create = 0;

	Get_default_lb_attr (&(pat.attr));
	CS_entry ("max_size", 1 | CS_INT, 0,
				(void *)&(pat.attr.msg_size));
	CS_entry ("lb_n_msgs", 1 | CS_INT, 0,
				(void *)&(pat.attr.maxn_msgs));
	if (CS_entry ("lb_types", 1, TMP_BUF_SIZE, tmp) > 0)
		Set_lb_types (&(pat.attr.types), tmp);

	pat.persistent = 0;
	pat.mrpg_init = 0;
	pat.is_extension = Read_extension_table;
	pat.compr_code = 0;
	pat.wp_size = 0;
	pat.wp = NULL;

	CS_control (CS_KEY_REQUIRED);
	CS_level (CS_UP_LEVEL);
	line = CS_NEXT_LINE;

	new = MISC_table_new_entry (Pat_tbl, NULL);
	if (new == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return (Close_cs_file (-1));
	}
	memcpy (new, &pat, sizeof (Mrpg_dat_entry_t));
    }

    return (Close_cs_file (0));
}

/******************************************************************

    Reads comms link configuration table.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Read_comms_link_table () {
    int n_links;

    LE_send_msg (LE_VL1, "Reading comms configuration");
    if (MAIN_is_operational ())
	return (Read_de_comms_info ());

    CS_cfg_name (Comms_config_name);
    CS_control (CS_COMMENT | '#');

    if (CS_entry ("number_links", 1 | CS_INT, 0, (char *)&n_links) <= 0)
	return (Close_cs_file (-1));

    Cms = (Mrpg_comms_link_t *)malloc (n_links * sizeof (Mrpg_comms_link_t));
    if (Cms == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (Close_cs_file (-1));
    }

    N_cms = 0;
    while (N_cms < n_links) {	/* enable according to comms config */
	int un, cn;

	if (CS_entry ((char *)N_cms, CS_INT_KEY | 1 | CS_INT, 0, 
						(char *)&un) <= 0 ||
	    CS_entry ((char *)N_cms, CS_INT_KEY | 2 | CS_INT, 0, 
						(char *)&cn) <= 0 ||
	    CS_entry ((char *)N_cms, CS_INT_KEY | 7, MRPG_NAME_SIZE, 
						Cms[N_cms].cm_mgr) <= 0)
	    return (Close_cs_file (-1));
	Cms[N_cms].link = N_cms;
	Cms[N_cms].user = un;
	Cms[N_cms].cm = cn;
	N_cms++;
    }

    if (CS_entry ("RDA_link", 1 | CS_INT, 0, (char *)&RDA_link) <= 0) {
	LE_send_msg (GL_ERROR, "RDA link is not defined in %s", Comms_config_name);
	return (Close_cs_file (-1));
    }

    return (Close_cs_file (0));
}

/******************************************************************

    Reads comms link info from the DEA DB. Returns 0 on success or 
    -1 on failure.
	
******************************************************************/

static int Read_de_comms_info () {
    int ret, i, n_misc;
    double d, *values;
    char *p;

    if ((ret = DEAU_get_values ("comms.n_link", &d, 1)) <= 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_get_values comms.n_link failed (%d)\n", ret);
	return (-1);
    }
    N_cms = (int)d;

    Cms = (Mrpg_comms_link_t *)malloc (N_cms * sizeof (Mrpg_comms_link_t));
    values = (double *)malloc (N_cms * sizeof (double));
    if (Cms == NULL || values == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }
    memset (Cms, 0, N_cms * sizeof (Mrpg_comms_link_t));

    for (i = 0; i < N_cms; i++)
	Cms[i].link = i;
    if (Get_de_value ("comms.user_index", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_cms; i++)
	Cms[i].user = (int)values[i];
    if (Get_de_value ("comms.cm_index", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_cms; i++)
	Cms[i].cm = (int)values[i];
    if (Get_de_value ("comms.cm_name", 1, &p) < 0)
	return (-1);
    for (i = 0; i < N_cms; i++) {
	strncpy (Cms[i].cm_mgr, p, MRPG_NAME_SIZE - 1);
	p += strlen (p) + 1;
    }
    free (values);

    n_misc = DEAU_get_string_values ("comms.misc_spec", &p);
    RDA_link = -1;
    for (i = 0; i < n_misc; i++) {
	char *tp;
	if ((tp = strstr (p, "RDA_link")) != NULL &&
	    sscanf (tp + strlen ("RDA_link") + 1, "%d", &RDA_link) < 1)
	    RDA_link = -1;
	p += strlen (p) + 1;
    }
    if (RDA_link < 0) {
	LE_send_msg (GL_ERROR, "RDA link is not defined in comms DEA");
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Reads N_cms values of DE "de_id" and puts them in "values". Returns 0
    on success or -1 on failure.

**************************************************************************/

static int Get_de_value (char *de_id, int is_str_type, void *values) {
    int ret;

    if (is_str_type)
	ret = DEAU_get_string_values (de_id, (char **)values);
    else
	ret = DEAU_get_values (de_id, (double *)values, N_cms);
    if (ret != N_cms) {
	LE_send_msg (GL_ERROR, 
		"DEAU_get_values %s failed (%d != %d)\n", de_id, ret, N_cms);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Removes DAT entries defined in pat and those not used in terms
    of comms config.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Process_DAT () {
    int i, link;

    for (i = 0; i < N_dat; i++) {	/* first disable all */
	int did;

	did = Dat[i].data_id;
	if ((did >= ORPGDAT_CM_REQUEST && did < ORPGDAT_CM_REQUEST + 15) ||
	    (did >= ORPGDAT_CM_RESPONSE && did < ORPGDAT_CM_RESPONSE + 80) ||
	    (did >= ORPGDAT_OT_RESPONSE && did < ORPGDAT_OT_RESPONSE + 80))
	    Dat[i].no_create = 1;
    }
    for (link = 0; link < N_cms; link++) {
					/* enable according to comms config */
	for (i = 0; i < N_dat; i++) {
	    int did;

	    did = Dat[i].data_id;
	    if (did == ORPGDAT_CM_RESPONSE + Cms[link].link)
		Dat[i].no_create = 0;
	    if (did == ORPGDAT_OT_RESPONSE + Cms[link].user)
		Dat[i].no_create = 0;
	    if (did == ORPGDAT_CM_REQUEST + Cms[link].cm)
		Dat[i].no_create = 0;
	}
    }
    return (0);
}

/**************************************************************************

    Description: Sets LB attributes "attr" to default values.

    Output:	attr - the LB attribute structure.

**************************************************************************/

static void Get_default_lb_attr (LB_attr *attr) {
    attr->remark[0] = '\0';
    attr->mode = 0664;
    attr->msg_size = 0;
    attr->maxn_msgs = 0;
    attr->types = 0;
    attr->tag_size = LB_DEFAULT_NRS << NRA_SIZE_SHIFT;
}

/**************************************************************************

    Description: Sets LB attribute bits in terms of string "tmp".

    Input:	tmp - string of LB types.

    Output:	types - the LB type flags.

**************************************************************************/

static void Set_lb_types (int *types, char *tmp) {
    char *cpt;
    int flags = 0;

    cpt = strtok (tmp, " \t\n");
    while (cpt != NULL) {
   
	if (strcmp ("LB_MEMORY", cpt) == 0)
	    flags |= LB_MEMORY;
	if (strcmp ("LB_SINGLE_WRITER", cpt) == 0)
	    flags |= LB_SINGLE_WRITER;
	if (strcmp ("LB_NOEXPIRE", cpt) == 0)
	    flags |= LB_NOEXPIRE;
	if (strcmp ("LB_UNPROTECTED", cpt) == 0)
	    flags |= LB_UNPROTECTED;
	if (strcmp ("LB_REPLACE", cpt) == 0)
	    flags |= LB_REPLACE;
	if (strcmp ("LB_DB", cpt) == 0)
	    flags |= LB_DB;
	if (strcmp ("LB_POOL", cpt) == 0)
	    flags |= LB_POOL;
	if (strcmp ("LB_MUST_READ", cpt) == 0)
	    flags |= LB_MUST_READ;
	if (strcmp ("LB_FILE", cpt) == 0)
	    flags |= LB_FILE;
	if (strcmp ("LB_NORMAL", cpt) == 0)
	    flags |= LB_NORMAL;
	if (strcmp ("LB_DIRECT", cpt) == 0)
	    flags |= LB_DIRECT;
	if (strcmp ("LB_UN_TAG", cpt) == 0)
	    flags |= LB_UN_TAG;
	if (strcmp ("LB_MSG_POOL", cpt) == 0)
	    flags |= LB_MSG_POOL;
	cpt = strtok (NULL, " \t\n");
    }
    *types = *types | flags;
}

/*******************************************************************

    Returns the name of the first (dir_name != NULL) or the next 
    (dir_name = NULL) file in directory "dir_name" whose name matches 
    "basename".*. The caller provides the buffer "buf" of size 
    "buf_size" for returning the file name. It returns 0 on success 
    or -1 on failure.

*******************************************************************/

int MRD_get_next_file_name (char *dir_name, char *basename, 
					char *buf, int buf_size) {
    static DIR *dir = NULL;	/* the current open dir */
    static char saved_dirname[MRPG_NAME_SIZE] = "";
    struct dirent *dp;

    if (dir_name != NULL) {
	int len;

	len = strlen (dir_name);
	if (len + 1 >= MRPG_NAME_SIZE) {
	    LE_send_msg (GL_ERROR, 
		"dir name (%s) does not fit in tmp buffer\n", dir_name);
	    return (-1);
	}
	strcpy (saved_dirname, dir_name);
	if (len == 0 || saved_dirname[len - 1] != '/')
	    strcat (saved_dirname, "/");
	if (dir != NULL)
	    closedir (dir);
	dir = opendir (dir_name);
	if (dir == NULL)
	    return (-1);
    }
    if (dir == NULL)
	return (-1);

    while ((dp = readdir (dir)) != NULL) {
	struct stat st;
	char fullpath[2 * MRPG_NAME_SIZE];

	if (strncmp (basename, dp->d_name, strlen (basename)) != 0)
	    continue;

	if (strlen (dp->d_name) >= MRPG_NAME_SIZE) {
	    LE_send_msg (GL_ERROR, 
		"file name (%s) does not fit in tmp buffer\n", dp->d_name);
	    continue;
	}
	strcpy (fullpath, saved_dirname);
	strcat (fullpath, dp->d_name);
	if (stat (fullpath, &st) < 0) {
	    LE_send_msg (GL_ERROR, 
		"stat (%s) failed, errno %d\n", fullpath, errno);
	    continue;
	}
	if (!(st.st_mode & S_IFREG))	/* not a regular file */
	    continue;

	if (strlen (fullpath) >= buf_size) {
	    LE_send_msg (GL_ERROR,
		"caller's buffer is too small (for %s)\n", fullpath);
	    continue;
	}
	strcpy (buf, fullpath);
	return (0);
    }
    return (-1);
}

/******************************************************************

    Searches in directory "dir" (must without trailing '/') for file
    named "dir/basename" or "dir/basenames". If such a file is found,
    the full path of the file name is returned in "name". If it is
    not found, one of these names is returned. The buffer size
    of "name" are assumed to be big enough. "size" is the 
    buffer size of "tmp" which is assumed to be no less than the size
    of "name".

    Returns 0 on success or -1 if both files are found.
	
******************************************************************/

int MRD_get_table_file_name (char *name, char *dir, char *basename, 
						char *tmp, int size) {
    int len, cnt;
    char *call_name;

    strcpy (tmp, dir);
    strcat (tmp, "/");
    strcat (tmp, basename);
    strcpy (name, tmp);

    call_name = dir;
    len = strlen (name);
    cnt = 0;
    while (MRD_get_next_file_name (call_name, basename, 
						tmp, size) == 0) {
	if (strlen (tmp) == len)
	    cnt++;
	else if (strlen (tmp) == len + 1 && tmp[len] == 's') {
	    if (cnt == 0)
		strcat (name, "s");
	    cnt++;
	}
	call_name = NULL;
    }
    if (cnt >= 2) {
	LE_send_msg (GL_ERROR, "dupllicated %s found in %s", basename, dir);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Publishes data info. Returns 0 on success or -1 on failure.
	
******************************************************************/

int MRD_publish_data_info () {
    Mrpg_dat_entry_t *d;
    int size, i, ret;
    char *buf, *cp;
    Mrpg_data_t *di;

    size = 0;
    for (i = 0; i < N_dat; i++) {	/* estimate the buffer size needed */
	d = Dat + i;
	if (d->compr_code == 0 && d->wp_size == 0)
	    continue;
	size += sizeof (Mrpg_data_t);
	if (d->wp_size > 1)
	    size += (d->wp_size - 1) *sizeof (Mrpg_wp_item);
    }

    buf = malloc (size);
    if (buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }

    size = 0;
    for (i = 0; i < N_dat; i++) {	/* create the message */
	int start;

	d = Dat + i;
	if (d->compr_code == 0 && d->wp_size == 0)
	    continue;
	start = size;
	cp = buf + size;
	di = (Mrpg_data_t *)cp;
	di->data_id = d->data_id;
	if (di->data_id < 0)
	    di->data_id = -di->data_id;
	di->compr_code = d->compr_code;
	di->wp_size = d->wp_size;
	size += sizeof (Mrpg_data_t);
	memcpy (di->wp, d->wp, d->wp_size * sizeof (Mrpg_wp_item));
	if (d->wp_size > 1)
	    size += (d->wp_size - 1) * sizeof (Mrpg_wp_item);
	di->size = size - start;
    }

    ret = ORPGDA_write (ORPGDAT_TASK_STATUS, buf, size, MRPG_RPG_DATA_MSGID);
    if (ret != size) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_write MRPG_RPG_DATA_MSGID failed (%d)\n", ret);
	free (buf);
	return (-1);
    }
    free (buf);
    return (0);
}

/******************************************************************

    Frees data table entry of index "ind".
	
******************************************************************/

static void Free_data_entry (int ind) {
    Mrpg_dat_entry_t *da;

    da = Dat + ind;
    free (da->path);
    if (da->wp != NULL)
	free (da->wp);
    MISC_table_free_entry (Dat_tbl, ind);
}

/******************************************************************

    Frees product table entry of index "ind".
	
******************************************************************/

static void Free_product_entry (int ind) {
    Mrpg_dat_entry_t *pa;

    pa = Pat + ind;
    free (pa->path);
    if (pa->wp != NULL)
	free (pa->wp);
    MISC_table_free_entry (Pat_tbl, ind);
}





