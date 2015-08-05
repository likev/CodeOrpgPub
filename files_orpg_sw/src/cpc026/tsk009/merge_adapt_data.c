
/******************************************************************

    This is the main module for process_adapt_data.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/03/05 22:19:16 $
 * $Id: merge_adapt_data.c,v 1.17 2009/03/05 22:19:16 jing Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#include "mad_def.h"


static char *Merge_src = NULL;	/* source DB file (LB) for merge */
static char *Merge_dest = NULL;	/* destination DB file for merge */
static int Legacy_source;	/* use legacy source for merge */
static int Verbose;
static int Diff_DBs = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Merge ();
static int Get_next_old_values (char **de_id, int *type, char **values);
static int Initialize ();
static int Is_different (int type, char *p, char *pn);
static void Convert_from_enum (DEAU_attr_t *at, 
				int n_values, int *type, void **values);
static void Print_new_values (int is_string, void *values, int n_values);
static int Get_next_old_atts (char **de_id, char **atts);
static int Do_diff_DBS ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {

/*
MADRL_test ();
exit (0);
*/

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Diff_DBs) {
	if (Initialize () < 0 ||
	    Do_diff_DBS () < 0)
	    exit (2);
	exit (0);
    }

    if (Initialize () < 0 ||
	(!Legacy_source &&
		Get_next_old_values (NULL, NULL, NULL) < 0))
	exit (1);

    Merge ();
    exit (0);
}

/**************************************************************************

    Prints an array of values for testing purpose.

**************************************************************************/

/*
static void Print_values (int type, int n_values, void *p) {
    int i;

    printf ("    %d values: ", n_values);
    for (i = 0; i < n_values; i++) {
	if (type == DEAU_T_STRING) {
	    printf ("%s ", (char *)p);
	    p += strlen (p) + 1;
	}
	else {
	    printf ("%6.2f ", *((double *)p));
	    p += sizeof (double);
	}
    }
    printf ("\n");
}
*/

/**************************************************************************

    Initializes the task.

**************************************************************************/

static int Initialize () {
    char *p;

    if (Merge_src == NULL) {
	fprintf (stderr, "Source dir is not specified\n");
	return (-1);
    }
    if (Merge_dest == NULL) {
	fprintf (stderr, "Destination dir is not specified\n");
	return (-1);
    }

    p = Merge_src + strlen (Merge_src) - 1;
    while (p >= Merge_src && *p == '/') {
	*p = '\0';
	p--;
    }
    Merge_src = STR_cat (Merge_src, "/adapt_data.lb.adapt##");

    p = Merge_dest + strlen (Merge_dest) - 1;
    while (p >= Merge_dest && *p == '/') {
	*p = '\0';
	p--;
    }
    Merge_dest = STR_cat (Merge_dest, "/adapt_data.lb.adapt##");
    return (0);
}

/**************************************************************************

    Returns the values for the next DE in the source (old) DB. Only editable
    DEs are processed. The DE ID, type and values are returned with the 
    respective parameters. Returns the number of values on success or
    0 if there is no more DEs. Returns an negative error code on failure.
    If de_id = NULL, this function reads all editable DE values from the
    source DB. Source DE's without a value is not processed.

**************************************************************************/

