/************************************************************************
 *									*
 *	Module:  hci_display_rpg_vcp_definitions.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to display RPG VCP adaptation data.	 	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:41:47 $
 * $Id: hci_display_rpg_vcp_definitions.c,v 1.21 2014/11/07 21:41:47 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */

/*	Local include files definitions.				*/

#include <hci.h>
#include <hci_vcp_data.h>
#include <hci_rda_adaptation_data.h>

/*	Local constants.						*/

#define	PRF_LABEL		0
#define	RMAX_LABEL		1
#define	SCROLL_WINDOW_HEIGHT	300
#define	SCROLL_WINDOW_WIDTH	1025

/*	Global widget variables.					*/

static	Widget	Display_vcp_dialog = (Widget) NULL;
static	Widget	Display_vcp_form   = (Widget) NULL;
static	Widget	Data_scroll       = (Widget) NULL;
static	Widget	Data_form         = (Widget) NULL;

static	int	Vcp_number [VCPMAX+1]; /* VCP number lookup table. */
static	int	Number_vcps = 0; /* Number of VCPs in VCP buffer */
static	int	Prf_rmax_flag  = PRF_LABEL; /* PRF/RMAX mode */

static	Widget	Prf1_text  = (Widget) NULL;
static	Widget	Prf2_text  = (Widget) NULL;
static	Widget	Prf3_text  = (Widget) NULL;

static	Widget	VI_label = (Widget) NULL;

static	char	Buf [256]; /* shared buffer for string functions */
static	XmString Str; /* shared buffer for compound string functions */
static	Ele_attr *ele_attr; /* pointer to VCP elevation attributes */

static	int	Current_vcp = 0; /* Currently active VCP number */
static	Vcp_struct	Vcp; /* Unedited VCP data */

/*	Definition of the structure containing VCP table widgets	*/

typedef	struct {

	Widget	number;
	Widget	angle;
	Widget	rate;
	Widget	dual_pol;
	Widget	super_res;
	Widget	waveform;
	Widget	azi1;
	Widget	prf1;
	Widget	azi2;
	Widget	prf2;
	Widget	azi3;
	Widget	prf3;
	Widget	snr_surv;
	Widget	snr_vel;
	Widget	snr_spw;
	Widget	snr_zdr;
	Widget	snr_phase;
	Widget	snr_corr;

} vcp_table_widgets_t;

static	vcp_table_widgets_t	Vcp_table_widgets [VCP_MAXN_CUTS];

/*	Local prototypes						*/

void	display_vcp_close (Widget w,
		XtPointer client_data, XtPointer call_data);
void	display_vcp_select_callback (Widget w, XtPointer client_data,
		XtPointer call_data);
void	hci_vcp_prf_rmax_callback (Widget w, XtPointer client_data,
		XtPointer call_data);
void	hci_display_vcp_table_widgets ();
int	vcp_comp( const void *, const void * );
static void center_elevation_num( int );
static void center_elevation_degree( float );
static void center_scan_rate( int );
static void center_sector_prf( int );
static void center_sector_azimuth( float );
static void center_sector_range( int );
static void center_snr( float );

/************************************************************************
 *	Description: This function is activated from the VCP Control	*
 *		     task when the user selects the "View VCP		*
 *		     "Adaptation Data" button.				*
 *									*
 *	Input:  w - Id of calling widget				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_rpg_vcp_definitions (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Arg	arg [10];
	int	n;
	Widget	button;
	int	i,j;			/* looping variables */
	Widget	display_vcp_frame;
	Widget	display_vcp_rowcol;
	Widget	prf_rmax_option;
	Widget	clip_window;
	Widget	bottom_label;
	Widget	vcp_list;
	Vcp_struct	*vcp;
static	int	init_flag = 1;
	short	temp_vcp_array[VCPMAX];
	int	original_index = 0;


/*	Check to see if VCP dialog active.  If it is			*
 *	then do nothing and return.					*/

	if (Display_vcp_dialog != NULL)
	{
	  HCI_Shell_popup( Display_vcp_dialog );
	  return;
	}
	
	HCI_LE_log("View RPG VCP Definitions selected");
	    
