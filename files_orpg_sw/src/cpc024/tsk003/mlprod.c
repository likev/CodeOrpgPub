
/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2010/02/03 18:48:52 $ */
/* $Id: mlprod.c,v 1.7 2010/02/03 18:48:52 ccalvert Exp $ */
/* $Revision: 1.7 $ */
/* $State: Exp $ */

#include <mlprod.h>
#include <hca.h>
#include <packet_0e03.h>
#include <packet_0802.h>
#include <packet_8.h>
#define FLOAT
#include <rpgcs_coordinates.h>

/* Global Variables */
       int   DB_RADIAL = -1;        /* Single radial for debug info.  Set to -1 for no debug */
       ML_r_t ML_range[MAX_ML_AZ]; /* Melting layer ranges (meters) for each radial */
       float ML_azimuth[730];      /* Azimuths (degrees) for each radial */ 
       short Num_labels = 0;             
       float Scale = 1;
       float Offset = 0;
       int   Totalbytes = 0;


/*********************************************************************

   Description:
      Buffer control module for the melting layer product.

   Returns:
      Currently, always returns 0.

*********************************************************************/
int mlprod_buffer_control(){

    int opstat;

    char *obrptr = NULL;
    char *mldataptr = NULL;

    /* Initialization. */
    opstat = 0;
    Endelcut = 0;
    Proc_rad = 1;

    /* Acquire output buffer for product. */
    obrptr = RPGC_get_outbuf_by_name( OUTDATA_NAME, OBUF_SIZE, &opstat );

    if( opstat == NORMAL ){

      /* Request first radial-based input buffer and process it. */
	 mldataptr = RPGC_get_inbuf_by_name( INDATA_NAME, &opstat);

      /* Check for successful read of input buffer. */
	 if( opstat != NORMAL ){

	     /* Release input buffer and do abort processing. */
		RPGC_rel_inbuf( mldataptr );
		RPGC_rel_outbuf( obrptr, DESTROY );
          RPGC_log_msg(GL_INFO,"No Input HCA data!");
		RPGC_abort_because( PGM_DISABLED_MOMENT );
		return 0;
	 } /* end if input buffer opstat != NORMAL */

      /* Do For all input HCA radials until end of elevation encountered. */
      while(1){

        if( opstat == NORMAL ){

           /* Initialize the end of elevation flag */
           Endelcut = 0;

           /* Call the product generation control routine. */
           mlprod_generation_control( obrptr, mldataptr );

           /* Release the input radial. */
	      RPGC_rel_inbuf( mldataptr );

 	      if( Endelcut ){
              /* Elevation cut completed. */
		    RPGC_rel_outbuf( obrptr, FORWARD|RPGC_EXTEND_ARGS, Totalbytes);
              break;
 
	      } /* end if !Endelcut */

           if( !Endelcut ){
              /* Retrieve next input radial. */
		    mldataptr = RPGC_get_inbuf_by_name( INDATA_NAME, &opstat );
              continue;
           }
	   } 
        else {
           /* If the input data stream has been canceled for some reason, 
              then release and destroy the product buffer, and return to 
              wait for activation. */
	      RPGC_rel_outbuf( obrptr, DESTROY );
           RPGC_log_msg(GL_INFO,"Input data stream canceled!");
		 RPGC_abort_because( PGM_DISABLED_MOMENT );
           break;

	   }/* end if input buffer opstat == NORMAL */
      } /* end while */
    } /* end if output buffer opstat == NORMAL */
    else {
       RPGC_log_msg(GL_INFO,"No available output buffers!");
       RPGC_abort_because( opstat );
    }

    /* Return to wait for activation. */
    return 0;

/* End of mlprod_buffer_control() */
} 


