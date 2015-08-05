/************************************************************************
 *									*
 *	Module:  hci_control_panel_input.c				*
 *									*
 *	Description:  This module is used to handle mouse button 	*
 *		      events inside the HCI main window.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:05:29 $
 * $Id: hci_control_panel_input.c,v 1.57 2014/07/21 20:05:29 ccalvert Exp $
 * $Revision: 1.57 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>
#include <hci_rda_control_functions.h>

/************************************************************************
 *	Description: This function handles all mouse events over all	*
 *		     non-Motif objects in the RPG Control/Status window.*
 *									*
 *	Input:  w         - ID of drawing area widget			*
 *	        *event    - data passed through event			*
 *		*args     - arguments					*
 *		*num_args - number of arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_input (
Widget		top_widget,
XEvent		*event,
String		*args,
int		*num_args
)
{
	int	i;
static	int	last_in = 0;
	int	in = 0;
	int	pixel1;
	int	pixel2;
	int	scanl1;
	int	scanl2;
	int	status;
static	int	old_pixel = 0;
static	int	old_scanl = 0;
	int	busy_object = 0;

	unsigned char	flag;

	hci_control_panel_object_t	*object;

	void	verify_vad_update_change (Widget w,
			XtPointer client_data);
	void	verify_model_ewt_update_change (Widget w,
			XtPointer client_data);
	void	verify_clear_air_switch (Widget w );
	void	verify_precip_switch (Widget w );

	busy_object = 0;

/*	If the left mouse button pressed, then determine if it is	*
 *	over a "hot spot".  If it is, then invoke the callback for	*
 *	the object.  We use the first argument to determine what caused	*
 *	this function to be invoked.					*/

	if (!strcmp (args[0], "down1")) {

/*	    Check all objects that are defined.				*/

	    for (i=RDA_BUTTON;i<LAST_OBJECT;i++) {

		object = hci_control_panel_object (i);

/*		We only want to check non-Motif objects in this block.	*/

		if (object->widget == (Widget) NULL) {

		    pixel1 = object->pixel;
		    scanl1 = object->scanl;
		    pixel2 = object->pixel + object->width;
		    scanl2 = object->scanl + object->height;

/*		    If the cursor is over a selectable object we want	*
 *		    to invoke the appropriate action.			*/

		    if ((event->xbutton.x >= pixel1) &&
			(event->xbutton.x <= pixel2) &&
			(event->xbutton.y >= scanl1) &&
			(event->xbutton.y <= scanl2) &&
			(!(object->flags & BUSY_OBJECT)) &&
			(object->flags & OBJECT_SELECTABLE)) {

/*			If the cursor wasn't over the object before we	*
 *			want to change the cursor shape.		*/

			if (in != last_in) {

			    XUndefineCursor (HCI_get_display(),
			     hci_control_panel_window());

			    last_in = 0;

			}
		    
			switch (i) {
 
/*			    Activate the Product Distribution Comms	*
 *			    Status task.				*/

			    case NARROWBAND_OBJECT:

				hci_activate_cmd ((Widget)NULL,
				(XtPointer)NARROWBAND_OBJECT, (XtPointer)NULL);
				busy_object = 1;

				break;
 
/*			    Activate the RDA-RPG Interface Control/	*
 *			    Status task.				*/

			    case WIDEBAND_OBJECT :

				hci_activate_cmd ((Widget)NULL,
				(XtPointer)WIDEBAND_OBJECT, (XtPointer)NULL);
				busy_object = 1;

				break;
 
/*			    The radome object has no action.		*/

			    case RADOME_OBJECT :

				break;
 
/*			    The tower object has no action.		*/

			    case TOWER_OBJECT :

				break;
 
/*			    Activate the RDA Power control verification	*
 *			    popup.					*/

			    case POWER_OBJECT :

/*			    This command can only be activated if the	*
 *			    RPG does not have control.			*/

				if (ORPGRDA_get_status (RS_CONTROL_STATUS) !=
				    RDA_IN_CONTROL) {

				    object = hci_control_panel_object (RDA_CONTROL_BUTTON);

/*				Get the current power source from the most*
 *				recent RDA status message.		  */

				    if ((ORPGRDA_get_status (RS_AUX_POWER_GEN_STATE) &1)) {
					Verify_power_source_change (object->widget,
					(XtPointer) UTILITY_POWER);

				    } else {

					Verify_power_source_change (object->widget,
					(XtPointer) AUXILLIARY_POWER);
					
				    }
				}

				break;
 
/*			    Activate the Mode Status task	*/

			    case MODE_STATUS_OBJECT :

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) MODE_STATUS_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;

				break;
 
/*			    Toggle the Clear Air Switch	*/

			    case CLEAR_AIR_SWITCH_OBJECT :

			      if( !hci_disallow_on_faa_inactive( top_widget ) )
                              {
				verify_clear_air_switch (top_widget);
                              }

				break;
 
/*			    Toggle the Precip Switch	*/

			    case PRECIP_SWITCH_OBJECT :

			      if( !hci_disallow_on_faa_inactive( top_widget ) )
                              {
				verify_precip_switch (top_widget);
                              }
				break;
 
/*			    Activate the VCP Control task	*/

			    case VCP_CONTROL_OBJECT :

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) VCP_CONTROL_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;

				break;

/*			    Activate the PRF Control task	*/	

			    case PRFMODE_STATUS_OBJECT :

			      if( !hci_disallow_on_faa_inactive( top_widget ) )
                              { 
				hci_activate_cmd ((Widget) NULL,
					(XtPointer) PRFMODE_STATUS_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;
                             } 

				break; 
 
/*			    Activate the Precipitation Status task	*/

			    case PRECIP_STATUS_OBJECT :

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) PRECIP_STATUS_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;

				break;
 
/*			    Activate the Super Resolution  update flag  *
 *			    verification popup.				*/

			    case SUPER_RES_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{
				object = hci_control_panel_object (RDA_CONTROL_BUTTON);
/*			    Get the current SR flag value		*/

				flag = ORPGINFO_is_super_resolution_enabled ();

			        if (flag) {

				    Verify_super_res_change (object->widget,
					(XtPointer) HCI_NO_FLAG);

				} else {

				    Verify_super_res_change (object->widget,
				    (XtPointer) HCI_YES_FLAG);

				}
			}

				break;
 
/*			    Activate the Clutter Mitigation update Decision  *
 *			    update flag verification popup.		     */

                            case CMD_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{
                                object = hci_control_panel_object (RDA_CONTROL_BUTTON);

/*                          Get the current autoprf flag value          */

                                flag = ORPGINFO_is_cmd_enabled ();

                                if (flag) {

                                    Verify_cmd_change (object->widget,
                                        (XtPointer) HCI_NO_FLAG);

                                } else {

                                    Verify_cmd_change (object->widget,
                                    (XtPointer) HCI_YES_FLAG);

                                }
			}

                                break;

/*                          Activate the Automated Volume Scan Evaluation    *
 *                          and Termination update flag verification popup.  */

                            case AVSET_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{
                                object = hci_control_panel_object (RDA_CONTROL_BUTTON);

/*                          Get the current AVSET flag value          */

                                if ( ORPGINFO_is_avset_enabled() ) {

                                    Verify_avset_change (object->widget,
                                        (XtPointer) HCI_NO_FLAG);

                                } else {

                                    Verify_avset_change (object->widget,
                                    (XtPointer) HCI_YES_FLAG);

                                }
			}

                                break;

/*                          Activate SAILS */

                            case SAILS_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{
				hci_activate_cmd ((Widget) NULL,
					(XtPointer) SAILS_STATUS_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;
			}

				break;

/*                          Activate the RDA Control HCI */

                            case PERFCHECK_STATUS_OBJECT :

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) PERFCHECK_STATUS_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;

				break;

/*                          Activate the VAD update flag verification popup */

			    case ENW_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{

				object = hci_control_panel_object (RDA_CONTROL_BUTTON);

/*				Get the current VAD update flag value	*/

				if (hci_get_vad_update_flag () == HCI_YES_FLAG) {

				    verify_vad_update_change (object->widget,
					(XtPointer) HCI_NO_FLAG);

				} else {

				    verify_vad_update_change (object->widget,
					(XtPointer) HCI_YES_FLAG);

				}
			}

				break;

/*                          Activate the Model Environmental Wind 	*
 *			    update flag verification popup.		*/

                            case MODEL_EWT_STATUS_OBJECT :

			if( !hci_disallow_on_faa_inactive( top_widget ) )
			{
                                object = hci_control_panel_object (RDA_CONTROL_BUTTON);

/*                              Get the current Model EWT update flag value   */

                                if (hci_get_model_update_flag () == HCI_YES_FLAG) {

                                    verify_model_ewt_update_change (object->widget,
                                        (XtPointer) HCI_NO_FLAG);

                                } else {

                                    verify_model_ewt_update_change (object->widget,
                                        (XtPointer) HCI_YES_FLAG);

                                }

			}
                                break;

/*			    Activate the Load Shed Categories task	*/

			    case LOAD_SHED_OBJECT :

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) LOAD_SHED_OBJECT,
					(XtPointer) NULL);
				busy_object = 1;

				break;

/*			    Toggle the inhibit RDA messages flag	*/

			    case RDA_INHIBIT_OBJECT :

				if (hci_info_inhibit_RDA_messages()) {

				    hci_info_set_inhibit_RDA_messages (0);

				} else {

				    hci_info_set_inhibit_RDA_messages (1);

				}

				break;

/*			    Force adaptation data update on the other	*
 *			    channel (FAA Redundant only)*		*/

			    case FAA_REDUNDANT_OBJECT :

				object = hci_control_panel_object (RPG_CONTROL_BUTTON);

				if (ORPGRED_channel_state (ORPGRED_OTHER_CHANNEL) != ORPGRED_CHANNEL_ACTIVE) {

				    hci_faa_redundant_force_update (object->widget,
					(XtPointer) NULL,
					(XtPointer) NULL);

				}

				break;

/*			    Display RDA alarm data for the 1st button	*/

			    case RDA_ALARM1_OBJECT :

				object = hci_control_panel_object (RDA_ALARM1_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 2nd button	*/

			    case RDA_ALARM2_OBJECT :

				object = hci_control_panel_object (RDA_ALARM2_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 3rd button	*/

			    case RDA_ALARM3_OBJECT :

				object = hci_control_panel_object (RDA_ALARM3_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 4th button	*/

			    case RDA_ALARM4_OBJECT :

				object = hci_control_panel_object (RDA_ALARM4_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 5th button	*/

			    case RDA_ALARM5_OBJECT :

				object = hci_control_panel_object (RDA_ALARM5_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 6th button	*/

			    case RDA_ALARM6_OBJECT :

				object = hci_control_panel_object (RDA_ALARM6_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 7th button	*/

			    case RDA_ALARM7_OBJECT :

				object = hci_control_panel_object (RDA_ALARM7_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

/*			    Display RDA alarm data for the 8th button	*/

			    case RDA_ALARM8_OBJECT :

				object = hci_control_panel_object (RDA_ALARM8_OBJECT);

				status = ORPGDA_write( ORPGDAT_HCI_DATA,
				                 ( char * ) &( object-> data ),
				                 sizeof( int ),
				                 HCI_RDA_DEVICE_DATA_MSG_ID );

				if( status < 0 )
				{
				  HCI_LE_log("Error writing RDA device filter to HCI_DATA LB. Return code: %d", status );
				}

				hci_activate_cmd ((Widget) NULL,
					(XtPointer) RDA_ALARMS_BUTTON,
					(XtPointer) NULL);

				break;

			}

			if (busy_object) {

			    object->flags = object->flags | BUSY_OBJECT;

			    XDefineCursor (HCI_get_display(),
				   hci_control_panel_window(),
				   hci_control_panel_cursor (BUSY_OBJECT, object));

			}
		    }
		}
	    }

/*	For all other cursor/mouse conditions, check the position to	*
 *	see if the cursor moved in/out of a selectable object domain.	*
 *	If so, change the cursor shape accordingly.			*/

	} else {

	    in = 0;

	    for (i=RDA_BUTTON;i<LAST_OBJECT;i++) {

		object = hci_control_panel_object (i);

		if (object->widget == (Widget) NULL) {

		    pixel1 = object->pixel;
		    scanl1 = object->scanl;
		    pixel2 = object->pixel + object->width;
		    scanl2 = object->scanl + object->height;

		} else {

		    pixel1 = 0;
		    scanl1 = 0;
		    pixel2 = object->width;
		    scanl2 = object->height;

		}

		if ((event->xbutton.x >= pixel1) &&
		    (event->xbutton.x <= pixel2) &&
		    (event->xbutton.y >= scanl1) &&
		    (event->xbutton.y <= scanl2) &&
		    (object->flags & OBJECT_SELECTABLE))
		{

/*		    If the cursor position hasn't changed but we did	*
 *		    receive a mouse event, it was probably forced by	*
 *		    the XWarpPointer() function which was called to	*
 *		    refresh the cursor after a child startup event.	*/

		    if ((event->xbutton.x == old_pixel) &&
			(event->xbutton.y == old_scanl)) {

			last_in = 0;

		    }

		    in = 1;
		    break;

		}
	    }

	    old_pixel = event->xbutton.x;
	    old_scanl = event->xbutton.y;

/*	    If the cursor is over a selectable object and it wasn't	*
 *	    previously, change the cursor shape.			*/

	    if (in) {

		if (in != last_in) {

		    if (object->flags & BUSY_OBJECT) {

			XDefineCursor (HCI_get_display(),
				   hci_control_panel_window(),
				   hci_control_panel_cursor (BUSY_OBJECT, object));

		    } else {

			XDefineCursor (HCI_get_display(),
				   hci_control_panel_window(),
				   hci_control_panel_cursor (ACTIVE_CURSOR, object));

		    }

		    last_in = in;

		}

/*	    If the cursor is not over a selectable object but was	*
 *	    previously, then change the cursor shape.			*/

	    } else {

		if (in != last_in) {

		    XUndefineCursor (HCI_get_display(),
				     hci_control_panel_window());

		    last_in = in;

		}
	    }
	}
}

/************************************************************************
 *	Description: This function handles all mouse events over all	*
 *		     Motif objects in the RPG Control/Status window.	*
 *		     It's purpose is to change the cursor shape when	*
 *		     an object gains focus.				*
 *									*
 *	Input:  w         - ID of widget				*
 *	        *event    - data passed through event			*
 *		*args     - arguments					*
 *		*num_args - number of arguments				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_button_input( Widget w,
                                     XEvent *event,
                                     String *args,
                                     int *num_args )
{
  int i;
  int found = 0;
  hci_control_panel_object_t *object;

  /* Determine the object which this function was invoked for. */

  for( i = RDA_BUTTON; ( !found && i < LAST_OBJECT ); i++ ) 
  {
    object = hci_control_panel_object( i );
    found = ( ( object->widget == w ) && ( w != NULL ) );
  }

  if( !found )
  {
    object= ( hci_control_panel_object_t* ) NULL;
  }

  /* If the cursor has moved over it change the cursor to a hand.
     Else, the cursor is no longer over the object so change it back
       to a pointer. */

  if( !strcmp( args[0], "enter" ) )
  {
    if( object->flags & BUSY_OBJECT )
    {
      XDefineCursor( HCI_get_display(),
                     hci_control_panel_window(),
                     hci_control_panel_cursor( BUSY_OBJECT, object ) );
    }
    else
    {
      XDefineCursor( HCI_get_display(),
                     hci_control_panel_window(),
                     hci_control_panel_cursor( ACTIVE_CURSOR, object ) );
    }
  }
  else if( !strcmp( args[0], "leave" ) )
  {
    XUndefineCursor( HCI_get_display(),
                     hci_control_panel_window());
  }
}

