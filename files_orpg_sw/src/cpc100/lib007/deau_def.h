/*******************************************************************

    Private header file for the data element attribute utility library 
    module.

*******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:40:23 $
 * $Id: deau_def.h,v 1.9 2005/09/14 15:40:23 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */


#ifndef DEAU_DEF_H
#define DEAU_DEF_H

#include <deau.h>

enum {				/* for DEAU_range_t.range_type */
    DEAU_R_UNDEFINED, DEAU_R_NOT_EVALUATED, DEAU_R_MINMAX, 
	DEAU_R_DISCRETE_NUMBERS, DEAU_R_DISCRETE_STRINGS, DEAU_R_METHOD};

enum {				/* for DEAU_range_t.b_min and b_max */
    DEAU_R_INCLUSIVE, DEAU_R_NOT_INCLUSIVE, DEAU_R_NOT_DEFINED};

enum {				/* for DEAU_conversion_t.conv_type */
    DEAU_C_UNDEFINED, DEAU_C_NOT_EVALUATED, DEAU_C_SCALE, DEAU_C_METHOD};

typedef struct {
    char range_type;		/* range type (DEAU_R_UNDEFINED ...) */
    char b_min;			/* min boundary type (DEAU_R_INCLUSIVE ...) */
    char b_max;			/* max boundary type (DEAU_R_INCLUSIVE ...) */
    double min;			/* min value */
    double max;			/* max value */
    int n_values;		/* number of valid discrete values */
    char *values;		/* list of valid discrete values. An array of
				   doubles or a multiple null terminated string
				   */
    int (*method) (void *);	/* custom range checking function */
} DEAU_range_t;

typedef struct {
    char conv_type;		/* conversion type (DEAU_C_UNDEFINED ...) */
/*    char src_type;		 source data type (DEAU_T_UNDEFINED ...) */
    double scale;		/* conversion scale factor */
    double offset;		/* conversion offset value */
    void (*method) (void *, void *);
				/* custom conversion function */
} DEAU_conversion_t;

typedef struct {		/* hash table entry for LB dea data base */
    unsigned int hash;		/* hash value */
    LB_id_t msgid;		/* message id */
} DEAU_hash_e_t;

typedef struct {		/* hash table struct for LB dea data base */
    int is_big_endian;		/* the data is in big endian byte order */
    int sizeof_des;		/* size of array des */
    DEAU_hash_e_t *des;		/* array of hash table entries */
} DEAU_hash_tbl_t;

/* The hash table message consists of the structure of DEAU_hash_tbl_t. The 
   hash table is sorted in terms of the hash value. */

typedef struct {		/* identifier table entry for LB dea data base 
				   */
    unsigned int offset;	/* identifier offset value in the id buffer */
    LB_id_t msgid;		/* message id */
} DEAU_id_e_t;

/* The identifier table message consists of DEAU_id_tbl_t followed by the DE
   identifier string buffer. The table is sorted in terms of the identifier 
   strings.
*/

#define	DEAU_HASH_TABLE 1	/* LB msgid for the hash table */

int DEAU_rep_err_ret (char *msg, char *desc, int ret_value);
int DEAU_parse_conversion_desc (char *desc, DEAU_conversion_t *c);
int DEAU_parse_range_desc (char *desc, int type, DEAU_range_t *r);
unsigned int DEAU_get_hash_value (char *name);
int DEAU_set_next_dea (int ind);
char *DEAU_get_LB_name ();
int DEAU_serialize_dea (DEAU_attr_t *de, char **buf);
char *DEAU_get_node_name (char *node);
void DEAU_set_get_cache_attr_by_id (DEAU_attr_t *(*func) (char *, DEAU_attr_t *));
int DEAU_get_data_type_by_string (char *type_name);


#endif			/* #ifndef DEAU_DEF_H */
