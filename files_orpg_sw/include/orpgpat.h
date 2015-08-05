/************************************************************************
 *									*
 *	Module:  orpgpat.h						*
 *		This is the global include file for ORPGPAT.		*
 *									*
 ************************************************************************/


/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 15:59:31 $
 * $Id: orpgpat.h,v 1.39 2005/12/27 15:59:31 steves Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */


#ifndef ORPGPAT_H

#define ORPGPAT_H

/* Message ID of Table is product tables LB. */
#define PROD_ATTR_MSG_ID	3 /* message ID for the prod attribute table */

#define	MAX_PAT_TBL_SIZE	2000

#define	MAX_MNE_LENGTH		4	/* max length of product mneumonic */
#define PROD_NAME_LEN           32 	/* max length of product name */
#define PARAMETER_NAME_LEN      32 	/* max length of parameter descriptor */
#define PARAMETER_UNITS_LEN     16 	/* max length of parameter units string */

/* Product type macro definitions. */
#define	TYPE_VOLUME		0
#define	TYPE_ELEVATION		1
#define	TYPE_TIME		2
#define	TYPE_ON_DEMAND		3
#define	TYPE_ON_REQUEST		4
#define TYPE_RADIAL		5
#define TYPE_EXTERNAL		6

/* Data compression type macro definitions. */
#define COMPRESSION_NONE        0
#define COMPRESSION_BZIP2       1
#define COMPRESSION_ZLIB        2

/* Product format type macro definitions. */

#define FORMAT_TYPE_RADIAL		1
#define FORMAT_TYPE_HIRES_RADIAL	2
#define FORMAT_TYPE_RASTER		3
#define FORMAT_TYPE_HIRES_RASTER	4
#define FORMAT_TYPE_BASEDATA		5
#define FORMAT_TYPE_BASEDATA_ELEV	6

/* Product resolution flags macros	*/

#define ORPGPAT_X_AZI_RES	0
#define ORPGPAT_Y_RAN_RES	1


#define ORPGPAT_MIN_PRIORITY    0
#define ORPGPAT_MAX_PRIORITY    255

#define	STRIP_NOTHING			0
#define	STRIP_MNEMONIC			1

#define	ORPGPAT_DATA_NOT_FOUND		-999
#define ORPGPAT_ERROR			-998

#define	ORPGPAT_MAX_PRODUCT_CODE	2000

#include <mrpg.h>
#include <cs.h>

#include <orpg_def.h>
#include <prod_distri_info.h>
#include <gen_stat_msg.h>
#include <basedata.h>


