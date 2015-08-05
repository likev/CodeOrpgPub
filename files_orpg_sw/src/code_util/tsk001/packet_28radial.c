/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 19:47:46 $
 * $Id: packet_28radial.c,v 1.9 2012/09/13 19:47:46 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* packet_28radial.c */

#include <math.h>
#include <Xm/Xm.h>
#include "packet_28_cvg.h"

#include "byteswap.h"


char data_type[32];
char *data_attr_string = NULL;

/*  GLOBALS: */
  RPGP_product_t *generic_product;
  RPGP_radial_t   *radial_comp=NULL;
    
  
  
  
  
  
void display_radial(int index, int location_flag, int replay)
{
int retval=1;

    if(verbose_flag)
        fprintf(stderr,
             "\n----- Display Generic Radial Data ----- index is %d\n\n", index);

/* only decode when not replaying from history */
if(replay == FALSE)
   retval = decode_generic_radial(index, location_flag);
   
   if(retval!=0)
       output_generic_radial(location_flag);
   
}





 
/* /////////////////////////////////////////////////////////////// */
int decode_generic_radial(int index, int location_flag)
{


    int len, i;


    Prod_header *hdr;
    Graphic_product *gp;

    short *product_data = (short *)(sd->icd_product);
    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);

   unsigned short temp_max_level, temp_n_l_flags, temp_n_t_flags;
   int retval;
   
/* TEST */
/* unsigned short *us_ptr=NULL; */



int found_tok;
char *tok;   /*  token pointer for parsing attribute string */
char tok_sep[] = ",;= ";  /*  token separator list, do I need \" ??  */





fprintf(stderr,
             "\n----- Decode Generic Radial Data ----- index is %d\n\n", index);

    /**** SECTION 1 ***********************************************************/
    /* Reading Radial Component Header Information                            */
    /**************************************************************************/


    
   generic_product = (RPGP_product_t *)sd->generic_prod_data;
   radial_comp = (RPGP_radial_t *)generic_product->components[index];
   
   
   /* Allocate memory for the generic radial screen data structure */
   sd->gen_rad = malloc(sizeof(generic_radial_data));
   assert( sd->gen_rad != NULL );

  /* Extract number of data elements, range interval, and number of 
    * radials.  CVG ASSUMES ALL RADIALS HAVE THE SAME NUMBER OF BINS
    * EVEN THOUGH THE NUMBER IS SPECIFIED FOR EACH RADIAL - THIS IS
    * TESTED IN SECTION 3
    */
   sd->gen_rad->data_elements    = (int)   radial_comp->radials[1].n_bins;
   sd->gen_rad->range_interval   = (float) radial_comp->bin_size;
   sd->gen_rad->number_of_radials= (int)   radial_comp->numof_radials;

