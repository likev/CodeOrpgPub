/************************************************************************
 *									*
 *	Module:  hci_control_panel_environmental_winds.c		*
 *									*
 *	Description:  This module is used to set the highlight color	*
 *		      around the environmental winds button based on	*
 *		      whether the data have expired (RED) or not.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:57 $
 * $Id: hci_control_panel_environmental_winds.c,v 1.12 2009/02/27 22:25:57 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/* Local include files. */

#include <hci_control_panel.h>

/* Define macros. */

#define	TIME_NOT_EXPIRED	0
#define	TIME_EXPIRED		1
#define	WIND_TIMEOUT_VALUE		720

/* Global/static variables. */

static int	Wind_timeout_s = WIND_TIMEOUT_VALUE; /* Short pulse timeout */
static int	Wind_timeout_l = WIND_TIMEOUT_VALUE; /* Long pulse timeout  */
static int	Wind_timeout = WIND_TIMEOUT_VALUE; /* Timeout value  */
static int	first_time = 1;
static int	prev_timeout_flag = TIME_NOT_EXPIRED;

/************************************************************************
 *	Description: This function changes the border color around the	*
 *		     environmental data button based on the winds time	*
 *		     time out value.  If the last time the wind data	*
 *		     have been updated is longer than the timeout value	*
 *		     then the border color is red. This routine must	*
 *		     be called after all of the main control window	*
 *		     widgets have been created.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_environmental_winds( int force_draw )
{
  hci_control_panel_object_t *button;
  A3cd97 *enw;
  time_t tm;
  int current_minutes;
  int i;
  int timeout_flag;
  double values = 0;
  int ret = 0;

  /* If this is the first time this has been called, get any
     adaptation data that needs to be registered. */

  if( first_time == 1 )
  {
    first_time = 0;

    /* If we cannot open the algorithm adaptation data then set
       the timeout value to WIND_TIMEOUT_VALUE minutes. */

    ret = DEAU_get_values( "alg.radazvd.env_winds_timeout_s", &values, 1 );

    if( ret < 0 )
    {
      HCI_LE_error( "DEAU_get_values() failed (%d)", ret );
      Wind_timeout_s = WIND_TIMEOUT_VALUE;
    }
    else
    {
      Wind_timeout_s = ( int ) values;
    }

    ret = DEAU_get_values( "alg.radazvd.env_winds_timeout_l", &values, 1 );

    if( ret < 0 )
    {
      HCI_LE_error( "DEAU_get_values() failed (%d)", ret );
      Wind_timeout_l = WIND_TIMEOUT_VALUE;
    }
    else
    {
      Wind_timeout_l = ( int ) values;
    }
  }

  /* Get reference to environmental winds button object. */

  button = hci_control_panel_object( ENVIRONMENTAL_WINDS_BUTTON );

  /* Get the current clock time. */

  tm = time( NULL );
  current_minutes = tm/60;  /* Convert to minutes */

  /* Get the current environmental winds data. */

  enw = ( A3cd97 * ) hci_get_environmental_wind_data_ptr();

  /* Determine the pulse width from the current VCP so we can
     decide which timeout value to use. */

  for( i = 0; i < VCPMAX; i++ )
  {
    if( hci_rda_adapt_vcp_table_vcp_num( i ) == ORPGVST_get_vcp() )
    {
      if( hci_rda_adapt_vcp_table_pulse_width( i ) != HCI_VCP_SHORT_PULSE )
      {
        Wind_timeout = Wind_timeout_l;
      }
      else
      {
        Wind_timeout = Wind_timeout_s;
      }
      break;
    }
  }

  /* If the time difference is greater than the timeout value, set
     the timeout flag. */

  if( ( current_minutes - enw->sound_time ) >= Wind_timeout )
  {
    timeout_flag = TIME_EXPIRED;
  }
  else
  {
    timeout_flag = TIME_NOT_EXPIRED;
  }

  /* If winds have timed out, set the border color to the alarm color. */

  if( timeout_flag != prev_timeout_flag || force_draw )
  {
    if( timeout_flag == TIME_EXPIRED )
    {
      XtVaSetValues( button->widget,
                     XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                     XmNbackground, hci_get_read_color( ALARM_COLOR1 ),
                     NULL );
    }
    else
    {
      XtVaSetValues( button->widget,
                     XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                     XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                     NULL);
    }

    prev_timeout_flag = timeout_flag;
  }
}



