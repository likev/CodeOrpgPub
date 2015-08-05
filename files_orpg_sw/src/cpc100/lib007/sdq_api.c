
/******************************************************************

	This implements the SDQ interface.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/31 20:36:30 $
 * $Id: sdq_api.c,v 1.16 2013/05/31 20:36:30 steves Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */  

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#if !defined(LINUX) && !defined(__WIN32__)
#include <inttypes.h>
#endif
#include <dlfcn.h>

#include <infr.h> 

#define LOCAL_NAME_SIZE 128

/* for sdb_table_info_t.data_endian */
enum {DATA_SERIAL, DATA_BIG_ENDIAN, DATA_LITTLE_ENDIAN};

typedef struct {		/* lb info */
    char *lb_name;		/* LB name */
    char *service;		/* service name for the LB */
    int is_big_endian;		/* the LB's endianness */
    int lbd;			/* open LB descriptor */
    char *dll_name;		/* base name of the sdqs shared lib */
    char *src_t;		/* source (LB message header) type */
    SMI_info_t *(*smi_func)(char *, void *);
				/* SMI function for this table; NULL indicates
				   not loaded. */
    int data_endian;		/* data format of the LB */
} lb_info_t;			/* per LB */

typedef struct {		/* table of LB names */
    char *call_name;		/* LB names used in SDQ calls */
    lb_info_t *info;		/* LB info for call_names */
} lb_list_t;			/* per call name */

#define DEFAULT_MAXN_RECS 1024
static int Query_mode = 0;	/* SQL mode (current SDQ_select only) */
static int Maxn_records = DEFAULT_MAXN_RECS;	
				/* current max number of records */

static lb_list_t *Lbns = NULL;	/* LB call name list */
static int N_lbns = 0;		/* number of items in Lbns */
static void *Lbns_tblid = NULL;	/* Lbns table id */

static int Cmp_name (void *e1, void *e2);
static int Get_sdb_info (char *call_name, lb_info_t **info_p);
int SDQ_get_info_from_lb (char *lb_name, char *addr,
						int *is_big_endian);
static int Get_sdqs_server_address (char *lb_name,
				lb_info_t **info_p, unsigned int hid);
static void Free_info (lb_info_t *info);
static int Get_sdb_and_open_lb (void *result, lb_info_t **sdb_p);
static int Write_lb_message (lb_info_t *sdb, 
			LB_id_t msg_id, char *msg, int msg_len);
static int Read_lb_message (lb_info_t *sdb, 
				LB_id_t msg_id, char **msg);
static int Parse_full_lb_name (char *lb_name, char **h_name, char **lb_n);


/******************************************************************

    Returns the number of records found in the search in query 
    result "rslt". Returns 0 on failure.

******************************************************************/

int SDQ_get_n_records_found (void *rslt) {
    SDQM_query_results *r = (SDQM_query_results *)rslt;
    if (r == NULL)
	return (0);
    return (r->n_recs_found);
}

/******************************************************************

    Returns the error code in query result "rslt". Returns -1 on 
    failure.

******************************************************************/

int SDQ_get_query_error (void *rslt) {
    SDQM_query_results *r = (SDQM_query_results *)rslt;
    if (r == NULL)
	return (-1);
    return (r->err_code);
}

/******************************************************************

    Returns the number of returned records in query result "rslt". 
    Returns 0 on failure.

******************************************************************/

int SDQ_get_n_records_returned (void *rslt) {
    SDQM_query_results *r = (SDQM_query_results *)rslt;
    if (r == NULL)
	return (0);
    return (r->n_recs);
}

/******************************************************************

    Returns the "n"-th query record in query result "rslt" with
    "rec". Returns non-zero on success or 0 on failure.

******************************************************************/

int SDQ_get_query_record (void *rslt, int n, void **recp) {
    int i;
    char *rec;
    SDQM_query_results *r = (SDQM_query_results *)rslt;

    if (r == NULL)
	return (0);
    if (n < 0 || n >= r->n_recs)
	return (0);
    rec = (char *)r + r->recs_off + n * r->rec_size;
    for (i = 0; i < r->n_vf; i++) {
	int foff;
	int *pt;

	foff = ((int *)((char *)r + r->vf_spec_off))[i];
	pt = (int *)(rec + foff);
	*((char **)pt) = (char *)r + *pt;
    }
    *recp = rec + sizeof (LB_id_t);
    return (1);
}

