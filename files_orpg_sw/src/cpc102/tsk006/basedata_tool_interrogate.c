/************************************************************************
 *									*
 *	Module:  basedata_tool_interrogate.c				*
 *									*
 *	Description: This module is used by the RPG Base Data Display	*
 *		     Tool task to interrogate a pixel in the base data	*
 *	             display window and return the raw data value.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/21 17:03:31 $
 * $Id: basedata_tool_interrogate.c,v 1.1 2011/03/21 17:03:31 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci.h>
#include <basedata_tool.h>

/************************************************************************
 *	Description: This function returns the value at a specified	*
 *		     polar coordinate.					*
 *									*
 *	Input:  mode_flag	- flag to indicate real-time or replay	*
 *		target_azimuth	- azimuth of the location from radar	*
 *		target_range	- range of the location from radar	*
 *		basedata_LB_id	- id of basedata replay LB		*
 *		moment		- Data field to interrogate		*
 *		ptr		- message ID to begin search.		*
 *	Return: data value						*
 ************************************************************************/

float hci_basedata_tool_interrogate( int mode_flag, float target_azimuth,
   float target_range, int moment, int basedata_LB_id, int ptr )
{
  Base_data_header *hdr = NULL;
  LB_id_t lb_id = 0;
  LB_id_t start_id = 0;
  LB_id_t stop_id = 0;
  unsigned int vol_num = 0;
  int status = 0;
  int sub_type = 0;
  int i = 0;
  int range_index = 0;
  int msg_len = 0;
  float value = LOWER_MOMENT_THRESHOLD;
  float diff = 0.0;
  float diff_thresh = 0.0;
  char data[ SIZEOF_BASEDATA ];

  /* If ptr is negative, then return the equivalent of "No Data". */

  if( ptr < 0 ){ return value; }

  /* Ptr is valid. Take action depending on mode. */

  if( mode_flag == DISPLAY_MODE_DYNAMIC )
  {
    /* Make sure the basedata LB isn't locked. If it is,
       return the equivalent of "No Data". */

    if( hci_basedata_tool_get_lock_state() ){ return value; }

    /* Lock the basedata LB so it won't change while being read. */

    hci_basedata_tool_set_lock_state(1);

    /* Save the message ID of the current radial so we
       can restore it later. */

    lb_id = hci_basedata_tool_msgid();

    /* Move the message pointer corresponding to the radial
       closest to the target azimuth. */

    status = hci_basedata_tool_seek(ptr);

    if( status != LB_SUCCESS )
    {
      HCI_LE_error("ERROR LB_seek: %d", status);
    }
    else if( hci_basedata_tool_data_available( moment ) )
    {
      status = hci_basedata_tool_read_radial( LB_NEXT, HCI_BASEDATA_COMPLETE_READ );

      /* If the read is successful, then determine the index
         of the target range and set the data value to return. */

      if( status > 0 )
      {
        range_index = hci_basedata_tool_range_index( target_range*1000, moment );
        if( range_index < 0 ){ value = LOWER_MOMENT_THRESHOLD; }
        else{ value = hci_basedata_tool_value_index( range_index, moment ); } 
      }
    }

    /* Restore the LB pointer to the state prior to entering this routine. */

    status = hci_basedata_tool_seek( lb_id );

    /* Unlock the basedata LB so another task can use it. */

    hci_basedata_tool_set_lock_state( 0 );

  } /* End of mode_flag == DISPLAY_MODE_DYNAMIC */
  else
  {
    vol_num = ORPGVST_get_volume_number();

    if( moment != VELOCITY && moment != SPECTRUM_WIDTH )
    {
      sub_type = (BASEDATA_TYPE | REFLDATA_TYPE);
    }
    else
    {
      sub_type = (BASEDATA_TYPE | COMBBASE_TYPE);
    }

    start_id = ORPGBDR_get_start_of_elevation_msgid( basedata_LB_id,
                 sub_type, vol_num, ptr );

    stop_id = ORPGBDR_get_end_of_elevation_msgid( basedata_LB_id,
                 sub_type, vol_num, ptr );

    if ((start_id <= 0) || (stop_id <= 0))
    {
      vol_num--;

      start_id = ORPGBDR_get_start_of_elevation_msgid( basedata_LB_id,
                 sub_type, vol_num, ptr );

      stop_id = ORPGBDR_get_end_of_elevation_msgid( basedata_LB_id,
                 sub_type, vol_num, ptr );

      if( (start_id <= 0) || (stop_id <= 0) )
      {
        return value;
      }
    }

    if( (start_id > 0) && (stop_id > 0) )
    {
      for( i=start_id; i<stop_id; i++ )
      {
        msg_len = ORPGBDR_read_radial( basedata_LB_id,
                &data[0], SIZEOF_BASEDATA, (LB_id_t) i );

        if( msg_len <= 0 )
        {
          HCI_LE_error("interrogate ORPGBDR failed (%d) for id: %d",
                 msg_len, i );
          continue;
        }

        hdr = ( Base_data_header * ) data;

        diff = fabs( (double) (target_azimuth - hdr->azimuth) );

        if( hdr->azm_reso == BASEDATA_HALF_DEGREE ){ diff_thresh = 0.5; }
        else{ diff_thresh = 1.0; }

        /* If diff is within the threshold tolerance,
           use this radial to find the data value. */

        if( diff < diff_thresh )
        {
          hci_basedata_tool_set_data_ptr( &data[0], msg_len );
          range_index = hci_basedata_tool_range_index( target_range*1000, moment );
          value = hci_basedata_tool_value_index( range_index, moment );
          break;
        } /* End of diff < BEAM_WIDTH */
      } /* End of for loop */
    } /* End of start_id > 0 && stop_id > 0 */
  } /* End of mode_flag != DYNAMIC */

  return value;
}
