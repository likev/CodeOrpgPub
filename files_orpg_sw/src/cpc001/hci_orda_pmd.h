/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/16 15:25:40 $
 * $Id: hci_orda_pmd.h,v 1.4 2012/11/16 15:25:40 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


#ifndef HCI_PMD_H
#define HCI_PMD_H

#include <hci_consts.h>
#include <orda_pmd.h>

#define	CNVRTD_GEN_FUEL_LEVEL	1
#define	PERF_CHECK_TIME		2

enum { HCI_PC_STATE_AUTO, HCI_PC_STATE_PENDING };
enum { HCI_PC_STATUS_NONPENDING, HCI_PC_STATUS_PENDING };

typedef struct
{
  int status;
  int state;
  int num_hrs;
  int num_mins;
} hci_perfcheck_info_t;

int hci_initialize_orda_pmd ();
int hci_read_orda_pmd ();
orda_pmd_t *hci_get_orda_pmd_ptr ();
int hci_get_orda_pmd_update_flag ();
void hci_set_orda_pmd_update_flag (int state);
void hci_request_new_orda_pmd ();
int hci_orda_pmd (int);
int hci_orda_pmd_initialized ();
char *hci_orda_pmd_time ();
void hci_orda_pmd_unavailable_popup ();
hci_perfcheck_info_t *hci_get_perfcheck_info();

#endif
