/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/09/12 20:33:07 $
 * $Id: BDE_build_product.c,v 1.6 2008/09/12 20:33:07 cmn Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <basedata.h>
#include <basedata_elev.h>
#include <a309.h>
#include <orpg.h>

static int Num_radials = 0;

/***********************************************************************

   Description: 
      Builds the Base data elevation product.

   Inputs:
      radial - input radial data.

   Outputs:
      outbuf - elevation product containing input radial.
      size - current size, in bytes, of the output buffer.

   Returns:
      There is no return value defined for this function.

   Notes:
 
***********************************************************************/
void BDE_build_product( void *radial, void *outbuf, int *size ){

   Base_data_header *hdr = (Base_data_header *) radial;
   Base_data_radial *rad = (Base_data_radial *) radial;
   Compact_basedata_elev *out = (Compact_basedata_elev *) outbuf;

   int i, offset, size_of_data;
   short rad_status = hdr->status;

   /* If the radial status is beginning of elevation or volume,
      reset the number of radials in product (i.e., initialize
      product buffer to 0s. */
   if( (rad_status == GOODBEL) || (rad_status == GOODBVOL) ){
     
      Num_radials = 0;
      *size = 0;
      memset( out, 0, sizeof(Compact_basedata_elev) );

   }

   /* Copy the radial input buffer header to the output buffer location. */
   memcpy( (void *) &out->radial[Num_radials].bdh, (void *) radial, 
           BASEDATA_HDR_SIZE );

   /* Copy the moment data ....... */

   /* If reflectivity enabled, copy reflectivity data. */
   if( (hdr->msg_type & REF_ENABLED_BIT) ){

      /* The offset is defined relative to the start of the data instead 
         of the start of the radial header. */
      offset = hdr->ref_offset - sizeof(Base_data_header);
      if( offset != 0 )
         offset = (offset+1)/sizeof(short);

      for( i = 0; i < hdr->n_surv_bins; i++ )
         out->radial[Num_radials].radar_data[offset+i] = rad->ref[i];

      /* Redefine the offset to the reflectivity in the radial header. */
      out->radial[Num_radials].bdh.ref_offset = offset;

   }

   /* If the velocity enabled, copy velocity data. */
   if( (hdr->msg_type & VEL_ENABLED_BIT) ){

      /* The offset is defined relative to the start of the data instead 
         of the start of the radial header. */
      offset = hdr->vel_offset - sizeof(Base_data_header);
      if( offset != 0 )
         offset = (offset+1)/sizeof(short);

      for( i = 0; i < hdr->n_dop_bins; i++ )
         out->radial[Num_radials].radar_data[offset+i] = rad->vel[i];

      /* Redefine the offset to the velocity in the radial header. */
      out->radial[Num_radials].bdh.vel_offset = offset;

   }

   /* If the spectrum width enabled, copy spectrum width data. */
   if( (hdr->msg_type & WID_ENABLED_BIT) ){

      /* The offset is defined relative to the start of the data instead 
         of the start of the radial header. */
      offset = hdr->spw_offset - sizeof(Base_data_header);
      if( offset != 0 )
         offset = (offset+1)/sizeof(short);

      for( i = 0; i < hdr->n_dop_bins; i++ )
         out->radial[Num_radials].radar_data[offset+i] = rad->spw[i];

      /* Redefine the offset to the spectrum width in the radial header. */
      out->radial[Num_radials].bdh.spw_offset = offset;

   }

   /* Set the size of the radial .... size is in bytes. */
   size_of_data = out->radial[Num_radials].bdh.msg_len - sizeof(Base_data_header)/sizeof(short);
   out->radial[Num_radials].bdh.msg_len = sizeof(Base_data_header) + size_of_data;

   /* Increment the size of the output buffer. */
   *size += sizeof(Compact_radial);

   /* Increment the number of radials this elevation. */
   Num_radials++;

   /* If the radial status is end of elevation or end of volume, then
      set fields in the elevation product header. */
   if( (rad_status == GENDEL) || (rad_status == GENDVOL) ){

      out->num_radials = Num_radials;
      out->type = hdr->msg_type;
      out->elev_ind = hdr->rpg_elev_ind;

   }

   return;

/* End of BDE_build_product() */
}
