#ifndef HCI_RDA_PERFORMANCE_H

#define HCI_RDA_PERFORMANCE_H

#include <hci_consts.h>

enum { GEN_FUEL_LEVEL };

#include <rda_performance_maintenance.h>

/*	Functions dealing with rda performance data	*/

int			hci_initialize_rda_performance_data ();
int			hci_read_rda_performance_data       ();
rda_performance_t	*hci_get_rda_performance_data_ptr   ();
int			hci_get_rda_performance_update_flag ();
void			hci_set_rda_performance_update_flag (int state);
void			hci_request_new_rda_performance_data ();

int			hci_rda_performance_data (int item);
int			hci_rda_performance_data_initialized ();

char			*hci_rda_performance_time ();

#endif
