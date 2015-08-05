/************************************************************************
 *									*
 *	Module:  hci_clutter_censor_zones_funtions.c			*
 *									*
 *	Description:  This file contains a collection of modules	*
 *	that manipulate clutter censor zone data.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/24 16:58:37 $
 * $Id: hci_clutter_censor_zones_functions_orda.c,v 1.9 2012/01/24 16:58:37 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_clutter_censor_zones_orda.h>

/*	Local Constants							*/

static	char	*Clutter_data = NULL;
static	char	*Download_info_data = NULL;

/************************************************************************
 *	Description: This function builds a composite clutter map from	*
 *		     the current clutter regions data buffer.		*
 *									*
 *	Input:  data - pointer to clutter region data			*
 *		segment - segment number				*
 *		regions - number of clutter regions			*
 *	Output: clutter_map - pointer to composite clutter map data	*
 *	Return: NONE							*
 ************************************************************************/

void
hci_build_clutter_map (
unsigned char		*clutter_map,
Hci_clutter_data_t	*data,
int			segment,
int			regions
)
{
  int	i;
  int	j;
  int	start_azimuth;
  int	stop_azimuth;
  int	azimuth;
  int	range;
  unsigned char	select_code;

  /* For each clutter region update the clutter map based on priority. */

  for( i=0; i<=HCI_CCZ_RADIALS; i++ )
  {
    for( j=0; j<=HCI_CCZ_GATES; j++)
    {
      * (clutter_map + i*(HCI_CCZ_GATES+1) + j) = 16;
    }
  }

  for( i=0; i<regions; i++ )
  {
    /* Since there are clutter maps for up to five elevation cuts,
       only process those for the current selected cut. */

    if( segment == data [i].segment )
    {
      /* Check to see if the azimuths for this sector cross 0
         degrees.  If so, add 360 to the stop azimuth. */

      start_azimuth = data [i].start_azimuth;
      stop_azimuth  = data [i].stop_azimuth;

      if( start_azimuth > stop_azimuth )
      {
        stop_azimuth = stop_azimuth + 360;
      }

      /* Process each beam in the clutter region between and
         including the start and stop azimuths. */

      for( j=start_azimuth; j<=stop_azimuth; j++ )
      {
        if( j >=360 ){ azimuth = j-360; }
        else{ azimuth = j; }

        /* For each gate in the sector determine if the
           filter priority is at or higher than the current
           value in the clutter map.  If it is, then change
           the contents of the	clutter map. */

        for( range =data[i].start_range; range<=data[i].stop_range; range++ )
        {
          select_code = ((*(clutter_map + azimuth*(HCI_CCZ_GATES+1) + range)) >> 4) & 3;

          if( ((data [i].select_code == HCI_CCZ_FILTER_NONE) &&
              (select_code == HCI_CCZ_FILTER_BYPASS)) ||
              (data [i].select_code == HCI_CCZ_FILTER_ALL) ||
              (data [i].select_code == select_code) )
          {
            if( data [i].select_code == select_code )
            {
              select_code = data [i].select_code<<4;
            }
            else
            {
              select_code = data [i].select_code<<4;
            }

            *(clutter_map + azimuth*(HCI_CCZ_GATES+1) + range) = select_code;
          } 
        }
      }
    }
  }
}

/************************************************************************
 *	Description: This function write the local clutter regions data	*
 *		     buffer to the Clutter LB.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: write status (negative indicates error)			*
 ************************************************************************/

int
hci_write_clutter_regions_file()
{
  int	status = 0;

  HCI_LE_status("Updating clutter censor zones data");

  if( Clutter_data != NULL )
  {
    status = ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, Clutter_data,
                 sizeof (ORPG_clutter_regions_msg_t), ORPGCCZ_DEFAULT );
  }

  if( (status <= 0) || (Clutter_data == NULL) )
  {
    HCI_LE_error("ORPGCCZ_set_censor_zones of ORPGCCZ_ORDA_ZONES failed: %dn", status);
  }

  return status;
}

/************************************************************************
 *	Description: This function reads the local clutter regions data	*
 *		     buffer from the Clutter LB.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: read status (negative indicates error)			*
 ************************************************************************/