/******************************************************************

    Returns the "n"-th query record index in query result "rslt" with
    "ind". Returns non-zero on success or 0 on failure.

******************************************************************/

int SDQ_get_query_index (void *rslt, int n, int *ind) {
    SDQM_query_results *r = (SDQM_query_results *)rslt;

    if (r == NULL || r->index_off == 0)
	return (0);
    if (n < 0 || n >= r->n_recs)
	return (0);
    *ind = *((unsigned short *)(
		(char *)r + r->index_off + n * sizeof (unsigned short)));
    return (1);
}

/******************************************************************

    Retrieves the n-th message in the SDQ_select result. See "man sdq".

******************************************************************/

int SDQ_get_message (void *result, int n, char **message) {
    lb_info_t *sdb;
    int ret;
    LB_id_t msg_id;

    *message = NULL;
    ret = SDQ_get_msg_id (result, n, &msg_id);
    if (!ret)
	return (SDQM_RECORD_NOT_FOUND);

    ret = Get_sdb_and_open_lb (result, &sdb);
    if (ret < 0)
	return (ret);
    return (Read_lb_message (sdb, msg_id, message));
}

/******************************************************************

    Updates, with "message", the message in LB "lb_name" that matches
    expression "where". See "Man sdq" for more details.

******************************************************************/

int SDQ_update (char *lb_name, char *where, void *message, int msg_len) {
    lb_info_t *sdb;
    void *result;
    int ret;
    LB_id_t msg_id;

    SDQ_set_maximum_records (2);
    ret = SDQ_select (lb_name, where, &result);

    if (ret < 0)
	return (ret);
    if (ret == 0)
	return (SDQM_RECORD_NOT_FOUND);
    if (ret > 1)
	return (SDQM_MSG_AMBIGUOUS);
    ret = Get_sdb_and_open_lb (result, &sdb);
    if (ret < 0)
	return (ret);

    ret = SDQ_get_msg_id (result, 0, &msg_id);
    return (Write_lb_message (sdb, msg_id, (char *)message, msg_len));
}

/******************************************************************

    Locks/unlocks the message in LB "lb_name" that matches
    expression "where". See "Man sdq" for more details.

******************************************************************/

int SDQ_lock (char *lb_name, char *where, int command) {
    lb_info_t *sdb;
    void *result;
    int ret;
    LB_id_t msg_id;

    SDQ_set_maximum_records (2);
    ret = SDQ_select (lb_name, where, &result);
    if (ret < 0)
	return (ret);
    if (ret == 0)
	return (SDQM_RECORD_NOT_FOUND);
    if (ret > 1)
	return (SDQM_MSG_AMBIGUOUS);
    ret = Get_sdb_and_open_lb (result, &sdb);
    if (ret < 0)
	return (ret);
    ret = SDQ_get_msg_id (result, 0, &msg_id);
    if (!ret)
	return (SDQM_RECORD_NOT_FOUND);
    return (RSS_LB_lock (sdb->lbd, command, msg_id));
}

/******************************************************************

    Inserts "message" into LB "lb_name" if there is no message in it
    that matches expression "where". See "Man sdq" for more details.

******************************************************************/

int SDQ_insert (char *call_name, void *message, int msg_len) {
    lb_info_t *sdb;
    void *result;
    SDQM_query_results r;
    int ret, lbns_ind;

    if ((lbns_ind = Get_sdb_info (call_name, &sdb)) < 0)
	return (lbns_ind);
    if (sdb->dll_name == NULL) {
	SDQ_set_maximum_records (1);
	ret = SDQ_select (call_name, "", &result);
	if (ret < 0)
	    return (ret);
    }
    else {
	r.tmp_p = sdb;
	result = &r;
    }

    ret = Get_sdb_and_open_lb (result, &sdb);
    if (ret < 0)
	return (ret);
    return (Write_lb_message (sdb, LB_ANY, (char *)message, msg_len));
}

