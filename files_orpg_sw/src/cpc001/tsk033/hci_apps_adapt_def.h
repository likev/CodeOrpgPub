
/***********************************************************************

    Internal include file for hci_apps_adapt.

***********************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:47 $
 * $Id: hci_apps_adapt_def.h,v 1.10 2009/02/27 22:25:47 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */  

#ifndef HCI_APPS_ADAPT_DEF_H

#define HCI_APPS_ADAPT_DEF_H



#define MAX_NAME_SIZE 128

typedef struct {		/* *_o are offset in Sb */
    int id_o;			/* data element ID */
    int perm_o;			/* data element permission */
    int name_o;			/* data element name */
    int description_o;		/* data element description */
    int desc_o;			/* data element range */
    int type;			/* data element type */
    int de_num;			/* DE number in the original DAT file */
    int n_values;		/* number of values (array size) */
    void *values;		/* pointer to the value array */
    void *back_values;		/* backuped value array for undo */
    void *baseline;		/* pointer to the baseline value array. NULL
				   indicates bad baseline (not defined or the 
				   number of baseline values are not equal to 
				   the current). Bad baseline is not used or 
				   updated. */
    int n_sel_values;		/* the number of allowed values to select */
    int sel_values_o;		/* allowed values to select */
    short value_upd;		/* the value attr has been updated */
    short baseline_upd;		/* the baseline attr has been updated */
    double accuracy;		/* the accuracy attribute */
} Dea_t;			/* data element struct */

typedef struct {		/* *_o are offset in Sb */
    int app_name_o;		/* application name */
    int display_name_o;		/* apps name for display in the menu */
    int n_des;			/* number of DEs for this application */
    int de_ind;			/* index number in the full DE list */
    Dea_t *des;			/* list of DEs for this application */
    int name_field_width;	/* width of the name field */
    int desc_field_width;	/* width of the description field */
    int has_selection;		/* this app has at least one selection field */
} Apps_list_t;			/* application DE struct */


int HAA_gui_main (int operational);
void *HAA_malloc (int size);
int HAA_convert_to_double (char *tv, double *dp);
void HAA_update_apps_dea_file (int app_ind);
char *HAA_get_text_from_value (Dea_t *de, int ind);
int HAA_get_size_of_value_field (int type, int n_values, void *values);
int HAA_read_data_elements (int app_ind);
int HAA_set_values (char *id, int is_str_type, void *values, 
					int n_items, int is_base_line);
void HAA_delete_des ();
void HAA_read_app_names ();
void HAA_form_num_string (double v, int type, char *buf);
void HAA_set_init_app_name (char *optarg);


#endif		/* #ifndef HCI_APPS_ADAPT_DEF_H */
