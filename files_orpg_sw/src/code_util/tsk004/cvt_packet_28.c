/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 19:48:43 $
 * $Id: cvt_packet_28.c,v 1.7 2012/09/13 19:48:43 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
 
/* cvt_packet_28.c */

#include "cvt_packet_28.h"
 

/* CVT 4.4 */
static char *Add_space = "";         /* for product print out */
int params_found;

/* internal diagnostics */
    char *truefalse[]={"FALSE","TRUE"}; 
    char *format[]={"NONE","RLE","BSCAN"}; 
    char *scale[]={"NOSCALE","REFL","VEL1","VEL2","SW","FDECODE","PDECODE"}; 







/* CVT 4.4 - added the flag parameter */
void packet_28(char *buffer,int *offset,int *flag) {
  /* display information contained within packet 28 */

  short align_hw;
  int ser_len, ret;
  char *serial_data;  /* offset to the serialized data in the packet */
  RPGP_product_t *prod_de;  /* deserialized product data */
  
  int TEST=TRUE;

  if(TEST) {
     printf("Offset value = %d \n",*offset);
     }

  align_hw = read_half(buffer,offset);
  ser_len = read_word(buffer,offset);

fprintf(stderr,"Packet 28 offset of serial data is %d\n",*offset);  

/* use a local variable rather than offset ?? */
  serial_data = buffer+*offset;
     

  printf("Contents of Align HW = %hd\n",align_hw);
  printf("Length of Serialized Data = %d bytes\n",ser_len);


  ret = cvt_RPGP_product_deserialize(serial_data, ser_len, (void *)&prod_de); 
      
  if(ret < 0) {
       fprintf(stderr, "ERROR - Unable to deserialize the data in packet 28\n");
       exit (1); 
  }
  



 /* CVT 4.4 - passed flag array to new print function */
/*  Print_prod(prod_de); */
   cvt_Print_prod((void*)prod_de, flag); 


  ret = cvt_RPGP_product_free((void *)prod_de);


  *offset +=ser_len;
/* CVT 4.4 - commented out */
/*  fprintf(stderr, "Packet 28 ending offset is %d\n",*offset);  */

  printf("\n");
  printf("Packet 28 Complete\n");
  
} /* end packet_28() */





/*************************************************************/
/* CVT 4.4 - new print prod functions passing flag parameter */
/*         - only radial component has scale offset decoding */
/* The following functions are modified versions of print    */
/* functions in xdr_test.c  (cpc101/lib003)                  */
/* (last verified with Build 11 rel 1.2)                     */
/*************************************************************/
/* LIMITATIONS OF THE PRINT FUNCTIONS (as of 12/2008)
 * 1. The function Print_binaty_data() does not support all data types.
 *    Currently only "short", "ushort", "ubyte" and "float" are supported.
 *    The functiion cvt_Print_binary_data support all types.
 * 2. The function Get_data_type() has a limitation in that a token
 *    can be a maximum of 16 characters. So passing in a larger size
 *    does no good.
 */


