/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/08/24 14:33:14 $
 * $Id: build_status_prod.c,v 1.7 2009/08/24 14:33:14 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#define BUILD_STATUS_PROD_C
#include <rpg_status_prod.h>
#include <packet_28.h>
#include <orpgsite.h>
#include <rpg_globals.h>
#include <rpgc.h>
#include <orpg_product.h>
#include <rpgp.h>

#define MAX_ATTR_SIZE            128

/* Function Prototypes. */
static int Build_prod_sym_block( int vol_num, char **outbuf, 
                                 RPGP_product_t *xdrbuf, int num_nodes,
                                 int *length );
static int Build_RPGP_text_t( char *xdrbuf, RPGP_text_t *text_packet[] );
static unsigned int Get_color_code( char *text, unsigned int code );

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Driver for building the RPG Status product.
//
///////////////////////////////////////////////////////////////////////\*/
int Build_status_product( int num_messages ){

   RPGP_product_t *ptrXDR = NULL;
   Graphic_product *gpptr = NULL;
   char *ptrASP = NULL;
   int vol_num, length, result, offsetPSB;
   short params[10] = {0}; 

   /* Allocate the RPGP_product_t structure (generic format product). Abort on error. */
   length = sizeof(RPGP_product_t) + num_messages*sizeof(RPGP_text_t);
   if( (ptrXDR = (RPGP_product_t *) calloc( 1, length )) == NULL ){

      RPGC_log_msg( GL_ERROR, "Error obtaining XDR buffer for ASP product\n");
      RPGC_rel_outbuf( ptrASP, DESTROY );
      RPGC_abort();
      return(-1);

   }

   /* Get the current volume scan number.  Needed for the product symbology block. */
   vol_num = ORPGMISC_vol_scan_num( ORPGVST_get_volume_number() );

   /* Start building the product symbology block. */
   length = 0;
   ptrASP = NULL;
   offsetPSB = Build_prod_sym_block( vol_num, &ptrASP, ptrXDR, num_messages, &length );
   if( (offsetPSB >= 0) && (ptrASP != NULL) ){

      /* Set some of the Product Description Block fields. */
      RPGC_prod_desc_block( (void*) ptrASP, STATPROD, vol_num);

      /* Put the halfword offsets for each block into the PDB.        */
      gpptr = (Graphic_product *) ptrASP;
      RPGC_set_product_int( (void *) &gpptr->sym_off, offsetPSB );
      RPGC_set_product_int( (void *) &gpptr->gra_off, 0 );    /* Not used */
      RPGC_set_product_int( (void *) &gpptr->tab_off, 0 );    /* Not used */

      /* Set the RDA Build Number and RPG Build Number as the first two
         product dependent parameters. Note:  The RPG Build Number currently
         currently has the major number scaled by 10 in ORPGMISC_RPG_build_number.
         To be consistent with RDA Build number, scale this number by 10 
         so the major number is scaled by 100. */
      params[0] = ORPGRDA_get_status( RS_RDA_BUILD_NUM );
      params[1] = ORPGMISC_RPG_build_number() * 10;

      /* Set the product dependent parameters in the Product Description
         Block. */
      result = RPGC_set_dep_params( (void*) ptrASP, params );

      /* Fill the Message Header Block. (This result is used so check it)*/
      /* (Subtract the header length because function doesn't expect it.)*/
      length -= sizeof(Graphic_product);
      RPGC_log_msg( GL_INFO, "Product Length Without Graphic_product structure: %d\n", length );
      result = RPGC_prod_hdr((void*)ptrASP, STATPROD, &length);

      /* Upon successful completion of the product release buffer   */
      if( result == 0 ){

         RPGC_rel_outbuf( ptrASP, FORWARD );
         ptrASP = NULL;

      }
      else{

         /* Destroy the output buffer, if allocated, then abort. */
         if( ptrASP != NULL ){

            RPGC_rel_outbuf( ptrASP, DESTROY );
            ptrASP = NULL;
            RPGC_log_msg( GL_ERROR, "ASP outbuf destroyed\n" );

         }

         RPGC_abort_datatype_because( STATPROD, PROD_MEM_SHED );

      }

      /* Release any memory allocated on behalf of the RPGP_product_t    */
      RPGP_product_free(ptrXDR);

   }

   return 0;

} /* End of Build_status_product() */

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Driver for building the Product Symbology Block.  For the Status
//      product, we use Packet 28 (Generic Product packet).  
//
//   Inputs:
//      vol_num - volume scan number.
//      xdrbuf - buffer to hold the generic product data.
//      num_messages - number of status messages in product.
//
//   Outputs:
//      outbuf - product buffer.
//      length - stores the length of the product.  This is the value to
//               be stored in the Product Header.
//
//   Returns:
//      -1 on error, 0 otherwise.
//
/////////////////////////////////////////////////////////////////////////\*/
static int Build_prod_sym_block( int vol_num, char **outbuf, 
                                 RPGP_product_t *xdrbuf, int num_messages, 
                                 int *length ){

   RPGP_text_t **text_packet;
   packet_28_t packet28; 
   Symbology_block sym; 
   char *serial_data = NULL;
   int data_len, num_bytes, size, status;

   /* Format the Symbology block header. */
   packet28.code  = PACKET_28;
   packet28.align = 0;

   /* Advance the length by the size of the Product Header and Product
      Description Block. */
   *length += sizeof( Graphic_product );

   /* Format the RPGP_product_t block. */
   RPGP_build_RPGP_product_t( STATPROD, vol_num, "ASP", 
                              "Archive III Status Product", xdrbuf );

   /* Format the RPGP_text_t block. */
   text_packet = calloc( num_messages, sizeof( RPGP_text_t * ) );
   if( text_packet == NULL ){

      RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                    num_messages*sizeof( RPGP_text_t * ) );
      return(-1);

   }

   Build_RPGP_text_t( ((char *) xdrbuf) + sizeof(RPGP_product_t), text_packet );

   /* Complete the RPGP_product_t initialization. */
   RPGP_finish_RPGP_product_t( xdrbuf, 0, NULL, num_messages, (void **) text_packet );

   /* Serialize the product. */
   packet28.num_bytes = RPGP_product_serialize( xdrbuf, &serial_data );
   if (serial_data == NULL) {

      RPGC_log_msg( GL_ERROR, "Error serializing ASP product!\n" );
      return(-1);

   }

   RPGC_log_msg( GL_INFO, "The Status Product Holds %d Status Messages\n", num_messages );
   RPGC_log_msg( GL_INFO, "The Size of Packet 28 is %d Bytes (Serialized)\n", packet28.num_bytes );

   /* The size includes the product header, product description block, generic
      product header, symbology block header, and generic product text packets. */
   size = sizeof(RPGP_product_t) + sizeof(Graphic_product) + sizeof(Symbology_block) +
          packet28.num_bytes;

   /* Get the output buffer.  Abort on error. */
   RPGC_log_msg( GL_INFO, "Allocating Output Buffer to Hold %d Bytes\n", size );
   *outbuf = (char *) RPGC_get_outbuf_by_name( "STATPROD", size, &status );
   if( (*outbuf == NULL) || (status != NORMAL) ){

      RPGC_log_msg(GL_ERROR, "Error obtaining output buffer (%d)\n", status );
      if( status == NO_MEM )
         RPGC_abort_because( PROD_MEM_SHED );
      else
         RPGC_abort();

      return(-1);

   }

   /* Start populating the Symbology Block fields. */
   sym.divider = (short) -1; 
   sym.block_id = (short) 1; 
   sym.n_layers = (short) 1;
   sym.layer_divider = (short) -1;
   
   /* Compute the length of layer field for the data layer. */
   data_len = sizeof(packet_28_t) + packet28.num_bytes;
   RPGC_set_product_int( (void *) &sym.data_len, data_len );
   
   /* Compute the length of block field for the symbology block. */
   RPGC_set_product_int( (void *) &sym.block_len, 
                         (unsigned int) (sizeof(Symbology_block) + data_len) );
   
   /* Copy the completed symbology block header */
   memcpy( (*outbuf) + (*length), &sym, sizeof(Symbology_block));
   *length += sizeof(Symbology_block);

   RPGC_log_msg( GL_INFO, "Accounting for Symbology Block Header, Product Size is %d Bytes\n",
                 *length );
   
   /* Set the number of bytes in packet 28 */
   num_bytes = packet28.num_bytes;
   RPGC_set_product_int( (void *) &packet28.num_bytes, num_bytes );
   
   /* Copy the packet into the output product. */
   memcpy( (*outbuf) + (*length), &packet28, sizeof(packet_28_t) );
   *length += sizeof(packet_28_t);

   RPGC_log_msg( GL_INFO, "Accounting for Symbology Block with Packet 28 Header, Product Size is %d Bytes\n",
                 *length );
   
   /* Copy the serialized data. */
   memcpy( (*outbuf) + (*length), serial_data, num_bytes );
   *length += num_bytes;

   RPGC_log_msg( GL_INFO, "Entire Product Header, Description Block and Symbology Block Size is %d Bytes\n",
                 *length );
   
   free(serial_data);

   return( sizeof(Graphic_product)/2 );
}

