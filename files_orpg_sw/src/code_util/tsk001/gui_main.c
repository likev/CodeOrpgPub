/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:38 $
 * $Id: gui_main.c,v 1.5 2009/05/15 17:52:38 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* setup_main.c */

#include "gui_main.h"



void setup_gui_display() 
{

  Widget hsep, sm_sep, menu, menubut, but, fsep, seph; 
  
  Widget build_opt, build_menu, build4but, build5but, build6but;
  Widget build7but, build8but;
  
  Widget prod_file_button, im_prod_lb_button, db_select_title;

  Widget all_packetbut;


  Widget filter_label; 

  Widget label2, sep;
  Widget disp_list_but, filter_list_but, label3, label5;

  Widget prod_info_frame, prod_info_hdr_lbl;
  Widget alt_src_frame, alt_src_form, alt_src_frame_label;

XmString *initlist;

   Arg args[20];
   int n=0;
   int i=0;

   XmString xmstr, labelstr, list_label_str; 

  static char* help1 = HELP_FILE_MAIN_WIN;
  static char* help2 = HELP_FILE_DISPLAY_WIN;
  static char* help3 = HELP_FILE_PROD_SPEC;
  static char* help4 = HELP_FILE_SITE_SPEC;

  static char *helpfile = HELP_FILE_PDLB_SELECT;

  static int build4 = BUILD_4_AND_EARLIER;
  static int build5 = BUILD_5;
  static int build6 = BUILD_6;
  static int build7 = BUILD_7;
  static int build8 = BUILD_8; /*  and later */


   /* end declarations */



  
  mainwin = XtVaCreateManagedWidget("mainwin",
     xmMainWindowWidgetClass, shell,
     NULL);

  form = XtVaCreateManagedWidget("form", xmFormWidgetClass, mainwin,
     NULL);

  setup_gui_colors();
  


  /* --- BEGIN MENU BAR --- */

/*  NEW MENU */
/*  Menu Bars must have homogeneous content, in order to add an option menu */
/*  at the top, the manu bar should be a child of the form rather than the */
/*  main window so it does not cross the complete top of the window */
/* //  menubar = XmCreateMenuBar(mainwin, "menubar", NULL, 0); */
  menubar = XmCreateMenuBar(form, "menubar", NULL, 0);

  XtVaSetValues(menubar,
            XmNtopAttachment,    XmATTACH_FORM,
            XmNtopOffset,        0,
            XmNbottomAttachment, XmATTACH_NONE,     
            XmNleftAttachment,   XmATTACH_FORM,
            XmNleftOffset,        0, 
            XmNrightAttachment,  XmATTACH_NONE, 
            NULL); 
        
  XtVaSetValues(XtParent(menubar),
            XmNallowShellResize,     TRUE,
            NULL); 



  /* create file menu */

  menu = XmCreatePulldownMenu(menubar, "filemenu", NULL, 0);
  menubut = XtVaCreateManagedWidget("File",xmCascadeButtonWidgetClass,menubar,
                    XmNsubMenuId, menu, NULL);
  but = XtVaCreateManagedWidget("Preferences...",xmPushButtonWidgetClass,menu,NULL); 
  XtAddCallback(but, XmNactivateCallback, pref_window_callback, NULL);
  
  fsep = XtVaCreateManagedWidget("fsep2",xmSeparatorWidgetClass,menu,NULL);  
  
  but = XtVaCreateManagedWidget("Exit",xmPushButtonWidgetClass,menu,NULL); 
  XtAddCallback(but, XmNactivateCallback, exit_callback, NULL);   
 


  /* create help menu */

  menu = XmCreatePulldownMenu(menubar, "helpmenu", NULL, 0);
  menubut = XtVaCreateManagedWidget("Help",xmCascadeButtonWidgetClass,menubar,
                    XmNsubMenuId, menu, NULL);
  but = XtVaCreateManagedWidget("About CVG",xmPushButtonWidgetClass,menu,NULL); 
  XtAddCallback(but, XmNactivateCallback, about_callback, NULL);

  seph = XtVaCreateManagedWidget("seph1",xmSeparatorWidgetClass,menu,NULL);

  but = XtVaCreateManagedWidget("Main Window",xmPushButtonWidgetClass,menu,NULL); 
  XtAddCallback(but, XmNactivateCallback, help_window_callback, help1);
  but = XtVaCreateManagedWidget("Display Window",xmPushButtonWidgetClass,menu,NULL); 
  XtAddCallback(but, XmNactivateCallback, help_window_callback, help2);
  but = XtVaCreateManagedWidget("Product Configuration",
                xmPushButtonWidgetClass,menu,NULL);
  XtAddCallback(but, XmNactivateCallback, help_window_callback, help3);
  but = XtVaCreateManagedWidget("Site Specific Information",
                xmPushButtonWidgetClass,menu,NULL);
  XtAddCallback(but, XmNactivateCallback, help_window_callback, help4);



  XtVaSetValues(menubar, XmNmenuHelpWidget, menubut, NULL);
 
  XtManageChild(menubar);

  /* --- END MENUBAR --- */



/*****************************************************************/
  labelstr = XmStringCreateLtoR("Product from ORPG ", XmFONTLIST_DEFAULT_TAG);
  build_opt = XmCreateOptionMenu(form, "build_opt", NULL, 0);
  build_menu = XmCreatePulldownMenu(form, "disp_menu", NULL, 0);

  build4but = XtVaCreateManagedWidget("Build 4 -", xmPushButtonWidgetClass, 
             build_menu, NULL);
  XtAddCallback(build4but, XmNactivateCallback, orpg_build_callback, (XtPointer) &build4);
  
  build5but = XtVaCreateManagedWidget("Build 5   ", xmPushButtonWidgetClass, 
            build_menu, NULL);
  XtAddCallback(build5but, XmNactivateCallback, orpg_build_callback, (XtPointer) &build5);
  
  build6but = XtVaCreateManagedWidget("Build 6   ", xmPushButtonWidgetClass, 
             build_menu, NULL);
  XtAddCallback(build6but, XmNactivateCallback, orpg_build_callback, (XtPointer) &build6);
  
  build7but = XtVaCreateManagedWidget("Build 7   ", xmPushButtonWidgetClass, 
             build_menu, NULL);
  XtAddCallback(build7but, XmNactivateCallback, orpg_build_callback, (XtPointer) &build7);
  
  build8but = XtVaCreateManagedWidget("Build 8 +", xmPushButtonWidgetClass, 
            build_menu, NULL);
  XtAddCallback(build8but, XmNactivateCallback, orpg_build_callback, (XtPointer) &build8);

  XtVaSetValues(build_opt, 
        XmNsubMenuId,        build_menu,
        XmNorientation,      XmHORIZONTAL,
        XmNspacing,          0,
        XmNlabelString,      labelstr,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        0,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNrightAttachment,  XmATTACH_FORM, 
        XmNrightOffset,      5,
        XmNleftAttachment,   XmATTACH_NONE,
        NULL);

  XmStringFree(labelstr);

if(orpg_build_i <= 4)
    XtVaSetValues(build_opt, XmNmenuHistory, build4but, NULL);
else if(orpg_build_i == 5)
    XtVaSetValues(build_opt, XmNmenuHistory, build5but, NULL);
else if(orpg_build_i == 6)
    XtVaSetValues(build_opt, XmNmenuHistory, build6but, NULL);   
else if(orpg_build_i == 7)
    XtVaSetValues(build_opt, XmNmenuHistory, build7but, NULL); 
else if(orpg_build_i >= 8)
    XtVaSetValues(build_opt, XmNmenuHistory, build8but, NULL);

  XtManageChild(build_opt);

/*******************************************************************************/


  /* --- BEGIN RADIO BOX --- */
  
  screen_radio_label = XtVaCreateManagedWidget("Screen:", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        45,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       5,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  screen_radiobox = XmCreateRadioBox(form, "radioselect", NULL, 0);

  XtVaSetValues(screen_radiobox,
        XmNorientation,      XmHORIZONTAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        38,
        XmNbottomAttachment, XmATTACH_NONE,     
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       screen_radio_label,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
  XtManageChild(screen_radiobox);


  /* create the toggle buttons within the radio box */
 
  s1_radio = XtVaCreateManagedWidget("1", xmToggleButtonWidgetClass,
                     screen_radiobox, XmNset, TRUE, NULL);
  XtAddCallback(s1_radio, XmNarmCallback, screen1_radio_callback, NULL);
  
  s2_radio = XtCreateManagedWidget("2", xmToggleButtonWidgetClass,
                   screen_radiobox, NULL, 0);
  XtAddCallback(s2_radio, XmNarmCallback, screen2_radio_callback, NULL);
  
  s3_radio = XtCreateManagedWidget("Aux", xmToggleButtonWidgetClass,
                   screen_radiobox, NULL, 0);
  XtAddCallback(s3_radio, XmNarmCallback, screen3_radio_callback, NULL);
  

  /* the default settings: */
  XtSetSensitive(s1_radio, True);
  XtSetSensitive(s2_radio, False);
  XtSetSensitive(s3_radio, True);

  /* --- END RADIO BOX --- */




   alt_src_frame = XtVaCreateManagedWidget ("alternatesrcframe",
     xmFrameWidgetClass,    form,
     XmNwidth,             231,
     XmNheight,            68,
     XmNshadowType,        XmSHADOW_ETCHED_IN,
     
     XmNtopAttachment,     XmATTACH_FORM,
     XmNtopOffset,         20,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       screen_radiobox,
     XmNleftOffset,       2,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_NONE,
     NULL);

  alt_src_frame_label = XtVaCreateManagedWidget ("Alternate Data Source",
    xmLabelGadgetClass,        alt_src_frame,
    XmNframeChildType,         XmFRAME_TITLE_CHILD,
    XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
    NULL);



   alt_src_form = XtVaCreateManagedWidget("altform",
       xmFormWidgetClass, alt_src_frame,
       XmNwidth,             231,
       XmNheight,            68,
       NULL);

  prod_file_button = XtVaCreateManagedWidget("Product Disk File",
     xmPushButtonWidgetClass, alt_src_form,
     XmNtopAttachment,    XmATTACH_FORM,
     XmNtopOffset,        2,
     XmNleftAttachment,   XmATTACH_FORM,
     XmNleftOffset,       6,     
     XmNbottomAttachment, XmATTACH_FORM,
     XmNbottomOffset,       8, 
     XmNrightAttachment,  XmATTACH_NONE,
     XmNwidth,            bwidth-20,
     XmNheight,           bheight,
     NULL);
  XtAddCallback(prod_file_button, 
     XmNactivateCallback, diskfile_select_callback,
     NULL);



  im_prod_lb_button = XtVaCreateManagedWidget("Product LB",
     xmPushButtonWidgetClass, alt_src_form,
     XmNtopAttachment,    XmATTACH_FORM,
     XmNtopOffset,        2,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       prod_file_button,
     XmNleftOffset,       4,
     XmNrightAttachment,   XmATTACH_FORM,
     
     XmNrightOffset,       8,
     
     XmNbottomAttachment, XmATTACH_NONE,
     XmNwidth,            bwidth-55,
     XmNheight,           bheight,
     NULL);
  XtAddCallback(im_prod_lb_button, XmNactivateCallback, ilb_file_select_callback, NULL);



  /*  display all packets button, default is unset */
  all_packetbut = XtVaCreateManagedWidget("Display All Packets", 
     xmToggleButtonWidgetClass, form, 
     XmNtopAttachment,    XmATTACH_FORM,
     XmNtopOffset,        27,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       alt_src_frame,
     XmNleftOffset,       5,
     XmNbottomAttachment, XmATTACH_NONE,
     XmNrightAttachment,  XmATTACH_NONE,
     NULL);   
  
  XtAddCallback(all_packetbut, XmNvalueChangedCallback, select_all_graphic_callback, NULL);

  sm_sep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, form,
      XmNtopAttachment,      XmATTACH_WIDGET,
      XmNtopWidget,          build_opt,
      XmNtopOffset,          6,
      XmNleftAttachment,     XmATTACH_WIDGET,
      XmNleftWidget,         all_packetbut,
      XmNleftOffset,         5,       
      XmNrightAttachment,    XmATTACH_FORM, 
      XmNrightOffset,        8,
      XmNbottomAttachment,   XmATTACH_NONE,      
      NULL);

  /*  overlay button, only active if all packets are selected */
  overlay_but = XtVaCreateManagedWidget("Overlay Packets ", 
     xmToggleButtonWidgetClass, form, 
     XmNtopAttachment,    XmATTACH_FORM,
     XmNtopOffset,        49,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       alt_src_frame,
     XmNleftOffset,       23,
     XmNbottomAttachment, XmATTACH_NONE,
     XmNrightAttachment,  XmATTACH_NONE,
     NULL);  
  
  /* may not be used. instead value for is_set is read by packet_selection_manu */
  XtAddCallback(overlay_but, XmNvalueChangedCallback, overlayflag_callback, NULL);



  packet_button = XtVaCreateManagedWidget("Select Packets",
     xmPushButtonWidgetClass,form,
     XmNtopAttachment,    XmATTACH_FORM,
     XmNtopOffset,        48,
     XmNleftAttachment,   XmATTACH_WIDGET,
     XmNleftWidget,       all_packetbut,
     XmNleftOffset,       10,     
     XmNbottomAttachment, XmATTACH_NONE,
     XmNrightAttachment,  XmATTACH_NONE,
     XmNwidth,            bwidth-33,
     XmNheight,           bheight,
     NULL);
  XtAddCallback(packet_button, XmNactivateCallback, display_packetselect_callback, NULL);

   /*  initial state of the all_packetbut, overlay_but and packet_button */
   if(select_all_flag == TRUE) {
     XtVaSetValues(all_packetbut,
         XmNset,           XmSET,
         NULL);
      XtVaSetValues(overlay_but,
         XmNsensitive,     True,
         XmNset,           XmSET,
         NULL);
     XtVaSetValues(packet_button,
         XmNsensitive,     False,
         NULL);
     
   } else {
     XtVaSetValues(all_packetbut,
         XmNset,           XmUNSET,
         NULL);
     XtVaSetValues(overlay_but,
         XmNsensitive,     False,
         XmNset,           XmUNSET,
         NULL);    
     XtVaSetValues(packet_button,
         XmNsensitive,     True,
         NULL);
   }

  

 hsep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, form,
      XmNseparatorType,      XmSHADOW_ETCHED_IN,
      XmNseparatorType,      XmDOUBLE_LINE,
      XmNmargin,             20,
      XmNtopAttachment,      XmATTACH_FORM,
      XmNtopOffset,          77,
      XmNleftAttachment,     XmATTACH_FORM,
      XmNrightAttachment,    XmATTACH_FORM,
      XmNbottomAttachment,   XmATTACH_NONE,
      XmNwidth,              510,
      XmNheight,             5,
      NULL);


/*  BEGIN GUI ELEMENTS FROM DB SELECT HERE */



   db_select_title = XtVaCreateManagedWidget(" Product Database Selection Dialog ", 
     xmLabelWidgetClass, form,
     XmNtopAttachment,     XmATTACH_WIDGET,
     XmNtopWidget,         hsep,
     XmNtopOffset,         5,
     XmNleftAttachment,    XmATTACH_FORM,
     XmNleftOffset,        5,
     XmNbottomAttachment,  XmATTACH_NONE,
     XmNrightAttachment,   XmATTACH_FORM,
     XmNrightOffset,       5,
     XmNalignment,         XmALIGNMENT_CENTER,
     XmNrecomputeSize,     True,
     NULL);

    xmstr = XmStringCreateLtoR(" Product Database Selection Dialog ", "largefont"); 
    XtVaSetValues(db_select_title, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);


/* the filter selection widgets here */


    filter_label = XtVaCreateManagedWidget("PRODUCT LIST FILTER SELECTION", 
       xmLabelWidgetClass, form,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         db_select_title,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        85,
       NULL);


    prodid_text = XtVaCreateManagedWidget("", 
       xmTextFieldWidgetClass, form, 
       XmNcolumns,           5,
       XmNmaxLength,         4,       
       XmNmarginHeight,      2,
       XmNmarginWidth,       2,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         filter_label,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        290,
       NULL);


    vol_text = XtVaCreateManagedWidget("", 
       xmTextFieldWidgetClass, form, 
       XmNcolumns,           4,
       XmNmaxLength,         3,
       XmNmarginHeight,      2,
       XmNmarginWidth,       2,
       XmNtopAttachment,     XmATTACH_OPPOSITE_WIDGET,
       XmNtopWidget,         prodid_text,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        40,
       NULL);



/*-------------------*/
  prod_filter_radiobox = XmCreateRadioBox(form, "filterselect", NULL, 0);

  XtVaSetValues(prod_filter_radiobox,
        XmNorientation,      XmHORIZONTAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        filter_label,
        XmNtopOffset,        2,
        XmNbottomAttachment, XmATTACH_NONE,             
        XmNrightAttachment,  XmATTACH_WIDGET,
        XmNrightWidget,      prodid_text, 
        XmNrightOffset,      0,
        NULL);
  XtManageChild(prod_filter_radiobox);
  
  /* create the toggle buttons within the radio box */
 
  prodid_radio = XtVaCreateManagedWidget("ProdID", xmToggleButtonWidgetClass,
                   prod_filter_radiobox, XmNset, TRUE, NULL);
  XtAddCallback(prodid_radio, XmNvalueChangedCallback, prodid_filter_callback, NULL);

  type_radio = XtCreateManagedWidget("Name:", xmToggleButtonWidgetClass,
                   prod_filter_radiobox, NULL, 0);
  XtAddCallback(type_radio, XmNvalueChangedCallback, type_filter_callback, NULL);
  pcode_radio = XtCreateManagedWidget("PCode:", xmToggleButtonWidgetClass,
                   prod_filter_radiobox, NULL, 0);
  XtAddCallback(pcode_radio, XmNvalueChangedCallback, pcode_filter_callback, NULL);

/*-------------------*/


              
    label2 = XtVaCreateManagedWidget("Vol:", 
       xmLabelWidgetClass, form,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         filter_label,
       XmNtopOffset,         9,
       XmNrightAttachment,   XmATTACH_WIDGET,
       XmNrightWidget,       vol_text, 
       XmNrightOffset,       3,            
       NULL);
       

  sep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, form,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNorientation,        XmVERTICAL,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          db_select_title,
       XmNtopOffset,          5,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNleftOffset,         345,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              4,
       XmNheight,             60,
       NULL);