/*	If low bandwidth, display a progress meter since we are going	*
 *	to do I/O.							*/

	HCI_PM( "Initialize VCP Adaptation Data information" );	

/*	Initialize the VCP info structure by updating the contents	*
 *	of the adaptation data containing the current VCP data.		*/

	if (init_flag) {

	    init_flag = 0;
	    Vcp_number [0] = 0;

	    for (i=1;i<VCPMAX+1;i++) {

		vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (i-1);
	        Vcp = *vcp;
	        Vcp_number [i] = Vcp.vcp_num;

	    }
	}

/*	Define the widgets which make up the display adaptation vcp	*
 *	form.								*/

	HCI_Shell_init( &Display_vcp_dialog, "View RPG VCP Definitions" );

/*	Use a form widget to manage placement of various widgets in	*
 *	window.								*/

	Display_vcp_form = XtVaCreateWidget ("display_rpg_vcp_defs_form",
		xmFormWidgetClass,	Display_vcp_dialog,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*	Use a rowcolumn widget to manage action selections		*/

	display_vcp_rowcol = XtVaCreateWidget ("top_rowcol",
		xmRowColumnWidgetClass,	Display_vcp_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	display_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, display_vcp_close, NULL);

	XtManageChild (display_vcp_rowcol);

/*	Create a container and a set of radio buttons based on the	*
 *	number of VCP's defined.					*/

	display_vcp_frame = XtVaCreateManagedWidget ("display_vcp_frame",
		xmFrameWidgetClass,	Display_vcp_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		display_vcp_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("VCP Selection",
		xmLabelWidgetClass,	display_vcp_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	display_vcp_rowcol = XtVaCreateWidget ("top_rowcol",
		xmRowColumnWidgetClass,	display_vcp_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL);       n++;

	vcp_list = XmCreateRadioBox (display_vcp_rowcol,
		"vcp_list", arg, n);

        /* Initialize number of "valid" vcps */

	Number_vcps = 0;

        /* Loop through vcps and pull out "valid" values */

        for (i=1;i<VCPMAX+1;i++)
        {
          if ((Vcp_number [i] != 0) && (Vcp_number [i] < 1000))
          {
            temp_vcp_array[ Number_vcps ] = Vcp_number[i];
            Number_vcps++;
          }
        }

        /* Sort vcps so gui buttons will be ordered. */
 
        qsort( temp_vcp_array, Number_vcps, sizeof(short), vcp_comp);

        for (i=0;i<Number_vcps;i++)
        {
          sprintf (Buf,"%d",temp_vcp_array[i]);
          Str = XmStringCreateLocalized (Buf);

          button = XtVaCreateManagedWidget ("vcp",
		xmToggleButtonWidgetClass,vcp_list,
		XmNlabelString,Str,
		XmNforeground,hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,hci_get_read_color (WHITE),
		XmNtraversalOn,False,
		XmNfontList,hci_get_fontlist (LIST),
		XmNset,False,
		NULL);

          XmStringFree (Str);

          for( j = 0; j< VCPMAX; j++ )
          {
            if( temp_vcp_array[i] == Vcp_number[j] )
            {
              original_index = j;
              break;
            }
          }

          XtAddCallback (button,
                         XmNvalueChangedCallback,
                         display_vcp_select_callback,
                         (XtPointer) j);

	  /* Make sure starting values of GUI are for the current VCP. */
          if ( temp_vcp_array[i] == hci_current_vcp() )
          {
	    XtVaSetValues(button,XmNset,True,NULL);
	    Current_vcp = original_index;
	    vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (Current_vcp-1);
	    Vcp = *vcp;
          }
	}

	XtManageChild (vcp_list);
	XtManageChild (display_vcp_rowcol);

/*	Create a set of radio buttons to control whether PRF number	*
 *	is displayed in the table or unambiguous range (RMAX).  Also	*
 *	define a set of radio buttons to control the velocity increment	*/

	display_vcp_frame = XtVaCreateManagedWidget ("display_vcp_frame",
		xmFrameWidgetClass,	Display_vcp_form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		display_vcp_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	display_vcp_rowcol = XtVaCreateWidget ("display_vcp_rowcol",
		xmRowColumnWidgetClass,	display_vcp_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtVaCreateManagedWidget ("Show:",
		xmLabelWidgetClass,	display_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	n = 0;

	XtSetArg (arg [n], XmNforeground,	hci_get_read_color (TEXT_FOREGROUND));    n++;
	XtSetArg (arg [n], XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (arg [n], XmNpacking,		XmPACK_TIGHT); n++;
	XtSetArg (arg [n], XmNorientation,	XmHORIZONTAL); n++;

	prf_rmax_option = XmCreateRadioBox (display_vcp_rowcol,
		"prf_rmax_option", arg, n);

	button = XtVaCreateManagedWidget ("PRF#",
		xmToggleButtonWidgetClass,	prf_rmax_option,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNtraversalOn,			False,
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	if (Prf_rmax_flag == PRF_LABEL) {
	
	    XtVaSetValues (button,
			XmNset,	True,
			NULL);

	} else {
	
	    XtVaSetValues (button,
			XmNset,	False,
			NULL);

	}

	XtAddCallback (button,
		XmNvalueChangedCallback, hci_vcp_prf_rmax_callback,
		(XtPointer) PRF_LABEL);

	button = XtVaCreateManagedWidget ("RMAX (NM)",
		xmToggleButtonWidgetClass,	prf_rmax_option,
		XmNforeground,			hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,			hci_get_read_color (WHITE),
		XmNtraversalOn,			False,
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	if (Prf_rmax_flag == RMAX_LABEL) {
	
	    XtVaSetValues (button,
			XmNset,	True,
			NULL);

	} else {
	
	    XtVaSetValues (button,
			XmNset,	False,
			NULL);

	}

	XtAddCallback (button,
		XmNvalueChangedCallback, hci_vcp_prf_rmax_callback,
		(XtPointer) RMAX_LABEL);

	XtManageChild (prf_rmax_option);

	if (Vcp.vel_resolution == HCI_VELOCITY_RESOLUTION_HIGH)
	{
	  VI_label = XtVaCreateManagedWidget ("     Velocity Increment: 0.97 kts",
		xmLabelWidgetClass,	display_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginTop,		8,
		NULL);
	}
	else
	{
	  VI_label = XtVaCreateManagedWidget ("     Velocity Increment: 1.94 kts",
		xmLabelWidgetClass,	display_vcp_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginTop,		8,
		NULL);
	}

	XtManageChild (display_vcp_rowcol);

/*	Create a scrolled list to manage the VCP data table.		*/

	Data_scroll = XtVaCreateManagedWidget ( "data_scroll",
		xmScrolledWindowWidgetClass,	Display_vcp_form,
		XmNscrollingPolicy,	XmAUTOMATIC,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		display_vcp_rowcol,
		XmNheight,		SCROLL_WINDOW_HEIGHT,
		XmNwidth,		SCROLL_WINDOW_WIDTH,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL );

	XtVaGetValues( Data_scroll, XmNclipWindow, &clip_window, NULL );
	XtVaSetValues( clip_window,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
		NULL );

/*	Below the table we want to display a label indicating the	*
 *	range for azimuth and range values.				*/

	bottom_label = XtVaCreateManagedWidget ("Azimuth Range: (0 to 359.9 Deg) -    SNR Range: (-12.0 to 20.0 dB)",
		xmLabelWidgetClass,	Display_vcp_form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Data_scroll,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	XtManageChild (Display_vcp_form);
	
/*	Build/fill table of values. */

	hci_display_vcp_table_widgets ();

	HCI_Shell_start( Display_vcp_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 *									*
 *	Input:  w - Id of calling widget				*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_vcp_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
        HCI_LE_log("Display Adaptation VCP Close button selected");
	HCI_Shell_popdown( Display_vcp_dialog );
}

/************************************************************************
 *	Description: This function is activated when the user changes	*
 *		     VCP number.  It is also called by the undo		*
 *		     callback in order to force a refresh of the table.	*
 *									*
 *	Input:  w - Id of calling widget (unused)			*
 *		client_data - VCP number				*
 *		call_data - toggle state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_vcp_select_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Vcp_struct	*vcp;
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

/*	    Set the current VCP table ID.		*/

	    Current_vcp = (int) client_data;

/*	    Get the pointer to the new table.		*/

	    vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (Current_vcp-1);
	    Vcp = *vcp;

/*	    Update the widgets in the VCP table.	*/

	    hci_display_vcp_table_widgets ();

	}
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the PRF/RMAX buttons.			*
 *									*
 *	Input:  w - Id of calling widget				*
 *		client_data -PRF_LABEL, RMAX_LABEL			*
 *		call_data - tobble state data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_vcp_prf_rmax_callback (
Widget		 w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	i;

	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    Prf_rmax_flag = (int) client_data;

	    if (Prf_rmax_flag == PRF_LABEL) {

		sprintf (Buf, " PRF # ");

	    } else {

		sprintf (Buf, " RMAX ");

	    }

	    XmTextSetString (Prf1_text, Buf);
	    XmTextSetString (Prf2_text, Buf);
	    XmTextSetString (Prf3_text, Buf);
	    for (i=0;i<Vcp.n_ele;i++) {

		ele_attr = (Ele_attr *) &Vcp.vcp_ele [i];

		if ( ele_attr->wave_type == WAVEFORM_CONTIGUOUS_SURVEILLANCE) {

		    if (Prf_rmax_flag == PRF_LABEL) {

			center_sector_prf((int) ele_attr->surv_prf_num);

		    } else {

			center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

		    }

		    XmTextSetString (Vcp_table_widgets [i].prf1, Buf);
		    XmTextSetString (Vcp_table_widgets [i].prf2, Buf);
		    XmTextSetString (Vcp_table_widgets [i].prf3, Buf);

		} else {

		    if (Prf_rmax_flag == PRF_LABEL) {

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_prf(ele_attr->surv_prf_num);

                        else
			   center_sector_prf(ele_attr->dop_prf_num_1);

			XmTextSetString (Vcp_table_widgets [i].prf1, Buf);

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_prf(ele_attr->surv_prf_num);

                        else
			   center_sector_prf(ele_attr->dop_prf_num_2);

			XmTextSetString (Vcp_table_widgets [i].prf2, Buf);

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_prf(ele_attr->surv_prf_num);

                        else
			   center_sector_prf(ele_attr->dop_prf_num_3);

			XmTextSetString (Vcp_table_widgets [i].prf3, Buf);

		    } else {

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                        else
			   center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_1)*HCI_KM_TO_NM + 0.5));

			XmTextSetString (Vcp_table_widgets [i].prf1, Buf);

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                        else
			   center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_2)*HCI_KM_TO_NM + 0.5));

			XmTextSetString (Vcp_table_widgets [i].prf2, Buf);

                        if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                        else
			   center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_3)*HCI_KM_TO_NM + 0.5));

			XmTextSetString (Vcp_table_widgets [i].prf3, Buf);

		    }
		}
	    }
	}
}

