 /*
  * RCS info
  * $Author: ccalvert $
  * $Locker:  $
  * $Date: 2009/02/27 22:25:52 $
  * $Id: hci_clutter_censor_zones_legacy.h,v 1.3 2009/02/27 22:25:52 ccalvert Exp $
  * $Revision: 1.3 $
  * $State: Exp *
  */

/*	hci_clutter_censor_zones.h - This header file defines		*
 *	constants used by the HCI for clutter censor zone data.		*/

#ifndef HCI_CLUTTER_CENSOR_ZONES_DEF
#define	HCI_CLUTTER_CENSOR_ZONES_DEF

#define	HCI_CCZ_START_AZIMUTH		0
#define	HCI_CCZ_STOP_AZIMUTH		1
#define	HCI_CCZ_START_RANGE		2
#define	HCI_CCZ_STOP_RANGE		3
#define	HCI_CCZ_SEGMENT			4
#define	HCI_CCZ_SELECT_CODE		5
#define	HCI_CCZ_DOPPL_LEVEL		6
#define	HCI_CCZ_SURV_LEVEL		7

#define	HCI_CCZ_AZI_SCALE	(256.0/360.0)

#define	HCI_CCZ_RADIALS		360
#define	HCI_CCZ_GATES		512

#define	HCI_CCZ_FILTER_NONE	  0
#define	HCI_CCZ_FILTER_BYPASS	  1
#define	HCI_CCZ_FILTER_ALL	  2

#define	HCI_CCZ_SEGMENT_LOW	  1
#define	HCI_CCZ_SEGMENT_HIGH	  2

#define	HCI_CCZ_SURV_CHANNEL	  1
#define	HCI_CCZ_DOPL_CHANNEL	  2

#define	HCI_CCZ_MIN_RANGE	  2
#define	HCI_CCZ_MAX_RANGE	510

#define	HCI_CCZ_MIN_AZIMUTH	  0
#define	HCI_CCZ_MAX_AZIMUTH	360

/*	Include files needed.						*/

#include <clutter.h>

typedef struct {

	float	start_azimuth;
	float	stop_azimuth;
	float	start_range;
	float	stop_range;
	int	segment;
	int	select_code;
	int	dopl;
	int	surv;

} Hci_clutter_data_t;

void	hci_build_clutter_map (unsigned char		*clutter_map,
				Hci_clutter_data_t	*data,
			       int			 segment,
			       int			 regions);
int	hci_read_clutter_regions_file  ();
int	hci_write_clutter_regions_file ();
int	hci_get_clutter_region_regions (int file);
void	hci_set_clutter_region_regions (int file,
				int regions);
int	hci_get_clutter_region_data (int file,
				int region,
				int element);
int	hci_set_clutter_region_data (int file,
				int region,
				int element,
				int value);
char	*hci_get_clutter_region_file_label (int file);
int	hci_set_clutter_region_file_label (int file,
				char *label);
int	hci_get_clutter_region_download_time ();
int	hci_set_clutter_region_download_time ();
int	hci_get_clutter_region_download_file ();
int	hci_set_clutter_region_download_file ();
int	hci_get_clutter_region_file_time (int file);
int	hci_set_clutter_region_file_time (int file,
				int tm);
int	hci_download_clutter_regions_file (int file);
RPG_clutter_regions_t	*hci_get_clutter_regions_data_ptr (int file);

int	hci_delete_clutter_regions_file (int file);

#endif
