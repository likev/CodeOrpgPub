
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/FileSB.h>
#include <Xm/MessageB.h>
#include <Xm/Separator.h>
#include <Xm/MainW.h>
#include <Xm/DrawingA.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <Xm/Label.h>
#include <Xm/DrawnB.h>
#include <Xm/PanedW.h>

#include "simulate.h"
#include "prod_user_msg.h"
#include "orpgumc.h"
#include <infr.h>
#include <orpgevt.h>
#include <lb.h>
#include <en.h>
#include <misc.h>

/* integer values used to distinguish the call to menuCB. */
#define MENU_CONNECTION     1
#define MENU_STATUS_MSG     2
#define MENU_ROUTINE        3
#define MENU_ONE_TIME       4
#define MENU_PRODUCT_LIST   5
#define MENU_BIAS_TABLE     6

#define MENU_SIGN_ON        8
#define MENU_F4             9
#define MENU_F2             10
#define MENU_MAX_CONN       11

#define MENU_ALERT          12
#define MENU_CLEAR	    13
#define MENU_EXIT           14

#define MENU_CONNECT	    15
#define MENU_DISCONNECT	    16

XtAppContext    context;

XmStringCharSet char_set = XmSTRING_DEFAULT_CHARSET;

/* all widgets are global to make life easier. */
static Widget toplevel, textshow, textenter, form, menu_bar;
static Widget close_option, quit_option, class1_one_time_option,
                weather_option, exit_option, bias_option;
static Widget   f1, f2, f3, f11, f12;

int Cur_link_ind;
int Little_endian;
int Src_id;
int Dest_id;
int No_connection;
int Use_comms_manager;
int Allow_ICD_send;

static char *In_lb = "";
static char *Out_lb = "";

static void Create_menus (Widget menu_bar);
static Widget Make_menu (char *menu_name, Widget menu_bar);
static Widget Make_menu_option (char *option_name, int client_data, 
							Widget menu);
static void menuCB (Widget w, int client_data, XmAnyCallbackStruct *call_data);
static void timer_callback (XtPointer client_data, XtIntervalId *timer_id);
static int Read_options (int argc, char **argv);

/* Timer callback called on time out. */
static void timer_callback (XtPointer client_data, XtIntervalId *timer_id)
{
    XtIntervalId g_timer_id;

    g_timer_id = XtAppAddTimeOut(context,
				 200,	/* interval */
				 (XtTimerCallbackProc) timer_callback,
				 (XtPointer) NULL);

    Md_search_input_LB();
}

static void menuCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
/* handles menu options. */
{

    switch (client_data) {
    case MENU_STATUS_MSG:

	Md_puprpgop_to_rpg_status_msg();
	break;


    case MENU_ROUTINE:

	Md_Routine_product();
	break;

    case MENU_ONE_TIME:

	Md_Class1_one_time();
	break;

    case MENU_PRODUCT_LIST:

	Md_product_list_req();
	break;

    case MENU_BIAS_TABLE:

	Md_bias_table_msg();
	break;

    case MENU_SIGN_ON:

	Md_Sign_on();
	break;

    case MENU_F2:

	Md_Class1_one_time();
	break;

    case MENU_MAX_CONN:
	Md_max_conn_dis();
	break;

    case MENU_F4:
	XmTextSetString(textenter, "");
	break;

    case MENU_CONNECT:
	Md_connect ();
	break;

    case MENU_DISCONNECT:
	Md_disconnect ();
	break;

    case MENU_ALERT:
	Md_alert();
	break;

    case MENU_CLEAR:
	XmTextSetString(textshow, " \0");

	break;

    case MENU_EXIT:
	printf ("pup_emu terminating\n");
	exit(0);
    }
}

static Widget Make_menu_option(char *option_name, int client_data, Widget menu)
{
    int             ac;
    Arg             al[10];
    Widget          b;

    ac = 0;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreateLtoR(option_name, char_set));
    ac++;
    b = XmCreatePushButton(menu, option_name, al, ac);
    XtManageChild(b);

    XtAddCallback(b, XmNactivateCallback, (XtCallbackProc)menuCB, 
					(XtPointer *)client_data);
    return (b);
}

static Widget Make_menu(char *menu_name, Widget menu_bar)
{
    int             ac;
    Arg             al[10];
    Widget          menu, cascade;

    ac = 0;
    menu = XmCreatePulldownMenu(menu_bar, menu_name, al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNsubMenuId, menu);
    ac++;
    XtSetArg(al[ac], XmNlabelString,
	     XmStringCreateLtoR(menu_name, char_set));
    ac++;
    cascade = XmCreateCascadeButton(menu_bar, menu_name, al, ac);
    XtManageChild(cascade);

    return (menu);
}