/************************************************************************
 *	Description: This function updates the widgets contained in the	*
 *		     View VCP Adaptation Data window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_display_vcp_table_widgets ()
{
	Widget	table_rowcol = NULL;
	Widget	text = NULL;
	Widget	label = NULL;
	int	i;
	int	doppler_cut_num = 0;
        float	elev_angle_deg;
	int	elev_angle_deg10;
	XmString label_string;

	doppler_cut_num = 0;

	if (Data_form != (Widget) NULL) {

	    Widget	vsb;
	    int		sz,incr,pg_incr;

	    /* Set slider to top. */

	    XtVaGetValues (Data_scroll,
		XmNverticalScrollBar,	&vsb,
		NULL);

	    XtVaGetValues (vsb,
		XmNsliderSize, &sz,
		XmNincrement, &incr,
		XmNpageIncrement, &pg_incr,
		NULL);

	    XmScrollBarSetValues(vsb, 0, sz, incr, pg_incr, 1 );

	    XtDestroyWidget (Data_form);

	}

/*	Create the form which will hold the VCP data table.  It is the	*
 *	immediate child of the 5croll widget.				*/

	Data_form = XtVaCreateWidget ("vcp_select_form",
		xmFormWidgetClass,	Data_scroll,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNverticalSpacing,	0,
		XmNhorizontalSpacing,	0,
		NULL);

        label = XtVaCreateManagedWidget ("Elevation     Scan     DP      SR   Waveform    Sector 1        Sector 2        Sector 3                Signal/Noise Ratio (dB)        ",
		xmLabelWidgetClass,	Data_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

        table_rowcol = XtVaCreateWidget ("table_rowcol",
                xmRowColumnWidgetClass, Data_form,
                XmNorientation,         XmHORIZONTAL,
                XmNpacking,             XmPACK_COLUMN,
                XmNbackground,          hci_get_read_color (BLACK),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           label,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNspacing,             0,
                XmNmarginWidth,         0,
                XmNmarginHeight,        0,
                XmNentryAlignment,      XmALIGNMENT_CENTER,
                XmNnavigationType,      XmNONE,
                XmNtraversalOn,         False,
                NULL);

        sprintf (Buf,"   #   ");

        text = XtVaCreateManagedWidget ("num",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             4,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf,"  Deg  ");

        text = XtVaCreateManagedWidget ("deg",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf,"  Sec  ");

        text = XtVaCreateManagedWidget ("sec",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             5,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf,"  Y/N  ");

        text = XtVaCreateManagedWidget ("dp",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             4,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);


        sprintf (Buf,"  Y/N  ");

        text = XtVaCreateManagedWidget ("sr",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             4,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);


        sprintf (Buf,"  Type ");

        text = XtVaCreateManagedWidget ("type",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf,"  Azm  ");

        text = XtVaCreateManagedWidget ("azm",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNnavigationType,      XmNONE,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XmTextSetString (text, Buf);

        if (Prf_rmax_flag == PRF_LABEL) {

           sprintf (Buf," PRF # ");

        } else {

           sprintf (Buf,"  RMAX ");

        }

        Prf1_text = XtVaCreateManagedWidget ("prf#",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             5,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (Prf1_text, Buf);

        sprintf (Buf,"  Azm  ");

        text = XtVaCreateManagedWidget ("azm",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

        if (Prf_rmax_flag == PRF_LABEL) {

           sprintf (Buf," PRF # ");

        } else {

           sprintf (Buf,"  RMAX ");

        }

        Prf2_text = XtVaCreateManagedWidget ("prf#",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             5,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (Prf2_text, Buf);

        sprintf (Buf,"  Azm  ");

        text = XtVaCreateManagedWidget ("azm",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

        if (Prf_rmax_flag == PRF_LABEL) {

           sprintf (Buf," PRF # ");

        } else {

           sprintf (Buf,"  RMAX ");

        }

        Prf3_text = XtVaCreateManagedWidget ("prf#",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             5,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (Prf3_text, Buf);

        sprintf (Buf,"  Refl ");

        text = XtVaCreateManagedWidget ("Refl",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf,"  Vel  ");

        text = XtVaCreateManagedWidget ("velocity",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

        sprintf (Buf," Width ");

        text = XtVaCreateManagedWidget ("width",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

       sprintf (Buf,"  Zdr  ");

        text = XtVaCreateManagedWidget ("zdr",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

       sprintf (Buf," Phase ");

        text = XtVaCreateManagedWidget ("phase",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

       sprintf (Buf,"  Corr ");

        text = XtVaCreateManagedWidget ("corr",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             7,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                NULL);

        XmTextSetString (text, Buf);

        XtManageChild (table_rowcol);

/*	For each cut defined in the VCP create a table entry.	*/

	for (i=0;i<Vcp.n_ele;i++) {

	    table_rowcol = XtVaCreateWidget ("table_rowcol",
			xmRowColumnWidgetClass,	Data_form,
			XmNorientation,		XmHORIZONTAL,
			XmNpacking,		XmPACK_COLUMN,
			XmNbackground,		hci_get_read_color (BLACK),
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		table_rowcol,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNspacing,		0,
			XmNmarginWidth,		0,
			XmNmarginHeight,	0,
			XmNborderWidth,		0,
			XmNentryAlignment,	XmALIGNMENT_CENTER,
			NULL);

	    ele_attr = (Ele_attr *) &Vcp.vcp_ele [i];

	    center_elevation_num( i+1 );

	    Vcp_table_widgets [i].number = XtVaCreateManagedWidget ("num",
		xmTextFieldWidgetClass,	table_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNcolumns,		4,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNeditable,		False,
		XmNsensitive,		True,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	    XmTextSetString (Vcp_table_widgets [i].number, Buf);

	    elev_angle_deg = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                                  ele_attr->ele_angle );
            if( elev_angle_deg >= 0.0 )
               elev_angle_deg10 = elev_angle_deg*10.0 + 0.5;
            else
               elev_angle_deg10 = elev_angle_deg*10.0 - 0.5;

	    center_elevation_degree( (float) elev_angle_deg10/10.0 );

	    Vcp_table_widgets [i].angle = XtVaCreateManagedWidget ("angle",
		xmTextFieldWidgetClass,	table_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNeditable,		False,
		XmNsensitive,		True,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	    XmTextSetString (Vcp_table_widgets [i].angle, Buf);

	    center_scan_rate( (int) (360/(ele_attr->azi_rate*AZI_RATE_SCALE)+0.5));

	    Vcp_table_widgets [i].rate = XtVaCreateManagedWidget ("rate",
		xmTextFieldWidgetClass,	table_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNcolumns,		5,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNeditable,		False,
		XmNsensitive,		True,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	    XmTextSetString (Vcp_table_widgets [i].rate, Buf);

            if( (ele_attr->super_res & VCP_DUAL_POL_ENABLED) != 0 )
                sprintf (Buf, "   Y   " );
            else
                sprintf (Buf, "   N   " );

            Vcp_table_widgets [i].dual_pol = XtVaCreateManagedWidget (" DP ",
                xmTextFieldWidgetClass, table_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNcolumns,             4,
                XmNmarginHeight,        0,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNuserData,            (XtPointer) i,
                XmNeditable,            False,
                XmNsensitive,           True,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNnavigationType,      XmNONE,
                XmNtraversalOn,         False,
                NULL);

            XmTextSetString (Vcp_table_widgets [i].dual_pol, Buf);

            if( (ele_attr->super_res & HCI_ELEV_SR_BITMASK) != 0 )
		sprintf (Buf, "   Y   " );
            else
		sprintf (Buf, "   N   " );

	    Vcp_table_widgets [i].super_res = XtVaCreateManagedWidget (" SR ",
		xmTextFieldWidgetClass,	table_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNcolumns,		4,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNeditable,		False,
		XmNsensitive,		True,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	    XmTextSetString (Vcp_table_widgets [i].super_res, Buf);

	    switch ( ele_attr->wave_type ) {

		case WAVEFORM_CONTIGUOUS_SURVEILLANCE :

		    if( ele_attr->phase == VCP_PHASE_SZ2 )
	    	        sprintf (Buf," CS/SZ2");
		    else
	    	        sprintf (Buf,"  CS/W ");
		    break;

		case WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB :

		    if( ele_attr->phase == VCP_PHASE_SZ2 )
	    	        sprintf (Buf," CD/SZ2");
		    else
	    	        sprintf (Buf,"  CD/W ");
		    break;

		case WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB :

	    	    sprintf (Buf," CD/WO ");
		    break;

		case WAVEFORM_BATCH :

	    	    sprintf (Buf,"   B   ");
		    break;

		case WAVEFORM_STAGGERED_PULSE_PAIR :

	    	    sprintf (Buf,"   SP  ");
		    break;

		default :

		    break;

	    }

	    Vcp_table_widgets [i].waveform = XtVaCreateManagedWidget ("waveform",
		xmTextFieldWidgetClass,	table_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNcolumns,		7,
		XmNmarginHeight,	0,
		XmNshadowThickness,	2,
		XmNmarginWidth,		0,
		XmNborderWidth,		0,
		XmNuserData,		(XtPointer) i,
		XmNeditable,		False,
		XmNsensitive,		True,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNnavigationType,	XmNONE,
		XmNtraversalOn,		False,
		NULL);

	    XmTextSetString (Vcp_table_widgets [i].waveform, Buf);

	    if ( ele_attr->wave_type == WAVEFORM_CONTIGUOUS_SURVEILLANCE ) {

		center_sector_azimuth((double)((int) (ele_attr->azi_ang_1/VCP_AZIMUTH_SCALE+ 0.5)));
		Vcp_table_widgets [i].azi1 = XtVaCreateManagedWidget ("azimuth1",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi1, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

		    center_sector_prf(ele_attr->surv_prf_num);

		} else {

		    center_sector_range( (int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf1 = XtVaCreateManagedWidget ("prf1",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf1, Buf);

		center_sector_azimuth((double)((int) (ele_attr->azi_ang_2/VCP_AZIMUTH_SCALE+0.5)));

		Vcp_table_widgets [i].azi2 = XtVaCreateManagedWidget ("azimuth2",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi2, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

		    center_sector_prf(ele_attr->surv_prf_num);

		} else {

		    center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf2 = XtVaCreateManagedWidget ("prf2",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf2, Buf);

		center_sector_azimuth((double)((int) (ele_attr->azi_ang_3/VCP_AZIMUTH_SCALE+0.5)));
		Vcp_table_widgets [i].azi3 = XtVaCreateManagedWidget ("azimuth3",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi3, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

		    center_sector_prf(ele_attr->surv_prf_num);

		} else {

		    center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf3 = XtVaCreateManagedWidget ("prf3",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf3, Buf);

		center_snr( ele_attr->surv_thr_parm/8.0 );
		Vcp_table_widgets [i].snr_surv = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_surv, Buf);

		center_snr( ele_attr->vel_thrsh_parm/8.0 );
		Vcp_table_widgets [i].snr_vel = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_vel, Buf);

		center_snr( ele_attr->spw_thrsh_parm/8.0 );
		Vcp_table_widgets [i].snr_spw = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_spw, Buf);

		center_snr( ele_attr->zdr_thrsh_parm/8.0 );
                Vcp_table_widgets [i].snr_zdr = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground, hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground, hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_zdr, Buf);

		center_snr( ele_attr->phase_thrsh_parm/8.0 );
                Vcp_table_widgets [i].snr_phase = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_phase, Buf);

		center_snr( ele_attr->corr_thrsh_parm/8.0 );
                Vcp_table_widgets [i].snr_corr = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_corr, Buf);

	    } else {

		center_sector_azimuth(ele_attr->azi_ang_1/VCP_AZIMUTH_SCALE);
		Vcp_table_widgets [i].azi1 = XtVaCreateManagedWidget ("1",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi1, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
		       center_sector_prf(ele_attr->surv_prf_num);

                    else
		       center_sector_prf(ele_attr->dop_prf_num_1);

		} else {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
		       center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                    else
		       center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_1)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf1 = XtVaCreateManagedWidget ("1",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf1, Buf);

		center_sector_azimuth(ele_attr->azi_ang_2/VCP_AZIMUTH_SCALE);
		Vcp_table_widgets [i].azi2 = XtVaCreateManagedWidget ("2",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi2, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
		       center_sector_prf(ele_attr->surv_prf_num);

                    else
		       center_sector_prf(ele_attr->dop_prf_num_2);

		} else {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
			   center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                    else
		       center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_2)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf2 = XtVaCreateManagedWidget ("2",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf2, Buf);

		center_sector_azimuth(ele_attr->azi_ang_3/VCP_AZIMUTH_SCALE);
		Vcp_table_widgets [i].azi3 = XtVaCreateManagedWidget ("3",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].azi3, Buf);

		if (Prf_rmax_flag == PRF_LABEL) {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
		       center_sector_prf(ele_attr->surv_prf_num);

                    else
		       center_sector_prf(ele_attr->dop_prf_num_3);

		} else {

                    if( ele_attr->wave_type == WAVEFORM_STAGGERED_PULSE_PAIR )
		       center_sector_range((int)(hci_get_unambiguous_range_sprt (hci_rda_adapt_delta_pri (), ele_attr->surv_prf_num)*HCI_KM_TO_NM + 0.5));

                    else
		       center_sector_range((int)(hci_get_unambiguous_range (hci_rda_adapt_delta_pri (), ele_attr->dop_prf_num_3)*HCI_KM_TO_NM + 0.5));

		}

		Vcp_table_widgets [i].prf3 = XtVaCreateManagedWidget ("3",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		5,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].prf3, Buf);

		center_snr(ele_attr->surv_thr_parm/8.0);
		Vcp_table_widgets [i].snr_surv = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_surv, Buf);

		center_snr(ele_attr->vel_thrsh_parm/8.0);
		Vcp_table_widgets [i].snr_vel = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_vel, Buf);

		center_snr(ele_attr->spw_thrsh_parm/8.0);
		Vcp_table_widgets [i].snr_spw = XtVaCreateManagedWidget ("0",
			xmTextFieldWidgetClass,	table_rowcol,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNcolumns,		7,
			XmNmarginHeight,	0,
			XmNshadowThickness,	2,
			XmNmarginWidth,		0,
			XmNborderWidth,		0,
			XmNuserData,		(XtPointer) i,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNeditable,		False,
			XmNsensitive,		True,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNnavigationType,	XmNONE,
			XmNtraversalOn,		False,
			NULL);

		XmTextSetString (Vcp_table_widgets [i].snr_spw, Buf);

		doppler_cut_num++;

                center_snr(ele_attr->zdr_thrsh_parm/8.0);
                Vcp_table_widgets [i].snr_zdr = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_zdr, Buf);

                center_snr(ele_attr->phase_thrsh_parm/8.0);
                Vcp_table_widgets [i].snr_phase = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_phase, Buf);

                center_snr(ele_attr->corr_thrsh_parm/8.0);
                Vcp_table_widgets [i].snr_corr = XtVaCreateManagedWidget ("0",
                        xmTextFieldWidgetClass, table_rowcol,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNcolumns,             7,
                        XmNmarginHeight,        0,
                        XmNshadowThickness,     2,
                        XmNmarginWidth,         0,
                        XmNborderWidth,         0,
                        XmNuserData,            (XtPointer) i,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNeditable,            False,
                        XmNsensitive,           True,
                        XmNfontList,            hci_get_fontlist (LIST),
                        XmNnavigationType,      XmNONE,
                        XmNtraversalOn,         False,
                        NULL);

                XmTextSetString (Vcp_table_widgets [i].snr_corr, Buf);

	    }

	    XtManageChild (table_rowcol);

	}

	XtManageChild (Data_form);
	XtManageChild (XtParent (Data_form));

	if( Vcp.vel_resolution == HCI_VELOCITY_RESOLUTION_HIGH )
	{
	  label_string = XmStringCreateLocalized( "     Velocity Increment: 0.97 kts" );
	}
	else
	{
	  label_string = XmStringCreateLocalized( "     Velocity Increment: 1.94 kts" );
	}
	XtVaSetValues( VI_label, XmNlabelString, label_string, NULL );	
	XmStringFree( label_string );
}

static void center_elevation_num( int num )
{
  if( num < 10 ){ sprintf( Buf, "   %1d   ", num ); }
  else{ sprintf( Buf, "   %2d  ", num ); }
}

static void center_elevation_degree( float deg )
{
  if( deg < 10.0 )
  {
    sprintf (Buf,"  %3.1f  ", deg );
  }
  else
  {
    sprintf (Buf,"  %4.1f ", deg );
  }
}

static void center_scan_rate( int rate )
{
  sprintf( Buf, "  %3d  ", rate );
}

static void center_sector_azimuth( float az )
{
  if( az < 10.0 ){ sprintf( Buf,"  %3.1f  ", az ); }
  else if( az < 100.0 ){ sprintf( Buf,"  %4.1f ", az ); }
  else{ sprintf( Buf," %5.1f ", az ); }
}

static void center_sector_range( int range )
{
  if( range < 10 ){ sprintf( Buf,"   %1d   ", range ); }
  else if( range < 100 ){ sprintf( Buf,"   %2d  ", range ); }
  else{ sprintf( Buf,"  %3d  ", range ); }
}

static void center_sector_prf( int prf )
{
  sprintf( Buf,"   %1d   ", prf );
}

static void center_snr( float snr )
{
  if( snr <= -9.99 ){ sprintf( Buf, " %5.1f ", snr ); }
  else if( snr < 0.0 ){ sprintf( Buf, "  %4.1f ", snr ); }
  else if( snr < 10.0 ){ sprintf( Buf, "  %3.1f  ", snr ); }
  else{ sprintf( Buf, "  %4.1f ", snr ); }
}

