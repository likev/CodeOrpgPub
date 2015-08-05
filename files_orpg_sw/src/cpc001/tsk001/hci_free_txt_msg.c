/************************************************************************
 *	hci_free_txt_msg.c - This module creates a free text message	*
 *	product form a user defined string.  The user must also supply	*
 *	a list of the destinations the message is to be sent to.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:07 $
 * $Id: hci_free_txt_msg.c,v 1.12 2009/02/27 22:26:07 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#include <hci_control_panel.h>

/*\////////////////////////////////////////////////////////
//
// Description:
//   Given a text string and product dependent parameters,
//   this module builds a free text message product.  This
//   module uses the services of librpg and librpgc.
//
// Inputs:
//    string - pointer to text string.  This string is the
//             alphanumeric data of the product.
//    params - pointer to the product dependent parameters
//             for this product.  These are bit maps
//             designating the line ids of the recipients of
//             this product.
//
// Returns:
//    0 - all normal completion, -1 otherwise.
//
///////////////////////////////////////////////////////\*/  
int hci_free_txt_msg( char *string, short *params ){

   int vol_num, length, status;
   int prod_id = FTXTMSG;
   short *ptr;

   /*
     Get current volume scan number.
   */
   vol_num = RPGC_get_current_vol_num();

   /*
     Acquire output buffer for Free Text Message.  Return -1 on failure.
   */
   ptr = (short *) RPGC_get_outbuf( FTXTMSG, 5000, &status );
   if( ptr == NULL )
   {
      HCI_LE_error("NULL pointer returned from RPGC_get_outbuf");
      return (-1);
   }


   /*
     Fill in the product description block.  Return -1 on failure.
   */
   status = RPGC_prod_desc_block( (void *) ptr, prod_id, vol_num );
   if( status < 0 )
   {
      HCI_LE_error("RPGC_prod_desc_block Failed");
      return (-1);
   }
 
   /*
     Set the product dependent parameters in the product description
     block.
   */
   RPGC_set_dep_params( (void *) ptr, params );

   /*
     Build the standalone alphanumeric data.  Return -1 on failure.
   */
   status = RPGC_stand_alone_prod( (void *) ptr, string, &length );
   if( status < 0 )
   {
      HCI_LE_error("RPGC_stand_alone_prod Failed");
      return (-1);
   }

   /*
     Build product header.  Return -1 on failure.  
   */
   status = RPGC_prod_hdr( (void*) ptr, prod_id, &length );
   if( status < 0 )
   {
      HCI_LE_error("RPGC_prod_hdr Failed");
      return (-1);
   }

   
   {
      /* For dumping the messsage header block. */
/*
      fprintf( stderr, "PRODUCT HEADER\n" );
      fprintf( stderr, "LENGTH = %d\n", prod->msg_len);
      fprintf( stderr, "--> Message Code = %d\n", prod->msg_code );
      fprintf( stderr, "--> Number of Blocks = %d\n", prod->n_blocks );
*/
   }

   {
      /* For dumping the product description block. */
/*
      fprintf( stderr, "PRODUCT DESCRIPTION BLOCK\n" );

      fprintf( stderr, "--> Divider = %d\n", prod->divider );
      fprintf( stderr, "--> Latitude = %d\n", prod->latitude );
      fprintf( stderr, "--> Longitude = %d\n", prod->longitude );
      fprintf( stderr, "--> Height = %d\n", prod->height );
      fprintf( stderr, "--> Product Code = %d\n", prod->prod_code );
      fprintf( stderr, "--> Op Mode = %d\n", prod->op_mode );
      fprintf( stderr, "--> VCP Num = %d\n", prod->vcp_num );
      fprintf( stderr, "--> Vol Num = %d\n", prod->vol_num );
      fprintf( stderr, "--> Vol Date = %d\n", prod->vol_date );
      fprintf( stderr, "--> Vol Time (MSW) = %d\n", prod->vol_time_ms );
      fprintf( stderr, "--> Vol Time (LSW) = %d\n", prod->vol_time_ls );
      fprintf( stderr, "--> Gen Date = %d\n", prod->gen_date );
      fprintf( stderr, "--> Gen Time = %d\n", prod->gen_time );
      fprintf( stderr, "--> Elev Index = %d\n", prod->elev_ind );
      fprintf( stderr, "--> # Maps = %d\n", prod->n_maps );
      fprintf( stderr, "--> Sym Offset = %d\n", prod->sym_off );
      fprintf( stderr, "--> Gra Offset = %d\n", prod->gra_off );
      fprintf( stderr, "--> Tab Offset = %d\n", prod->tab_off );

      fprintf( stderr, "--> Product Dependent Parameters\n" );
      fprintf( stderr, "----> Parm 1 = %d\n", prod->param_1 );
      fprintf( stderr, "----> Parm 2 = %d\n", prod->param_2 );
      fprintf( stderr, "----> Parm 3 = %d\n", prod->param_3 );
      fprintf( stderr, "----> Parm 4 = %d\n", prod->param_4 );
      fprintf( stderr, "----> Parm 5 = %d\n", prod->param_5 );
      fprintf( stderr, "----> Parm 6 = %d\n", prod->param_6 );
      fprintf( stderr, "----> Parm 7 = %d\n", prod->param_7 );
      fprintf( stderr, "----> Parm 8 = %d\n", prod->param_8 );
      fprintf( stderr, "----> Parm 9 = %d\n", prod->param_9 );
      fprintf( stderr, "----> Parm 10 = %d\n", prod->param_10 );

      fprintf( stderr, "--> Data Levels\n" );
      fprintf( stderr, "----> Level 1 = %x\n", prod->level_1 );
      fprintf( stderr, "----> Level 2 = %x\n", prod->level_2 );
      fprintf( stderr, "----> Level 3 = %x\n", prod->level_3 );
      fprintf( stderr, "----> Level 4 = %x\n", prod->level_4 );
      fprintf( stderr, "----> Level 5 = %x\n", prod->level_5 );
      fprintf( stderr, "----> Level 6 = %x\n", prod->level_6 );
      fprintf( stderr, "----> Level 7 = %x\n", prod->level_7 );
      fprintf( stderr, "----> Level 8 = %x\n", prod->level_8 );
      fprintf( stderr, "----> Level 9 = %x\n", prod->level_9 );
      fprintf( stderr, "----> Level 10 = %x\n", prod->level_10 );
      fprintf( stderr, "----> Level 11 = %x\n", prod->level_11 );
      fprintf( stderr, "----> Level 12 = %x\n", prod->level_12 );
      fprintf( stderr, "----> Level 13 = %x\n", prod->level_13 );
      fprintf( stderr, "----> Level 14 = %x\n", prod->level_14 );
      fprintf( stderr, "----> Level 15 = %x\n", prod->level_15 );
      fprintf( stderr, "----> Level 16 = %x\n", prod->level_16 );
*/
   }

   {

      /*
        For dumping the tabular data.
      */
      short num_pages, num_chars, *tabular;
      char *char_data, *start_of_tabular;
      int j;

      start_of_tabular = (char *) ptr + sizeof( Graphic_product );
      tabular = (short *) start_of_tabular;
/*
      fprintf( stderr, "TABULAR DATA\n" );
      fprintf( stderr, "--> Block Divider = %d\n", *tabular );
*/
      tabular++;

      num_pages = *tabular;
/*
      fprintf( stderr, "--> Number of Pages = %d\n", num_pages );
*/
      tabular++;

      for( j = 0; j < num_pages; j++ ){


         while( *tabular != -1 ){
/*
            fprintf( stderr, "--> Number of Characters This Line = %d\n", *tabular );
*/
            num_chars = *tabular;
            tabular++;

            char_data = (char *) tabular;
/*
            for( i = 0; i < num_chars; i++ )
               fprintf( stderr, "%c", *(char_data + i) );

            fprintf( stderr, "\n" );
*/
            tabular += (num_chars / sizeof(short));

         }

      }

   }

   
   /*
     Release the output buffer. 
   */
   RPGC_rel_outbuf( ptr, FORWARD );

   /*
     Return Normal.
   */
   return 0;    

/* End of hci_free_text_msg( ) */
}