/*\///////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Driver for building the RPGP_text_t packet.
//
//   Outputs:
//      xdrbuf - holds the RPGP_text_t structures.
//      text_packet - array of pointers ot RPGP_text_t.  Used in RPGP_product_t
//                    structure.
//
//   Returns:
//      -1 on error, or 0 on success.
//
///////////////////////////////////////////////////////////////////////////////\*/
int Build_RPGP_text_t( char *xdrbuf, RPGP_text_t *text_packet[] ){

   Text_node_t *node;
   int num_nodes, size;
   unsigned int code;

   /* Build the RPGP_text_t structure for each message. */
   node = List_head;
   num_nodes = 0;
   size = 0;
   if( node != NULL ){

      while(1){

         text_packet[ num_nodes ] = (RPGP_text_t *) calloc( 1, sizeof(RPGP_text_t) ); 
         if( text_packet[ num_nodes ] == NULL ){

            RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n", sizeof(RPGP_text_t) );
            return(-1);

         }

         /* Set fields in this structure. */
         text_packet[ num_nodes ]->comp_type = RPGP_TEXT_COMP;
         text_packet[ num_nodes ]->numof_comp_params = 1;
         code = Get_color_code( node->text, node->code );

         text_packet[ num_nodes ]->comp_params = 
                    (RPGP_parameter_t *) calloc( 1, sizeof(RPGP_parameter_t) );
         if( text_packet[ num_nodes ]->comp_params == NULL ){

            LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n", sizeof(RPGP_parameter_t) );
            return(-1);

         }

         switch( code ){

            case RPG_INFO_STATUS_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG INFO", 1, 0 );
               break;
            }
            case RPG_GEN_STATUS_MSG:
            default:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG GEN STATUS", 1, 0 );
               break;
            }
            case RPG_WARN_STATUS_MSG:        
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG WARNING", 1, 0 );
               break;
            }
            case RPG_NB_COMMS_STATUS_MSG:        
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "NB COMMS", 1, 0 );
               break;
            }
            case RPG_MAM_ALARM_ACTIVATED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG MAM ALARM", 1, 0 );
               break;
            }
            case RPG_MAR_ALARM_ACTIVATED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG MAR ALARM", 1, 0 );
               break;
            }
            case RPG_LS_ALARM_ACTIVATED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG LS ALARM", 1, 0 );
               break;
            }
            case RPG_ALARM_CLEARED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RPG ALARM CLEARED", 1, 0 );
               break;
            }
            case RDA_SEC_ALARM_ACTIVATED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA SECONDARY ALARM", 1, 0 );
               break;
            }
            case RDA_MAR_ALARM_ACTIVATED_MSG: 
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA MAR ALARM", 1, 0 );
               break;
            }
            case RDA_MAM_ALARM_ACTIVATED_MSG:
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA MAM ALARM", 1, 0 );
               break;
            }
            case RDA_INOP_ALARM_ACTIVATED_MSG: 
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA INOP ALARM", 1, 0 );
               break;
            }
            case RDA_NA_ALARM_ACTIVATED_MSG:  
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA NA ALARM", 1, 0 );
               break;
            }
            case RDA_ALARM_CLEARED_MSG:      
            {
               RPGP_set_string_param( text_packet[ num_nodes ]->comp_params, "Msg Type", 
                                      "", "RDA ALARM CLEARED", 1, 0 );
               break;
            }

         }

         text_packet[ num_nodes ]->text = node->text;

         /* Add this RPGP_text_t packet to the product. */
         memcpy( xdrbuf + size, text_packet, sizeof( RPGP_text_t ) );
         size += sizeof( RPGP_text_t );

         /* Prepare for next node. */
         node = node->next;
         if( node == NULL )
            break;

         num_nodes++;
      
      }

   }

   return 0;

}