int
hci_read_clutter_regions_file()
{
  int	status;
	
  /* Check to see if clutter data previous read. If so, need to
     free memory allocated to it. */

  if( Clutter_data != NULL )
  {
    free( Clutter_data );
    Clutter_data = NULL;
  }

  status = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES,
                    (char **) &Clutter_data, ORPGCCZ_DEFAULT );

  return status;
}

/************************************************************************
 *	Description: This function returns a pointer to the clutter	*
 *		     regions data for the specified file in the clutter	*
 *		     regions data buffer.				*
 *									*
 *	Input:  file_index - index of file in data buffer		*
 *	Output: NONE							*
 *	Return: pointer to clutter regions file data			*
 ************************************************************************/

ORPG_clutter_regions_t *
hci_get_clutter_regions_data_ptr( int file_index )
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_regions_data_ptr: Clutter_data is NULL" );
    return NULL;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return (ORPG_clutter_regions_t *) &clutter->file [file_index].regions;
}

/************************************************************************
 *	Description: This function gets the number of defined regions	*
 *		     in a specified clutter regions data file.		*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: number of regions in file				*
 ************************************************************************/

int
hci_get_clutter_region_regions( int file )
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region_regions: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return (int) clutter->file [file].regions.regions;
}

/************************************************************************
 *	Description: This function sets the number of defined regions	*
 *		     in a specified clutter regions data file.		*
 *									*
 *	Input:  file - file index					*
 *		regions - number of regions in file			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_set_clutter_region_regions( int file, int regions )
{
  time_t	tm;
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region_regions: Clutter_data is NULL" );
    return;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  clutter->file [file].regions.regions = regions;

  tm = time(NULL);

  clutter->file [file].file_id.time = tm;
}

/************************************************************************
 *	Description: This function returns a specified data element	*
 *		     from a clutter regions sector and file.		*
 *									*
 *	Input:  file - file index					*
 *		region - region index					*
 *		element - HCI_CCZ_START_AZIMUTH				*
 *			  HCI_CCZ_STOP_AZIMUTH				*
 *			  HCI_CCZ_START_RANGE				*
 *			  HCI_CCZ_STOP_RANGE				*
 *			  HCI_CCZ_SELECT_CODE				*
 *			  HCI_CCZ_SEGMENT				*
 *	Output: NONE							*
 *	Return: element value						*
 ************************************************************************/

int
hci_get_clutter_region_data( int file, int region, int element )
{
  int	num;
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region_data: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  switch( element )
  {
    case HCI_CCZ_START_AZIMUTH :

      num = clutter->file [file].regions.data [region].start_azimuth;
      break;

    case HCI_CCZ_STOP_AZIMUTH :

      num = clutter->file [file].regions.data [region].stop_azimuth;
      break;

    case HCI_CCZ_START_RANGE :

      num = clutter->file [file].regions.data [region].start_range;
      break;

    case HCI_CCZ_STOP_RANGE :

      num = clutter->file [file].regions.data [region].stop_range;
      break;

    case HCI_CCZ_SELECT_CODE :

      num = clutter->file [file].regions.data [region].select_code;
      break;

    case HCI_CCZ_SEGMENT :

      num = clutter->file [file].regions.data [region].segment;
      break;

    default :

      num = -1;
  }

  return num;
}

/************************************************************************
 *	Description: This function sets a specified data element	*
 *		     in a clutter regions file.				*
 *									*
 *	Input:  file - file index					*
 *		region - region index					*
 *		element - HCI_CCZ_START_AZIMUTH				*
 *			  HCI_CCZ_STOP_AZIMUTH				*
 *			  HCI_CCZ_START_RANGE				*
 *			  HCI_CCZ_STOP_RANGE				*
 *			  HCI_CCZ_SELECT_CODE				*
 *			  HCI_CCZ_SEGMENT				*
 *		num - new element value
 *	Output: NONE							*
 *	Return: element value or -1 on error				*
 ************************************************************************/