/********************************************************************* 

   Description:
      Controls the processing of data for the melting layer product. 

   Inputs:
      obrptr - pointer to product buffer.
      mldataptr - pointer to radial message.

   Returns:
      Always returns 0.

******************************************************************* */
int mlprod_generation_control( char *obrptr, char *mldataptr ){

    /* Local variables */
    int volnumber, vcpnumber; 
    short topEdge, topCenter, botEdge, botCenter;

    double coselev;
    static double elang = 0;
    static int elevindex = 0;
    
    Scan_Summary *summary = NULL;
    Base_data_header *radhead = (Base_data_header *) mldataptr;

    /* Static variables. */
    Generic_moment_t gm;   /* Input generic moment data */
    
    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) 
                    || 
        (radhead->status == GOODBEL) ){

      /* Initialize the min and max data level. */
	 Mxpos = 0;
      Mnpos = 999;

      /* Initialize the radial counter. */
      Radcount = 0;

      /* Get elevation angle from volume coverage pattern and elevation 
         index number. */
	 volnumber = RPGC_get_buffer_vol_num( mldataptr );
      summary   = RPGC_get_scan_summary( volnumber );
	 vcpnumber = summary->vcp_number;
      elevindex = RPGC_get_buffer_elev_index( mldataptr );
      RPGC_log_msg(GL_INFO,"Start of elevation index = %d",elevindex);
	 Elmeas = (int) RPGCS_get_target_elev_ang(  vcpnumber, elevindex );
	 elang = Elmeas * .1f;

      /* Cosine of elevation angle computation for combination with 
         scale factor for the scan projection correction. */
	 coselev = cos( elang * DEGTORAD );
	 Vc = coselev * 1e3f;

      /* Pack product header fields. */
	 mlprod_header( obrptr, volnumber );

    } /*end if beginning of elevation or volume */
 
    /* Perform individual radial processing if radial not flagged 'BAD'. */
    if( Proc_rad ){
        unsigned short *dp_data = NULL;
        strcpy(gm.name,"DML");
        dp_data = (unsigned short *)RPGC_get_radar_data((void *)radhead, RPGC_DANY, &gm);

        if (dp_data == NULL) {
          RPGC_log_msg(GL_INFO,"Failed to get input data, radial #%d",Radcount);
		RPGC_abort_because( PGM_DISABLED_MOMENT );
          return 0;
        }

        /* Retrieve the scale factor and offset for the data */
        Scale  = gm.scale;
        Offset = gm.offset;

        if (Radcount == DB_RADIAL) {
          RPGC_log_msg(GL_INFO,"dp_data[MLTT]=%d, [MLT]=%d, [MLB]=%d [MLBB]=%d",
                               dp_data[MLTT],dp_data[MLT],dp_data[MLB],dp_data[MLBB]);
        }
        /* Extract the four bin values to local variables */
        topEdge   = RPGC_NINT((dp_data[MLTT] - Offset) / Scale);
        topCenter = RPGC_NINT((dp_data[MLT] - Offset) / Scale);
        botCenter = RPGC_NINT((dp_data[MLB] - Offset) / Scale);
        botEdge   = RPGC_NINT((dp_data[MLBB] - Offset) / Scale);

        /* Convert the input bin number to a range (meters)
           for each of the four melting layer values        */
        ML_range[Radcount].r_tt = topEdge * gm.bin_size;
        ML_range[Radcount].r_t  = topCenter  * gm.bin_size;
        ML_range[Radcount].r_b  = botCenter  * gm.bin_size;
        ML_range[Radcount].r_bb = botEdge * gm.bin_size;
        ML_azimuth[Radcount]    = radhead->azimuth;

     if (Radcount == DB_RADIAL){
      RPGC_log_msg(GL_INFO,"Radial%d topEdge=%d topCenter=%d botCenter=%d botEdge=%d bin size=%d ML_range[0]=%f azimuth=%f",
                     Radcount,topEdge,topCenter,botCenter,botEdge,gm.bin_size,ML_range[0].r_tt,radhead->azimuth);
      RPGC_log_msg(GL_INFO,"Scale = %f Offset = %f",Scale,Offset);
     }

        /* Increment the radial count. */
        Radcount++;

        /* Is this the maximum for the elevation? */
        if( topCenter > Mxpos )
           Mxpos = topCenter;

        /* Is this the minimum for the elevation? */
        if( botCenter < Mnpos)
           Mnpos = botCenter;

    } /* end if Proc_rad */

    /* Test for psuedo-end of elev or pseudo-end of volume. */
    if( (radhead->status == PGENDEL) || (radhead->status == PGENDVOL) )
	Proc_rad = 0;

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){
	Endelcut = 1;

        /* If last radial encountered, fill remaining fields in product buffer. */
     elevindex = RPGC_get_buffer_elev_index( mldataptr );
	mlprod_end_of_product_processing(elevindex, elang, obrptr );
    }

    /* Return to buffer control routine. */
    return 0;