/* end filter selection widgets */

/* display list widget */

   disp_list_but = XtVaCreateManagedWidget("Update List & Filter",
      xmPushButtonWidgetClass, form,
      XmNtopAttachment,    XmATTACH_WIDGET,
      XmNtopWidget,        db_select_title,
      XmNtopOffset,        5,
      XmNrightAttachment,  XmATTACH_NONE,
      XmNleftAttachment,   XmATTACH_WIDGET,
      XmNleftWidget,       sep,
      XmNleftOffset,       15,
      XmNwidth,            120,
      XmNheight,           25,
      NULL);
   XtAddCallback(disp_list_but, XmNactivateCallback, build_list_Callback, NULL); 
  
   XtManageChild(disp_list_but);
   
/*  Add capability to set text in disp_list_but label to red when there is */
/*  a new database list to update   Black - current, Red - new data available */


   filter_list_but = XtVaCreateManagedWidget("Apply Filter Only",
      xmPushButtonWidgetClass, form,
      XmNtopAttachment,    XmATTACH_WIDGET,
      XmNtopWidget,        db_select_title,
      XmNtopOffset,        5,
      XmNrightAttachment,  XmATTACH_NONE,
      XmNleftAttachment,   XmATTACH_WIDGET,
      XmNleftWidget,       sep,
      XmNleftOffset,       145,
      XmNwidth,            105,
      XmNheight,           25,
      NULL);
   XtAddCallback(filter_list_but, XmNactivateCallback, filter_list_Callback, NULL); 
  
    XtManageChild(filter_list_but);