static int Get_next_old_values (char **de_id, int *type, char **values) {
    static char *ivb = NULL, *buf = NULL, *dbuf = NULL;
    static int cnt, ind, n_fids, byte_order_fixed;
    DEAU_attr_t *at;
    char *id, *p, *fids;
    int ret, i;
    typedef struct {
	int type;
	int id_off;
	int n_values;
	int value_off;
    } id_value_t;
    id_value_t *ivs;

    if (de_id != NULL) {
	if (ind >= cnt || ivb == NULL)
	    return (0);
	ivs = (id_value_t *)ivb + ind;
	ind++;
	*de_id = buf + ivs->id_off;
	*type = ivs->type;
	if (ivs->type == DEAU_T_STRING)
	    *values = buf + ivs->value_off;
	else
	    *values = dbuf + ivs->value_off;
	return (ivs->n_values);
    }
    if (ivb != NULL)		/* already initialized */
	return (0);

    /* find all force-merge items */
    DEAU_LB_name (Merge_dest);	
    fids = NULL;
    n_fids = 0;
    DEAU_get_next_dea (NULL, NULL);
    while (1) {
	ret = DEAU_get_next_dea (&id, &at);
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    fprintf (stderr, "DEA LB (%s) access failed (%d)\n",
						Merge_dest, ret);
	    return (-1);
	}
	if (strstr (at->ats[DEAU_AT_MISC], "MERGE@-merge-@") != NULL) {
	    p = at->ats[DEAU_AT_ID];
	    fids = STR_append (fids, p, strlen (p) + 1);
	    n_fids++;
	}
    }

    if (Verbose)
	printf ("Reading merge source (%s)\n", Merge_src);
    DEAU_LB_name (Merge_src);
    ivb = STR_reset (ivb, 10000);
    buf = STR_reset (buf, 100000);
    dbuf = STR_reset (dbuf, 100000);
    byte_order_fixed = 0;
    ind = 0;
    cnt = 0;
    DEAU_get_next_dea (NULL, NULL);
    while (1) {
	ret = DEAU_get_next_dea (&id, &at);
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    fprintf (stderr, "DEA LB (%s) access failed (%d)\n",
						Merge_src, ret);
	    return (-1);
	}
	if (ret == LB_BAD_BYTE_ORDER && !byte_order_fixed) {
	    ret = LB_fix_byte_order (Merge_src);
	    if (ret < 0) {
		fprintf (stderr, "LB_fix_byte_order %s failed (ret %d)\n",
				Merge_src, ret);
		return (-1);
	    }
	    byte_order_fixed = 1;
	    if (Verbose)
		printf ("Byte order fix LB %s\n", Merge_src);
	    continue;
	}
	if (ret < 0) {
	    fprintf (stderr, "DEAU_get_next_dea (%s) failed (%d)\n", Merge_src, ret);
	    return (-1);
	}
	p = fids;
	for (i = 0; i < n_fids; i++) {
	    if (strcmp (at->ats[DEAU_AT_ID], p) == 0)
		break;
	    p += strlen (p) + 1;
	}
	if (i < n_fids ||
	    DEAU_check_permission (at, "URC") > 0 ||
	    DEAU_check_permission (at, "AGENCY") > 0) {
	    id_value_t iv;
	    int nv;

	    iv.id_off = STR_size (buf);
	    buf = STR_append (buf, (char *)at->ats[DEAU_AT_ID], 
				    strlen (at->ats[DEAU_AT_ID]) + 1);
	    iv.type = DEAU_get_data_type (at);
	    nv = DEAU_get_number_of_values (NULL);
	    if (nv > 0) {
		iv.n_values = nv;
		if (iv.type == DEAU_T_STRING) {
		    iv.value_off = STR_size (buf);
		    if ((ret = DEAU_get_string_values (NULL, &p)) != nv) {
			fprintf (stderr, "DEAU_get_string_values failed (%d)\n", ret);
			return (-1);
		    }
		    for (i = 0; i < nv; i++) {
			buf = STR_append (buf, p, strlen (p) + 1);
			p += strlen (p) + 1;
		    }
		}
		else {
		    static char *tbf = NULL;
		    iv.value_off = STR_size (dbuf);
		    tbf = STR_reset (tbf, nv * sizeof (double));
		    DEAU_get_values (NULL, (double *)tbf, nv);
		    dbuf = STR_append (dbuf, tbf, nv * sizeof (double));
		}
		ivb = STR_append (ivb, (char *)&iv, sizeof (id_value_t));
		cnt++;
	    }
	}
    }
    STR_free (fids);
    return (0);
}

