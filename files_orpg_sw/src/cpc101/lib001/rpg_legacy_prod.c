/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/05/18 21:25:34 $
 * $Id: rpg_legacy_prod.c,v 1.14 2012/05/18 21:25:34 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

#include <product.h>
#include <orpg.h>
#include <rpg_port.h>
#include <orpgpat.h>
#include <rpg.h>
#include <orpgsite.h>

/* 
  Macro Definitions.
*/
#define MAX_CHARS_LINE     80
#define MAX_LINES_PAGE     16
#define MAX_CHARS_PAGE     (MAX_CHARS_LINE * MAX_LINES_PAGE)
#define MAX_PAGES          48

#define MAX_DEP_PARAMS     10
#define MAX_DATA_LEVELS    16

#define VOL_SPOT_BLANKED   0x80000000

/*\//////////////////////////////////////////////////////////
//
//   Description:
//      Given a pointer to a product buffer, fill in the 
//      appropriate field of the legacy product header.
//
//   Inputs:
//      ptr - pointer to legacy product buffer.
//      prod_id - pointer product ID.
//      length - pointer to length of product, in bytes.  Does 
//               not include length of product header and
//               product description block.
//      status - pointer to status of operation.
//
//   Outputs:
//      status - receives the status of the operation.  
//               0 - Successful, -1 - Unsuccessful.
//      length - total length of product, in bytes.
//
//   Returns:
//      There is no return value defined for this function.
//
//   Notes:
//      The pointer "ptr" must be fullword aligned.
//
//////////////////////////////////////////////////////////\*/
void RPG_prod_hdr( void *ptr, int *prod_id, int *length,
                   int *status ){

   Graphic_product *prod_hdr;
   int prod_code, blocks;
   unsigned int offset = 0;

   /*
     Cast input pointer to Legacy_prod_hdr_t structure pointer.
   */
   prod_hdr = (Graphic_product *) ptr;

   /*
     Initialize status to NORMAL.
   */
   *status = 0;

   /*
     Get the product code from the product ID.
   */
   if( (prod_code = ORPGPAT_get_code( *prod_id )) < 0 ){

      LE_send_msg( GL_ERROR, "Product ID (%d) Does Not Have Product Code\n",
                   *prod_id );
      *status = -1;
      return;

   }

   /*
     Fill in appropriate fields of the legacy product header.
   */
   prod_hdr->msg_code = (short) prod_code;
   *length += sizeof( Graphic_product);
   RPG_set_product_int( (void *) &prod_hdr->msg_len, (void *) length );

   /* 
     Get the ID.
   */
   prod_hdr->src_id = ORPGSITE_get_int_prop( ORPGSITE_RPG_ID );
   if( ORPGSITE_error_occurred() ){

      ORPGSITE_clear_error();
      *status = -1;
      return;

   }

   /*
     Compute the number of blocks in this product.  Base on offsets in 
     product description block.
   */
   blocks = 2;
   RPG_get_product_int( (void *) &prod_hdr->sym_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   RPG_get_product_int( (void *) &prod_hdr->gra_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   RPG_get_product_int( (void *) &prod_hdr->tab_off, (void *) &offset );
   if( offset != 0 )
      blocks++;

   prod_hdr->n_blocks = (short) blocks;


   /*
     Return NORMAL completion.
   */
   return;

/* End of RPG_prod_hdr( ) */
}


/*\///////////////////////////////////////////////////////////////
//
//   Description:
//      Fills in fields of the product description block.  The
//      only fields it does not fill in are the thresh levels,
//      product dependent parameters, version, and offsets 
//      (symbology, graphic attribute, and tabular).
//
//   Inputs:
//      ptr - pointer to start of product buffer.
//      prod_id - pointer to int containing product ID.
//      vol_num - pointer to int containing volume scan
//                number (modulo 80).
//      status - pointer to status of operation.
//
//   Outputs:
//      status - receives the status of the operation.  
//               0 - Successful, -1 - Unsuccessful.
//   Returns:
//      There is no return value define for this function.
//
//   Notes:
//
/////////////////////////////////////////////////////////////////\*/
void RPG_prod_desc_block( void *ptr, int *prod_id, int *vol_num,
                          int *status ){

   Graphic_product *prod_hdr = NULL, *descrip_block = NULL;
   int prod_code, type, index;
   unsigned int vol_start_time, gen_time, latitude, longitude;
   short gen_date;
   time_t cur_time;

   static Summary_Data *summary = NULL;

   /* Check for valid pointer to product header/description block. */
   if( ptr == NULL ){

      /* Invalid pointer .... return error. */
      *status = -1;
      return;

   }

   prod_hdr = descrip_block = (Graphic_product *) ptr;

   /*
     Initialize the status to NORMAL.
   */
   *status = 0;

   /*
     Set block divider.  This divides the product header from the 
     product description block
   */
   prod_hdr->divider = (short) 0xffff;

   /* 
     First get the product code from the product ID.
   */
   if( (prod_code = ORPGPAT_get_code( *prod_id )) < 0 ){

      LE_send_msg( GL_ERROR, "Product ID (%d) Does Not Have Product Code\n",
                   *prod_id );

      *status = -1;
      return;

   }

   /*
     Set latitude, longitude, and radar height in product description
     block.
   */
   latitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LATITUDE );
   RPG_set_product_int( (void *) &descrip_block->latitude, (void *) &latitude );
   longitude = ORPGSITE_get_int_prop( ORPGSITE_RDA_LONGITUDE );
   RPG_set_product_int( (void *) &descrip_block->longitude, (void *) &longitude );
   descrip_block->height = ORPGSITE_get_int_prop( ORPGSITE_RDA_ELEVATION );
   if( ORPGSITE_error_occurred() ){

      ORPGSITE_clear_error();
      *status = -1;
      return;

   }

   /*
     Set the product code. 
   */
   descrip_block->prod_code = (short) prod_code; 

   /* 
     Read scan summary for VCP number, volume date, volume time,
     and operational mode.

     Note: We first have to register the scan summary buffer if it is
     not already registered.  Then we must read the scan summary data.
   */
   if( (summary = (Summary_Data *) SS_get_summary_data()) == NULL ){

      summary = (Summary_Data *) malloc( sizeof( Summary_Data ) );
      if( summary == NULL ){

         LE_send_msg( GL_MEMORY, "malloc Failed For %d Bytes\n",
                      sizeof( Summary_Data ) );
         *status = -1;
         return;

      }

      SS_send_summary_array( (int *) summary );   
      RPG_read_scan_summary();

   }
   else{

      RPG_read_scan_summary();
      summary = (Summary_Data *) SS_get_summary_data();

   }

   /*
     If for some reason scan summary data can not be read or is otherwise
     unavailable, return failure.
   */
   if( summary == NULL ){

      LE_send_msg( GL_ERROR, "Unable To Read Scan Summary Data\n" );
      *status = -1;
      return;
      
   }

   descrip_block->op_mode = (short) summary->scan_summary[ *vol_num].weather_mode;
   descrip_block->vcp_num = (short) summary->scan_summary[ *vol_num ].vcp_number;
   descrip_block->vol_date = (short) summary->scan_summary[ *vol_num ].volume_start_date;

   vol_start_time = (unsigned int ) summary->scan_summary[ *vol_num ].volume_start_time;
   RPG_set_product_int( (void *) &descrip_block->vol_time_ms, (void *) &vol_start_time );

   /* 
     Set the volume scan number.
   */
   descrip_block->vol_num = *vol_num;

   /*
     Set the product generation date and time.
   */
   cur_time = time( NULL );
   gen_time = RPG_TIME_IN_SECONDS( cur_time );
   RPG_set_product_int( (void *) &descrip_block->gen_time, (void *) &gen_time );
   descrip_block->gen_date = RPG_JULIAN_DATE( cur_time );

   /*
     A volume scan number of 0 indicates the initial volume scan.
     Substitute the volume scan start time and date with the 
     generation time and date.
   */
   if( *vol_num == 0 ){

      vol_start_time = descrip_block->gen_time;
      RPG_set_product_int( (void *) &descrip_block->vol_time_ms, (void *) &vol_start_time );
      descrip_block->vol_date = descrip_block->gen_date;

      descrip_block->vol_num = 80;

   }

   /*
     If product is of type TYPE_ELEVATION, set the elevation index
     and spot-blanking status.  If of type TYPE_VOLUME, just set the 
     spot_blanking status.
   */
   if( (type = ORPGPAT_get_type( *prod_id )) == TYPE_ELEVATION ){

      PS_get_current_elev_index( &index );
      descrip_block->elev_ind = index;
      
      if( summary->scan_summary[ *vol_num ].spot_blank_status & (1 << (31 - index)) )
         descrip_block->n_maps = 1;

            /* Check if this product was generated on a Supplemental Scan. */
      if( WA_supplemental_scans_allowed() ){

         /* Task is allowed to use supplemental scan.  Now determine
            is this product was generated from a supplemental scan. */
         if( VI_is_supplemental_scan( descrip_block->vcp_num, *vol_num, index ) ){

            Prod_header *phd = NULL;
            unsigned int vtime = 0;

            /* Extract the elevation start time from the ORPG product 
               header. */
            if( OB_hd_info_set()
                       &&
                (phd = OB_hd_info()) != NULL ){

               gen_time = RPG_TIME_IN_SECONDS( phd->elev_t );
               gen_date = RPG_JULIAN_DATE( phd->elev_t );

               RPG_get_product_int( (void *) &descrip_block->vol_time_ms, (void *) &vtime );
               LE_send_msg( GL_INFO, "Setting New Volume Time for Supplemental Scan Product\n" );
               LE_send_msg( GL_INFO, "--->Was: date/time: %d/%d, Is: %d/%d\n",
                            descrip_block->vol_date, vtime, gen_date, gen_time );

               /* Set the volume start date/time to be elevation start date/time. */
               descrip_block->vol_date = (short) gen_date;
               RPG_set_product_int( (void *) &descrip_block->vol_time_ms, (void *) &gen_time );

            }

         }

      }

   }
   else if( type == TYPE_VOLUME ){

      if( summary->scan_summary[ *vol_num ].spot_blank_status & VOL_SPOT_BLANKED )
         descrip_block->n_maps = 1;

   } 

   /*
     Return NORMAL completion.
   */
   return;

/* End of RPG_prod_desc_block() */
}

/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      This module takes a pointer to a product buffer and a character
//      string and builds the alphanumeric part of a Stand-alone
//      alphanumeric product message (see RPG/PUP ICD for format).
//
//      This function returns to length of the tabular portion of the
//      product message.
//
//      If the initial string is empty, or too many pages required to
//      build the product, or memory allocation failure, the status
//      value receives -1.  Otherwise, it receives 0.
//
//   Inputs:
//      ptr - pointer to start of product buffer.
//      string - pointer to string to put in alphanumeric product.
//      length - pointer to int to receive length of product.
//      status - pointer to int to receive status of operation.
//
//   Outputs:
//      length - length in bytes of the tabular portion of the 
//               product message.
//      status - status of the operation (-1 = failure, 0 = success).
//
//   Returns:
//      There is no return value define for this function.
//
/////////////////////////////////////////////////////////////////////\*/
void RPG_stand_alone_prod( void *ptr, char *string, int *length, 
                           int *status ){

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

   /*
     Initialize the length of the tabular data.
   */
   *length = 0;

   /*
     Check if string has any data to build product. 
   */
   if( (int) strlen(string) == 0 ){

      LE_send_msg( GL_ERROR, "Stand-alone Product Has No Data\n" );
      *status = -1;
      return;

   }

   strings = (String_t *) malloc( sizeof( String_t ) * MAX_LINES_PAGE );
   if( strings == NULL ){

      LE_send_msg( GL_MEMORY, "Unable to Create Stand-alone Product\n" );
      *status = -1;
      return;

   }

   /* 
     Set the block divider in product buffer.  
   */
   start_of_tabular = (char *) ptr + sizeof( Graphic_product );
   tabular = (short *) start_of_tabular;
   *tabular = (short) 0xffff;
   tabular++;

   /* 
     Save address of buffer where number of pages is to be stored.
   */
   number_pages = tabular;
   tabular++;

   start_string = end_string = string;
   page_num = 0;

   /*
     Build pages of the stand-alone product.
   */
   while( (page_num < MAX_PAGES) && (*end_string != '\0') ){ 

      /* 
        Build page lines.
      */
      line_num = -1;
      while( line_num < (MAX_LINES_PAGE-1) ){

         /*
           Traverse string until new line, end of string, or maximum number
           of characters per line encountered.
         */
         while( (*end_string != (char) '\n') && 
                (*end_string != (char) '\0') && 
                ((end_string - start_string) < MAX_CHARS_LINE) )
            end_string++; 

         if( (line_length = (int) (end_string - start_string)) > 0 ){

            line_num++;
            memcpy( (void *) strings[line_num].string, (void *) start_string, 
                    line_length );

            /*
              We pad the end of the line with blank characters.
            */
            if( line_length < MAX_CHARS_LINE ){
     
               char *end_line;

               end_line = strings[line_num].string + line_length;
               memset( (void *) end_line, (int) 0x20, (MAX_CHARS_LINE - line_length) );
               line_length = MAX_CHARS_LINE;

            }

            strings[line_num].length = line_length;

         }

         /*
           End of string encountered.
         */
         if( *end_string == '\0' )
            break;

         /*
           Skip the new line character in the line.
         */
         if( *end_string == '\n' )
            start_string = ++end_string;
         else
            start_string = end_string;

      }

      /*
        Transfer page lines to product buffer.
      */
      for( i = 0; i <= line_num; i++ ){

         *tabular =  (short) strings[i].length;
         tabular++;
         memcpy( (void *) tabular, (void *) strings[i].string, 
                 (size_t) strings[i].length );

         tabular += ((strings[i].length+1) / sizeof(short));

      }

      /* 
        Put in page divider.
      */
      if( line_num >= 0 ){

         *tabular = (short) 0xffff;
         tabular++;
         page_num++;

      }

   }

   /*
     Free malloced block.
   */
   free(strings);
      
   /*
     Validate the number of pages.  If valid, set the number of
     pages in the product.
   */
   if( page_num > MAX_PAGES ){

      LE_send_msg( GL_ERROR, "Standalone Product Too Many Pages (%d)\n",
                   number_pages );

      *status = -1;
      return;

   }
   else
      *number_pages = page_num;


   /*
     Set length of tabular data in bytes.
   */
   *length = ((char *) tabular - start_of_tabular);

   /*
     Set the offset to the product message in the product
     description block.  For some reason, offset to symbology
     is used for this purpose.
   */
   hdr = (Graphic_product *) ptr;
   sym_off = (int) sizeof( Graphic_product ) / sizeof( short );
   RPG_set_product_int( (void *) &hdr->sym_off, (void *) &sym_off );

   /*
     Return to caller.
   */
   return;

/* End of RPG_stand_alone_prod( ) */
}

/*\/////////////////////////////////////////////////////////////
//
// Description:
//    This module sets the product dependent parameters in the
//    product description block assuming legacy product format.
//
// Inputs:
//    ptr - pointer to start of product buffer.
//    params - pointer to product dependent parameters.  
//             Currently assumes all 10 are passed, even if not
//             defined.
//
////////////////////////////////////////////////////////////\*/
void RPG_set_dep_params( void *ptr, short *params  ){

   Graphic_product *prod = (Graphic_product *) ptr;
   int i;
   
   /*
     Transfer parameters to product description block.
   */
   for( i = 0; i < MAX_DEP_PARAMS; i++ ){

      switch( i ){

         case 1:
         {
            prod->param_1 = *params;
            break;
         }
         case 2:
         {
            prod->param_2 = *(params + 1); 
            break;
         }
         case 3:
         {
            prod->param_3 = *(params + 2); 
            break;
         }
         case 4:
         {
            prod->param_4 = *(params + 3); 
            break;
         }
         case 5:
         {
            prod->param_5 = *(params + 4); 
            break;
         }
         case 6:
         {
            prod->param_6 = *(params + 5); 
            break;
         }
         case 7:
         {
            prod->param_7 = *(params + 6); 
            break;
         }
         case 8:
         {
            prod->param_8 = *(params + 7); 
            break;
         }
         case 9:
         {
            prod->param_9 = *(params + 8); 
            break;
         }
         case 10:
         {
            prod->param_10 = *(params + 9); 
            break;
         }
            
      }

   }

}
