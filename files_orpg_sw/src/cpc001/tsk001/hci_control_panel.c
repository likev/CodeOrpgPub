/************************************************************************
 *							                *
 *	Module:  hci_control_panel.c			                *
 *							                *
 *	Description:  This module is used to set up the widgets used   	*
 *		      for the HCI GUI.				        *
 *					                        	*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:05:29 $
 * $Id: hci_control_panel.c,v 1.227 2014/07/21 20:05:29 ccalvert Exp $
 * $Revision: 1.227 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/*	Various X data.						*/

static	Display		*Hci_display;
				/* Pointer to display structure */
static	Window		Hci_window = (Window) NULL;
				/* Drawable for the visible window */
static	GC		Hci_gc     = (GC)     NULL;
				/* Graphic context for drawing */
static	Pixmap		Hci_pixmap = (Pixmap) NULL;
				/* Drawable which is drawn to and mapped to
				 * the screen when drawing is completed. */
static	int		Hci_depth       = 8; /* Depth of window (bits) */
static	int		Hci_num_fields  =  3; /* Number of moments */
static	int		Hci_font_height =  0; /* Used to store current font
						 height for object placement */

static	Cursor		Hci_active_cursor   = (Cursor) NULL; /* Hand cursor */
static	Cursor		Hci_busy_cursor   = (Cursor) NULL;   /* Watch cursor */

static	char		channel_number [32]; /* used to store channel argument
						in FAA redundant configs. Used
						for window management. */
static	int		launched_rda_pmd = 0; /* RDA PMD was launched. */
static	int		rda_pmd_channel_number = 1; /* Channel of RDA PMD */

/*	Define a custom translation table so we can track cursor	*
 *	movements inside the main window.  We need to do this so	*
 *	we can deterimne location to change cursor shape when over	*
 *	a selectable object.						*/

String	Hci_translations =
	"<PtrMoved>:	hci_control_panel_input(move) \n\
	<Btn1Down>:	hci_control_panel_input(down1)	ManagerGadgetArm() \n\
	<Btn1Up>:	hci_control_panel_input(up1)	ManagerGadgetActivate() \n\
	<Btn2Down>:	hci_control_panel_input(down2)	ManagerGadgetArm() \n\
	<Btn2Up>:	hci_control_panel_input(up2)	ManagerGadgetActivate() \n\
	<Btn3Down>:	hci_control_panel_input(down3)	ManagerGadgetArm() \n\
	<Btn3Up>:	hci_control_panel_input(up3)	ManagerGadgetActivate()";

String	Button_translations =
	"#override \n\
	<EnterWindow>:	hci_control_panel_button_input(enter) \n\
	<LeaveWindow>:	hci_control_panel_button_input(leave)";

static	hci_control_panel_object_t	Control_panel_object [LAST_OBJECT];
/* An array containing properties of
				   objects in RPG Control/Status window. */
void	hci_console_message_callback (Widget w,
				      XtPointer client_data,
				      XtPointer call_data);
int	destroy_callback ();
int	hci_activate_child (Display *d, Window w, char *cmd,
			    char *proc, char *win, int object_index);
void	hci_launch_rda_pmd();

/*	Event handlers							*/

void	hci_control_panel_hci_command_issued (en_t evtcd,
			void *ptr, size_t msglen);
void    hci_control_panel_register_for_startup_events();

/************************************************************************
 *	Description: This module is responsible for defining the	*
 *		     objects which are displayed in the RPG Control/	*
 *		     Status windw.					*
 *									*
 *	Input:  argc - number of items in command line string		*
 *		argv - pointer to command line string			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel (
	int	argc,
	char	*argv []
)
{
	XGCValues	gcv;
	int		i;
	Pixmap		pixmap;
	XtActionsRec	actions;
	XFontStruct	*fontinfo;
	int		rda_config_flag = -1; /* Is RDA legacy or ORDA? */
	int		offset_hci = HCI_NO_FLAG;
	extern	void	hci_define_bitmaps ();

/*	Initialize window object data.					*/
	for (i=TOP_WIDGET;i<LAST_OBJECT;i++) 
	{
	    Control_panel_object [i].widget = (Widget) NULL;
	    Control_panel_object [i].flags  = 0;
	    strcpy(Control_panel_object [i].app_name, "");
	}

/*									*
 *	Initialize the Xt Toolkit and create the top level widget for	*
 *	the main control panel.						*
 *									*/
	
	Control_panel_object [TOP_WIDGET].widget = HCI_get_top_widget();

/*	Get references to various X functions data structures. 	*/

	Hci_display = HCI_get_display();
	Hci_depth   = XDefaultDepthOfScreen (XDefaultScreenOfDisplay (Hci_display));

/*      Determine if RDA configuration is legacy or ORDA. */
        rda_config_flag = ORPGRDA_get_rda_config( NULL );

        /* Console message support. */
        RPGC_out_data_wevent( FTXTMSG, ORPGEVT_FREE_TXT_MSG );

        RPGC_task_init( VOLUME_BASED, argc, argv );

/*	Define the properties of the main top level widget.		*/

	Control_panel_object [TOP_WIDGET].pixel    = 0;
	Control_panel_object [TOP_WIDGET].scanl    = 0;
	Control_panel_object [TOP_WIDGET].width    = CONTROL_PANEL_WIDTH;
	Control_panel_object [TOP_WIDGET].height   = CONTROL_PANEL_HEIGHT;
	Control_panel_object [TOP_WIDGET].fg_color = hci_get_read_color (BLACK);
	Control_panel_object [TOP_WIDGET].bg_color = hci_get_read_color (BACKGROUND_COLOR1);
	Control_panel_object [TOP_WIDGET].flags    = 0;
	

/*	Set write permission of ITC for EWT and Model data. */
	ORPGDA_write_permission( 100400 );

/*	start out with a medium font for main window scalable text	*
 *	objects.							*/

	fontinfo = hci_get_fontinfo (MEDIUM);
	Hci_font_height = 2*(fontinfo->ascent + fontinfo->descent) + 5;

/*	If low bandwidth, display a progress meter while I/O is		*
 *	occurring.							*/

	HCI_PM( "Reading adaptation data and RPG state information" );

/*	Set channel number. If FAA, channel two HCI should be offset
 *	so it does not cover channel one HCI.				*/

	sprintf (channel_number,"-A 0");

	if (HCI_get_system() == HCI_FAA_SYSTEM)
	{
          sprintf( channel_number, "-A %1d",
                   ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) );
	  if( ORPGRED_channel_num(ORPGRED_MY_CHANNEL) == 2 )
	  {
	    offset_hci = HCI_YES_FLAG;
	  }

	}

/*	Set the main window title and size constraints.			*/

	XtVaSetValues (Control_panel_object [TOP_WIDGET].widget,
		XmNwidth,	Control_panel_object [TOP_WIDGET].width,
		XmNheight,	Control_panel_object [TOP_WIDGET].height+Hci_font_height,
		XmNminWidth,	Control_panel_object [TOP_WIDGET].width,
		XmNminHeight,	Control_panel_object [TOP_WIDGET].height,
		XmNmaxWidth,	MAX_CONTROL_PANEL_WIDTH,
		XmNmaxHeight,	MAX_CONTROL_PANEL_HEIGHT,
		NULL);

	if( offset_hci == HCI_YES_FLAG )
	{
		XtVaSetValues (Control_panel_object [TOP_WIDGET].widget,
			XmNdefaultPosition, False,
			XmNx, 100,
			XmNy, 100,
			NULL);
	}

	HCI_set_destroy_callback( destroy_callback );