/* CVT 4.4 - PRINT GENERIC PRODUCT */
/************************************************************/
void cvt_Print_prod (void *prodp, int *flag) {
  
    RPGP_product_t *prod;
    
  /* added for CVT to decode times */
  char *date_string[30];
  char *time_string[30];
  
  /* added for CVT - must agree with cvt.h */
  char *flag8_string[]={"LIST_NONE","Listing All","Listing Area",
                        "Listing Radial","Listing Text","Listing Table",
                        "Listing Grid","Listing Event"};
  char *flag9_string[]={"PRINT_NONE","Printing All","Printing Area",
                        "Printing Radial","Printing Text","Printing Table",
                        "Printing Grid","Printing Event"};
  /* added for CVT - must agree with cvt_orpg_product.h */
  char *prod_type_str[]={"NOT_USED","Volume","Elevation","Time","On-Demand",
                        "On-Request","Radial","External"};
  char *op_mode_str[]={"Maintenance","Clear Air","Severe Wx"};

    prod = (RPGP_product_t *)prodp;
    printf ("\n");

    /* CVT 4.4 - placed desc on the next line */
    printf ("product id %d, name: %s, \ndesc: %s.\n", 
            prod->product_id, prod->name, prod->description);

    /* for CVT, changed 'time' to 'prod gen time', added  */
    /*          added type string                                      */
    printf ("    type: %d (%s), prod gen time: %d \n",prod->type, 
                      prod_type_str[prod->type], prod->gen_time);
    /* for CVT - DECODE PROD GEN TIME */
    if(prod->gen_time != 0) {
        /* CVT 4.4.1 - only print one of the decoded values */
        if(prod->gen_time <= 86400) {
            *time_string = msecs_to_string (prod->gen_time*1000);
            printf (
               "NOTE: Class 1 ICD states all times are 'Unix Time' but some\n"
               "      products have seconds after midnight for Generation time.\n");
            printf ("    Decoded Gen time is %s (sec after midnight)\n",
                                                                   *time_string);
        } else {
            decode_prod_time(prod->gen_time, date_string, time_string);
            printf ("    Decoded Gen time is %s %s (Unix Time)\n", 
                                                          *date_string, *time_string);
        }
        
    } /* end if gen time != 0 */
    
    if (prod->type == RPGP_EXTERNAL) { /* THIS IS AN EXTERNAL PRODUCT */
        RPGP_ext_data_t *eprod = (RPGP_ext_data_t *)prodp;
        printf ("    compress type %d, size %d\n", 
                eprod->compress_type, eprod->size_decompressed);

        printf("\nThis Product has %d parameters\n", prod->numof_prod_params);
        if(flag[12]==FALSE)
/*        Print_params (eprod->numof_prod_params, eprod->prod_params); */
            cvt_Print_params (eprod->numof_prod_params, eprod->prod_params);
        else
            printf("     Printing of Product Parameters not Selected\n");
            
        printf("\nThis Product has %d components\n", prod->numof_components);
        if(flag[10] != PRINT_NONE || flag[9] != LIST_NONE || flag[11] != -1) {
            if(flag[9] != LIST_NONE)
                printf("     %s Components\n", flag8_string[flag[9]] );
            if( flag[9] == LIST_NONE && flag[10] != PRINT_NONE )
                printf("     %s Components\n", flag9_string[flag[10]] );
/*        Print_components (eprod->numof_components, (char **)eprod->components); */
            cvt_Print_components ( prod->numof_components, 
                               (char **)prod->components, flag );
        } else {
            printf("     Printing / Listing of Product Components not Selected\n");
        }
    
    }
    else {  /* THIS IS A WSR-88D PRODUCT */
        /* for CVT changed lat/lon from 8.4f to 7.3f and height from 8.4f to 5.1f */
        printf ("\n    Radar name: %s, lat %7.3f, lon %7.3f, height %5.1f\n", 
                prod->radar_name, prod->radar_lat, 
                prod->radar_lon, prod->radar_height);

        /* for CVT - added '(Unix Time)'  and broke into two lines of output */
        printf ("    Vol time %d (Unix Time), elev time %d (Unix Time), \n"
                "    vol number %d, elev number %d\n", 
                prod->volume_time, prod->elevation_time, 
                prod->volume_number, prod->elevation_number);
        /* added for CVT - decoded volume date time */
        if(prod->volume_time != 0) {
            decode_prod_time(prod->volume_time, date_string, time_string);
            printf ("    Decoded Vol time is %s %s\n", *date_string, *time_string);
        }
        /* added for CVT - decoded elevation date time */
        if(prod->elevation_time != 0) {
            decode_prod_time(prod->elevation_time, date_string, 
                                                                       time_string);
            printf ("    Decoded Elev time is %s %s\n", *date_string, *time_string);
        }
        /* for CVT changed elev from 8.4f to 5.1f and 'elev' to 'target elev' */
        /*         added op mode string                                       */
        printf ("\n    VCP %d, Op mode %d (%s), target elev %5.1f\n", prod->vcp, 
                prod->operation_mode, op_mode_str[prod->operation_mode], 
                prod->elevation_angle);

        printf ("    compress type %d, size %d\n", 
                prod->compress_type, prod->size_decompressed);

        printf("\nThis Product has %d parameters\n", prod->numof_prod_params);
        if(flag[12]==FALSE) {
/*        Print_params (prod->numof_prod_params, prod->prod_params); */
            cvt_Print_params (prod->numof_prod_params, prod->prod_params);
        } else {
            printf("     Printing of Product Parameters not Selected\n");
        }
        
        printf("\nThis Product has %d components\n", prod->numof_components);
        if(flag[10] != PRINT_NONE || flag[9] != LIST_NONE || flag[11] != -1) {
            if(flag[9] != LIST_NONE)
                printf("     %s Components\n", flag8_string[flag[9]] );
            if( flag[9] == LIST_NONE && flag[10] != PRINT_NONE )
                printf("     %s Components\n", flag9_string[flag[10]] );
/*        Print_components (prod->numof_components, (char **)prod->components); */
            cvt_Print_components ( prod->numof_components, 
                                   (char **)prod->components, flag );
        } else {
            printf("     Printing / Listing of Product Components not Selected\n");
        }
        
    } /* end WSR-88D PRODUCT */
    
    printf ("\n"); 
  
  
} /* end cvt_Print_prod */






/* CVT 4.4 - used by cvt_Print_prod */
/************************************************************/
void decode_prod_time(unsigned int in_time, char **date_str, 
                                                     char **time_str)
{
    
  unsigned short date_i;
  unsigned int time_i;
  
/* NOTE: in_time = (date_i - 1)*86400 + time_i */

    time_i = in_time % 86400;
    date_i = (in_time - time_i)/86400 + 1;
/* TEST */
/*fprintf(stderr,"TEST input time is %u\n", in_time);                             */
/*fprintf(stderr,"TEST integer date is %d, integer time is %u\n", date_i, time_i);*/

    *date_str = date_to_string (date_i);
    *time_str = msecs_to_string (time_i*1000);
    
} /* end decode_gen_prod_time */