label3 = XtVaCreateManagedWidget("Products   Available / Listed: ", 
       xmLabelWidgetClass, form,
       XmNtopAttachment,    XmATTACH_WIDGET, 
       XmNtopWidget,        disp_list_but,
       XmNtopOffset,        5,
       XmNleftAttachment,    XmATTACH_WIDGET,
       XmNleftWidget,        sep,
       XmNleftOffset,        15,
       XmNleftOffset,        10,
       NULL);



/* Linux Issue - Red Hat sometimes has a problem initially rendering
                 label widgets if they are allowed to resize themselves
*/

  
   num_prod_label = XtVaCreateManagedWidget("---", 
      xmLabelWidgetClass,  form,
      XmNtopAttachment,    XmATTACH_WIDGET, 
      XmNtopWidget,        disp_list_but,
      XmNtopOffset,        5,
      XmNrightAttachment,  XmATTACH_NONE,
      XmNleftAttachment,   XmATTACH_WIDGET, 
      XmNleftWidget,       label3,
      XmNleftOffset,       1,
      XmNbottomAttachment, XmATTACH_NONE,
      XmNwidth,            35,
      XmNrecomputeSize,    False,
      XmNalignment,        XmALIGNMENT_END,
      NULL);


    label5 = XtVaCreateManagedWidget("/", 
       xmLabelWidgetClass,  form,
       XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET, 
       XmNtopWidget,        num_prod_label,
       XmNleftAttachment,   XmATTACH_WIDGET, 
       XmNleftWidget,       num_prod_label,
       XmNleftOffset,       1,
       NULL);



   list_size_label = XtVaCreateManagedWidget("---", 
      xmLabelWidgetClass,  form,
      XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
      XmNtopWidget,        num_prod_label,
      XmNrightAttachment,  XmATTACH_NONE,
      XmNleftAttachment,   XmATTACH_WIDGET, 
      XmNleftWidget,       label5,
      XmNleftOffset,       1,
      XmNbottomAttachment, XmATTACH_NONE,
      XmNwidth,            35,
      XmNrecomputeSize,    False,
      XmNalignment,        XmALIGNMENT_END,
      NULL);





   prod_info_frame = XtVaCreateManagedWidget ("prodinfoframe",
     xmFrameWidgetClass,     form,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         list_size_label,
       XmNtopOffset,         33,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        3,
       XmNrightAttachment,   XmATTACH_NONE,      
       XmNbottomAttachment,  XmATTACH_NONE, 
           
     NULL);

   prod_info_label = XtVaCreateManagedWidget("  ", 
     xmLabelWidgetClass,     prod_info_frame,
     XmNalignment,           XmALIGNMENT_BEGINNING,
     XmNrecomputeSize,       False,
     XmNwidth,               598,     
     NULL);


   prod_info_hdr_lbl = XtVaCreateManagedWidget( " ", 

     xmLabelWidgetClass,        form,
       XmNbottomAttachment,     XmATTACH_WIDGET,
       XmNbottomWidget,         prod_info_frame,
       XmNbottomOffset,         2, 
       XmNleftAttachment,       XmATTACH_OPPOSITE_WIDGET,
       XmNleftWidget,           prod_info_frame,     
       XmNalignment,            XmALIGNMENT_BEGINNING,
       NULL);