int
hci_set_clutter_region_data( int file, int region, int element, int num )
{
  time_t	tm;
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region_data: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  switch( element )
  {
    case HCI_CCZ_START_AZIMUTH :

      clutter->file [file].regions.data [region].start_azimuth = num;
      break;

    case HCI_CCZ_STOP_AZIMUTH :

      clutter->file [file].regions.data [region].stop_azimuth = num;
      break;

    case HCI_CCZ_START_RANGE :

      clutter->file [file].regions.data [region].start_range = num;
      break;

    case HCI_CCZ_STOP_RANGE :

      clutter->file [file].regions.data [region].stop_range = num;
      break;

    case HCI_CCZ_SELECT_CODE :

      clutter->file [file].regions.data [region].select_code = num;
      break;

    case HCI_CCZ_SEGMENT :

      clutter->file [file].regions.data [region].segment = num;
      break;

    default :

      num = -1;
  }

  /* If something was modified, update the file tag time field	*/

  if( num >= 0 )
  {
    tm = time(NULL);;
    clutter->file [file].file_id.time = tm;
  }

  return num;
}

/************************************************************************
 *	Description: This function sets the label tag associated with	*
 *		     a clutter regions data file.			*
 *									*
 *	Input:  file - file index					*
 *		label - pointer to string containing new tag		*
 *	Output: NONE							*
 *	Return: length of tag						*
 ************************************************************************/

int
hci_set_clutter_region_file_label( int file, char *label )
{
  int	len;
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region_file label: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  /* Get the length of the input string. */

  len = strlen(label);

  /* If the input string larger than the maximum allowed,
     truncate it appropriately. */

  if( len > MAX_LABEL_SIZE )
  {
    len = MAX_LABEL_SIZE;
  }

  strcpy( clutter->file [file].file_id.label, label );

  return len;
}

/************************************************************************
 *	Description: This function returns a pointer to the label tag	*
 *		     for the specified clutter regions data file.	*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: pointer to tag string					*
 ************************************************************************/

char
*hci_get_clutter_region_file_label( int file )
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region_file label: Clutter_data is NULL" );
    return NULL;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return (char *) clutter->file [file].file_id.label;
}

/************************************************************************
 *	Description: This function gets the time the last clutter	*
 *		     regions file was downloaded from the HCI.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: time (julian seconds) of last clutter regions download	*
 ************************************************************************/

int
hci_get_clutter_region_download_time()
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region_download_time: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return (int) clutter->last_dwnld_time;
}

/************************************************************************
 *	Description: This function sets the time of the last HCI	*
 *		     downloaded clutter regions file to the current	*
 *		     system time.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: time (julian seconds) of last clutter regions download	*
 ************************************************************************/

int
hci_set_clutter_region_download_time()
{
  time_t	tm;
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region_download_time: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  tm = time(NULL);

  clutter->last_dwnld_time = tm;

  return (int) tm;
}

/************************************************************************
 *	Description: This function gets the time the specified clutter	*
 *		     regions file was last modified.			*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: time (julian seconds)					*
 ************************************************************************/

int
hci_get_clutter_region_file_time( int file )
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region_file_time: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return (int) clutter->file [file].file_id.time;
}

/************************************************************************
 *	Description: This function sets the time the specified clutter	*
 *		     regions file was last modified.			*
 *									*
 *	Input:  file - file index					*
 *		tm - time (unix time in julian seconds)			*
 *	Output: NONE							*
 *	Return: time (tm)						*
 ************************************************************************/

int
hci_set_clutter_region_file_time( int file, int tm )
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region_file_time: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  clutter->file [file].file_id.time = tm;

  return tm;
}


/************************************************************************
 *	Description: This function updates the download tags in the	*
 *		     clutter regions data and issues a download command	*
 *		     to the rda control task.  If it returns a number	*
 *		     greater than 0, then the update was successful	*
 *		     (doesn't mean that rda control was successful).	*
 *		     If not, then an error occurred while writing	*
 *		     the clutter data to the clutter linear buffer.	*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: write/command status (negative indicates error)		*
 ************************************************************************/