/* End of mlprod_generation_control() */
} 


/********************************************************************
   
   Description:
      Fills in product description block, symbology block information.

   Inputs:
      obrptr - pointer to output buffer.
      vol_num - volume scan number.

   Returns:
      Always returns 0.

********************************************************************/
int mlprod_header( char *obrptr, int vol_num ){

    Graphic_product *phd = (Graphic_product *) obrptr;
    Symbology_block *sym = (Symbology_block *) 
                                 (obrptr + sizeof(Graphic_product));
    int elev_ind;
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, Prod_id, vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    elev_ind = ORPGPAT_get_elevation_index( Prod_id );
    if( elev_ind >= 0 )
        params[elev_ind] = Elmeas;

    RPGC_set_dep_params( obrptr, params );
    
    /* Store offset to symbology. */
    RPGC_set_prod_block_offsets( phd, sizeof(Graphic_product)/sizeof(short), 0, 0 );

    /* Store product block divider, block ID, number of layers and 
       layer divider. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    return 0;

/* End of mlprod_header() */
}


/**********************************************************************

   Description:
      Fill remaining Product Header fields for melting layer product. 


   Inputs:
      ndpd - number of bytes in the product so far.
      elev_ind - elevation index.
      obrptr - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int mlprod_end_of_product_processing( int elev_ind, float elev,
                                      char *obrptr ){

    int   pbuffind, ndpb, r;
    float maxval = 0.0;
    float minval = 0.0;
    float x, y;
    float maxheight, minheight;
    short savei,savej;

    Graphic_product *phd = (Graphic_product *) obrptr;
    Symbology_block *sym = (Symbology_block *)(obrptr + sizeof(Graphic_product));
    short       *outbuff = (short *)(obrptr + sizeof(Graphic_product) + sizeof(Symbology_block));
    packet_0e03_hdr_t packet_0e03;
    packet_0802_t     packet_0802;
 
    /* Enter the data threshold values   */
    phd->level_1=(short)0x0001; /* ML Top (beam edge)      */
    phd->level_2=(short)0x0002; /* ML Top (beam center)    */
    phd->level_3=(short)0x0003; /* ML Bottom (beam center) */
    phd->level_4=(short)0x0004; /* ML Bottom (beam edge)   */

    /* Number of blocks in product = 3   */
    phd->n_blocks=(short)3;

    /* Initialize the output buffer pointer */
    pbuffind = ndpb = 0;
    Num_labels = 0;

    /* Set the color which also identifies the contour as melting layer top (beam edge) */
    packet_0802.code              = (short)0x0802; /* packet code 0802 for set color */
    packet_0802.indicator         = (short)0x0002; /* Color value indicator          */
    packet_0802.value             = (short)ML_TOP_EDGE; /* Color value                     */
    memcpy(&outbuff[pbuffind], &packet_0802, sizeof(packet_0802_t));
    ndpb += sizeof(packet_0802_t);
    pbuffind += sizeof(packet_0802_t) / 2;

    /* Construct the packet 0e03 layer header. */
    packet_0e03.code              = (short)0x0E03; /* packet code 0E03 for linked vectors */
    packet_0e03.initial_pt_ind    = (short)0x8000; /* initial point indicator             */
    packet_0e03.length            = (Radcount)*4;  /* 4 shorts per vector                 */

    /* Compute the first I and J coordinates for the ML top (beam edge) */ 
    RPGCS_azranelev_to_xy(ML_range[0].r_tt, ML_azimuth[0], elev, &x, &y);
