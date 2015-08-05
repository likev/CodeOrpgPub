/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:01 $
 * $Id: anim_opt.c,v 1.7 2008/03/13 22:45:01 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* anim_opt.c */
/* setting animation options */

#include "anim_opt.h"


/***************************/
/* file scope variables    */
int selected_loop_size;

/********************************************************************************/

/* pops up dialog to to set time animation options */
void time_option_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

if(XtParent(XtParent(w)) == dshell1)
    open_time_opt_dialog(1);

if(XtParent(XtParent(w)) == dshell2)
    open_time_opt_dialog(2);

if(XtParent(XtParent(w)) == dshell3)
    open_time_opt_dialog(3);


} /* end time_option_callback  */





/**********************************************************************/
void open_time_opt_dialog(int screen_N)
{
    Widget opt_shell=NULL;
    Widget d, animform;
    XmString oklabel, cnxlabel, helplabel;
    Arg al[20];
    Cardinal ac;
    static char *helpfile = HELP_FILE_ANIM_OPT;

    XmString labelstr;
    Widget  labelv, labelwidget1, hsep;
    Widget loop_opt, loop_menu, loopbut, loopbutnormal, labelwidget2;

    /* must be a constant to pass as client_data */
    static int screen;


    if(screen_N==1) {
         screen=SCREEN_1;
         opt_shell=dshell1;
    }
    if(screen_N==2) {
         screen=SCREEN_2;
         opt_shell=dshell2;
    }
    if(screen_N==3) {
         screen=SCREEN_3;
         opt_shell=dshell3;
    }

/*     selected_loop_size = 5; */
    selected_loop_size = 0;

    /* here we make the dialog and ensure it has the buttons we need */
    oklabel = XmStringCreateLtoR("OK", XmFONTLIST_DEFAULT_TAG);

    cnxlabel = XmStringCreateLtoR("Cancel", XmFONTLIST_DEFAULT_TAG);
    helplabel = XmStringCreateLtoR("Help", XmFONTLIST_DEFAULT_TAG);

    ac = 0;

    XtSetArg(al[ac], XmNokLabelString, oklabel);  ac++;	
    XtSetArg(al[ac], XmNcancelLabelString, cnxlabel);  ac++;
    XtSetArg(al[ac], XmNhelpLabelString, helplabel);  ac++;

    d = XmCreateTemplateDialog(opt_shell, "animd", al, ac);

    XtVaSetValues(XtParent(d), XmNtitle, "Time Series Options", NULL);

/*     XtAddCallback(d, XmNokCallback, time_ok_callback, NULL); */
    XtAddCallback(d, XmNokCallback, time_ok_callback, (XtPointer)(&screen));
    XtAddCallback(d, XmNcancelCallback, ts_pref_cancel_callback, NULL); 
    XtAddCallback(d, XmNhelpCallback, help_window_callback, helpfile);

    /* now, make a animform widget and add all the rest of the buttons */
    animform = XtVaCreateManagedWidget("animform", xmFormWidgetClass, d,
				       XmNresizable,  False,
				       NULL);





/* 	NEW RESET BASE VOLUME LABEL */


  labelv = XtVaCreateManagedWidget("Reset the Lower Volume Limit",
     xmLabelWidgetClass,  animform, 
	XmNtopAttachment,    XmATTACH_FORM,
	XmNtopOffset,        5,  
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftAttachment,   XmATTACH_FORM, 
     XmNleftOffset,       10,            
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_NONE,
     NULL);  


labelwidget1 = XtVaCreateManagedWidget("to the currently displayed product",
     xmLabelWidgetClass,  animform,
     XmNtopAttachment,    XmATTACH_WIDGET,             
     XmNtopWidget,        labelv,
     XmNtopOffset,        5,
     XmNleftAttachment,   XmATTACH_FORM,
     XmNleftOffset,       10,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_NONE,
     NULL);


 hsep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, animform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             5,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          labelwidget1,
       XmNtopOffset,          10,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              250,
       XmNheight,             3,
       NULL);

