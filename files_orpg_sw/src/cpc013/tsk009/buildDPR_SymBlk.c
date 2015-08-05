/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:42:04 $
 * $Id: buildDPR_SymBlk.c,v 1.4 2009/10/27 22:42:04 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "buildDPR_SymBlk.h"
#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: buildDPR_SymBlk.c
 
    Description:
    ============
       buildDPR_SymBlk() creates the Digital Precipitation Rate (DPR) product
    using Packet 28 (Generic Product Packet) using generic radial component,
    xdr based product format.

    Function calls:
       Prod_Description(ptrXDR, vol_num, vcp_num, rate_out, sadpt);
       Build_RadialComp(ptrXDR, RateData);

    Inputs:
       RPGP_product_t* ptrXDR               - pointer to XDR formatted 
                                              generic product
       int             vol_num              - Volume scan number
       int             vcp_num              - Volume Coverage Pattern number
       unsigned short  RateData[][MAX_BINS] - Rain Rate data 
                                              (1000th of inches/hr)
       Rate_Buf_t*     rate_out             - rate output buffer
       Siteadp_adpt_t* sadpt                - site adaptation data

    Outputs:
       char** ptrDPR  - pointer to DPR product buffer
       int*   length  - Number of bytes used (PSB)

    Return:
       The halfword offset to the start of product symbology block (PSB) or
    -1 if failure.

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    ----        -------   ----------          -----
    06/01/07    0000      Cham Pham           Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    01/27/09    0001      James Ward          Commented out Prod_Description 
                                              in favor of 
                                              RPGP_build_RPGP_product_t().
******************************************************************************/

