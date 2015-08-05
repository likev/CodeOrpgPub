/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/05/23 21:02:44 $
 * $Id: print_iprod.c,v 1.1 2007/05/23 21:02:44 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <print_iprod.h>

char *Prod_names[] = {
   "COMBATTR",
   "ETTAB",
   "VADTMHGT"};

/*******************************************************************************
* Description:
*    Print the main menu of options and store the user's selections in the 
*    input array.
*
* Inputs:
*    int     select_arr     The main menu option that the user selected.
*
* Outputs:
*    int     select_arr     The main menu option that the user selected.
*
* Returns:
*    PRINT_IPROD_SUCCESS or PRINT_IPROD_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int Get_main_menu_selects( unsigned short select_arr[PRINT_IPROD_NUM_PRODS])
{
   int ret_val = PRINT_IPROD_SUCCESS;
   int idx = 0; /* generic loop variable */
   int select_idx = 0; /* counter for user's menu selections */
   int err = 0;
   char response;
   int low_choice = 1;
   int high_choice = PRINT_IPROD_NUM_PRODS + 1;
   extern char *Prod_names[];

   if ((system ("clear")) >= 0)
   {
      fprintf (stderr, "**********************************************\n");
      fprintf (stderr, "*                                            *\n");
      fprintf (stderr, "*         Print Intermediate Product         *\n");
      fprintf (stderr, "*               User Interface               *\n");
      fprintf (stderr, "*                                            *\n");
      fprintf (stderr, "**********************************************\n");
   }

   fprintf (stderr, "\n\n");
   fprintf (stderr, "Select the IP data types to print:\n");
   for ( idx = 0; idx < PRINT_IPROD_NUM_PRODS; idx++ )
   {
      fprintf (stderr, "%d - %s\n", idx+1, Prod_names[idx]);
   }
   fprintf (stderr, "%d - Quit\n\n", PRINT_IPROD_EXIT_CODE);
   
   select_idx = 0;
   fprintf(stderr, "Enter choice: ");
   if (((err = scanf("%hu", &select_arr[select_idx])) != 1) ||
       (select_arr[select_idx] < low_choice) ||
       (select_arr[select_idx] > high_choice))
   {
      fprintf(stderr, "Invalid entry.\n");
      exit(0);
   }
   else
   {
      fprintf(stderr, "Do you want to choose another product? (y/n)");
      if ( (err = scanf("%s", &response)) != 1)
      {
         fprintf(stderr, "Problem interpreting selection.\n");
         exit(0);
      }
      else
      {
         while ( response == 'y' || response == 'Y' )
         {
            select_idx++;
            fprintf(stderr, "Enter another selection: ");
            if ( (err = scanf("%hu", &select_arr[select_idx])) != 1)
            {
               fprintf(stderr, "Invalid selection.\n");
               break;
            }
            fprintf(stderr, "Do you want to choose another product? (y/n)");
            if ( (err = scanf("%s", &response)) != 1)
            {
               fprintf(stderr, "Problem interpreting selection.\n");
               break;
            }
         }
      }
   }

   return (ret_val);

} /* end Get_main_menu_selects() */


/*******************************************************************************
* Description:
*    Get the product data types associated with the products whose indexes are
*    stored in the "p_idx_arr" array, and store the types in the "p_type_arr"
*    array.
*
* Inputs:
*    unsigned short    p_idx_arr[]    Array of product indexes corresponding to
*                                     the user's menu choices.
*    int               p_type_arr[]   Array of product data types corresponding
*                                     to the user's menu choices.
* Outputs:
*    int               p_type_arr[]   Array of product data types corresponding
*                                     to the user's menu choices.
*
* Returns:
*    PRINT_IPROD_SUCCESS or PRINT_IPROD_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int Get_prod_types( unsigned short p_idx_arr[PRINT_IPROD_NUM_PRODS], 
   int p_type_arr[PRINT_IPROD_NUM_PRODS])
{
   int idx = 0;
   extern char *Prod_names[];

   for ( idx = 0; idx < PRINT_IPROD_NUM_PRODS; idx++ )
   {
      if ( p_idx_arr[idx] == 0 )
      {
         continue;
      }
      else
      {
         p_type_arr[idx] = RPGC_get_id_from_name(Prod_names[p_idx_arr[idx] - 1]);
      }
   }

   return (PRINT_IPROD_SUCCESS);

} /* end Get_prod_types() */


