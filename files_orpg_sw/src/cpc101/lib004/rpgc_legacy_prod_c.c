/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/23 15:14:16 $
 * $Id: rpgc_legacy_prod_c.c,v 1.22 2013/08/23 15:14:16 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#include <rpgc.h>
#include <rpgcs.h>

#include <product.h>
#include <orpg.h>
#include <rpg_port.h>
#include <orpgpat.h>
#include <orpgsite.h>
#include <packet_16.h>
#include <packet_17.h>

/* Macro Definitions. */
#define MAX_CHARS_LINE     80
#define MAX_LINES_PAGE     16
#define MAX_CHARS_PAGE     (MAX_CHARS_LINE * MAX_LINES_PAGE)
#define MAX_PAGES          48

#define MAX_DEP_PARAMS     10
#define MAX_DATA_LEVELS    16

#define VOL_SPOT_BLANKED   0x80000000


/* Local function prototypes. */
static void Store_byte( int i, short *j, int k);
static void Padfront( int startix, int bstep, short *outbuf, int oidx, 
                      int *padcnt, int *strtdecr );
static void Padback( int byteflg, short *outbuf, int bstep, int pbuffind, 
                     int iendix, int numbins, int *finalidx );

/********************************************************************
   Description:
      C/C++ interface for building the legacy product header. 

   Inputs:
      ptr - pointer to legacy product buffer.
      prod_id - product ID.
      length - pointer to current length of product, in bytes.
               This length does not include the length of the 
               product header or product description block.
      
   Outputs:
      length - length of product, in bytes.

   Returns:
      Returns the status of the operation: 0 - successful, -1 -
      unsuccessful.
   
********************************************************************/
int RPGC_prod_hdr( void *ptr, int prod_id, int *length ){

   Graphic_product *prod_hdr;
   int prod_code, blocks;
   unsigned int offset = 0;

   /*
     Cast input pointer to Legacy_prod_hdr_t structure pointer.
   */
   prod_hdr = (Graphic_product *) ptr;

   /* Get the product code from the product ID. */
   if( (prod_code = ORPGPAT_get_code( prod_id )) < 0 ){

      LE_send_msg( GL_ERROR, "Product ID (%d) Does Not Have Product Code\n",
                   prod_id );
      return(-1 );

   }

   /* Fill in appropriate fields of the legacy product header. */
   prod_hdr->msg_code = (short) prod_code;
   *length += sizeof( Graphic_product);
   RPGC_set_product_int( (void *) &prod_hdr->msg_len, *length );

   /* Get the ID. */
   prod_hdr->src_id = ORPGSITE_get_int_prop( ORPGSITE_RPG_ID );
   if( ORPGSITE_error_occurred() ){

      ORPGSITE_clear_error();
      return( -1 );

   }

   /* Compute the number of blocks in this product.  Base on offsets in
      product description block. */
   blocks = 2;
   RPGC_get_product_int( (void *) &prod_hdr->sym_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   RPGC_get_product_int( (void *) &prod_hdr->gra_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   RPGC_get_product_int( (void *) &prod_hdr->tab_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   prod_hdr->n_blocks = (short) blocks;


   /* Return NORMAL. */
   return(0);
   
/* End of RPGC_prod_hdr( ) */
}

/********************************************************************
   
   Description:
      Sets the Symbology Block, Graphic Alphanumeric Block and 
      Tabular Block offsets in the Product Description Block.

   Inputs:
      ptr - pointer to product buffer.
      sym_offset - Symbology Block offset in number of shorts
                   from start of product header.
      gra_offset - Graphic Alphanumeric Block offset in number 
                   of shorts from start of product header.
      tab_offset - Tabular Block offset in number of shorts
                   from start of product header.

   Returns:
      Currently, always returns 0.

   Notes:
      If offset is negative, the offset is ignored.

********************************************************************/
int RPGC_set_prod_block_offsets( void *ptr, int sym_offset, 
                                 int gra_offset, int tab_offset ){

    Graphic_product *phd = (Graphic_product *) ptr;

    /* Set offset to product symbology block, graphic alphanumeric
       block and tabular alphanumeric block. */
    if( sym_offset >= 0 )
       RPGC_set_product_int( &phd->sym_off, sym_offset );

    if( gra_offset >= 0 )
       RPGC_set_product_int( &phd->gra_off, gra_offset );

    if( tab_offset >= 0 )
       RPGC_set_product_int( &phd->tab_off, tab_offset );

    return 0;

/* End of RPGC_set_prod_block_offsets() */
}


/********************************************************************
   Description:
      C/C++ interface for the building of the product description
      block.

   Inputs:
      ptr - pointer to legacy product buffer.
      prod_id - product ID.
      vol_num - current volume scan number, modulo 80.

   Returns:
      Status of the operation: 0 - successful, -1 - unsuccessful.

********************************************************************/
int RPGC_prod_desc_block( void *ptr, int prod_id, int vol_num ){

   Graphic_product *prod_hdr = NULL, *descrip_block = NULL;
   int prod_code, type, index;
   unsigned int vol_start_time, gen_time, gen_date, latitude, longitude;
   time_t cur_time;

   static Scan_Summary *summary = NULL;

   /* Check for valid pointer to product header/description block. */
   if( ptr == NULL ){

      /* Invalid pointer .... return error. */
      return(-1);

   }

   prod_hdr = descrip_block = (Graphic_product *) ptr;

   /* Set block divider.  This divides the product header from the
      product description block. */
   prod_hdr->divider = (short) 0xffff;

   /* First get the product code from the product ID. */
   if( (prod_code = ORPGPAT_get_code( prod_id )) < 0 ){

      LE_send_msg( GL_ERROR, "Product ID (%d) Does Not Have Product Code\n",
                   prod_id );

      return(-1);

   }

   /* Set latitude, longitude, and radar height in product description
      block. */
   latitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE );
   RPGC_set_product_int( (void *) &descrip_block->latitude, latitude );
   longitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE );
   RPGC_set_product_int( (void *) &descrip_block->longitude, longitude );
   descrip_block->height = ORPGSITE_get_int_prop( ORPGSITE_RDA_ELEVATION );
   if( ORPGSITE_error_occurred() ){

      ORPGSITE_clear_error();
      return(-1);

   }

   /* Set the product code. */
   descrip_block->prod_code = (short) prod_code;

   /* If for some reason scan summary data can not be read or is otherwise
      unavailable, return failure. */
   if( (summary = RPGC_get_scan_summary( vol_num )) == NULL ){

      LE_send_msg( GL_ERROR, "Unable To Read Scan Summary Data\n" );
      return(-1);

   }

   descrip_block->op_mode = (short) summary->weather_mode;
   descrip_block->vcp_num = (short) summary->vcp_number;
   descrip_block->vol_date = (short) summary->volume_start_date;

   vol_start_time = (unsigned int ) summary->volume_start_time;
   RPGC_set_product_int( (void *) &descrip_block->vol_time_ms, vol_start_time );

   /* Set the volume scan number. */
   descrip_block->vol_num = vol_num;

   /* Set the product generation date and time. */
   cur_time = time( NULL );
   gen_time = RPG_TIME_IN_SECONDS( cur_time );
   gen_date = RPG_JULIAN_DATE( cur_time );
   RPGC_set_date_time( (void *) &descrip_block->gen_time,
                       (void *) &descrip_block->gen_date,
                       gen_time, gen_date );

   /* A volume scan number of 0 indicates the initial volume scan.
      Substitute the volume scan start time and date with the
      generation time and date. */
   if( vol_num == 0 ){

      RPGC_set_product_int( (void *) &descrip_block->vol_time_ms, gen_time );
      descrip_block->vol_date = descrip_block->gen_date;
      descrip_block->vol_num = 80;

   }

   /* If product is of type TYPE_ELEVATION, set the elevation index
      and spot-blanking status.  If of type TYPE_VOLUME, just set the
      spot_blanking status. */
   if( (type = ORPGPAT_get_type( prod_id )) == TYPE_ELEVATION ){

      PS_get_current_elev_index( &index );
      descrip_block->elev_ind = index;

      if( summary->spot_blank_status & (1 << (31 - index)) )
         descrip_block->n_maps = 1;

      /* Check if this product was generated on a Supplemental Scan. */
      if( WA_supplemental_scans_allowed() ){

         /* Task is allowed to use supplemental scan.  Now determine
            is this product was generated from a supplemental scan. */
         if( VI_is_supplemental_scan( descrip_block->vcp_num, vol_num, index ) ){

            Prod_header *phd = NULL;
            unsigned int vtime = 0;
            int i, waveform = 0, rda_elev_ind = -1;

            short *elev_ind_tab = RPGCS_get_elev_index_table( descrip_block->vcp_num );
            
            /* The start of elevation time for products generated from the supplemental
               (SAILS) cut should be the start of surveillance split cut. */
            gen_time = gen_date = 0;
            rda_elev_ind = -1;
            if( elev_ind_tab != NULL ){

               /* Go through all elevations looking for match on 
                  RPG elevation index. */
               for( i = 0; i < ECUTMAX; i++ ){

                  if( elev_ind_tab[i] == descrip_block->elev_ind ){

                     /* This function assumes 0 indexed RDA elevation indices. */
                     waveform = RPGCS_get_elev_waveform( descrip_block->vcp_num, i );

                     /* Assumes the first cut of a split cut is always
                        the surveillance cut. */
                     if( waveform == VCP_WAVEFORM_CS ){

                        /* Get the RDA elevation index. */
                        rda_elev_ind = i;
                        break;

                     }

                  }

               }
 
               /* Free the elevation index table. */
               free( elev_ind_tab );

            }

            /* If the RDA elevation index is defined, get the elevation  
               start time from Accounting Data. */
            if( rda_elev_ind >= 0 ){

               RPGC_get_elev_time( RPGC_START_TIME, vol_num, rda_elev_ind,
                                   (int *) &gen_time, (int *) &gen_date );

               LE_send_msg( GL_INFO, "Start Time of Surveillance Cut for RDA Elev # %d\n",
                            rda_elev_ind );
               LE_send_msg( GL_INFO, "--->gen_time: %d, gen_date: %d\n", gen_time, gen_date );

            }

            /* Extract the elevation start time from the ORPG product header 
               if the date and time are not already defined. */
            if( (gen_time == 0) || (gen_date == 0) ){

               if( (OB_hd_info_set())
                          && 
                   (phd = OB_hd_info()) != NULL ){

                  gen_time = RPG_TIME_IN_SECONDS( phd->elev_t );
                  gen_date = RPG_JULIAN_DATE( phd->elev_t );

               }

            }

            RPGC_get_product_int( (void *) &descrip_block->vol_time_ms, &vtime );
            LE_send_msg( GL_INFO, "Setting New Volume Time for Supplemental Scan Product\n" );
            LE_send_msg( GL_INFO, "--->Was: date/time: %d/%d, Is: %d/%d\n",
                         descrip_block->vol_date, vtime, gen_date, gen_time );

            /* Set the volume start date/time to be elevation start date/time. */
            descrip_block->vol_date = (short) gen_date;
            RPGC_set_product_int( (void *) &descrip_block->vol_time_ms, gen_time );

         }

      }

   }
   else if( type == TYPE_VOLUME ){

      if( summary->spot_blank_status & VOL_SPOT_BLANKED )
         descrip_block->n_maps = 1;

   }

   /* Return NORMAL completion. */
   return(0);

/* End of RPGC_prod_desc_block( ) */
}

/********************************************************************
   Description:
      C/C++ interface for building of legacy standalone product.

   Input:
      ptr - pointer to start of product buffer.
      string - pointr to string to place in standalone product.
      length - pointer to int to receive length of product.

   Output:
      length - length of alphanumeric block of standalone product,
               in bytes.

   Returns:
      Status of the operation: 0 - successful, -1 - unsuccessful.

********************************************************************/
int RPGC_stand_alone_prod( void *ptr, char *string, int *length ){

   int line_num, page_num, line_length, i;
   unsigned int sym_off;
   char *start_of_tabular = NULL;
   char *start_string, *end_string;
   short *tabular = NULL, *number_pages;
   Graphic_product *hdr;

   /* Note: 80 characters is the line length limit.  1 character is
            needed for string terminator, the others are for padding. */
   typedef struct {

      int length;
      char string[84];

   } String_t;

   String_t *strings = NULL;

   /* Initialize the length of the tabular data. */
   *length = 0;

   /* Check if string has any data to build product. */
   if( (int) strlen(string) == 0 ){

      LE_send_msg( GL_ERROR, "Stand-alone Product Has No Data\n" );
      return(-1);

   }

   strings = (String_t *) malloc( sizeof( String_t ) * MAX_LINES_PAGE );
   if( strings == NULL ){

      LE_send_msg( GL_MEMORY, "Unable to Create Stand-alone Product\n" );
      return(-1);

   }

   /* Set the block divider in product buffer. */
   start_of_tabular = (char *) ptr + sizeof( Graphic_product );
   tabular = (short *) start_of_tabular;
   *tabular = (short) 0xffff;
   tabular++;

   /* Save address of buffer where number of pages is to be stored. */
   number_pages = tabular;
   tabular++;

   start_string = end_string = string;
   page_num = 0;

   /* Build pages of the stand-alone product. */
   while( (page_num < MAX_PAGES) && (*end_string != '\0') ){

      /* Build page lines. */
      line_num = -1;
      while( line_num < (MAX_LINES_PAGE-1) ){

         /* Traverse string until new line, end of string, or maximum number
            of characters per line encountered. */
         while( (*end_string != (char) '\n') &&
                (*end_string != (char) '\0') &&
                ((end_string - start_string) < MAX_CHARS_LINE) )
            end_string++;
         if( (line_length = (int) (end_string - start_string)) > 0 ){

            line_num++;
            memcpy( (void *) strings[line_num].string, (void *) start_string,
                    line_length );

            /* We pad the end of the line with blank characters. */
            if( line_length < MAX_CHARS_LINE ){

               char *end_line;

               end_line = strings[line_num].string + line_length;
               memset( (void *) end_line, (int) 0x20, (MAX_CHARS_LINE - line_length) );
               line_length = MAX_CHARS_LINE;

            }

            strings[line_num].length = line_length;

         }

         /* End of string encountered. */
         if( *end_string == '\0' )
            break;

         /* Skip the new line character in the line. */
         if( *end_string == '\n' )
            start_string = ++end_string;
         else
            start_string = end_string;

      }

      /* Transfer page lines to product buffer. */
      for( i = 0; i <= line_num; i++ ){

         *tabular =  (short) strings[i].length;
         tabular++;
         memcpy( (void *) tabular, (void *) strings[i].string,
                 (size_t) strings[i].length );

         tabular += ((strings[i].length+1) / sizeof(short));

      }

      /* Put in page divider. */
      if( line_num >= 0 ){

         *tabular = (short) 0xffff;
         tabular++;
         page_num++;

      }

   }

   /* Free malloced block. */
   free(strings);

   /* Validate the number of pages.  If valid, set the number of
      pages in the product. */
   if( page_num > MAX_PAGES ){

      LE_send_msg( GL_ERROR, "Standalone Product Too Many Pages (%d)\n",
                   number_pages );

      return(-1);

   }
   else
      *number_pages = page_num;


   /* Set length of tabular data in bytes. */
   *length = ((char *) tabular - start_of_tabular);

   /* Set the offset to the product message in the product 
      description block.  For some reason, offset to symbology
      is used for this purpose. */
   hdr = (Graphic_product *) ptr;
   sym_off = (int) sizeof( Graphic_product ) / sizeof( short );
   RPGC_set_product_int( (void *) &hdr->sym_off, sym_off );

   /* Return to caller. */
   return(0);

/* End of RPGC_stand_alone_prod( ) */
}

/********************************************************************
   Description:
      C/C++ interface for setting time fields in the Product 
      Description block.

   Input:
      atime  - pointer to location where time is to be stored.
      adate  - pointer to location where date is to be stored.
      the_time - time value to store, in seconds since midnight 
      the_date - date value to store, in Modified Julian.

   Returns:
      0 on success, -1 on error.

********************************************************************/
int RPGC_set_date_time( void *atime, void *adate, int the_time,
                        int the_date ){

   unsigned short *gtime = (unsigned short *) atime;
   unsigned short *gdate = (unsigned short *) adate;

   /* Check to make sure pointers are not NULL. */
   if( (gtime == NULL) || (gdate == NULL) )
      return 0;

   /* Set the time and date.  Assumes time is stored as MSW/LSW 
      format. */
   RPGC_set_product_int( (void *) gtime, the_time );
   *gdate = the_date;

   return 1;

/* End of RPGC_set_date_time(). */
}


/********************************************************************
   Description:
      C/C++ interface for setting product dependent parameters in
      the product description block.

   Input:
      ptr - pointer to start of product buffer. 
      params - pointer to array of product dependent parameters. 
               Currently, all 10 parameters are required, even if
               not defined.

   Returns:
      Status of the operation: 0 - successful, -1 - unsuccessful.

********************************************************************/
int RPGC_set_dep_params( void *ptr, short *params ){

   Graphic_product *prod = (Graphic_product *) ptr;
   int i;

   /* Transfer parameters to product description block. */
   for( i = 0; i < MAX_DEP_PARAMS; i++ ){

      switch( i ){

         case 0:
         {
            prod->param_1 = *params;
            break;
         }
         case 1:
         {
            prod->param_2 = *(params + 1);
            break;
         }
         case 2:
         {
            prod->param_3 = *(params + 2);
            break;
         }
         case 3:
         {
            prod->param_4 = *(params + 3);
            break;
         }
         case 4:
         {
            prod->param_5 = *(params + 4);
            break;
         }
         case 5:
         {
            prod->param_6 = *(params + 5);
            break;
         }
         case 6:
         {
            prod->param_7 = *(params + 6);
            break;
         }
         case 7:
         {
            prod->param_8 = *(params + 7);
            break;
         }
         case 8:
         {
            prod->param_9 = *(params + 8);
            break;
         }
         case 9:
         {
            prod->param_10 = *(params + 9);
            break;
         }

      }

   }

   return 0;

/* End of RPGC_set_dep_params( ) */
}

/***************************************************************************

   Description:
      This module is used to run-length encode data for raster-formatted 
      products.  It processes the input data grid one row at a time using 
      the Color Data Table to get the color levels for the data.  It stores 
      the run-length encoded data grid in the product output buffer.  This 
      module also does a boundary check to insure not writing over the 
      buffer boundary.  If there is not enough room to complete run-length 
      encoding, a3cm22 returns to the calling module with a buffer completion 
      status of incomplete (status=1).

   Inputs:
      nrows - Number of rows in data grid.
      ncols - Number of columns in data grid.
      inbuf - Input buffer address - used to reference input data grid.
      cltab - Product data levels adaptation data for the product.
      maxind - Maximum output buffer index that this module can store into. 
      obuffind - output buffer starting index.

   Outputs:
      outbuf - output buffer address used to specify output buffer location 
               to store a byte.
      istar2s - number of I*2 words stored in the product output buffer 
                by this module.
   
   Returns:
      0 - complete, 1 incomplete.

***************************************************************************/
int RPGC_raster_run_length( int nrows, int ncols, short *inbuf, short *cltab, 
                            int maxind, short *outbuf, int obuffind, 
                            int *istar2s ){
 
    /* Initialized data */
    static int runtab[15] = { 16,32,48,64,80,96,112,128,144,160,176,192,
	                      208,224,240 };

    /* Local variables */
    int byteflag, firstpix, sobuffind, maxrow_ln, run, row; 
    int bufstat, nrlew, column, oldpix, runcol, newpix, obfrind;

    obfrind = obuffind;
    maxrow_ln = ncols / 2 + 1;
    bufstat = 0;
    byteflag = 0;
    firstpix = 1;
    run = 0;
    oldpix = 0;
    newpix = 0;
    *istar2s = 0;
    row = 1;

    /* Do for all elements in the input buffer: */
L103:
    if( (row < nrows) && (obfrind <= (maxind - maxrow_ln + 1)) ) {

	sobuffind = obfrind;
	++obfrind;
	nrlew = 0;
	for( column = 0; column < ncols; ++column) {

            /* Perform color table look-up for this pixel. */
	    newpix = cltab[ inbuf[ (row - 1)*ncols + column ] ];

            /* First pixel in the row, or first pixel after a run of fifteen. */
	    if( firstpix ){

		run = 1;
		oldpix = newpix;
		firstpix = 0;

            /* Intermediate pixel. */
	    }
            else if( newpix == oldpix ) 
		++run;

            /* New color level --> store last run store on left. */
            else if( byteflag == 0 ){

		runcol = runtab[run - 1] + oldpix;
		Store_byte( runcol, &outbuf[obfrind], 0 );
		byteflag = 1;
		run = 1;
		oldpix = newpix;

            /* Store on right. */
	    }
            else{

		runcol = runtab[run - 1] + oldpix;
		Store_byte( runcol, &outbuf[obfrind], 1 );
		byteflag = 0;
		++obfrind;
		run = 1;
		oldpix = newpix;
		++nrlew;
		++(*istar2s);

	    }

            /* Run of fifteen pixels detected. */
	    if( run == 15 ){

		runcol = runtab[run - 1] + oldpix;
		if( byteflag == 0 ){

		    Store_byte( runcol, &outbuf[obfrind], 0 );
		    byteflag = 1;

		}
                else{

		    Store_byte( runcol, &outbuf[obfrind], 1 );
		    ++nrlew;
		    ++obfrind;
		    ++(*istar2s);
		    byteflag = 0;

		}

		run = 0;
		firstpix = 1;
	    }

	}

        /* End of row processing.  Store remaining pixels. */
	if( !firstpix ){

	    runcol = runtab[run - 1] + oldpix;
	    if( byteflag == 0 ){

		Store_byte( runcol, &outbuf[obfrind], 0 );
		Store_byte( 0, &outbuf[obfrind], 1);

	    }
            else{

		Store_byte( runcol, &outbuf[obfrind], 1 );
		byteflag = 0;

	    }

	    ++nrlew;
	    ++(*istar2s);
	    ++obfrind;

            /* If the last pixel of the row was the last element 
               in a run of fifteen, and that run-length encoded 
               byte was written to the left side of the 16-bit 
               i*2 word, then zero-pad the right side: */
	}
        else if( byteflag == 1 ){

	    Store_byte( 0, &outbuf[obfrind], 1 );
	    byteflag = 0;
	    ++nrlew;
	    ++(*istar2s);
	    ++obfrind;

	}

        /* Calculate RLE bytes for raster packet. */
	outbuf[sobuffind] = (short) (nrlew << 1);
	++(*istar2s);
	firstpix = 1;

        /* Next pixel will be the first pixel (of the new row). */
	++row;
	goto L103;

        /* Above GOTO used to simulate while loop */
    }

    if( row <= nrows ){

       /* Not enough buffer space for run-length encoding - product 
          incomplete. */
	bufstat = 1;
    }

    return bufstat;

/* End of RPGC_raster_run_length() */
} 

/***********************************************************************

   Description:
      Perform run-length encoding (see a3cm01.ftn)

   Inputs:
      start - start angle (deg*10).
      delta - delta angle (deg*10).
      input - input data to be run-length encoded.
      startix - index into "input" to start of data to run-length encode.
      endix - index into "input" to end of data to run-length encode.
      max_num_bins - maximum number of data bins,
      buffstep - number of words per entry in "inbuf".
      cltab - pointer to color table.
      buffind - product buffer index.
      outbuf - pointer to output buffer.

   Outputs:
      nrleb - number of run-length encoded bytes

   Returns:
      Currently always returns 0.
   
***********************************************************************/
int RPGC_run_length_encode_byte( int start, int delta, unsigned char *inbuf, 
                                 int startix, int endix, int max_num_bins, 
                                 int buffstep, short *cltab, int *nrleb, 
	                         int buffind, short *outbuf ){

   short *data = NULL;
   int i, end;

   /* Copy the data. */
   end = endix;
   if( (endix - startix + 1)/buffstep > max_num_bins )
      end = startix + (max_num_bins * buffstep) - 1;

   /* Allocate space for the input data. */
   data = calloc( 1, end*sizeof(short) );
   if( data == NULL ){

      LE_send_msg( GL_ERROR, "malloc Failed for %d Bytes\n",
                   end*sizeof(short) );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   for( i = startix; i <= end; i++ )
      data[i] = (short) inbuf[i];

   /* Perform the run-length encoding. */
   RPGC_run_length_encode( start, delta, data, startix, 
                           endix, max_num_bins, buffstep, 
                           cltab, nrleb, buffind, outbuf );

   /* Free space allocated to the copied data. */
   free( data );

   return( 0 );

/* End of RPGC_run_length_encode_byte(). */
}

/***********************************************************************

   Description:
      Perform run-length encoding (see a3cm01.ftn)

   Inputs:
      start - start angle (deg*10).
      delta - delta angle (deg*10).
      input - input data to be run-length encoded.
      startix - index into "input" to start of data to run-length encode.
      endix - index into "input" to end of data to run-length encode.
      max_num_bins - maximum number of data bins.
      buffstep - number of words per entry in "inbuf".
      cltab - pointer to color table.
      buffind - product buffer index.
      outbuf - pointer to output buffer.

   Outputs:
      nrleb - number of run-length encoded bytes

   Returns:
      Currently always returns 0.
   
***********************************************************************/
int RPGC_run_length_encode( int start, int delta, short *inbuf, 
                            int startix, int endix, int max_num_bins, 
                            int buffstep, short *cltab, int *nrleb, 
	                    int buffind, short *outbuf ){

    /* Initialized data */
    static int runtab[15] = { 16,32,48,64,80,96,112,128,144,160,176,192,
	                      208,224,240 };

    /* Local variables */
    static int ibuffind, byteflag, pbuffind, sbuffind, newindex, strtdecr;
    static int firstpix;
    static int nstartix, run, nrlew;
    static int padcnt, oldpix;
    static int runcol, newpix;
    static int i, j;

    /* Make start angle and delta assignments (first, save product   
       buffer index for future assignment in the product buffer   
       of the number of RLE words in this input buffer. */
    *nrleb = 0;
    pbuffind = sbuffind = buffind;
    ++pbuffind;

    outbuf[pbuffind] = (short) start;
    ++pbuffind;
    outbuf[pbuffind] = (short) delta;
    ++pbuffind;

    /* Pad the start of the runs with FOFO if there are missing data bins
       before the start of good data. */
    Padfront( startix, buffstep, outbuf, pbuffind, &padcnt, &strtdecr );
    pbuffind += padcnt;
    nstartix = startix - strtdecr;

    /* Initialize data for doing RLE of good data. */
    byteflag = 0;
    firstpix = 1;
    run = 0;
    oldpix = 0;
    newpix = 0;
    nrlew = 0;

    /* Process all the data designated as good data in radial. */
    i = endix;
    j = buffstep;
    for( ibuffind = nstartix; ibuffind <= endix; ibuffind += buffstep ){

        /* Perform color table look-up for this pixel. */
	newpix = cltab[ inbuf[ibuffind] ];

        /* First pixel in the input buffer, or first pixel after a run of 15. */
	if( firstpix ){

	    run = 1;
	    oldpix = newpix;
	    firstpix = 0;

	}
        else if( newpix == oldpix ){

            /* Intermediate pixel. */
	    ++run;

	}
        else if( byteflag == 0 ){

	    runcol = runtab[run - 1] + oldpix;
	    Store_byte( runcol, &outbuf[pbuffind], 0 );
	    byteflag = 1;
	    run = 1;
	    oldpix = newpix;

	}
        else{

	    runcol = runtab[run - 1] + oldpix;
	    Store_byte( runcol, &outbuf[pbuffind], 1 );
	    byteflag = 0;
	    ++pbuffind;
	    run = 1;
	    oldpix = newpix;

	}

        /* Run of 15 pixels detected. */
	if( run == 15 ){

	    runcol = runtab[run - 1] + oldpix;
	    if( byteflag == 0 ){

		Store_byte( runcol, &outbuf[pbuffind], 0 );
		byteflag = 1;

	    }
            else{

		Store_byte( runcol, &outbuf[pbuffind], 1 );
		++pbuffind;
		byteflag = 0;

	    }

	    run = 0;
	    firstpix = 1;

	}

    }

    /* End of input buffer processing.   Set up byteflags for Padback
       routine.  Flags and pointers point to next available storage 
       location. */
    if( !firstpix ){

	runcol = runtab[run - 1] + oldpix;
	if( byteflag == 0 ){

	    Store_byte( runcol, &outbuf[pbuffind], 0 );
	    byteflag = 1;

	}
        else{

	    Store_byte( runcol, &outbuf[pbuffind], 1 );
	    byteflag = 0;
	    ++pbuffind;

	}

    }

    /* Now do the end processing -- pack any needed runs of zero level to
       account for bins of missing data.  The index passed is the next 
       available one to use. */
    Padback( byteflag, outbuf, buffstep, pbuffind, endix, 
	     max_num_bins, &newindex );

    /* Now calculate the nrlev and nrlew. */
    nrlew = newindex - sbuffind - 3;

    /* Note that nrlew does not include the 3 words before the RLE starts,
       but these bytes are included in the byte count. */
    *nrleb = (newindex - sbuffind) * 2;

    /* Assign the number of RLE words to the appropriate position in the 
       radial header. */
    outbuf[sbuffind] = (short) nrlew;

    /* Completed RLE processing for this buffer. */
    return 0;

/* End of RPGC_run_length_encode() */
} 

/********************************************************************

   Description:
      Helper function for populating packet 17 data header.

   Inputs:
      lfm_boxes_in_row - number of LFM boxes per row.
      num_rows - number of rows.

   Outputs:
      output - pointer where the packet 17 header is populated.

   Returns:
      Number of bytes this radial.

   Notes:
      See RPG to Class 1 User ICD for more information.

********************************************************************/
int RPGC_digital_precipitation_data_hdr( int lfm_boxes_in_row, 
                                         int num_rows, void *output ){

   Packet_17_hdr_t *outbuf = (Packet_17_hdr_t *) output;

   outbuf->code = PACKET_17;
   outbuf->spare1 = 0;
   outbuf->spare2 = 0;
   outbuf->lfm_boxes_in_row = (short) lfm_boxes_in_row;
   outbuf->num_rows = (short) num_rows;

   return 0;

/* End of RPGC_digital_precipitation_data_hdr() */
}

/********************************************************************

   Description:
      Helper function for populating packet 16 data header.

   Inputs:
      first_bin_idx - index of the first range bin.
      num_bins - number of range bins.
      icenter - icenter of sweep.
      jcenter - jcenter of sweep.
      scale_factor - range scale factor.
      num_radials - number of radials in sweep.
    
   Outputs:
      output - pointer where the packet 16 header is populated.

   Returns:
      Number of bytes this radial.

   Notes:
      See RPG to Class 1 User ICD for more information.

********************************************************************/
int RPGC_digital_radial_data_hdr( int first_bin_idx, int num_bins,
                                  int icenter, int jcenter, 
                                  int scale_factor, int num_radials,
                                  void *output ){

   Packet_16_hdr_t *outbuf = (Packet_16_hdr_t *) output;

   outbuf->code = PACKET_16;
   outbuf->first_bin = (short) first_bin_idx;
   outbuf->num_bins = (short) num_bins;
   outbuf->icenter = (short) icenter;
   outbuf->jcenter = (short) jcenter;
   outbuf->scale_factor = (short) scale_factor;
   outbuf->num_radials = (short) num_radials;

   return 0;

/* End of RPGC_digital_radial_data_hdr() */
}

/********************************************************************

   Description:
      Helper function for populating packet 16 data.

   Inputs:
      input - pointer to start of input data array.
      input_size - either RPGC_BYTE_DATA, RPGC_SHORT_DATA or
                   RPGC_INT_DATA.
      start_data_idx - index of first range bin in input.
      end_data_idx - index of last range bin in input.
      start_idx - index of first range bin in product.
      num_rad_bins - number of radial bins in product.
      radstep - bin step size.
      start_angle - radial leading edge azimuth (deg*10).
      delta_angle - radial width (deg*10).

   Outputs:
      output - pointer to where packet 16 is to be placed

   Returns:
      Number of bytes this radial (i.e., sizeof(Packet_16_data_t)
      data structure including all data) or -1 on error.

   Notes:
      See RPG to Class 1 User ICD for more information.

********************************************************************/
int RPGC_digital_radial_data_array( void *input, int input_size, 
                                    int start_data_idx, int end_data_idx, 
                                    int start_idx, int num_rad_bins,
                                    int binstep, int start_angle, 
                                    int delta_angle, void *output ){

   int switch_lr, bin, nbytes, ix_outbuf;
   unsigned char *data;
   Packet_16_data_t *outbuf;

   /* Verify output is valid pointer. */
   if( output == NULL )
      return -1;

   outbuf = (Packet_16_data_t *) output;
   data = (unsigned char *) &outbuf->data;

   /* Initialization .... */
   ix_outbuf = 0;
   switch_lr = 0;

   /* Packet header initialization. */
   outbuf->start_angle = (short) start_angle;
   outbuf->delta_angle = (short) delta_angle;

   /* Do For All bins ....... */
   for( bin = start_idx; bin < num_rad_bins; bin += binstep ){

      /* Outside boundaries of the data. Put in 0. */
      if( (bin < start_data_idx) || (bin > end_data_idx) )
         *data = 0;

      else{

         /* Within boundaries of the data. */
         if( input_size == RPGC_BYTE_DATA )
            *data = ((unsigned char *) input)[bin];

         else if( input_size == RPGC_SHORT_DATA ){

            short temp =  (short) ((short *) input)[bin];
        
            if( (temp < 0) || (temp > 256) )
               *data = 0;

            else
               *data = (unsigned char) temp;

         }
         else if( input_size == RPGC_INT_DATA ){

            int temp = (int) ((int *) input)[bin];

            if( (temp < 0) || (temp > 256) )
               *data = 0;

            else
               *data = (unsigned char) temp;

         }

      }

      /* Store pix_value in proper byte position. */
      if( switch_lr == 0 )
         switch_lr = 1;

      else{

         ++ix_outbuf;
         switch_lr = 0;

      }

      data++;

   }

   /* If the last bin falls on an even byte boundary, pad with 0. */
   if( switch_lr == 1 ){

      *data = 0;
      ++ix_outbuf;

   }

   /* Calculate the number of bytes this radial. */
   nbytes = ix_outbuf*sizeof(unsigned short);
   outbuf->size_bytes = nbytes;

   nbytes += sizeof(Packet_16_data_t) - sizeof(unsigned short);
   return nbytes;

/* End of RPGC_digital_radial_data_array() */
}

/*\////////////////////////////////////////////////////////////////////

    Description:
       Convenience function for filling in RPG/Associated User ICD packet
       16.  This function only fills in the radial data portion.  Packet
       16 header must be filled in by the user.

    Inputs:
       packet - pointer to buffer to hold the radial data.
       start_angle - radial start angle (deg*10)
       angle_delta - radial angle delta (deg*10)
       data - radial data
       num_values - number of items in radial data.

    Returns:
       Function returns number of bytes in this portion of packet.
       Includes bytes associated with start and delta angles, as well
       as number of 8-bit values field.

    Notes:
       This function may still be called by some algorithms.   This
       function assumes the data is passed as BYTE values and the data 
       starts at index 0.

////////////////////////////////////////////////////////////////////\*/
int RPGP_set_packet_16_radial( unsigned char *packet, short start_angle,
                               short angle_delta, unsigned char *data,
                               int num_values ){

   int num_bytes = 0;

   /* Verify the packet boundary is on short boundary. */
   if( (((unsigned int) packet) % sizeof(short)) != 0 ){

      LE_send_msg( GL_ERROR, "Packet boundary not on short boundary.\n" );
      return( num_bytes );

   }

   /* Assume the input data and product data start and end at the same
      bin number and the bin step size is 1. */
   num_bytes = RPGC_digital_radial_data_array( data, RPGC_BYTE_DATA, 0,
                                               num_values-1, 0, num_values,
                                               1, start_angle, angle_delta,
                                               (void *) packet );

   /* Check if error occurred.  If error, return 0 (no bytes). */
   if( num_bytes < 0 )
      return 0;

   num_bytes -= (sizeof(Packet_16_data_t) - sizeof(short));
   return num_bytes;

/* End of RPGP_set_packet_16_radial(). */
}


/********************************************************************

   Description:
      Helper function for populating packet 17 data.

   Inputs:
      input - pointer to start of input data array.
      input_size - either RPGC_BYTE_DATA, RPGC_SHORT_DATA or
                   RPGC_INT_DATA.
      lfm_boxes_per_row - number of LFM boxes per row.
      num_row - number of rows.

   Outputs:
      output - pointer to where packet 17 is to be placed

   Returns:
      Number of bytes data encoded (i.e., Packet_17_data_t 
      with all data included) or -1 on error

   Notes:
      See RPG to Class 1 User ICD for more information.

       Array input is assumed stored input[num_row][lfm_boxes_in_row].

********************************************************************/
int RPGC_digital_precipitation_data_array( void *input, int input_size,
                                           int lfm_boxes_in_row,
                                           int num_rows, void *output ){

    int firstpix, col, run, row, nrlew, nrlei2, obfrind, tsize;
    short oldpix, newpix;

    Packet_17_hdr_t *hdr = (Packet_17_hdr_t *) output;
    Packet_17_data_t *data_hdr = 
            (Packet_17_data_t *) ((char *) hdr) + sizeof(Packet_17_hdr_t);
    short *data = (short *) &data_hdr->data;

    /* Verify output is valid pointer. */
    if( output == NULL )
        return -1;

    /* Initialize run length encoding variables. */
    obfrind = 0;
    firstpix = 1;
    run = 0;
    oldpix = 0;
    newpix = 0;
    nrlei2 = 0;
    row = 0;

    /* Do For All rows ....... */
    for( row = 0; row < num_rows; row++ ){

        /* Some column initialization. */
        nrlew = 0;

        /* Do For All columns ...... */
        for( col = 0; col < lfm_boxes_in_row; col++ ){

            /* Check input data size and extract according to size. */
            if( input_size == RPGC_BYTE_DATA )
                newpix = ((unsigned char *) input)[row*num_rows + col];

            else if( input_size == RPGC_SHORT_DATA ){

                short temp =  (short) ((short *) input)[row*num_rows + col];

                if( (temp < 0) || (temp > 256) )
                    newpix = 0;

                else
                    newpix = (unsigned char) temp;

            }
            else if( input_size == RPGC_INT_DATA ){

                int temp = (int) ((int *) input)[row*num_rows + col];

                if( (temp < 0) || (temp > 256) )
                    newpix = 0;

                else
                    newpix = (unsigned char) temp;

            }

            /* first pixel in the row, or first pixel after a run of fifteen. */
            if( firstpix ){

                run = 1;
                oldpix = newpix;
                firstpix = 0;

            /* intermediate pixel. */
            }
            else if( newpix == oldpix ){

                ++run;

            /* New color level --> store last run store run count on left
               & color level on right */
            }
            else {

                Store_byte( run, &data[obfrind], 0 );
                Store_byte( oldpix, &data[obfrind], 1 );
                ++obfrind;
                run = 1;
                oldpix = newpix;

                /* Increment count of RLE'd words */
                ++nrlew;
                ++nrlei2;

            }

            /* Run of 255 pixels detected. */
            if( run == 255 ){

                Store_byte( run, &data[obfrind], 0 );
                Store_byte( oldpix, &data[obfrind], 1 );
                ++obfrind;
                run = 0;

                /* Increment count of RLE'd words */
                ++nrlew;
                ++nrlei2;
                firstpix = 1;

            }

        } /* End of for( col = 0; ..... */

        /* End of row in grid... */
        Store_byte( run, &data[obfrind], 0 );
        Store_byte( oldpix, &data[obfrind], 1 );
        ++obfrind;
        run = 0;

        /* Increment count of RLE'd words */
        ++nrlew;
        ++nrlei2;
        firstpix = 1;

        /* Calculate Run-length-encoded bytes for raster packet. */
        data_hdr->bytes_per_row = (short) (nrlew*2);
        ++nrlei2;
        firstpix = 1;

        /* Prepare for next row. */
        data_hdr = (Packet_17_data_t *) 
                   ((char *) data_hdr + data_hdr->bytes_per_row);

    } /* End of for( row = 0; ...... */

    /* Compute the size of the packet.  First the number of bytes to
       encode the data. */
    tsize = nrlei2*sizeof(short);

    /* Add the size of the Packet 17 data header. */
    tsize += (sizeof(Packet_17_data_t) - sizeof(short));

    /* Return the number of bytes used to encode the data. */
    return( tsize );

/* End of RPGC_digital_precipitation_data_array() */
}


/********************************************************************

   Description:
      This function stores the right byte of i into the kth byte of
      argument j. k = 0 for the left byte of first halfword; k = 1
      for the right byte of first halfword; k = 2 for the left byte
      of second halfword; and so on.

   Inputs:
      i - the byte value to store. 
      k - kth byte of j.
      
   Outputs:
      j - the value i is stored at position k.

   Returns:     
      There is no return value.

*********************************************************************/
static void Store_byte( int i, short *j, int k){

   int ind = k >> 1;

   if ((k % 2) == 0)
      j[ind] = ((i & 0xff) << 8) | (j[ind] & 0xff);

   else
      j[ind] = (i & 0xff) | (j[ind] & 0xff00);

/* End of Store_byte() */
}

/********************************************************************

   Description:

   Inputs:
      startix - index of beginning of output buffer for which this 
                radial's run-length encoded data will be placed. 
                (assumes zero indexed array).
      bstep - Number of words per entry in output buffer.
      outbuf - pointer to output buffer.
      oidx - index of output buffer to where padding at the beginning
             of the radial begins (assumes zero indexed array).

   Outputs:
      padnct - number of padded runs. 
      strtdecr - number of bins to subtract from start bin after
                 padding with runs of 0.
   Returns:
      Always returns 0.

********************************************************************/
static void Padfront( int startix, int bstep, short *outbuf, int oidx, 
                      int *padcnt, int *strtdecr ){

    /* Initialized data */
    static short f0f0 = 61680;
    static short todd_run[29] = { 4096,4112,8208,12304,16400,20496,24592, 28688,
                                  32784,36880,40976,45072,49168, 53264,57360,
                                  61456,61472,61488, 61504, 61520,61536,61552,
                                  61568,61584,61600, 61616,61632,61648,61664 };

    /* Local variables */
    int odd_bins, padd_run, i, f0_bins, f0_runs;

    *strtdecr = 0;
    *padcnt = 0;

    /* Calculate number of runs of 30 and the odd bins of missing data. */
    if( startix > 0 ){

        f0_bins = startix - 1;

        /* Calculate number of groups depending upon binstep. */
	*strtdecr = f0_bins % bstep;
	f0_bins /= bstep;
	f0_runs = f0_bins / 30;
	odd_bins = f0_bins % 30;
	padd_run = 0;

	if( odd_bins > 0 ){

	    if( odd_bins == 1 ){

                /* Set flag for caller to decrement starting bin by 1.  To
                   make an even number of runs of 30 of missing data. */
		*strtdecr += (odd_bins * bstep);

            }
	    else
		padd_run = todd_run[odd_bins - 1];
	    
	}

	for( i = 1; i <= f0_runs; ++i) 
	    outbuf[oidx + i - 1] = f0f0;

	*padcnt = f0_runs;
	if( padd_run != 0 ){

            /* Store odd runs and increment count. */
	    outbuf[oidx + f0_runs] = (short) padd_run;
	    ++(*padcnt);

	}

    }

/* End of Padfront() */
} 

/***************************************************************************
   Description:
      Pack runs of zero level for bins of missing data.

   Inputs:
      byteflag - padding character byte flag.
      bstep - number of words per entry in buffer.
      pbuffind - index in output buffer where back padding zeros are
                 to be placed (assumes zero indexed array ).
      iendix - index for last data bin in output buffer (assumes zero 
               indexed array).
      numbins - number of data bins.
      
   Outputs:
      outbuf - output buffer containing runs of zero level for missing data.
      finalidx - pointer to next available word in output buffer.

***************************************************************************/
static void Padback( int byteflg, short *outbuf, int bstep, int pbuffind, 
                     int iendix, int numbins, int *finalidx ){

    /* Initialized data */
    static int tpad[15] = { 16,32,48,64,80,96,112,128,144,160,176,192,208, 224,240 };
    static short todd_run[29] = { 4096,4112,8208,12304,16400,20496,24592,28688,
                                  32784,36880,40976,45072,49168,53264,57360,61456,
                                  61472,61488,61504,61520,61536,61552,61568,61584,
                                  61600,61616,61632,61648,61664 };

    /* Local variables */
    int odd_runs, i, traln_bins, pad;
    int f0_runs;

    /* Calculate the number of missing data bins on end.  Subtract 1 from
       "numbins" to account for unit versus zero indexing (iendix assumes
       zero indexing). */
    traln_bins = ((numbins-1) - iendix) / bstep;

    /* Determine if we need to pad one byte after good data. */
    *finalidx = pbuffind;
    if( byteflg == 1 ){

	pad = 0;
	if( traln_bins > 0 ){

	    if( traln_bins >= 15 ){

		pad = 240;
		traln_bins += -15;

	    }
            else{

		pad = tpad[traln_bins - 1];
		traln_bins = 0;

	    }

	}

	Store_byte( pad, &outbuf[*finalidx], 1 );
	++(*finalidx);

    }

    /* Now store the remaining bins of 0 data. */
    if( traln_bins != 0 ){

        /* Calculate runs of 30 and odd bins. */
	f0_runs = traln_bins / 30;
	odd_runs = traln_bins % 30;

        /* Store all complete runs of 30. */
	for( i = 1; i <= f0_runs; ++i )
	    outbuf[*finalidx + i - 1] = 61680;

        /* Update buffer indexes. */
	*finalidx += f0_runs;

        /* Table look-up for any odd runs. */
	if( odd_runs > 0 ){

	    outbuf[*finalidx] = todd_run[odd_runs - 1];

            /* Update index. */
	    ++(*finalidx);

	}

    }

/* End of Padback() */
} 