typedef struct {                /* product type attribute table entry */
    short entry_size;           /* size of this table entry; < 0 means unused
                                   record; SMI_vss_size this->entry_size; */
    prod_id_t prod_id;          /* product buffer number; < 0 indicates
                                   array-terminating entry */
    short class_id;             /* class ID of the class of data for which the
                                   product belongs (e.g., BASEDATA is the class
                                   id for prod_id REFLDATA and COMBBASE). */
    short class_mask;           /* mask value to get the product type from the
                                   class. */
    char gen_task[ORPG_TASKNAME_SIZ];         
                                /* name of the task that generates this product
                                   type */
    short wx_modes;             /* bit fields indicating the product type can
                                   be generated for certain weather modes: bit
                                   0 for weather mode 0 and so on. */
    char disabled;              /* yes(non-zero)/no(0); The product is disabled
                                   by the operator */
    char  n_priority;           /* number of weather modes + default (each
                                   weather mode has its own priority for
                                   the product). */
    short priority_list;        /* offset, in number of bytes from the
                                   beginning of this structure, of the
                                   priority list associated with this product
                                   type (default, M, A, B). */
    char compression;           /* integer field indicating data compression
                                   and compression type (see macros defined
                                   in orpgpat.h). */
    char format_type;           /* integer field indicating the format of the
                                   product (see macros defined in orpgpat.h).*/
    short x_azi_res;            /* product resolution: E-W resolution if
                                   cartesian (meters) and azimuth resolution
                                   (degrees*10) if polar. */
    short y_ran_res;            /* product resolution: N-S resolution if
                                   cartesian (meters) and range resolution
                                   (meters) if polar. */
    short n_dep_prods;          /* number of input products required to
                                   generate this product */
    short dep_prods_list;       /* offset, in number of bytes from the
                                   beginning of this structure, of the index
                                   list of the products that are required
                                   in order to generate this product */
    short n_opt_prods;          /* number of optional input products used to
                                   generate this product */
    short opt_prods_list;       /* offset, in number of bytes from the
                                   beginning of this structure, of the index
                                   list of the products that are not required
                                   (i.e., optional) in order to generate this
                                   product */
    short prod_code;            /* legacy product code; use 0 for non-legacy
                                   products */
    short type;                 /* Type of product from:
                                        0 = Volume
                                        1 = Elevation
                                        2 = Time
                                        3 = On Demand
                                        4 = On Request  */
    short elev_index;           /* elevation parameter index (for elevation
                                   based products only). -1 means not an
                                   elevation based product. */
    short alert;                /* Alert/threshold product paired flag
                                        0 = Alert Pairing disallowed
                                        1 = Alert Pairing allowed */
    int warehoused;             /* Number of seconds to warehouse the product.
                                   A value of 0 indicates the product is not
                                   warehoused. */
    int warehouse_id;           /* Data ID where data is warehoused. */
    int warehouse_acct_id;      /* Data ID where warehouse accounting data is stored. */
    int max_size;               /* maximum product size (bytes).        */
    short desc;                 /* Offset, in number of bytes from the
                                   beginning of this structure, of the
                                   product description string (NULL terminated
                                   string). */
    short n_params;             /* Number of product parameters defined
                                   for this product.    */
    short params;               /* Offset, in number of bytes from the
                                   beginning of this structure, of the
                                   product parameter list.      */
    char name [PROD_NAME_LEN];  /* Name of product */
    short aliased_prod_id;      /* Product ID alias. */
    /* SMI_vss_field  prod_id_t f1[this->n_dep_prods] (this->dep_prods_list) */
    /* SMI_vss_field  prod_id_t f2[this->n_params] (this->params) */
    /* SMI_vss_field  prod_id_t f3[this->n_priority] (this->priority_list) */
    /* SMI_vss_field  prod_id_t f4[this->n_opt_prods] (this->opt_prods_list) */
} Pd_attr_entry;

/* Structure defining the attributes for each product parameter.        */

typedef struct {

    short       index;          /* Product parameter index.             */
    short       min;            /* Minimum allowable value (scaled)     */
    short       max;            /* Maximum allowable value (scaled)     */
    short       def;            /* Default value (scaled)               */
    int         scale;          /* Scale factor applied to parameter    */
    char        name [PARAMETER_NAME_LEN];
                                /* Parameter descriptor.                */
    char        units [PARAMETER_UNITS_LEN];
                                /* Parameter units descriptor.          */

} Pd_params_entry;

/*	Leave the following typedef to support ASCII PAT access via	*
 *	ORPGDATA functions.						*/

typedef struct {
	size_t	size_bytes;	/* Size of memory array of PAT entries	*/
	Pd_attr_entry	*ptr;	/* pointer to array of PAT entries	*/
} Orpgpat_mem_ary_t;


/*	Functions dealing with product attributes table		*/

void	ORPGPAT_error (void (*user_exception_callback)());

int 	ORPGPAT_io_status();
int	ORPGPAT_clear_tbl ();
int     ORPGPAT_read_ASCII_PAT( char *filename );
int	ORPGPAT_read_tbl  ();
int	ORPGPAT_write_tbl ();
int	ORPGPAT_get_update_flag ();
void	ORPGPAT_set_update_flag ();

char	*ORPGPAT_get_tbl_ptr     (int indx);
Pd_attr_entry	*ORPGPAT_get_tbl_entry (int prod_id);

/* The following function is used by the ORPGDA libary and is not	* 
 * intended to be used publicly.					*/
Mrpg_data_t     *ORPGPAT_get_data_table_entry (int prod_id, int *size);


int	ORPGPAT_elevation_based  (int prod_id);
int	ORPGPAT_prod_in_tbl     (int prod_id);

int	ORPGPAT_add_prod       (int prod_id);
int	ORPGPAT_delete_prod    (int prod_id);

int	ORPGPAT_num_tbl_items ();

