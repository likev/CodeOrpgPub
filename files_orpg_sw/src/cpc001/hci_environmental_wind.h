/*	hci_environmental_wind.h - This header file defines s 		*
 *	constant and function definitions for environmental wind.	*/

#ifndef HCI_ENVIRONMENTAL_WIND_DEF
#define	HCI_ENVIRONMENTAL_WIND_DEF

#include <itc.h>
#include <hci_consts.h>

#define	DISPLAY_CURRENT	1
#define	DISPLAY_MODEL	2

int	hci_read_environmental_wind_data ();
int	hci_write_environmental_wind_data ();
int	hci_set_vad_update_flag (int state);
int	hci_get_vad_update_flag ();
A3cd97	*hci_get_environmental_wind_data_ptr ();
int	hci_get_ewt_display_flag ();
void	hci_set_ewt_display_flag (int flag);
int	hci_get_model_update_flag ();
int	hci_set_model_update_flag (int flag);

#endif
