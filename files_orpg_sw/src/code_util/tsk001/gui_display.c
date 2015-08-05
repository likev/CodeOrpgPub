/* RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:37 $
 * $Id: gui_display.c,v 1.7 2009/05/15 17:52:37 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* gui_display.c */

#include "gui_display.h"
/* #include "click.c" */



/****************************************************************************/
/****************************************************************************/


/**************************************************************/
/* opens a window for product display, two screens allowed,
 * which screen is desired _must_ be specified correctly */
void open_display_screen(int screen_num)
{
  Widget pshell, winform, winstat, winstat2;

  Widget i_menubar, out1, out2, isep;
  Widget i_menu, menubut, s1, s2, s3, s0;

  Widget draw_widget, scroll_widget, vbar, hbar;
  Widget base_info, base_info_2, prod_descript;
  Widget legend_frame, legend_draw;
  Widget click_info_frame;

  
  Pixmap label_pix;
  Widget  sep, but1, but2, labelw, gbut, tbut; 
  Widget sep2, labelw2, but3, but4, stop_but, time_opt_but, file_opt_but;
  XPoint X[5], X2[5];    
  XmString  labelstr;
  
  Widget draw_popup;
  Widget radar_center_but, mouse_center_but, hide_ctr_loc_but;

  
  char *screen_ID;
  char *title;

  Pixmap pix, legend_pix;
  
  unsigned short attr_flag=0;


  static float zoom11factor = 1.0;
  
  static float zoom21factor = 2.0;
  static float zoom41factor = 4.0;
  static float zoom81factor = 8.0;
  static float zoom161factor = 16.0;
  static float zoom321factor = 32.0;
  
  static float zoom12factor = 0.5;
  static float zoom14factor = 0.25;
  static float zoom18factor = 0.125;
  static float zoom116factor = 0.0625;
  static float zoom132factor = 0.03125;
  
  /* CVG 9.0 */
  int leg_frame_hgt;
  

/* see poiner events in Young Xt pg 253 */
/* aee X.h for event definitions */    
    
    XtTranslations trans;
    static char draw_trans[] =  "<Btn1Down>: return_prod_info()";
    static XtActionsRec actions[] = {
      {"return_prod_info", (XtActionProc) click_info}
    };

/*  use extern int disp_width, disp_height;  size of display screen */
/*  to manage maximum graphic product screen size */
/*  and to control placement of the second display screen */

    
    /* check to make sure the screen's not already open */
    if(screen_num == SCREEN_1) {
        if(screen_1 != NULL)    /* if the widget number is non-zero, it exists */
            return;

        title = "Display Screen 1"; 
        screen_ID = "1";

    } else if(screen_num == SCREEN_2) {
        if(screen_2 != NULL)
            return;

        title = "Display Screen 2"; 
        screen_ID = "2";

    } else if(screen_num == SCREEN_3) {
        if(screen_3 != NULL)
            return;

        title = "Auxiliary Screen"; 
        screen_ID = "3";

    } else {
        if(verbose_flag)
            printf("Invalid screen specified: %d\n", screen_num);
        return;
    }


  
    /* reset animation  information */
    reset_elev_series(screen_num, ANIM_FULL_INIT);
    reset_time_series(screen_num, ANIM_FULL_INIT);
    reset_auto_update(screen_num, ANIM_FULL_INIT);



    /* create the new window, and populate it */
    pshell = XtVaCreatePopupShell(title, topLevelShellWidgetClass, shell,
/* the following is critical for initial sizing, but must */
/* reset to FALSE after shell is displayed */
    XmNallowShellResize,     TRUE,

/*  Better to allow the shell to be sized by the child widgets */
/*  than to preset size                                        */


    XmNmwmDecorations,    MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU^MWM_DECOR_RESIZEH,
    /*  limit available window functions */
    XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE^MWM_FUNC_RESIZE,

                  NULL);
                  
    XtAddCallback(pshell, XmNdestroyCallback, screenkill_callback, NULL);

    /* CVG 9.0 */
    if( large_screen_flag == FALSE)
        calculate_small_screen();
    else
        calculate_large_screen();
    /* prevent legend frame from being larger than legend pix */
    leg_frame_hgt = height + 75;
    if(leg_frame_hgt > sidebarheight)
        leg_frame_hgt = sidebarheight;
    
    /* CVG 9.0 */
    if( large_image_flag == FALSE)
        pwidth = pheight = img_size = SMALL_IMG;
    else
        pwidth = pheight = img_size = LARGE_IMG;



    winform = XtVaCreateManagedWidget("winform", xmFormWidgetClass, pshell,
                      XmNresizable,  True,
                      NULL);
 
    gbut = XtVaCreateManagedWidget("GAB | >", xmPushButtonWidgetClass, winform,
        XmNheight,            27,
        XmNwidth,             48,
        XmNtopAttachment,     XmATTACH_FORM,
        XmNtopOffset,         35,
        XmNleftAttachment,    XmATTACH_FORM,  
        XmNleftOffset,        2,        
        XmNbottomAttachment,  XmATTACH_NONE,
        XmNrightAttachment,   XmATTACH_NONE,
        NULL);
    XtAddCallback(gbut, XmNactivateCallback, gab_page_up_callback, 
                                                          (XtPointer) screen_ID);

    tbut = XtVaCreateManagedWidget("[ TAB ]", xmPushButtonWidgetClass, winform,
        XmNheight,            27,
        XmNwidth,             48,
        XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,      
        XmNtopWidget,        gbut,
        XmNtopOffset,        0,
        XmNbottomAttachment, XmATTACH_NONE,     
        XmNleftAttachment,   XmATTACH_WIDGET, 
        XmNleftWidget,       gbut,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE, 
        NULL);
     
    XtAddCallback(tbut, XmNactivateCallback, display_tab_callback, 
                                                         (XtPointer) screen_ID);


/*  cannot get menubar separated from the window title - why? */
/*  cannot resize menubar - why? */
  i_menubar = XmCreateMenuBar(winform, "menubar", NULL, 0); 
  
 
  XtVaSetValues(i_menubar,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNtopOffset,        0,
            XmNbottomAttachment, XmATTACH_NONE,     
            XmNleftAttachment,   XmATTACH_FORM,
            XmNleftOffset,        0, 
            XmNrightAttachment,  XmATTACH_NONE, 
            XmNheight,           25,
            XmNwidth,            48,
            NULL); 

/*  Attempting to set an attribute of the menu shell causes the */
/*  graphical image to not be centered when the screen opens    */
  XtVaSetValues(XtParent(i_menubar),
            XmNallowShellResize,     TRUE,
  /*             XmNheight,           25, */
  /*             XmNwidth,            50, */
            NULL); 


/*  parent must be a menubar not the form!!   */

  i_menu = XmCreatePulldownMenu(i_menubar, "screenmenu", NULL, 0);
  
      XtVaSetValues(i_menu,
            XmNheight,           25,
            XmNwidth,            48,
            NULL); 
            
  menubut = XtVaCreateManagedWidget("Screen",xmCascadeButtonWidgetClass,i_menubar,
                XmNsubMenuId, i_menu, NULL);
                
    XtVaSetValues(menubut,
            XmNheight,           25,
            XmNwidth,            48,
            NULL); 

  s0 = XtVaCreateManagedWidget("Center Image",xmPushButtonWidgetClass,i_menu,NULL);
  XtAddCallback(s0, XmNactivateCallback, center_image_callback, NULL);
  
  isep = XtVaCreateManagedWidget("isep0",xmSeparatorWidgetClass,i_menu,NULL); 
  
  s1 = XtVaCreateManagedWidget("Replot Image",xmPushButtonWidgetClass,i_menu,NULL); 
  XtAddCallback(s1, XmNactivateCallback, replot_image_callback, NULL);
  s3 = XtVaCreateManagedWidget("Clear Screen",xmPushButtonWidgetClass,i_menu,NULL); 
  XtAddCallback(s3, XmNactivateCallback, clear_screen_callback, NULL);

  isep = XtVaCreateManagedWidget("isep1",xmSeparatorWidgetClass,i_menu,NULL); 

  s2 = XtVaCreateManagedWidget("Open Compare Screen",xmPushButtonWidgetClass,
                                                                      i_menu,NULL); 
  XtAddCallback(s2, XmNactivateCallback, compare_screen_callback, NULL);

  isep = XtVaCreateManagedWidget("isep2",xmSeparatorWidgetClass,i_menu,NULL); 

  out1 = XtVaCreateManagedWidget("Output to GIF...",xmPushButtonWidgetClass,
                                                                     i_menu,NULL); 
  XtAddCallback(out1, XmNactivateCallback, gif_output_file_select_callback, NULL);
  out2 = XtVaCreateManagedWidget("Output to PNG...",xmPushButtonWidgetClass,
                                                                     i_menu,NULL);
  XtAddCallback(out2, XmNactivateCallback, png_output_file_select_callback, NULL);


 XtManageChild(i_menubar);

  /* CVG 9.0 - disabled due to bug, renable when fixed */
  XtSetSensitive(out1, False); 




  win_size_opt = XmCreateOptionMenu(winform, "win_size_opt", NULL, 0);
  win_size_menu = XmCreatePulldownMenu(winform, "win_size_menu", NULL, 0);

  sm_winbut = XtVaCreateManagedWidget(" Small ", xmPushButtonWidgetClass, 
                                                             win_size_menu, NULL);
  XtAddCallback(sm_winbut, XmNactivateCallback, sm_win_callback, 
                                                           (XtPointer) screen_ID);

  lg_winbut = XtVaCreateManagedWidget(" Large ", xmPushButtonWidgetClass, 
                                                             win_size_menu, NULL);
  XtAddCallback(lg_winbut, XmNactivateCallback, lg_win_callback, 
                                                           (XtPointer) screen_ID);

  XtVaSetValues(win_size_opt, 
        XmNsubMenuId,        win_size_menu,
        XmNmenuHistory,      sm_winbut,     
        XmNtopAttachment,    XmATTACH_FORM, 
        XmNtopOffset,        0,
        XmNbottomAttachment, XmATTACH_NONE, 
        XmNleftAttachment,   XmATTACH_WIDGET, 
        XmNleftWidget,        i_menubar,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);


 
  /* CVG 9.0 - use size flag rather than the size in pixels */
  if( large_screen_flag == FALSE)
    XtVaSetValues(win_size_opt,
           XmNmenuHistory,      sm_winbut,
           NULL);
    
  else
    XtVaSetValues(win_size_opt,
           XmNmenuHistory,      lg_winbut,
           NULL);
           
           
      /*  label not used - permits closer placement to attached objects */
      XtUnmanageChild(XmOptionLabelGadget(win_size_opt));

  XtManageChild(win_size_opt);



    winstat = XtVaCreateManagedWidget("", xmLabelWidgetClass, winform,

     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         32,          
     XmNleftAttachment,    XmATTACH_WIDGET,
     XmNleftWidget,        tbut,
     XmNleftOffset,        0,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_NONE,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);


   click_info_frame = XtVaCreateManagedWidget ("clickinfoframe",
     xmFrameWidgetClass,    winform,
     XmNwidth,             198,
     XmNheight,            58,     
     XmNshadowType,        XmSHADOW_ETCHED_IN,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         1,
     XmNleftAttachment,    XmATTACH_FORM,  
     XmNleftOffset,        141,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_NONE,
     NULL);


    winstat2 = XtVaCreateManagedWidget("", xmLabelWidgetClass, click_info_frame,

     XmNwidth,             198,
     XmNheight,            58,     
     XmNrecomputeSize,      False, 
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);


    base_info_2 = XtVaCreateManagedWidget("", xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_NONE,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_FORM,
     XmNrightOffset,       2,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);
    
    base_info = XtVaCreateManagedWidget("", xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_NONE,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_WIDGET,
     XmNrightWidget,       base_info_2,
     XmNrightOffset,       0,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);



   prod_descript = XtVaCreateManagedWidget(
      "[---   ---  Product Description Label  ---   ---]", 
      xmLabelWidgetClass, winform,
      XmNtopAttachment,     XmATTACH_WIDGET,
      XmNtopWidget,         click_info_frame,
      XmNtopOffset,         0,
      XmNleftAttachment,    XmATTACH_FORM,
      XmNleftOffset,        120,
      XmNbottomAttachment,  XmATTACH_NONE,
      XmNbottomOffset,      2,
      XmNrightAttachment,   XmATTACH_NONE,
      XmNrightOffset,       2,
      XmNalignment,         XmALIGNMENT_BEGINNING,
      NULL);   


 
   legend_frame = XtVaCreateManagedWidget ("legendframe",
     xmFrameWidgetClass,    winform,
     XmNwidth,                       sidebarwidth,
     XmNheight,                      leg_frame_hgt,
     XmNshadowType,                  XmSHADOW_IN,
     XmNtopAttachment,               XmATTACH_WIDGET,
     XmNtopWidget,                   click_info_frame,
     XmNtopOffset,                   19,
     XmNleftAttachment,              XmATTACH_NONE,
     XmNbottomAttachment,            XmATTACH_NONE,
     XmNrightAttachment,             XmATTACH_FORM,
     XmNrightOffset,                 5,
     NULL);
   
    legend_draw = XtVaCreateWidget("drawing_area",
     xmDrawingAreaWidgetClass, legend_frame,
     XmNwidth,                 sidebarwidth,
     XmNheight,                height+75,
     NULL);

    XtManageChild(legend_draw);
    XtAddCallback(legend_draw, XmNexposeCallback, legend_expose_callback, NULL);
    
  scroll_widget = XtVaCreateManagedWidget ("scrolledwindow",
     xmScrolledWindowWidgetClass,    winform,
     XmNscrollingPolicy,             XmAUTOMATIC,
     XmNwidth,                       width,
     XmNheight,                      height,
     XmNtopAttachment,               XmATTACH_OPPOSITE_WIDGET,
     XmNtopWidget,                   legend_frame,
     XmNtopOffset,                   0,
     XmNleftAttachment,              XmATTACH_FORM,
     XmNleftOffset,                  2,
/*  this spacing can cause conflicts with the heght of the lower control widgets */
     XmNbottomAttachment,            XmATTACH_FORM,
     XmNbottomOffset,                81,     
     XmNrightAttachment,             XmATTACH_WIDGET,
     XmNrightWidget,                 legend_frame,
     NULL);

    XtVaGetValues(scroll_widget, XmNverticalScrollBar, &vbar, NULL);
    XtAddCallback(vbar, XmNvalueChangedCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNdecrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNincrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNpageDecrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNpageIncrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNtoBottomCallback, vert_scroll_callback,  NULL);
    XtAddCallback(vbar, XmNtoTopCallback, vert_scroll_callback,  NULL);
    XtVaSetValues(vbar,
          XmNminimum,       0,
          XmNmaximum,       pheight,
          NULL);

    XtVaGetValues(scroll_widget, XmNhorizontalScrollBar, &hbar, NULL);
    XtAddCallback(hbar, XmNvalueChangedCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNdecrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNincrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNpageDecrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNpageIncrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNtoBottomCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(hbar, XmNtoTopCallback, horiz_scroll_callback,  NULL);
    XtVaSetValues(hbar,
          XmNminimum,       0,
          XmNmaximum,       pwidth+barwidth,
          NULL);



    draw_widget = XtVaCreateWidget("drawing_area",
        xmDrawingAreaWidgetClass, scroll_widget,
        XmNwidth,                 pwidth+barwidth, 
        XmNheight,                pheight, 
        XmNresizePolicy,          XmRESIZE_ANY,
        NULL);
        


/*  should callbacks be before or after manage widget???? */
    XtManageChild(draw_widget);
    
    /* POP-UP MENU FOR OFF-CENTER IMAGE */
    XtAddCallback(draw_widget, XmNpopupHandlerCallback, draw_menu_callback, 
              NULL);    
    XtAddCallback(draw_widget, XmNexposeCallback, expose_callback, NULL);



/*  METHOD 1 */
/* p_label = XmStringCreateLtoR("Image Center Selection", XmFONTLIST_DEFAULT_TAG);*/
/* center = XmStringCreateLtoR("Center at Radar", XmFONTLIST_DEFAULT_TAG); */
/* mouse = XmStringCreateLtoR("Center at Mouse Position", XmFONTLIST_DEFAULT_TAG);*/
/*  */
/* draw_popup = XmVaCreateSimplePopupMenu( draw_widget, "popup_menu", */
/*                                         img_center_menu_cb, */
/*                         XmNpopupEnabled,   XmPOPUP_AUTOMATIC, */
/*                         XmVaTITLE, p_label, */
/*                         XmVaDOUBLE_SEPARATOR, */
/*                         XmVaPUSHBUTTON, center, 'R', NULL, NULL, */
/*                         XmVaPUSHBUTTON, mouse, 'M', NULL, NULL, */
/*                         NULL); */
/* XmStringFree(p_label); */
/* XmStringFree(center); */
/* XmStringFree(mouse); */
/*  END METHOD 1 */
/*  METHOD 2 */
    draw_popup = XmCreatePopupMenu( draw_widget, "Image Center Selection", 
                                                                   NULL, 0);
    XtCreateManagedWidget( "Image Center Selection", xmLabelWidgetClass, 
                                                       draw_popup, NULL, 0);
    XtCreateManagedWidget( "Separator", xmSeparatorWidgetClass, draw_popup, 
                                                                   NULL, 0);
    radar_center_but = XtCreateManagedWidget( "Center at Radar", 
                            xmPushButtonWidgetClass, draw_popup,   NULL, 0);
    mouse_center_but = XtCreateManagedWidget( "Center at Mouse Position", 
                            xmPushButtonWidgetClass, draw_popup,   NULL, 0); 
    XtCreateManagedWidget( "Separator", xmSeparatorWidgetClass, draw_popup, 
                                                                   NULL, 0);
    hide_ctr_loc_but = XtCreateManagedWidget( "Set Dynamically", 
                            xmPushButtonWidgetClass, draw_popup,   NULL, 0);
    XtAddCallback ( radar_center_but, XmNactivateCallback, img_center_menu_cb, 
                                          (XtPointer) 0);
    XtAddCallback ( mouse_center_but, XmNactivateCallback, img_center_menu_cb, 
                                          (XtPointer) 1);
    XtAddCallback ( hide_ctr_loc_but, XmNactivateCallback, img_center_menu_cb, 
                                          (XtPointer) 2);
    XtVaSetValues( draw_popup, XmNpopupEnabled,   XmPOPUP_AUTOMATIC, NULL);
    hide_xmstr = XmStringCreateLtoR("Hide Center Location Icon", 
                                                        XmFONTLIST_DEFAULT_TAG);
    show_xmstr = XmStringCreateLtoR("Show Center Location Icon", 
                                                        XmFONTLIST_DEFAULT_TAG);
    if(ctr_loc_icon_visible_flag == TRUE)
        XtVaSetValues( hide_ctr_loc_but, XmNlabelString, hide_xmstr, NULL);
    else
        XtVaSetValues( hide_ctr_loc_but, XmNlabelString, show_xmstr, NULL);
/*  END METHOD 2 */


    /*  for the button 1 data click     */
    XtAppAddActions(app, actions, XtNumber(actions));
    trans = XtParseTranslationTable(draw_trans);
    XtOverrideTranslations(draw_widget, trans);




/* According to some texts, shells are not managed or realized! */
/* However without realizing the shell, run-time errors with */
/* creation of pixmap occur */
    XtRealizeWidget(pshell);  


    /* set up the offscreen pixmap for this window */
    pix = XCreatePixmap(XtDisplay(draw_widget), XtWindow(draw_widget), 
            pwidth+barwidth, pheight, XDefaultDepthOfScreen(XtScreen(pshell)));
    XSetForeground(XtDisplay(draw_widget), gc, black_color);
    XFillRectangle(XtDisplay(draw_widget), pix, gc, 0, 0, pwidth+barwidth, pheight);
    XCopyArea(XtDisplay(draw_widget), pix, XtWindow(draw_widget), gc, 0, 0, 
          pwidth+barwidth, pheight, 0, 0);


    /* and one for the extra legend */
    legend_pix = XCreatePixmap(XtDisplay(legend_draw), XtWindow(legend_draw), 
                   sidebarwidth, sidebarheight, 
                   XDefaultDepthOfScreen(XtScreen(pshell)) );
    XSetForeground(XtDisplay(legend_draw), gc, black_color);
    XFillRectangle(XtDisplay(legend_draw), legend_pix, gc, 0, 0, 
           sidebarwidth, sidebarheight);
    XCopyArea(XtDisplay(legend_draw), legend_pix, XtWindow(legend_draw), gc, 0, 0, 
                                                sidebarwidth, sidebarheight, 0, 0);




    /* centering the scrollbars on the image */

    center_screen(vbar, hbar);



/* BEGIN GUI ELEMENTS BELOW SCREEN */

/* -------- begin new overlay label option menu ---------- */

      ovly_label_opt = XmCreateOptionMenu(winform, "ovly_label_op", NULL, 0);
      ovly_label_menu = XmCreatePulldownMenu(winform, "ovly_label_menu", NULL, 0);
    
      label_clearbut = XtVaCreateManagedWidget("Transp Lbl", xmPushButtonWidgetClass, 
                                                ovly_label_menu, NULL);
      XtAddCallback(label_clearbut, XmNactivateCallback, norm_format_Callback, 
                      (XtPointer) screen_ID);
    
      label_blackbut = XtVaCreateManagedWidget("Black Lbl", xmPushButtonWidgetClass, 
                                                ovly_label_menu, NULL);
      XtAddCallback(label_blackbut, XmNactivateCallback, bkgd_format_Callback, 
                      (XtPointer) screen_ID);
    
      XtVaSetValues(ovly_label_opt, 
            XmNsubMenuId,        ovly_label_menu,
            XmNtopAttachment,    XmATTACH_WIDGET,
            XmNtopWidget,        scroll_widget,
            XmNtopOffset,        46,
            XmNbottomAttachment, XmATTACH_NONE,
            XmNleftAttachment,   XmATTACH_FORM, 
            XmNleftOffset,       107,
            XmNrightWidget,      XmATTACH_NONE,
            NULL);
      /*  label not used - permits closer placement to attached objects */
      XtUnmanageChild(XmOptionLabelGadget(ovly_label_opt));
    
      XtManageChild(ovly_label_opt);


/* --------  end new overlay label option menu  ---------- */



/* -------- begin new display attribute option menu ----------- */

  labelstr = XmStringCreateLtoR("Display Attributes", XmFONTLIST_DEFAULT_TAG);
  disp_opt = XmCreateOptionMenu(winform, "disp_opt", NULL, 0);
  disp_menu = XmCreatePulldownMenu(winform, "disp_menu", NULL, 0);

  dispbut0 = XtVaCreateManagedWidget(" NONE ", xmPushButtonWidgetClass, 
             disp_menu, NULL);
  XtAddCallback(dispbut0, XmNactivateCallback, disp_none_callback, 
                                                         (XtPointer) screen_ID);

  dsep = XtVaCreateManagedWidget("dsep1",xmSeparatorWidgetClass,disp_menu,NULL); 
  
  dispbut1 = XtVaCreateManagedWidget("Range", xmPushButtonWidgetClass, 
            disp_menu, NULL);
  XtAddCallback(dispbut1, XmNactivateCallback, disp_r_callback, 
                                                         (XtPointer) screen_ID);
  
  dispbut2 = XtVaCreateManagedWidget("Azimuth", xmPushButtonWidgetClass, 
             disp_menu, NULL);
  XtAddCallback(dispbut2, XmNactivateCallback, disp_a_callback, 
                                                           (XtPointer) screen_ID);
  
  dispbut3 = XtVaCreateManagedWidget("Rng & Az", xmPushButtonWidgetClass, 
             disp_menu, NULL);
  XtAddCallback(dispbut3, XmNactivateCallback, disp_ra_callback, 
                                                           (XtPointer) screen_ID);
  
  dispbut4 = XtVaCreateManagedWidget(" Map ", xmPushButtonWidgetClass, 
            disp_menu, NULL);
  XtAddCallback(dispbut4, XmNactivateCallback, disp_m_callback, 
                                                           (XtPointer) screen_ID);

  dispbut5 = XtVaCreateManagedWidget("Map & Rng", xmPushButtonWidgetClass, 
            disp_menu, NULL);
  XtAddCallback(dispbut5, XmNactivateCallback, disp_mr_callback, 
                                                           (XtPointer) screen_ID);
    
  dispbut6 = XtVaCreateManagedWidget("Map & Az", xmPushButtonWidgetClass, 
            disp_menu, NULL);
  XtAddCallback(dispbut6, XmNactivateCallback, disp_ma_callback, 
                                                           (XtPointer) screen_ID);

  dsep = XtVaCreateManagedWidget("dsep2",xmSeparatorWidgetClass,disp_menu,NULL); 
  
  dispbut7 = XtVaCreateManagedWidget(" ALL ", xmPushButtonWidgetClass, 
            disp_menu, NULL);
  XtAddCallback(dispbut7, XmNactivateCallback, disp_all_callback, 
                                                           (XtPointer) screen_ID);  

  XtVaSetValues(disp_opt, 
        XmNsubMenuId,        disp_menu,
        XmNmenuHistory,      dispbut4,
        XmNlabelString,      labelstr,
        XmNorientation,      XmVERTICAL,
        XmNspacing,          0,
        XmNtopAttachment,    XmATTACH_WIDGET,       
        XmNtopWidget,        scroll_widget,
        XmNtopOffset,        0,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM, 
        XmNleftOffset,       107,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);

  XtManageChild(disp_opt);

  XmStringFree(labelstr);


/* set by preferences or previous screen opening */
  if(screen_num == SCREEN_1) { 
     attr_flag = ( range_ring_flag1 | (az_line_flag1 << 1) | (map_flag1 << 2) ); 
  } else if(screen_num == SCREEN_2) {  
     attr_flag = ( range_ring_flag2 | (az_line_flag2 << 1) | (map_flag2 << 2) );
  } else if(screen_num == SCREEN_3) {  
     attr_flag = ( range_ring_flag3 | (az_line_flag3 << 1) | (map_flag3 << 2) );
  }


  switch (attr_flag) {
  case 0:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut0, NULL);
     break;
  case 1:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut1, NULL);
     break;
  case 2:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut2, NULL);
     break;
  case 3:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut3, NULL);
     break;
  case 4:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut4, NULL);
     break;
  case 5:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut5, NULL);
     break;
  case 6:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut6, NULL);
     break;
  case 7:
     XtVaSetValues(disp_opt, XmNmenuHistory, dispbut7, NULL);    
  }