/*******************************************************************************
* Description:
*    Determine type of input data received.  If type is requested for printing,
*    print the data to the screen.
*
* Inputs:
*    int     data_types[]    Data IDs of products requested for printing
*
* Outputs:
*    None
*
* Returns:
*    PRINT_IPROD_SUCCESS or PRINT_IPROD_FAILURE
*
* Globals:
*
* Notes:
*******************************************************************************/
int Process_input_data( int data_types[PRINT_IPROD_NUM_PRODS] )
{
   void *inbuf_ptr; /* Ptr to incoming data */
   int type = 0;    /* Data type received */
   int stat = 0;    /* Status of call to get inbuf */
   int ret_val = PRINT_IPROD_SUCCESS; /* This function's return value */
   int prod_idx = 0; /* Index for product loop */


   inbuf_ptr = RPGC_get_inbuf_any( &type, &stat );
   if ( stat == RPGC_NORMAL )
   {
      for ( prod_idx = 0; prod_idx < PRINT_IPROD_NUM_PRODS; prod_idx++ )
      {
         if ( type == data_types[prod_idx] )
         {
            Print_prod_data(type, inbuf_ptr);
         }
      }

   }
   else
   {
      LE_send_msg(GL_INFO,
         "Process_input_data: Error returned from RPGC_get_inbuf_any (%d)\n",
         stat);
      ret_val = PRINT_IPROD_FAILURE;
   }

   return (ret_val);

} /* end Process_input_data() */


/*******************************************************************************
* Description:
*    Determine the type of data and call the appropriate function to print
*    the product data.
*
* Inputs:
*    int     prod_type     Product data type.
*    void    *buf          Data buffer containing product data.
*
* Outputs:
*    None
*
* Returns:
*    None
*
* Globals:
*    None
*
* Notes:
*******************************************************************************/
void Print_prod_data( int prod_type, void *buf)
{
   switch ( prod_type )
   {
      case COMBATTR:
         Print_combattr_data(buf);
         break;

      case ETTAB:
         Print_ettab_data(buf);
         break;

      case VADTMHGT:
         Print_vadtmhgt_data(buf);
         break;

      default:
         break;
   }

} /* end Print_product_data() */