/*	The following functions get specific Product Attributes Table	*
 *	properties.  Most of these functions identify an attributes	*
 *	record by product ID (prod_id); note that buffer_number is the	*
 *	same as prod_id.						*/

int	ORPGPAT_get_code                    (int prod_id);
int	ORPGPAT_get_type                    (int prod_id);
int	ORPGPAT_get_class_id                (int prod_id);
unsigned int ORPGPAT_get_class_mask         (int prod_id);
int	ORPGPAT_get_alert                   (int prod_id);
int     ORPGPAT_get_aliased_prod_id         (int prod_id);
int	ORPGPAT_get_warehoused              (int prod_id);
int	ORPGPAT_get_warehouse_id            (int prod_id);
int	ORPGPAT_get_warehouse_acct_id       (int prod_id);
int	ORPGPAT_get_elevation_index         (int prod_id);
int	ORPGPAT_get_compression_type        (int prod_id);
int	ORPGPAT_get_format_type             (int prod_id);
int	ORPGPAT_get_resolution              (int prod_id, int flg);
int	ORPGPAT_get_disabled                (int prod_id);
char   *ORPGPAT_get_gen_task                (int prod_id);
int	ORPGPAT_get_max_size                (int prod_id);
int	ORPGPAT_get_wx_modes                (int prod_id);
int	ORPGPAT_get_prod_id                 (int indx);
int	ORPGPAT_get_prod_id_from_code       (int indx);
char	*ORPGPAT_get_description     (int prod_id, int option);
char	*ORPGPAT_get_mnemonic        (int prod_id);

int	ORPGPAT_add_parameter         (int prod_id);
int	ORPGPAT_delete_parameter      (int prod_id, int indx);
int	ORPGPAT_get_num_parameters    (int prod_id);
int	ORPGPAT_get_parameter_index   (int prod_id, int indx);
int	ORPGPAT_get_parameter_min     (int prod_id, int indx);
int	ORPGPAT_get_parameter_max     (int prod_id, int indx);
int	ORPGPAT_get_parameter_default (int prod_id, int indx);
int	ORPGPAT_get_parameter_scale   (int prod_id, int indx);
char	*ORPGPAT_get_parameter_name   (int prod_id, int indx);
char	*ORPGPAT_get_parameter_units  (int prod_id, int indx);

int	ORPGPAT_get_num_priorities  (int prod_id);
int	ORPGPAT_add_priority        (int prod_id, int priority);
int	ORPGPAT_delete_priority     (int prod_id, int indx);
int	ORPGPAT_get_priority        (int prod_id, int indx);
short	*ORPGPAT_get_priority_list  (int prod_id);
int	ORPGPAT_set_priority	    (int prod_id, int indx, int priority);

int	ORPGPAT_get_num_dep_prods   (int prod_id);
int	ORPGPAT_add_dep_prod        (int prod_id, int dep_prod_id);
int	ORPGPAT_delete_dep_prod     (int prod_id, int indx);
int	ORPGPAT_get_dep_prod        (int prod_id, int indx);
short	*ORPGPAT_get_dep_prods_list (int prod_id);
int	ORPGPAT_set_dep_prod	    (int prod_id, int indx, int dep_prod_id);

int	ORPGPAT_get_num_opt_prods   (int prod_id);
int	ORPGPAT_add_opt_prod        (int prod_id, int dep_prod_id);
int	ORPGPAT_delete_opt_prod     (int prod_id, int indx);
int	ORPGPAT_get_opt_prod        (int prod_id, int indx);
short	*ORPGPAT_get_opt_prods_list (int prod_id);
int	ORPGPAT_set_opt_prod	    (int prod_id, int indx, int dep_prod_id);

char	*ORPGPAT_get_name                   (int prod_id);
int	ORPGPAT_get_prod_id_by_name         (char *name);


/*	The following functions set specific Product Attributes Table	*
 *	properties.							*/