/*  TEST */
/* fprintf(stderr,"TEST DECODE - gen_rad range interval is %f KM\n", sd->gen_rad->range_interval); */
/* fprintf(stderr,"TEST DECODE - scale factor is %f NM\n", sd->scale_factor); */



   /* read PDB parameters for decoding using the scale offset         */
   /* these values will only be correct for those products using them */
   sd->gen_rad->decode.decode_flag = NO_DECODE;
   
   /* the following must be used to read either float or 4-byte integers */
   /* because the first HW always contains the most significant short in */
   /* a WSR-88D product.                                                 */
   retval = read_orpg_product_float( (void*) &product_data[THR_01_OFFSET/2],
                                     (float*) &sd->gen_rad->decode.prod_Scale);
   retval = read_orpg_product_float( (void*) &product_data[THR_03_OFFSET/2],
                                     (float*) &sd->gen_rad->decode.prod_Offset);
   
   /* note: the data structure can handle four bytes for temp_max_level but */
   /*       the design of the product limits it to two bytes in the product */
   temp_max_level            = (unsigned short) product_data[THR_06_OFFSET/2];
   
   temp_n_l_flags            = (unsigned short) product_data[THR_07_OFFSET/2];
   temp_n_t_flags            = (unsigned short) product_data[THR_08_OFFSET/2];
   sd->gen_rad->decode.prod_max_level = (unsigned int) temp_max_level;
   sd->gen_rad->decode.prod_n_l_flags = (int)          temp_n_l_flags;
   sd->gen_rad->decode.prod_n_t_flags = (int)          temp_n_t_flags;



   if(verbose_flag) {
       fprintf(stderr,"NOTE: THE FOLLOWING OUTPUT IS MEANINGLESS UNLESS THE\n");
       fprintf(stderr,"         PRODUCT CONTAINS THE OFFSET SCALE PARAMETERS \n");
       fprintf(stderr,"Gen Radial - Scale: %f   Offset: %f \n"                    
                     "    max level: %u    lead flags: %d    trail flags: %d\n",  
                  sd->gen_rad->decode.prod_Scale, sd->gen_rad->decode.prod_Offset,
                  sd->gen_rad->decode.prod_max_level,                             
                  sd->gen_rad->decode.prod_n_l_flags,                             
                  sd->gen_rad->decode.prod_n_t_flags);                            
   }


   /* advance pointer to beginning of packet block */
   product_data = product_data + ((96 + 120 + 16) / 2); 
                  /*  96 bytes internal hdr, 120 bytes mhb & pdb */
   if(verbose_flag) {
       fprintf(stderr,"packet code=%hd\n",product_data[0]);
       fprintf(stderr,"GENERIC RADIAL - number of bins is %d\n",
                                                  sd->gen_rad->data_elements);
       fprintf(stderr,"     number of radials is %d, range interval is %6.2f\n",
                 sd->gen_rad->number_of_radials, sd->gen_rad->range_interval);
   }


    /*  SO FAR NO DISTINCTION MADE */
    if(location_flag == PRODUCT_FRAME) {
        ;
    } else if(location_flag == PREFS_FRAME) {
        ;
    } /* end if PREFS_FRAME     */

    
    /**** SECTION 2 ***********************************************************/
    /* Reading Radial Component Parameters                                    */
    /**************************************************************************/
    
    /*  FUTURE ENHANCEMENT */
    

    
    /**** SECTION 3 ***********************************************************/
    /* Decoding Radial Component                                              */
    /**************************************************************************/

    if(verbose_flag)
        fprintf(stderr, "*****Decoding Generic Radial*****\n");

    /*  SO FAR NO DISTINCTION MADE */
    if(location_flag == PRODUCT_FRAME) {
        ;
    } else if(location_flag == PREFS_FRAME) {
        ;
    }

 
   /* Extract the data type contained in the generic data structure.
    * CVG REQUIRES THAT ALL RADIALS HAVE THE SAME DATA TYPE EVEN THOUGH
    * THIS IS SPECIFIED IN THE DATA STRUCTRUE FOR EACH RADIAL
    */

    len = strlen(radial_comp->radials[1].bins.attrs);
    data_attr_string = malloc( (len + 1) * sizeof( char) );
    strcpy(data_attr_string, radial_comp->radials[1].bins.attrs);

    if(verbose_flag) 
        fprintf(stderr,"Generic Radial - data attribute string is \n'%s'\n", 
                                                           data_attr_string); 

    /*  get the 'type' value by parsing attributes ------------------ */ 
    /*  FUTURE ENHANCEMENT - make this section a stand-alone function  */
    /*  first version assumes lower case */
    found_tok = FALSE;
    strcpy(data_type, "No_Data_Type_Found");
    tok = strtok(data_attr_string, tok_sep);
    if(tok==NULL) {
        fprintf(stderr,"\nERROR IN GENERIC RADIAL PRODUCT - data attribute "
                         "     string is empty.\n\n");

    } else { /*  parse attribute string */
        if(strcmp(tok, "type") == 0) {
                found_tok = TRUE;
        }
        while((tok = strtok(NULL, tok_sep)) != NULL ) {
            if(found_tok == TRUE) {
                strcpy(data_type, tok);
                break;
            }
            if(strcmp(tok, "type") == 0) {
                found_tok = TRUE;
            }
        } /*  end while */
        
    } /*  end else parse attribute string */
    
    if(found_tok==FALSE) {
        fprintf(stderr,"\nERROR IN GENERIC RADIAL PRODUCT - data type "
                         "     is not specified in attribute list.\n\n");
        free(sd->gen_rad);
        sd->gen_rad=NULL;
        return(0);
    }
    /*  finished getting the 'type' value ----------------------------- */
