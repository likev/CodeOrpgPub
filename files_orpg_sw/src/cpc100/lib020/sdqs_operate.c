
/******************************************************************

	This is the sdmqm module for oparational execution.
	
******************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/12 14:12:52 $
 * $Id: sdqs_operate.c,v 1.21 2014/03/12 14:12:52 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */


#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <infr.h> 

#include "sdqs_def.h"

#define SAVED_RECORDS_MSGID 1	/* ID of the message saving records for fast
				   rpgdbm restart (must be the first LB ID) */

#define SAV_REC_MAGIC 432000983	/* A magic number for identifying the saved 
				   record msg */

static int N_tables = 0;
static table_t *Tables = NULL;
static char *Msg_buf = NULL, *Rec_buf = NULL;
static int Msg_buf_size = 0;
static int No_compilation;			/* run without compilation */
static int Css_port;				/* port used for server socket
						   binding */


static void Load_dynamic_routines ();
static void Init_sdb_table (table_t *table);
static void Lb_ntf_cb (int fd, LB_id_t msgid, int msg_len, void *arg);
static void Set_msg_rec_sizes (table_t *table);
static void Open_sdb_LB (table_t *table, runtime_t *rt);
static void Build_data_base (table_t *table);
static int Read_and_insert (table_t *table, LB_id_t msgid, int msg_len);
static void Delete_record (table_t *table, LB_id_t msgid);
static char *(*Get_macro)();
static void Alloc_buffers ();
static void Complete_tables (table_t *table);
static void Init_tables ();
static char *Get_saved_record (runtime_t *rt, LB_id_t msgid, char *db_name);
static void Save_records (table_t *table);


/**************************************************************************

    Initializes this module.

**************************************************************************/

void SDQSO_init (int no_compilation) {
    int i;

    No_compilation = no_compilation;

    N_tables = SDQSC_get_sdb_tables (&Tables);
    Load_dynamic_routines ();

    LB_NTF_control (LB_NTF_BLOCK);

    for (i = 0; i < N_tables; i++)
	Init_sdb_table (Tables + i);

    Alloc_buffers ();
    for (i = 0; i < N_tables; i++)
	Build_data_base (Tables + i);

    if (No_compilation) {		/* try to bind the server port */
	int port, max_port, max_res_port, ret;

	port = SDQSO_get_port ();
	max_port = port + 100;		/* we try at most consecutive ports */
	if (max_port > 0xffff)
	    max_port = 0xffff;
	max_res_port = port + 2;	/* reserve 2 ports for owr, future */
	ret = 0;
	while (port < max_port) {
	    char addr[64];
	    sprintf (addr, "%d", port);
	    if ((ret = CSS_sv_init (addr, MAXN_CONNS)) != CSS_BIND_FAILED)
		break;
	    port++;
	    while (port <= max_res_port)
		port++;
	}
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"CSS_sv_init (port %d) failed (ret %d)", port, ret);
	    exit (1);
	}
	Css_port = port;
    }
    else
	Css_port = SDQSO_get_port ();

    for (i = 0; i < N_tables; i++) {	/* set CSS port and IP in the LB */
	char *cpt;
	unsigned int hid;
	if (Tables[i].rt.lb_fd < 0)
	    continue;
	if ((cpt = SDQM_get_full_lb_name (Tables[i].src_lb, &hid)) != NULL)
	    free (cpt);
	RSS_LB_sdqs_address (Tables[i].rt.lb_fd, 1, &Css_port, &hid);
	if (i == 0)
	    LE_send_msg (GL_INFO, "port %d host id %d used by this server\n", 
							Css_port, hid);
    }

    LB_NTF_control (LB_NTF_UNBLOCK);
}

/**************************************************************************

    Returns the port number for the server.

**************************************************************************/

int SDQSO_get_port () {
    char *t;
    int p;

    t = SDQSM_get_port ();
    if ((t = Get_macro (t)) == NULL)
	t = SDQSM_get_port ();
    if (sscanf (t, "%d", &p) != 1) {
	LE_send_msg (GL_ERROR, "bad server port number: %s\n", t);
	exit (1);
    }
    return (p);
}

/******************************************************************

    Returns the CSS port number.

******************************************************************/

int SDQSO_get_css_port () {
    return (Css_port);
}

/**************************************************************************

    Calls the house keep function for each sdb table.

**************************************************************************/