/* CVT 4.4 - used by cvt_Print_prod and all print component functions */
/************************************************************/
void cvt_Print_params (int n_params, RPGP_parameter_t *params) {
    int i;

    int k, offset, num_char, begin_length;
    char *attr_buf;
    char param_beginning[60];
    
/*    printf ("%s        # %d params:\n", Add_space, n_params); */
    
    for (i = 0; i < n_params; i++) {
        
/*        printf ("%s            param[%d]: id: %s, attrs: %s\n", */
/*            Add_space, i, params[i].id, params[i].attrs);       */
        
        /* new for CVT -- created a line wrapped output ------------ ---- */
        /* for CVT - reduced leading spaces and relpaced %d with %2d */
        attr_buf = (char *) params[i].attrs;
        
        sprintf(param_beginning, "%s    param[%2d]: id: %s, attrs: ",
                                                       Add_space, i, params[i].id);
        begin_length = strlen(param_beginning);
        printf("%s\n               ", param_beginning); /* note 15 spaces */
        
        offset = 0;
        num_char = 0;
        for(k=0; k < strlen(params[i].attrs); k++) {
            unsigned char c;

            num_char++;
            c = read_byte(attr_buf, &offset);
            printf( "%c", c ); 
            
            if( (num_char > (60)) && (c == ';' || c == ',') ) {
                if( (k+1) < strlen(params[i].attrs) ) {
                    printf("\n               "); /* note 15 spaces */
                    num_char = 0;
                }
            } /* end if detected need to wrap */
            
        } /* end for k < length */
        
        printf("\n");
        /* end new for CVT ---------------------------------------------- */
        
    } /* end for i < n_params */

} /* end cvt_Print_params */






/* CVT 4.4 - PRINT SELECTED COMPONENTS */
/************************************************************/
void cvt_Print_components (int n_comps, char **comps, int *flag) {
  
    RPGP_area_t **gencomps;
    int i;

/*    printf ("%s    number of components %d\n", Add_space, n_comps); */
    gencomps = (RPGP_area_t **)comps;

        
    for (i = 0; i < n_comps; i++) {

        switch (gencomps[i]->comp_type) {
    
            case RPGP_AREA_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_AREA ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_AREA || flag[11]==i )
/*                    Print_area ((RPGP_area_t *)comps[i]); */
                    cvt_Print_area ((RPGP_area_t *)comps[i], i, flag);
                break;
    
            case RPGP_TEXT_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_TEXT ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_TEXT || flag[11]==i )
/*                    Print_text ((RPGP_text_t *)comps[i]); */
                    cvt_Print_text ((RPGP_text_t *)comps[i], i, flag);
                break;
    
            case RPGP_EVENT_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_EVENT ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_EVENT || flag[11]==i )
/*                    Print_event ((RPGP_event_t *)comps[i]); */
                    cvt_Print_event ((RPGP_event_t *)comps[i], i, flag);
                break;
    
            case RPGP_GRID_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_GRID ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_GRID || flag[11]==i )
/*                    Print_grid ((RPGP_grid_t *)comps[i]); */
                    cvt_Print_grid ((RPGP_grid_t *)comps[i], i, flag);
                break;
    
            case RPGP_RADIAL_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_RAD ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_RAD || flag[11]==i )
/*                    Print_radial ((RPGP_radial_t *)comps[i]); */
                    cvt_Print_radial ((RPGP_radial_t *)comps[i], i, flag);
                break;
    
            case RPGP_TABLE_COMP:
                if( flag[9]==LIST_ALL  || flag[9]==LIST_TABLE ||
                    flag[10]==PRINT_ALL || flag[10]==PRINT_TABLE || flag[11]==i )
/*                    Print_table ((RPGP_table_t *)comps[i]); */
                    cvt_Print_table ((RPGP_table_t *)comps[i], i, flag);
                break;
    
            default:
            break;

        } /* end switch */
        
    } /* end for */
  
} /* end cvt_Print_components */




/* CVT 4.4 - PRINT RADIAL COMPONENT */
/************************************************************/
void cvt_Print_radial (RPGP_radial_t *radial, int index, int *flag) {
  
    int i;

   /* CVT Added */
   int start=0,end=0;

    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_RAD ) {
        printf ("(Index %2d) RADIAL Component: (%d)\n", index, radial->comp_type);
        
        return;
    } 


    printf ("\n(Index %2d) RADIAL Component (%d): description: %s\n", 
            index, radial->comp_type, radial->description);
    /* for CVT changed %f to %5.1f */
    printf ("%s    bin_size %5.1f, first_range %5.1f, %d radials\n", 
             Add_space, radial->bin_size, radial->first_range, 
                        radial->numof_radials);

    printf("\nThis Component has %d parameters\n", radial->numof_comp_params);
    if(flag[13]==FALSE)