/* CVG 9.0 - improve alignment of labels and list on some X-servers */
{
   char *label_buf = 
     " msg  Vol ProdID  Param1     Param2     Param3     Param4     Param5     Param6 ";
   labelstr = XmStringCreateLtoR(label_buf, "tabfont");
   XtVaSetValues(prod_info_hdr_lbl, XmNlabelString, labelstr, NULL);
   XmStringFree(labelstr);
}

/* --- DB PRODUCT SELECTION BOX  --- */


/* an initial empty list */
initlist = (XmStringTable) XtMalloc(5 * sizeof (XmString));
initlist[0] = XmStringCreateLtoR (initstr[0], "tabfont");
initlist[1] = XmStringCreateLtoR (initstr[1], "tabfont");
initlist[2] = XmStringCreateLtoR (initstr[2], "tabfont");
initlist[3] = XmStringCreateLtoR (initstr[3], "tabfont");
initlist[4] = XmStringCreateLtoR (initstr[4], "tabfont");


/*****************BEGIN ORIGINAL SELECT DILOG***********************/
   /* create dialog window */
   n=0;
   /* controls height of list area */
   XtSetArg(args[n], XmNlistVisibleItemCount, NUM_VISIBLE); n++;


   XtSetArg(args[n], XmNokLabelString,     
        XmStringCreateLtoR("Select Database Product", XmFONTLIST_DEFAULT_TAG)); n++; 

   XtSetArg(args[n], XmNhelpLabelString,     
        XmStringCreateLtoR("Database Dialog  Help", XmFONTLIST_DEFAULT_TAG)); n++; 