int SDQSO_house_keep (int in_termination_phase) {
    int i;

    for (i = 0; i < N_tables; i++) {
	runtime_t *rt = &(Tables[i].rt);
	if (rt->lb_fd < 0)
	    continue;
	if (rt->House_keep != NULL) {
	    if (in_termination_phase)
		rt->n_recs = rt->House_keep (SDQM_HK_TERM, rt->n_recs, 
						rt->lb_fd, rt->db_id);
	    else
		rt->n_recs = rt->House_keep (SDQM_HK_ROUTINE, rt->n_recs, 
						rt->lb_fd, rt->db_id);
	}
	if (in_termination_phase)
	    Save_records (Tables + i);
    }
    return (0);
}

/******************************************************************

    Allocates buffer for Read_and_insert so we don't have to alloc
    latter in a callback.

******************************************************************/

static void Alloc_buffers () {
    int rec_buf_size, i;

    Msg_buf_size = rec_buf_size = 0;
    for (i = 0; i < N_tables; i++) {
	runtime_t *rt;

	rt = &(Tables[i].rt);
	if (rt->lb_fd < 0)
	    continue;
	if (Msg_buf_size < rt->src_msg_size)
	    Msg_buf_size = rt->src_msg_size;
	if (rec_buf_size < rt->rec_size)
	    rec_buf_size = rt->rec_size;
    }
    if (Msg_buf_size > 0)
	Msg_buf = SDQSM_malloc (Msg_buf_size);
    Rec_buf = SDQSM_malloc (rec_buf_size);
}

/******************************************************************

    Goes through the LB and builds sdb table "table".

******************************************************************/

static void Build_data_base (table_t *table) {
    runtime_t *rt;
    int ret, n, i;
    LB_info *list;

    rt = &(table->rt);
    if (rt->lb_fd < 0)
	return;
    if (rt->House_keep != NULL) {
	ret = rt->House_keep (SDQM_HK_BUILD, rt->n_recs, rt->lb_fd, rt->db_id);
	if (ret >= 0) {
	    rt->n_recs = ret;
	    return;
	}
    }

    list = malloc (rt->maxn_recs * sizeof (LB_info));
    if (list == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed in Build_data_base\n");
	exit (1);
    }
	
    n = LB_list (rt->lb_fd, list, rt->maxn_recs);
    if (n < 0) {
	LE_send_msg (GL_ERROR, "LB_list failed (%d)\n", n);
	exit (1);
    }

    for (i = 0; i < n; i++) {
	char *rec;

	if (list[i].size < 0)
	    continue;

	rec = Get_saved_record (rt, list[i].id, table->name);
	if (rt->has_saved_recs && list[i].id == SAVED_RECORDS_MSGID)
	    continue;
	if (rec == NULL) {
	    if (Read_and_insert (table, list[i].id, list[i].size) < 0)
		break;
	}
	else {
	    ret = SDQM_insert (rt->db_id, rec);
	    if (ret < 0) {
	        LE_send_msg (GL_ERROR,  
			"SDQM_insert failed (ret %d)\n", ret);
	        exit (1);
	    }
	    rt->n_recs++;
	} 
    }
    Get_saved_record (rt, 0, table->name);	/* free the buffer */
    free (list);

    LE_send_msg (LE_VL1, "table %s initialized: %d items\n", 
					table->name, rt->n_recs);
    return;
}

/******************************************************************

    Reads a source message of "msgid" and inserts it in the data base.
    Returns LB_read return value or -1 in other error conditions.

******************************************************************/