/*\//////////////////////////////////////////////////////////////////////////////////////
//   
//   Description:
//      Assigns a color code (filter value) for each status message.
//
//   Inputs:
//      text - status message text
//      code - code value from LE_critical_msg structure.
//
//   Returns:
//      Returns filer value corresponding to the text and code value.
//
//////////////////////////////////////////////////////////////////////////////////////\*/
static unsigned int Get_color_code( char *text, unsigned int code ){

   unsigned int rpg_msg = (code & LE_RPG_STATUS_TYPE_MASK);
   unsigned int rpg_alarm = (code & LE_RPG_ALARM_TYPE_MASK);
   unsigned int rda_alarm = (code & LE_RDA_ALARM_TYPE_MASK);

   /* RPG Alarm messages. */
   if( rpg_alarm ){

      if( code & LE_RPG_AL_CLEARED )
         return( RPG_ALARM_CLEARED_MSG );

      switch( rpg_alarm ){

         case LE_RPG_AL_MAR:
         default:
            return( RPG_MAR_ALARM_ACTIVATED_MSG );

         case LE_RPG_AL_MAM:
            return( RPG_MAM_ALARM_ACTIVATED_MSG );

         case LE_RPG_AL_LS:
            return( RPG_LS_ALARM_ACTIVATED_MSG );

      }

   /* RDA Alarm messages. */
   } else if( rda_alarm ){


      if( code & LE_RDA_AL_CLEARED )
         return( RDA_ALARM_CLEARED_MSG );

      /* Get the RDA alarm level so we can set the appropriate type. */
      switch( rda_alarm ){

         case LE_RDA_AL_NOT_APP:
            return( RDA_NA_ALARM_ACTIVATED_MSG );
                            
         case LE_RDA_AL_SEC:
            return( RDA_SEC_ALARM_ACTIVATED_MSG );
                            
         case LE_RDA_AL_MAR:
            return( RDA_MAR_ALARM_ACTIVATED_MSG );
                            
         case LE_RDA_AL_MAM:
            return( RDA_MAM_ALARM_ACTIVATED_MSG );
                            
         case LE_RDA_AL_INOP:
            return( RDA_INOP_ALARM_ACTIVATED_MSG );
                           
         default:
            return( RDA_SEC_ALARM_ACTIVATED_MSG );
                        
      }

   /* RPG messages. */
   } else if ( rpg_msg ){

      if( rpg_msg == LE_RPG_WARN_STATUS )
         return( RPG_WARN_STATUS_MSG );

      else if( rpg_msg == LE_RPG_INFO_MSG )
         return( RPG_INFO_STATUS_MSG );

      else if( rpg_msg == LE_RPG_COMMS )
         return( RPG_NB_COMMS_STATUS_MSG );

      else if( rpg_msg == LE_RPG_GEN_STATUS )
         return( RPG_GEN_STATUS_MSG );

   /* All other messages. */
   } else {

      if( code & GL_ERROR_BIT )
         return( RPG_WARN_STATUS_MSG );

      else
         return( RPG_GEN_STATUS_MSG );

   }

   return 0;

} /* End of Get_color_code() */