/* --------  end new display attribute option menu  ----------- */





/* must correspond to the XmNset resource */
  if(screen_num == SCREEN_1) {
    norm_format1 = TRUE;
    bkgd_format1 = FALSE;
  } else if(screen_num == SCREEN_2) {
    norm_format2 = TRUE;
    bkgd_format2 = FALSE;
  } else if(screen_num == SCREEN_3) {
    norm_format3 = TRUE;
    bkgd_format3 = FALSE;
  }


  zoom_opt = XmCreateOptionMenu(winform, "zoom_opt", NULL, 0);
  zoom_menu = XmCreatePulldownMenu(winform, "zoom_menu", NULL, 0);

  zoombut = XtVaCreateManagedWidget("32:1 Zoom", xmPushButtonWidgetClass, 
                                                               zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom321factor);

  zoombut = XtVaCreateManagedWidget("16:1 Zoom", xmPushButtonWidgetClass, 
                                                               zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom161factor);


  
  zoombut = XtVaCreateManagedWidget("8:1 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom81factor);
  
  zoombut = XtVaCreateManagedWidget("4:1 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom41factor);
  
  zoombut = XtVaCreateManagedWidget("2:1 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom21factor);

  zsep = XtVaCreateManagedWidget("zsep1",xmSeparatorWidgetClass,zoom_menu,NULL);
  
  zoombutnormal = XtVaCreateManagedWidget("1:1 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombutnormal, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom11factor);

  zsep = XtVaCreateManagedWidget("zsep2",xmSeparatorWidgetClass,
                                                               zoom_menu,NULL);
  
  zoombut = XtVaCreateManagedWidget("1:2 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom12factor);
  
  zoombut = XtVaCreateManagedWidget("1:4 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom14factor);
  
  zoombut = XtVaCreateManagedWidget("1:8 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                    (XtPointer) &zoom18factor);



  zoombut = XtVaCreateManagedWidget("1:16 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                   (XtPointer) &zoom116factor);

  zoombut = XtVaCreateManagedWidget("1:32 Zoom", xmPushButtonWidgetClass, 
                                                              zoom_menu, NULL);
  XtAddCallback(zoombut, XmNactivateCallback, zoom_callback, 
                                                   (XtPointer) &zoom132factor);

  XtVaSetValues(zoom_opt, 
        XmNsubMenuId,        zoom_menu,
        XmNmenuHistory,      zoombutnormal,     
        XmNtopAttachment,    XmATTACH_WIDGET,       
        XmNtopWidget,        scroll_widget,
        XmNtopOffset,        46,
        XmNbottomAttachment, XmATTACH_NONE, 
        XmNleftAttachment,   XmATTACH_FORM, 
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
        
      /*  label not used - permits closer placement to attached objects */
      XtUnmanageChild(XmOptionLabelGadget(zoom_opt));

  XtManageChild(zoom_opt);


/* IMAGE SIZE MENU HERE */

/* since the button reflects the settings for both screens, the menu selection */
/* of the other window must be set by the program. Easy with different wigit ids. */

      labelstr = XmStringCreateLtoR("Image Control", XmFONTLIST_DEFAULT_TAG); 
      img_size_opt = XmCreateOptionMenu(winform, "img_size_opt", NULL, 0);
      img_size_menu = XmCreatePulldownMenu(winform, "img_size_menu", NULL, 0);
    
      img_sizebutnormal = XtVaCreateManagedWidget("Large Image", 
                                     xmPushButtonWidgetClass, img_size_menu, NULL);
      XtAddCallback(img_sizebutnormal, XmNactivateCallback, img_large_callback, 
                                                            (XtPointer) screen_ID);
    
      img_sizebut = XtVaCreateManagedWidget("Small Image", xmPushButtonWidgetClass, 
                                                               img_size_menu, NULL);
      XtAddCallback(img_sizebut, XmNactivateCallback, img_small_callback, 
                                                            (XtPointer) screen_ID);
    
      XtVaSetValues(img_size_opt, 
            XmNsubMenuId,        img_size_menu,
            XmNlabelString,      labelstr,
            XmNorientation,      XmVERTICAL,
            XmNspacing,          0,
            XmNtopAttachment,    XmATTACH_WIDGET,
            XmNtopWidget,        scroll_widget,
            XmNtopOffset,        0,
            XmNbottomAttachment, XmATTACH_NONE,
            XmNleftAttachment,   XmATTACH_FORM, 
            XmNleftOffset,       0,
            XmNrightWidget,      XmATTACH_NONE,
            NULL);
    
      XtManageChild(img_size_opt);

      XmStringFree(labelstr);

    /* CVG 9.0 - used global constant */
    if(img_size==LARGE_IMG)
        XtVaSetValues(img_size_opt, 
            XmNmenuHistory,      img_sizebutnormal, 
            NULL);
    /* CVG 9.0 - changed 760 to 768 (1.5 times the PUP screen */
    else if(img_size==SMALL_IMG)
        XtVaSetValues(img_size_opt, 
            XmNmenuHistory,      img_sizebut, 
            NULL);

/* END IMAGE SIZE MENU */


 
  /* animation LABELS */

  labelw2 = XtVaCreateManagedWidget("Animation Options",
    xmLabelWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_WIDGET,
     XmNtopWidget,        scroll_widget, 
     XmNtopOffset,        3,       
     XmNleftAttachment,   XmATTACH_FORM,
     XmNleftOffset,       230,        
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNwidth,            115,
     NULL);

  
  labelw = XtVaCreateManagedWidget("Animation Control",
     xmLabelWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_WIDGET,
     XmNtopWidget,        scroll_widget,   
     XmNtopOffset,        3,    
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       labelw2,
     XmNleftOffset,       25,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
     XmNbottomWidget,     labelw2,
     XmNwidth,            120,
     NULL);



  /*    ANIM OPTION BUTTONS  */


  time_opt_but = XtVaCreateManagedWidget("Set Vol",
     xmPushButtonWidgetClass,winform,
     XmNtopAttachment,    XmATTACH_WIDGET,
     XmNtopWidget,        labelw2,
     XmNtopOffset,        1,
     XmNleftAttachment,   XmATTACH_FORM,
     XmNleftOffset,       230,    
     XmNbottomAttachment, XmATTACH_NONE,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNwidth,            55,
     XmNheight,           25,
     NULL);
  XtAddCallback(time_opt_but, XmNactivateCallback, time_option_callback, 
                                                         (XtPointer) screen_ID);


  file_opt_but = XtVaCreateManagedWidget("Set File",
     xmPushButtonWidgetClass,winform,
     XmNtopAttachment,    XmATTACH_WIDGET,
     XmNtopWidget,        labelw2,
     XmNtopOffset,        1,
     XmNleftAttachment,   XmATTACH_WIDGET,     
     XmNleftWidget,       time_opt_but,
     XmNleftOffset,       2,
     XmNbottomAttachment, XmATTACH_NONE,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNwidth,            55,
     XmNheight,           25,
     NULL);
  XtAddCallback(file_opt_but, XmNactivateCallback, file_option_callback, 
                                                          (XtPointer) screen_ID);  

  /* gray out unimplemented options */
  XtSetSensitive(file_opt_but, False);

  /* END ANIM OPTION BUTTONS */


  /* ANIM CONTROL BUTTONS */

/* ----- begin new animation selection menu button ----- */

  anim_opt = XmCreateOptionMenu(winform, "anim_opt", NULL, 0);
    
  anim_menu = XmCreatePulldownMenu(winform, "anim_menu", NULL, 0);

  XtManageChild ( anim_opt);

/*  label not used - permits closer placement to attached objects */
      XtUnmanageChild( XmOptionLabelGadget(anim_opt) );
  
/* does not work here, size of opntion is larger than necessary   */
  XtVaSetValues(anim_opt,
/*  attempting to set subMenuID here causes core dump, WHY???? */
/*       XmNsubMenuId,        anim_menu, */
             XmNresizeHeight,             False,
             XmNresizeWidth,              False,
             XmNwidth,                       120,
             XmNheight,                      20,
             NULL);

  XtManageChild ( anim_opt);

/*  may be a solution to resizing the option menu   */
  XtVaSetValues( XmOptionButtonGadget(anim_opt),
          XmNheight,        20,
          XmNwidth,         120,
          NULL );


  animbut1 = XtVaCreateManagedWidget("Volume", 
             xmPushButtonWidgetClass, anim_menu,
             XmNrecomputeSize,            False, 
             XmNheight,                      20,             
             NULL);
  XtAddCallback(animbut1, XmNactivateCallback, select_time_callback, 
                (XtPointer) screen_ID);
  
  animbut2 = XtVaCreateManagedWidget("Elevation", 
             xmPushButtonWidgetClass, anim_menu,
             XmNrecomputeSize,            False, 
             XmNheight,                      20,             
             NULL);
  XtAddCallback(animbut2, XmNactivateCallback, select_elev_callback, 
                (XtPointer) screen_ID);
  
  animbut3 = XtVaCreateManagedWidget("Most Recent", 
             xmPushButtonWidgetClass, anim_menu,
             XmNrecomputeSize,            False, 
             XmNheight,                      20,             
             NULL);
  XtAddCallback(animbut3, XmNactivateCallback, select_update_callback, 
                (XtPointer) screen_ID);

  asep = XtVaCreateManagedWidget("asep1",xmSeparatorWidgetClass,anim_menu,NULL);
  
  animbut4 = XtVaCreateManagedWidget("File Series", 
             xmPushButtonWidgetClass, anim_menu,
             XmNrecomputeSize,            False, 
             XmNheight,                      20,             
             NULL);
  XtAddCallback(animbut4, XmNactivateCallback, select_file_callback, 
                (XtPointer) screen_ID);
 
  XtSetSensitive(animbut4, False);
  
  XtVaSetValues(anim_opt, 
        XmNsubMenuId,        anim_menu,
        XmNmenuHistory,      animbut1,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        scroll_widget,
        XmNtopOffset,        45,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       235,    
        XmNbottomAttachment, XmATTACH_NONE,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);



/* -----  end new animation selection menu button  ----- */ 


  but1 = XtVaCreateManagedWidget("< |",
     xmPushButtonWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_WIDGET, 
     XmNtopWidget,        labelw,
     XmNtopOffset,        5,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       labelw2,  
     XmNleftOffset,       20,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_NONE,
     XmNwidth,            animb_width,
     XmNheight,           animb_height+2,
     XmNmarginBottom,                  2,
     XmNrecomputeSize,             False,
     NULL);
  XtAddCallback(but1, XmNactivateCallback, back_one_callback, (XtPointer)screen_ID);



    linked_toggle = XtVaCreateManagedWidget("Linked", 
       xmToggleButtonWidgetClass, winform, 
       XmNtopAttachment,    XmATTACH_WIDGET, 
       XmNtopWidget,        labelw,  
       XmNtopOffset,        3,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          but1,
       XmNleftOffset,          5,       
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       NULL);


    if(linked_flag == TRUE)
        XtVaSetValues(linked_toggle, XmNset, True, NULL);

    XtAddCallback(linked_toggle, XmNvalueChangedCallback, 
                     linked_callback, (XtPointer) screen_ID);

    /* the Auxiliary screen cannot be linked at this time! */    
    if(screen_num == SCREEN_3) {
        XtVaSetValues(linked_toggle, XmNset, False, NULL);
        XtSetSensitive(linked_toggle, False);
    }
        


  
  /* first, a backwards play arrow button... */
  but2 = XtVaCreateManagedWidget("<",
     xmPushButtonWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_WIDGET, 
     XmNtopWidget,        but1,
     XmNtopOffset,        2,
     XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
     XmNleftWidget,       but1,  
     XmNleftOffset,       0,     
     XmNrightAttachment,   XmATTACH_OPPOSITE_WIDGET,
     XmNrightWidget,       but1,  
     XmNrightOffset,       0,     
     XmNbottomAttachment, XmATTACH_NONE,
     XmNwidth,            animb_width,
     XmNheight,           animb_height,
     NULL);
  label_pix = XCreatePixmap(XtDisplay(but2), XtWindow(but2), 25, 15, 
                XDefaultDepthOfScreen(XtScreen(shell)));
  /* set up drawing correctly */
  XSetForeground(display,gc,black_color);
  XFillRectangle(display,label_pix,gc,0,0,25,15);
  XSetForeground(display, gc, green_color);
  XSetBackground(display, gc, black_color);
  XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
  X[0].x = 18;
  X[0].y = 2;
  X[1].x = 6;
  X[1].y = 7;
  X[2].x = 18;
  X[2].y = 12;
  X[3].x = 18;
  X[3].y = 2;
  XFillPolygon(display, label_pix, gc, X, 4, Convex, CoordModeOrigin);
  XtVaSetValues(but2, XmNlabelType, XmPIXMAP, XmNlabelPixmap, label_pix, NULL);
  XtAddCallback(but2, XmNactivateCallback, back_play_callback, (XtPointer)screen_ID);

  /* then a stop button */
  stop_but = XtVaCreateManagedWidget("O",
     xmPushButtonWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET, 
     XmNtopWidget,        but2,
     XmNleftAttachment,      XmATTACH_WIDGET,
     XmNleftWidget,          but2,
     XmNleftOffset,          2,
     XmNrightAttachment,     XmATTACH_NONE,     
     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
     XmNbottomWidget,     but2,
     XmNwidth,            62,
     XmNheight,           animb_height,
     NULL);
  label_pix = XCreatePixmap(XtDisplay(stop_but), XtWindow(stop_but), 54, 22, 
                XDefaultDepthOfScreen(XtScreen(shell)));
  /* set up drawing correctly */
  XSetForeground(display,gc,grey_color);
  XFillRectangle(display,label_pix,gc,0,0,54,22);
  XSetForeground(display,gc,black_color);
  XFillRectangle(display,label_pix,gc,1,2,51,16);
  XSetForeground(display, gc, red_color);
  XSetBackground(display, gc, black_color);
  XFillRectangle(display, label_pix, gc, 21, 4, 10, 10);
  XtVaSetValues(stop_but, XmNlabelType, XmPIXMAP, XmNlabelPixmap, label_pix, NULL);
  XtAddCallback(stop_but, XmNactivateCallback, stop_anim_callback, 
                                                            (XtPointer) screen_ID);



  but3 = XtVaCreateManagedWidget("| >",
     xmPushButtonWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_WIDGET, 
     XmNtopWidget,        labelw,
     XmNtopOffset,        5,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       stop_but,
     XmNleftOffset,       2 ,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
     XmNbottomWidget,     but1,
     XmNwidth,            animb_width,
     XmNheight,           animb_height+2,
     XmNmarginBottom,                  2,
     XmNrecomputeSize,             False,
     NULL);
  XtAddCallback(but3, XmNactivateCallback, fwd_one_callback, (XtPointer)screen_ID);



  /* and finally a play foward button */
  but4 = XtVaCreateManagedWidget(">",
     xmPushButtonWidgetClass,  winform,   
     XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET, 
     XmNtopWidget,        stop_but,
     XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
     XmNleftWidget,       but3,  
     XmNleftOffset,       0,        
     XmNrightAttachment,   XmATTACH_OPPOSITE_WIDGET,
     XmNrightWidget,       but3,  
     XmNrightOffset,       0,      
     XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
     XmNbottomWidget,     stop_but,
     XmNwidth,            animb_width,
     XmNheight,           animb_height,
     NULL);
  label_pix = XCreatePixmap(XtDisplay(but4), XtWindow(but4), 25, 15, 
                XDefaultDepthOfScreen(XtScreen(shell)));

  /* set up drawing correctly */
  XSetForeground(display,gc,black_color);
  XFillRectangle(display,label_pix,gc,0,0,25,15);
  XSetForeground(display, gc, green_color);
  XSetBackground(display, gc, black_color);
  XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);
  X2[0].x = 6;
  X2[0].y = 2;
  X2[1].x = 18;
  X2[1].y = 7;
  X2[2].x = 6;
  X2[2].y = 12;
  X2[3].x = 6;
  X2[3].y = 2;
  XFillPolygon(display, label_pix, gc, X2, 3, Convex, CoordModeOrigin);
  XtVaSetValues(but4, XmNlabelType, XmPIXMAP, XmNlabelPixmap, label_pix, NULL);
  XtAddCallback(but4, XmNactivateCallback, fwd_play_callback, (XtPointer)screen_ID);


  sep2 = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, winform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNorientation,        XmVERTICAL,
       XmNmargin,             5,
       XmNtopAttachment,      XmATTACH_OPPOSITE_WIDGET,
       XmNtopWidget,          labelw,
       XmNtopOffset,          7,
       XmNleftAttachment,     XmATTACH_WIDGET,
       XmNleftWidget,         labelw2,
       XmNleftOffset,         8,
       XmNrightAttachment,    XmATTACH_NONE,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              3,
       XmNheight,             65,
       NULL);

  /* END ANIM CONTROL BUTTONS */


  /* moved to after radiao box for Linux */
  /* Do to a bug in Linux involving the initial resize of the radio box class */
  /* widget, it does not completely realize if it is the last widget created */
  sep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, winform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNorientation,        XmVERTICAL,
       XmNmargin,             5,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          scroll_widget,
       XmNtopOffset,          2,       
       XmNleftAttachment,     XmATTACH_FORM,
       XmNleftOffset,         220,       
       XmNrightAttachment,    XmATTACH_NONE,
       XmNbottomAttachment,   XmATTACH_FORM,
       XmNbottomOffset,       5,
       XmNwidth,              2,
       NULL);


    if(screen_num == SCREEN_1) {
      
      XtVaSetValues(pshell,
         XmNtitle,    " Graphic Product Display Screen 1 ("CVG_SHORT_VER")", 
         XmNx,         screen1x,
         XmNy,         screen1y,     
         NULL);
         
    } else if(screen_num == SCREEN_2) {
      
        XtVaSetValues(pshell,
           XmNtitle,    " Graphic Product Display Screen 2 ("CVG_SHORT_VER")", 
           XmNx,         screen2x,     
           XmNy,         screen2y,     
           NULL);
    
    } else if(screen_num == SCREEN_3) {
      
        XtVaSetValues(pshell,
           XmNtitle,    " Graphic Product Auxiliary Screen ("CVG_SHORT_VER")", 
           XmNx,         screen3x,     
           XmNy,         screen3y,     
           NULL);
    
    }


/* DEBUG */
/* fprintf(stderr,"DEBUG SCREEN %d X IS %d, \n", screen_num, screenx); */

    
    /* make the new window the main one */
    /* also set up a bunch of uglyish globals -- consider it a quick reference
     * sheet of the important ones 
     */
    if(screen_num == SCREEN_1) {
        screen_1 = draw_widget;
        legend_screen_1 = legend_draw;
        legend_frame_1 = legend_frame;
        dshell1 = pshell;
        vbar1 = vbar;
        hbar1 = hbar;
        port1 = scroll_widget;
        scroll1_xpos = scroll1_ypos = 0;
        sd1->pixmap = pix;
        sd1->legend_pixmap = legend_pix;
        sd1->data_display = winstat;
        sd1->data_display_2 = winstat2;
        sd1->gab_page = 1;
        sd1->tab_window = NULL;
        sd1->base_info = base_info;
        sd1->base_info_2 = base_info_2;

        strcpy(sd1->rad_id, "KMLB"); /*  FUTURE ENHANCEMENT, MAKE A PREF */
        sd1->prev_lat = 28.113;
        sd1->prev_lon = -80.654;
        
        sd1->prod_descript  = prod_descript;
        img_size_opt1 = img_size_opt;
        img_size_menu1 = img_size_menu;
        img_sizebut1 = img_sizebut;
        img_sizebutnormal1 = img_sizebutnormal;
        linked_toggle1 = linked_toggle;

        radar_center1 = radar_center_but;
        mouse_center1 = mouse_center_but;
        hide_ctr_loc_but1 = hide_ctr_loc_but;
        zoom_opt1 = zoom_opt;
        zoombutnormal1 = zoombutnormal;

    } else if(screen_num == SCREEN_2) {
        screen_2 = draw_widget;
        legend_screen_2 = legend_draw;
        legend_frame_2 = legend_frame;
        dshell2 = pshell;
        vbar2 = vbar;
        hbar2 = hbar;
        port2 = scroll_widget;
        scroll2_xpos = scroll2_ypos = 0;
        sd2->pixmap = pix;
        sd2->legend_pixmap = legend_pix;
        sd2->data_display = winstat;
        sd2->data_display_2 = winstat2;
        sd2->gab_page = 1;
        sd2->tab_window = NULL;
        sd2->base_info = base_info;
        sd2->base_info_2 = base_info_2;

        strcpy(sd2->rad_id, "KMLB"); /*  FUTURE ENHANCEMENT, MAKE A PREF */
        sd2->prev_lat = 28.113;
        sd2->prev_lon = -80.654;
        
        sd2->prod_descript  = prod_descript;    
        img_size_opt2 = img_size_opt;
        img_size_menu2 = img_size_menu;
        img_sizebut2 = img_sizebut;
        img_sizebutnormal2 = img_sizebutnormal;
        linked_toggle2 = linked_toggle;

        radar_center2 = radar_center_but;
        mouse_center2 = mouse_center_but;
        hide_ctr_loc_but2 = hide_ctr_loc_but;
        zoom_opt2 = zoom_opt;
        zoombutnormal2 = zoombutnormal;

    } else if(screen_num == SCREEN_3) {
        screen_3 = draw_widget;
        legend_screen_3 = legend_draw;
        legend_frame_3 = legend_frame;
        dshell3 = pshell;
        vbar3 = vbar;
        hbar3 = hbar;
        port3 = scroll_widget;
/*         scroll3_xpos = scroll3_ypos = 0; */
        sd3->pixmap = pix;
        sd3->legend_pixmap = legend_pix;
        sd3->data_display = winstat;
        sd3->data_display_2 = winstat2;
        sd3->gab_page = 1;
        sd3->tab_window = NULL;
        sd3->base_info = base_info;
        sd3->base_info_2 = base_info_2;

        strcpy(sd3->rad_id, "KMLB"); /*  FUTURE ENHANCEMENT, MAKE A PREF */
        sd3->prev_lat = 28.113;
        sd3->prev_lon = -80.654;
        
        sd3->prod_descript  = prod_descript;    
        img_size_opt3 = img_size_opt;
        img_size_menu3 = img_size_menu;
        img_sizebut3 = img_sizebut;
        img_sizebutnormal3 = img_sizebutnormal;
        linked_toggle3 = linked_toggle;

        radar_center3 = radar_center_but;
        mouse_center3 = mouse_center_but;
        hide_ctr_loc_but3 = hide_ctr_loc_but;
        zoom_opt3 = zoom_opt;
        zoombutnormal3 = zoombutnormal;
    }



/*  TRY MOVING TO THE END AFTER ALL OTHER WIDGETS ARE DEFINED! */
    XtPopup(pshell, XtGrabNone);

/* END GUI ELEMENTS FROM MAIN SCREEN */

  /* must be set to false or all display/redisplay/click actions */
  /* will resize the shell */
  XtVaSetValues(pshell, 
                XmNallowShellResize,     FALSE,
                NULL);

  /* reset the colors */
  XSetForeground(display, gc, white_color);
  XSetBackground(display, gc, black_color);


}  /* end open_display_screen */





/****************************************************************************/
/****************************************************************************/





/* called whenever one of the screens is destroyed */
void screenkill_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    int rv;
    
    if(compare_shell) {
        XtDestroyWidget(compare_shell);
        compare_draw=NULL;
        compare_shell=NULL;
        compare_data_display = NULL;
        compare_data_display_2 = NULL;
        compare_base_info_2 = NULL;
    }



/*********************************************************/
    if(w == dshell1) {
        
    /***closing second window after zoom problem***/
    clear_pixmap(SCREEN_1);
    legend_clear_pixmap(SCREEN_1);
    
        /* reset animation  information */
    reset_elev_series(SCREEN_1, ANIM_FULL_INIT);
    reset_time_series(SCREEN_1, ANIM_FULL_INIT);
    reset_auto_update(SCREEN_1, ANIM_FULL_INIT);
        
    /*  NEED A DESTROY WIDGET HERE */
    /* close TAB window if open */
    if(sd1->tab_window != NULL) {
        XtDestroyWidget(sd1->tab_window);
    }

    XFreePixmap(XtDisplay(screen_1),sd1->pixmap);       
        screen_1 = NULL;
    dshell1 = NULL;

    clear_history(&(sd1->history), &(sd1->history_size));
    
    if(sd1->icd_product != NULL)
        free(sd1->icd_product);

/*  GENERIC_PROD */
    if((sd1->generic_prod_data != NULL)) {
        rv = cvg_RPGP_product_free((void *)sd1->generic_prod_data);
    }   
    
    if(sd1->layers != NULL)
        delete_layer_info(sd1->layers, sd1->num_layers);
               
    /***closing second window after zoom problem***/        
    sd = sd1;
    
        /* delete any extra info lying around */
    if(sd1->last_image == RASTER_IMAGE)
        delete_raster();
    else if(sd1->last_image == RLE_IMAGE)
        delete_radial_rle();
    else if(sd1->last_image == DIGITAL_IMAGE)
        delete_packet_16();
    else if(sd1->last_image == PRECIP_ARRAY_IMAGE)
        delete_packet_17();
    else if(sd1->last_image == GENERIC_RADIAL)
        delete_generic_radial();


    /* --- Resetting default screen data values (assumes memory freed) --- */
    
    setup_default_screen_data_values(sd1);
    
    /* --- Resetting the screen radio button and selected_screen --- */

    /* if the other primary screen is open, select it */
    if(screen_2 != NULL) {
        XtVaSetValues(s1_radio, XmNset, False, NULL);
        XtVaSetValues(s2_radio, XmNset, True, NULL);
            XtSetSensitive(s1_radio, True); 
            XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_2;
        
        } else if(screen_3 != NULL) {
        XtVaSetValues(s1_radio, XmNset, False, NULL);
        XtVaSetValues(s3_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_3;
    }
       
    /* if there is no other screen, return to default settings */
    if(screen_2 == NULL && screen_3 == NULL) {
        XtVaSetValues(s3_radio, XmNset, False, NULL);
        XtVaSetValues(s2_radio, XmNset, False, NULL);
        XtVaSetValues(s1_radio, XmNset, True, NULL);        
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, False);
        XtSetSensitive(s3_radio, True);

        selected_screen = SCREEN_1;
    }

    
    
/*********************************************************/    
    } else if(w == dshell2) {
        
    /***closing second window after zoom problem***/
    clear_pixmap(SCREEN_2);
    legend_clear_pixmap(SCREEN_2);

        /* reset animation information */
        reset_elev_series(SCREEN_2, ANIM_FULL_INIT);
        reset_time_series(SCREEN_2, ANIM_FULL_INIT);
        reset_auto_update(SCREEN_2, ANIM_FULL_INIT);

    /*  NEED A DESTROY WIDGET HERE? */
    /* close TAB window if open */
    if(sd2->tab_window != NULL) {
        XtDestroyWidget(sd2->tab_window);
    }

    XFreePixmap(XtDisplay(screen_2),sd2->pixmap); 
        screen_2 = NULL;
    dshell2 = NULL;

    clear_history(&(sd2->history), &(sd2->history_size));

    if(sd2->icd_product != NULL)
            free(sd2->icd_product);

/*  GENERIC_PROD */
    if((sd2->generic_prod_data != NULL)) {
        rv = cvg_RPGP_product_free((void *)sd2->generic_prod_data);
    }   
        
    if(sd2->layers != NULL)
        delete_layer_info(sd2->layers, sd2->num_layers);
    
    /***closing second window after zoom problem***/        
    sd = sd2;
    
    /* delete any extra info lying around */
    if(sd2->last_image == RASTER_IMAGE)
        delete_raster();
    else if(sd2->last_image == RLE_IMAGE)
        delete_radial_rle();
    else if(sd2->last_image == DIGITAL_IMAGE)
        delete_packet_16();
    else if(sd2->last_image == PRECIP_ARRAY_IMAGE)
        delete_packet_17();
    else if(sd2->last_image == GENERIC_RADIAL)
        delete_generic_radial();


    /* --- Resetting default screen data values (assumes memory freed) --- */
    
    setup_default_screen_data_values(sd2);

    /* --- Resetting the screen radio button and selected_screen --- */

    /* if the other primary screen is open, select it */
    if(screen_1 != NULL) {
        XtVaSetValues(s2_radio, XmNset, False, NULL);
        XtVaSetValues(s1_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_1;
        
    } else if(screen_3 != NULL) {
        XtVaSetValues(s2_radio, XmNset, False, NULL);
        XtVaSetValues(s3_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_3; 
    }    
       
    /* if there is no other screen, return to default settings */
    if(screen_1 == NULL && screen_3 == NULL) {
        XtVaSetValues(s3_radio, XmNset, False, NULL);
        XtVaSetValues(s2_radio, XmNset, False, NULL);
        XtVaSetValues(s1_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, False); 
        XtSetSensitive(s3_radio, True);
        selected_screen = SCREEN_1;
    }
    
 
/*********************************************************/    
  } else if(w == dshell3) {
        
    /***closing second window after zoom problem***/
    clear_pixmap(SCREEN_3);
    legend_clear_pixmap(SCREEN_3);

        /* reset animation information */
        reset_elev_series(SCREEN_3, ANIM_FULL_INIT);
        reset_time_series(SCREEN_3, ANIM_FULL_INIT);
        reset_auto_update(SCREEN_3, ANIM_FULL_INIT);

    /*  NEED A DESTROY WIDGET HERE? */
    /* close TAB window if open */
    if(sd3->tab_window != NULL) {
        XtDestroyWidget(sd3->tab_window);
    }

    XFreePixmap(XtDisplay(screen_3),sd3->pixmap); 
        screen_3 = NULL;
    dshell3 = NULL;

    clear_history(&(sd3->history), &(sd3->history_size));

    if(sd3->icd_product != NULL)
            free(sd3->icd_product);

    /*  GENERIC_PROD */
    if((sd3->generic_prod_data != NULL)) {
        rv = cvg_RPGP_product_free((void *)sd3->generic_prod_data);
    }   
        
    if(sd3->layers != NULL)
        delete_layer_info(sd3->layers, sd3->num_layers);
    
    /***closing second window after zoom problem***/        
    sd = sd2;
    
    /* delete any extra info lying around */
    if(sd3->last_image == RASTER_IMAGE)
        delete_raster();
    else if(sd3->last_image == RLE_IMAGE)
        delete_radial_rle();
    else if(sd3->last_image == DIGITAL_IMAGE)
        delete_packet_16();
    else if(sd3->last_image == PRECIP_ARRAY_IMAGE)
        delete_packet_17();
    else if(sd3->last_image == GENERIC_RADIAL)
        delete_generic_radial();

    /* --- Resetting default screen data values (assumes memory freed) --- */
    
    setup_default_screen_data_values(sd3);

    /* --- Resetting the screen radio button and selected_screen --- */

    /* if the other primary screen is open, select it */
    if(screen_1 != NULL) {
        XtVaSetValues(s3_radio, XmNset, False, NULL);
        XtVaSetValues(s1_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_1;
        
    } else if(screen_2 != NULL) {
        XtVaSetValues(s3_radio, XmNset, False, NULL);
        XtVaSetValues(s2_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, True); 
        selected_screen = SCREEN_2; 
    } 

    /* if there is no other screen, return to default settings */
    if(screen_1 == NULL && screen_2 == NULL) {
        XtVaSetValues(s3_radio, XmNset, False, NULL);
        XtVaSetValues(s2_radio, XmNset, False, NULL);
        XtVaSetValues(s1_radio, XmNset, True, NULL);
        XtSetSensitive(s1_radio, True); 
        XtSetSensitive(s2_radio, False); 
        XtSetSensitive(s3_radio, True);
        selected_screen = SCREEN_1;
    }
    
  }  /*  end dshell3 */

/*********************************************************/       
    


} /*  end screenkill_callback */








void setup_default_screen_data_values(screen_data *the_sd)
{
  the_sd->icd_product = NULL;
  the_sd->generic_prod_data = NULL;
  the_sd->packet_28_offset = 0;
  
  the_sd->history = NULL;
  the_sd->history_size = 0;
  
  the_sd->lb_filename = NULL;
  
  the_sd->pixmap = (Pixmap)NULL;
  the_sd->legend_pixmap = (Pixmap)NULL;
  
  the_sd->this_image = NO_IMAGE;
  the_sd->last_image = NO_IMAGE;
  
  the_sd->tab_window = NULL;
  the_sd->gab_displayed = FALSE;
  the_sd->gab_page = 0;
  
  the_sd->resolution = 0;
  the_sd->scale_factor = 1.0;

  the_sd->x_center_offset = 0;   
  the_sd->y_center_offset = 0;   

  the_sd->base_info = NULL;
  the_sd->base_info_2 = NULL;
  the_sd->data_display = NULL;
  the_sd->data_display_2 = NULL;
  the_sd->prod_descript = NULL;

  the_sd->last_plotted = FALSE;  
        
  the_sd->layers = NULL;
  the_sd->num_layers = 0;
  the_sd->packet_select_type = SELECT_NONE;
  the_sd->packet_select = 0;
  the_sd->layer_select = 0;

  the_sd->raster = NULL;
  the_sd->radial = NULL;
  the_sd->dhr = NULL;
  the_sd->digital_precip = NULL;
  
}








void reset_screen_size(int screen_num)
{

Widget horizbar=NULL, vertbar=NULL;


  if(screen_num == SCREEN_1) { 

    XFreePixmap(XtDisplay(screen_1),sd1->pixmap); 
    
    sd1->pixmap = XCreatePixmap(XtDisplay(screen_1), XtWindow(screen_1), 
                                   pwidth + barwidth, pheight, 
                   XDefaultDepthOfScreen(XtScreen(dshell1)));
    XSetForeground(XtDisplay(screen_1), gc, black_color);
    XFillRectangle(XtDisplay(screen_1), sd1->pixmap, gc, 0, 0, 
                   pwidth + barwidth, pheight);     
    XtVaSetValues(screen_1,
         XmNwidth,       pwidth+barwidth,
         XmNheight,      pheight,
         NULL);

    horizbar = hbar1;
    vertbar = vbar1;
    
  } else if(screen_num == SCREEN_2) {  

    XFreePixmap(XtDisplay(screen_2),sd2->pixmap); 
    
    sd2->pixmap = XCreatePixmap(XtDisplay(screen_2), XtWindow(screen_2), 
                                   pwidth + barwidth, pheight, 
                   XDefaultDepthOfScreen(XtScreen(dshell2)));
    XSetForeground(XtDisplay(screen_2), gc, black_color);
    XFillRectangle(XtDisplay(screen_2), sd2->pixmap, gc, 0, 0, 
                   pwidth + barwidth, pheight);     
    XtVaSetValues(screen_2,
         XmNwidth,       pwidth+barwidth,
         XmNheight,      pheight,
         NULL);
         
    horizbar = hbar2;
    vertbar = vbar2;
    
  } else if(screen_num == SCREEN_3) {  

    XFreePixmap(XtDisplay(screen_3),sd3->pixmap); 
    
    sd3->pixmap = XCreatePixmap(XtDisplay(screen_3), XtWindow(screen_3), 
                                   pwidth + barwidth, pheight, 
                   XDefaultDepthOfScreen(XtScreen(dshell3)));
    XSetForeground(XtDisplay(screen_3), gc, black_color);
    XFillRectangle(XtDisplay(screen_3), sd3->pixmap, gc, 0, 0, 
                   pwidth + barwidth, pheight);     
    XtVaSetValues(screen_3,
         XmNwidth,       pwidth+barwidth,
         XmNheight,      pheight,
         NULL);
         
    horizbar = hbar3;
    vertbar = vbar3;
    
  }

/*  recenter the display canvas (the drawing area) */

    center_screen(vertbar, horizbar);




/* /////////////////////////////////////////////////////////////////////// */
/*  now do the same for the compare screen */


/*  the auxiliary screen (screen_3) is not related to the compare screen */

    if( (screen_num != SCREEN_3) && compare_shell) {
    

        XtVaSetValues(compare_draw,
            XmNwidth,       pwidth+barwidth,
            XmNheight,      pheight,
            NULL);


       center_screen(compare_vbar, compare_hbar);

    
    
    } /*  end if compare_shell */


}









/* centering the scrollbars on the image */
/* centers the image canvas (drawing area) */
void center_screen(Widget vertical_bar, Widget horizontal_bar)
{
    
int hsize, vsize;
int hincr, vincr, hpagei, vpagei;

    
   XtVaGetValues(vertical_bar,
        XmNsliderSize,    &vsize,
        XmNincrement,     &vincr,
        XmNpageIncrement, &vpagei,
        NULL);    
    
    XtVaGetValues(horizontal_bar,
        XmNsliderSize,    &hsize,
        XmNincrement,     &hincr,
        XmNpageIncrement, &hpagei,
        NULL);    

    XmScrollBarSetValues(vertical_bar, 
/*  LINUX ISSUE - when called in conjunction with moving from a large window    */
/*                to a small window, the read vsize is still close to the large */
/*                value for some unknown reason; therefor use hsize instead.    */
/*                          (pheight/2)-(vsize/2), */
                         (pheight/2)-(hsize/2),   /*  value */
                         vsize,                   /*  slider size   */
                         vincr,                   /*  increment      */
                         vpagei,                  /*  page increment */
                         True);

    XmScrollBarSetValues(horizontal_bar, 
                         (pwidth/2)-(hsize/2),    /*  value          */
                         hsize,                   /*  slider size   */
                         hincr,                   /*  increment      */
                         hpagei,                  /*  page increment */
                         True);

    
}









/******************************************************/
/* action for the on-screen canvas that reports information about the 
 * specific bin of data, if a binned product has been plotted 
 */ 
static void click_info(Widget w, XButtonEvent *event, String *params, 
                                                    Cardinal *num_params)
{
  if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell1) {
      do_click_info(SCREEN_1, event->x, event->y);
      
      if(screen_2 != NULL) /*  ALWAYS LINKED DATA CLICK */
      do_click_info(SCREEN_2, event->x, event->y);
      
      if(compare_draw != NULL)  {
          update_compare_click_info();
      }  
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell2) {
      do_click_info(SCREEN_2, event->x, event->y);
      
      if(screen_1 != NULL)  /*  ALWAYS LINKED DATA CLICK */
      do_click_info(SCREEN_1, event->x, event->y);
      
      if(compare_draw != NULL)  {
          update_compare_click_info();
      }   
      
  } else if(XtParent(XtParent(XtParent(XtParent(w)))) == dshell3) {
    
      do_click_info(SCREEN_3, event->x, event->y);
      
      /*  the auxuiliary screen (screen3 is not related to other  */
      /*  screens, including the compare screen */
      
  }
  
  
  
}







/*#ifdef SUNOS*/
static void compare_click_info(Widget w, XButtonEvent *event, String *params, 
                                                        Cardinal *num_params)
{
   do_click_info(SCREEN_1, event->x, event->y);
   do_click_info(SCREEN_2, event->x, event->y);
   update_compare_click_info();
}
/*#endif*/








/*  the auxiliary screen (screen3) is not related to the compare screen */
void update_compare_click_info() {

  XmString info_xmstr, info2_xmstr; 

    if(compare_num==1) { 
        XtVaGetValues(sd1->data_display, XmNlabelString,  &info_xmstr, NULL);
        XtVaGetValues(sd1->data_display_2, XmNlabelString,  &info2_xmstr, NULL);
    } 
    else { 
        XtVaGetValues(sd2->data_display, XmNlabelString,  &info_xmstr, NULL);
        XtVaGetValues(sd2->data_display_2, XmNlabelString,  &info2_xmstr, NULL); 
    }
    
    XtVaSetValues(compare_data_display, XmNlabelString, info_xmstr, NULL);    
    XtVaSetValues(compare_data_display_2, XmNlabelString, info2_xmstr, NULL); 

}







/*************************************************************/
/* This is the call back for the Product Comparison Screen */

void compare_screen_callback(Widget w,XtPointer client_data, XtPointer call_data)
{

    Widget winform, but, label;
    Widget  scroll_widget;
    /* The major compare screen widgets are declared globally */

    char *title;
    Widget vert_space;
    Widget label2;
    Widget d;
    XmString xmstr;
    
    

/* New button 1 action here Core Dumps on Linux*/
/*#ifdef SUNOS */
    XtTranslations trans;

    static char cmp_draw_trans[] =  "<Btn1Down>: cmp_prod_info()";
    static XtActionsRec cmp_actions[] = {
         {"cmp_prod_info", (XtActionProc) compare_click_info}
    };
/*#endif*/

    if(compare_draw != NULL)  /*if the widget number is non-zero, it exists*/
    return;

   if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell3) {
        d = XmCreateInformationDialog(XtParent(XtParent(w)), 
                                      "Screen Compare Message", NULL, 0);
        xmstr = XmStringCreateLtoR("The Auxiliary Screen does not have a\n"
                                   "Compare Screen function.",
                                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(XtParent(d), XmNtitle, "Screen Compare Message", NULL);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        fprintf(stderr, 
                "The Auxiliary Screen does not have a Compare Screen function.\n"); 
        return;
   }
    
    if(screen_1==NULL || screen_2==NULL) {
        d = XmCreateInformationDialog(XtParent(XtParent(w)), 
                                      "Screen Compare Error", NULL, 0);
        xmstr = XmStringCreateLtoR("Both screens must have displayed products in \n"
                                   "them for the Compare Screen function to work.",
                                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(XtParent(d), XmNtitle, "Screen Compare Error", NULL);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        fprintf(stderr, "Both screens must have displayed products in them "
                        "for the Compare Screen function to work.\n");
        return;
    }
    else {
    /* create the new window, and populate it */
    title = "Product Comparison Screen";
    compare_shell = XtVaCreatePopupShell(title, topLevelShellWidgetClass, shell,
                    
                  NULL);
    XtAddCallback(compare_shell, XmNdestroyCallback, comparekill_callback, NULL);


    winform = XtVaCreateManagedWidget("winform", xmFormWidgetClass, compare_shell,
                      XmNresizable,  True,
                      NULL);

   /* Added for LINUX */
   vert_space = XtVaCreateManagedWidget(" \n \n \n \n \n", 
     xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         10,
     XmNleftAttachment,    XmATTACH_FORM,
     XmNleftOffset,        2,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,
     XmNrightOffset,       5,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);

 
    but = XtVaCreateManagedWidget("Toggle\nScreen", 
     xmPushButtonWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_FORM,
     XmNleftOffset,        10,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,
     NULL);
    XtAddCallback(but, XmNactivateCallback, toggle_compare_callback, NULL);

    label = XtVaCreateManagedWidget("Selected Data: ",
     xmLabelWidgetClass,   winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_WIDGET,
     XmNleftWidget,        but,
     XmNleftOffset,        15,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,

     NULL);

    label2 = XtVaCreateManagedWidget("  Screen:",
     xmLabelWidgetClass,   winform,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,     but,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_FORM,
     XmNleftOffset,        5,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,

     NULL);
     
    compare_screennum = XtVaCreateManagedWidget("1",
     xmLabelWidgetClass,   winform,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,         but,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_WIDGET,
     XmNleftWidget,        label2,
     XmNleftOffset,        2,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,

     NULL);
 

 
    compare_data_display = XtVaCreateManagedWidget("", 
     xmLabelWidgetClass,   winform,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,         label,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_WIDGET,
     XmNleftWidget,        compare_screennum,
     XmNleftOffset,        20,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);

    compare_data_display_2 = XtVaCreateManagedWidget("", 
     xmLabelWidgetClass,   winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_WIDGET,
     XmNleftWidget,        label,
     XmNleftOffset,        5,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL); 
     
    compare_base_info_2 = XtVaCreateManagedWidget("", 
     xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         2,
     XmNleftAttachment,    XmATTACH_NONE,
     XmNleftOffset,        5,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_FORM,
     XmNrightOffset,       2,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);     

    compare_datetime = XtVaCreateManagedWidget("", 
     xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,         compare_base_info_2,
     XmNtopOffset,         1,
     XmNleftAttachment,    XmATTACH_NONE,
     XmNleftOffset,        5,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_FORM,
     XmNrightOffset,       2,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);       
     
     
   compare_prod_descript = XtVaCreateManagedWidget(
     "[---   ---  Product Description Label  ---   ---]", 
     xmLabelWidgetClass, winform,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,         compare_datetime,
     XmNtopOffset,         0,
     XmNleftAttachment,    XmATTACH_FORM,
     XmNleftOffset,        35,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNbottomOffset,      2,
     XmNrightAttachment,   XmATTACH_NONE,
     XmNrightOffset,       2,
     XmNalignment,         XmALIGNMENT_BEGINNING,
     NULL);  
    
  scroll_widget = XtVaCreateManagedWidget ("scrolledwindow",
     xmScrolledWindowWidgetClass,    winform,
     XmNscrollingPolicy,             XmAUTOMATIC,
     XmNwidth,                       width,
     XmNheight,                      height,
     XmNtopAttachment,               XmATTACH_WIDGET,
     XmNtopWidget,                   vert_space,
     XmNtopOffset,                   5,
     XmNleftAttachment,              XmATTACH_FORM,
     XmNleftOffset,                  2,
     XmNbottomAttachment,            XmATTACH_FORM,
     XmNbottomOffset,                2,
     XmNrightAttachment,             XmATTACH_FORM,
     XmNrightOffset,                 5,
     NULL);

    XtVaGetValues(scroll_widget, XmNverticalScrollBar, &compare_vbar, NULL);
    XtAddCallback(compare_vbar, XmNvalueChangedCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNdecrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNincrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNpageDecrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNpageIncrementCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNtoBottomCallback, vert_scroll_callback,  NULL);
    XtAddCallback(compare_vbar, XmNtoTopCallback, vert_scroll_callback,  NULL);
    XtVaSetValues(compare_vbar,
          XmNminimum,       0,
          XmNmaximum,       pheight,
          NULL);

    XtVaGetValues(scroll_widget, XmNhorizontalScrollBar, &compare_hbar, NULL);
    XtAddCallback(compare_hbar, XmNvalueChangedCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNdecrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNincrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNpageDecrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNpageIncrementCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNtoBottomCallback, horiz_scroll_callback,  NULL);
    XtAddCallback(compare_hbar, XmNtoTopCallback, horiz_scroll_callback,  NULL);
    XtVaSetValues(compare_hbar,
          XmNminimum,       0,
          XmNmaximum,       pwidth+barwidth,
          NULL);

    compare_draw = XtVaCreateWidget("drawing_area",
     xmDrawingAreaWidgetClass, scroll_widget,
     XmNwidth,                 pwidth+barwidth,
     XmNheight,                pheight,
     XmNresizePolicy,          XmRESIZE_ANY,
     NULL);
    XtManageChild(compare_draw);
    XtAddCallback(compare_draw, XmNexposeCallback, expose_compare_callback, NULL);
    

/* New button 1 action Core Dumps on Linux */
/*#ifdef SUNOS*/
    XtAppAddActions(app, cmp_actions, XtNumber(cmp_actions));
    trans = XtParseTranslationTable(cmp_draw_trans);
    XtOverrideTranslations(compare_draw, trans);
/*#endif*/

    XtRealizeWidget(compare_shell);
    XtPopup(compare_shell, XtGrabNone);


    /* centering the scrollbars on the image */
    /* centers the image canvas (drawing area) */

    center_screen(compare_vbar, compare_hbar); 
        
     compare_num = 1;
     
     display_compare_product();
     }
     
} /* end compare_screen_callback */






void toggle_compare_callback(Widget w,XtPointer client_data, XtPointer call_data) {

    if(compare_num == 1) {
        compare_num=2;
        display_compare_product();
    }
    else {
        compare_num=1;
        display_compare_product();
    }
}




void expose_compare_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    
    display_compare_product(); 
}



void comparekill_callback(Widget w, XtPointer client_data, XtPointer call_data) {
    compare_draw=NULL;
    compare_shell=NULL;
     compare_data_display = NULL;
     compare_data_display_2 = NULL;
     compare_base_info_2 = NULL;

     compare_prod_descript = NULL;
}



void display_compare_product()
{
    Graphic_product *gp=NULL;
    XmString date_xmstr, prod_xmstr, info_xmstr, info2_xmstr, xmstr;

    XmString descript_xmstr;
    
    int                 dd, dm, dy;
    int                 year;
    static char         sdate[15];
    char                v_date_time[30];

    if(verbose_flag)
        fprintf(stderr,"Copying Information to Compare Screen.\n");

    if(compare_num == 1) {
    XCopyArea(display,sd1->pixmap,XtWindow(compare_draw),gc,0,0,
              pwidth+barwidth,pheight,0,0);
    xmstr = XmStringCreateLtoR("1", XmFONTLIST_DEFAULT_TAG); 
    } else {
    XCopyArea(display,sd2->pixmap,XtWindow(compare_draw),gc,0,0,
              pwidth+barwidth,pheight,0,0);
    xmstr = XmStringCreateLtoR("2", XmFONTLIST_DEFAULT_TAG); 
    }

    XtVaSetValues(compare_screennum,XmNlabelString,xmstr,NULL);

    XmStringFree(xmstr);
    
    if(compare_num==1) {
        XtVaGetValues(sd1->base_info_2, XmNlabelString,  &prod_xmstr, NULL); 
        XtVaGetValues(sd1->data_display, XmNlabelString,  &info_xmstr, NULL);
        XtVaGetValues(sd1->data_display_2, XmNlabelString,  &info2_xmstr, NULL);

        XtVaGetValues(sd1->prod_descript, XmNlabelString,  &descript_xmstr, NULL);
        
    } 
    else {
        XtVaGetValues(sd2->base_info_2, XmNlabelString,  &prod_xmstr, NULL); 
        XtVaGetValues(sd2->data_display, XmNlabelString,  &info_xmstr, NULL);
        XtVaGetValues(sd2->data_display_2, XmNlabelString,  &info2_xmstr, NULL);

        XtVaGetValues(sd2->prod_descript, XmNlabelString,  &descript_xmstr, NULL);
    }


    if(compare_num==1) {
        if(sd1->history != NULL)
            gp  = (Graphic_product *)(sd1->history[0].icd_product + 96);   
    } 
    else {
        if(sd2->history != NULL)
            gp  = (Graphic_product *)(sd2->history[0].icd_product + 96); 
    }
    

    /* create short date */
    if(gp != NULL) {
        calendar_date(gp->vol_date, &dd, &dm, &dy);
        year = 1900 + dy; 
        sprintf(sdate, "%d/%d/%d", year, dm, dd);
    
        sprintf(v_date_time, "%s - %s", sdate, 
                           _88D_secs_to_string(((int)(gp->vol_time_ms) << 16)
                           | ((int)(gp->vol_time_ls) & 0xffff)) );
       
        date_xmstr = XmStringCreateLtoR(v_date_time, XmFONTLIST_DEFAULT_TAG);
    
    } else { /* handle a blank screen */

        date_xmstr = XmStringCreateLtoR("", XmFONTLIST_DEFAULT_TAG);    
    }


    XtVaSetValues(compare_data_display, XmNlabelString, info_xmstr, NULL);    
    XtVaSetValues(compare_data_display_2, XmNlabelString, info2_xmstr, NULL);    
    XtVaSetValues(compare_base_info_2, XmNlabelString, prod_xmstr, NULL);        
    XtVaSetValues(compare_datetime, XmNlabelString, date_xmstr, NULL);        

    XtVaSetValues(compare_prod_descript, XmNlabelString, descript_xmstr, NULL);

    XmStringFree(info_xmstr);
    XmStringFree(info2_xmstr);
    XmStringFree(prod_xmstr);
    XmStringFree(date_xmstr);

    XmStringFree(descript_xmstr);

}