/*    RPGCS_azran_to_xy(ML_range[0].r_tt, ML_azimuth[0], &x, &y);*/
    packet_0e03.i_start = (short)RPGC_NINT(x * M_TO_QKM);
    packet_0e03.j_start = (short)RPGC_NINT(y * M_TO_QKM);
    packet_0e03.length  = Radcount * BYTES_PER_VECT;

    /* Save the start coordinates because they also are the end
       coordinates for the last vector.                        */
    savei = packet_0e03.i_start;
    savej = packet_0e03.j_start;

    /* Copy the vector packet header for the first contour into the output product buffer */
    memcpy(&outbuff[pbuffind], &packet_0e03, sizeof(packet_0e03_hdr_t));
    ndpb += sizeof(packet_0e03_hdr_t);
    pbuffind += sizeof(packet_0e03_hdr_t) / 2;

    /* Loop for each of the remaining radials using the melting layer top (beam edge) */
    for (r = 1; r < Radcount; r++) {
       RPGCS_azranelev_to_xy(ML_range[r].r_tt, ML_azimuth[r], elev, &x, &y);
/*       RPGCS_azran_to_xy(ML_range[r].r_tt, ML_azimuth[r], &x, &y);*/
       outbuff[pbuffind]   = (short)RPGC_NINT(x * M_TO_QKM);
       outbuff[pbuffind+1] = (short)RPGC_NINT(y * M_TO_QKM);
       ndpb += BYTES_PER_VECT;
       pbuffind += SHORTS_PER_VECT;
    } /* end for all radials */

    /* Close the contour by adding the saved I and J from the start point */
    outbuff[pbuffind]   = savei;
    outbuff[pbuffind+1] = savej;
    ndpb += BYTES_PER_VECT;
    pbuffind += SHORTS_PER_VECT;

    /* Set the color which also identifies the contour as melting layer top (beam center) */
    packet_0802.code              = (short)0x0802; /* packet code 0802 for set color */
    packet_0802.indicator         = (short)0x0002; /* Color value indicator          */
    packet_0802.value             = (short)ML_TOP_CENTER; /* Color value                     */
    memcpy(&outbuff[pbuffind], &packet_0802, sizeof(packet_0802_t));
    ndpb += sizeof(packet_0802_t);
    pbuffind += sizeof(packet_0802_t) / 2;

    /* Compute the first I and J coordinates for the melting layer top (beam center) */ 
    RPGCS_azranelev_to_xy(ML_range[0].r_t, ML_azimuth[0], elev, &x, &y);