/* test */
/* fprintf(stderr,"TEST Value of the type attribute is '%s'\n", data_type);  */
 
   
    if (strcmp (data_type, "short") == 0) {
       sd->gen_rad->type_data = DATA_SHORT;

    } else if (strcmp (data_type, "byte") == 0) {
        sd->gen_rad->type_data = DATA_BYTE;

    } else if (strcmp (data_type, "int") == 0) {
        sd->gen_rad->type_data = DATA_INT;

    } else if (strcmp (data_type, "float") == 0) {
        sd->gen_rad->type_data = DATA_FLOAT;
       
    } else if (strcmp (data_type, "double") == 0) {
        sd->gen_rad->type_data = DATA_DOUBLE;

    } else if (strcmp (data_type, "uint") == 0) {
        sd->gen_rad->type_data = DATA_UINT;
       
    } else if (strcmp (data_type, "ushort") == 0) {
        sd->gen_rad->type_data = DATA_USHORT;
       
    } else if (strcmp (data_type, "ubyte") == 0) {
        sd->gen_rad->type_data = DATA_UBYTE;

    } else {
       fprintf (stderr, "GENERIC RADIAL ERROR: data: type (%s) not supported\n",
                                                                      data_type);
       free(sd->gen_rad);
       sd->gen_rad=NULL;
       return(0);
    }

   /* Allocate memory for the radial data in the screen data structure */
   /* Copy the radial data arrays into the screen data structure       */
   allocate_and_load_screen_radial_data(sd->gen_rad->type_data);

   /* Allocate memory for the azimuth and width data */
   sd->gen_rad->azimuth_angle = 
                         malloc( sd->gen_rad->number_of_radials*sizeof(float) );
   assert( sd->gen_rad->azimuth_angle != NULL );

   sd->gen_rad->azimuth_width = 
                         malloc( sd->gen_rad->number_of_radials*sizeof(float) );
   assert( sd->gen_rad->azimuth_width != NULL );
 

   /* copy the azimuth and width data to the screen data structure */
   for(i=0; i<sd->gen_rad->number_of_radials; i++ ) {   
    
      sd->gen_rad->azimuth_width[i] = radial_comp->radials[i].width;
      /* CVG 9.1b - the azimuth angle contained in the radial header of the    */
      /*            of the radial component was changed from the center        */
      /*            azimuth to the beginning azimuth in order to be consistent */
      /*            with packet 16 and packet AF1F (Build 12 ORPG change)      */
/*  need to convert center azimuth to beginning azimuth for display */
/*      sd->gen_rad->azimuth_angle[i] = radial_comp->radials[i].azimuth -
 *                                         (radial_comp->radials[i].width / 2);
 */
      /* BUILD 12 PATCH 1 */
      sd->gen_rad->azimuth_angle[i] = radial_comp->radials[i].azimuth;
   }

      /* FUTURE ENHANCEMENT - add logic here to test consistency of
       * number of bins, elevation, and data type.
       */   
   
   
   
/* // test section to confirm the data was copied //////////////////////// */
/* for(i=0; i<sd->gen_rad->number_of_radials; i++ ) { */
/*    if(i < 10) { */
/*    fprintf(stderr,"TEST radial %d, read width = %f, read start = %f\n", i, */
/*                   radial_comp->radials[i].width, radial_comp->radials[i].azimuth); */
/*    fprintf(stderr,"TEST radial %d, stored width = %f, stored start = %f\n", i, */
/*                   sd->gen_rad->azimuth_width[i], sd->gen_rad->azimuth_angle[i]);                */
/*    } */
/*    // test */
/*    if(i==1) { */
/*        fprintf(stderr,"Screen Data, radial %d\n", i); */
/*        for(j=0; j<sd->gen_rad->data_elements; j++) { */
/*            fprintf(stderr," %d ", sd->gen_rad->data.ushort_d[i][j]); */
/*        } */
/*        fprintf(stderr,"\n"); */
/*    } */
/*    // test2 */
/*    if(i==1) { */
/*        fprintf(stderr,"Radial Data, radial %d\n", i); */
/*        for(j=0; j<sd->gen_rad->data_elements; j++) { */
/*            us_ptr = (unsigned short *)radial_comp->radials[i].bins.data; */
/*            fprintf(stderr," %d ", us_ptr[j] ); */
/*        } */
/*        fprintf(stderr,"\n"); */
/*    } */
/* } // end for number_of_radials */
/* // end test section /////////////////////////////////////////////////////// */