int buildDPR_SymBlk (char **ptrDPR, RPGP_product_t* ptrXDR,
                     int vol_num, int vcp_num, int* length,
                     unsigned short RateData[MAX_AZM][MAX_BINS],
                     Rate_Buf_t* rate_out, Siteadp_adpt_t* sadpt)
{
   char*           serial_data = NULL;   /* Holds serialized generic product data  */
   Symbology_block sym;                  /* Product Symbology Block structure.     */
   packet_28_t     packet28;             /* ICD defined packet 28 for generic data */
   unsigned int    data_len = 0;         /* Symbology block layer length           */
   unsigned int    block_len = 0;        /* Symbology block length                 */
   int             num_bytes = 0;        /* Temporary storage for packet 28 #bytes */
   int             opstat = RPGC_NORMAL; /* Status of output buffer reallocation   */
   int             shortage = 0;         /* Extra bytes needed over original DPR
                                          * allocation                             */ 

   static unsigned int graphic_size   = sizeof(Graphic_product);
   static unsigned int sym_size       = sizeof(Symbology_block);
   static unsigned int packet_28_size = sizeof(packet_28_t);

   /* Advance the length pointer to after the Product Description Block.     */

   *length += graphic_size; /* 120 bytes / 2 = 60 half-word */

   #ifdef QPERATE_DEBUG
      fprintf ( stderr,"MHB and PDB Length: %d\n", *length );
   #endif

   /* Set up the packet 28 header fields. */

   packet28.code  = PACKET_28;
   packet28.align = 0;

   /* Set the generic product header fields. Old code:
    *
    * Prod_Description(ptrXDR, vol_num, vcp_num, rate_out, sadpt); 
    */

   RPGP_build_RPGP_product_t(DPRCODE,
                             vol_num,
                             "Digital Precipitation Rate (DPR)",
                             "Data array product output from QPE RATE",
                             ptrXDR);

   /* Number of Parameters. Note: This product does not have parameter list  */

   ptrXDR->numof_prod_params = NUM_PROD_PARAM;

   /* Set the product parameter list */

   #ifdef QPERATE_DEBUG
   if ( ptrXDR->numof_prod_params == 0 )
      fprintf( stderr, "XDR generic product format does not have product "
                       "parameter\n" );
   #endif

   /* Set number of components, we use only 1 component (grid type) */

   ptrXDR->numof_components = (int) NUM_COMP;

   /* Set radial data product component */

   if ( ptrXDR->numof_components > 0 )
      Build_RadialComp(ptrXDR, RateData);

   /* Print product */

   #ifdef QPERATE_DEBUG
   {
      fprintf(stderr,"About to print ptrXDR: %p\n", (void*) ptrXDR);
      RPGP_print_prod(ptrXDR);
   }
   #endif

   /* We need to serialize the data for transmission.             *
    * NOTE: This may be an infrastructure function in the future. */

   serial_data = NULL;
   packet28.num_bytes = RPGP_product_serialize (ptrXDR, &serial_data );

   #ifdef QPERATE_DEBUG
      fprintf (stderr,
               "Back from serialization, num_bytes=%d\n",
               packet28.num_bytes);
   #endif

   if ( serial_data == NULL )
   {
      RPGC_log_msg(GL_INFO, "buildDPR_SymBlk: Error serializing DPR product!\n");
      return -1;
   }

   /* Make the output buffer is large enough.  If not, reallocate */

   if ( ( packet28.num_bytes + (*length) ) > DPRSIZE )
   {
      /* Compute the byte shortage.  Add some for pre-ICD header (?) */

      shortage = (packet28.num_bytes + (*length)) - DPRSIZE + 100;
      *ptrDPR = (char*)RPGC_realloc_outbuf( (void*)(*ptrDPR),
                                            DPRSIZE + shortage, &opstat );
      if ( opstat != RPGC_NORMAL )
      {
         RPGC_log_msg (GL_ERROR,
                       "buildDPR_SymBlk: Error reallocating DPR output buffer (%d)\n",
                        opstat);

         if (opstat == NO_MEM)
             RPGC_abort_because(PROD_MEM_SHED);
         else
             RPGC_abort();

         if(serial_data != NULL)
            free(serial_data);

         return -1;
      }
   }

   /* Build the first fields of the symbology block but don't copy    */
   /* it to the output buffer yet.  We don't know the length fields.  */

   sym.divider       = (short)-1; /* block divider always -1          */
   sym.block_id      = (short) 1; /* always 1 for symbology block     */
   sym.n_layers      = (short) 1; /* we put DPR product in one layer  */
   sym.layer_divider = (short)-1; /* layer divider always -1          */

   /* Compute the length of layer field for the data layer. */

   data_len = packet_28_size + packet28.num_bytes;

   RPGC_set_product_int( (void *) &sym.data_len, data_len );

   #ifdef QPERATE_DEBUG
   {
      fprintf ( stderr,
            "DATA LEN: %d = sizeof(packet28_t): %d + packet28.num_bytes: %d\n",
            data_len, packet_28_size, packet28.num_bytes );
   }
   #endif

   /* Compute the length of block field for the symbology block. */

   block_len = sym_size + data_len;

   RPGC_set_product_int((void *) &sym.block_len, block_len);

   #ifdef QPERATE_DEBUG
   {
      fprintf ( stderr,
            "BLOCK LEN: %d = sizeof(Symbology_block): %d + data len: %d\n",
             block_len, sym_size, data_len );
   }
   #endif

   /* Copy the completed symbology block header */

   memcpy((*ptrDPR) + (*length), &sym, sym_size);

   *length += sym_size;

   #ifdef QPERATE_DEBUG
      fprintf(stderr,"Completed symbology block header: LENGTH=%d", *length);
   #endif

   /* Set the number of bytes in packet 28 */

   num_bytes = packet28.num_bytes;
   RPGC_set_product_int( (void *) &packet28.num_bytes, num_bytes );

   /* Copy the packet into the output product. */

   memcpy( (*ptrDPR) + (*length), &packet28, packet_28_size);

   *length += packet_28_size;

   #ifdef QPERATE_DEBUG
      fprintf ( stderr,"Completed the packet 28: LENGTH=%d", *length );
   #endif

   /* Copy the serialized data. */

   memcpy( (*ptrDPR) + (*length), serial_data, num_bytes );

   *length += num_bytes;

   #ifdef QPERATE_DEBUG
      fprintf ( stderr,"TOTAL BUFFER SIZE: %d\n", *length );
   #endif

   /* Free the serialized data */

   if(serial_data != NULL)
      free(serial_data);

   #ifdef QPERATE_DEBUG
      fprintf ( stderr,"\nExiting buildDPR_PSB\n" );
   #endif

   /* Return the halfword offset to the start of this data . */

   return((int) (graphic_size / 2));

} /* end buildDPR_SymBlk () =========================================== */