/* the following is critical if using selection box in application shell */
   XtSetArg(args[n], XmNdialogStyle, XmDIALOG_MODELESS); n++;
/* the following keeps this window open */   
   XtSetArg(args[n], XmNautoUnmanage,False); n++;

/* this resource should keep list from getting wider, it doesn't */
/*XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++; */
/* this resource prevents change in size but dramatically slows list function if window too narrow */
/*XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++; */


   db_dialog = XmCreateSelectionBox(form, "product_selectd", args, n);
   
   XtVaSetValues(db_dialog,
      XmNtopAttachment,      XmATTACH_WIDGET,
      XmNmarginHeight,       0,
      XmNmarginWidth,        0,
      XmNtopWidget,          prod_info_frame,     
      XmNtopOffset,          0,
      XmNleftAttachment,     XmATTACH_FORM,
      XmNleftOffset,         5,
      XmNrightAttachment,    XmATTACH_FORM,
      XmNrightOffset,        5,
      XmNbottomAttachment,   XmATTACH_FORM,
      XmNbottomOffset,       5,
      NULL);
   
   /* get rid of buttons 'n' stuff we don't want */
   XtUnmanageChild(XmSelectionBoxGetChild(db_dialog, XmDIALOG_SELECTION_LABEL));
   XtUnmanageChild(XmSelectionBoxGetChild(db_dialog, XmDIALOG_TEXT));
   XtUnmanageChild(XmSelectionBoxGetChild(db_dialog, XmDIALOG_APPLY_BUTTON));
   XtUnmanageChild(XmSelectionBoxGetChild(db_dialog, XmDIALOG_CANCEL_BUTTON));


   /* CVG 9.0 - improve alignment of labels and list on some X-servers */
   create_db_list_label(&list_label_str);
    
   XtVaSetValues(db_dialog,
      XmNlistLabelString,    list_label_str,
      NULL);
   XmStringFree(list_label_str);
 
   
   /* default select first item in list */
   db_list = XmSelectionBoxGetChild(db_dialog, XmDIALOG_LIST);  
 