/*    RPGCS_azran_to_xy(ML_range[0].r_t, ML_azimuth[0], &x, &y);*/
    packet_0e03.i_start = (short)RPGC_NINT((float)x * M_TO_QKM);
    packet_0e03.j_start = (short)RPGC_NINT((float)y * M_TO_QKM);

    /* Save the start coordinates because they also are the end
       coordinates for the last vector.                        */
    savei = packet_0e03.i_start;
    savej = packet_0e03.j_start;

    /* Add the header for the melting layer top (beam center) */
    memcpy(&outbuff[pbuffind], &packet_0e03, sizeof(packet_0e03_hdr_t));
    ndpb += sizeof(packet_0e03_hdr_t);
    pbuffind += sizeof(packet_0e03_hdr_t) / 2;

    /* Add the remaining vector points for the melting layer top (beam center) */
    for (r = 1; r < Radcount; r++) {
       RPGCS_azranelev_to_xy(ML_range[r].r_t, ML_azimuth[r], elev, &x, &y);
/*          RPGCS_azran_to_xy(ML_range[r].r_t, ML_azimuth[r], &x, &y);*/
       outbuff[pbuffind]   = (short)RPGC_NINT(x * M_TO_QKM);
       outbuff[pbuffind+1] = (short)RPGC_NINT(y * M_TO_QKM);
/**       add_label(ML_range[r].r_t, outbuff[pbuffind], outbuff[pbuffind+1], "MLT");**/
       ndpb += BYTES_PER_VECT;
       pbuffind += SHORTS_PER_VECT;
    } /* end for all radials */

    /* Close the contour by adding the saved I and J from the start point */
    outbuff[pbuffind]   = savei;
    outbuff[pbuffind+1] = savej;
    ndpb += BYTES_PER_VECT;
    pbuffind += SHORTS_PER_VECT;

    /* Set the color which also identifies the contour as melting layer bottom (beam center) */
    packet_0802.code              = (short)0x0802; /* packet code 0802 for set color */
    packet_0802.indicator         = (short)0x0002; /* Color value indicator          */
    packet_0802.value             = (short)ML_BOT_CENTER; /* Color value                     */
    memcpy(&outbuff[pbuffind], &packet_0802, sizeof(packet_0802_t));
    ndpb += sizeof(packet_0802_t);
    pbuffind += sizeof(packet_0802_t) / 2;

    /* Compute the first I and J coordinates for the melting layer bottom (beam center) */ 
    RPGCS_azranelev_to_xy(ML_range[0].r_b, ML_azimuth[0], elev, &x, &y);
/*    RPGCS_azran_to_xy(ML_range[0].r_b, ML_azimuth[0], &x, &y);*/
    packet_0e03.i_start = (short)RPGC_NINT(x * M_TO_QKM);
    packet_0e03.j_start = (short)RPGC_NINT(y * M_TO_QKM);

    /* Save the start coordinates because they also are the end
       coordinates for the last vector.                        */
    savei = packet_0e03.i_start;
    savej = packet_0e03.j_start;

    /* Add the header for the melting layer bottom (beam center) */
    memcpy(&outbuff[pbuffind], &packet_0e03, sizeof(packet_0e03_hdr_t));
    ndpb += sizeof(packet_0e03_hdr_t);
    pbuffind += sizeof(packet_0e03_hdr_t) / 2;

    /* Add the remaining vector points for the melting layer bottom (beam center) */
    for (r = 1; r < Radcount; r++) {
       RPGCS_azranelev_to_xy(ML_range[r].r_b, ML_azimuth[r], elev, &x, &y);
/*       RPGCS_azran_to_xy(ML_range[r].r_b, ML_azimuth[r], &x, &y);*/
       outbuff[pbuffind]   = (short)RPGC_NINT(x * M_TO_QKM);
       outbuff[pbuffind+1] = (short)RPGC_NINT(y * M_TO_QKM);
/**       add_label(ML_range[r].r_b, outbuff[pbuffind], outbuff[pbuffind+1], "MLB");**/
       ndpb += BYTES_PER_VECT;
       pbuffind += SHORTS_PER_VECT;
    } /* end for all radials */

    /* Close the contour by adding the saved I and J from the start point */
    outbuff[pbuffind]   = savei;
    outbuff[pbuffind+1] = savej;
    ndpb += BYTES_PER_VECT;
    pbuffind += SHORTS_PER_VECT;

    /* Set the color which also identifies the contour as melting layer bottom (beam edge) */
    packet_0802.code              = (short)0x0802; /* packet code 0802 for set color */
    packet_0802.indicator         = (short)0x0002; /* Color value indicator          */
    packet_0802.value             = (short)ML_BOT_EDGE; /* Color value                     */
    memcpy(&outbuff[pbuffind], &packet_0802, sizeof(packet_0802_t));
    ndpb += sizeof(packet_0802_t);
    pbuffind += sizeof(packet_0802_t) / 2;

    /* Compute the first I and J coordinates for the melting layer bottom (beam edge) */ 
    RPGCS_azranelev_to_xy(ML_range[0].r_bb, ML_azimuth[0], elev, &x, &y);