int    ORPGPAT_set_code                    (int prod_id, int val);
int    ORPGPAT_set_type                    (int prod_id, int val);
int    ORPGPAT_set_class_id                (int prod_id, int val);
unsigned int ORPGPAT_set_class_mask        (int prod_id, unsigned int val);
int    ORPGPAT_set_aliased_prod_id         (int prod_id, int val);
int    ORPGPAT_set_alert                   (int prod_id, int val);
int    ORPGPAT_set_warehoused              (int prod_id, int val);
int    ORPGPAT_set_warehouse_id            (int prod_id, int val);
int    ORPGPAT_set_warehouse_acct_id       (int prod_id, int val);
int    ORPGPAT_set_elevation_index         (int prod_id, int val);
int    ORPGPAT_set_compression_type        (int prod_id, int val);
int    ORPGPAT_set_format_type             (int prod_id, int val);
int    ORPGPAT_set_resolution              (int prod_id, int flag, int val);
int    ORPGPAT_set_disabled                (int prod_id, int val);
int    ORPGPAT_set_gen_task                (int prod_id, char *task_name);
int    ORPGPAT_set_max_size                (int prod_id, int val);
int    ORPGPAT_set_wx_modes                (int prod_id, int val);
int    ORPGPAT_set_prod_id                 (int indx,    int val);
int    ORPGPAT_set_parameter_index         (int prod_id, int indx,   int val);
int    ORPGPAT_set_parameter_min           (int prod_id, int indx,   int val);
int    ORPGPAT_set_parameter_max           (int prod_id, int indx,   int val);
int    ORPGPAT_set_parameter_default       (int prod_id, int indx,   int val);
int    ORPGPAT_set_parameter_scale         (int prod_id, int indx,   int val);
int    ORPGPAT_set_parameter_name          (int prod_id, int indx,   char *val);
int    ORPGPAT_set_parameter_units         (int prod_id, int indx,   char *val);
int    ORPGPAT_set_description             (int prod_id, char *val);
int    ORPGPAT_set_name			   (int prod_id, char  *val);

/*
 * ORPG Product Tables (PRODT) CS File
 */
#define ORPGPAT_CS_DFLT_PT_FNAME	"product_tables"
#define ORPGPAT_CS_PT_COMMENT		'#'
#define ORPGPAT_CS_PT_MAXLINELEN       80


#define ORPGPAT_CS_ATTRTBL_KEY		"Prod_attr_table"
#define ORPGPAT_CS_DFLTGEN_KEY		"Default_prod_gen"

#define ORPGPAT_CS_ATTR_PROD_KEY	"Product"

#define ORPGPAT_CS_PROD_ID_KEY		"prod_id"
#define ORPGPAT_CS_PROD_ID_TOK		(1 | (CS_SHORT))
#define ORPGPAT_CS_ALIASED_PROD_ID_KEY	"aliased_prod_id"
#define ORPGPAT_CS_ALIASED_PROD_ID_TOK	(1 | (CS_SHORT))
#define ORPGPAT_CS_CLASS_ID_KEY		"class_id"
#define ORPGPAT_CS_CLASS_ID_TOK		(1 | (CS_SHORT))
#define ORPGPAT_CS_CLASS_MASK_KEY	"class_mask"
#define ORPGPAT_CS_CLASS_MASK_TOK	(1 | (CS_SHORT))
#define ORPGPAT_CS_PROD_CODE_KEY	"prod_code"
#define ORPGPAT_CS_PROD_CODE_TOK	(1 | (CS_SHORT))
#define ORPGPAT_CS_GEN_TASK_KEY		"gen_task"
#define ORPGPAT_CS_WX_MODES_KEY		"wx_modes"
#define ORPGPAT_CS_WX_MODES_TOK		(1 | (CS_SHORT))
#define ORPGPAT_CS_DISABLED_KEY		"disabled"
#define ORPGPAT_CS_DISABLED_TOK		(1 | (CS_BYTE))
#define ORPGPAT_CS_N_PRIORITY_KEY	"n_priority"
#define ORPGPAT_CS_N_PRIORITY_TOK	(1 | (CS_BYTE))
#define ORPGPAT_CS_COMPRESSION_KEY	"compression"
#define ORPGPAT_CS_COMPRESSION_TOK	(1 | (CS_BYTE))
#define ORPGPAT_CS_FORMAT_TYPE_KEY	"format_type"
#define ORPGPAT_CS_FORMAT_TYPE_TOK	(1 | (CS_BYTE))
#define ORPGPAT_CS_N_DEP_PRODS_KEY	"n_dep_prods"
#define ORPGPAT_CS_N_DEP_PRODS_TOK	(1 | (CS_BYTE))
#define ORPGPAT_CS_N_OPT_PRODS_KEY	"n_opt_prods"
#define ORPGPAT_CS_N_OPT_PRODS_TOK	(1 | (CS_BYTE))
#define ORPGPAT_CS_ALERT_KEY		"alert"
#define ORPGPAT_CS_ALERT_TOK		(1 | (CS_SHORT))
#define ORPGPAT_CS_WAREHOUSED_KEY	"warehoused"
#define ORPGPAT_CS_WAREHOUSED_TOK	(1 | (CS_INT))
#define ORPGPAT_CS_WAREHOUSE_ID_KEY	"warehouse_id"
#define ORPGPAT_CS_WAREHOUSE_ID_TOK	(1 | (CS_INT))
#define ORPGPAT_CS_WAREHOUSE_ACCT_ID_KEY "warehouse_acct_id"
#define ORPGPAT_CS_WAREHOUSE_ACCT_ID_TOK (1 | (CS_INT))
#define ORPGPAT_CS_TYPE_KEY		"type"
#define ORPGPAT_CS_TYPE_TOK		(1 | (CS_SHORT))
#define ORPGPAT_CS_MAX_SIZE_KEY		"max_size"
#define ORPGPAT_CS_MAX_SIZE_TOK		(1 | (CS_INT))
#define ORPGPAT_CS_ELEV_INDEX_KEY	"elev_index"
#define ORPGPAT_CS_ELEV_INDEX_TOK	(1 | (CS_SHORT))