/*******************************************************************************
* Description:
*    Print the combined attributes data.
*
* Inputs:
*    void     *combatt_buf    Data buf containing combined attributes data.
*
* Outputs:
*    Prints the combined attributes data to the screen.
*
* Returns:
*    None
*
* Globals:
*
* Notes:
*******************************************************************************/
void Print_combattr_data( void *combatt_buf )
{
   combattr_t *combatt_ptr = (combattr_t *)combatt_buf;
   int num_storms = 0; /* number of detected storm cells */
   int storm_idx = 0; /* storm cell loop index */
   int feat_idx = 0; /* storm cell feature index */
   int attr_idx = 0; /* storm cell attribute index */
   int pos_idx = 0; /* storm cell fcst position index */
   int storm_id_int; /* integer representation of storm ID */
   char storm_id_str[CAT_MXSTMS][3]; /* Storm IDs for all storms*/

   /* Used for printing COMBATTR COMB_ATT data */
   char *Combattr_att_names[] = {
      "Azi",
      "Slant Rng",
      "Height",
      "Elev",
      "Base Hgt",
      "Max Refl",
      "Hgt Max Refl",
      "Elev Max Refl",
      "Cell VIL",
      "Storm Top Hgt",
      "Fcst Dir",
      "Fcst Spd",
      "TVS Base Azi",
      "TVS Base Rng",
      "TVS Base Elev",
      "MDA Base Azi",
      "MDA Base Rng",
      "MDA Base Elev",
      "MDA Strength Rank"};

   /* Used for printing COMBATTR TVS data */
   char *Combattr_tvs_names[] = {
      "TVS Base Azimuth",
      "TVS Base Range",
      "TVS Base Elevation"};

   /* Used for printing COMBATTR MDA data */
   char *Combattr_mda_names[] = {
      "MDA Base Azimuth",
      "MDA Base Range",
      "MDA Strength Rank",
      "MDA Base Elevation"};

   /*** print CAT header ***/
   fprintf(stderr,
      "===================== Combined Attributes Data =====================\n");

   /*** print the number of storms ***/
   num_storms = combatt_ptr->cat_num_storms;
   fprintf(stderr, "\nCAT Num Storms:\n");
   fprintf(stderr, "%d\n", num_storms);

   /*** store the storm IDs for future printing */
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      storm_id_int = combatt_ptr->cat_feat[storm_idx][CAT_SID];
      storm_id_str[storm_idx][0] = (char )*((char *)(&storm_id_int) + 0);
      storm_id_str[storm_idx][1] = (char )*((char *)(&storm_id_int) + 1);
      storm_id_str[storm_idx][2] = '\0';
   }

   /*** print the storm cell features ***/
   fprintf(stderr, "CAT Storm Features:\n");
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      fprintf(stderr, "  Storm ID: %s\n", storm_id_str[storm_idx]);
      fprintf(stderr, "        TVS Flag: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_TVS]);
      fprintf(stderr, "             POH: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_POH]);
      fprintf(stderr, "            POSH: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_POSH]);
      fprintf(stderr, "            MEHS: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_MEHS]);
      fprintf(stderr, "      Hail Pot'l: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_HAIL]);
      fprintf(stderr, "       Cell Type: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_TYPE]);
      fprintf(stderr, "             MDA: %d\n",
         combatt_ptr->cat_feat[storm_idx][CAT_MDA]);
   }

   /* print the storm cell attributes */
   fprintf(stderr, "CAT Storm Attributes:\n");
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      fprintf(stderr, "  Storm ID: %s\n", storm_id_str[storm_idx]);
      for ( attr_idx = 0; attr_idx < CAT_DAT; attr_idx++ )
      {
         fprintf(stderr, "    %s: %f\n",
            Combattr_att_names[attr_idx],
            combatt_ptr->comb_att[storm_idx][attr_idx]);
      }
   }

   /* print the number of forecast positions */
   fprintf(stderr, "CAT Num Fcst Positions:\n");
   fprintf(stderr, "%d\n", combatt_ptr->num_fposits);

   /* print the forecast positions */
   fprintf(stderr, "CAT Fcst Positions:\n");
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      fprintf(stderr, "  Storm ID: %s\n", storm_id_str[storm_idx]);
      for ( pos_idx = 0; pos_idx < combatt_ptr->num_fposits; pos_idx++ )
      {
         fprintf(stderr, "    Fcst Pos %d, X: %f, Y: %f\n", pos_idx,
            combatt_ptr->forcst_posits[storm_idx][pos_idx][CAT_FX],
            combatt_ptr->forcst_posits[storm_idx][pos_idx][CAT_FY]);
      }
   }

   /* print the number of TVS detections this volume */
   fprintf(stderr, "CAT Num TVS:\n");
   fprintf(stderr, "%d\n", combatt_ptr->cat_num_rcm[RCM_TVS]);

   /* print the number of MDA detections this volume */
   fprintf(stderr, "CAT Num MDA:\n");
   fprintf(stderr, "%d\n", combatt_ptr->cat_num_rcm[RCM_MDA]);

   /* print the TVS features */
   fprintf(stderr, "CAT TVS Features:\n");
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      fprintf(stderr, "  Storm ID: %s\n", storm_id_str[storm_idx]);
      for ( feat_idx = 0; feat_idx < CAT_NTVS; feat_idx++ )
      {
         fprintf(stderr, "    %s: %f\n",
            Combattr_tvs_names[feat_idx],
            combatt_ptr->cat_tvst[storm_idx][feat_idx]);
      }
   }

   /* print the MDA features */
   fprintf(stderr, "CAT MDA Features:\n");
   for ( storm_idx = 0; storm_idx < num_storms; storm_idx++ )
   {
      fprintf(stderr, "  Storm ID: %s\n", storm_id_str[storm_idx]);
      for ( feat_idx = 0; feat_idx < CAT_NMDA; feat_idx++ )
      {
         fprintf(stderr, "    %s: %f\n", 
            Combattr_mda_names[feat_idx], 
            combatt_ptr->cat_mdat[storm_idx][feat_idx]);
      }
   }

} /* end Print_combattr_data() */


/*******************************************************************************
* Description:
*    Print the echo tops table data.
*
* Inputs:
*    void     *ettab_buf    Data buf containing echo tops data.
*
* Outputs:
*    Prints the echo tops data to the screen.
*
* Returns:
*    None
*
* Globals:
*
* Notes:
*******************************************************************************/
void Print_ettab_data( void *ettab_buf )
{
   Echo_top_params_t *ettab_ptr = (Echo_top_params_t *)ettab_buf;
   int word_idx = 0;
   int row_idx = 0;
   int col_idx = 0;

   /* print ETTAB header */
   fprintf(stderr,
      "========================= Echo Tops Data ===========================\n");

   /* print the number of columns */
   fprintf(stderr, "ETTAB Num Cols:\n");
   fprintf(stderr, "%d\n", ettab_ptr->ncols);

   /* print the number of rows */
   fprintf(stderr, "ETTAB Num Rows:\n");
   fprintf(stderr, "%d\n", ettab_ptr->nrows);

   /* print the max analysis slant range (km) */
   fprintf(stderr, "ETTAB Max Analysis Slant Range (km):\n");
   fprintf(stderr, "%d\n", ettab_ptr->max_rng);

   /* print the Min Reflectivity Threshold (dBZ*10) */
   fprintf(stderr, "ETTAB Min Reflectivity Threshold (dBZ*10):\n");
   fprintf(stderr, "%d\n", ettab_ptr->min_refl);

   /* print the Max Echo Top Value (kft) */
   fprintf(stderr, "ETTAB Max Echo Top Value (kft):\n");
   fprintf(stderr, "%d\n", ettab_ptr->max_ht);

   /* print the Max Echo Top Column Position */
   fprintf(stderr, "ETTAB Max Echo Top Column Position:\n");
   fprintf(stderr, "%d\n", ettab_ptr->max_col_posn);

   /* print the Max Echo Top Row Position */
   fprintf(stderr, "ETTAB Max Echo Top Row Position:\n");
   fprintf(stderr, "%d\n", ettab_ptr->max_row_posn);

   /* print the overlay bitmap of echo tops found at max elev angle */
   fprintf(stderr, "ETTAB Overlay:\n");
   for ( word_idx = 0; word_idx < NUMW; word_idx++ )
   {
      fprintf(stderr, "Word: %d  Value: %0x\n", word_idx,
         ettab_ptr->overlay[word_idx]);
   }

   /* print the echo top values */
   fprintf(stderr, "ETTAB Echo Top Values:\n");
   for ( row_idx = 0; row_idx < NETROW; row_idx++ )
   {
      for ( col_idx = 0; col_idx < NETCOL; col_idx++ )
      {
         fprintf(stderr, "Row: %d, Col: %d, Value: %d\n", row_idx, col_idx,
            ettab_ptr->etval[row_idx][col_idx]);
      }
   } 
} /* end Print_ettab_data() */


/*******************************************************************************
* Description:
*    Print the VAD Time Height data.
*
* Inputs:
*    void     *vadtmhgt_buf    Data buf containing VAD time height data.
*
* Outputs:
*    Prints the VAD time height data to the screen.
*
* Returns:
*    None
*
* Globals:
*
* Notes:
*******************************************************************************/
void Print_vadtmhgt_data( void *vadtmhgt_buf )
{
   Vad_params_t *vadtmhgt_ptr = (Vad_params_t *)vadtmhgt_buf;
   int vol_idx = 0; /* VAD volume index */
   int ht_idx = 0; /* VAD height index */
   int az_idx = 0; /* VAD azimuth index */
   int parm_idx = 0; /* VAD parameter index */
   int ar_idx = 0; /* VAD ambig range index */

   /* print VADTMHGT header */
   fprintf(stderr,
      "========================= VAD Time Height Data =====================\n");

   /* print the vol date */
   fprintf(stderr, "VADTMHGT Vol Date:\n");
   fprintf(stderr, "%d\n", vadtmhgt_ptr->vol_date);

   /* print the vol num */
   fprintf(stderr, "VADTMHGT Vol Num:\n");
   fprintf(stderr, "%d\n", vadtmhgt_ptr->vol_num);

   /* print the vad times */
   fprintf(stderr, "VADTMHGT Times:\n");
   for ( vol_idx = 0; vol_idx < MAX_VAD_VOLS; vol_idx++ )
   {
      fprintf(stderr, "Vol: %d, Value: %d\n", vol_idx,
         vadtmhgt_ptr->times[vol_idx]);
   }

   /* print the vad missing data val */
   fprintf(stderr, "VADTMHGT Missing Data Val:\n");
   fprintf(stderr, "%f\n", vadtmhgt_ptr->missing_data_val);

   /* print the vad algorithm data */
   fprintf(stderr, "VADTMHGT VAD Alg Data:\n");
   for ( ht_idx = 0; ht_idx < MAX_VAD_HTS; ht_idx++ )
   {
      fprintf(stderr, "Height Index: %d\n", ht_idx);

      /* Height Parm Loop */
      for ( parm_idx = 0; parm_idx < VAD_HT_PARAMS; parm_idx++ )
      {
         fprintf(stderr, "Parm Index: %d, Value: %f\n", parm_idx,
            vadtmhgt_ptr->vad_data_hts[ht_idx][parm_idx]);
      }

      /* Azimuth Parms */
      for ( az_idx = 0; az_idx < VAD_NAZIMUTHS; az_idx++ )
      {
         fprintf(stderr,
            "Az Index: %d, Parm 1: %f, Parm 2: %f, Parm 3: %f\n", az_idx,
            vadtmhgt_ptr->vad_data_azm[ht_idx][az_idx][0],
            vadtmhgt_ptr->vad_data_azm[ht_idx][az_idx][1],
            vadtmhgt_ptr->vad_data_azm[ht_idx][az_idx][2]);
      }

      /* Ambig Range Parms */
      for ( ar_idx = 0; ar_idx < VAD_AMBIG_RANGES; ar_idx++ )
      {
         fprintf(stderr,
            "AR Index: %d, Parm 1: %f, Parm 2: %f\n", ar_idx,
            vadtmhgt_ptr->vad_data_ar[ht_idx][ar_idx][0],
            vadtmhgt_ptr->vad_data_ar[ht_idx][ar_idx][1]);
      }
   }
} /* end Print_vadtmhgt_data() */