/*	Define a drawing area widget to be used as a canvas for all	*
 *	RPG Control/Status window objects.  Modify the translation	*
 *	table to handle mouse input inside window.			*/

/*		Mouse input over non-Motif objects	*/

	actions.string = "hci_control_panel_input";
	actions.proc   = (XtActionProc) hci_control_panel_input;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

/*		Mouse input over Motif objects		*/

	actions.string = "hci_control_panel_button_input";
	actions.proc   = (XtActionProc) hci_control_panel_button_input;
	XtAppAddActions (HCI_get_appcontext(), &actions, 1);

	Control_panel_object [DRAW_WIDGET].pixel    = 0;
	Control_panel_object [DRAW_WIDGET].scanl    = 0;
	Control_panel_object [DRAW_WIDGET].width    = CONTROL_PANEL_WIDTH;
	Control_panel_object [DRAW_WIDGET].height   = CONTROL_PANEL_HEIGHT+Hci_font_height;
	Control_panel_object [DRAW_WIDGET].fg_color = hci_get_read_color (BLACK);
	Control_panel_object [DRAW_WIDGET].bg_color = hci_get_read_color (BACKGROUND_COLOR1);
	Control_panel_object [DRAW_WIDGET].flags    = 0;

	Control_panel_object [DRAW_WIDGET].widget = XtVaCreateWidget ("hci_main",
		xmDrawingAreaWidgetClass,	Control_panel_object [TOP_WIDGET].widget,
		XmNwidth,		Control_panel_object [DRAW_WIDGET].width,
		XmNheight,		Control_panel_object [DRAW_WIDGET].height,
		XmNtranslations,	XtParseTranslationTable (Hci_translations),
		XmNtraversalOn,		True,
		NULL);