/*    RPGCS_azran_to_xy(ML_range[0].r_bb, ML_azimuth[0], &x, &y);*/
    packet_0e03.i_start = (short)RPGC_NINT(x * M_TO_QKM);
    packet_0e03.j_start = (short)RPGC_NINT(y * M_TO_QKM);

    /* Save the start coordinates because they also are the end
       coordinates for the last vector.                        */
    savei = packet_0e03.i_start;
    savej = packet_0e03.j_start;

    /* Add the header for the melting layer bottom (beam edge) */
    memcpy(&outbuff[pbuffind], &packet_0e03, sizeof(packet_0e03_hdr_t));
    ndpb += sizeof(packet_0e03_hdr_t);
    pbuffind += sizeof(packet_0e03_hdr_t) / 2;

    /* Add the remaining vector points for the melting layer bottom (beam edge) */
    for (r = 1; r < Radcount; r++) {
       RPGCS_azranelev_to_xy(ML_range[r].r_bb, ML_azimuth[r], elev, &x, &y);
/*       RPGCS_azran_to_xy(ML_range[r].r_bb, ML_azimuth[r], &x, &y);*/
       outbuff[pbuffind]   = (short)RPGC_NINT(x * M_TO_QKM);
       outbuff[pbuffind+1] = (short)RPGC_NINT(y * M_TO_QKM);
       ndpb += BYTES_PER_VECT;
       pbuffind += SHORTS_PER_VECT;
    } /* end for all radials */

    /* Close the contour by adding the saved I and J from the start point */
    outbuff[pbuffind]   = savei;
    outbuff[pbuffind+1] = savej;
    ndpb += BYTES_PER_VECT;
    pbuffind += SHORTS_PER_VECT;

    /* Length of symbology data layer. */
    RPGC_set_product_int( &sym->data_len, ndpb);

    /* Append the contour labels in a second symbology block layer */
/**    RPGC_log_msg(GL_INFO,"Number of labels: %d",Num_labels);
    secondbytecnt = 0;
    if (Num_labels > 0) {
       sym->n_layers++;
       outbuff[pbuffind] = 0xFFFF;
       secondbytecnt = Num_labels * BYTES_PER_LABEL;
       RPGC_set_product_int(&(outbuff[pbuffind+1]),secondbytecnt);
       ndpb += 6;      
       pbuffind += 3;
       for (cl = 0; cl < Num_labels; cl++) {
          memcpy(&outbuff[pbuffind],&Label_data[cl],BYTES_PER_LABEL);
          ndpb += BYTES_PER_LABEL;
          pbuffind += SHORTS_PER_LABEL;
       }
    RPGC_log_msg(GL_INFO,"ndpb after labels = %d",ndpb);
    RPGC_log_msg(GL_INFO,"second layer length = %d",secondbytecnt);
    }***//* end if Num_labels > 0 */

    /* Elevation index                   */
    phd->elev_ind=(short)elev_ind;

    /* Assign the minimum and maximum data levels to the product header. */
    /* Input melting layer data is in bin number units. Need to convert  */
    /* to height in KFEET.                                               */
    minval = ((float)Mnpos - Offset) / Scale;
    maxval = ((float)Mxpos - Offset) / Scale;

    RPGCS_height_u((float)(minval*250.),METERS,elev,DEG,&minheight,KFEET);
    RPGCS_height_u((float)(maxval*250.),METERS,elev,DEG,&maxheight,KFEET);

    phd->param_4 = (short) RPGC_NINT( minheight );
    phd->param_5 = (short) RPGC_NINT( maxheight );

    /* Length of block. */
    Totalbytes = ndpb + sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, Totalbytes);

    /* Complete the product header. This function call adds the bytes for */
    /* the message header and the product description block (120 bytes)   */
    RPGC_prod_hdr( obrptr, Prod_id, &Totalbytes);

    Totalbytes += 100;

    /* Return to the product generation control routine. */
    return 0;

}/* End of mlprod_end_of_product_processing() */