/**************************************************************************

    Merges the old data values into "Merge_dest". The old values comes
    either from the legacy or the new adapt DB. Merging is performed if
    the permission, type and range checks are all favorable and the values
    are sufficiently different.

**************************************************************************/

static void Merge () {

    if (Verbose)
	printf ("Merging to (%s)\n", Merge_dest);
    DEAU_LB_name (Merge_dest);
    while (1) {
	DEAU_attr_t *at;
	char *values, *id, *p, *pn;
	int type, t, n_values, n_nvs, n, ret, ret1, diff;
	double dbuf[512];

        if ((n_values = Get_next_old_values 
					(&id, &type, &values)) <= 0)
           break;

	ret = DEAU_get_attr_by_id (id, &at);
	if (ret >= 0) {		/* DE exists in the destination DB */
	    if (DEAU_check_permission (at, "URC") <= 0 &&
		DEAU_check_permission (at, "AGENCY") <= 0) {
		if (strstr (at->ats[DEAU_AT_MISC], "MERGE@-merge-@") != NULL) {
		    if (Verbose)
			printf ("Data (id %s) forced to merge\n", id);
		}
		else {
		    if (Verbose)
			printf (
			"Data (id %s) permission changed - not merged\n", id);
		    continue;
		}
	    }
	    if (strstr (at->ats[DEAU_AT_MISC], "MERGE@-Not_merge") != NULL) {
		if (Verbose)
		    printf ("Data (id %s) merge disabled - not merged\n", id);
		continue;
	    }
	    if (Legacy_source && type != DEAU_T_STRING)
		Convert_from_enum (at, n_values, &type, (void *)&values);
	    t = DEAU_get_data_type (at);
	    if ((type == DEAU_T_STRING && t != DEAU_T_STRING) ||
		(t == DEAU_T_STRING && type != DEAU_T_STRING)) {
		if (Verbose)
		    printf ("Data (id %s) type does not match - not merged\n",
								id);
		continue;
	    }
	    if (type == DEAU_T_STRING)
		n_nvs = DEAU_get_string_values (NULL, &pn);
	    else {
		n_nvs = DEAU_get_values (NULL, dbuf, 512);
		pn = (char *)dbuf;
	    }
	    p = values;
	    diff = 0;
	    for (n = 0; n < n_values; n++) {
		if (n < n_nvs && Is_different (type, p, pn))
		    diff = 1;
		if (type == DEAU_T_STRING)
		    ret = DEAU_check_data_range (NULL, type, 1, p);
		else
		    ret = DEAU_check_data_range (NULL, DEAU_T_DOUBLE, 1, p);
		if (ret == -1) {
		    if (Verbose)
			printf ("Data (id %s) out of range - not merged\n", 
								id);
		    break;
		}
		else if (ret < 0) {
		    if (Verbose)
			printf (
			"Data (id %s) range check failed (%d) - not merged\n", 
							id, ret);
		    break;
		}
		if (type == DEAU_T_STRING) {
		    p += strlen (p) + 1;
		    pn += strlen (pn) + 1;
		}
		else {
		    p += sizeof (double);
		    pn += sizeof (double);
		}
	    }
	    if (n < n_values || (n_values == n_nvs && !diff))
		continue;

	    if (type == DEAU_T_STRING) {
		if (Verbose)
		    Print_new_values (1, values, n_values);
		ret = DEAU_set_values (id, 1, values, n_values, 0);
		ret1 = DEAU_set_values (id, 1, values, n_values, 1);
	    }
	    else {
		if (Verbose)
		    Print_new_values (0, values, n_values);
		ret = DEAU_set_values (id, 0, values, n_values, 0);
		ret1 = DEAU_set_values (id, 0, values, n_values, 1);
	    }
	    if (ret < 0)
		fprintf (stderr, "DEAU_set_values value (id %s) failed (%d)\n", 
						    id, ret);
	    else if (ret1 < 0)
		fprintf (stderr, "DEAU_set_values baseline (id %s) failed (%d)\n",
						    id, ret1);
	    else if (Verbose)
		printf ("Merge (id %s) done\n", id);
	}
    }

    if (Verbose)
	printf ("merge_adapt_data completed\n");
    return;
}

