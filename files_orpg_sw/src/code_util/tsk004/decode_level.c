/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:27:00 $
 * $Id: decode_level.c,v 1.1 2009/05/15 17:27:00 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* decode_level.c */



#include "decode_level.h"






/* modified to use the new scale and offset functions */
/* correctly accounts for leading flag values */
/* input the scaled 8-bit integer value in packet 16 
 * output the decoded floating point value
 * special case: output -999.0 for first flag value (below threshold)
 *               output -888.0 for second flag value (range folded)
 *               output  999.0 if called with incorrect flag value
 */
float scale_parameter(short val,int flag) {

  /*  function only works for standard moment decoding */
  float offset_code = 999.0, scale_code = 999.0;


  switch(flag) {
      case 1:  /*  standard reflectivity decoding */
             offset_code = 66.0;
             scale_code = 2.0;
             break;
      case 2:  /*  standard velocity decoding (0.5 m/s) */
             offset_code = 129.0;
             scale_code = 2.0;
             break;
      case 3:  /*  standard velocity decoding (1.0 m/s) */
             offset_code = 129.0;
             scale_code = 1.0;
             break;
      case 4:  /*  standard spectrum width decoding */
             offset_code = 129.0;
             scale_code = 2.0;
             break;
             
      default: return( 999.0 ); /*  called with bad flag value */
  }
  
  if(val == 0) /*  first flag */
      return (-999.0);
  else if(val == 1) /*  second flag */
      return (-888.0);
  else /*  decode value */
      return ( ((float)val -  offset_code) / scale_code );
  
} /* end scale_parameter */