static int Read_and_insert (table_t *table, LB_id_t msgid, int msg_len) {
    runtime_t *rt;
    int size, read_size, ret, iambig;
    char *src_struct;

    rt = &(table->rt);
    read_size = msg_len;
    if (rt->src_msg_size > 0)
	read_size = rt->src_msg_size;
    if (read_size > Msg_buf_size) {
	char *p;
	int new_size = read_size + 512;
	p = malloc (new_size);
	if (p == NULL) {
            LE_send_msg (GL_ERROR, "malloc (%d) failed\n", new_size);
	    return (-1);
	}
	if (Msg_buf != NULL)
	    free (Msg_buf);
	Msg_buf = p;
	Msg_buf_size = new_size;
    }
    size = RSS_LB_read (rt->lb_fd, Msg_buf, read_size, msgid);

    if (size < 0 && size != LB_TO_COME)
        LE_send_msg (GL_ERROR, 
		"LB_read failed (table %s, msgid %u, ret %d)\n", 
					table->name, msgid, size);
    if (size <= 0)
        return (size);

    ret = 0;
    iambig = MISC_i_am_bigendian ();
    if (table->data_endian == SDQS_SERIAL) {
	ret = SMIA_deserialize (table->src_t, Msg_buf, &src_struct, size);
	if (ret <= 0) {
            LE_send_msg (GL_ERROR, "SMIA_deserialize failed (%d)\n", ret);
	    return (ret);
	}
	else if ((int)table->lb_is_bigendian != iambig)
	    SMIA_bswap_input (table->src_t, src_struct, size);
    }
    else if ((table->data_endian == SDQS_BIG_ENDIAN && !iambig) ||
	     (table->data_endian == SDQS_LITTLE_ENDIAN && iambig)) {
	ret = SMIA_bswap_input (table->src_t, Msg_buf, size);
	src_struct = Msg_buf;
    }
    else
	src_struct = Msg_buf;

    /* the msgid is the first field in Rec_buf */
    ret = rt->Get_record (src_struct, rt->msgid, Rec_buf + sizeof (LB_id_t));
    if (table->data_endian == SDQS_SERIAL && src_struct != Msg_buf)
	SMIA_free_struct (table->src_t, src_struct);
    if (ret < 0)
	return (-1);
    *((int *)Rec_buf) = rt->msgid;

    ret = SDQM_insert (rt->db_id, Rec_buf);
    if (ret < 0) {
        LE_send_msg (GL_ERROR, "SDQM_insert (table %s) failed (ret %d)\n", 
						table->name, ret);
        return (-1);
    }
    rt->n_recs++;
    return (ret);
}

/**************************************************************************

    Initializes a sdb table.

**************************************************************************/