/*************************************************************************

    Prints "values".

*************************************************************************/

static void Print_new_values (int is_string, void *values, int n_values) {
    int i;

    if (is_string) {
	char *p;
	p = (char *)values;
	for (i = 0; i < n_values; i++) {
	    printf ("    New: %s\n", p);
	    p += strlen (p) + 1;
	}
    }
    else {
	double *d;
	d = (double *)values;
	for (i = 0; i < n_values; i++)
	    printf ("    New: %f\n", d[i]);
    }
}

/**************************************************************************

    Converts an array of "n_values" enum values (double, stored in
    "values") to their string values. The pointer to the string values
    are returned with "values". "type" returnes DEAU_T_STRING. "at" is
    the attributes of the DE.

**************************************************************************/

static void Convert_from_enum (DEAU_attr_t *at, 
				int n_values, int *type, void **values) {
    static char *buf = NULL;
    double *d;
    int i;

    buf = STR_reset (buf, 100);
    d = (double *)(*values);
    for (i = 0; i < n_values; i++) {
	int en, ret;
	char str[512];

	en = (int)d[i];
	ret = DEAU_get_enum (at, str, &en, 512);
	if (ret < 0)
	    return;
	buf = STR_append (buf, str, strlen (str) + 1);
    }
    *type = DEAU_T_STRING;
    *values = buf;
}

/************************************************************************

    Compares two values pointed by "p" and "pn" of type "type". If they
    different, returns 1. Otherwise returns 0. If two numerical values 
    are close enough, we consider them identical.

************************************************************************/

static int Is_different (int type, char *p, char *pn) {
    double d1, d2, max, diff;

    if (type == DEAU_T_STRING) {
	double v1, v2;
	char c;
	if (sscanf (p, "%lf%c", &v1, &c) != 1 || 
				sscanf (pn, "%lf%c", &v2, &c) != 1) {
	    if (strcmp (p, pn) == 0)
		return (0);
	    return (1);
	}
	d1 = (double)v1;
	d2 = (double)v2;
    }
    else {
	d1 = *((double *)p);
	d2 = *((double *)pn);
    }
    if (d1 * d2 < 0.)
	return (1);
    if (d1 < 0.)
	d1 = -d1;
    if (d2 < 0.)
	d2 = -d2;
    max = d1;
    if (d2 > max)
	max = d2;
    if (max == 0.)
	return (0);
    diff = d1 - d2;
    if (diff < 0.)
	diff = -diff;
    if (diff < .00001 * max)
	return (0);
    return (1);
}

/**************************************************************************

    Returns the attributes for the next DE in the source (old) DB. The
    DE ID and the pointer to attributes are returned with the
    respective parameters. Returns the size of the attributes on
    success or 0 if there is no more DEs. Returns an negative error
    code on failure. If de_id = NULL, this function reads all DE
    values from the source DB.

**************************************************************************/

