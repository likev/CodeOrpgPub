
/******************************************************************

    This module reads the adaptation data from the named data store.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/08/24 18:16:29 $
 * $Id: read_legacy.c,v 1.7 2005/08/24 18:16:29 ryans Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  


#include "mad_def.h"

#define NUM_DATA_ITEMS 47        /* num elements in Data_items array */

typedef struct {
    char *de_id;	/* ID in the DE database */
    char *obj_name;     /* object (message) names in the named data store */
    char *type;		/* data type: float, int, short or string */
    int cat_num;        /* Alert category number */
    int n_values;	/* number of values */
    void *value;	/* char * for string type double * for other types */
} Data_entry_t;

/* statically define all of the Alerting adaptation data except the values */
static Data_entry_t Lgcy_data_items[] =
{
   {"Alert.velocity.thresh", "Velocity", "int", 1, 6, NULL},
   {"Alert.velocity.prod_code", "Velocity", "int", 1, 1, NULL},
   {"Alert.comp_refl.thresh", "Composite Refl", "int", 2, 6, NULL},
   {"Alert.comp_refl.prod_code", "Composite Refl", "int", 2, 1, NULL},
   {"Alert.echo_tops.thresh", "Echo Tops", "int", 3, 4, NULL},
   {"Alert.echo_tops.prod_code", "Echo Tops", "int", 3, 1, NULL},
   {"Alert.svr_wx_prob.thresh", "SVR Wx Prob", "int", 4, 5, NULL},
   {"Alert.svr_wx_prob.prod_code", "SVR Wx Prob", "int", 4, 1, NULL},
   {"Alert.vil.thresh", "VIL", "int", 6, 6, NULL},
   {"Alert.vil.prod_code", "VIL", "int", 6, 1, NULL},
   {"Alert.vad.thresh", "VAD", "int", 7, 6, NULL},
   {"Alert.vad.prod_code", "VAD", "int", 7, 1, NULL},
   {"Alert.max_hail_sz_vol.thresh", "Max Hail Size", "int", 8, 6, NULL},
   {"Alert.max_hail_sz_vol.prod_code", "Max Hail Size", "int", 8, 1, NULL},
   {"Alert.meso_vol.thresh", "Mesocyclone", "int", 9, 3, NULL},
   {"Alert.meso_vol.prod_code", "Mesocyclone", "int", 9, 1, NULL},
   {"Alert.tvs_vol.thresh", "TVS", "int", 10, 2, NULL},
   {"Alert.tvs_vol.prod_code", "TVS", "int", 10, 1, NULL},
   {"Alert.max_refl_vol.thresh", "Max Storm Refl", "int", 11, 6, NULL},
   {"Alert.max_refl_vol.prod_code", "Max Storm Refl", "int", 11, 1, NULL},
   {"Alert.prob_hail_vol.thresh", "Prob Hail", "int", 12, 6, NULL},
   {"Alert.prob_hail_vol.prod_code", "Prob Hail", "int", 12, 1, NULL},
   {"Alert.prob_svr_hail_vol.thresh", "Prob SVR Hail", "int", 13, 6, NULL},
   {"Alert.prob_svr_hail_vol.prod_code", "Prob SVR Hail", "int", 13, 1, NULL},
   {"Alert.storm_top_vol.thresh", "Storm Top", "int", 14, 6, NULL},
   {"Alert.storm_top_vol.prod_code", "Storm Top", "int", 14, 1, NULL},
   {"Alert.max_1hr_precip.thresh", "Max 1hr Precip", "int", 15, 4, NULL},
   {"Alert.max_1hr_precip.prod_code", "Max 1hr Precip", "int", 15, 1, NULL},
   {"Alert.max_hail_sz_fcst.thresh", "Max Hail Size", "int", 25, 6, NULL},
   {"Alert.max_hail_sz_fcst.prod_code", "Max Hail Size", "int", 25, 1, NULL},
   {"Alert.meso_fcst.thresh", "Mesocyclone", "int", 26, 3, NULL},
   {"Alert.meso_fcst.prod_code", "Mesocyclone", "int", 26, 1, NULL},
   {"Alert.tvs_fcst.thresh", "TVS", "int", 27, 2, NULL},
   {"Alert.tvs_fcst.prod_code", "TVS", "int", 27, 1, NULL},
   {"Alert.max_refl_fcst.thresh", "Max Storm Refl", "int", 28, 6, NULL},
   {"Alert.max_refl_fcst.prod_code", "Max Storm Refl", "int", 28, 1, NULL},
   {"Alert.prob_hail_fcst.thresh", "Prob Hail", "int", 29, 6, NULL},
   {"Alert.prob_hail_fcst.prod_code", "Prob Hail", "int", 29, 1, NULL},
   {"Alert.prob_svr_hail_fcst.thresh", "Prob SVR Hail", "int", 30, 6, NULL},
   {"Alert.prob_svr_hail_fcst.prod_code", "Prob SVR Hail", "int", 30, 1, NULL},
   {"Alert.storm_top_fcst.thresh", "Storm Top", "int", 31, 6, NULL},
   {"Alert.storm_top_fcst.prod_code", "Storm Top", "int", 31, 1, NULL},
   {"site_info.hci_password_agency", "", "string", -1, 1, NULL},
   {"site_info.hci_password_roc", "", "string", -1, 1, NULL},
   {"site_info.hci_password_urc", "", "string", -1, 1, NULL},
   {"ccz.legacy_zones", "", "string", -1, 1, NULL},
   {"ccz.orda_zones", "", "string", -1, 1, NULL}
};