/*    Print_params (radial->numof_comp_params, radial->comp_params); */
        cvt_Print_params (radial->numof_comp_params, radial->comp_params);
    else
        printf("     Printing of Component Parameters not Selected\n");

    printf("\n"); /* added for CVT */

    if (radial->numof_radials <= 0)
        return;

   /* CVT 4.4 - ************ Begin Added for CVT ***************************/
   /*add test for format flag etc. */
   if(flag[7]==NOSCALE) {
      fprintf(stderr,"  Data Values left in Raw Value\n\n");
   }
   else if(flag[7]==FDECODE) {
      fprintf(stderr,"  Data Values Decoded using File Parameters (fdecode)\n\n");
   }
   else if(flag[7]==PDECODE) {
      fprintf(stderr,"  Data Values Decoded using Product Parameters (pdecode)\n\n");
   }
   
  /*intneral disgnostics */
  /*printf("Diagnostic Information:\n");                   */
  /*start=flag[1];                                         */
  /*printf("Start Field:\t\t\t\t%hd\n",start);             */
  /*end=flag[2];                                           */
  /*printf("End Field:\t\t\t\t%hd\n",end);                 */
  /*printf("All Flag:\t\t\t\t%s\n",truefalse[flag[3]]);    */
  /*printf("Degree Flag:\t\t\t\t%s\n",truefalse[flag[4]]); */
  /*printf("Output Format:\t\t\t\t%s\n",format[flag[5]]);  */
  /*printf("Scale Flag:\t\t\t\t%s\n",scale[flag[7]]);      */

   start = flag[1];
   end = flag[2];
   
   if(flag[1]==0 && flag[2]==0) /* assume all radials desired */
      flag[3] = TRUE;
   
   /* check for 'all' flag & reset start and end flags */
   if(flag[3]==TRUE) {
      if(flag[4]==TRUE) { /* use degree values */
         start = 0;
         end = 359;
      } else { /* use all values */
         start = 1;
         end = radial->numof_radials;
      }
   }

   /* begin quality control input values */
   if(start > radial->numof_radials) {
      printf("Range Error: The Radial Start Value of %d Exceeds "
            "the Max Number of Radials (%d)\n", start, radial->numof_radials);
     return;
   }

   if(end < 0) {
      printf("Range Error: The Radial End Value of %d is out of bounds\n", end);
      return;
   }

   if(flag[4]==TRUE && (start > 359 || start < 0)) {
      printf("Range Error: The Radial Start Value of %d must be within "
             " 0-359 degrees\n", start);
      return;
   }

   if(flag[4]==TRUE && (end > 359 || start < 0)) {
      fprintf(stderr,"Range Error: The Radial End Value of %d must be within "
                     "0-359 degrees\n", end);
      return;
   }
   /* end quality control input values */
   
   if(flag[7]==PDECODE || flag[7]==FDECODE) {
      params_found = get_decode_params(flag[7], md.ptr_to_product, &s_o_params);
      if(params_found == FALSE)
        fprintf(stderr,
                "ERROR in obtaining decode parameters, raw levels displayed\n\n");
   }
   
   /* CVT 4.4 - ************ End Added for CVT ***************************/


    for (i = 0; i < radial->numof_radials; i++) {
 
/*        printf ("    RADIAL %d\n", i); */
/*        Print_a_radial (radial->radials + i); */

        cvt_Print_a_radial (radial->radials + i, flag, i+1, start, end);

    } /* end for num_radials */
    
    
    
    
   /* Added for CVT */
   if(flag[7]==PDECODE || flag[7]==FDECODE) {
      if(params_found == FALSE)
        fprintf(stderr,
                "ERROR in obtaining decode parameters, raw levels displayed\n\n");
   }
    
  
} /* end cvt_Print_radial */