static int Get_next_old_atts (char **de_id, char **atts) {
    typedef struct {
	int idoff;
	int atoff;
	int atsize;
    } de_t;
    static de_t *des = NULL;
    static char *atbuf = NULL;
    static int n_des = 0, index = 0;
    int atoff;

    if (de_id != NULL) {
	int s;
	if (index >= n_des || des == NULL)
	    return (0);
	*de_id = atbuf + des[index].idoff;
	*atts = atbuf + des[index].atoff;
	s = des[index].atsize;
	index++;
	return (s);
    }

    if (des != NULL)		/* already initialized */
	return (0);
    if (Verbose)
	printf ("Reading merge source (%s)\n", Merge_src);
    DEAU_LB_name (Merge_src);
    atoff = n_des = 0;
    index = 0;
    DEAU_get_next_dea (NULL, NULL);
    while (1) {
	DEAU_attr_t *at;
	de_t de;
	int ret, size;
	char *p;

	ret = DEAU_get_next_dea (NULL, &at);
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    fprintf (stderr, "DEAU_get_next_dea (%s) failed (%d)\n", Merge_src, ret);
	    return (ret);
	}
	p = (char *)at->ats[DEAU_AT_N_ATS - 1];
	size = (p - (char *)at->ats[0]) + strlen (p) + 1;
	atbuf = STR_append (atbuf, (char *)at->ats[0], size);
	de.idoff = atoff + (char *)at->ats[DEAU_AT_ID] - (char *)at->ats[0];
	de.atoff = atoff;
	de.atsize = size;
	des = (de_t *)STR_append ((char *)des, (char *)&de, sizeof (de_t));
	atoff += size;
	n_des++;
    }
    return (0);
}

/**************************************************************************

    Compares attrubutes of all DEs in the source DB with that in the old.
    If a difference is found, the function exits with 1. Otherwise it 
    returns 0 on success or a negative error code.

**************************************************************************/

static int Do_diff_DBS () {
    int cnt, total, ret;
    char *idp;

    if (Verbose)
	printf ("Do diff between DBs (%s %s)\n", Merge_src, Merge_dest);

    if ((ret = Get_next_old_atts (NULL, NULL)) < 0)
	return (ret);
    DEAU_LB_name (Merge_dest);
    cnt = 0;
    while (1) {
	DEAU_attr_t *at;
	char *atts, *id, *p;
	int size, s;

        if ((size = Get_next_old_atts (&id, &atts)) == 0)
           break;
	if (size < 0)
	    return (size);

	ret = DEAU_get_attr_by_id (id, &at);
	if (ret == DEAU_DE_NOT_FOUND) {
	    if (Verbose)
		printf ("DE (%s) not found in %s\n", id, Merge_dest);
	    exit (1);
	}
	if (ret < 0) {
	    fprintf (stderr, "DEAU_get_attr_by_id (id %s, %s) failed %d\n", 
					id, Merge_dest, ret);
	    return (ret);
	}
	p = (char *)at->ats[DEAU_AT_N_ATS - 1];
	s = (p - (char *)at->ats[0]) + strlen (p) + 1;
	if (s != size) {
	    if (Verbose)
		printf ("Size differ (%d, %d, %s)\n", s, size, id);
	    exit (1);
	}
	if (memcmp ((char *)at->ats[0], atts, s) != 0) {
	    if (Verbose)
		printf ("Attr differ (%s)\n", id);
	    exit (1);
	}
	cnt++;
    }
    DEAU_get_next_dea (NULL, NULL);
    total = 0;
    while (DEAU_get_next_dea (&idp, NULL) >= 0)
	total++;
    if (total != cnt) {
	if (Verbose)
	    printf ("Total number of DEs differ (%d %d)\n", cnt, total);
	exit (1);
    }
    return (0);

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
    Verbose = 0;
    while ((c = getopt (argc, argv, "s:d:cvh?")) != EOF) {
	switch (c) {

	    case 's':
		Merge_src = STR_copy (Merge_src, optarg);
		break;

	    case 'd':
		Merge_dest = STR_copy (Merge_dest, optarg);
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'c':
		Diff_DBs = 1;
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
        Merge adaptation data during installation. The old data are\n\
        read from source_dir and merged into dest_dir.\n\
        Options:\n\
            -c (Checks if the source and destination DBs are identical.\n\
                Returns 0 if they are, 1 if not, or 2 on errors)\n\
            -v (Verbose mode)\n\
            -h (Print usage info)\n\
";

    printf ("Usage:  %s [options] -s source_dir -d dest_dir \n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}



