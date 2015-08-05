/****************************************************************
 *								*
 *	hci_product_status.c - This routine is used to display	*
 *	the products which are available in the ORPG database.	*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:19 $
 * $Id: hci_product_status.c,v 1.60 2009/02/27 22:26:19 ccalvert Exp $
 * $Revision: 1.60 $
 * $State: Exp $
 */


/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros.								*/

#define	MAX_PRODUCTS		1000	/* max # product codes */
#define	MIN_PRODUCT_CODE	16	/* min product code to consider */
#define	MAX_PRODUCTS_IN_LIST	8000	/* max products in DB query */

enum	{VOLUME, HOUR, MINUTE};

/*	Global widget declarations.					*/

Widget	Top_widget         = (Widget) NULL;
Widget	Form               = (Widget) NULL;
Widget	Data_scroll        = (Widget) NULL;
Widget	Current_scan_info  = (Widget) NULL;
Widget	Volume_time        = (Widget) NULL;
Widget	Volume_time_list   = (Widget) NULL;
Widget	Product            = (Widget) NULL;
Widget	Product_list       = (Widget) NULL;

int	Current_buf_num		= 0;	/* product ID of active product */
int	Current_volume_time	= 0;	/* volume time of active volume */
int	Update_flag = HCI_NO_FLAG;

char	*Prod_status_msg = (char *) NULL; /* product status data buffer */
char	*Prod_summary    = (char *) NULL; /* product summary data buffer */

ORPGDBM_query_data_t	Query_data [16]; /* productquery parameters */
RPG_prod_rec_t		Db_info [MAX_PRODUCTS_IN_LIST]; /* product query
					    results */

/*	X properties							*/

char	*Buf;		/* common buffer */
char	buf [128];	/* common buffer for string operations */
int	M = 0;
int	Num_products     = 0;	/* Number of products in product menu */
int	Num_volume_times = 0;	/* Number of timess in time menu */

int	List_product     [MAX_PRODUCTS]; /* products in product menu */
int	List_volume_time [MAX_PRODUCTS]; /* times in time menu */

char	*Wx_mode_code [] = {
	" ",
	"A",
	"B"
};

void	select_new_product_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void	select_new_volume_time (Widget w,
		XtPointer client_data, XtPointer call_data);
void	update_button_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	update_info ();
void	close_product_status_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	timer_proc();

int	hci_display_product_status_data (int buf_num);

int	hci_update_product_list ();
int	hci_update_volume_time_list ();

/************************************************************************
 *	Description: This is the main function for the RPG Products	*
 *		     in Database task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	int	i, j;
	int	found;
	int	buf_num;
	Widget	frame;
	Widget	button;
	Widget	label;
	Widget	form;
	Widget	rowcol;
	Widget	time_frame;
	Widget	show_rowcol;
	Widget	show_frame;
	Widget	select_frame;
	int		status;
	XmString	str;
	int		month, day, year;
	int		hour, minute, second;
	long int	seconds;
	int		products;

	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_PSTAT_TASK );

	Top_widget = HCI_get_top_widget();

/*	Build wigets.							*/

	form = XtVaCreateWidget ("product_status_form",
		xmFormWidgetClass,	Top_widget,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);
		
	XtAddCallback (form,
		XmNfocusCallback, hci_force_resize_callback,
		(XtPointer) Top_widget);