int
hci_download_clutter_regions_file (
int	file
)
{
	int	status;
	ORPG_clutter_regions_msg_t	*clutter;
	char	buf [80];

        if( Clutter_data == NULL )
        {
          HCI_LE_error( "hci_set_clutter_region_file_time: Clutter_data is NULL" );
          return -1;
        }

	clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

	hci_set_clutter_region_download_time ();
	hci_set_clutter_region_download_file (file);

	status = hci_write_clutter_regions_file ();

/*	Request that the updated clutter censor zone data be	*
 *	downloaded to the RDA.					*/

	if (status > 0) {

	    sprintf (buf,"Downloading new Clutter Censor Zones");

	    HCI_display_feedback( buf );

	    ORPGRDA_send_cmd (COM4_SENDCLCZ,
			(int) HCI_INITIATED_RDA_CTRL_CMD,
			(int) clutter->file [file].regions.regions,
			file,
			(int) 0,
			(int) 0,
			(int) 0,
			NULL);

	    if (HCI_get_system() == HCI_FAA_SYSTEM) {

		if (ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL) != ORPGRED_CHANNEL_ACTIVE) {

		    Redundant_cmd_t	red_cmd;

		    red_cmd.cmd        = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
		    red_cmd.lb_id      = 0;
		    red_cmd.msg_id     = 0;
		    red_cmd.parameter1 = clutter->file [file].regions.regions;
		    red_cmd.parameter2 = file;

		    status = ORPGRED_send_msg (red_cmd);

		    if (status != 0) {

			HCI_LE_error("Error downloading clutter regions to redundant channel: %d", status);

		    } else {

			HCI_LE_status("Clutter regions downloaded to redundant channel");

		    }
		}

	    } else if (HCI_get_system() == HCI_NWSR_SYSTEM) {

		Redundant_cmd_t	red_cmd;

		red_cmd.cmd        = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
		red_cmd.lb_id      = 0;
		red_cmd.msg_id     = 0;
		red_cmd.parameter1 = clutter->file [file].regions.regions;
		red_cmd.parameter2 = file;

		status = ORPGRED_send_msg (red_cmd);

		if (status != 0) {

		    HCI_LE_error("Error downloading clutter regions to redundant channel: %d", status);

		} else {

		    HCI_LE_status("Clutter regions downloaded to redundant channel");

		}
	    }

	} else {

	    HCI_LE_error("hci_write_clutter_regions_file() failed: %d", status);

	}

	return status;

}

/************************************************************************
 *	Description: This function deletes a clutter regions file by	*
 *		     setting the label tag to NULL and setting the	*
 *		     number of regions to 0.				*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: write status (negative indicates error)			*
 ************************************************************************/

int
hci_delete_clutter_regions_file( int file )
{
  int	status;
  time_t	tm;
  ORPG_clutter_regions_msg_t	*clutter;

  if( (file <  0) || (file >= MAX_CLTR_FILES) )
  {
    status = -1;
  }
  else
  {
    if( Clutter_data == NULL )
    {
      HCI_LE_error( "hci_delete_clutter_region: Clutter_data is NULL" );
      return -1;
    }

    clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

    hci_set_clutter_region_file_label(file,"");
    hci_set_clutter_region_regions(file, 0);

    /* Update the files modification time so we can tell later
       when it was deleted. */

    tm = time(NULL);

    clutter->file [file].file_id.time = tm;

    status = hci_write_clutter_regions_file();
  }

  return status;
}

/************************************************************************
 *	Description: This function gets the index of the last HCI	*
 *		     downloaded clutter regions data file.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: file index						*
 ************************************************************************/

int
hci_get_clutter_region_download_file()
{
  ORPG_clutter_regions_msg_t	*clutter;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_get_clutter_region download file: Clutter_data is NULL" );
    return -1;
  }

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  return clutter->last_dwnld_file;
}

/************************************************************************
 *	Description: This function sets the index of the last HCI	*
 *		     downloaded clutter regions data file.		*
 *									*
 *	Input:  file - file index					*
 *	Output: NONE							*
 *	Return: file index						*
 ************************************************************************/

int
hci_set_clutter_region_download_file( int file )
{
  ORPG_clutter_regions_msg_t	*clutter;

  clutter = (ORPG_clutter_regions_msg_t *) Clutter_data;

  if( Clutter_data == NULL )
  {
    HCI_LE_error( "hci_set_clutter_region download file: Clutter_data is NULL" );
    return -1;
  }

  clutter->last_dwnld_file = file;

  return file;
}