/* CVT 4.4 - Used by cvt_Print_radial */
/************************************************************/
void cvt_Print_a_radial (RPGP_radial_data_t *rad, int *flag, 
                                              int rad_num, int start, int end) {
  
  
/* CVT 4.4 - ************ Begin Added for CVT ***************************/
short requested_radial = FALSE;
int trunc_az;
  
  
  trunc_az = (int)rad->azimuth;
  
  if(flag[4]==FALSE) { /* process actual radial values */
    
    if(end==0) {  /* only one radial requested */
      if(rad_num == start)
        requested_radial = TRUE;
    
    } else {     /* multiple radial requested */
      if(start > end) { /* range crosses max val, e.g. 355 - 5 */
        if(rad_num <= start && rad_num >= end)
          requested_radial = TRUE;
      }else {
        if(rad_num >= start && rad_num <= end)
          requested_radial = TRUE;
      }
          
    } /* else multiple radials */
    
  } /* end if using radial number */
  
  
  if(flag[4]==TRUE) { /* process radial azimuth */
    
    if(end==0) {  /* only one radial requested */
      if(trunc_az == start)
        requested_radial = TRUE;
    
    } else {     /* multiple radial requested */
      if(start > end) { /* range crosses max val, e.g. 355 - 5 */
        if(trunc_az <= start && trunc_az >= end)
          requested_radial = TRUE;
      } else {
        if(trunc_az >= start && trunc_az <= end)
          requested_radial = TRUE;
      }

    } /* else multiple radials */
    
  } /* end if using radial azimuth */

  /* CVT 4.4 - ************ End Added for CVT ***************************/
  
  
  /* conditional added for CVT */
  if(requested_radial == TRUE) {
    
    /* removed Add_space and added rad_num for CVT */
    printf ("Radial %d   begin azimuth %5.1f, width %4.1f, "
            "elevation %4.1f, n_bins %d\n", 
            rad_num, rad->azimuth, rad->width, rad->elevation, rad->n_bins);
            
  /*intneral disgnostics */
  /*printf("Diagnostic Information:\n");                   */
  /*printf("Start Field:\t\t\t\t%hd\n",start);             */
  /*printf("End Field:\t\t\t\t%hd\n",end);                 */
  /*printf("All Flag:\t\t\t\t%s\n",truefalse[flag[3]]);    */
  /*printf("Degree Flag:\t\t\t\t%s\n",truefalse[flag[4]]); */
  /*printf("Output Format:\t\t\t\t%s\n",format[flag[5]]);  */
  /*printf("Scale Flag:\t\t\t\t%s\n",scale[flag[7]]);      */
  
  
    /*    Print_binaty_data (&(rad->bins), rad->n_bins); */
    cvt_Print_binary_data (&(rad->bins), rad->n_bins, flag);
  
  } /* end requested_radial */
  
  
} /* end cvt_Print_a_radial */