return(1);

} /*  END DECODE_GENERIC_RADIAL */








/* ///////////////////////////////////////////////////////////////////////// */

void output_generic_radial(int location_flag) 
{
    int beam;

    int i;
    float   azimuth1, azimuth2;
    
    unsigned int data_u=0;
    int data_s=0;
    float data_f=0.0;
    double data_d=0.0;
    
    int Color;
    int old_Color;

    XPoint  X[12];
    float   sin1, sin2;
    float   cos1, cos2;   
    float   scale_x, scale_y;
    
    int     center_pixel;
    int     center_scanl;


    char *legend_filename_ptr=NULL, legend_filename[200];
    int digital_flag, *flag_ptr;
    FILE *legend_file;

    int legend_error = GOOD_LEGEND;
    
    Prod_header *hdr;
    Graphic_product *gp;


    Widget d;
    XmString xmstr;


    /**** SECTION 4 ***********************************************************/
    /* Displaying Radial Component                                            */
    /**************************************************************************/


    hdr = (Prod_header *)(sd->icd_product);
    gp  = (Graphic_product *)(sd->icd_product + 96);

    sd->last_image = GENERIC_RADIAL;
    
    
    center_pixel = pwidth/2;  /* initialized to center of canvas */
    center_scanl = pheight/2; /* initialized to center of canvas */
    

    /*  sd->scale_factor is set in decode_generic_radial() rather than during */
    /*                   product load as with traditional products */
    
    /* correct center pixel for the off_center value (if any) */
    center_pixel = center_pixel - (int) (sd->x_center_offset * sd->scale_factor);  
    center_scanl = center_scanl - (int) (sd->y_center_offset * sd->scale_factor); 

    scale_x =  sd->scale_factor;
    scale_y = -sd->scale_factor;

/* TEST */
/* fprintf(stderr,"TEST - display p16, x center is %d, y center is %d\n", */
/*                               center_pixel, center_scanl); */
/* fprintf(stderr,"TEST -             x offset is %d, y offset is %d, zoom is %f\n", */
/*                      sd->x_center_offset, sd->y_center_offset, sd->scale_factor); */

    /*  SO FAR NO DISTINCTION MADE */
    if(location_flag == PRODUCT_FRAME) {
        ;
    } else if(location_flag == PREFS_FRAME) {
        ;
    }
 
 
     /* if the palette size is, for whatever reason, 0,
     * we don't want to be printing out the product */
    if (palette_size <= 0 ||  palette_size > 256) {
       fprintf(stderr, "* WARNING *****************************************\n");
       fprintf(stderr, "*     Size of color palette exceeds the permitted *\n");
       fprintf(stderr, "*     number of colors for generic leged files.   *\n");
       fprintf(stderr, "***************************************************\n");
      
    }


    /* see if the product's legend info is encoded like the digital products' */
    flag_ptr = assoc_access_i(digital_legend_flag, hdr->g.prod_id);
    
    if(flag_ptr == NULL)
      digital_flag = 0;
    else
      digital_flag = *flag_ptr;


    /* make sure the digital product info is set up correctly */
    if(digital_flag == 0) {
            fprintf(stderr,"\nCONFIGURATION ERROR\n");
            fprintf(stderr,"Generic Radial product incorrectly configured\n");
            fprintf(stderr,"using Legend Method %d.\n", digital_flag);
            
            d = XmCreateErrorDialog(shell, "Configuration Error", NULL, 0);
            xmstr = XmStringCreateLtoR("Generic Radial product incorrectly "
                                       "configured using Legend Method 0 - 3.\n"
                                       "Edit preferences for this product using "
                                       "the CVG Product Specific Info Menu.",
                XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(XtParent(d), XmNtitle, "Configuration Error", NULL);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            
        return;
    }


     /* ----- read legend file --------------------*/

      legend_filename_ptr = assoc_access_s(digital_legend_file, hdr->g.prod_id);
      /*  NO LEGEND FILE ERROR IS TESTED DURING PRODUCT POST LOAD */

      sprintf(legend_filename, "%s/legends/%s", 
                                       config_dir, legend_filename_ptr);
      if((legend_file = fopen(legend_filename, "r")) == NULL) {
          fprintf(stderr,"\nCONFIGURATION ERROR\n");
          fprintf(stderr, "Could not open legend file: %s", 
                                            legend_filename);
          return;
      }
      
      legend_error = read_generic_legend(legend_file, legend_filename, 
                                                   digital_flag, PRODUCT_FRAME);
      
      
      fclose(legend_file); 
      


     if(legend_error==LEGEND_ERROR) {
          fprintf(stderr,"ERROR - Display of Generic Radial Aborted Due to "
                          " errors in the generic legend file.\n");
          return;
      }


      /*  TO DO - IN FUTURE, SUPPORT ALL TYPES */
      if(sd->gen_rad->type_data==DATA_BYTE) 
          ;
      else if(sd->gen_rad->type_data==DATA_UBYTE)
          ;
      else if(sd->gen_rad->type_data==DATA_SHORT) 
          ;
      else if(sd->gen_rad->type_data==DATA_USHORT)
          ;
      else if(sd->gen_rad->type_data==DATA_INT) 
          ;
      else if(sd->gen_rad->type_data==DATA_UINT)
          ;
      else if(sd->gen_rad->type_data==DATA_FLOAT)
          ;
      else if(sd->gen_rad->type_data==DATA_DOUBLE)
          ;

   
    
    
    for (beam=0;beam<sd->gen_rad->number_of_radials;beam++) {
        float extra_width = 1.5; /* Added to eliminate black speckels between radials */
        
        /* text output of a single radial for diagnostics */
        if((beam==0) && (verbose_flag)) { 
            fprintf(stderr,"Printing Radial %d: angle %6.2f delta %6.2f\n", 
                         beam+1, sd->gen_rad->azimuth_angle[beam],
                                          sd->gen_rad->azimuth_width[beam]);
            print_a_radial(beam);
        }
        

    
        /* DRAW A RADIAL */
        
        /*  Artifacts avoided */
        /*  by drawing BLACK, the background color, for all segments. */
        /* Don't draw such a wide beam on the last radial */
        if (beam == sd->gen_rad->number_of_radials-1) extra_width = 0.5;
        azimuth1 = sd->gen_rad->azimuth_angle [beam];
        azimuth2 = azimuth1 + sd->gen_rad->azimuth_width [beam] + extra_width;
  
        sin1 = sin ((double) (azimuth1+90)*DEGRAD);
        sin2 = sin ((double) (azimuth2+90)*DEGRAD);
        cos1 = cos ((double) (azimuth1-90)*DEGRAD);
        cos2 = cos ((double) (azimuth2-90)*DEGRAD);
  
        old_Color = -1;
        Color = -1;

        /*  To reduce the number of XPolygonFill operations, 
         *  only paint gate(s) when a color change occurs.
         */

        /*                 X[0],X[4]      X[3] */
        /*                 - - * - - - - - * - - */
        /*     PREVIOUS BIN <= |           | => NEXT BIN */
        /*               - - - * - - - - - * - - */
        /*                    X[1]        X[2] */

        /*  Initialize near points of sector to be drawn        */
        X[0].x = X[1].x = X[4].x = center_pixel;
        X[0].y = X[1].y = X[4].y = center_scanl;
        
        for (i=0; i < sd->gen_rad->data_elements; i++) {

            /*  TO DO - IN FUTURE, SUPPORT ALL TYPES */
            if(sd->gen_rad->type_data==DATA_BYTE) 
                data_s = (int) sd->gen_rad->data.byte_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_UBYTE)
                data_u = (unsigned int) sd->gen_rad->data.ubyte_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_SHORT) 
                data_s = (int) sd->gen_rad->data.short_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_USHORT)
                data_u = (unsigned int) sd->gen_rad->data.ushort_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_INT) 
                data_s = (int) sd->gen_rad->data.int_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_UINT)
                data_u = (unsigned int) sd->gen_rad->data.uint_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_FLOAT)
                data_f = (float) sd->gen_rad->data.float_d[beam][i];
            
            else if(sd->gen_rad->type_data==DATA_DOUBLE)
                data_d = (double) sd->gen_rad->data.double_d[beam][i];
            
            /*  TO DO - NEED TO TEST SIGNED INTEGER, FLOAT AND DOUBLE TYPES */
            if(sd->gen_rad->type_data==DATA_UBYTE  ||
               sd->gen_rad->type_data==DATA_USHORT ||
               sd->gen_rad->type_data==DATA_UINT)
                Color = lookup_gen_color_u(data_u);
            
            else if(sd->gen_rad->type_data==DATA_BYTE  ||
                    sd->gen_rad->type_data==DATA_SHORT ||
                    sd->gen_rad->type_data==DATA_INT)
               Color = lookup_gen_color_s(data_s);  
            
            else if(sd->gen_rad->type_data==DATA_FLOAT)
               Color = lookup_gen_color_f(data_f); 
            
            else if(sd->gen_rad->type_data==DATA_DOUBLE)
               Color = lookup_gen_color_d(data_d);
 
 
 
            
            if(Color == -1) {
                Color = 0;
                fprintf(stderr,
                        "ERROR GEN RADIAL - Color for data level %ul not found\n",
                                                            data_u);
            }
           
           /*  THE FOLLOWING ASSUMES THAT COLOR 0 IS BACKGROUND (I.E. BLACK or WHITE) */
           /*  FUTURE CHANGE - If Color == 0, set color = Assigned Background */

            /*  If successive colors are the same we draw as one large segement */
            if (Color != old_Color) {

                X[2].x = i * cos2 * scale_x + center_pixel;
                X[2].y = i * sin2 * scale_y + center_scanl;
                X[3].x = i * cos1 * scale_x + center_pixel;
                X[3].y = i * sin1 * scale_y + center_scanl;
    
                XSetForeground(display, gc, display_colors[old_Color].pixel);
                XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, 
                                                            CoordModeOrigin);
    
                X[0].x = X[3].x;
                X[0].y = X[3].y;
                X[1].x = X[2].x;
                X[1].y = X[2].y;
                X[4].x = X[0].x;
                X[4].y = X[0].y;

            } /*  end Color != old_Color */

            old_Color = Color;
            
        }  /*  end for each data element */


        /* draw last segmemt in case there was no change in color */
        i = sd->gen_rad->data_elements;        
        X[2].x = (i-1) * cos2 * scale_x + center_pixel;
        X[2].y = (i-1) * sin2 * scale_y + center_scanl;
        X[3].x = (i-1) * cos1 * scale_x + center_pixel;
        X[3].y = (i-1) * sin1 * scale_y + center_scanl;
       
        XSetForeground(display, gc, display_colors[old_Color].pixel);
        XFillPolygon(display, sd->pixmap, gc, X, 4, Convex, CoordModeOrigin);
        
    
    }  /*  end for each radial */
    