static void Init_sdb_table (table_t *table) {
    runtime_t *rt;
    SDQM_query_field_t *qfs;
    SDQM_query_index_t *its;
    int cnt, ret, *ip, i, extra_recs, text_len;
    char *cpt, *text, *data_endian;

    rt = &(table->rt);
    Set_msg_rec_sizes (table);
    Open_sdb_LB (table, rt);
    if (rt->lb_fd < 0)
	return;

    if (rt->House_keep != NULL)	{/* first call. after LB open - msg # needed */
	void *func_p[6];
	func_p[0] = (void *)SDQS_select_i;
	func_p[1] = (void *)SDQM_insert;
	func_p[2] = (void *)SDQM_delete;
	rt->House_keep (SDQM_HK_INIT, rt->maxn_recs, rt->lb_fd, (void *)func_p);
    }

    qfs = (SDQM_query_field_t *)SDQSM_malloc (
			table->n_qfs * sizeof (SDQM_query_field_t));
    qfs[0].type = SDQM_QFT_INT;		/* add msg_id field - The first QF */
    qfs[0].foff = 0;
    qfs[0].compare = NULL;
    qfs[0].set_fields = NULL;
    rt->msgid_field_ind = 0;

    cpt = table->qfs + 1;
    for (i = 1; i < table->n_qfs; i++) {
	int type, offset;

	offset = rt->Get_misc (cpt, &type);
	if (offset == -2) {
	    LE_send_msg (GL_ERROR, 
		"macro incorrect for field %s of table %s\n", 
						cpt, table->name);
	    exit (1);
	}
	if (offset < 0) {
	    LE_send_msg (GL_ERROR, 
		"field %s of table %s not found\n", cpt, table->name);
	    exit (1);
	}
	qfs[i].type = type;
	qfs[i].foff = offset + sizeof (LB_id_t);	/* msg_id in front */
	qfs[i].compare = NULL;
	qfs[i].set_fields = NULL;
	cpt += strlen (cpt) + 1;
    }

    cnt = 1;		/* We add an IT (The last IT) for msg_id */
    for (i = 0; i < table->n_its; i++)
	cnt += table->its[i].n_fields;
    its = (SDQM_query_index_t *)SDQSM_malloc (
	(table->n_its + 1) * sizeof (SDQM_query_index_t) + cnt * sizeof (int));
    ip = (int *)((char *)its + 
		(table->n_its + 1) * sizeof (SDQM_query_index_t));
    for (i = 0; i < table->n_its; i++) {
	int k, m;
	char *qf_name;

	its[i].n_itfs = table->its[i].n_fields;
	its[i].itfs = ip;
	cpt = table->its[i].fields;
	for (k = 0; k < its[i].n_itfs; k++) {
	    qf_name = table->qfs;
	    for (m = 0; m < table->n_qfs; m++) {
		if (strcmp (qf_name, cpt) == 0)
		    break;
		qf_name += strlen (qf_name) + 1;
	    }
	    if (m >= table->n_qfs) {
		LE_send_msg (GL_ERROR, 
			"index field %s of table %s is not a query field\n", 
						cpt, table->name);
		exit (1);
	    }
	    *ip = m;
	    ip++;
	    cpt += strlen (cpt) + 1;
	}
    }
    its[i].n_itfs = 1;			/* The last IT is the msg_id */
    its[i].itfs = ip;
    *ip = 0;

    extra_recs = rt->maxn_recs / 20;	/* extra slots is needed in case */
    if (extra_recs < 10)		/* of near full LB and high rate */
	extra_recs = 10;		/* messages deletion and insertion */ 
    ret = SDQM_define_sdb (table->name, rt->maxn_recs + extra_recs, 
		rt->rec_size, table->n_qfs, qfs, table->n_its + 1, its, 
					rt->Byte_swap);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "SDQM_define_sdb (table %s) failed, ret %d\n", 
						table->name, ret);
	exit (1);
    }

    if (table->data_endian == SDQS_BIG_ENDIAN)
	data_endian = "big endian";
    else if (table->data_endian == SDQS_LITTLE_ENDIAN)
	data_endian = "little endian";
    else
	data_endian = "serial";
    SDQSC_convert_chars (table->n_qfs - 1, table->qfs, '\0', ':');
    SDQSC_convert_chars (table->n_qfs - 1, table->types, '\0', ':');
    text_len = strlen (table->src_t) + 1 + 
		strlen (table->qfs) + 1 + strlen (table->types) + 1 + 
		strlen (SDQSM_get_shared_lib_basename ()) + 1 +
		strlen (data_endian) + 1;
    text = SDQSM_malloc (text_len);
    cpt = text;
    strcpy (cpt, table->src_t);
    cpt += strlen (cpt) + 1;
    strcpy (cpt, table->qfs);
    cpt += strlen (cpt) + 1;
    strcpy (cpt, table->types);
    cpt += strlen (cpt) + 1;
    strcpy (cpt, SDQSM_get_shared_lib_basename ());
    cpt += strlen (cpt) + 1;
    strcpy (cpt, data_endian);

    cpt = SDQM_get_full_lb_name (table->src_lb, NULL);
    if (cpt == NULL) {
	LE_send_msg (GL_ERROR, "bad host name (%s)\n", table->src_lb);
	exit (1);
    }
    ret = SDQM_set_sdb_meta (table->name, cpt, text_len, text);
    if (cpt != table->src_lb)
	free (cpt);
    free (text);
    SDQSC_convert_chars (table->n_qfs - 1, table->qfs, ':', '\0');
    SDQSC_convert_chars (table->n_qfs - 1, table->types, ':', '\0');
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
	"SDQM_set_sdb_meta (table %s) failed, ret %d\n", table->name, ret);
	exit (1);
    }

    rt->db_id = SDQM_get_dbid (table->name);
    if (rt->db_id == NULL) {
	LE_send_msg (GL_ERROR,  "SDQM_get_dbid (%s) failed\n", table->name);
	exit (1);
    }
    rt->n_recs = 0;

    free (its);
    free (qfs);
}

/**************************************************************************

    Sets msg size and record size fields in rt. It also retrieves
    the number of query fields and query field names and types.

**************************************************************************/

static void Set_msg_rec_sizes (table_t *table) {
    runtime_t *rt;
    char *buf, *p;

    rt = &(table->rt);
    if (table->n_qfs > 0 && !No_compilation) {
	free (table->qfs);
	free (table->types);
    }
    table->n_qfs = rt->Get_misc ("sdb_gf_names", &p);
    table->n_qfs++;		/* add msg_is field as the first QF */
    table->qfs = SDQSM_malloc (strlen (p) + 2);
    table->qfs[0] = ':';	/* field name is "" for msg_id */
    strcpy (table->qfs + 1, p);
    SDQSC_convert_chars (table->n_qfs - 1, table->qfs, ':', '\0');

    rt->Get_misc ("sdb_gf_types", &p);
    table->types = SDQSM_malloc (strlen (p) + 2);
    table->types[0] = ':';	/* field type is "" for msg_id */
    strcpy (table->types + 1, p);
    SDQSC_convert_chars (table->n_qfs - 1, table->types, ':', '\0');

    if (table->n_qfs <= 1) {
	LE_send_msg (GL_ERROR, "No query field found\n");
	exit (1);
    }

    buf = SDQSM_malloc (strlen (table->sdb_t) + 32);
    sprintf (buf, "sizeof (%s)", table->sdb_t);
    rt->rec_size = rt->Get_misc (buf, NULL);
    if (rt->rec_size < 0) {
	LE_send_msg (GL_ERROR, "Record size not found\n");
	exit (1);
    }
    rt->rec_size += sizeof (LB_id_t);	/* reserve space for msg_id */
    free (buf);
    if (rt->src_msg_size < 0) {
	buf = SDQSM_malloc (strlen (table->src_t) + 32);
	sprintf (buf, "sizeof (%s)", table->src_t);
	rt->src_msg_size = rt->Get_misc (buf, NULL);
	if (rt->src_msg_size < 0) {
	    LE_send_msg (GL_ERROR, "Source size not found\n");
	    exit (1);
	}
	free (buf);
    }
    LE_send_msg (LE_VL1, "Message read size: %d; record size: %d - %s\n", 
				rt->src_msg_size, rt->rec_size, table->name);
}