int
hci_get_clutter_region_num_files()
{
  int i, cnt = 0;
  char *file_buf = NULL;

  for( i=0; i<MAX_CLTR_FILES; i++ )
  {
    if( ( file_buf = hci_get_clutter_region_file_label( i ) ) == NULL )
    {
      HCI_LE_error( "hci_get_clutter_region_num_files: file_buf (%d) is NULL", i );
      continue;
    }
    if( strlen( hci_get_clutter_region_file_label( i ) ) != 0 )
    {
      cnt++;
    }
  }

  return cnt;
}

/************************************************************************
 *	Description: This function reads information regarding the last	*
 *		     Clutter File downloaded to the RDA.		*
 *									*
 *	Return: read status (negative indicates error)			*
 ************************************************************************/

int
hci_read_clutter_download_info()
{
  /* Check to see if download info previously read. If so, need to
     free memory allocated to it. */

  if( Download_info_data != NULL )
  {
    free( Download_info_data );
    Download_info_data = NULL;
  }

 return ORPGCCZ_get_download_info( (char **) &Download_info_data );
}

/************************************************************************
 *	Description: This function returns the UNIX time of the last	*
 *		     clutter file downloaded to the RDA.		*
 *									*
 *	Return: integer UNIX time					*
 ************************************************************************/

int
hci_get_clutter_download_time( int channel_number )
{
  Clutter_region_download_info_t *download_info;

  if( Download_info_data == NULL ){ return -1; }

  download_info = (Clutter_region_download_info_t *) Download_info_data;

  if( channel_number == 2 )
  {
    return download_info->channel[CCZ_CHANNEL_2].last_download_time;
  }
  return download_info->channel[CCZ_CHANNEL_1].last_download_time;
}

/************************************************************************
 *	Description: This function returns the file index of the last	*
 *		     downloaded clutter file to the RDA.		*
 *									*
 *	Return: integer file index					*
 ************************************************************************/

int
hci_get_clutter_download_file_index( int channel_number )
{
  Clutter_region_download_info_t *download_info;

  if( Download_info_data == NULL ){ return -1; }

  download_info = (Clutter_region_download_info_t *) Download_info_data;

  if( channel_number == 2 )
  {
    return download_info->channel[CCZ_CHANNEL_2].last_download_file;
  }
  return download_info->channel[CCZ_CHANNEL_1].last_download_file;
}

/************************************************************************
 *	Description: This function returns the number of regions from	*
 *		     the last downloaded clutter file to the RDA.	*
 *									*
 *	Return: integer number of regions				*
 ************************************************************************/

int
hci_get_clutter_download_num_regions( int channel_number )
{
  Clutter_region_download_info_t *download_info;

  if( Download_info_data == NULL ){ return -1; }

  download_info = (Clutter_region_download_info_t *) Download_info_data;

  if( channel_number == 2 )
  {
    return download_info->channel[CCZ_CHANNEL_2].regions;
  }
  return download_info->channel[CCZ_CHANNEL_1].regions;
}

/************************************************************************
 *	Description: This function returns clutter censor data from the	*
 *		     last downloaded clutter file to the RDA.		*
 *									*
 *	Return: ptr to ORPG_clutter_region_data_t struct		*
 ************************************************************************/

ORPG_clutter_region_data_t *
hci_get_clutter_download_region_data( int channel_number )
{
  Clutter_region_download_info_t *download_info;

  if( Download_info_data == NULL ){ return NULL; }

  download_info = (Clutter_region_download_info_t *) Download_info_data;

  if( channel_number == 2 )
  {
    return download_info->channel[CCZ_CHANNEL_2].data;
  }
  return download_info->channel[CCZ_CHANNEL_1].data;
}

/************************************************************************
 *	Description: This function finds the file name associated with	*
 *		     the file index in the clutter file download info.	*
 *									*
 *	Return: name of file or empty string if file not found		*
 ************************************************************************/