/* CVT 4.4  */
/* function to read scale and offset values to decode       */
/* input unsigned char arrays (packet 16 and generic moment)*/
/* and input unsigned short arrays (generic moment)         */
/* returns FALSE if fdecode and no parameters are found,    */
/* returns TRUE otherwise                                   */
/*****************************************************************************/
/*****************************************************************************/
/* CVT 4.4  */
int get_decode_params(int decode_flag, short *product, decode_params_t *params) {
    
int retval;

int exist_params_found = FALSE, file_params_found = FALSE;

int i;

unsigned int   temp_Scale;
unsigned int   temp_Offset;
unsigned short temp_max_level;
unsigned short temp_n_l_flags;
unsigned short temp_n_t_flags;

Graphic_product prod_hdr;
short *hdr_ptr;

Graphic_product *gp;
int the_pcode;

  /* CVT 4.4 */
  struct stat confdirstat;
  char config_file[128];

/* see misc_functions2.h for definition of existing_params[] */

   /* FOR DEVELOPMENT OF decode_level */
   params->Scale=1.0; 
   params->Offset=10.0;
   params->max_level=255;
   params->n_l_flags=0;
   params->n_t_flags=0;
    

   gp = (Graphic_product*) (product);
   the_pcode = gp->msg_code;
#ifdef LITTLE_ENDIAN_MACHINE
   the_pcode = SHORT_BSWAP(the_pcode);
#endif
   



   /*************************************************************************/
   if(decode_flag==PDECODE) {
 
      fprintf(stderr,"   NOTE: if the product does not contain the Scale Offset\n"
                     "         parameters the decoded values will be incorrect.\n\n");

      if(product==NULL) {
         fprintf(stderr,"ERROR get_decode_params, product buffer is NULL\n");
         return FALSE;
      }

   
      memcpy(&prod_hdr, gp, sizeof(Graphic_product));
   
#ifdef LITTLE_ENDIAN_MACHINE   
      MISC_short_swap(&prod_hdr,60);
      prod_hdr.msg_time=INT_SSWAP(prod_hdr.msg_time);
      prod_hdr.msg_len=INT_SSWAP(prod_hdr.msg_len);
      prod_hdr.latitude=INT_SSWAP(prod_hdr.latitude);
      prod_hdr.longitude=INT_SSWAP(prod_hdr.longitude);
      prod_hdr.gen_time=INT_SSWAP(prod_hdr.gen_time);
      prod_hdr.sym_off=INT_SSWAP(prod_hdr.sym_off);
      prod_hdr.gra_off=INT_SSWAP(prod_hdr.gra_off);
      prod_hdr.tab_off=INT_SSWAP(prod_hdr.tab_off);
#endif

      hdr_ptr = (short*) &prod_hdr;
      
      temp_Scale = (unsigned int) hdr_ptr[THR_01_OFFSET/2];
      temp_Offset = (unsigned int) hdr_ptr[THR_03_OFFSET/2];
      temp_max_level = (unsigned short) hdr_ptr[THR_06_OFFSET/2];
      temp_n_l_flags = (unsigned short) hdr_ptr[THR_07_OFFSET/2];
      temp_n_t_flags = (unsigned short) hdr_ptr[THR_08_OFFSET/2];
      
      
      retval = read_orpg_product_float( (void*) &temp_Scale,
                                           (float*) &params->Scale);
      retval = read_orpg_product_float( (void*) &temp_Offset,
                                           (float*) &params->Offset);

      /* note: the data structure can handle four bytes for max_level but  */
      /*       the design of the product limits it to two bytes            */
      params->pcode     = the_pcode;
      params->n_l_flags = temp_n_l_flags;
      params->n_t_flags = temp_n_t_flags;
      params->max_level = (unsigned int) temp_max_level;
   
      fprintf(stderr,"   Product %d, Scale is %f, Offset is %f \n"
                 "   max level is %d, n lead flags is %d, n trail flags is %d\n\n",
              params->pcode, params->Scale, params->Offset, params->max_level, 
              params->n_l_flags, params->n_t_flags);


   /*************************************************************************/
   } else if (decode_flag==FDECODE) {
      
      /* A. check to see if parameters already exist for this product */
        
      exist_params_found = FALSE;
      for(i=0; i<NUM_EXISTING_PARAMS; i++) {
         if(existing_params[i].pcode == the_pcode) {
            params->pcode     = existing_params[i].pcode;
            params->Scale     = existing_params[i].Scale;
            params->Offset    = existing_params[i].Offset;
            params->n_l_flags = existing_params[i].n_l_flags;
            params->n_t_flags = existing_params[i].n_t_flags;
            params->max_level = existing_params[i].max_level;
            exist_params_found = TRUE;
            break;
         } /* end if == the_pcode */
      } /* end for NUM_EXISTING_PARAMS */
   
      
      
      /* B. look for a user supplied decode parameter file for this pcode */
      
      sprintf(config_file, "%s/.cvt/decode_params.%d", home_dir, the_pcode);
      
      if(stat(config_file, &confdirstat) < 0) {
         /* user file for this product does not exist */
         file_params_found = FALSE;
      } else { /* file exists so use it! */
         file_params_found = TRUE;
      } /* end file exists so use it */

      if(file_params_found == TRUE) {
        
         file_params_found = read_params_from_file(config_file);
      
         if(file_params_found == FALSE) 
            fprintf(stderr,"ERROR reading user config file '%s'\n", config_file);
            
         else if(exist_params_found == TRUE)
            fprintf(stderr,
                 "   Overriding existing configuration with user config file \n"
                 "           '%s'\n\n", config_file);
         
      } /* end if file_params_found == TRUE */
      
      
      if(file_params_found == TRUE && exist_params_found == FALSE)
         fprintf(stderr,"   Using decode parameters from user config file \n"
                        "            '%s'\n\n", config_file);
      else if(file_params_found == FALSE && exist_params_found == TRUE) 
         fprintf(stderr,"   Using pre-configured decode parameters\n");
      
      
      if(exist_params_found == TRUE || file_params_found == TRUE) {
         fprintf(stderr,"   Product %d, Scale is %f, Offset is %f \n"
                 "   max level is %d, n lead flags is %d, n trail flags is %d\n\n",
              params->pcode, params->Scale, params->Offset, params->max_level, 
              params->n_l_flags, params->n_t_flags);
              
      } else { /* there are no file params for this product */
        
        return FALSE;
        
      }
    
   } /* end if FDECODE */
   /*************************************************************************/
    
    
   /* TO DO - test of Scale == 0.0 and Scale < 0.0 errors */
   /* Scale value 0.0 is not a valid value                */
   /* Scale value must be positive because by 
    * convention the decoded value must increase
    * with the raw data level
    */
   
    
   return TRUE;
    
} /* end get_decode_params */