/******************************************************************

    Opens the LB for table "name", registers the UN and finds out 
    the size of the LB.

******************************************************************/

static void Open_sdb_LB (table_t *table, runtime_t *rt) {
    char *lb_name, tmp[128], *p;
    LB_status status;
    LB_attr attr;
    int data_id, lb_fd, ret, i, is_vsize;
    extern int Allow_missing_LBs;

    if ((lb_name = Get_macro (table->src_lb)) == NULL)
	lb_name = table->src_lb;
    if (sscanf (lb_name, "%d", &data_id) == 1 &&
	CS_entry_int_key (data_id, 1, 128, tmp) > 0)
	lb_name = tmp;
    if (lb_name != table->src_lb) {
	if (!No_compilation)
	    free (table->src_lb);
	table->src_lb = SDQSM_malloc (strlen (lb_name) + 1);
	strcpy (table->src_lb, lb_name);
    }

    if (Allow_missing_LBs) {
	int len = MISC_char_cnt (lb_name, "\0:");
	if (len > 0 && lb_name[len] == ':') {
	    LE_send_msg (0, "LB %s is remote - not managed\n", lb_name);
	    rt->lb_fd = -1;
	    return;
	}
    }
    lb_fd = RSS_LB_open (lb_name, LB_WRITE, NULL);
			/* write permission is needed for deletion */
    if (lb_fd < 0) {
	if (Allow_missing_LBs) {
	    LE_send_msg (0, "LB_open %s failed - not managed\n", lb_name);
	    rt->lb_fd = -1;
	    return;
	}
	else
	    LE_send_msg (GL_ERROR, "LB_open %s failed (ret %d)\n", lb_name, lb_fd);
	exit (1);
    }

    RSS_LB_register (lb_fd, LB_ID_ADDRESS, (void *)&(rt->msgid));

    if ((ret = LB_UN_register (lb_fd, LB_ANY, Lb_ntf_cb)) < 0 ||
	(!table->no_msg_exp_proc &&
	 (ret = LB_UN_register (lb_fd, LB_MSG_EXPIRED, Lb_ntf_cb))) < 0) {
	LE_send_msg (GL_ERROR, "LB_UN_register failed (ret %d)\n", ret);
	exit (1);
    }
    if (rt->src_msg_size > 0)
	RSS_LB_read_window (lb_fd, 0, rt->src_msg_size);

    /* find out the LB size */
    status.attr = &attr;
    status.n_check = 0;
    RSS_LB_stat (lb_fd, &status);
    rt->maxn_recs = attr.maxn_msgs;
    rt->lb_fd = lb_fd;
    rt->has_saved_recs = 0;
    LE_send_msg (LE_VL1, "LB %s (size: %d messages) opened\n", 
						lb_name, rt->maxn_recs);
    if (table->data_endian == SDQS_LOCAL_ENDIAN) {
	if (LB_misc (lb_fd, LB_IS_BIGENDIAN))
	    table->data_endian = SDQS_BIG_ENDIAN;
	else
	    table->data_endian = SDQS_LITTLE_ENDIAN;
    }
    if (table->data_endian == SDQS_SERIAL)
	table->lb_is_bigendian = LB_misc (lb_fd, LB_IS_BIGENDIAN);

    is_vsize = 0;		/* is there a variable size QF */
    p = table->types;		/* we dont save variable size records */
    for (i = 0; i < table->n_qfs; i++) {
	if (strstr (p, "*") != NULL)
	    is_vsize = 1;
	p += strlen (p) + 1;
    }
    if (status.n_msgs == 0 && 
	rt->src_msg_size >= rt->rec_size - sizeof (LB_id_t) && !is_vsize) {
				/* create the save record message */
	int magic[2], ret;
	magic[0] = SAV_REC_MAGIC;
	magic[1] = INT_BSWAP (SAV_REC_MAGIC);
	if ((ret = LB_write (lb_fd, (char *)magic, 
					2 * sizeof (int), LB_ANY)) < 0) {
	    LE_send_msg (GL_ERROR, 
	       "LB_write creating record saving msg failed (ret %d)", ret);
	    exit (1);
	}
	if ((ret = LB_previous_msgid (lb_fd)) != SAVED_RECORDS_MSGID) {
	    LE_send_msg (GL_ERROR, 
			    "Unexpected first LB message ID (%d)", ret);
	    exit (1);
	}
	rt->has_saved_recs = 1;
    }

    return;
}