/*    XSetForeground(display, gc, white_color); */
    
    

    /**** SECTION 5 ***********************************************************/
    /* Cleanup                                                                */
    /**************************************************************************/    

    /*  SO FAR NO DISTINCTION MADE */
    if(location_flag == PRODUCT_FRAME) {
        ;
    } else if(location_flag == PREFS_FRAME) {
        ;
    }
    
    if(data_attr_string!=NULL) {
       free(data_attr_string);
       data_attr_string = NULL;
    }
    

        
} /*  end output_generic_radial() */







/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/* this function is used in the following files: callbacks.c/h, 
 *     dispatcher.c/h, gui_display.c/h, and prod_display.c/h
 */
void delete_generic_radial()         
{                               
  int i;                        
                                
    if(verbose_flag)            
        fprintf(stderr, "*****ENTER delete_generic_radial*****\n");  
                                
    if(sd->gen_rad != NULL) {       

      /* for the purposes of freeing allocated data, any member of the union is OK */
      /***type***/
      for(i=0; i < sd->gen_rad->number_of_radials; ++i)
          free(sd->gen_rad->data.double_d[i]);
      /***type***/     

 
      free(sd->gen_rad->data.double_d);
      
      free(sd->gen_rad->azimuth_angle);
      free(sd->gen_rad->azimuth_width);
      free(sd->gen_rad);
      sd->gen_rad = NULL;
      sd->last_image=NO_IMAGE;
  
    } else 
      fprintf(stderr, "ERROR - attempted to delete generic radial with NULL structure!\n"); 
  
} 