/******************************************************************

    Deletes messages in LB "lb_name" that matches expression "where". 
    See "Man sdq" for more details.

******************************************************************/

int SDQ_delete (char *lb_name, char *where) {
    lb_info_t *sdb;
    void *result;
    int n_msgs, ret, i;
    LB_id_t msg_id;

    n_msgs = SDQ_select (lb_name, where, &result);
    if (n_msgs <= 0)
	return (n_msgs);
    ret = Get_sdb_and_open_lb (result, &sdb);
    if (ret < 0)
	return (ret);
    for (i = 0; i < n_msgs; i++) {
	ret = SDQ_get_msg_id (result, i, &msg_id);
	if (!ret)
	    return (SDQM_RECORD_NOT_FOUND);
	ret = RSS_LB_delete (sdb->lbd, msg_id);
	if (ret < 0)
	    return (ret);
    }
    return (n_msgs);
}

/******************************************************************

    Finds and returns in "sdb_p" the sdb info associated with 
    SDQ_select result "result". It also verifies if the msg_id field 
    exists. It then opens the LB if it is not already opened. It
    returns 0 on success or a negative error number.

******************************************************************/

static int Get_sdb_and_open_lb (void *result, lb_info_t **sdb_p) {
    SDQM_query_results *r;
    lb_info_t *sdb;

    r = (SDQM_query_results *)result;
    sdb = (lb_info_t *)(r->tmp_p);

    if (sdb->lbd < 0) {
	char name[256], *h_name, *p;
	int ret = Parse_full_lb_name (sdb->lb_name, &h_name, &p);
	if (ret < 0)
	    return (SDQM_BAD_HOST_NAME);
	if (strlen (p) + strlen (h_name) + 1 >= 256) {
	    MISC_log ("SDQ: host name too long (%s)\n", h_name);
	    return (SDQM_BAD_HOST_NAME);
	}
	if (ret == 0)
	    sprintf (name, "%s", p);
	else
	    sprintf (name, "%s:%s", h_name, p);
	sdb->lbd = RSS_LB_open (name, LB_WRITE, NULL);
	if (sdb->lbd < 0)
	    return (sdb->lbd);
    }

    if (sdb->smi_func == NULL) {	/* load DLL for serialization */
	void *handle;
	SMI_info_t *(*smi_get_info)(char *, void *);
	char *error;
	int i;

	if (sdb->dll_name == NULL) {	/* get dll_name */
	    char *dll_n, *data_e, *src_t;
	    char *p = (char *)result + r->db_info_off;
	    dll_n = data_e = src_t = NULL;	/* no effect */
	    for (i = 0; i < 5; i++) {
		if (i == 0)
		    src_t = p;
		if (i == 3)
		    dll_n = p;
		if (i == 4)
		    data_e = p;
		p += strlen (p) + 1;
	    }
	    sdb->dll_name = (char *)MISC_malloc 
				(strlen (dll_n) + strlen (src_t) + 2);
	    strcpy (sdb->dll_name, dll_n);
	    sdb->src_t = sdb->dll_name + strlen (dll_n) + 1;
	    strcpy (sdb->src_t, src_t);
	    if (strcmp (data_e, "serial") == 0)
		sdb->data_endian = DATA_SERIAL;
	    else if (strcmp (data_e, "little endian") == 0)
		sdb->data_endian = DATA_LITTLE_ENDIAN;
	    else 
		sdb->data_endian = DATA_BIG_ENDIAN;
	}

	for (i = 0; i < N_lbns; i++) {
	    if (Lbns[i].info->dll_name != NULL &&
		strcmp (Lbns[i].info->dll_name, sdb->dll_name) == 0 &&
		Lbns[i].info->smi_func != NULL) {
		sdb->smi_func = Lbns[i].info->smi_func;
		break;
	    }
	}
	if (i >= N_lbns) {	/* not yet loaded */
	    char tmp[128];

	    sprintf (tmp, "lib%s.so", sdb->dll_name);
	    if ((handle = dlopen (tmp, RTLD_LAZY)) == NULL)
		return (SDQM_DLOPEN_FAILED);
	    sprintf (tmp, "SDQS_SMI_get_info_%s", sdb->dll_name);
	    smi_get_info = (SMI_info_t *(*)(char *, void *))
						dlsym (handle, tmp);
	    error = dlerror();
	    if (error != NULL) {
		dlclose (handle);
		return (SDQM_DLSYM_FAILED);
	    }
	    sdb->smi_func = smi_get_info;
	}
    }

    SMIA_set_smi_func (sdb->smi_func);
    *sdb_p = sdb;
    return (0);
}