/* CVT 4.4 */
int read_params_from_file(char *params_filename) {
    
  FILE *params_file=NULL;
  
  char buf[100];
  int   gen_error = FALSE;
  int   line_read=0;

  
  float        local_Scale, local_Offset;
  unsigned int local_max_level;
  unsigned short local_n_l_flags, local_n_t_flags;
  
    
    if((params_file = fopen(params_filename, "r")) == NULL) {
        fprintf(stderr, "ERROR Could not open configuration file: %s", 
                                                    params_filename);
        return FALSE;
    }


    
     /* 1. Skip comments and read Scale parameter for decoding */
     buf[0] = '#'; 
     while( buf[0] == '#' && feof(params_file) == 0 ) {
        read_to_eol(params_file, buf);
        line_read ++;
     }
     if(feof(params_file) == 0 && buf[0] != '\0' && buf[0] != ' ')
        sscanf(buf, "%f", &local_Scale);
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR "
                       "     unable to read SCALE value on line %d\n",line_read);
     }

     /* 2. Skip comments and read Offset parameter for decoding */
     buf[0] = '#'; 
     while( buf[0] == '#' && feof(params_file) == 0 ) {
        read_to_eol(params_file, buf);
        line_read ++;
     }
     if(feof(params_file) == 0 && buf[0] != '\0' && buf[0] != ' ')
        sscanf(buf, "%f", &local_Offset);
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR "
                       "     unable to read OFFSET value on line %d\n",line_read);
     }

    /* 3. Skip comments and read max data level */
     buf[0] = '#';
     while( buf[0] == '#' && feof(params_file) == 0 ) {
        read_to_eol(params_file, buf);
        line_read ++;
     }
     if(feof(params_file) == 0 && buf[0] != '\0' && buf[0] != ' ')
        sscanf(buf, "%u", &local_max_level);   
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR "
                       "     unable to read MAX LEVEL on line %d\n",line_read);;
     }
    
     /* 4. Skip comments and read number of leading flags */
     buf[0] = '#';
     while( buf[0] == '#' && feof(params_file) == 0 ) {
        read_to_eol(params_file, buf);
        line_read ++;
     }
     if(feof(params_file) == 0 && buf[0] != '\0' && buf[0] != ' ')
        sscanf(buf, "%hu", &local_n_l_flags);   
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR "
                       "     unable to read NUM L FLAGS on line %d\n",line_read);
     }
     
     /* 5. Skip comments and read number of trailing flags */
     buf[0] = '#';
     while( buf[0] == '#' && feof(params_file) == 0 ) { 
        read_to_eol(params_file, buf);
        line_read ++;
     }
     if(buf[0] != '\0' && buf[0] != ' ')
        sscanf(buf, "%hu", &local_n_t_flags);   
     else {
        gen_error = TRUE;
        fprintf(stderr,"\nERROR "
                       "     unable to read NUM T FLAGS on line %d\n",line_read);
     }
    
    fclose(params_file);
    
    /* protect against division by 0 */
    if(local_Scale == 0.0) {
        fprintf(stderr,"CONFIGURATION ERROR - Attempting to use Scale Offset \n"
                       "     for data level decoding with Scale value 0.0\n");
        return FALSE;
    }
    
    if(gen_error == TRUE) {
        return FALSE;
    
    } else {
        s_o_params.Scale = local_Scale;
        s_o_params.Offset = local_Offset;
        s_o_params.max_level = local_max_level;
        s_o_params.n_l_flags = local_n_l_flags;
        s_o_params.n_t_flags = local_n_t_flags;
        return TRUE;
        
    }
    
    
} /* end read_params_from_file */





/* CVT 4.4  */
void decode_level(unsigned int d_level, char *decode_val, decode_params_t *params,
                                                               int num_dec_spaces) {

double decode_real = 0;

int dec_places;
char  num_disp_format[10];



/* default local variables */
int   n_l_flags = 0;
int   n_t_flags = 0;
float   Scale = 1.0;
float   Offset = 0.0;
unsigned int max_d_lvl = 0; /* used to locate trailing flags */

    /* parameters read from global structure */
    n_l_flags = params->n_l_flags;
    n_t_flags = params->n_t_flags;
    max_d_lvl = params->max_level;
    Scale     = params->Scale;
    Offset    = params->Offset;
        
    /* protect against division by 0 */
    if(Scale == 0.0) {
        sprintf(decode_val, "  Error ");
        fprintf(stderr,"CONFIGURATION ERROR - Attempting to use Scale Offset \n"
                       "     for data level decoding with Scale value 0.0\n");
        return;
    }


if(num_dec_spaces != -1)
    dec_places = num_dec_spaces;
else 
    dec_places = DEFAULT_DEC_PLACES;

/* should 8 be changed to 10?? would require changes to print functions */
sprintf(num_disp_format,"%c%d.%df", 37, 8, dec_places);



    /* DECODING LOGIC - Flag values are not decoded via Scale Offset */
    if( (n_l_flags==0) && (n_t_flags==0) ) {
        decode_real = ( ((double)d_level) - ((double)Offset) ) / (double)Scale;
/*        sprintf(decode_val, "%8.4f", decode_real); */
        sprintf(decode_val, num_disp_format, decode_real);
        
    } else if( (n_l_flags!=0) && (d_level < n_l_flags) ) {
        /* return leading flag label */
        /* leading flags always begin with data level 0 */
        /* TO DO - REPLACE THE LABEL WITH "L Flag 1" , etc */
        sprintf(decode_val, " L_Flag%d", d_level + 1);
        
    } else if( (n_t_flags!=0) && (d_level > (max_d_lvl - n_t_flags)) ) {
        /* return trailing flag label */
        sprintf(decode_val, " T_Flag%d", d_level - max_d_lvl + n_t_flags);
        
    } else { /* not a flag value */
        decode_real = ( ((double)d_level) - ((double)Offset) ) / (double)Scale;
/*        sprintf(decode_val, "%8.4f", decode_real); */
        sprintf(decode_val, num_disp_format, decode_real);
        
    }
    
}  /* end decode_level */