XtVaSetValues(db_list,
/*  ATTEMPT TO DELEAY CALLBACKS UNTIL BUTTON RELEASED */
/*     XmNautomaticSelection, XmNO_AUTO_SELECT, */
    XmNautomaticSelection, XmAUTO_SELECT,

    XmNselectionPolicy,    XmBROWSE_SELECT,

    XmNitems,              initlist,
    XmNitemCount,          5,
    XmNmatchBehavior,      XmNONE,  /*  disables character match navigation */
    XmNdoubleClickInterval, 250,
    NULL);

/*  FREE initlist; */
    for(i=0; i<5; i++) 
            XmStringFree(initlist[i]);
        
        XtFree((char *)initlist); 
        initlist=NULL;

    XtAddCallback(db_list, XmNbrowseSelectionCallback, browse_select_Callback, NULL);


/*  do not add defaultActionCallback for the db_list itself! disables double click */
/*     XtAddCallback(db_list, XmNdefaultActionCallback, listselection_Callback, NULL); */

   XtAddCallback(db_dialog, XmNokCallback, listselection_Callback, NULL);

   
   
   XtAddCallback(db_dialog, XmNhelpCallback, help_window_callback, helpfile);

   
     XtManageChild(db_dialog);
/*****************END ORIGINAL SELECT DIALOG***********************/   