/******************************************************************

    Returns, with "msg_id", the message ID for "ind"-th record in 
    query result "result". Returns 1 on success or 0 on failure.

******************************************************************/

int SDQ_get_msg_id (void *result, int ind, LB_id_t *msg_id) {
    SDQM_query_results *r;
    char *rec;

    r = (SDQM_query_results *)result;
    if (ind < 0 || ind >= r->n_recs)
	return (0);
    rec = (char *)r + r->recs_off + ind * r->rec_size;
    *msg_id = *((LB_id_t *)rec);
    return (1);
}

/******************************************************************

    Serializes "msg" and writes it with ID "msg_id" to the LB of 
    "sdb". "msg_len" is only used for type 3 VSS where it is the 
    total size of the message. Returns number of bytes written to 
    the LB on success or a negative error code.

******************************************************************/

static int Write_lb_message (lb_info_t *sdb, 
			LB_id_t msg_id, char *msg, int msg_len) {
    char *serial_data;
    int len, i_am_bigendian;

    if (sdb->data_endian == DATA_SERIAL) {
	len = SMIA_serialize (sdb->src_t, msg, &serial_data, msg_len);
	if (len < 0)
	    return (len);
	if (serial_data == msg && msg_len > len)
	    len = msg_len;
    }
    else {
	i_am_bigendian = MISC_i_am_bigendian ();
	if ((sdb->data_endian == DATA_BIG_ENDIAN && !i_am_bigendian) ||
	    (sdb->data_endian == DATA_LITTLE_ENDIAN && i_am_bigendian)) {
	    serial_data = (char *)malloc (msg_len);
	    if (serial_data == NULL)
		return (SDQM_MALLOC_FAILED);
	    memcpy (serial_data, msg, msg_len);
	    len = SMIA_bswap_input (sdb->src_t, serial_data, msg_len);
	    if (len >= 0 && msg_len > len)
		len = msg_len;
	}
	else {
	    serial_data = msg;
	    len = msg_len;
	}
    }

    if (len > 0)
	len = RSS_LB_write (sdb->lbd, serial_data, len, msg_id);

    if (serial_data != NULL && serial_data != msg)
	free (serial_data);
    return (len);
}

/******************************************************************

    Reads the message of ID "msg_id" from the LB of "sdb" and 
    deserializes it. The message is returned with "msg" on success.
    The caller must free the returned pointer if it is not NULL.
    Returns number of bytes used for deserializing on success or
    a negative error code.

******************************************************************/

static int Read_lb_message (lb_info_t *sdb, 
					LB_id_t msg_id, char **msg) {
    char *buf, *c_data;
    int ret, len, i_am_bigendian;

    *msg = NULL;
    len = RSS_LB_read (sdb->lbd, (char *)&buf, LB_ALLOC_BUF, msg_id);
    if (len < 0)
	return (len);
    i_am_bigendian = MISC_i_am_bigendian ();
    c_data = buf;
    ret = 0;
    if (sdb->data_endian == DATA_SERIAL)
	ret = SMIA_deserialize (sdb->src_t, buf, &c_data, len);
    else if ((sdb->data_endian == DATA_BIG_ENDIAN && !i_am_bigendian) ||
	     (sdb->data_endian == DATA_LITTLE_ENDIAN && i_am_bigendian))
	ret = SMIA_bswap_input (sdb->src_t, buf, len);
    if (ret < 0)
	return (ret);
    if (buf != NULL && buf != c_data)
	free (buf);
    *msg = c_data;
    return (len);
}