static void Create_menus (Widget menu_bar)
{
    Widget          menu;

    menu = Make_menu("CLASS I", menu_bar);
    /*
     * open_option = Make_menu_option("Connection",MENU_CONNECTION,menu);
     */
    close_option = Make_menu_option("PUP/RPGOP to RPG Status msg",
				    MENU_STATUS_MSG, menu);

    weather_option = Make_menu_option("Product list request",
				      MENU_PRODUCT_LIST, menu);
    quit_option = Make_menu_option("Routine Product request",
				   MENU_ROUTINE, menu);
    class1_one_time_option = Make_menu_option("One time product request",
					      MENU_ONE_TIME, menu);
    bias_option = Make_menu_option("Bias table message",
				   MENU_BIAS_TABLE, menu);



    menu = Make_menu("ClASS II", menu_bar);
    f1 = Make_menu_option("Sign-on", MENU_SIGN_ON, menu);
    f2 = Make_menu_option("One-time product", MENU_F2, menu);
    f3 = Make_menu_option("Max Conn. Time Disable request", MENU_MAX_CONN, menu);
    /*
     * f4 = Make_menu_option("Clear",MENU_F4,menu);
     */

    menu = Make_menu("OTHERS", menu_bar);
    f11 = Make_menu_option("connect", MENU_CONNECT, menu);
    f11 = Make_menu_option("disconnect", MENU_DISCONNECT, menu);
    f11 = Make_menu_option("Alert request", MENU_ALERT, menu);
    f12 = Make_menu_option("Clear Screen", MENU_CLEAR, menu);
    exit_option = Make_menu_option("Exit", MENU_EXIT, menu);

}

int main(int argc, char *argv[])
{
    Arg             al[20];
    int             ac;
    int             canvaswidth, canvasheight;

    /* read command line options */
    if (Read_options (argc, argv) != 0)
	exit (-1);

    /* create the toplevel shell */
    toplevel = XtAppInitialize(&context, "", NULL, 0,
			       &argc, argv, NULL, NULL, 0);

    CS_error((void (*) ()) printf);

    printf("We serve line %d\n", Cur_link_ind);

    SIM_init (In_lb, Out_lb);

    /* default window size. */
    ac = 0;
    canvaswidth = 400;
    canvasheight = 700;
    XtSetArg(al[ac], XmNheight, canvaswidth);
    ac++;
    XtSetArg(al[ac], XmNwidth, canvasheight);
    ac++;
    XtSetValues(toplevel, al, ac);

    /* create a form widget. */
    ac = 0;
    form = XmCreateForm(toplevel, "form", al, ac);
    XtManageChild(form);

    /* create a menu bar and attach it to the form. */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM);
    ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);
    ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM);
    ac++;
    menu_bar = XmCreateMenuBar(form, "menu_bar", al, ac);
    XtManageChild(menu_bar);

    /* create a textshow widget and attach it to the form. */
    ac = 0;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_WIDGET);
    ac++;
    XtSetArg(al[ac], XmNtopWidget, menu_bar);
    ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);
    ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_POSITION);
    ac++;
    XtSetArg(al[ac], XmNleftPosition, 0);
    ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_POSITION);
    ac++;
    XtSetArg(al[ac], XmNbottomPosition, 93);
    ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);
    ac++;
    textshow = XmCreateScrolledText(form, "text", al, ac);
    XtManageChild(textshow);
    XmTextSetEditable(textshow, False);

    Create_menus(menu_bar); 

    XtAppAddTimeOut(context,
		    950,
		    (XtTimerCallbackProc) timer_callback,
		    (XtPointer) NULL);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(context);

    return (0);
}

/***********************************************************************

    Description: This function writes a text string in the text display
		window.

***********************************************************************/

void Output_text (char *text)
{
    static int pos = 0;

    XmTextInsert (textshow, pos, text);
    pos += strlen (text);
    return;
}

/**************************************************************************

    Description: This function parses command line arguments.

    Input:	argc - number of command arguments
		argv - command line argument arrary

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    /* default values */
    Cur_link_ind = 0;
    Little_endian = 0;
    Src_id = 450;
    Dest_id = 3;
    No_connection = 0;
    Use_comms_manager = 0;
    Allow_ICD_send = 0;

    err = 0;
    while ((c = getopt (argc, argv, "s:d:I:O:anmlh?")) != EOF) {
	switch (c) {

	    case 'l':
		Little_endian = 1;
		break;
	    case 's':
		if (sscanf (optarg, "%d", &Src_id) < 1)
		    err = 1;
		break;
	    case 'd':
		if (sscanf (optarg, "%d", &Dest_id) < 1)
		    err = 1;
		break;
	    case 'I':
		In_lb = optarg;
		break;
	    case 'O':
		Out_lb = optarg;
		break;
	    case 'n':
		No_connection = 1;
		break;
	    case 'a':
		Allow_ICD_send = 1;
		break;

	    case 'm':
		Use_comms_manager = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;

	    default:
		LE_send_msg (0, "Unexpected option (%c) - %s\n", c);
		err = 1;
		break;
	}
    }

    if (optind == argc - 1) {       /* get the file name  */
	if (sscanf (argv[optind], "%d", &Cur_link_ind) != 1 ||
			Cur_link_ind < 0) {
	    LE_send_msg (0, 
			"line number is not specified - we use default 0\n");
	    Cur_link_ind = 0;
	}
    }

    if (strlen (In_lb) == 0 || strlen (Out_lb) == 0) {
	LE_send_msg (0, "Input/Outpur LB names must be specified\n");
	err = 1;
    }

    if (err == 1) { 			/* Print usage message */
	printf ("Usage: pup_emu (options) line_number\n");
	printf ("       Options:\n");
	printf ("       -l (p_server is on a different endian host and\n");
	printf ("           thus the comm headers are byte swapped)\n");
	printf ("       -s src_id (source ID; default = 450)\n");
	printf ("       -d dest_id (destination ID; default = 3)\n");
	printf ("          The line number is optional with default of 0\n");
	printf ("       -I in_lb (Input LB name; default = "")\n");
	printf ("       -O out_lb (Output LB name; default = "")\n");
	printf ("       -m (use_comms_manager)\n");
	printf ("       -n (no connection)\n");
	printf ("       -a (Allow sending ICD message regardless of state)\n");
	return (-1);
    }

    return (0);
}