/* NEW LOOP BUTTON */

  labelstr = XmStringCreateLtoR("Loop Size of", XmFONTLIST_DEFAULT_TAG); 
  loop_opt = XmCreateOptionMenu(animform, "loop_opt", NULL, 0);
  loop_menu = XmCreatePulldownMenu(animform, "loop_menu", NULL, 0);

  loopbut = XtVaCreateManagedWidget("  2 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop2_callback, NULL);

  loopbut = XtVaCreateManagedWidget("  3 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop3_callback, NULL);

  loopbut = XtVaCreateManagedWidget("  4 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop4_callback, NULL);

/*   loopbutnormal = XtVaCreateManagedWidget("  5 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL); */
  loopbut = XtVaCreateManagedWidget("  5 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
/*   XtAddCallback(loopbutnormal, XmNactivateCallback, loop5_callback, NULL); */
  XtAddCallback(loopbut, XmNactivateCallback, loop5_callback, NULL);

  loopbut = XtVaCreateManagedWidget("  6 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop6_callback, NULL);

  loopbut = XtVaCreateManagedWidget("  8 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop8_callback, NULL);

  loopbut = XtVaCreateManagedWidget("10 Volumes  ", xmPushButtonWidgetClass, loop_menu, NULL);
  XtAddCallback(loopbut, XmNactivateCallback, loop10_callback, NULL);
  
/*   loopbut = XtVaCreateManagedWidget("Entire Buffer", xmPushButtonWidgetClass, loop_menu, NULL); */
  loopbutnormal = XtVaCreateManagedWidget("Entire Buffer", xmPushButtonWidgetClass, loop_menu, NULL);
/*   XtAddCallback(loopbut, XmNactivateCallback, loop_no_limit_callback, NULL); */
  XtAddCallback(loopbutnormal, XmNactivateCallback, loop_no_limit_callback, NULL);


  XtVaSetValues(loop_opt, 
		XmNsubMenuId,        loop_menu,
		XmNmenuHistory,      loopbutnormal,
		XmNlabelString,      labelstr,
		XmNtopAttachment,    XmATTACH_WIDGET,             
		XmNtopWidget,        hsep,
		XmNtopOffset,        5,
		XmNbottomAttachment, XmATTACH_NONE,
		XmNleftAttachment,   XmATTACH_FORM,
		XmNleftOffset,       10,
		XmNrightWidget,      XmATTACH_NONE,
		NULL);

  XtManageChild(loop_opt);

/* END LOOP BUTTON */

labelwidget2 = XtVaCreateManagedWidget("determines the Upper Volume Limit",
     xmLabelWidgetClass,  animform,
     XmNtopAttachment,    XmATTACH_WIDGET,             
     XmNtopWidget,        loop_opt,
     XmNtopOffset,        5,
     XmNleftAttachment,   XmATTACH_FORM,
     XmNleftOffset,       10,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_NONE,
     NULL);


    XtManageChild(d);


}   /* end open_time_opt_dialog */




/* callbacks */

/*****************************************************************************/


void time_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

int screen_num = *((int *)client_data);

    /*  if no other initialization type has been set, command FULL_INIT */
    /*  otherwise use existing init type */


/*  new logic */
    if( (screen_num==1) ||
        ( (dshell1 != NULL) && (linked_flag==TRUE) ) ) {
        user_reset_time_s1 = TRUE; 
        anim1.loop_size = selected_loop_size;
        if(anim1.reset_ts_flag==ANIM_NO_INIT)
            reset_time_series(1, ANIM_FULL_INIT);
        if(anim1.reset_ts_flag==ANIM_CHANGE_MODE_INIT)
            reset_time_series(1, ANIM_CHANGE_MODE_NEW_TS_INIT);
    } 
    
    if(screen_num==2 ||
        ( (dshell2 != NULL) && (linked_flag==TRUE) ) ) {
        user_reset_time_s2 = TRUE;
        anim2.loop_size = selected_loop_size;
        if(anim2.reset_ts_flag==ANIM_NO_INIT)
            reset_time_series(2, ANIM_FULL_INIT);
        if(anim2.reset_ts_flag==ANIM_CHANGE_MODE_INIT)
            reset_time_series(2, ANIM_CHANGE_MODE_NEW_TS_INIT);             
    } 
    /*  the auxiliary screen (screen3) is not linked */
    if(screen_num==3  ) {
        user_reset_time_s3 = TRUE;
        anim3.loop_size = selected_loop_size;
        if(anim3.reset_ts_flag==ANIM_NO_INIT)
            reset_time_series(3, ANIM_FULL_INIT);
        if(anim3.reset_ts_flag==ANIM_CHANGE_MODE_INIT)
            reset_time_series(3, ANIM_CHANGE_MODE_NEW_TS_INIT);
    } 

    
    XtUnmanageChild(w);
    
} /* end anim_ok_callback */


void ts_pref_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    XtUnmanageChild(w);  

}



void loop2_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 2;
}
void loop3_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 3;
}
void loop4_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 4;
}
void loop5_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 5;
}
void loop6_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 6;
}
void loop8_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 8;
}
void loop10_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 10;
}

void loop_no_limit_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
	selected_loop_size = 0;
}






/**************************************************************************/
/**************************************************************************/

/* pops up dialog to to set file animation options */
void file_option_callback(Widget w, XtPointer client_data, XtPointer call_data) {

	;

}