/******************************************************************

    Sets the maximum number of records can be returned.

******************************************************************/

void SDQ_set_maximum_records (int maxn_records) {
    Maxn_records = maxn_records;
    if (maxn_records <= 0)
	Maxn_records = DEFAULT_MAXN_RECS;
}

/******************************************************************

    Sets the query mode.

******************************************************************/

void SDQ_set_query_mode (int mode) {
    Query_mode = mode;
}

/******************************************************************

    Retrieves the n-th record in the SDQ_select result. See "man sdq".

******************************************************************/

int SDQ_get_record (void *result, int n, char **record) {
    int ret;
    ret = SDQ_get_query_record (result, n, (void **)record);
    if (ret) {
	SDQM_query_results *r = (SDQM_query_results *)result;
	return (r->rec_size - sizeof (LB_id_t));
    }
    return (SDQM_RECORD_NOT_FOUND);
}

/******************************************************************

    Executes data query on LB "lb_name". The select expression is
    in "where". It returns number of records found on success or
    a negative error code.

******************************************************************/

#define MAX_N_TKS 128			/* max number of tokens in "where" */
#define EXPR_SIZE 256			/* max size for select expressin */

int SDQ_select (char *call_name, char *where, void **result) {
    static char *vb = NULL;
    char buf[64];
    int mode, maxn_recs, lbns_ind, ret;
    lb_info_t *info;

#ifdef TEST_OPTIONS
    if (MISC_test_options ("PROFILE")) {
	char b[128];
	MISC_string_date_time (b, 128, (const time_t *)NULL);
	fprintf (stderr, "%s PROF: SDQ_select %s\n", b, call_name);
    }
#endif

    mode = Query_mode;
    Query_mode = 0;
    maxn_recs = Maxn_records;
    Maxn_records = DEFAULT_MAXN_RECS;

    if ((lbns_ind = Get_sdb_info (call_name, &info)) < 0)
	return (lbns_ind);

    vb = STR_copy (vb, "Select: ");
    vb = STR_cat (vb, info->lb_name);
    sprintf (buf, " %d ", mode);
    vb = STR_cat (vb, buf);
    sprintf (buf, "%d %d ", MISC_i_am_bigendian (), maxn_recs);
    vb = STR_cat (vb, buf);
    vb = STR_cat (vb, where);
    ret = CSS_get_service (info->service, 
				vb, STR_size (vb), 20, (char **)result);

    if (ret >= 0) {
	SDQM_query_results *r;
	r = (SDQM_query_results *)(*result);
	if (r->err_code != 0)
	    ret = -r->err_code;
	else
	    ret = r->n_recs;
	r->tmp_p = info;
    }
    else
	Free_info (info);		/* activate LB info re-read */

    return (ret);
}

/******************************************************************

    Returns the server server info in "info_p" for LB "call_name". 
    Returns the LB call_name table index on success or a negative 
    error number.

******************************************************************/

static int Get_sdb_info (char *call_name, lb_info_t **info_p) {
    lb_list_t ent;
    int ret, call_ind, i;
    char *lb_name;
    lb_info_t *info;
    unsigned int hid;

    if (Lbns_tblid == NULL) {		/* initialize the SDB info table */
	if ((Lbns_tblid = MISC_open_table (sizeof (lb_list_t), 
				8, 1, &N_lbns, (char **)&Lbns)) == NULL)
	    return (SDQM_MALLOC_FAILED);
    }

    call_ind = -1;
    ent.call_name = call_name;		/* search existing call names */
    if (MISC_table_search (Lbns_tblid, &ent, Cmp_name, &i)) {
	if (Lbns[i].info != NULL) {
	    *info_p = Lbns[i].info;
	    return (i);
	}
	call_ind = i;
    }

    lb_name = SDQM_get_full_lb_name (call_name, &hid);
    if (lb_name == NULL)
	return (SDQM_BAD_HOST_NAME);

    info = NULL;
    for (i = 0; i < N_lbns; i++) {  /* look for an alias */
	if (Lbns[i].info != NULL &&
			strcmp (lb_name, Lbns[i].info->lb_name) == 0) {
	    info = Lbns[i].info;
	    break;
	}
    }

    if (info == NULL) {			/* a new LB */
	ret = Get_sdqs_server_address (lb_name, &info, hid);
	if (ret < 0) {
	    free (lb_name);
	    return (ret);
	}
    }
    free (lb_name);

    if (call_ind >= 0) {
	Lbns[call_ind].info = info;
	*info_p = Lbns[call_ind].info;
	return (call_ind);
    }

    ent.call_name = (char *)malloc (strlen (call_name) + 1);
    if (ent.call_name == NULL)
	return (SDQM_MALLOC_FAILED);
    strcpy (ent.call_name, call_name);
    ent.info = info;
    if ((call_ind = MISC_table_insert (Lbns_tblid, &ent, Cmp_name)) < 0)
	return (SDQM_MALLOC_FAILED);

    *info_p = Lbns[call_ind].info;
    return (call_ind);
}