/* /////////////////////////////////////////////////////////////////// */
/*  */
/*  ADDITIONAL HELPER FUNCTIONS */
/*  */
/* /////////////////////////////////////////////////////////////////// */

void allocate_and_load_screen_radial_data(generic_d_type array_type)
{
int i;    

   /* assures correct sizing and access of a 2-D array in screen data */
   switch (array_type) {
   
      case DATA_UBYTE:
         sd->gen_rad->data.ubyte_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(unsigned char *));
         assert(sd->gen_rad->data.ubyte_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.ubyte_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(unsigned char));
            assert( sd->gen_rad->data.ubyte_d[i] != NULL );
            memcpy(sd->gen_rad->data.ubyte_d[i], radial_comp->radials[i].bins.data,
                   sizeof(unsigned char) * sd->gen_rad->data_elements);
         } 
         break;

      case DATA_BYTE:
         sd->gen_rad->data.byte_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(char *));
         assert(sd->gen_rad->data.byte_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.byte_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(char));
            assert( sd->gen_rad->data.byte_d[i] != NULL );
            memcpy(sd->gen_rad->data.byte_d[i], radial_comp->radials[i].bins.data,
                   sizeof(char) * sd->gen_rad->data_elements);
         } 
         break;

      case DATA_USHORT:
         sd->gen_rad->data.ushort_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(unsigned short *));
         assert(sd->gen_rad->data.ushort_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.ushort_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(unsigned short));
            assert( sd->gen_rad->data.ushort_d[i] != NULL );
            memcpy(sd->gen_rad->data.ushort_d[i], radial_comp->radials[i].bins.data,
                   sizeof(unsigned short) * sd->gen_rad->data_elements);
         } 
         break; 

      case DATA_SHORT:
         sd->gen_rad->data.short_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(short *));
         assert(sd->gen_rad->data.short_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.short_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(short));
            assert( sd->gen_rad->data.short_d[i] != NULL );
            memcpy(sd->gen_rad->data.short_d[i], radial_comp->radials[i].bins.data,
                   sizeof(short) * sd->gen_rad->data_elements);
         }
         break; 

      case DATA_UINT:
         sd->gen_rad->data.uint_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(unsigned int *));
         assert(sd->gen_rad->data.uint_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.uint_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(unsigned int));
            assert( sd->gen_rad->data.uint_d[i] != NULL );
            memcpy(sd->gen_rad->data.uint_d[i], radial_comp->radials[i].bins.data,
                   sizeof(unsigned int) * sd->gen_rad->data_elements);
         }
         break;    

      case DATA_INT:
         sd->gen_rad->data.int_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(int *));
         assert(sd->gen_rad->data.int_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.int_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(int));
            assert( sd->gen_rad->data.int_d[i] != NULL );
            memcpy(sd->gen_rad->data.int_d[i], radial_comp->radials[i].bins.data,
                   sizeof(int) * sd->gen_rad->data_elements);
         }
         break;

      case DATA_FLOAT:
         sd->gen_rad->data.float_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(float *));
         assert(sd->gen_rad->data.float_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.float_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(float));
            assert( sd->gen_rad->data.float_d[i] != NULL );
            memcpy(sd->gen_rad->data.float_d[i], radial_comp->radials[i].bins.data,
                   sizeof(float) * sd->gen_rad->data_elements);
         }
         break;

      case DATA_DOUBLE:
         sd->gen_rad->data.double_d =
             malloc( sd->gen_rad->number_of_radials * sizeof(double *));
         assert(sd->gen_rad->data.double_d != NULL);

         for( i = 0; i < sd->gen_rad->number_of_radials; i++ ) {
            sd->gen_rad->data.double_d[i] = 
                malloc((sd->gen_rad->data_elements + 1) * sizeof(double));
            assert( sd->gen_rad->data.double_d[i] != NULL );
            memcpy(sd->gen_rad->data.double_d[i], radial_comp->radials[i].bins.data,
                   sizeof(double) * sd->gen_rad->data_elements);
         }
         break;


      default:
         break;
   } /*  end switch */
   
    
} /*  end allocate_and_load_screen_radial_data() */



