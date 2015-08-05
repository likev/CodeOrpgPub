/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:51 $
 * $Id: hci_clutter_bypass_map_functions.c,v 1.25 2009/02/27 22:25:51 ccalvert Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_clutter_bypass_map_editor.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      related to the Clutter Bypass Map.		*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_clutter_bypass_map.h>

static RDA_bypass_map_msg_t Bypass_map; /* local copy of bypass map data */
static ORDA_bypass_map_msg_t ORDA_Bypass_map; /* local copy of bypass map data*/
static int rda_config_flag = -1;

/************************************************************************
 *	Description: This function initializes the local bypass map	*
 *		     by reading the contents of the edit version of the	*
 *		     clutter bypass map.  If there is an error reading	*
 *		     the edit version or if there are no segments	*
 *		     defined, then read the baseline bypass map and use	*
 *		     it to initialize the edit version.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Initialization status: error if <= 0; OK if > 0		*
 ************************************************************************/

int
hci_clutter_bypass_map_initialize( int config_flag )
{
  int status = -1;

/* Set rda_config_flag variable. */

  rda_config_flag = config_flag;

/*	Initialize the edit buffer by reading bypass map data from	*
 *	edit version of the RDA bypass map LB.				*/

  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                          (char *) &Bypass_map,
                          sizeof (RDA_bypass_map_msg_t),
                          LBID_BYPASSMAP_LGCY);
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                          (char *) &ORDA_Bypass_map,
                          sizeof (ORDA_bypass_map_msg_t),
                          LBID_BYPASSMAP_ORDA);
  }

/*	If there was an error or if there are no defined segments, read	*
 *	the baseline version and write it back to the edit version.	*/

  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    if ((status <= 0) ||
        ((int) Bypass_map.bypass_map.num_segs <= 0) ||
        ((int) Bypass_map.msg_hdr.julian_date <= 0))
    {
      HCI_LE_log("Using baseline for initialization: status %d -- num_segs: %d",
                   status, Bypass_map.bypass_map.num_segs);

      status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                            (char *) &Bypass_map,
                            sizeof (RDA_bypass_map_msg_t), LBID_BYPASSMAP_LGCY);
      if (status < 0)
      {
        HCI_LE_error("Error reading baseline clutter bypass map: %d",
                     status);
        return (status);
      }
      else
      {
        if (Bypass_map.bypass_map.num_segs <= 0)
        {
          HCI_LE_log("Warning: Number of segments %d.  Setting to 2",
                       Bypass_map.bypass_map.num_segs);

          Bypass_map.bypass_map.num_segs = 2;
          Bypass_map.bypass_map.segment[0].seg_num = 1;
          Bypass_map.bypass_map.segment[1].seg_num = 2;
        }
      }
    }

    if ((int) Bypass_map.msg_hdr.julian_date <= 0)
    {
      status = 0;
    }
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    if ((status <= 0) ||
        ((int) ORDA_Bypass_map.bypass_map.num_segs <= 0) ||
        ((int) ORDA_Bypass_map.msg_hdr.julian_date <= 0))
    {
      HCI_LE_log("Using baseline for initialization: status %d -- num_segs: %d",
                   status, ORDA_Bypass_map.bypass_map.num_segs);

      status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                           (char *) &ORDA_Bypass_map,
                           sizeof (ORDA_bypass_map_msg_t), LBID_BYPASSMAP_ORDA);
      if (status < 0)
      {
        HCI_LE_error("Error reading baseline clutter bypass map: %d",
                     status);
        return (status);
      }
      else
      {
        if (ORDA_Bypass_map.bypass_map.num_segs <= 0)
        {
          HCI_LE_log("Warning: Number of segments %d.  Setting to 2",
                       ORDA_Bypass_map.bypass_map.num_segs);

          ORDA_Bypass_map.bypass_map.num_segs = 2;
          ORDA_Bypass_map.bypass_map.segment[0].seg_num = 1;
          ORDA_Bypass_map.bypass_map.segment[1].seg_num = 2;
        }
      }
    }

    if ((int) ORDA_Bypass_map.msg_hdr.julian_date <= 0)
    {
      status = 0;
    }
  }

  return status;
}

/************************************************************************
 *	Description: This function returns the value of the local	*
 *		     bypass map at the specifid azimuth and range.  The	*
 *		     return value is either 0 (clutter) or 1 (no	*
 *		     clutter).						*
 *									*
 *	Input:  azimuth - Azimuth (deg) angle of location		*
 *		range	- Range (km) of location			*
 *		segment - Segment number				*
 *	Output: NONE							*
 *	Return: value (0 or 1)						*
 ************************************************************************/

int
hci_get_bypass_map_data (
  float azimuth,
  float range,
  int segment)
{
  int aindex = -1;
  int rindex;
  int word;
  int bit;
  int num = -1;

/*	Convert the actual azimuth angle into an azimuth index into	*
 *	the Bypass map structure.  There are 256 azimuth (representing	*
 *	360 degrees).							*/

  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    aindex = (int) (((azimuth + AZINT) * 256.0) / 360.0);
    if (aindex < 0)
    {
      aindex = 0;
    }
    else if (aindex > 255)
    {
      aindex = 0;
    }
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    aindex = (int) (azimuth + ORDA_AZINT);
    if (aindex < 0)
    {
      aindex = 0;
    }
    else if (aindex > 359)
    {
      aindex = 0;
    }
  }

  rindex = (int) range;

/*	Determine the halfword for the defined azimuth which contains	*
 *	the cell.							*/
  word = rindex / 16;