/**************************************************************************

    Returns host name and the lb name of the full name "lb_name". Returns
    1 if the lb is remote, 0 if local or -1 on failure.

**************************************************************************/

static int Parse_full_lb_name (char *lb_name, char **h_name, char **lb_n) {
    int hid;
    char *p;

    *h_name = "\0";
    if (sscanf (lb_name, "%d", &hid) != 1) {
	*lb_n = lb_name;
	return (0);
    }
    p = lb_name + MISC_char_cnt (lb_name, "\0:");
    if (*p != ':') {
	MISC_log ("SDQ: Bad full lb_name (%s)\n", lb_name);
	return (-1);
    }
    *lb_n = p + 1;
    if (hid == RMT_lookup_host_index (RMT_LHI_LIX, NULL, 0))	/* local LB */
	return (0);

    if (RMT_lookup_host_index (RMT_LHI_IX2H, h_name, hid) < 0) {
	MISC_log ("SDQ: %d_th (index) host not found (1)\n", hid);
	return (-1);
    }
    return (1);
}

/******************************************************************

    Creates the lb_info_t struct for LB "lb_name" and returns it 
    with "info_p". Returns 0 on success or a negative error code.

******************************************************************/

static int Get_sdqs_server_address (char *lb_name,
			lb_info_t **info_p, unsigned int hid) {
    lb_info_t *info;
    char addr[256], *p, service[128], *h_name;
    int is_big_endian, ret;

    ret = Parse_full_lb_name (lb_name, &h_name, &p);
    if (ret == -1)
	return (SDQM_BAD_HOST_NAME);
    if (ret == 0)	/* local LB */
	ret = SDQ_get_info_from_lb (p, addr, &is_big_endian);
    else {	/* remote lb - This involves single RPC instead of three. */
	char name[256];
	int rssret;
	if (strlen (h_name) + 64 > 256) {
	    MISC_log ("SDQ: Host name too long (%s)\n", h_name);
	    return (SDQM_BAD_HOST_NAME);
	}
	sprintf (name, "%s:%s", h_name, "SDQ_get_info_from_lb");
	rssret = RSS_rpc (name, "i-r s-i ba-128-o ia-o", 
				&ret, p, addr, &is_big_endian);
	if (rssret < 0)
	    return (rssret);
    }

    if (ret < 0)
	return (ret);

    sprintf (service, "srvc%s", addr);

    {		/* replace host index by host name in addr */
	int hid, port;
	char *h_name;
	if (sscanf (addr, "%d%*c%d", &hid, &port) != 2) {
	    MISC_log ("SDQ: Bad server address (%s) for %s\n", addr, lb_name);
	    return (SDQM_BAD_HOST_NAME);
	}
	if (RMT_lookup_host_index (RMT_LHI_IX2H, &h_name, hid) < 0 ||
		strlen (h_name) + 16 >= 256) {
	    MISC_log ("SDQ: %d_th (index) host not found or too long\n", hid);
	    return (SDQM_BAD_HOST_NAME);
	}
	sprintf (addr, "%s:%d", h_name, port);
    }

    ret = CSS_set_server_address (service, addr, CSS_ANY_LINK);
    if (ret < 0)
	return (ret);
    p = (char *)malloc (sizeof (lb_info_t) + strlen (lb_name) + 
						strlen (service) + 8);
    if (p == NULL)
	return (SDQM_MALLOC_FAILED);

    info = (lb_info_t *)p;
    p += sizeof (lb_info_t);
    info->lb_name = p;
    strcpy (info->lb_name, lb_name);
    p += strlen (lb_name) + 1;
    info->service = p;
    strcpy (info->service, service);
    p += strlen (service) + 1;
    info->is_big_endian = is_big_endian;
    info->lbd = -1;
    info->smi_func = NULL;
    info->dll_name = NULL;

    *info_p = info;
    return (0);
}