static int Next_ind = 0;

static void Parse_legacy_alert_data (int fd, char *names, int len);
static void Parse_legacy_hci_data (int fd, char *buf, int len);
static void Parse_legacy_ccz_data (int fd, char *lgcy_buf, char *orda_buf,
                                   int len);

/**************************************************************************

    Returns the legacy adaptation data values for the next DE in the legacy 
    data table. The de_id, type and values are returned with "de_id", "type"
    and "values" respectively. It returns the number of values on success 
    or 0 on failure. All enum values are returned as numerical. Everything 
    is numerical except the color tables.

**************************************************************************/

int MADRL_next_legacy_values (char **de_id, int *type, void **values) {

    while (1) {
	if (Next_ind >= NUM_DATA_ITEMS)
	    return (0);
	if (Lgcy_data_items[Next_ind].n_values > 0)
	    break;
	Next_ind++;
    }
    *de_id = Lgcy_data_items[Next_ind].de_id;
    *values = Lgcy_data_items[Next_ind].value;
    if (strcmp (Lgcy_data_items[Next_ind].type, "string") == 0)
	*type = DEAU_T_STRING;
    else
	*type = DEAU_T_DOUBLE;
    Next_ind++;
    return (Lgcy_data_items[Next_ind - 1].n_values);
}

/**************************************************************************

    Reads the legacy data LBs and initializes legacy data structures.

**************************************************************************/

int MADRL_read_legacy_data (char *lb_dir) {
    static int initialized = 0;

    if (!initialized) {
        char* lb_name;
	char *buf;
	char *lgcy_buf;
	char *orda_buf;
	int fd, len;

        /* Alerting data - construct alerting LB name */
        lb_name = STR_copy(lb_name, lb_dir);
        lb_name = STR_cat(lb_name, "/alert_threshold.lb.adapt##");

	fd = LB_open (lb_name, LB_READ, NULL);
	if (fd == LB_OPEN_FAILED) {
	    printf ("Legacy alert data not found\n");
	    return (fd);
	}
	if (fd < 0) {
	    printf ("LB_open %s failed (%d)\n", lb_name, fd);
	    return (fd);
	}

	len = LB_read (fd, (char *)&buf, LB_ALLOC_BUF, 1);
	if (len < 0) {
	    printf ("LB_read names failed (%d)\n", len);
	    return (len);
	}
	Parse_legacy_alert_data (fd, buf, len);
	free (buf);

        /* HCI passwd data - construct LB name */
        lb_name = STR_copy(lb_name, lb_dir);
        lb_name = STR_cat(lb_name, "/hci_data.lb.adapt##");

	fd = LB_open (lb_name, LB_READ, NULL);
	if (fd == LB_OPEN_FAILED) {
	    printf ("Legacy HCI data not found\n");
	    return (fd);
	}
	if (fd < 0) {
	    printf ("LB_open %s failed (%d)\n", lb_name, fd);
	    return (fd);
	}

	len = LB_read (fd, (char *)&buf, LB_ALLOC_BUF, 2);
	if (len < 0) {
	    printf ("LB_read names failed (%d)\n", len);
	    return (len);
	}
	Parse_legacy_hci_data (fd, buf, len);
	free (buf);

        /* CCZ data - construct LB name */
        lb_name = STR_copy(lb_name, lb_dir);
        lb_name = STR_cat(lb_name, "/rda_clutter.lb.adapt##");

	fd = LB_open (lb_name, LB_READ, NULL);
	if (fd == LB_OPEN_FAILED) {
	    printf ("Legacy CCZ data not found\n");
	    return (fd);
	}
	if (fd < 0) {
	    printf ("LB_open %s failed (%d)\n", lb_name, fd);
	    return (fd);
	}

        /* First read the Legacy RDA CCZ data */
	len = LB_read (fd, (char *)&lgcy_buf, LB_ALLOC_BUF, 2);
	if (len < 0) {
	    printf ("LB_read names failed (%d)\n", len);
	    return (len);
	}

        /* Now read the Open RDA CCZ data */
	len = LB_read (fd, (char *)&orda_buf, LB_ALLOC_BUF, 7);
	if (len < 0) {
	    printf ("LB_read names failed (%d)\n", len);
	    return (len);
	}

	Parse_legacy_ccz_data (fd, lgcy_buf, orda_buf, len);
	free (lgcy_buf);
	free (orda_buf);

	Next_ind = 0;
	initialized = 1;
    }

    return (0);
}