/*  END GUI ELEMENTS FROM DB SELECT */

  /* cleanup */

  XtVaSetValues(mainwin,

        XmNworkWindow,        form,
        
        
        NULL);

/* the top level shell has already been realized in cvg.c */
  XtRealizeWidget(mainwin);



  /* reset the colors */
  XSetForeground(display, gc, white_color);
  XSetBackground(display, gc, black_color);
  
} /* end setup_gui_display */



/****************************************************************************/
/****************************************************************************/



void setup_gui_colors() 
{
  XColor kolor,unused;
  int *depths, num_depths;

  display = XtDisplay(shell);
  window = XtWindow(shell);

  if((depths = XListDepths(display, XScreenNumberOfScreen(XtScreen(shell)), 
               &num_depths))==NULL) {
      printf("XListDepths() failed.\n");
      exit(0);
  }

  cmap = DefaultColormap(display, 0); 

  XAllocNamedColor(display,cmap,"black",&kolor,&unused);
  black_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"white",&kolor,&unused);
  white_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"lawngreen",&kolor,&unused);
  green_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"red",&kolor,&unused);
  red_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"blue",&kolor,&unused);
  blue_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"grey",&kolor,&unused);
  grey_color = kolor.pixel;

  XAllocNamedColor(display,cmap,"yellow",&kolor,&unused);
  yellow_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"orange",&kolor,&unused);
  orange_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"magenta",&kolor,&unused);
  magenta_color = kolor.pixel;  
  
  XAllocNamedColor(display,cmap,"cyan",&kolor,&unused);
  cyan_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"brown",&kolor,&unused);
  brown_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"light grey",&kolor,&unused);
  light_grey_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"snow",&kolor,&unused);
  snow_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"indian red",&kolor,&unused);
  indian_red_color = kolor.pixel;
  
  XAllocNamedColor(display,cmap,"ghost white",&kolor,&unused);
  ghost_white_color = kolor.pixel;
  
  gcv.foreground = white_color;
  gcv.background = black_color;
  gcv.graphics_exposures = FALSE;
  gc = XCreateGC(display, window, GCBackground|GCForeground|GCGraphicsExposures, &gcv);
  
}



/****************************************************************************/
/****************************************************************************/

/* CVG 9.0 */
void create_db_list_label(XmString *lst_lbl_str)
{

   if(sort_method == 1) {
      *lst_lbl_str = XmStringCreateLtoR(
      " MM/DD-HH:MM  Vol Elev ProdID Name PCode          Product Description", 
      "tabfont");
    
   } else if(sort_method == 2) {

      *lst_lbl_str = XmStringCreateLtoR(
      "MM/DD-HH:MM VolSeq Elev ProdID Name PCode          Product Description", 
      "tabfont");
   }


}