/******************************************************************

    Frees the server info struct "info" and removes referenced to 
    it from the call name table.

******************************************************************/

static void Free_info (lb_info_t *info) {
    int i;

    for (i = 0; i < N_lbns; i++) {
	if (Lbns[i].info == info)
	    Lbns[i].info = NULL;
    }
    if (info->dll_name != NULL)
	free (info->dll_name);
    if (info->lbd >= 0)
	RSS_LB_close (info->lbd);
    free (info);
}

/******************************************************************

    Gets the sdqs host index, port number and endianness from LB 
    "lb_name" and returns them with "addr" and "is_big_endian". 
    Returns 0 on success or a negative error number. We make this
    function global so it can be called with RPC.

******************************************************************/

int SDQ_get_info_from_lb (char *lb_name, char *addr, 
						int *is_big_endian) {
    int fd, port, ret;
    unsigned int ip;

    if ((fd = LB_open (lb_name, LB_READ, NULL)) < 0)
	return (fd);
    if ((ret = LB_sdqs_address (fd, 0, &port, &ip)) < 0) {
	LB_close (fd);
	return (ret);
    }
    sprintf (addr, "%d:%d", ip, port);
    *is_big_endian = LB_misc (fd, LB_IS_BIGENDIAN);
    LB_close (fd);
    return (0);
}

/************************************************************************

    Name comparison function for LB call name table search and insertion.

************************************************************************/

static int Cmp_name (void *e1, void *e2) {
    lb_list_t *f1, *f2;
    f1 = (lb_list_t *)e1;
    f2 = (lb_list_t *)e2;
    return (strcmp (f1->call_name, f2->call_name));
}

/**************************************************************************

    Constructs the full LB name in the form of host_index:full_path. This
    is saved in the sdqs server for matching that sent from the client
    for meta query. This will not work for file links. The input can be
    "path" or "host_name:path". The function returns the full LB name on
    success or NULL on failure (e.g. the specified host name not found).
    The caller must free the returned pointer if it is not NULL. If "hid"
    is not NULL, the host index is returned with this argument.

**************************************************************************/

char *SDQM_get_full_lb_name (char *name, unsigned int *hid) {
    char *cpt, host[LOCAL_NAME_SIZE], *path;
    unsigned int ip;
    int ind;

    host[0] = '\0';
    path = name;
    cpt = name;
    while (*cpt != '\0') {
	if (*cpt == ':') {
	    int len = cpt - name;
	    if (len >= LOCAL_NAME_SIZE)
		len = LOCAL_NAME_SIZE - 1;
	    memcpy (host, name, len);
	    host[len] = '\0';
	    path = cpt + 1;
	    break;
	}
	cpt++;
    }

    ip = NET_get_ip_by_name (host);
    if (ip == INADDR_NONE)
	return (NULL);

    ind = RMT_lookup_host_index (RMT_LHI_I2IX, &ip, 0);
    if (ind < 0) {
	MISC_log ("SDQ: host index for IP %x not found\n", ip);
	return (NULL);
    }

    cpt = (char *)malloc (strlen (path) + 64);
    if (cpt == NULL)
	return (NULL);
    sprintf (cpt, "%d:%s", ind, path);

    if (hid != NULL)
	*hid = ind;
    return (cpt);
}