/******************************************************************

    SDB LB NTF callback.

******************************************************************/

static void Lb_ntf_cb (int fd, LB_id_t msgid, int msg_len, void *arg) {
    int i;

    for (i = 0; i < N_tables; i++)
	if (Tables[i].rt.lb_fd == fd)
	    break;
    if (i >= N_tables) {
	LE_send_msg (GL_INFO, "Unexpected UN (fd %d)", fd);
	return;
    }
    if (msgid == SAVED_RECORDS_MSGID && Tables[i].rt.has_saved_recs)
	return;
    if (msg_len == LB_MSG_EXPIRED)
	Delete_record (Tables + i, msgid);
    else {
	runtime_t *rt = &(Tables[i].rt);
	if (!Tables[i].no_msg_exp_proc)
	    Delete_record (Tables + i, msgid);
	Read_and_insert (Tables + i, msgid, msg_len); 
	if (rt->House_keep != NULL)
	    rt->n_recs = rt->House_keep (SDQM_HK_NEW_RECORD, 
					rt->n_recs, rt->lb_fd, rt->db_id);
    }
}

/******************************************************************

    Removes the record of "msgid" in table "table".

******************************************************************/

static void Delete_record (table_t *table, LB_id_t msgid) {
    SDQM_index_results *qr;	/* query results */
    int ret;
    char *rec;
    runtime_t *rt;

    rt = &(table->rt);
    if (rt->msgid_field_ind < 0)
	return;

    SDQM_query_begin (table->name);
    SDQM_select_int_value (rt->msgid_field_ind, msgid);
    ret = SDQM_execute_query_index (1, (void *)&qr);
    if (ret <= 0) {
	if (ret < 0)
	    LE_send_msg (GL_ERROR, 
		"SDQM_execute_query_index (table %s) failed, ret %d\n", 
						table->name, ret);
	return;
    }
    if (qr->n_recs != 1) {	/* does not exist */
	free (qr);
	return;
    }

    rec = (char *)((char *)qr->rec_add + qr->list[0] * qr->rec_size);
    LE_send_msg (LE_VL1,  
	"delete record of msg_id %u in table %s\n", msgid, table->name);
    rt->Free_record (rec + sizeof (LB_id_t));
    ret = SDQM_delete (rt->db_id, qr->list[0]);
    if (ret < 0)
        LE_send_msg (GL_ERROR,  
	    "SDQM_delete (ind %d, table %s) failed (ret %d)\n", 
				qr->list[0], table->name, ret);
    free (qr);
    rt->n_recs--;	
}

/**************************************************************************

    Loads all dynamically loaded functions.

**************************************************************************/