/* CVT 4.4 used by cvt_Print_a_radial and future cvt_Print_grid */
/************************************************************/
void cvt_Print_binary_data (RPGP_data_t *data, int cnt, int *flag) {
  
    char type[128];
    
    /* added for CVT */
    int last_in_line = 9; /* 10 values per line */
    int k = 0;
    char result_str[15];

    printf ("data attributes: %s\n", data->attrs);
/*    if (Get_data_type (data->attrs, type, 128) > 0) { */
    if (cvt_Get_data_type (data->attrs, type, 128) > 0) {
        int i;
        
        if (strcmp (type, "short") == 0) {
            short *spt;
            printf ("Data:\n");
            spt = (short *)data->data;
            
            for (i = 0; i < cnt; i++) {
            /* changed %d to %5hd for CVT */
            printf (" %5hd", spt[i]);
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
        
        
        }
        else if (strcmp (type, "ubyte") == 0) {
            unsigned char *cpt;
            printf ("Data:\n");
            cpt = (unsigned char *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* added decode option for CVT */
                if(flag[7]==FDECODE || flag[7]==PDECODE) {
                    if(params_found == TRUE) {
                        decode_level((unsigned int)cpt[i], result_str, 
                                      &s_o_params, flag[8]);
                        printf("%s ", result_str);
                    }
                } else {
                        /* changed %d to %3hu for CVT */
                        printf (" %3hu   ", cpt[i]);
                } /* end if FDECODE or PDECODE */
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
            
            
        }
        else if (strcmp (type, "ushort") == 0) {
            unsigned short *spt;
            printf ("Data:\n");
            spt = (unsigned short *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* added decode option for CVT */
                if(flag[7]==FDECODE || flag[7]==PDECODE) {
                    if(params_found == TRUE) {
                        decode_level((unsigned int)spt[i], result_str, 
                                      &s_o_params, flag[8]);
                        printf("%s ", result_str);
                    }
                } else {
                    /* changed %d to %5hu for CVT */
                    printf (" %5hu", spt[i]);
                } /* end if FDECODE or PDECODE */
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
    /*  CVT - BEGIN ADDED FOR CVT HERE ---------------- */
        
        }
        else if (strcmp (type, "byte") == 0) {
            char *cpt;
            printf ("Data:\n");
            cpt = (char *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* changed %d to %3hd for CVT */
                printf (" %3hd   ", cpt[i]);
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
        
        
        }
        else if (strcmp (type, "int") == 0) {
            int *ipt;
            printf ("Data:\n");
            ipt = (int *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* changed %d to %10d for CVT */
                printf (" %10d", ipt[i]);
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
        
        
        }
        else if (strcmp (type, "uint") == 0) {
            unsigned int *ipt;
            printf ("Data:\n");
            ipt = (unsigned int *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* added decode option for CVT */
                if(flag[7]==FDECODE || flag[7]==PDECODE) {
                    if(params_found == TRUE) {
                        decode_level((unsigned int)ipt[i], result_str, 
                                      &s_o_params, flag[8]);
                        printf("%s ", result_str);
                    }
                } else {
                        /* changed %d to %10u for CVT */
                        printf (" %10u", ipt[i]);
                } /* end if FDECODE or PDECODE */
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
        
        
        }
        else if (strcmp (type, "double") == 0) {
            double *fpt;
            printf ("Data:\n");
            fpt = (double *)data->data;
            
            for (i = 0; i < cnt; i++) {
                printf (" %f", fpt[i]);
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            
            printf ("\n");
    /*  - CVT - END ADDED FOR CVT -------------------- */
        
        }
        else if (strcmp (type, "float") == 0) {
            float *fpt;
            printf ("Data:\n");
            fpt = (float *)data->data;
            
            for (i = 0; i < cnt; i++) {
                /* changed %f to %8.4f for CVT */
                printf (" %8.4f", fpt[i]);
                /* added for CVT */
                if(k==last_in_line) {
                    printf("\n");
                    k = 0;
                } else
                    k++;
            } /* end for */
            printf ("\n");
        
        
        }
        else
    /*        printf ("Print_grid: type (%s) not implemented\n", type); */
            printf ("cvt_Print_binary_data: type (%s) not implemented\n", type);
       
       
    }
    else
    /* printf ("Print_grid: type not found\n"); */
    printf ("cvt_Print_binary_data: 'type' attribute not found\n");
  
  
} /* end cvt_Print_binary_data */






/* CVT 4.4 - PRINT AREA COMPONENT*/
/************************************************************/
void cvt_Print_area (RPGP_area_t *area, int index, int *flag) {

/* local variables added for CVT */
  int area_type = RPGP_AREA_TYPE (area->area_type);
  int loc_type = RPGP_LOCATION_TYPE (area->area_type);
  char area_type_str[30];
  char loc_type_str[30];

    /* added for CVT */
    if(area_type==RPGP_AT_POINT)
        strcpy(area_type_str, "Geographical Point");
    else if(area_type==RPGP_AT_AREA)
        strcpy(area_type_str, "Geographical Area");
    else if(area_type==RPGP_AT_POLYLINE)
        strcpy(area_type_str, "Geographical Polyline");

    if(loc_type==RPGP_LATLON_LOCATION)
        strcpy(loc_type_str, "Lat Lon Coordinates");
    else if(loc_type==RPGP_XY_LOCATION)
        strcpy(loc_type_str, "XY Coordinates");
    else if(loc_type==RPGP_AZRAN_LOCATION)
        strcpy(loc_type_str, "Polar Coordinates");

    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_AREA ) {
        printf ("(Index %2d) AREA Component: (%d), %s, %s\n", 
             index, area->comp_type, area_type_str, loc_type_str);
        return;
    } 


/*    printf ("AREA: (%d), area type %d, location type %x\n",    */
/*             area->comp_type, RPGP_AREA_TYPE (area->area_type),*/
/*             RPGP_LOCATION_TYPE (area->area_type));            */
    printf ("\n(Index %2d) AREA Component: (%d), %s, %s\n", 
             index, area->comp_type, area_type_str, loc_type_str);

    printf("\nThis Component has %d parameters\n", area->numof_comp_params);
    if(flag[13] == FALSE)
/*    Print_params (area->numof_comp_params, area->comp_params); */
        cvt_Print_params (area->numof_comp_params, area->comp_params);
    else
        printf("     Printing of Component Parameters not Selected\n");
    
    printf("\n"); /* added for CVT */
    
    switch (RPGP_LOCATION_TYPE (area->area_type)) {

    case RPGP_LATLON_LOCATION:
/*        Print_points (area->numof_points, area->points); */
        cvt_Print_points (area->numof_points, area->points);
        break;

    case RPGP_XY_LOCATION:
/*        Print_xy_points (area->numof_points, area->points); */
        cvt_Print_xy_points (area->numof_points, area->points);
        break;

    case RPGP_AZRAN_LOCATION:
/*        Print_azran_points (area->numof_points, area->points); */
        cvt_Print_azran_points (area->numof_points, area->points);
        break;

    default:
        printf ("Unexpected location type (%d)\n", 
            RPGP_LOCATION_TYPE (area->area_type));
    }
    
    
} /* end cvt_Print_area */






/* CVT 4.4 used by cvt_Print_area */
/************************************************************/
void cvt_Print_points (int n_points, RPGP_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    
    for (i = 0; i < n_points; i++) {
        printf ("%s            point[%d]: lat = %8.2f, lon = %8.2f\n", 
            Add_space, i, points[i].lat, points[i].lon);
    }

} /* end cvt_Print_points */




/* CVT 4.4 used by cvt_Print_area */
/************************************************************/
void cvt_Print_xy_points (int n_points, RPGP_xy_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    
    for (i = 0; i < n_points; i++) {
        printf ("%s            point[%d]: x = %8.2f, y = %8.2f\n", 
            Add_space, i, points[i].x, points[i].y);
    }

} /* end cvt_Print_xy_points */




/* CVT 4.4 used by cvt_Print_area */
/************************************************************/
void cvt_Print_azran_points (int n_points, RPGP_azran_location_t *points) {
    int i;

    printf ("%s        $ %d points:\n", Add_space, n_points);
    
    for (i = 0; i < n_points; i++) {
        printf ("%s            point[%d]: range = %8.2f, azi = %8.2f\n", 
            Add_space, i, points[i].range, points[i].azi);
    }

} /* end cvt_Print_azran_points */






/* CVT 4.4 - PRINT TEXT COMPONENT */
/************************************************************/
void cvt_Print_text (RPGP_text_t *text, int index, int *flag) {


    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_TEXT ) {
        printf ("(Index %2d) TEXT Component: (%d)\n", index, text->comp_type);
        return;
    } 


    printf ("\n(Index %2d) TEXT Component: (%d)\n", index, text->comp_type);
    
    printf("\nThis Component has %d parameters\n", text->numof_comp_params);
    if(flag[13] == FALSE)
/*    Print_params (text->numof_comp_params, text->comp_params); */
        cvt_Print_params (text->numof_comp_params, text->comp_params);
    else
        printf("     Printing of Component Parameters not Selected\n");
    
    printf("\n"); /* added for CVT */
    
    printf ("%s    text: %s\n", Add_space, text->text);

} /* end cvt_Print_text */





/* CVT 4.4 - PRINT TABLE COMPONENT */
/************************************************************/
void cvt_Print_table (RPGP_table_t *table, int index, int *flag) {
    int i, j;


    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_TABLE ) {
        printf ("(Index %2d) TABLE Component: (%d), n_columns %d, n_rows %d\n", 
                 index, table->comp_type, table->n_columns, table->n_rows);
        return;
    } 

    printf ("\n(Index %2d) TABLE Component: (%d), n_columns %d, n_rows %d\n", 
             index, table->comp_type, table->n_columns, table->n_rows);

    printf("\nThis Component has %d parameters\n", table->numof_comp_params);
    if(flag[13] == FALSE)
/*    Print_params (table->numof_comp_params, table->comp_params); */
        cvt_Print_params (table->numof_comp_params, table->comp_params);
    else
        printf("     Printing of Component Parameters not Selected\n");

    printf("\n"); /* added for CVT */

    printf ("    title: %s\n", table->title.text);
    printf ("    column lables:");

    for (i = 0; i < table->n_columns; i++)
        printf (" %s", table->column_labels[i].text);

    printf ("\n");
    printf ("    row lables:");

    for (i = 0; i < table->n_rows; i++)
        printf (" %s", table->row_labels[i].text);

    printf ("\n");
    printf ("    table:\n");

    for (i = 0; i < table->n_rows; i++) {
        printf ("    ");
        
        for (j = 0; j < table->n_columns; j++)
            printf (" %s", table->entries[i * table->n_columns + j].text);
        
        printf ("\n");
        
    } /* end for i < n_rows */

} /* end cvt_Print_table */






/* CVT 4.4 - PRINT GRID COMPONENT */
/************************************************************/
void cvt_Print_grid (RPGP_grid_t *grid, int index, int *flag) {
  
  int i, total_data;

  /* added for CVT */
  char grid_type_str[30];

    /* added for CVT */
    if(grid->grid_type==RPGP_GT_ARRAY)
        strcpy(grid_type_str, "Non-Geographical Array");
    else if(grid->grid_type==RPGP_GT_EQUALLY_SPACED)
        strcpy(grid_type_str, "Flat Equally Spaced Grid");
    else if(grid->grid_type==RPGP_GT_LAT_LON)
        strcpy(grid_type_str, "Equally Spaced Lat Lon Grid");
    else if(grid->grid_type==RPGP_GT_POLAR)
        strcpy(grid_type_str, "Polar Array Grid");

    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_GRID ) {
        printf ("(Index %2d) GRID Component: (%d), %s, ndim %d \n", 
                index, grid->comp_type, grid_type_str, grid->n_dimensions);
        return;
    } 
    

/*    printf ("GRID: (%d), gtype %d, ndim %d \n",                    */
/*             grid->comp_type, grid->grid_type, grid->n_dimensions);*/
    printf ("\n(Index %2d) GRID Component: (%d), %s, ndim %d \n", 
                index, grid->comp_type, grid_type_str, grid->n_dimensions);
    total_data = 1;

    for (i = 0; i < grid->n_dimensions; i++) {
        printf (" %d", grid->dimensions[i]);
        total_data *= grid->dimensions[i];
    }

    printf ("\n");

    printf("\nThis Component has %d parameters\n", grid->numof_comp_params);
    if(flag[13] == FALSE)
/*    Print_params (grid->numof_comp_params, grid->comp_params); */
        cvt_Print_params (grid->numof_comp_params, grid->comp_params);
    else
        printf("     Printing of Component Parameters not Selected\n");

    printf("\n"); /* added for CVT */

    if (grid->n_dimensions <= 0) {
        printf ("Print_grid: n_dimensions is %d\n", grid->n_dimensions);
        return;
    }

    if (total_data <= 0) {
        printf ("Print_grid: No data\n");
        return;
    }

/*    Print_binaty_data (&(grid->data), total_data); */
    cvt_Print_binary_data (&(grid->data), total_data, flag);

} /* end cvt_Print_grid */







/* CVT 4.4 - PRINT EVENT COMPONENT */
/************************************************************/
void cvt_Print_event (RPGP_event_t *event, int index, int *flag) {


    /* added for CVT */
    if( flag[9]==LIST_ALL  || flag[9]==LIST_EVENT ) {
        printf ("(Index %2d) EVENT Component: (%d)\n", index, event->comp_type);
        return;
    } 

    printf ("\n(Index %2d) EVENT Component: (%d)\n", index, event->comp_type);
    Add_space = "    ";

    printf("\nThis Component has %d parameters\n", event->numof_event_params);
    if(flag[13] == FALSE)
/*    Print_params (event->numof_event_params, event->event_params); */
        cvt_Print_params (event->numof_event_params, event->event_params);
    else
        printf("     Printing of Component Parameters not Selected\n");

    printf("\n"); /* added for CVT */

/*    Print_components (event->numof_components, (char **)event->components); */
    cvt_Print_components (event->numof_components, (char **)event->components, 
                                                                           flag);

    Add_space = "";

} /* end cvt_Print_event */






/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/******************************************************************

    Searches for the type value in attribute string "attrs". If the
    type value is found, it is returned in "buf" of size "buf_size" 
    and the function returns the length of the type value. Returns 
    -1 if the type is not found. The type value must be non-empty 
    and single token.  

******************************************************************/
/* CVT 4.4 used by cvt_Print_binary_data */
/************************************************************/
int cvt_Get_data_type (char *attrs, char *buf, int buf_size) {
    char t1[16], t2[16], t3[16], t4[16], *p;
    int new_attr;

    if (attrs == NULL)
        return (-1);
        
    p = attrs;
    new_attr = 1;

    while (1) {
/*        p = Get_token (p, t1, 16); */
        p = cvt_Get_token (p, t1, 16);
        if (t1[0] == '\0')
            break;
        
        if (strcmp (t1, ";") == 0) {
            new_attr = 1;
            continue;
        }
        
        if (!new_attr)
            continue;
        
        if (strcasecmp (t1, "type") == 0) {
/*            char *pp = Get_token (p, t2, 16); */
            char *pp = cvt_Get_token (p, t2, 16);
            
            if (strcmp (t2, "=") == 0) {
                int len;
                
/*                pp = Get_token (pp, t3, 16); */
                pp = cvt_Get_token (pp, t3, 16);
/*                pp = Get_token (pp, t4, 16); */
                pp = cvt_Get_token (pp, t4, 16);
                
                if ( t3[0] != '\0' &&
                    (t4[0] == '\0' || strcmp (t4, ";") == 0) ) {
                    len = strlen (t3);
                    if (len >= buf_size)
                        len = buf_size - 1;
                    strncpy (buf, t3, len);
                    buf[len] = '\0';
                    return (len);
                }
                
            } /* end if "=" */
            
        } /* end if "type" */

        new_attr = 0;
    
    } /* end while */

    buf[0] = '\0';

    return (-1);

} /* end cvt_Get_data_type */


/******************************************************************

    Finds the first token of "text" in "buf" of size "buf_size". The
    returned token is always null-terminated and possibly truncated.
    Returns the pointer after the token. A token is a word separated
    by space, tab or line return. "=" and ";" is considered as a
    token even if they are not separated by space. If "text" is an
    empty string, an empty string is returned in "buf" and the return
    value is "text".
    
******************************************************************/
/* CVT 4.4 used by cvt_Get_data_type */
/************************************************************/
char *cvt_Get_token (char *text, char *buf, int buf_size) {
    char *p, *st, *next;
    int len;

    p = text;

    while (*p == ' ' || *p == '\t' || *p == '\n')
        p++;

    st = p;
    
    if (*p == '=' || *p == ';')
        len = 1;
    else if (*p == '\0')
        len = 0;
    else {
        while (*p != '\0' && *p != ' ' && *p != '\t' && 
                *p != '\n' && *p != '='  && *p != ';')
        p++;
        len = p - st;
    }
    
    next = st + len;
    if (len >= buf_size)
        len = buf_size - 1;
    strncpy (buf, st, len);
    buf[len] = '\0';

    return (next);

} /* end cvt_Get_token */

