/************************************************************************
 *									*
 *	Module:  hci_control_panel_window_title.c			*
 *									*
 *	Description:  This module resets the window title for the RPG	*
 *		      Control/Status window if needed.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2009/05/06 16:05:09 $
 * $Id: hci_control_panel_window_title.c,v 1.4 2009/05/06 16:05:09 garyg Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*	Local include files.						*/

#include <hci_control_panel.h>

/************************************************************************
 *	Description: This function resets the title of the RPG		*
 *		     Control/Status gui if needed.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_window_title()
{
  int system_flag;
  char title [80];
  char *previous_title;
  char buf1 [40];
  char buf2 [40];
  hci_control_panel_object_t *top;
  int red_control_state;

  /* Get reference to top-level widget. If it is null, return. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get current title of window. */

    XtVaGetValues( top->widget, XmNtitle, &previous_title, NULL );

  /* Set system flag. */

  system_flag = HCI_get_system();

  /* Create window title based on current status. */

  if( system_flag == HCI_FAA_SYSTEM )
  {
    red_control_state = ORPGRED_rda_control_status( ORPGRED_MY_CHANNEL );

    if( ORPGRED_wb_link_state( ORPGRED_MY_CHANNEL ) == RS_DOWN )
    {
      sprintf( buf1, "Unknown" );
    }
    else if( red_control_state == ORPGRED_RDA_CONTROLLING )
    {
      sprintf( buf1, "Controlling" );
    }
    else if( red_control_state == ORPGRED_RDA_NON_CONTROLLING )
    {
      sprintf( buf1, "Non-controlling" );
    }
    else
    {
      sprintf( buf1, "Unknown" );
    }

    if( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) == ORPGRED_CHANNEL_ACTIVE )
    {
      sprintf( buf2, "Active" );
    }
    else
    {
      sprintf( buf2, "Inactive" );
    }

    sprintf( title,
             "RPG Control/Status - (FAA:%d %s/%s)",
             ORPGRED_channel_num( ORPGRED_MY_CHANNEL ),
             buf2, buf1 );

  }
  else if( system_flag == HCI_NWSR_SYSTEM )
  {
    sprintf( title,
             "RPG Control/Status - (NWS:%d)",
             ORPGRDA_channel_num() );
  }
  else
  {
    /* If not a redundant system, the title won't change. Set
       current title to previous title so nothing is reset. */

    strcpy( title, previous_title );
  }

  /* If current title differs from previous title,
     set to current title. */

  if( strcmp( previous_title, title ) != 0 )
  {
    XtVaSetValues( top->widget, XmNtitle, title, NULL );
  }
}