static void Load_dynamic_routines () {
    void *handle;
    char *error;
    int i;
    SMI_info_t *(*smi_get_info)(char *, void *);

    LE_send_msg (GL_INFO, "Loading library: %s", SDQSM_get_shared_lib_name ());
    if ((handle = dlopen (SDQSM_get_shared_lib_name (), RTLD_LAZY)) == NULL) {
	LE_send_msg (GL_ERROR, "dlopen failed: %s\n", dlerror ());
	LE_send_msg (GL_INFO, "    - run sdqs with compilation\n");
	exit (1);
    }

    error = NULL;
    Get_macro = (char *(*)())dlsym (handle, GET_MACRO_FUNC_NAME);
    error = dlerror();

    if (error == NULL && No_compilation)
	Init_tables ();

    for (i = 0; i < N_tables; i++) {
	runtime_t *rt;

	rt = &(Tables[i].rt);
	if (rt->lb_fd < 0)
	    continue;
	if (error != NULL)
	    break;
	rt->Get_record = (int (*)())dlsym (handle, 
			SDQSC_get_func_name (GET_RECORD_FUNC, Tables + i));
	if ((error = dlerror()) != NULL)
	    break;
	rt->Byte_swap = (void (*)())dlsym (handle, 
			SDQSC_get_func_name (BYTE_SWAP_FUNC, Tables + i));
	if ((error = dlerror()) != NULL)
	    break;
	rt->Get_misc = (int (*)())dlsym (handle, 
			SDQSC_get_func_name (GET_MISC_FUNC, Tables + i));
	if ((error = dlerror()) != NULL)
	    break;
	rt->Free_record = (int (*)())dlsym (handle, 
			SDQSC_get_func_name (FREE_RECORD_FUNC, Tables + i));
	if ((error = dlerror()) != NULL)
	    break;
	if (No_compilation)
	    Complete_tables (Tables + i);
	rt->House_keep = NULL;
	if (Tables[i].house_keep_func[0] != '\0') {
	    rt->House_keep = (int (*)())dlsym (handle, 
						Tables[i].house_keep_func);
	    if ((error = dlerror()) != NULL)
		break;
	}
    }
    smi_get_info = NULL;
    if (error == NULL) {
	smi_get_info = (SMI_info_t *(*)(char *, void *))
				dlsym (handle, SDQSM_get_smi_func_name ());
	error = dlerror();
    }
    if (error != NULL) {
	LE_send_msg (GL_ERROR, "dlsym failed: %s\n", error);
	LE_send_msg (GL_ERROR, "    from %s\n", SDQSM_get_shared_lib_name ());
	exit (1);
    }

    SMIA_set_smi_func (smi_get_info);
}

/******************************************************************

    Reads table names and initialize the SDB tables.

******************************************************************/

static void Init_tables () {
    char *names, *p;
    int i;

    names = Get_macro ("SDQS_sdb_names");
    p = names;
    N_tables = 1;
    while (*p != '\0') {
	if (*p == ' ')
	    N_tables++;
	p++;
    }

    p = SDQSM_malloc (N_tables * sizeof (table_t) + strlen (names) + 4);
    memset (p, 0, N_tables * sizeof (table_t));
    Tables = (table_t *)p;
    p += N_tables * sizeof (table_t);
    strcpy (p, names);
    while (*p != '\0') {
	if (*p == ' ')
	    *p = '\0';
	p++;
    }
    p = (char *)Tables + N_tables * sizeof (table_t);
    for (i = 0; i < N_tables; i++) {
	Tables[i].name = p;
	p += strlen (p) + 1;
	Tables[i].n_dis = 0;
	Tables[i].data_input_func = "";
	Tables[i].smi = NULL;
	Tables[i].n_qfs = 0;
	Tables[i].qfs = Tables[i].types = "";
    }
}

/******************************************************************

    Reads "config_info" data and fills more fields in SDB table "table".

******************************************************************/

static void Complete_tables (table_t *table) {
    char *config_info, *buf, *p;
    int i, n_its;
    index_tree_t *its;

    table->rt.Get_misc ("config_info", &config_info);
    buf = SDQSM_malloc (strlen (config_info) + 1);
    strcpy (buf, config_info);
    p = buf;
    while (*p != '\0') {
	if (*p == ' ')
	    *p = '\0';
	p++;
    }

    p = buf;
    table->src_t = p;
    p += strlen (p) + 1;
    table->sdb_t = p;
    p += strlen (p) + 1;
    table->src_lb = p;
    p += strlen (p) + 1;
    if (strcmp (p, "NULL") == 0)
	table->house_keep_func = "";
    else
	table->house_keep_func = p;
    p += strlen (p) + 1;

    n_its = 0;
    sscanf (p, "%d", &n_its);
    p += strlen (p) + 1;
    its = (index_tree_t *)SDQSM_malloc (n_its * sizeof (index_tree_t));
    table->n_its = n_its;
    table->its = its;
    for (i = 0; i < n_its; i++) {
	int n_fields, m;
	sscanf (p, "%d", &n_fields);
	p += strlen (p) + 1;
	its[i].n_fields = n_fields;
	its[i].fields = p;
	for (m = 0; m < n_fields; m++) {
	    p += strlen (p) + 1;
	}
    }
    sscanf (p, "%d", &i);
    p += strlen (p) + 1;
    table->no_msg_exp_proc = i;
    sscanf (p, "%d", &i);
    p += strlen (p) + 1;
    table->data_endian = i;
    sscanf (p, "%d", &i);
    p += strlen (p) + 1;
    table->rt.src_msg_size = i;
}