/**************************************************************************

    This function uses the alerting data structure and the buffer
    containing the alerting data (passed in) to assign the proper values
    in the Lgcy_data_items array. 
    
**************************************************************************/

static void Parse_legacy_alert_data (int fd, char *buf, int len) {
 
   orpg_alert_threshold_data_t* alt_data;
   short* thresh_ptr;  /* ptr to threshold data in legacy structure */
   int i, j, k, num_cat;

   /* from the length, determine the number of categories */
   num_cat = len / sizeof(orpg_alert_threshold_data_t);

   /* loop through the categories */
   for ( i = 0; i < num_cat; i++ )
   {
      alt_data = (orpg_alert_threshold_data_t*)
         (buf + i*sizeof(orpg_alert_threshold_data_t));   

      /* find the corresponding data item */
      for ( j = 0; j < NUM_DATA_ITEMS; j++ )
      {
         if ( Lgcy_data_items[j].cat_num == alt_data->category )
         {
            /* found a match!  need to allocate space for the value field
               and save the values in the structure. */
 
            /* thresh value field */
            Lgcy_data_items[j].value = MISC_malloc( Lgcy_data_items[j].n_values *
               sizeof(double) );
            thresh_ptr = &(alt_data->thresh_1);
            for ( k = 0; k < Lgcy_data_items[j].n_values; k++)
            {
               *((double*)(Lgcy_data_items[j].value) + k) =
                  (double)*(thresh_ptr + k);
            }

            /* prod_code value field */
            Lgcy_data_items[j+1].value = MISC_malloc( 1 * sizeof(double) );
            *(double *)(Lgcy_data_items[j+1].value) = (double)(alt_data->prod_code);

            break;   /* out of for loop */
         }
      }
   }
} /* end Parse_legacy_alert_data() */

/**************************************************************************

    This function uses the HCI password data structure and the buffer
    containing the HCI password data (passed in) to assign the proper values
    in the Lgcy_data_items array. 
    
**************************************************************************/

static void Parse_legacy_hci_data (int fd, char *buf, int len) {
 
   Hci_password_t* passwd_data;
   char agency_psswd[9];
   char* agency_buf;
   char agency_psswd_new[HCI_PASSWORD_LEN];
   char roc_psswd[9];
   char* roc_buf;
   char roc_psswd_new[HCI_PASSWORD_LEN];
   char urc_psswd[9];
   char* urc_buf;
   char urc_psswd_new[HCI_PASSWORD_LEN];

   passwd_data = (Hci_password_t*) (buf);   


   /* Decrypt the passwords, re-encrypt with new system, then store
      new value in the data entry struct. */
   Get_encrypted_passwd( passwd_data->agency, agency_psswd);
   agency_buf = ORPGMISC_crypt(agency_psswd);
   strcpy(agency_psswd_new, agency_buf);
   Lgcy_data_items[42].value = MISC_malloc( (size_t) 100 );
   strcpy(Lgcy_data_items[42].value, agency_psswd_new);

   Get_encrypted_passwd( passwd_data->roc, roc_psswd);
   roc_buf = ORPGMISC_crypt(roc_psswd);
   strcpy(roc_psswd_new, roc_buf);
   Lgcy_data_items[43].value = MISC_malloc( (size_t) 100 );
   strcpy(Lgcy_data_items[43].value, roc_psswd_new);

   Get_encrypted_passwd( passwd_data->urc, urc_psswd);
   urc_buf = ORPGMISC_crypt(urc_psswd);
   strcpy(urc_psswd_new, urc_buf);
   Lgcy_data_items[44].value = MISC_malloc( (size_t) 100 );
   strcpy(Lgcy_data_items[44].value, urc_psswd_new);

}

/**************************************************************************

    This function uses the CCZ data structure and the buffer
    containing the CCZ data (passed in) to assign the proper values
    in the Ccz_data_items array. 
    
**************************************************************************/

static void Parse_legacy_ccz_data (int fd, char *lgcy_buf, char *orda_buf,
   int len) {
 
   int ret;
   unsigned char* strp;

   /* encode and store lgcy ccz data */
   if ( (ret=DEAU_uuencode(lgcy_buf, sizeof(RPG_clutter_regions_msg_t), &strp))
      < 0 )
   {
      fprintf(stderr,
         "Parse_legacy_ccz_data: DEAU_uuencode(lgcy) returned error (%d)\n",
         ret);
      return;
   }
   
   Lgcy_data_items[45].value = (char *) MISC_malloc(ret);
   strcpy(Lgcy_data_items[45].value, strp);


   /* encode and store orda ccz data */
   if ( (ret=DEAU_uuencode(orda_buf, sizeof(ORPG_clutter_regions_msg_t), &strp))
      < 0 )
   {
      fprintf(stderr,
         "Parse_legacy_ccz_data: DEAU_uuencode(lgcy) returned error (%d)\n",
         ret);
      return;
   }

   Lgcy_data_items[46].value = (char *) MISC_malloc(ret);
   strcpy(Lgcy_data_items[46].value, strp);

   /* free pointer */
   if (ret > 0)
      free(strp);
}


