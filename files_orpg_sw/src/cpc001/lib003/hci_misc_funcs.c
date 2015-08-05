/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/05/11 17:02:53 $
 * $Id: hci_misc_funcs.c,v 1.1 2010/05/11 17:02:53 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <hci.h>

/************************************************************************
 *      Description: This function determines if the HCI is running on  *
 *                   an inactive FAA channel, and if so, checks to see  *
 *                   if the other channel is active. Some states/flags  *
 *                   should not be allowed to change in this situation. *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

int hci_disallow_on_faa_inactive( Widget top_widget )
{
  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    if( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) == ORPGRED_CHANNEL_INACTIVE
        && ORPGRED_channel_state( ORPGRED_OTHER_CHANNEL ) == ORPGRED_CHANNEL_ACTIVE )
    {
      char buf[ HCI_BUF_128 ];
      sprintf( buf, "This operation is not allowed on the inactive\nchannel when the other channel is active." );
      hci_warning_popup( top_widget, buf, NULL );
      return 1;
    }
  }

  return 0;
}