/******************************************************************

    Saves all records in the LB "rt" in message SAVED_RECORDS_MSGID. 

******************************************************************/

static void Save_records (table_t *table) {
    runtime_t *rt;
    SDQM_index_results *qr;	/* query results */
    int ret, i, *ip;
    char *buf, *p;
    LB_info info;

    rt = &(table->rt);
    if (rt == NULL || !rt->has_saved_recs)
	return;
    if (LB_msg_info (rt->lb_fd, SAVED_RECORDS_MSGID, &info) < 0)
	return;				/* the message does not exist */

    SDQM_query_begin (table->name);
    SDQM_select_int_range (0, 0, 0x7fffffff);	/* need to extend to uint */
    ret = SDQM_execute_query_index (rt->maxn_recs, (void *)&qr);
    if (ret <= 0) {
	if (ret < 0)
	    LE_send_msg (GL_ERROR, 
			"SDQM_execute_query_index failed, ret %d\n", ret);
	return;
    }

    buf = malloc (qr->n_recs * qr->rec_size + 2 * sizeof (int));
    if (buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc faild\n");
	free (qr);
	return;
    }
    p = buf;
    ip = (int *)p;
    ip[0] = SAV_REC_MAGIC;
    ip[1] = INT_BSWAP (SAV_REC_MAGIC);
    p += 2 * sizeof (int);
    for (i = 0; i < qr->n_recs; i++) {
	memcpy (p, (char *)(qr->rec_add) + qr->list[i] * qr->rec_size, 
							qr->rec_size);
	p += qr->rec_size;
    }

    ret = LB_write (rt->lb_fd, buf, 
		qr->n_recs * qr->rec_size + 2 * sizeof (int), 
		SAVED_RECORDS_MSGID);
    free (buf);
    if (ret < 0)
	LE_send_msg (GL_ERROR, "LB_write saved records failed (%d)\n", ret);
    LE_send_msg (GL_INFO, "%d records saved", qr->n_recs);
    free (qr);
    return;
}


/**********************************************************************

    Retrieves the record of message ID "msgid" saved in previous rpgdbm
    session. NULL is returned if the record is not found. If "msgid"
    is 0, it frees the memory allocated. A sorted index table is built
    and binary search is used for fast lookup.

***********************************************************************/

static char *Get_saved_record (runtime_t *rt, LB_id_t msgid, char *db_name) {
    static char *buffer = NULL;
    static int n_recs = -1;
    char *recs;
    int st, end, rec_size;

    if (msgid == 0) {
	if (buffer != NULL)
	    free (buffer);
	buffer = NULL;
	n_recs = -1;
	return (NULL);
    }

    if (n_recs < 0) {		/* read all saved records and initialize */
	int size, *ip;

	rt->has_saved_recs = 0;
	n_recs = 0;
	RSS_LB_read_window (rt->lb_fd, 0, 0);
	size = RSS_LB_read (rt->lb_fd, (char *)&buffer, 
				LB_ALLOC_BUF, SAVED_RECORDS_MSGID);

	if (rt->src_msg_size > 0)
	    RSS_LB_read_window (rt->lb_fd, 0, rt->src_msg_size);
	if (size < 0) {
	    if (size != LB_NOT_FOUND)
		LE_send_msg (GL_INFO, 
		    "LB_read saved records failed (%d) - %s\n", size, db_name);
	    return (NULL);
	}
	ip = (int *)buffer;
	if (size < 2 * sizeof (int) || ip[0] != INT_BSWAP (ip[1]) ||
		(ip[0] != SAV_REC_MAGIC && ip[1] != SAV_REC_MAGIC)) {
	    LE_send_msg (LE_VL1, 
			"Saved record message not found - %s\n", db_name);
	    return (NULL);
	}
	n_recs = (size - 2 * sizeof (int)) / rt->rec_size;
	rt->has_saved_recs = 1;
    }
    if (n_recs == 0)
	return (NULL);

    /* binary search message ID */
    recs = buffer + 2 * sizeof (int);
    rec_size = rt->rec_size;
    st = 0;
    end = n_recs - 1;
    while (1) {
	int ind;

	ind = (st + end) >> 1;
	if (st == ind) {
	    if (*((int *)(recs + st * rec_size)) == msgid)
		return (recs + st * rec_size);
	    if (*((int *)(recs + end * rec_size)) == msgid)
		return (recs + end * rec_size);
	    return (NULL);
	}
	if (*((int *)(recs + ind * rec_size)) > msgid)
	    end = ind;
	else
	    st = ind;
    }
    return (NULL);
}

