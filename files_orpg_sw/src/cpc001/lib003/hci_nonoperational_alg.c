 /*
  * RCS info
  * $Author: ccalvert $
  * $Locker:  $
  * $Date: 2011/09/21 21:59:23 $
  * $Id: hci_nonoperational_alg.c,v 1.3 2011/09/21 21:59:23 ccalvert Exp $
  * $Revision: 1.3 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: hci_nonoperational_alg.c				*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI that interacts with the Nonoperational.alg DEA	*
 *	file.								*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_nonoperational.h>

/************************************************************************
 *      Description: This function returns whether DP is allowed.	*
 ***********************************************************************/

int hci_allow_dualpol()
{
  char *p = NULL;
  int ret_code = 0;
  char dea_str[ HCI_BUF_128 ] = "";

  sprintf( dea_str, "%s.%s", NONOP_DEA_NAME, "allow_dp" );

  if( ( ret_code = DEAU_get_string_values( dea_str, &p ) ) < 0 )
  {
    HCI_LE_log("Error reading %s (%d)", dea_str, ret_code );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    if( strcmp( p, "Yes" ) == 0 ){ ret_code = HCI_YES_FLAG; }
    else{ ret_code = HCI_NO_FLAG; }
  }

  return ret_code;
}

