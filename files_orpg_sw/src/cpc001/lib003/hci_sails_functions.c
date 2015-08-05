/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/08/04 16:52:22 $
 * $Id: hci_sails_functions.c,v 1.6 2014/08/04 16:52:22 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/************************************************************************
  Module:  hci_sails_functions.c

  Description: This file contains a collection of modules used
               for SAILS control
 ************************************************************************/

/* Local include files */

#include <hci.h>
#include <hci_sails.h>

/* Static/Global variables */

/************************************************************************
  Returns status of SAILS. Values could be:

  HCI_SAILS_DISABLED - zero SAILS cuts (SAILS disabled)
  HCI_SAILS_INACTIVE - SAILS cuts set, but current VCP does not allow SAILS
  HCI_SAILS_ACTIVE   - SAILS cuts set and current VCP allows SAILS
 ************************************************************************/

int hci_sails_get_status()
{
  return ORPGSAILS_get_status();
}

/************************************************************************
  Returns 1 if current VCP allows SAILS, 0 otherwise.
 ************************************************************************/

int hci_sails_allowed()
{
  return ORPGSAILS_allowed();
}

/************************************************************************
  Returns number of SAILS cuts used in the current volume scan.
 ************************************************************************/

int hci_sails_get_num_cuts()
{
  return ORPGSAILS_get_num_cuts();
}

/************************************************************************
  Returns number of SAILS cuts to be used in the next volume scan.
 ************************************************************************/

int hci_sails_get_req_num_cuts()
{
  return ORPGSAILS_get_req_num_cuts();
}

/************************************************************************
  Returns maximum possible number of SAILS cuts.
 ************************************************************************/

int hci_sails_get_max_num_cuts()
{
  return ORPGSAILS_get_max_cuts();
}

/************************************************************************
  Returns maximum possible number of SAILS cuts for the current site.
 ************************************************************************/

int hci_sails_get_site_max_num_cuts()
{
  return ORPGSAILS_get_site_max_cuts();
}

/************************************************************************
  Sets number of SAILS cuts to request for the next volume scan.
  If num_sails_cuts is 0 then SAILS is disabled.
 ************************************************************************/

int hci_sails_set_num_cuts( int num_sails_cuts )
{
  return ORPGSAILS_set_req_num_cuts( num_sails_cuts );
}