/*	Determine the bit within the halfword containing the cell.	*/
  bit = 15 - rindex % 16;

/*	Determine the index of the halfword in the bypass map structure	*
 *	conaining the cell.						*/
  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    num =
      (Bypass_map.bypass_map.segment[segment].data[aindex][word] >> bit) & 1;
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    num =
      (ORDA_Bypass_map.bypass_map.segment[segment].
       data[aindex][word] >> bit) & 1;
  }

  return num;
}

/************************************************************************
 *	Description: This function sets the value at the specified	*
 *		     azimuth and range to a new value.			*
 *									*
 *	Input:  azimuth - Azimuth (deg) angle of location		*
 *		range	- Range (km) of location			*
 *		segment - Segment number				*
 *		value   - new value (0 or 1)				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void
hci_set_bypass_map_data (
  float azimuth,
  float range,
  int segment,
  int value)
{
  int aindex = -1;
  int rindex;
  int word;
  int bit;

/*	Convert the actual azimuth angle into an azimuth index into	*
 *	the Bypass map structure.  There are 256 azimuth (representing	*
 *	360 degrees).							*/

  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    aindex = (int) (((azimuth + AZINT) * 256.0) / 360.0);
    if (aindex < 0)
    {
      aindex = 0;
    }
    else if (aindex > 255)
    {
      aindex = 0;
    }
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    aindex = (int) (((azimuth + ORDA_AZINT) * 360.0) / 360.0);
    if (aindex < 0)
    {
      aindex = 0;
    }
    else if (aindex > 360)
    {
      aindex = 0;
    }
  }

  rindex = (int) range;

/*	Determine the halfword for the defined azimuth which contains	*
 *	the cell.							*/
  word = rindex / 16;

/*	Determine the bit within the halfword containing the cell.	*/
  bit = 15 - rindex % 16;

/*	Determine the index of the halfword in the bypass map structure	*
 *	conaining the cell.						*/
  if (value)
  {

/*	    Set the cell by first shifting to the proper bit and then	*
 *	    "or" the result with the halfword containing the cell.	*/
    if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
    {
      Bypass_map.bypass_map.segment[segment].data[aindex][word] =
        Bypass_map.bypass_map.segment[segment].
        data[aindex][word] | (ONE << bit);
    }
    else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
    {
      ORDA_Bypass_map.bypass_map.segment[segment].data[aindex][word] =
        ORDA_Bypass_map.bypass_map.segment[segment].
        data[aindex][word] | (ONE << bit);
    }
  }
  else
  {

/*	    Clear the cell by first shifting to the proper bit, taking	*
 *	    the one complement, and then "and" the result with the	*
 *	    the halfword containing the cell.				*/
    if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
    {
      Bypass_map.bypass_map.segment[segment].data[aindex][word] =
        Bypass_map.bypass_map.segment[segment].
        data[aindex][word] & (~(ONE << bit));
    }
    else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
    {
      ORDA_Bypass_map.bypass_map.segment[segment].data[aindex][word] =
        ORDA_Bypass_map.bypass_map.segment[segment].
        data[aindex][word] & (~(ONE << bit));
    }
  }
}

/************************************************************************
 *	Description: This function reads the local clutter bypass map	*
 *		     from the specified file.				*
 *									*
 *	Input:  id - input clutter map id				*
 *	Output: NONE							*
 *	Return: status of read; error if <= 0, OK if > 0		*
 ************************************************************************/

int
hci_clutter_bypass_map_read (
  int id)
{
  int status;

  if (id == LBID_BYPASSMAP_LGCY)
  {
    status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                          (char *) &Bypass_map,
                          sizeof (RDA_bypass_map_msg_t), id);
  }
  else if( id == LBID_BYPASSMAP_ORDA )
  {
    status = ORPGDA_read (ORPGDAT_CLUTTERMAP,
                          (char *) &ORDA_Bypass_map,
                          sizeof (ORDA_bypass_map_msg_t), id);
  }
  else
  {
    status = -1;
  }

  if (status <= 0)
  {
    HCI_LE_error("Error reading clutter bypass map %d: %d", id, status);
  }

  return status;
}

/************************************************************************
 *	Description: This function returns the number of segments in	*
 *	the local clutter bypass map.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Number of segments in local bypass map			*
 ************************************************************************/

int
hci_clutter_bypass_map_segments ()
{
  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    return Bypass_map.bypass_map.num_segs;
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    return ORDA_Bypass_map.bypass_map.num_segs;
  }

  return -1; /* make SUN compiler happy */
}

/************************************************************************
 *	Description: This function returns the date from the bypass	*
 *		     map msg.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: The julian date (from Jan 1, 1970) the message was	*
 *		created.						*
 ************************************************************************/

int
hci_get_bypass_map_date ()
{
  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    return (int) Bypass_map.bypass_map.date;
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    return (int) ORDA_Bypass_map.bypass_map.date;
  }

  return -1; /* make SUN compiler happy */
}

/************************************************************************
 *	Description: This function returns the time from the bypass	*
 *		     map msg.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: The time (in minutes after midnight) bypass map was	*
 *		created.						*
 ************************************************************************/

int
hci_get_bypass_map_time ()
{
  if (rda_config_flag == ORPGRDA_LEGACY_CONFIG)
  {
    return (int) Bypass_map.bypass_map.time;
  }
  else if (rda_config_flag == ORPGRDA_ORDA_CONFIG)
  {
    return (int) ORDA_Bypass_map.bypass_map.time;
  }

  return -1; /* make SUN compiler happy */
}