#define ORPGPAT_CS_PRIORITY_LIST_KEY		"priority_list"
#define ORPGPAT_CS_PRIORITY_LIST_TOK_FLAG	(CS_SHORT)
#define ORPGPAT_CS_DEP_PRODS_LIST_KEY		"dep_prods_list"
#define ORPGPAT_CS_DEP_PRODS_LIST_TOK_FLAG	(CS_SHORT)
#define ORPGPAT_CS_OPT_PRODS_LIST_KEY		"opt_prods_list"
#define ORPGPAT_CS_OPT_PRODS_LIST_TOK_FLAG	(CS_SHORT)

#define ORPGPAT_CS_DESCRIPTION_KEY	"desc"
#define ORPGPAT_CS_DESCRIPTION_TOK	1
#define ORPGPAT_CS_DESCRIPTION_BUFSIZE	(ORPGPAT_CS_PT_MAXLINELEN)

#define ORPGPAT_CS_NAME_KEY	"prod_id"
#define ORPGPAT_CS_NAME_TOK	2
#define ORPGPAT_CS_NAME_BUFSIZE	(PROD_NAME_LEN)

/*
 * Parameter List tokens ...
 */
#define ORPGPAT_CS_PARM_KEY		"params"
#define ORPGPAT_CS_PARM_TOK		1
#define ORPGPAT_CS_PARM_INDEX_TOK	(0 | (CS_SHORT))
#define ORPGPAT_CS_PARAM_MIN_TOK	(1 | (CS_SHORT))
#define ORPGPAT_CS_PARAM_MAX_TOK	(2 | (CS_SHORT))
#define ORPGPAT_CS_PARAM_DEF_TOK	(3 | (CS_SHORT))
#define ORPGPAT_CS_PARAM_SCALE_TOK	(4 | (CS_SHORT))
#define ORPGPAT_CS_PARM_NAME_TOK	5
#define ORPGPAT_CS_PARM_NAME_BUFSIZE	(PARAMETER_NAME_LEN)
#define ORPGPAT_CS_PARM_UNITS_TOK	6
#define ORPGPAT_CS_PARM_UNITS_BUFSIZE	(PARAMETER_UNITS_LEN)

#define ORPGPAT_CS_PARAM_UNUSED_STRING	      "UNU"
#define ORPGPAT_CS_PARAM_ANY_VALUE_STRING	   "ANY"
#define ORPGPAT_CS_PARAM_ALG_SET_STRING	      "ALG"
#define ORPGPAT_CS_PARAM_ALL_VALUES_STRING	   "ALL"
#define ORPGPAT_CS_PARAM_ALL_EXISTING_STRING	"EXS"


#endif