/*	If low bandwidth, dipslay a progress meter.			*/

	HCI_PM( "Initialize Task Information" );		
       
	rowcol = XtVaCreateWidget ("product_status_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, close_product_status_callback,
		NULL);

	Current_scan_info = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (rowcol);

	label = XtVaCreateManagedWidget ("Spacer    ",
		xmLabelWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	show_frame = XtVaCreateManagedWidget ("show_frame",
		xmFrameWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		label,
		NULL);

	show_rowcol = XtVaCreateWidget ("show_rowcol",
		xmRowColumnWidgetClass,	show_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		NULL);

	label = XtVaCreateManagedWidget ("Blank",
		xmLabelWidgetClass,	show_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	0,
		NULL);

	time_frame = XtVaCreateManagedWidget ("show_frame",
		xmFrameWidgetClass,	show_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Search Date/Time",
		xmLabelWidgetClass,	time_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Use a Combo Box widget for handling the volume time selection 	*
 *	and list.  A scrolled menu is presented which contains each	*
 *	time which is represented in the ORPG Products Database.	*/

	Volume_time = XtVaCreateWidget ("volume_time_list",
		xmComboBoxWidgetClass,	time_frame,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		25,
		NULL);
	
	XtVaGetValues (Volume_time,
		XmNlist,	&Volume_time_list,
		NULL);

	Num_volume_times = 0;

/*	Next, query the database for all unique volume times so we	*
 *	can build a list of the different times which are represented	*
 *	in the ORPG Products Database.					*/

	HCI_PM( "Query product database for volume times" );		

	Query_data[1].field = RPGP_VOLT;
	Query_data[1].value = 0;
	Query_data[0].field = ORPGDBM_MODE;
	Query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			      ORPGDBM_DISTINCT_FIELD_VALUES;

	products = ORPGDBM_query (Db_info,
				  Query_data,
				  2,
				  MAX_PRODUCTS_IN_LIST);
				  
	if (products == RMT_CANCELLED)
	   HCI_task_exit(HCI_EXIT_SUCCESS);

	for (i=0;i<products;i++) {

	    found = 0;

	    for (j=0;j<i;j++) {

		if (Db_info[i].vol_t == Db_info[j].vol_t) {

		    found = 1;
		    break;

		}
	    }

/*	    If the time was not previously detected, add item to	*
 *	    combo list.							*/

	    if (found == 0) {

		List_volume_time [Num_volume_times++] = Db_info[i].vol_t;

		seconds = Db_info[i].vol_t;

		unix_time (&seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

		sprintf (buf,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
			HCI_get_month(month), day, year,
			hour, minute, second);
		str = XmStringCreateLocalized (buf);
		XmListAddItemUnselected (Volume_time_list, str, 0);
		XmStringFree (str);

	    }
	}

	Current_volume_time = List_volume_time [0];

	XtManageChild (Volume_time);
	
	XtVaSetValues (Volume_time,
		XmNselectedPosition,	0,
		NULL);

	XtAddCallback (Volume_time,
		XmNselectionCallback, select_new_volume_time, NULL);
	
	label = XtVaCreateManagedWidget ("Blank",
		xmLabelWidgetClass,	show_rowcol,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNmarginHeight,	0,
		NULL);
	XtManageChild (show_rowcol);

	select_frame = XtVaCreateManagedWidget ("show_frame",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		show_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Select Product",
		xmLabelWidgetClass,	select_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Use a Combo Box widget for handling the product selection and	*
 *	list.  A scrolled menu is presented which contains each product	*
 *	which is contained in the ORPG Products Database.		*/

	Product = XtVaCreateWidget ("product_list",
		xmComboBoxWidgetClass,	select_frame,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		NULL);
	
	XtVaGetValues (Product,
		XmNlist,	&Product_list,
		NULL);

/*	First create a button for for selecting ALL product codes	*/

	str = XmStringCreateLocalized ("All Products");
	XmListAddItemUnselected (Product_list, str, 0);
	XmStringFree (str);

	Num_products     = 0;
	List_product [0] = 0;

/*	Next, query the database for all unique product codes so we	*
 *	can build a list of the different products which are in the	*
 *	ORPG Products Database.  This is a better design than showing	*
 *	a list of all distributable products defined in the Product	*
 *	Attributes Table (PAT) since some of those products may not	*
 *	be in the database (not generated).				*/
 
	HCI_PM( "Query product database for product codes" );	

	Query_data[1].field = RPGP_VOLT;
	Query_data[1].value = List_volume_time [1];
	Query_data[0].field = ORPGDBM_MODE;
	Query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH;

	products = ORPGDBM_query (Db_info,
				  Query_data,
				  2,
				  MAX_PRODUCTS_IN_LIST);
				  
	if (products == RMT_CANCELLED)
	   HCI_task_exit(HCI_EXIT_SUCCESS);
	   
	HCI_PM( "Attaching product attribute info to query results" );

	for (i=MIN_PRODUCT_CODE;i<=ORPGPAT_MAX_PRODUCT_CODE;i++) {

	    buf_num = ORPGPAT_get_prod_id_from_code (i);
	    if (buf_num < 0) {

		continue;

	    }

/*	    Check to see if product code is in the products list	*/

	    found = 0;

	    for (j=0;j<products;j++) {

		if (Db_info[j].prod_code == i) {

		    found = 1;
		    break;

		}
	    }

	    if (found == 1) {

		Num_products++;
		List_product [Num_products] = buf_num;

		sprintf (buf,"%s[%d] - %s",
			ORPGPAT_get_mnemonic (buf_num),
			i,
			ORPGPAT_get_description (buf_num, STRIP_MNEMONIC));
		str = XmStringCreateLocalized (buf);
		XmListAddItemUnselected (Product_list, str, 0);
		XmStringFree (str);

	    }
	}
	
	if (ORPGPAT_io_status() == RMT_CANCELLED)
	   HCI_task_exit(HCI_EXIT_SUCCESS);

	XtManageChild (Product);
	
	XtVaSetValues (Product,
		XmNselectedPosition,	0,
		NULL);

	XtAddCallback (Product,
		XmNselectionCallback, select_new_product_status, NULL);

	frame = XtVaCreateManagedWidget ("form",
		xmFrameWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		select_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("MNE[id]   Volume: Date   -    Time      Expire: Date   -   Time      Cut        ",
		xmLabelWidgetClass,	frame,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_BEGINNING,
		NULL);

	Data_scroll = XtVaCreateManagedWidget ("data_scroll",
		xmScrolledWindowWidgetClass,	form,
		XmNheight,		260,
		XmNscrollingPolicy,	XmAUTOMATIC,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("Select product(s) from Select Product list.\nSelect Volume(s) from the Search Date/Time list.",
		xmLabelWidgetClass,	form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_BEGINNING,
		NULL);

	XtVaSetValues (Data_scroll,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNbottomWidget,	label,
		NULL);

	XtManageChild (form);

	XtPopup (Top_widget, XtGrabNone);
	
	status = hci_display_product_status_data (Current_buf_num);
	if (status == RMT_CANCELLED)
	   HCI_task_exit(HCI_EXIT_SUCCESS);

	/* Start HCI loop. */

	HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This function is the timer procedure for the RPG	*
 *		     Products in Database task.  It updates the time	*
 *		     and product menus when the volume changes.		*
 *									*
 *	Input:  w - timer parent widget ID				*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
static	int	old_volume_number = 0;

	if (old_volume_number != ORPGVST_get_volume_number()) {

	    if (old_volume_number > 0) {

		HCI_PM( "Refreshing Product Status Information" );		
		hci_display_product_status_data ((int) Current_buf_num);
		hci_update_volume_time_list ();
		hci_update_product_list ();
	    }

	    old_volume_number = ORPGVST_get_volume_number();

	}

	if( Update_flag == HCI_YES_FLAG )
	{
	  Update_flag = HCI_NO_FLAG;
	  update_info();
	}
}

/************************************************************************
 *	Description: This function queries the RPG products database	*
 *		     for the specified product and displays all query	*
 *		     matches.						*
 *									*
 *	Input:  buf_num - product ID					*
 *	Output: NONE							*
 *	Return: -1 on error						*
 ************************************************************************/

int
hci_display_product_status_data (
int	buf_num
)
{
	int	i, j;
	int	month, day, year;
	int	hour, minute, second;
	long int	seconds;
	int	products;
	XmString	str;
	Widget	label = (Widget) NULL;
	char	buf1 [32];
	int	current_date;
	int	current_time;
	int	volume_scan_num;
	unsigned int volume_seq_num;
	int	current_seconds;
	int	prod_code;
	int	prod_id;
	int	cancel_status = 0;

/*	If the product status has been previously displayed, get rid	*
 *	of it and create a fresh display.				*/

	if (Form != (Widget) NULL) {

	    XtDestroyWidget (Form);

	}

	label = (Widget) NULL;

	prod_id = buf_num;

	Form = XtVaCreateWidget ("product_status_form",
		xmFormWidgetClass,	Data_scroll,
		XmNbackground,		hci_get_read_color (BLACK),
		XmNverticalSpacing,	1,
		NULL);

	XtVaSetValues (Data_scroll,
		XmNworkWindow,	Form,
		NULL);

	label = (Widget) NULL;

	M = 0;

/*	Get the current volume number and start time to use as a	*
 *	base for any filtering.						*/

	current_time    = ORPGVST_get_volume_time ();
	current_date    = ORPGVST_get_volume_date ()-1;

	if (current_date == 0) {

	    sprintf (buf,"     Current Volume [N/A]   Start: [N/A]");

	} else {

	    current_seconds = current_date*HCI_SECONDS_PER_DAY + current_time/HCI_MILLISECONDS_PER_SECOND;
	    seconds = current_seconds;

            volume_seq_num = ORPGVST_get_volume_number ();
            volume_scan_num = ORPGMISC_vol_scan_num (volume_seq_num);

	    unix_time (&seconds,
		&year,
		&month,
		&day,
		&hour,
		&minute,
		&second);

	    sprintf (buf,"     Current Volume %d (Seq: %d) Start: %3s %2d,%4d  %2.2d:%2.2d:%2.2d UT",
		volume_scan_num, (int) volume_seq_num,
		HCI_get_month(month), day, year,
		hour, minute, second);

	}

	str = XmStringCreateLocalized (buf);

	XtVaSetValues (Current_scan_info,
		XmNlabelString,	str,
		NULL);

	XmStringFree (str);
	
	HCI_PM( "Building query for selected product(s)" );

/*	Query the Products Data Base for the selected product.		*/

	j = 0;

	Query_data[j].field = ORPGDBM_MODE;
	Query_data[j++].value = ORPGDBM_EXACT_MATCH | ORPGDBM_HIGHEND_SEARCH | ORPGDBM_ALL_FIELD_VALUES;

	if (prod_id > 0) {

	    prod_code = ORPGPAT_get_code (prod_id);

	    if ((prod_code < MIN_PRODUCT_CODE) || (prod_code > ORPGPAT_MAX_PRODUCT_CODE)) {

		HCI_LE_error("Invalid product status request: (id: %d -- code: %d",
			prod_id, prod_code);
		return (-1);

	    }

	    Query_data [j].field = RPGP_PCODE;
	    Query_data [j++].value = prod_code;

	    if (Current_volume_time != 0) {

		Query_data [j].field = RPGP_VOLT;
		Query_data [j++].value = Current_volume_time;

	    }

	} else {

	    if (Current_volume_time != 0) {

		Query_data [j].field = RPGP_VOLT;
		Query_data [j++].value = Current_volume_time;

	    } else {

		return (-1);

	    }
	}
	cancel_status = ORPGPAT_io_status();
	if (cancel_status == RMT_CANCELLED)
	   return(cancel_status);

	HCI_PM( "Executing query for selected product(s)" );
	products = ORPGDBM_query (Db_info,
				  Query_data,
				  j,
				  MAX_PRODUCTS_IN_LIST);
				  
	if (products == RMT_CANCELLED)
	   return(products);

	for (i=0;i<products;i++) {

	    prod_id = ORPGPAT_get_prod_id_from_code (Db_info[i].prod_code);
	    
	    if ((prod_id <= 0) || (Db_info[i].prod_code <= 0)) {

		continue;

	    }

	    seconds = Db_info[i].vol_t;

	    unix_time ( &seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

/*	    The first thing we want to do is build the string for the	*
 *	    Volume date/time.						*/

	    sprintf (buf,"%3s[%2d]  [%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT]  ",
			ORPGPAT_get_mnemonic (prod_id), Db_info[i].prod_code,
			HCI_get_month(month), day, year,
			hour, minute, second);

/*	    Next we want to build the string for the expiration date	*
 *	    /time for the product and append it to the first string.	*/

	    seconds = Db_info[i].reten_t;

	    unix_time ( &seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

	    sprintf (buf1,"  [%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT]  ",
			HCI_get_month(month), day, year,
			hour, minute, second);

	    strcat (buf,buf1);

/*	    Next, if the product is elevation based, we want to find	*
 *	    all of the cuts which are available and build a string	*
 *	    which we can append to the previous string.			*/

	    if (ORPGPAT_elevation_based (prod_id) >= 0) {

		sprintf (buf1,"%4.1f       ",
			Db_info[i].elev/10.0);
		strcat (buf,buf1);

	    } else {

		sprintf (buf1,"N/A       ");
		strcat (buf,buf1);

	    }

	    sprintf (buf1,"  ");
	    strcat (buf,buf1);

	    str = XmStringCreateLocalized (buf);

	    if (label == (Widget) NULL) {

		label = XtVaCreateManagedWidget ("product",
			xmLabelWidgetClass,	Form,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNalignment,		XmALIGNMENT_BEGINNING,
			NULL);

	    } else {

		label = XtVaCreateManagedWidget ("product",
			xmLabelWidgetClass,	Form,
			XmNlabelString,		str,
			XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		label,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNfontList,		hci_get_fontlist (LIST),
			XmNalignment,		XmALIGNMENT_BEGINNING,
			NULL);

	    }

	    XtVaSetValues (label,
			XmNlabelString,		str,
			NULL);

	    XmStringFree (str);
	    
	}
	
        cancel_status = ORPGPAT_io_status();		
        if (cancel_status == RMT_CANCELLED)
		return(cancel_status);

	XtManageChild (Form);
	XtManageChild (Data_scroll);

	return (0);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a volume time from the time menu.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - combo box data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_new_volume_time (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmComboBoxCallbackStruct	*cbs =
		(XmComboBoxCallbackStruct *) call_data;

	Current_volume_time = List_volume_time [(int) cbs->item_position];

	update_button_callback ((Widget) NULL,
				(XtPointer) NULL,
				(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     a product from the product menu.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_new_product_status (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmComboBoxCallbackStruct	*cbs =
		(XmComboBoxCallbackStruct *) call_data;

	Current_buf_num = List_product [(int) cbs->item_position];

	update_button_callback ((Widget) NULL,
				(XtPointer) NULL,
				(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Update" button. 				*
 *		     NOTE: The "Update" button has been removed.	*
 *		     However, this function is called directly by	*
 *		     the select time and product callbacks.		*
 *									*
 *	Input:  w - widget ID (unused)					*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
update_button_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	Update_flag = HCI_YES_FLAG;
}

void update_info ()
{
	hci_display_product_status_data (Current_buf_num);
	hci_update_volume_time_list ();
	hci_update_product_list ();
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
close_product_status_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("Products in Database Close selected");
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *	Description: This function updates the volume time menu list.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: 0 on success						*
 ************************************************************************/

int
hci_update_volume_time_list ()
{
	int	i, j;
	int	hour, minute, second;
	int	month, day, year;
	long int	seconds;
	int	current_selection;
	int	products;
	int	found;
	int	item_count;
	XmString	str;

	current_selection = 0;

	XtVaGetValues (Volume_time_list,
		XmNitemCount,	&item_count,
		NULL);

/*	XmListDeleteItemsPos (Volume_time_list, item_count, 1);	*/
	XmListDeleteAllItems (Volume_time_list);

	Num_volume_times = 0;

	if (Current_buf_num > 0) {

	    str = XmStringCreateLocalized ("All Volume Times");
	    XmListAddItemUnselected (Volume_time_list, str, 0);
	    XmStringFree (str);

	    List_volume_time [Num_volume_times++] = 0;

	}

/*	Next, query the database for all unique volume times so we	*
 *	can build a list of the different times which are represented	*
 *	in the ORPG Products Database.					*/
 
	HCI_PM( "Querying product database for volume times" );		
	Query_data[1].field = RPGP_VOLT;
	Query_data[1].value = 0;
	Query_data[0].field = ORPGDBM_MODE;
	Query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			      ORPGDBM_DISTINCT_FIELD_VALUES;

	products = ORPGDBM_query (Db_info,
				  Query_data,
				  2,
				  MAX_PRODUCTS_IN_LIST);

	if (products == RMT_CANCELLED)
	   return(RMT_CANCELLED);
	   
	for (i=0;i<products;i++) {

	    found = 0;

	    for (j=0;j<i;j++) {

		if (Db_info[i].vol_t == Db_info[j].vol_t) {

		    found = 1;
		    break;

		}
	    }

/*	    If the time was not previously detected, add item to	*
 *	    combo list.							*/

	    if (found == 0) {

		List_volume_time [Num_volume_times++] = Db_info[i].vol_t;

		seconds = Db_info[i].vol_t;

		if (seconds == Current_volume_time) {

		    current_selection = Num_volume_times;

		}

		unix_time (&seconds,
			&year,
			&month,
			&day,
			&hour,
			&minute,
			&second);

		sprintf (buf,"%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
			HCI_get_month(month), day, year,
			hour, minute, second);
		str = XmStringCreateLocalized (buf);
		XmListAddItemUnselected (Volume_time_list, str, 0);
		XmStringFree (str);

	    }
	}
	
	XtManageChild (Volume_time);

	XmListSelectPos (Volume_time_list, current_selection, False);

	return(0);		
}

/************************************************************************
 *	Description: This function updates the product menu list.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: 0 on success						*
 ************************************************************************/

int
hci_update_product_list ()
{
	int	i, j;
	int	buf_num;
	int	current_selection;
	int	products;
	int	found;
	int	item_count;
	XmString	str;

	current_selection = 1;

	XtVaGetValues (Product_list,
		XmNitemCount,	&item_count,
		NULL);

/*	XmListDeleteItemsPos (Product_list, item_count, 1);	*/
	XmListDeleteAllItems (Product_list);

	Num_products     = 0;

	if (Current_volume_time > 0) {

	    str = XmStringCreateLocalized ("All Products");
	    XmListAddItemUnselected (Product_list, str, 0);
	    XmStringFree (str);

	    List_product [Num_products++] = 0;

	}

/*	Next, query the database for all unique product codes so we	*
 *	can build a list of the different products which are in the	*
 *	ORPG Products Database.  This is a better design than showing	*
 *	a list of all distributable products defined in the Product	*
 *	Attributes Table (PAT) since some of those products may not	*
 *	be in the database (not generated).				*/

	HCI_PM( "Querying product database for prod list" );		
	Query_data[1].field = RPGP_PCODE;
	Query_data[1].value = 0;
	Query_data[0].field = ORPGDBM_MODE;
	Query_data[0].value = ORPGDBM_FULL_SEARCH | ORPGDBM_HIGHEND_SEARCH |
			      ORPGDBM_DISTINCT_FIELD_VALUES;

	products = ORPGDBM_query (Db_info,
				  Query_data,
				  2,
				  MAX_PRODUCTS_IN_LIST);
				  
	if (products == RMT_CANCELLED)
	   return(RMT_CANCELLED);

	for (i=MIN_PRODUCT_CODE;i<=ORPGPAT_MAX_PRODUCT_CODE;i++) {

	    buf_num = ORPGPAT_get_prod_id_from_code (i);

	    if (buf_num < 0) {

		continue;

	    }

/*	    Check to see if product code is in the products list	*/

	    found = 0;

	    for (j=0;j<products;j++) {

		if (Db_info[j].prod_code == i) {

		    found = 1;
		    break;

		}
	    }

	    if (found == 1) {

		List_product [Num_products++] = buf_num;

		if (Current_buf_num == buf_num) {

		    current_selection = Num_products;

		}

		sprintf (buf,"%s[%d] - %s",
			ORPGPAT_get_mnemonic (buf_num),
			i,
			ORPGPAT_get_description (buf_num, STRIP_MNEMONIC));
		str = XmStringCreateLocalized (buf);
		XmListAddItemUnselected (Product_list, str, 0);
		XmStringFree (str);

	    }
	}
	
	if (ORPGPAT_io_status() == RMT_CANCELLED)
	   return(RMT_CANCELLED);

	XtManageChild (Product);

	XmListSelectPos (Product_list, current_selection, False);

	return(0);
}