/*	The following widget definitions define the pushbuttons for	*
 *	the various control panel items.  The exact placement of these	*
 *	widgets is done in the resize callback procedure.  The button	*
 *	macro name describes the object function.			*/

	Control_panel_object [RDA_CONTROL_BUTTON].flags = 0;
	Control_panel_object [RDA_CONTROL_BUTTON].widget = XtVaCreateManagedWidget ("Control",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

        if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        {
	   sprintf (Control_panel_object[RDA_CONTROL_BUTTON].app_name,
		    "hci_rdc_legacy %s", channel_number);
        }
        else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        {
	   sprintf (Control_panel_object[RDA_CONTROL_BUTTON].app_name,
		    "hci_rdc_orda %s", channel_number);
        }


	XtAddCallback (Control_panel_object [RDA_CONTROL_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd, (XtPointer)RDA_CONTROL_BUTTON);

	Control_panel_object [RDA_ALARMS_BUTTON].flags = 0;
	Control_panel_object [RDA_ALARMS_BUTTON].widget = XtVaCreateManagedWidget ("Alarms",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

        if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        {
	   sprintf (Control_panel_object[RDA_ALARMS_BUTTON].app_name,
		    "hci_rda_legacy %s", channel_number);
        }
        else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        {
	   sprintf (Control_panel_object[RDA_ALARMS_BUTTON].app_name,
		    "hci_rda_orda %s", channel_number);
        }

	XtAddCallback (Control_panel_object [RDA_ALARMS_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd, (XtPointer)RDA_ALARMS_BUTTON);

	Control_panel_object [RPG_CONTROL_BUTTON].flags = 0;
	Control_panel_object [RPG_CONTROL_BUTTON].widget = XtVaCreateManagedWidget ("Control",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf(Control_panel_object[RPG_CONTROL_BUTTON].app_name,
		"hci_rpc %s", channel_number);

	XtAddCallback (Control_panel_object [RPG_CONTROL_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd, (XtPointer)RPG_CONTROL_BUTTON);

	Control_panel_object [RPG_PRODUCTS_BUTTON].flags = 0;
	Control_panel_object [RPG_PRODUCTS_BUTTON].widget = XtVaCreateManagedWidget ("Products",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[RPG_PRODUCTS_BUTTON].app_name,
		"hci_prod %s", channel_number);

	XtAddCallback (Control_panel_object [RPG_PRODUCTS_BUTTON].widget,
		XmNactivateCallback, hci_RPG_products_button, NULL);

	Control_panel_object [RPG_STATUS_BUTTON].flags = 0;
	Control_panel_object [RPG_STATUS_BUTTON].widget = XtVaCreateManagedWidget ("Status",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[RPG_STATUS_BUTTON].app_name,
		"hci_status %s", channel_number);

	XtAddCallback (Control_panel_object [RPG_STATUS_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd, (XtPointer)RPG_STATUS_BUTTON);
			
	if (HCI_has_rms()) {
			
	    int	status;
	    int	rms_status;

	    Control_panel_object [RMS_CONTROL_BUTTON].flags = 0;
	    Control_panel_object [RMS_CONTROL_BUTTON].widget = XtVaCreateManagedWidget ("Messages",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNrecomputeSize,		False,
		XmNalignment,			XmALIGNMENT_CENTER,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);
	
	    XtAddCallback (Control_panel_object [RMS_CONTROL_BUTTON].widget,
			XmNactivateCallback, hci_RMS_control_button,
			(XtPointer)RMS_CONTROL_BUTTON);
				
	    status = ORPGDA_read (ORPGDAT_HCI_DATA, (char *) &rms_status,
				sizeof (int), HCI_RMS_STATUS_MSG_ID);

	    if (status <= 0)
		rms_status = 1;

	    hci_set_rms_down_flag(rms_status);
	
	}

/*	Define the pushbutton to open the basedata window as an icon	*
 *	in the applications region of the main window.			*/

	hci_define_bitmaps ();

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"basedata_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [BASEDATA_BUTTON].flags = 0;
	Control_panel_object [BASEDATA_BUTTON].widget = XtVaCreateManagedWidget ("Basedata",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[BASEDATA_BUTTON].app_name,
		"hci_basedata %s", channel_number);

	XtAddCallback (Control_panel_object [BASEDATA_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) BASEDATA_BUTTON);

/*	Define the pushbutton to open the clutter regions editor as an	*
 *	icon in the applications region of the main window.		*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"clutter_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [CENSOR_ZONES_BUTTON].flags = 0;
	Control_panel_object [CENSOR_ZONES_BUTTON].widget = XtVaCreateManagedWidget ("Clutter",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

        if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        {
	   sprintf (Control_panel_object[CENSOR_ZONES_BUTTON].app_name,
		"hci_ccz_legacy %s", channel_number);
        }
        else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        {
	   sprintf (Control_panel_object[CENSOR_ZONES_BUTTON].app_name,
		"hci_ccz_orda %s", channel_number);
        }

	XtAddCallback (Control_panel_object [CENSOR_ZONES_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) CENSOR_ZONES_BUTTON);

/*	Define the pushbutton to open the environmental winds editor	*
 *	as an icon in the applications region of the main window.	*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"environmental_winds_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [ENVIRONMENTAL_WINDS_BUTTON].flags = 0;
	Control_panel_object [ENVIRONMENTAL_WINDS_BUTTON].widget = XtVaCreateManagedWidget ("Environmental Winds",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[ENVIRONMENTAL_WINDS_BUTTON].app_name,
		"hci_wind %s", channel_number);

	XtAddCallback (Control_panel_object [ENVIRONMENTAL_WINDS_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) ENVIRONMENTAL_WINDS_BUTTON);

/*	Define the pushbutton to open the HCI misc gui as an	*
 *	icon in the applications region of the main window.	*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"misc_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [MISC_BUTTON].flags = 0;
	Control_panel_object [MISC_BUTTON].widget = XtVaCreateManagedWidget ("Misc",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[MISC_BUTTON].app_name,
		"hci_misc %s", channel_number);

	XtAddCallback (Control_panel_object [MISC_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) MISC_BUTTON);

/*	Define the pushbutton to open the HCI blockage gui as	*
 *	an icon in the applications region of the main window.	*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"blockage_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [BLOCKAGE_BUTTON].flags = 0;
	Control_panel_object [BLOCKAGE_BUTTON].widget = XtVaCreateManagedWidget ("Blockage",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[BLOCKAGE_BUTTON].app_name,
		"hci_blockage %s", channel_number);

	XtAddCallback (Control_panel_object [BLOCKAGE_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) BLOCKAGE_BUTTON);

/*	Define the pushbutton to open the clutter bypass map display as	*
 *	an icon in the applications region of the main window.		*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"clutter_map_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [BYPASS_MAP_BUTTON].flags = 0;
	Control_panel_object [BYPASS_MAP_BUTTON].widget = XtVaCreateManagedWidget ("Clutter Map",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

        if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        {
	  sprintf (Control_panel_object[BYPASS_MAP_BUTTON].app_name,
		"hci_cbm_legacy %s", channel_number);
        }
        else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        {
	  sprintf (Control_panel_object[BYPASS_MAP_BUTTON].app_name,
		"hci_cbm_orda %s", channel_number);
        }

	XtAddCallback (Control_panel_object [BYPASS_MAP_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) BYPASS_MAP_BUTTON);

/*	Define the pushbutton to open the RDA Performance Data window	*
 *	as an icon in the applications region of the main window.	*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"rda_performance_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [RDA_PERFORMANCE_BUTTON].flags = 0;
	Control_panel_object [RDA_PERFORMANCE_BUTTON].widget = XtVaCreateManagedWidget ("RDA Performance",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	XtAddCallback (Control_panel_object [RDA_PERFORMANCE_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) RDA_PERFORMANCE_BUTTON);

/*	Define the pushbutton to open the PRF Control window	*
 *	as an icon in the applications region of the main	*
 *	window.							*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"prf_control_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [PRF_CONTROL_BUTTON].flags = 0;
	Control_panel_object [PRF_CONTROL_BUTTON].widget = XtVaCreateManagedWidget ("PRF Control",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[PRF_CONTROL_BUTTON].app_name,
		"hci_prf %s", channel_number);

	XtAddCallback (Control_panel_object [PRF_CONTROL_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) PRF_CONTROL_BUTTON);

/*	Define the pushbutton to open the Console Messages window as	*
 *	an icon in the applications region of the main window.		*/

	pixmap = XmGetPixmap (XtScreen (Control_panel_object [DRAW_WIDGET].widget),
		"console_message_icon", hci_get_read_color (ICON_FOREGROUND), hci_get_read_color (ICON_BACKGROUND));

	Control_panel_object [CONSOLE_MESSAGE_BUTTON].flags = 0;
	Control_panel_object [CONSOLE_MESSAGE_BUTTON].widget = XtVaCreateManagedWidget ("Console Messages",
		xmDrawnButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			pixmap,
		XmNpushButtonEnabled,		True,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	XtAddCallback (Control_panel_object [CONSOLE_MESSAGE_BUTTON].widget,
		XmNactivateCallback, hci_console_message_callback, NULL);

/*	Define the pushbutton for activating the Product Distribution	*
 *	Comms Status window.						*/

	Control_panel_object [COMMS_BUTTON].flags = 0;
	Control_panel_object [COMMS_BUTTON].widget = XtVaCreateManagedWidget ("Comms",
		xmPushButtonWidgetClass,	Control_panel_object [DRAW_WIDGET].widget,
		XmNrecomputeSize,		False,
		XmNshadowThickness,		0,
		XmNforeground,			hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,			hci_get_read_color (BUTTON_BACKGROUND),
		XmNborderWidth,			0,
		XmNmarginWidth,			0,
		XmNmarginHeight,		0,
		XmNhighlightThickness,		0,
		XmNtranslations,	XtParseTranslationTable (Button_translations),
		NULL);

	sprintf (Control_panel_object[COMMS_BUTTON].app_name,
		"hci_nb %s", channel_number);

	XtAddCallback (Control_panel_object [COMMS_BUTTON].widget,
		XmNactivateCallback, hci_activate_cmd,
		(XtPointer) COMMS_BUTTON);

	XtManageChild   (Control_panel_object [DRAW_WIDGET].widget);

	XtRealizeWidget (Control_panel_object [TOP_WIDGET].widget);

/*	Set the object select flag for the other non-widget object	*
 *	which can be selected with the left mouse button.		*/

	Control_panel_object [POWER_OBJECT].widget           = (Widget) NULL;
	Control_panel_object [TOWER_OBJECT].widget           = (Widget) NULL;
	Control_panel_object [RADOME_OBJECT].widget          = (Widget) NULL;

	sprintf (Control_panel_object[VCP_CONTROL_OBJECT].app_name,
		"hci_vcp %s", channel_number);
	sprintf (Control_panel_object [PRFMODE_STATUS_OBJECT].app_name,
		"hci_prf %s", channel_number);
	sprintf (Control_panel_object[PRECIP_STATUS_OBJECT].app_name,
		"hci_precip_status %s", channel_number);
	sprintf (Control_panel_object[MODE_STATUS_OBJECT].app_name,
		"hci_mode_status %s", channel_number);
	sprintf (Control_panel_object[ALERTS_BUTTON].app_name,
		"hci_alt %s", channel_number);
	sprintf (Control_panel_object[PRODUCT_PARAMETERS_BUTTON].app_name,
		"hci_spp %s", channel_number);
	sprintf (Control_panel_object[PRODUCT_STATUS_BUTTON].app_name,
		"hci_pstat %s", channel_number);
	sprintf (Control_panel_object[RPG_ALGORITHMS_BUTTON].app_name,
		"hci_apps_adapt %s", channel_number);
	sprintf (Control_panel_object[LOAD_SHED_OBJECT].app_name,
		"hci_load %s", channel_number);
	sprintf (Control_panel_object[WIDEBAND_OBJECT].app_name,
		"hci_rda_link %s", channel_number);
	sprintf (Control_panel_object[NARROWBAND_OBJECT].app_name,
		"hci_nb %s", channel_number);
	sprintf (Control_panel_object[PERFCHECK_STATUS_OBJECT].app_name,
		"hci_rdc_orda %s", channel_number);
	sprintf (Control_panel_object[SAILS_STATUS_OBJECT].app_name,
		"hci_sails %s", channel_number);

	Control_panel_object [SUPER_RES_STATUS_OBJECT].widget= (Widget) NULL;
	Control_panel_object [CMD_STATUS_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [AVSET_STATUS_OBJECT].widget    = (Widget) NULL;
	Control_panel_object [SAILS_STATUS_OBJECT].widget    = (Widget) NULL;
	Control_panel_object [PERFCHECK_STATUS_OBJECT].widget = (Widget) NULL;
	Control_panel_object [ENW_STATUS_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [MODEL_EWT_STATUS_OBJECT].widget= (Widget) NULL;
	Control_panel_object [MODE_STATUS_OBJECT].widget     = (Widget) NULL;
	Control_panel_object [LOAD_SHED_OBJECT].widget       = (Widget) NULL;
	Control_panel_object [RDA_ALARM1_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM2_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM3_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM4_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM5_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM6_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM7_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [RDA_ALARM8_OBJECT].widget      = (Widget) NULL;
	Control_panel_object [WIDEBAND_OBJECT].widget	     = (Widget) NULL;
	Control_panel_object [NARROWBAND_OBJECT].widget	     = (Widget) NULL;
	Control_panel_object [RDA_INHIBIT_OBJECT].widget     = (Widget) NULL;
	Control_panel_object [FAA_REDUNDANT_OBJECT].widget   = (Widget) NULL;
	Control_panel_object [CLEAR_AIR_SWITCH_OBJECT].widget= (Widget) NULL;
	Control_panel_object [PRECIP_SWITCH_OBJECT].widget   = (Widget) NULL;

	Control_panel_object [POWER_OBJECT].flags           = OBJECT_SELECTABLE;
	Control_panel_object [TOWER_OBJECT].flags           = 0;
	Control_panel_object [RADOME_OBJECT].flags          = 0;
	Control_panel_object [VCP_CONTROL_OBJECT].flags   = OBJECT_SELECTABLE;
	Control_panel_object [PRECIP_STATUS_OBJECT].flags   = OBJECT_SELECTABLE;
	Control_panel_object [PRFMODE_STATUS_OBJECT].flags  = OBJECT_SELECTABLE;
	Control_panel_object [SUPER_RES_STATUS_OBJECT].flags= OBJECT_SELECTABLE;
	Control_panel_object [CMD_STATUS_OBJECT].flags      = OBJECT_SELECTABLE;
	Control_panel_object [AVSET_STATUS_OBJECT].flags    = OBJECT_SELECTABLE;
	Control_panel_object [SAILS_STATUS_OBJECT].flags    = OBJECT_SELECTABLE;
	Control_panel_object [PERFCHECK_STATUS_OBJECT].flags= OBJECT_SELECTABLE;
	Control_panel_object [ENW_STATUS_OBJECT].flags      = OBJECT_SELECTABLE;
	Control_panel_object [MODEL_EWT_STATUS_OBJECT].flags= OBJECT_SELECTABLE;
	Control_panel_object [MODE_STATUS_OBJECT].flags     = OBJECT_SELECTABLE;
	Control_panel_object [LOAD_SHED_OBJECT].flags       = OBJECT_SELECTABLE;
	Control_panel_object [WIDEBAND_OBJECT].flags        = OBJECT_SELECTABLE;
	Control_panel_object [NARROWBAND_OBJECT].flags      = OBJECT_SELECTABLE;
	Control_panel_object [RDA_INHIBIT_OBJECT].flags     = OBJECT_SELECTABLE;
	Control_panel_object [CLEAR_AIR_SWITCH_OBJECT].flags= OBJECT_SELECTABLE;
	Control_panel_object [PRECIP_SWITCH_OBJECT].flags   = OBJECT_SELECTABLE;

	if (HCI_get_system() == HCI_FAA_SYSTEM)
	{

	    Control_panel_object [FAA_REDUNDANT_OBJECT].flags = OBJECT_SELECTABLE;

	} else {

	    Control_panel_object [FAA_REDUNDANT_OBJECT].flags = 0;

	}

	Control_panel_object [RDA_ALARM1_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM2_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM3_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM4_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM5_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM6_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM7_OBJECT].flags      = 0;
	Control_panel_object [RDA_ALARM8_OBJECT].flags      = 0;

/*	Get the  various X properties for the drawing area widget to	*
 *	be used later in Xlib calls.					*/

	Hci_window  = XtWindow  (Control_panel_object [DRAW_WIDGET].widget);

/*	We must define a graphics context if we are going to do any	*
 *	drawing.							*/

	gcv.foreground = hci_get_read_color (WHITE);
	gcv.background = hci_get_read_color (BACKGROUND_COLOR1);
	gcv.graphics_exposures = FALSE;

	Hci_gc = XCreateGC (Hci_display, Hci_window,
		GCBackground | GCForeground | GCGraphicsExposures, &gcv);

	XtVaSetValues (Control_panel_object [DRAW_WIDGET].widget,
		XmNuserData,	Hci_gc,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,	hci_get_read_color (WHITE),
		NULL);

/*	Create a pixmap for the control panel.  All graphics are 	*
 *	written to the pixmap first before being displayed.  This is	*
 *	for handling exposure events.					*/

	Hci_pixmap = XCreatePixmap (Hci_display,
				    Hci_window,
				    Control_panel_object [DRAW_WIDGET].width+Hci_font_height,
				    Control_panel_object [DRAW_WIDGET].height,
				    Hci_depth);

/*	call hci_control_panel_resize to draw the contents of the	*
 *	control panel.							*/

	XSetFont (Hci_display,
		  Hci_gc,
		  hci_get_font (LIST));

	Hci_active_cursor   = XCreateFontCursor (Hci_display, XC_hand2);
	Hci_busy_cursor   = XCreateFontCursor (Hci_display, XC_watch);	

        if (HCI_is_low_bandwidth())
        {
           hci_control_panel_force_resize(1); 
        }
	hci_control_panel_resize (Control_panel_object [DRAW_WIDGET].widget,
		(XtPointer) NULL,
		(XtPointer) NULL);

/*	Set the foreground and background color of the top widget for	*
 *	its iconic look.						*/

	XtVaSetValues (Control_panel_object [TOP_WIDGET].widget,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (WARNING_COLOR),
		NULL);

/*	Register for HCI command issued event.				*/

	EN_register (ORPGEVT_HCI_COMMAND_ISSUED,
		     (void *)hci_control_panel_hci_command_issued);
	
/*   	Register to get notified of start up events from my children */
	hci_control_panel_register_for_startup_events();

/*	These callbacks are registered after all objects are created.	*
 *	If these callbacks are placed earlier, the progree meter will	*
 *	cause these callbacks to be executed before the entire control	*
 *	panel has been constructed.					 */

/*	A callback must be added for window expose events.  Otherwise,	*
 *	whenever part of the main control panel is obscured by another	*
 *	window and re-exposed, the portion hidden by the other window	*
 *	will remain blank.						*/

	XtAddCallback (hci_control_panel_object(DRAW_WIDGET)->widget,
			XmNexposeCallback, hci_control_panel_expose, NULL);
	
/*	The resize callback actually does the resizing and placement 	*
 *	of the control panel components.  Since it is called intitially	*
 *	when the control panel is displayed, actual placement of	*
 *	control widgets is done there instead of being repeated here.	*/

	XtAddCallback (hci_control_panel_object(DRAW_WIDGET)->widget,
			XmNresizeCallback, hci_control_panel_resize, NULL);

	/* Enter HCI loop. */

	HCI_start( hci_timer_proc, HCI_QUARTER_SECOND, RESIZE_HCI );
}

/************************************************************************
 *	Description: The following function is used to change the cursor*
 *		     to a watch shape when a child application is	*
 *		     launched and back to it's previous state when the	*
 *		     application has completed startup.			*
 *									*
 *	Input:  evtcd   - Event code activating module			*
 *		ptr     - Pointer to message string			*
 *		msglen  - Length of message string			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_change_busy_status (
en_t	evtcd,
void*	info,
size_t	msglen
)
{
	int	object_index;
	hci_child_started_event_t	*data;

	data = (hci_child_started_event_t *) info;

	object_index = data->child_id;

	if ((object_index > RDA_BUTTON) &&
	    (object_index < LAST_OBJECT)) {

	    Window	root, window;
	    int		rootx, rooty, winx, winy;
	    unsigned int mask;

/*	    The first thing we want to do is clear the object busy	*
 *	    bit and reset the cursor.					*/

	    Control_panel_object [object_index].flags =
	    	Control_panel_object [object_index].flags & (~BUSY_OBJECT);

	    XUndefineCursor (HCI_get_display(), hci_control_panel_window());

/*	    We now want to get the current cursor position and then	*
 *	    call XWarpPointer() so that a motion event is generated.	*
 *	    This is to force the input callback to be called so the	*
 *	    correct cursor shape can be defined.			*/

	    XQueryPointer (HCI_get_display(),
			   hci_control_panel_window(),
			   &root,
			   &window,
			   &rootx,
			   &rooty,
			   &winx,
			   &winy,
			   &mask);

	    XWarpPointer (HCI_get_display(),
			  None,
			  hci_control_panel_window(),
			  0,
			  0,
			  0,
			  0,
			  0,
			  0);

	    XWarpPointer (HCI_get_display(),
			  None,
			  hci_control_panel_window(),
			  0,
			  0,
			  0,
			  0,
			  winx,
			  winy);
	}
}

/************************************************************************
 *	Description: The following function is used to register for 	*
 *		     child startup and completion events.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void    hci_control_panel_register_for_startup_events()
{
	EN_register(ORPGEVT_HCI_CHILD_IS_STARTED,
                    (void *)hci_control_panel_change_busy_status);
}

/************************************************************************
 *      Description: The following function returns the window ID of the*
 *                   RPG Control/Status window.                         *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: ID of RPG Control/Status window                         *
 ************************************************************************/

Window
hci_control_panel_window ()
{
        return (Window) Hci_window;
}

/************************************************************************
 *	Description: The following function returns the pixmap ID of the*
 *		     RPG Control/Status window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: ID of RPG Control/Status pixmap				*
 ************************************************************************/

Pixmap
hci_control_panel_pixmap ()
{
	return (Pixmap) Hci_pixmap;
}

/************************************************************************
 *	Description: The following module creates a new pixmap for the	*
 *		     RPG Control/Status window.	 This should be done	*
 *		     after a window resize.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_new_pixmap (
int	width,
int	height
)
{
	if (Hci_pixmap != (Pixmap) NULL) {

	    XFreePixmap (Hci_display, Hci_pixmap);

	}

	Hci_pixmap = XCreatePixmap (Hci_display,
			Hci_window,
			width,
			height,
			Hci_depth);

}

/************************************************************************
 *	Description: The following function returns the graphics	*
 *		     context used in RPG Control/Status window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Graphics Context for RPG Control/Status window		*
 ************************************************************************/

GC
hci_control_panel_gc ()
{
	return (GC) Hci_gc;
}

/************************************************************************
 *	Description: The following function returns the number of	*
 *		     pixels used to give the 3D appearance of selected	*
 *		     objects in RPG Control/Status window.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Depth (pixels) of 3D objects in the RPG Control/Status	*
 *		window							*
 ************************************************************************/

int
hci_control_panel_3d ()
{
	return (int) HCI_DEPTH_3D;
}

/************************************************************************
 *	Description: The following function returns the width (pixels)	*
 *		     of the RPG Control/Status window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Width (pixels) of the RPG Control/Status window.	*
 ************************************************************************/

int
hci_control_panel_width ()
{
	return Control_panel_object [TOP_WIDGET].width;
}

/************************************************************************
 *	Description: The following function returns the height		*
 *		     (scanlines) of the RPG Control/Status window.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Height (scanlines) of the RPG Control/Status window.	*
 ************************************************************************/

int
hci_control_panel_height ()
{
	return Control_panel_object [TOP_WIDGET].height + Hci_font_height;
}

/************************************************************************
 *	Description: The following function returns the width (pixels)	*
 *		     of data flow boxes between components in the RPG	*
 *		     Control/Status window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Width (pixels) of a data flow box.			*
 ************************************************************************/

int
hci_control_panel_data_width ()
{
  XFontStruct *fontinfo = hci_get_fontinfo( SCALED );
  return fontinfo->ascent + fontinfo->descent;
}

/************************************************************************
 *	Description: The following function returns the height		*
 *		     (scanlines) of data flow boxes between components	*
 *		     in the RPG Control/Status window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Height (scanlines) of a data flow box.			*
 ************************************************************************/

int
hci_control_panel_data_height ()
{
  XFontStruct *fontinfo = hci_get_fontinfo( SCALED );
  return fontinfo->ascent + fontinfo->descent;
}

/************************************************************************
 *	Description: The following function returns the cursor data	*
 *		     associated with a specific cursor type.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Cursor data.						*
 ************************************************************************/

Cursor
hci_control_panel_cursor (
int	cid, 
hci_control_panel_object_t *object
)
{
	Cursor	cursor;
	
	switch (cid) 
	{

	  case ACTIVE_CURSOR :

		   cursor = Hci_active_cursor;
		break;

	  case BUSY_OBJECT :

		   cursor = Hci_busy_cursor;
		break;

	  default :

		cursor = Hci_active_cursor;
		break;

	}

	return cursor;
}

/************************************************************************
 *	Description: The following function returns the number of	*
 *		     moments defined.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: Number of monents defined.				*
 ************************************************************************/

int
hci_control_panel_fields ()
{
	return Hci_num_fields;
}

/************************************************************************
 *	Description: The following function creates a popup window	*
 *		     to inform the user that the Basedata Display task	*
 *		     is unavailable for current bandwidth.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_basedata_not_available (
Widget	w
)
{
	char buf[HCI_BUF_128];

	if (HCI_is_low_bandwidth())
	{
	  sprintf( buf, "The RPG Base Data Display window is\nnot available in low bandwidth mode." );
	}
	else if (HCI_is_satellite_connection())
	{
	  sprintf( buf, "The RPG Base Data Display window is not\navailable over a satellite connection." );
	}
	else
	{
	  sprintf( buf, "The RPG Base Data Display\nwindow is not available." );
	}
        hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
}

/************************************************************************
 *	Description: The following function is invoked for pushbuttons	*
 *		     that exec HCI child tasks.				*
 *									*
 *	Input:  w           - Widget ID of pushbutton activating	*
 *			      callback					*
 *		client_data - Macro defining child tasks associated	*
 *			      with the button				*
 *		call_data   - widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_activate_cmd (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

	ORPGDBM_query_data_t	query_data [2];
	RPG_prod_rec_t		db_info [2];
	int			products;
	int			status;
	char			window_name[256] = "";
	char			task_name[256] = "";
	char			buf[HCI_BUF_128];
        char			*rda_adapt_msg = NULL;
        char		        *censor_zones = NULL;
	int		        rda_config_flag = -1; /* Is RDA legacy/ORDA? */

	if (((int) client_data > RDA_BUTTON) &&
	    ((int) client_data < LAST_OBJECT)) {

	    Control_panel_object [(int) client_data].flags =
		Control_panel_object [(int) client_data].flags | BUSY_OBJECT;

	    XDefineCursor (HCI_get_display(),
			   hci_control_panel_window(),
			   hci_control_panel_cursor (BUSY_OBJECT,
                                  &Control_panel_object[(int) client_data]));

	}

        /* Determine if RDA configuration is legacy or ORDA. */
        /* If it is neither, something is wrong, so display  */
        /* popup and return.                                 */

        rda_config_flag = ORPGRDA_get_rda_config( NULL );

        if( ( rda_config_flag != ORPGRDA_LEGACY_CONFIG ) &&
            ( rda_config_flag != ORPGRDA_ORDA_CONFIG ) )
        {
           HCI_LE_error("Error with RDA configuration[%d]",rda_config_flag);
           sprintf( buf, "Error with RDA configuration.\nUnable to continue." );
           hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
           return;
        }

/*	From the widgets client data, use the object macro to detemine	*
 *	which child task is to be started and build a command string.	*/

	switch ((int) client_data) {

/*	    Alert Threshol Editor task					*/

	  case ALERTS_BUTTON :
		HCI_LE_log("Alerts Button selected");
		sprintf (window_name,"Alert Threshold");
	    	sprintf (task_name,
                         "hci_alt %s -O %d -name \"Alert Threshold\"",
                         channel_number, ALERTS_BUTTON);
		break;

/*	    Base Data Display task					*/

	  case BASEDATA_BUTTON :
		HCI_LE_log("Basedata Button selected");
		sprintf (window_name,"RPG Base Data Display");
		if (HCI_is_low_bandwidth() || HCI_is_satellite_connection())
		  hci_basedata_not_available(Control_panel_object [TOP_WIDGET].widget);
		else
                {
		  sprintf (task_name,
                       "hci_basedata %s -O %d -name \"RPG Base Data Display\"",
                       channel_number, BASEDATA_BUTTON);
                }
		break;


/*	    RDA Control task						*/

	  case RDA_CONTROL_BUTTON:
		HCI_LE_log("RDA Control Button selected");
		sprintf (window_name,"RDA Control");
       		if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        	{
			sprintf (task_name,
               	          "hci_rdc_legacy %s -O %d -name \"RDA Control\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
        	else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        	{
			sprintf (task_name,
               	          "hci_rdc_orda %s -O %d -name \"RDA Control\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
		break;

/*	    RDA Alarms task						*/

	  case RDA_ALARMS_BUTTON:
		HCI_LE_log("RDA Alarms Button selected");
		sprintf (window_name,"RDA Alarms");
       		if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        	{
			sprintf (task_name,
               	          "hci_rda_legacy %s -O %d -name \"RDA Alarms\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
        	else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        	{
			sprintf (task_name,
               	          "hci_rda_orda %s -O %d -name \"RDA Alarms\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
		break;

/*	    RDA Performance Maintenance task				*/

	  case RDA_PERFORMANCE_BUTTON :
		HCI_LE_log("RDA Performance Button selected");
		sprintf (window_name,"RDA Performance Data");
                if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
                {
		    sprintf (task_name, "hci_perf %s -O %d -name \"RDA Performance Data\"", 
				channel_number, RDA_PERFORMANCE_BUTTON);
		    sprintf (Control_panel_object[RDA_PERFORMANCE_BUTTON].app_name,
      	  			"hci_perf %s", channel_number);
                }
                else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
                {
                  hci_launch_rda_pmd();
                  return;
                }
		break;

/*	    Selectable Product Parameters Edit task			*/

	  case PRODUCT_PARAMETERS_BUTTON :

		HCI_LE_log("Product Parameters Button selected");
		sprintf (window_name,"Edit Selectable Product Parameters");
		sprintf (task_name, "hci_spp %s -O %d -name \"Edit Selectable Product Parameters\"", channel_number, PRODUCT_PARAMETERS_BUTTON);
		break;

/*	    Algorithms Adaptation Data Edit task			*/

	    case RPG_ALGORITHMS_BUTTON :

		HCI_LE_log("Algorithms Button selected");
		sprintf (window_name,"Algorithms");
		sprintf (task_name, "hci_apps_adapt %s -name \"Algorithms\"", channel_number);
		break;

/*	    Product Generation Table Edit task				*/

	    case RPG_PRODUCTS_BUTTON :

		HCI_LE_log("Product Generation Table Editor Button selected");
		sprintf (window_name,"RPG Product Generation Table Editor");
		sprintf (task_name, "hci_prod %s -O %d -name \"RPG Product Generation Table Editor\"", channel_number, RPG_PRODUCTS_BUTTON);
		break;

/*	    RPG Control task						*/

	    case RPG_CONTROL_BUTTON:

		HCI_LE_log("RPG Control Button selected");
		sprintf (window_name,"RPG Control");
	        sprintf (task_name, "hci_rpc %s -O %d -name \"RPG Control\"", channel_number, RPG_CONTROL_BUTTON);
	        break;

/*	    RPG Status task						*/

	    case RPG_STATUS_BUTTON:

		HCI_LE_log("RPG Status Button selected");
		sprintf (window_name,"RPG Status");
	        sprintf (task_name, "hci_status %s -O %d -name \"RPG Status\"", channel_number, RPG_STATUS_BUTTON);
	        break;

/*	    RPG Products Database task					*/

	    case PRODUCT_STATUS_BUTTON :

		HCI_LE_log("Products in Database Button selected");

/*		If the Products Database is empty, it makes no sense	*
 *		activating the Products in Database task.  Pop up a	*
 *		message informing the user that the Products database	*
 *		is empty and return.					*/

		query_data [1].field = RPGP_VOLT;
		query_data [1].value = 0;
		query_data [0].field = ORPGDBM_MODE;
		query_data [0].value = ORPGDBM_FULL_SEARCH |
				       ORPGDBM_HIGHEND_SEARCH |
				       ORPGDBM_DISTINCT_FIELD_VALUES;

		products = ORPGDBM_query (db_info,
					  query_data,
					  2,
					  2);

		if (products == 0) {

		    sprintf( buf, "The Products Database is empty" );
                    hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
		    return;

		} else if (products < 0) {

		    sprintf( buf, "The Products Database query failed!" );
                    hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
		    return;

		} else {

		    sprintf (window_name,"Products in Database");
		    sprintf (task_name, "hci_pstat %s -O %d -name \"Products in Database\"", channel_number, PRODUCT_STATUS_BUTTON);
		    break;

		}

/*	    Environmental Data Edit task				*/

	    case ENVIRONMENTAL_WINDS_BUTTON :

		HCI_LE_log("Environmental Winds Editor Button selected");
		sprintf (window_name,"Environmental Data Editor");
                if( !hci_disallow_on_faa_inactive( Control_panel_object[TOP_WIDGET].widget ) )
                {
		  sprintf (task_name, "hci_wind %s -O %d -name \"Environmental Data Editor\"", channel_number, ENVIRONMENTAL_WINDS_BUTTON);
                }
		break;

/*	    Misc tasks					*/

	    case MISC_BUTTON :

		HCI_LE_log("Misc Button selected");
		sprintf (window_name,"Misc");
		sprintf (task_name, "hci_misc %s -O %d -name \"Misc\"", channel_number, MISC_BUTTON);
		break;

/*	    Blockage tasks					*/

	    case BLOCKAGE_BUTTON :

		HCI_LE_log("Blockage Button selected");
		sprintf (window_name,"Blockage Data Display");
		sprintf (task_name, "hci_blockage %s -O %d -name \"Blockage Data Display\"", channel_number, BLOCKAGE_BUTTON);
		break;

/*	    PRF Control task			*/

    case PRF_CONTROL_BUTTON :

		HCI_LE_log("PRF Control Button selected");

/*		We need to handle the PRF control button differently	*
 *		if the RDA is in local control or the wideband is not	*
 *		connected.						*/

		if (((ORPGRDA_get_status (RS_CONTROL_STATUS) == RDA_IN_CONTROL) ||
		    (ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT) != RS_CONNECTED)) 
		    && ORPGMISC_is_operational() )
		{
		    sprintf( buf, "PRF Control is not allowed while the RDA\nis in local control or the RDA-RPG link is\nnot connected." );
                    hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
		    return;

		} else {

		    sprintf (window_name,"PRF Control");
		    sprintf (task_name, "hci_prf %s -O %d -name \"PRF Control\"", channel_number, PRF_CONTROL_BUTTON);

		}

		break;

/*	    Clutter Suppression Regions Edit task			*/

	    case CENSOR_ZONES_BUTTON :

		HCI_LE_log("Clutter Suppresion Regions Editor Button selected");

                /* There are a couple of failure points within the   */
                /* clutter censor zone hci task. Go ahead and check  */
                /* the failure points here. If there are failures,   */
                /* a popup with an appropriate message can be        */
                /* displayed. If we wait until entering the hci task */
                /* to check the failure points, then it is much      */
                /* harder to display the popup.                      */

                /* Failure point #1: Reading RDA adaptation data. */
                /* If RDA has ORDA configuration, make sure RDA   */
                /* adaptation data can be read. If it can't,      */
                /* display popup and return.                      */

                if( rda_config_flag == ORPGRDA_ORDA_CONFIG )
                {
                  ORPGDA_write_permission( ORPGDAT_RDA_ADAPT_DATA );

                  status = ORPGDA_read( ORPGDAT_RDA_ADAPT_DATA,
                                        ( char * ) &rda_adapt_msg,
                                        LB_ALLOC_BUF,
                                        ORPGDAT_RDA_ADAPT_MSG_ID );

                  if( status < 0 )
                  {
                    HCI_LE_error("Error reading RDA adaptation data [%d]",status);
                    sprintf( buf, "Error reading RDA adaptation data.\nUnable to continue." );
                    hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
                    return;
                  }
                }

                /* We don't need the RDA adaptation data anymore. */
                free( rda_adapt_msg );

                /* Failure point #2: Make sure clutter regions */
                /* file can be read.  If it can't, show popup  */
                /* and return. First determine which RDA       */
                /* configuration is being used so the          */
                /* appropriate censor zone data can be read.   */

                if( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
                {
                  status = ORPGCCZ_get_censor_zones( ORPGCCZ_LEGACY_ZONES,
                                                     (char **) &censor_zones,
                                                     ORPGCCZ_DEFAULT );
                }
                else
                {
                  status = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES,
                                                     (char **) &censor_zones,
                                                     ORPGCCZ_DEFAULT );
                }

                if( status < 0 )
                {
                  HCI_LE_error("Error accessing clutter regions [%d]", status );
                  sprintf( buf, "Error accessing clutter regions.\nUnable to continue." );
                  hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
                  return;
                }

                /* All is well, so proceed with calling task. */

                /* We don't need the censor zone data, so free it. */
                free( censor_zones );

		sprintf (window_name,"Clutter Regions");
		rda_config_flag = ORPGRDA_get_rda_config( NULL );

		if( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
  		{
			sprintf (task_name, "hci_ccz_legacy %s -O %d -name \"Clutter Regions\"", 
						channel_number, CENSOR_ZONES_BUTTON);
			sprintf (Control_panel_object[CENSOR_ZONES_BUTTON].app_name,
						"hci_ccz_legacy %s", channel_number);
		}
		else if( rda_config_flag == ORPGRDA_ORDA_CONFIG )
		{
			sprintf (task_name, "hci_ccz_orda %s -O %d -name \"Clutter Regions\"", 
						channel_number, CENSOR_ZONES_BUTTON);
			sprintf (Control_panel_object[CENSOR_ZONES_BUTTON].app_name,
						"hci_ccz_orda %s", channel_number);
		}
		else
		{
			HCI_LE_error("Error getting RDA Configuration (%d)", rda_config_flag );
			HCI_task_exit( HCI_EXIT_FAIL );
		}

		break;

/*	    Clutter Bypass Map Edit task				*/

	    case BYPASS_MAP_BUTTON :

		HCI_LE_log("Clutter Bypass Map Display Button selected");
		sprintf (window_name,"Clutter Bypass Map Display");
		if( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
  		{
			sprintf (task_name, "hci_cbm_legacy %s -O %d -name \"Clutter Bypass Map Display\"", 
						channel_number, BYPASS_MAP_BUTTON);
			sprintf (Control_panel_object[BYPASS_MAP_BUTTON].app_name,
						"hci_cbm_legacy %s", channel_number);
		}
		else if( rda_config_flag == ORPGRDA_ORDA_CONFIG )
		{
			sprintf (task_name, "hci_cbm_orda %s -O %d -name \"Clutter Bypass Map Display\"", 
						channel_number, BYPASS_MAP_BUTTON);
			sprintf (Control_panel_object[BYPASS_MAP_BUTTON].app_name,
						"hci_cbm_orda %s", channel_number);
		}
		else
		{
			HCI_LE_error("Error getting RDA Configuration, Return value is %d.", rda_config_flag );
			HCI_task_exit( HCI_EXIT_FAIL );
		}
		break;

/*	    Product Distribution Comms Status task			*/

	    case NARROWBAND_OBJECT:
	    case COMMS_BUTTON:

		HCI_LE_log("Narrowband Comms Button selected");
		sprintf (window_name,"Product Distribution Comms Status");
	    	sprintf (task_name, "hci_nb %s -O %d -name \"Product Distribution Comms Status\"", channel_number, COMMS_BUTTON);
	    	break;

/*	    Precipitation Status Status task			*/

	    case LOAD_SHED_OBJECT:

		HCI_LE_log("Load Shed Categories selected");
		sprintf (window_name,"Load Shed Categories");
	    	sprintf (task_name, "hci_load %s -O %d -name \"Load Shed Categories\"", channel_number, LOAD_SHED_OBJECT);
	    	break;

/*	    VCP Status task			*/

	    case VCP_CONTROL_OBJECT:

		HCI_LE_log("VCP Control selected");
		sprintf (window_name,"VCP and Mode Control");
	    	sprintf (task_name, "hci_vcp %s -O %d -name \"VCP and Mode Control\"", channel_number, VCP_CONTROL_OBJECT);
	    	break;

/*          PRF Status task			*/

            case PRFMODE_STATUS_OBJECT:

	        HCI_LE_log("PRF Mode Status selected" );
                if (((ORPGRDA_get_status (RS_CONTROL_STATUS) == RDA_IN_CONTROL) ||
                    (ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT) != RS_CONNECTED))
                    && ORPGMISC_is_operational() )
                {
                    sprintf( buf, "PRF Control is not allowed while the RDA\nis in local control or the RDA-RPG link is\nnot connected." );
                    hci_warning_popup( Control_panel_object[TOP_WIDGET].widget, buf, NULL );
                    return;

                } else {

                    sprintf (window_name, "PRF Control" );
                    sprintf (task_name, "hci_prf %s -O %d -name \"PRF Control\"", channel_number, PRFMODE_STATUS_OBJECT);

                }
                break;

/*	    Precipitation Status Status task			*/

	    case PRECIP_STATUS_OBJECT:

		HCI_LE_log("Precipitation Status selected");
		sprintf (window_name,"Precipitation Status");
	    	sprintf (task_name, "hci_precip_status %s -O %d -name \"Precipitation Status\"", channel_number, PRECIP_STATUS_OBJECT);
	    	break;

	    case MODE_STATUS_OBJECT:

		HCI_LE_log("Mode Status selected");
		sprintf (window_name,"Mode Automation Status");
	    	sprintf (task_name, "hci_mode_status %s -O %d -name \"Mode Automation Status\"", channel_number, MODE_STATUS_OBJECT);
	    	break;

/*	    RDA-RPG Interface Control/Status task			*/

	    case WIDEBAND_OBJECT:

		HCI_LE_log("RDA/RPG Interface Control Button Status selected");
		sprintf (window_name,"RDA/RPG Interface Control/Status");
	    	sprintf (task_name, "hci_rda_link %s -O %d -name \"RDA/RPG Interface Control/Status\"", channel_number, WIDEBAND_OBJECT);
	    	break;

/*	    Performance Check launches RDA Control task */

	    case PERFCHECK_STATUS_OBJECT:

		HCI_LE_log("Performance Check object selected");
		sprintf (window_name,"RDA Control");
       		if ( rda_config_flag == ORPGRDA_LEGACY_CONFIG )
        	{
			sprintf (task_name,
               	          "hci_rdc_legacy %s -O %d -name \"RDA Control\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
        	else if ( rda_config_flag == ORPGRDA_ORDA_CONFIG ) 
        	{
			sprintf (task_name,
               	          "hci_rdc_orda %s -O %d -name \"RDA Control\"",
               	          channel_number, RDA_ALARMS_BUTTON);
        	}
		break;

/*	    SAILS Control task */

	    case SAILS_STATUS_OBJECT:

		HCI_LE_log("SAILS object selected");
		sprintf (window_name,"SAILS Control");
		sprintf (task_name,
               	          "hci_sails %s -O %d -name \"SAILS Control\"",
               	          channel_number, SAILS_STATUS_OBJECT);
		break;

	}

/*	If a commandline built, then add any additional commandline	*
 *	options and use a "system()" call to start it.			*/

	if (strlen(task_name) > 0) {

            strcat(task_name, HCI_child_options_string());

	    HCI_LE_log("Spawning %s", task_name);
 
	    HCI_LE_log("Control_panel_object [%d].app_name [%s]",
			(int) client_data,
			Control_panel_object [((int) client_data)].app_name);

	    status = hci_activate_child (Hci_display,
		                         RootWindowOfScreen (HCI_get_screen()),
					 task_name,
					 Control_panel_object [((int) client_data)].app_name,
					 window_name,
					 (int) client_data);

	}

}

/************************************************************************
 *	Description: The following function displays a string in the	*
 *		     feedback line of the RPG Control/Status window	*
 *		     when an ORPGEVT_HCI_COMMAND_ISSUED event is	*
 *		     detected.	It is assumed that a string is passed	*
 *		     with the event.					*
 *									*
 *	Input:  evtcd   - Event code activating module			*
 *		ptr     - Pointer to message string			*
 *		msglen  - Length of message string			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_hci_command_issued (
en_t	evtcd,
void	*ptr,
size_t	msglen
)
{
	char	cmd [128];

/*	Truncate the mesage to 128 characters if it is longer than that	*/

	if (msglen >127) {

	    msglen = 128;

	}

	strncpy (cmd, ptr, msglen);

/*	Display the message in the feedback line.			*/

	hci_set_display_feedback_string( cmd );
}

/************************************************************************
 *	Description: The following function returns a pointer to the	*
 *		     data structure containing the specified window	*
 *		     object.						*
 *									*
 *	Input:  object - Object identifier				*
 *	Output: NONE							*
 *	Return: Pointer to specified object data			*
 ************************************************************************/

hci_control_panel_object_t
*hci_control_panel_object (
int	object
)
{
	if ((object >= 0) &&
	    (object < LAST_OBJECT)) {

	    return (hci_control_panel_object_t *) &Control_panel_object [object];

	} else {

	    return (hci_control_panel_object_t *) NULL;

	}
}

/************************************************************************
 *	Description: The following function draws a 3D border around	*
 *		     an object.						*
 *									*
 *	Input:  pixel	- leftmost pixel of object			*
 *		scanl	- topmost scanline of object			*
 *		width	- width (pixels) of object			*
 *		height	- height (scanlines) of object			*
 *		color	- color (use macro name) for border		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_draw_3d (
int	pixel,
int	scanl,
int	width,
int	height,
int	color
)
{
	int	i;

	XSetForeground (HCI_get_display(),
			hci_control_panel_gc      (),
			hci_get_read_color (color));

	for (i=0;i<=hci_control_panel_3d ();i++) {

	    XDrawLine (HCI_get_display(),
		       hci_control_panel_pixmap  (),
		       hci_control_panel_gc      (),
		       (int) (pixel + i),
		       (int) (scanl - i),
		       (int) (pixel + width + i),
		       (int) (scanl - i));

	    XDrawLine (HCI_get_display(),
		       hci_control_panel_pixmap  (),
		       hci_control_panel_gc      (),
		       (int) (pixel + width + i),
		       (int) (scanl - i),
		       (int) (pixel + width + i),
		       (int) (scanl + height - i));

	}
}

/************************************************************************
 *	Description: The following function is called when the RPG	*
 *		     Control/Status window is closed.			*
 *									*
 *	Input:  w           - ID of top widget				*
 *	        client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

int
destroy_callback ()
{
  char cmd[ 48 ];

  if( launched_rda_pmd )
  {
    if( rda_pmd_channel_number == 2 )
    {
      strcpy( cmd, "startCh2RpgPmd.sh stop > /dev/null 2>&1 &" );
    }
    else
    {
      strcpy( cmd, "startCh1RpgPmd.sh stop > /dev/null 2>&1 &" );
    }
  
    /* Don't check return status. We don't care, since it is
       launched in the background and the HCI is about to close. */ 

    MISC_system_to_buffer( cmd, NULL, 0, NULL );
  }

  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *	Description: The following function is called when the user	*
 *		     selects "Continue" from an information popup.	*
 *									*
 *	Input:  w           - ID of top widget				*
 *	        client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
acknowledge_invalid_value (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
}

/************************************************************************
 *	Description: Launch RDA PMD GUI.				*
 ************************************************************************/

void
hci_launch_rda_pmd()
{
  int ret;
  hci_child_started_event_t   task_data;
  char buf[ 128 ];

  rda_pmd_channel_number = 1; /* Default */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
     if( ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) == 2 )
     {
       rda_pmd_channel_number = 2;
     }
  }
  else if( HCI_get_system() == HCI_NWSR_SYSTEM )
  {
     if( ORPGRDA_channel_num() == 2 )
     {
       rda_pmd_channel_number = 2;
     }
  }

  HCI_LE_log("Attempting to launch RDA PMD GUI for channel %d", rda_pmd_channel_number );

  if( rda_pmd_channel_number == 2 )
  {
    strcpy( buf, "startCh2RpgPmd.sh start > /dev/null 2>&1 &" );
  }
  else
  {
    strcpy( buf, "startCh1RpgPmd.sh start > /dev/null 2>&1 &" );
  }

  ret = MISC_system_to_buffer( buf, NULL, 0, NULL );
  ret = ret >> 8;

  if( ret < 0 )
  {
    HCI_LE_error("MISC_system_to_buffer_failed (%d)",ret);
    HCI_LE_error("Unable to launch RDA PMD GUI");
    hci_error_popup( Control_panel_object[TOP_WIDGET].widget, "Unable to display RDA Performance Data GUI", NULL );
  }
  else
  {
    launched_rda_pmd = 1;
    HCI_LE_log("Successfully launched RDA PMD GUI");
  }

  /* Signal HCI to reset flag/cursor. */

  task_data.child_id = RDA_PERFORMANCE_BUTTON;
  EN_post (ORPGEVT_HCI_CHILD_IS_STARTED, &task_data,
           sizeof (hci_child_started_event_t), 0);
}