char *
hci_get_clutter_download_file_name( int channel_number )
{
  Clutter_region_download_info_t *download_info;
  ORPG_clutter_regions_t download_regions;
  int file_index;
  int i;

  download_info = (Clutter_region_download_info_t *) Download_info_data;

  if( Download_info_data == NULL ){ return ""; }

  /* Populate struct with info from last downloaded clutter file. */

  if( channel_number == 2 )
  {
    file_index = download_info->channel[CCZ_CHANNEL_2].last_download_file;
    download_regions.regions = download_info->channel[CCZ_CHANNEL_2].regions;
    memcpy( &(download_regions.data),
            (void *)download_info->channel[CCZ_CHANNEL_2].data,
            sizeof( ORPG_clutter_region_data_t )*MAX_NUMBER_CLUTTER_ZONES );
  }
  else
  {
    file_index = download_info->channel[CCZ_CHANNEL_1].last_download_file;
    download_regions.regions = download_info->channel[CCZ_CHANNEL_1].regions;
    memcpy( &(download_regions.data),
            (void *)download_info->channel[CCZ_CHANNEL_1].data,
            sizeof( ORPG_clutter_region_data_t )*MAX_NUMBER_CLUTTER_ZONES );
  }

  /* Find name of last downloaded clutter file. The name is not part
     of the download info, so clutter censor data from the download
     info must be compared to clutter censor data of the current
     clutter files until a match is found. If no match is found, one
     must assume either the file was deleted or a part of the file
     was rejected. Before looping through each clutter file, first
     compare with the file associated with the file index in the
     download info. Most of the time, this should work and no looping
     is needed. */
  
  if( hci_compare_clutter_regions( &download_regions, hci_get_clutter_regions_data_ptr( file_index ) ) )
  {
    return hci_get_clutter_region_file_label( file_index );
  }
  else
  {
    /* Downloaded file and file associated with downloaded file index
       are different (this can happen if the file is deleted, thus
       all other files' index decreases). Loop through the current
       clutter files and see if one matches. */
    for( i=0; i<hci_get_clutter_region_num_files(); i++ )
    {
      if( hci_compare_clutter_regions( &download_regions, hci_get_clutter_regions_data_ptr(i) ) )
      {
        /* Clutter file matches downloaded file. */
        return hci_get_clutter_region_file_label( i );
      }
    }
  }

  /* No match found. */

  return "";
}

/************************************************************************
 *	Description: This function finds the file index associated with	*
 *		     the passed in file name.				*
 *									*
 *	Return: integer file index or -1 if file name not found		*
 ************************************************************************/

int
hci_get_clutter_file_index( char *file_name )
{
  char *file_buf;
  int i;

  /* Stop here for empty or NULL ptrs. */
  if( ( file_name == NULL ) || strlen( file_name ) == 0 ){ return -1; }

  /* Loop through each clutter file until passed in file name
     is found. Return associated file index. */
  for( i=0; i<hci_get_clutter_region_num_files(); i++ )
  {
    file_buf = hci_get_clutter_region_file_label( i );
    if( file_buf == NULL )
    {
      HCI_LE_error( "hci_get_clutter_file_index: file_buf (%d) is NULL", i );
      continue;
    }
    if( strlen( file_buf ) != 0 && strcmp( file_buf, file_name ) == 0 )
    {
      return i;
    }
  }

  /* No match found. */

  return -1;
}

/************************************************************************
 *	Description: This function compares two clutter regions and	*
 *		     determines if they are the same.			*
 *									*
 *	Return: HCI_YES_FLAG if same, HCI_NO_FLAG if different		*
 ************************************************************************/

int
hci_compare_clutter_regions( ORPG_clutter_regions_t *a, ORPG_clutter_regions_t *b )
{
  int i;

  /* Check for NULLs. Also, If number of regions is different,
     then regions are different and no need to proceed. */

  if( a == NULL && b == NULL ){ return HCI_YES_FLAG; }
  else if( a == NULL || b == NULL ){ return HCI_NO_FLAG; }
  else if( a->regions != b->regions ){ return HCI_NO_FLAG; }

  /* Loop through each region, if anything differs, return as such. */

  for( i = 0; i < a->regions; i++ )
  {
    if( ( a->data[i].start_range != b->data[i].start_range ) ||
        ( a->data[i].stop_range != b->data[i].stop_range ) ||
        ( a->data[i].start_azimuth != b->data[i].start_azimuth ) ||
        ( a->data[i].stop_azimuth != b->data[i].stop_azimuth ) ||
        ( a->data[i].segment != b->data[i].segment ) ||
        ( a->data[i].select_code != b->data[i].select_code ) )
    {
      /* Clutter regions do not match. */
      return HCI_NO_FLAG;
    }
  }

  /* Clutter regions match. */

  return HCI_YES_FLAG;
}