void print_a_radial(int rad_num)
{
   int n, k;
       
   k = 0;
   
   switch (sd->gen_rad->type_data) {
   
      case DATA_UBYTE:
         fprintf(stderr,"Data Array Elements are type 'unsigned char'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.ubyte_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;

      case DATA_BYTE:
         fprintf(stderr,"Data Array Elements are type 'char'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.byte_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;

      case DATA_USHORT:
         fprintf(stderr,"Data Array Elements are type 'unsigned short'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.ushort_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break; 

      case DATA_SHORT:
         fprintf(stderr,"Data Array Elements are type 'short'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.short_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break; 

      case DATA_UINT:
         fprintf(stderr,"Data Array Elements are type 'unsigned int'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.uint_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;    

      case DATA_INT:
         fprintf(stderr,"Data Array Elements are type 'int'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5d ",sd->gen_rad->data.int_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;

      case DATA_FLOAT:
         fprintf(stderr,"Data Array Elements are type 'float'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5f ",sd->gen_rad->data.float_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;

      case DATA_DOUBLE:
         fprintf(stderr,"Data Array Elements are type 'double'\n");
         for(n=0;n<sd->gen_rad->data_elements;n++) {
            fprintf(stderr," %5f ",sd->gen_rad->data.double_d[rad_num][n]);
               if(k==9) {
                  printf("\n");
                  k=0;
               } else {
                  k++;
               }
         }
         printf("\n"); 
         break;


      default:
         break;
   } /*  end switch */
    
    
} /*  end print_a_radial() */




int lookup_gen_color_u(unsigned int d_level)
{
int i;
    
    for(i=1; i < sd->gen_rad->decode.n_leg_colors; i++ )
       if(d_level < sd->gen_rad->color.data.ui_dl[i])
          return(sd->gen_rad->color.dl_clr[i-1]);

    if(d_level >= sd->gen_rad->color.data.ui_dl[sd->gen_rad->decode.n_leg_colors-1])
       return(sd->gen_rad->color.dl_clr[sd->gen_rad->decode.n_leg_colors-1]);
    
    return(-1);
    
    
} /*  end lookup_gen_color_u */



int lookup_gen_color_s(int d_level)
{
int i;


    for(i=1; i < sd->gen_rad->decode.n_leg_colors; i++ )
       if(d_level < sd->gen_rad->color.data.si_dl[i])
          return(sd->gen_rad->color.dl_clr[i-1]);

    if(d_level >= sd->gen_rad->color.data.si_dl[sd->gen_rad->decode.n_leg_colors-1])
       return(sd->gen_rad->color.dl_clr[sd->gen_rad->decode.n_leg_colors-1]);
    
    return(-1);

} /*  end lookup_gen_color_s */



int lookup_gen_color_f(float d_level)
{
int i;


    for(i=1; i < sd->gen_rad->decode.n_leg_colors; i++ )
       if( (double)d_level < sd->gen_rad->color.data.d_dl[i])
          return(sd->gen_rad->color.dl_clr[i-1]);

    if((double)d_level >= 
                   sd->gen_rad->color.data.d_dl[sd->gen_rad->decode.n_leg_colors-1])
       return(sd->gen_rad->color.dl_clr[sd->gen_rad->decode.n_leg_colors-1]);
    
    return(-1);

} /*  end lookup_gen_color_f */



int lookup_gen_color_d(double d_level)
{
int i;


    for(i=1; i < sd->gen_rad->decode.n_leg_colors; i++ )
       if(d_level < sd->gen_rad->color.data.d_dl[i])
          return(sd->gen_rad->color.dl_clr[i-1]);

    if(d_level >= sd->gen_rad->color.data.d_dl[sd->gen_rad->decode.n_leg_colors-1])
       return(sd->gen_rad->color.dl_clr[sd->gen_rad->decode.n_leg_colors-1]);
    
    return(-1);

}/*  end lookup_gen_color_d */



