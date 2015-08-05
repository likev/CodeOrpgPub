/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:39 $
 * $Id: prefs.c,v 1.18 2014/03/25 13:51:39 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */
/* prefs.c */
/* user interface for managing preference data */

#include "prefs.h"



/* 12/29/05 TJG - separated loading preferences into a separate module */


int db_change_flag=FALSE;

/* /// FOR FUTURE TESTING */
/* /Widget label_A, text_A; */
 

int     bad_val = FALSE;


    /*  DEVELOPMENT NOTES: The logic for water feature_rank and the following */
    /*  preferences logic for selecting the amout of road and rail detail */
    /*  has been simplified because of the limits of water, road, rail data */
    /*  that has been placed into the map data files that mapx uses to */
    /*  create the cvg maps. Currently road rank up to 15 and rail rank up to */
    /*  72 is placed into the contiguous state maps.  This could be changed to */
    /*  including road up to 21 and rail up to 74 but requires changes to the */
    /*  detail definitions below and perhaps the map display logic */
#define ROAD_NONE 0
#define ROAD_MAJOR 12
#define ROAD_MORE 31  /*  NOTE: only upto 15 in CONUS maps */
#define RAIL_NONE 70
#define RAIL_MAJOR 75 /*  NOTE: only upto 72 in CONUS maps */





/* ************************************************************************ */
/* ************************************************************************ */


/* pops up dialog to change various program parameters */
void pref_window_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, prefform, mlabel, dblabel, mbut, dbbut, but1, sep;
    Widget label2, label3, but2;
    Widget sep2, gen_prod_label, area_comp_button, table_comp_button;
    Widget sep3, desc_radio_label;
    /* CVG 9.0 */
    Widget screen_size_label, image_size_label, sep1, label1;
    
    Widget sep4, map_opt_button;
    /*  the list buttons are used externally, see prefs.h */
    XmString oklabel, cancellabel, helplabel, labelstr;
    Arg al[20];
    Cardinal ac;

    /* here we make the dialog and ensure it has the buttons we need */
    oklabel = XmStringCreateLtoR("OK", XmFONTLIST_DEFAULT_TAG);
    cancellabel = XmStringCreateLtoR("Cancel", XmFONTLIST_DEFAULT_TAG);
    helplabel = XmStringCreateLtoR("Help", XmFONTLIST_DEFAULT_TAG);

    ac = 0;
    XtSetArg(al[ac], XmNcancelLabelString, cancellabel);  ac++;       
    XtSetArg(al[ac], XmNokLabelString, oklabel);  ac++; 
    XtSetArg(al[ac], XmNhelpLabelString, helplabel);  ac++; 

    d = XmCreateTemplateDialog(shell, "prefd", al, ac);
    XtVaSetValues(XtParent(d), XmNtitle, "Preferences", NULL);
    
    XmStringFree(oklabel);
    XmStringFree(cancellabel);
    XmStringFree(helplabel);

    XtAddCallback(d, XmNokCallback, pref_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, pref_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, help_window_callback, 
                                                      (XtPointer) "prefs.help");

    XtVaSetValues(XtParent(d), 
        XmNmwmDecorations,    
         MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU, 
         XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,
    
          NULL);    
    
    /* now, make a form widget and add all the rest of the buttons */
    prefform = XtVaCreateManagedWidget("form", xmFormWidgetClass, d,
                       XmNresizable,  False,
                       NULL);

    /* next line */


    dblabel = XtVaCreateManagedWidget("Product Database LB:",
       xmLabelWidgetClass,     prefform,
       XmNtopAttachment,       XmATTACH_FORM,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       NULL);

    dbbut = XtVaCreateManagedWidget("Choose...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_FORM,
       XmNtopOffset,           8,
       XmNleftAttachment,      XmATTACH_NONE,
       XmNleftWidget,          mapfile_text,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNbottomOffset,        5,
       XmNrightAttachment,     XmATTACH_FORM,
       XmNrightOffset,         5,
       NULL);
    XtAddCallback(dbbut, XmNactivateCallback, choose_pdlbfile_callback, NULL);

    databasefile_text = XtVaCreateManagedWidget("pdlb_file",
       xmTextFieldWidgetClass, prefform,
       XmNvalue,               product_database_filename,
       XmNcolumns,             50,
       XmNmaxLength,           255,
       XmNtopAttachment,       XmATTACH_FORM,
       XmNtopOffset,           5,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          dblabel,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNbottomOffset,        5,
       XmNrightAttachment,     XmATTACH_WIDGET,
       XmNrightWidget,         dbbut,
       XmNrightOffset,         5,
       NULL);

    /* Next Line */


    verbose_toggle = XtVaCreateManagedWidget("Verbose Console Output",
       xmToggleButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           dblabel,
       XmNtopOffset,           25,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNset,                 verbose_flag,
       NULL);



    /* select the sort type */
    labelstr = XmStringCreateLtoR("Sort Database List Using ", 
                                                         XmFONTLIST_DEFAULT_TAG);

    sort_method_opt = XmCreateOptionMenu(prefform, "sortmethod_opt", NULL, 0);

    sort_method_menu = XmCreatePulldownMenu(prefform, "sortmethod_menu", NULL, 0);
    
    
    sortm_but1 = XtVaCreateManagedWidget("Volume Date-Time (Normal)", 
            xmPushButtonWidgetClass, sort_method_menu, NULL);

    sortm_but2 = XtVaCreateManagedWidget("Volume Sequence Number", 
            xmPushButtonWidgetClass, sort_method_menu, NULL);

                    
    XtVaSetValues(sort_method_opt,  XmNsubMenuId, sort_method_menu,
        XmNorientation,         XmHORIZONTAL,  
        XmNspacing,             0, 
        XmNlabelString,         labelstr, 
        XmNtopAttachment,       XmATTACH_WIDGET,
        XmNtopWidget,           dblabel,
        XmNtopOffset,           20,
        XmNleftAttachment,      XmATTACH_WIDGET,
        XmNleftWidget,          verbose_toggle,
        XmNleftOffset,          40,
        XmNbottomAttachment,    XmATTACH_NONE,
        XmNrightAttachment,     XmATTACH_NONE,
        NULL);

    XmStringFree(labelstr);
        
    if(sort_method == 1)
        XtVaSetValues(sort_method_opt, XmNmenuHistory, sortm_but1, NULL);
    else if(sort_method == 2)
        XtVaSetValues(sort_method_opt, XmNmenuHistory, sortm_but2, NULL);

        
    XtManageChild(sort_method_opt);


 
    /* Next Line */

    /*-----------------------------------------------------------------*/
    sep = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prefform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          sort_method_opt,
       XmNtopOffset,          10,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              400,
       XmNheight,             3,
       NULL);
    /*-----------------------------------------------------------------*/


    label1 = XtVaCreateManagedWidget("Startup Default Product Display Settings - "
                                     " values can be altered in the Product Display Screen",
       xmLabelWidgetClass,  prefform,
       XmNtopAttachment,    XmATTACH_WIDGET, 
       XmNtopWidget,        sep,
       XmNtopOffset,        10,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       5,
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       NULL);


    label2 = XtVaCreateManagedWidget("Display Attribute Defaults",
       xmLabelWidgetClass,  prefform,
       XmNtopAttachment,    XmATTACH_WIDGET, 
       XmNtopWidget,        label1,
       XmNtopOffset,        10,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       15,
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       NULL);
        
    rangering_toggle = XtVaCreateManagedWidget("Range Rings", 
       xmToggleButtonWidgetClass, prefform, 
       XmNtopAttachment,    XmATTACH_WIDGET,
       XmNtopWidget,        label2,
       XmNtopOffset,        3,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       25,
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       XmNset,              def_ring_val,
       NULL);

    azilines_toggle = XtVaCreateManagedWidget("Azimuth Lines", 
       xmToggleButtonWidgetClass, prefform, 
       XmNtopAttachment,    XmATTACH_WIDGET,
       XmNtopWidget,        rangering_toggle,
       XmNtopOffset,        0,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       25,
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       XmNset,              def_line_val,
       NULL);

    mapback_toggle = XtVaCreateManagedWidget("Map Background", 
       xmToggleButtonWidgetClass, prefform, 
       XmNtopAttachment,    XmATTACH_WIDGET,
       XmNtopWidget,        azilines_toggle,
       XmNtopOffset,        0,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       25,
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       XmNset,              def_map_val,
       NULL);


 /* --- IMAGE SIZE RADIO BOX --- */
  
  image_size_label = XtVaCreateManagedWidget("Default Image Size", 
        xmLabelWidgetClass,  prefform,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        label1,
        XmNtopOffset,        10,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_NONE,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNrightOffset,       15,
        NULL);  
  
  img_size_radiobox = XmCreateRadioBox(prefform, "def_img_size", NULL, 0);

  XtVaSetValues(img_size_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        image_size_label,
        XmNtopOffset,        0,
        
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       image_size_label,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
 
  XtManageChild(img_size_radiobox);

  /* create the toggle buttons within the radio box */
 
  img_small_radio = XtCreateManagedWidget("Small", xmToggleButtonWidgetClass,
                   img_size_radiobox, NULL, 0);
  XtAddCallback(img_small_radio, XmNvalueChangedCallback, small_img_callback, NULL);;
  
  img_large_radio = XtCreateManagedWidget("Large", xmToggleButtonWidgetClass,
                   img_size_radiobox, NULL, 0);
  XtAddCallback(img_large_radio, XmNvalueChangedCallback, large_img_callback, NULL );
   
  /* the default settings: */
  XtSetSensitive(img_small_radio, True);
  XtSetSensitive(img_large_radio, True);

  if(large_image_flag == FALSE) {
      XtVaSetValues(img_small_radio,   XmNset, XmSET,   NULL);
      XtVaSetValues(img_large_radio,   XmNset, XmUNSET,   NULL);    
  } else {
      XtVaSetValues(img_small_radio,   XmNset, XmUNSET,   NULL);
      XtVaSetValues(img_large_radio,   XmNset, XmSET,   NULL);    
  }


 /* --- SCREEN SIZE RADIO BOX --- */
  
  screen_size_label = XtVaCreateManagedWidget("Default Screen Size", 
        xmLabelWidgetClass,  prefform,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        label1,
        XmNtopOffset,        10,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_NONE,
        XmNrightAttachment,  XmATTACH_WIDGET,
        XmNrightWidget,      image_size_label,
        XmNrightOffset,       15,
        NULL);  
  
  scr_size_radiobox = XmCreateRadioBox(prefform, "def_scr_size", NULL, 0);

  XtVaSetValues(scr_size_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        screen_size_label,
        XmNtopOffset,        0,
        
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       screen_size_label,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
 
  XtManageChild(scr_size_radiobox);

  /* create the toggle buttons within the radio box */
 
  scr_small_radio = XtCreateManagedWidget("Small", xmToggleButtonWidgetClass,
                   scr_size_radiobox, NULL, 0);
  XtAddCallback(scr_small_radio, XmNvalueChangedCallback, small_scr_callback, NULL);;
  
  scr_large_radio = XtCreateManagedWidget("Large", xmToggleButtonWidgetClass,
                   scr_size_radiobox, NULL, 0);
  XtAddCallback(scr_large_radio, XmNvalueChangedCallback, large_scr_callback, NULL );
   
  /* the default settings: */
  XtSetSensitive(scr_small_radio, True);
  XtSetSensitive(scr_large_radio, True);

  if(large_screen_flag == FALSE) {
      XtVaSetValues(scr_small_radio,   XmNset, XmSET,   NULL);
      XtVaSetValues(scr_large_radio,   XmNset, XmUNSET,   NULL);    
  } else {
      XtVaSetValues(scr_small_radio,   XmNset, XmUNSET,   NULL);
      XtVaSetValues(scr_large_radio,   XmNset, XmSET,   NULL);    
  }



    /*-----------------------------------------------------------------*/
    sep1 = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prefform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          mapback_toggle,
       XmNtopOffset,          10,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              400,
       XmNheight,             3,
       NULL);
    /*-----------------------------------------------------------------*/



    label3 = XtVaCreateManagedWidget("Edit Preference Files:",
       xmLabelWidgetClass,  prefform,
       XmNtopAttachment,    XmATTACH_WIDGET, 
       XmNtopWidget,        sep1,
       XmNtopOffset,        5,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          200,   
    
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       NULL);


 
    but2 = XtVaCreateManagedWidget("Edit Product Configuration...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           label3,
       XmNtopOffset,           0,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          20,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNwidth,               230,
       XmNheight,              25,
       NULL);
    XtAddCallback(but2, XmNactivateCallback, 
                                         product_info_edit_window_callback, NULL);

    
    but1 = XtVaCreateManagedWidget("Edit Site Specific Info...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           label3,
       XmNtopOffset,           0,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          but2,
       XmNleftOffset,          20,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNbottomOffset,        0,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNwidth,               230,
       XmNheight,              25,
       NULL);
    XtAddCallback(but1, XmNactivateCallback, site_info_edit_window_callback, NULL);





    /*-----------------------------------------------------------------*/
    sep2 = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prefform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          but2,
       XmNtopOffset,          15,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              400,
       XmNheight,             3,
       NULL);
    /*-----------------------------------------------------------------*/
    


 desc_radio_label = XtVaCreateManagedWidget("Product Description from:", 
        xmLabelWidgetClass,  prefform,
        XmNtopAttachment,    XmATTACH_WIDGET,             
        XmNtopWidget,        sep2,
        XmNtopOffset,        17,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       5,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  desc_list_radiobox = XmCreateRadioBox(prefform, "radioselect", NULL, 0);

  XtVaSetValues(desc_list_radiobox,
        XmNorientation,      XmHORIZONTAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,             
        XmNtopWidget,        sep2,
        XmNtopOffset,        10,
        XmNbottomAttachment, XmATTACH_NONE,     
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       desc_radio_label,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
  XtManageChild(desc_list_radiobox);


  /* create the toggle buttons within the radio box */
 
  list1_but = XtVaCreateManagedWidget("ORPG Product Info", 
                   xmToggleButtonWidgetClass,
                    desc_list_radiobox, XmNset, TRUE, NULL);
  XtAddCallback(list1_but, XmNvalueChangedCallback, list1_radio_callback, NULL);
  list2_but = XtCreateManagedWidget("CVG Product Descriptions", 
                   xmToggleButtonWidgetClass,
                    desc_list_radiobox, NULL, 0);
  XtAddCallback(list2_but, XmNvalueChangedCallback, list2_radio_callback, NULL);
 
  
  /* the default settings: */
  XtSetSensitive(list1_but, True);
  XtSetSensitive(list2_but, True);

  if(use_cvg_list_flag == FALSE) {
      XtVaSetValues(list1_but,
            XmNset,      XmSET,
            NULL);
      XtVaSetValues(list2_but,
            XmNset,      XmUNSET,
            NULL);    
      
  } else {
      XtVaSetValues(list1_but,
            XmNset,      XmUNSET,
            NULL);
      XtVaSetValues(list2_but,
            XmNset,      XmSET,
            NULL);    
    
  }
  

/* prevent ORPG info source selection if standalone */

  if(standalone_flag == TRUE) {
      XtVaSetValues(list1_but,
            XmNset,      XmUNSET,
            NULL);
      XtVaSetValues(list2_but,
            XmNset,      XmSET,
            NULL);
      XtSetSensitive(list1_but, False); 
  }       



    /*-----------------------------------------------------------------*/
    sep3 = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prefform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          desc_list_radiobox,
       XmNtopOffset,          10,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              400,
       XmNheight,             3,
       NULL);
    /*-----------------------------------------------------------------*/



    gen_prod_label = XtVaCreateManagedWidget("Generic Product Display Options",
       xmLabelWidgetClass,  prefform,
       XmNtopAttachment,    XmATTACH_WIDGET,             
       XmNtopWidget,        sep3,
       XmNtopOffset,        10,
       XmNleftAttachment,   XmATTACH_FORM,
       XmNleftOffset,       176,   
    
       XmNrightAttachment,  XmATTACH_NONE,
       XmNbottomAttachment, XmATTACH_NONE,
       NULL);


    area_comp_button = XtVaCreateManagedWidget("Area Component Options...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           gen_prod_label,
       XmNtopOffset,           5,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          76,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNwidth,               170,
       XmNheight,              25,
       NULL);
    XtAddCallback(area_comp_button, XmNactivateCallback, 
                                            area_comp_opt_window_callback, NULL);

    table_comp_button = XtVaCreateManagedWidget("Table Component Options...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           gen_prod_label,
       XmNtopOffset,           5,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          area_comp_button,
       XmNleftOffset,          40,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       
       XmNwidth,               170,
       XmNheight,              25,
       NULL);
    XtAddCallback(table_comp_button, XmNactivateCallback, 
                                            table_comp_opt_window_callback, NULL);

    XtSetSensitive(table_comp_button, False);



    /*-----------------------------------------------------------------*/
    sep4 = XtVaCreateManagedWidget("", xmSeparatorWidgetClass, prefform,
       XmNseparatorType,      XmSHADOW_ETCHED_IN,
       XmNmargin,             10,
       XmNtopAttachment,      XmATTACH_WIDGET,
       XmNtopWidget,          area_comp_button,
       XmNtopOffset,          10,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNwidth,              400,
       XmNheight,             3,
       NULL);
    /*-----------------------------------------------------------------*/
    
    
    

    map_opt_button = XtVaCreateManagedWidget("Map Display Options...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           sep4,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          180,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNwidth,               170,
       XmNheight,              25,
       NULL);
    XtAddCallback(map_opt_button, XmNactivateCallback, 
                                          map_opt_window_callback, NULL);



    mlabel = XtVaCreateManagedWidget("Background Map File:",
       xmLabelWidgetClass,     prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           map_opt_button,       
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       NULL);


    mbut = XtVaCreateManagedWidget("Choose...",
       xmPushButtonWidgetClass, prefform,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           map_opt_button,        
       XmNtopOffset,           8,
       XmNleftAttachment,      XmATTACH_NONE,
       XmNleftWidget,          mapfile_text,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNbottomOffset,        5,
       XmNrightAttachment,     XmATTACH_FORM,
       XmNrightOffset,         5,
       NULL);
    XtAddCallback(mbut, XmNactivateCallback, choose_mapfile_callback, NULL);

    mapfile_text = XtVaCreateManagedWidget("Map File: ",
       xmTextFieldWidgetClass, prefform,
       XmNvalue,               map_filename,
       XmNcolumns,             50,
       XmNmaxLength,           255,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           map_opt_button,        
       XmNtopOffset,           5,
       XmNleftAttachment,      XmATTACH_WIDGET,
       XmNleftWidget,          mlabel,
       XmNleftOffset,          5,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNbottomOffset,        5,
       XmNrightAttachment,     XmATTACH_WIDGET,
       XmNrightWidget,         mbut,
       XmNrightOffset,         5,
       NULL);




 
    XtManageChild(d);

} /*  end pref_window_callback */





/* save the preferences to disk */
void pref_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    FILE *pref_file;

    char filename[256], *str;
    char rr_buf[10], az_buf[10], map_buf[10], vb_buf[10];
    /* CVG 9.0 */
    char scr_buf[10], img_buf[10];
    
    Boolean is_set;

    
    XmString list_label_str=NULL;
    Widget curr_sort_but;

    fprintf(stderr," PREFS - Updating CVG preference file.\n");


    /*  BEGIN UPDATING THE PREFS FILE *************************** */
    
    /* open the program preferences data file */
    sprintf(filename, "%s/prefs", config_dir);
    if((pref_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "Could not open program preferences\n");
    exit(0);
    }

    /* commit the changes to each of the items that can actually
     * affect what the program is doing right now
     */
    XtVaGetValues(mapfile_text, XmNvalue, &str, NULL);
    strcpy(map_filename, str);
    free(str);
 
    XtVaGetValues(databasefile_text, XmNvalue, &str, NULL);
    strcpy(product_database_filename, str);
    free(str);

    XtVaGetValues(rangering_toggle, XmNset, &is_set, NULL);
    def_ring_val = is_set;
 
    XtVaGetValues(azilines_toggle, XmNset, &is_set, NULL);
    def_line_val = is_set; 

    XtVaGetValues(mapback_toggle, XmNset, &is_set, NULL);
    def_map_val = is_set;
    
    XtVaGetValues(verbose_toggle, XmNset, &is_set, NULL);
    verbose_flag = is_set;     

    
    /* write out each data item name and value to the file */

    if(map_filename == NULL)
        map_filename[0] = '\0';
/* fprintf(pref_file, "map_file %s\n", map_filename); */


    if(def_ring_val == TRUE)
        strcpy(rr_buf, "true");
    else
        strcpy(rr_buf, "false");
/* fprintf(pref_file, "range_ring_on %s\n", rr_buf); */
    

    if(def_line_val == TRUE)
        strcpy(az_buf, "true");
    else
        strcpy(az_buf, "false");
/* fprintf(pref_file, "azimuth_line_on %s\n", az_buf); */
    

    if(def_map_val == TRUE)
        strcpy(map_buf, "true");
    else
        strcpy(map_buf, "false");
/* fprintf(pref_file, "map_bkgd_on %s\n", map_buf); */


    if(verbose_flag == TRUE)
        strcpy(vb_buf, "true");
    else
        strcpy(vb_buf, "false");
/* fprintf(pref_file, "verbose_output %s\n", vb_buf); */

/* CVG 9.0 */
    if(large_screen_flag == TRUE)
        strcpy(scr_buf, "large");
    else
        strcpy(scr_buf, "small");

   if(large_image_flag == TRUE)
        strcpy(img_buf, "large");
    else
        strcpy(img_buf, "small");



    if(product_database_filename == NULL)
        product_database_filename[0] = '\0';
/*         fprintf(pref_file, "product_database_lb %s", product_database_filename); */
/*         fprintf(stderr," Update database filename,"); */
/*     } */
 
    
    fprintf(stderr,"\n");
    
    /* CVG 9.0 - added scr_buf and img_buf parameters */
    write_prefs_file(pref_file, map_filename, rr_buf, az_buf, map_buf, 
                     vb_buf, scr_buf, img_buf, product_database_filename);


    fflush(pref_file);
    fclose(pref_file);


    /*  END OF UPDATING THE PREFS FILE *********************************** */




    /* Update the sort method file and the database list heading label */
    XtVaGetValues(sort_method_opt,
        XmNmenuHistory, &curr_sort_but,
        NULL);
        
    if(curr_sort_but == sortm_but1) {
        sort_method = 1;
    } else if(curr_sort_but == sortm_but2) {
        sort_method = 2;
    }
    write_sort_method(sort_method);


    /* CVG 9.0 - improve alignment of labels and list on some X-servers */
    create_db_list_label(&list_label_str);
    
      XtVaSetValues(db_dialog,
         XmNlistLabelString,    list_label_str,
         NULL);
      XmStringFree(list_label_str);

  

    /* called in case the user left the map file or database file text boxes empty */
    load_program_prefs(FALSE); /*  during initial load is FALSE */

    
 /*  incase the source of product descriptions has changed  */
    if(use_cvg_list_flag != prev_cvg_list_flag) {
     
        write_descript_source(use_cvg_list_flag);
        
        assoc_clear_s(product_names);
        assoc_clear_s(short_prod_names);
        assoc_clear_s(product_mnemonics);
        
        load_product_names(FALSE,FALSE); /*  Not from child, not initial read */
        
    }

    /* Ensures toggle correcly set if load_product_names can't find ORPG list */
    if(use_cvg_list_flag == FALSE) {
        XtVaSetValues(list1_but,
              XmNset,      XmSET,
              NULL);
        XtVaSetValues(list2_but,
              XmNset,      XmUNSET,
              NULL);    
        
    } else {
        XtVaSetValues(list1_but,
              XmNset,      XmUNSET,
              NULL);
        XtVaSetValues(list2_but,
              XmNset,      XmSET,
              NULL);    
      
    }

    
    /*  if the database file name, sort method, or list_flag has changed, */
    /*  pass changes to background file */
    if( (strcmp(product_database_filename, prev_db_filename) != 0) ||
        (sort_method != prev_sort_method)                          || 
        (use_cvg_list_flag != prev_cvg_list_flag)                ) {  
            /*  send signal to read_db child to get new database fileneme */
            kill(new_pid, SIGUSR1);
            /*  force the db list to display an error message if database changed */
            build_msg_list(-1);
            strcpy(prev_db_filename, product_database_filename);
            prev_sort_method = sort_method;
            prev_cvg_list_flag = use_cvg_list_flag;
    }


    XtUnmanageChild(w);

} /*  end pref_ok_callback */







/* if canceled, we just don't do anything but kill the window */
void pref_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    /* sort_method = prev_sort_method; */
    use_cvg_list_flag = prev_cvg_list_flag;
    
    XtUnmanageChild(w);
}




/* pops up a file selection box to get a map file */
void choose_mapfile_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmfile, xmdir;
    char dirname[255];
    int i;
    char static *helpfile = HELP_FILE_FILE_SELECT;

    d = XmCreateFileSelectionDialog(w, "choose_mapfile_dialog", NULL, 0);

    XtVaSetValues(XtParent(d), 
          XmNtitle, "Choose File...", 

          XmNwidth,                560,
          XmNheight,               380,
          XmNallowShellResize,     FALSE,          

          NULL);

          
    XtVaSetValues(d, 
    
         XmNdialogStyle,       XmDIALOG_FULL_APPLICATION_MODAL, 
         /* why not set to XmDIALOG_FILE_SELECTION, the default? */
         XmNmarginHeight,      0,
         XmNmarginWidth,       0,
         
         XmNdirListLabelString,     
              XmStringCreateLtoR
              ("-- Directories ------------                                             ", 
                                                         XmFONTLIST_DEFAULT_TAG),
          XmNfileListLabelString,    
              XmStringCreateLtoR
              ("                                             ------------------ Files --", 
                                                         XmFONTLIST_DEFAULT_TAG),
          XmNlistVisibleItemCount,  16,          
          
          NULL);

    /* set the currently selected file to the current map file, and the
     * current directory to the directory that it is in
     */
    xmfile = XmStringCreateLtoR(map_filename, XmFONTLIST_DEFAULT_TAG);
    i = strlen(map_filename) - 1;
    while( (map_filename[i] != '/') && (i > 0) ) i--;
    strncpy(dirname, map_filename, i+2);  /* include the '/' */
    dirname[i+1] = '\0';
    xmdir = XmStringCreateLtoR(dirname, XmFONTLIST_DEFAULT_TAG);
   
    XtVaSetValues(d, 
          XmNdirSpec,           xmfile,
          XmNdirectory,         xmdir,
          NULL);
    XmStringFree(xmdir);
    XmStringFree(xmfile);
    
    XtAddCallback(d, XmNokCallback, mapfile_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, mapfile_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, help_window_callback, (XtPointer) helpfile);

    XtManageChild(d);
}




/* set the map file field to whatever was selected */
void mapfile_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct *cbs = 
                                   (XmFileSelectionBoxCallbackStruct *)call_data;
    char *filename;

    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
    XtVaSetValues(mapfile_text, XmNvalue, filename, NULL);
    XtUnmanageChild(w);
    free(filename);
}




/* if canceled, we just don't do anything but kill the window */
void mapfile_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}




/* pops up a file selection box to get a new location for the
 * product database linear buffer
 */
void choose_pdlbfile_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmfile, xmdir;
    char dirname[255];
    int i;
    char static *helpfile = HELP_FILE_FILE_SELECT;

    d = XmCreateFileSelectionDialog(w, "choose_pdlbfile_dialog", NULL, 0);

    XtVaSetValues(XtParent(d), XmNtitle, "Choose File...", 
    
          XmNwidth,                560,
          XmNheight,               380,
          XmNallowShellResize,     FALSE,      
    
          NULL);


    XtVaSetValues(d, 
    
         XmNdialogStyle,       XmDIALOG_FULL_APPLICATION_MODAL, 
         /*  why not set to XmDIALOG_FILE_SELECTION, the default? */
         XmNmarginHeight,      0,
         XmNmarginWidth,       0,
         
         XmNdirListLabelString,     
              XmStringCreateLtoR
              ("-- Directories ------------                                             ", 
                                                         XmFONTLIST_DEFAULT_TAG),
          XmNfileListLabelString,    
              XmStringCreateLtoR
              ("                                             ------------------ Files --", 
                                                         XmFONTLIST_DEFAULT_TAG),
          XmNlistVisibleItemCount,  16,          
          
          NULL);



    /* set the currently selected file to the current database file, and the
     * current directory to the directory that it is in
     */
    xmfile = XmStringCreateLtoR(product_database_filename, XmFONTLIST_DEFAULT_TAG);
    i = strlen(product_database_filename) - 1;
    while( (product_database_filename[i] != '/') && (i > 0) ) 
        i--;
    
    strncpy(dirname, product_database_filename, i+2);  /* include the '/' */
    dirname[i+1] = '\0';
    xmdir = XmStringCreateLtoR(dirname, XmFONTLIST_DEFAULT_TAG);
   
    XtVaSetValues(d, 
          XmNdirSpec,           xmfile,
          XmNdirectory,        xmdir,
          NULL);
    XmStringFree(xmdir);
    XmStringFree(xmfile);
    
    XtAddCallback(d, XmNokCallback, pdlbfile_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, pdlbfile_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, help_window_callback, (XtPointer) helpfile);

    XtManageChild(d);
}




/* set the pdlb file field to whatever was selected */
void pdlbfile_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct *cbs = 
                                 (XmFileSelectionBoxCallbackStruct *)call_data;
    char *filename;

    db_change_flag=TRUE;

    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
    XtVaSetValues(databasefile_text, XmNvalue, filename, NULL);
    XtUnmanageChild(w);
    free(filename);
}

/* if canceled, we just don't do anything but kill the window */
void pdlbfile_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}




void list1_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    use_cvg_list_flag = FALSE;
    
}


void list2_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    use_cvg_list_flag = TRUE;
    
}


void small_scr_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    large_screen_flag = FALSE;
    
}


void large_scr_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    large_screen_flag = TRUE;
    
}


void small_img_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    large_image_flag = FALSE;
    
}


void large_img_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    large_image_flag = TRUE;
    
}


/* ********************************************************************** */
/* ********************************************************************** */


/* window for graphic editing of product specific data */
void product_info_edit_window_callback(Widget w, XtPointer client_data, 
                                                            XtPointer call_data)
{
    Widget d, rc_a, label, label_, psep, psep2, fsep, fsep2, rsep, msep;
    Widget pseplabel, ovsep, lpsep, lpsep_; /* CVG 9.1  added */
    Widget pi_label;
    Widget rc_sub, but, rc_sub3, rc_b;
    Widget rc_sub2; /* added CVG 9.1 for override packt selection */
    Widget rc_sub1; /* added CVG 9.1 */
    Widget pref_legend_frame;
    XmString *xmlist;
    int i;
    char buf[200];


  /* the following are arguments to pushbutton callbacks in the
   * option menus.  They must support all defined option buttons.
   *
   * the load_product_info() function in prefs_load.c should not
   * give a warning message on the following valid values:
   *
   * the product_edit_fill_fields() function in prefs_edit.c must
   * accept these values and reject others by setting the default.
   *
   */   
  static int msgt0 = 0;
  static int msgt1 = 1;
  static int msgt2 = 2;
  static int msgt3 = 3;
  static int msgt4 = 4;
  static int msgt999 = 999;
  
  static int msgt1neg = -1;

/* CVG 9.1 - added packet 1 coord override for geographic products */
  static int pkt10 = 0;
  static int pkt11 = 1;

  static int resi0 = 0;
  static int resi1 = 1;
  static int resi2 = 2;
  static int resi3 = 3;
  static int resi4 = 4;
  static int resi5 = 5;
  static int resi6 = 6;
  static int resi7 = 7;
  static int resi8 = 8;
  static int resi9 = 9;
  static int resi10 = 10;
  static int resi11 = 11;
  static int resi12 = 12;
  static int resi13 = 13;
  
  static int resi1neg = -1;



  /* CVG 9.1 - added override of colors for non-2d array packets */
 static int over0 = 0;
  /*  possibly add 2 - Text Symb SYMBOL no value */
  static int over4 = 4;
  static int over6 = 6;
  /*  possibly add 7 - Unlinked Vector no value */
  static int over8 = 8;
  static int over9 = 9;
  static int over10 = 10;
  static int over20 = 20;
  static int over43 = 43; /*  Area Component */
  /*  perhaps never need to override color of 44 and 45 */
  /*   static int over44 = 44; // Text Component */
  /*   static int over45 = 45; // Table Component   */
  static int over51 = 51;  /*  contours */


  static int digf0 = 0;
  static int digf1 = 1;
  static int digf2 = 2;
  static int digf3 = 3;
  static int digf4 = 4;
  static int digf5 = 5;
  static int digf6 = 6;
    
  static int digf1neg = -1;


  static int assp0 = 0;
  /*  possibly add 2 - Text Symb SYMBOL no value */
  static int assp4 = 4;
  static int assp6 = 6;
  /*  possibly add 7 - Unlinked Vector no value */
  static int assp8 = 8;
  static int assp9 = 9;
  static int assp10 = 10;
  static int assp16 = 16;
  static int assp17 = 17;
  /*  static int assp18 = 18; */ /* removed cvg 9.0 */
  static int assp20 = 20;
  static int assp41 = 41; /*  Radial Component */
  static int assp42 = 42; /*  Grid Component */
  static int assp43 = 43; /*  Area Component */
  /*  perhaps never need to override color of 44 and 45 */
  /*   static int assp44 = 44; // Text Component */
  /*   static int assp45 = 45; // Table Component   */
  static int assp51 = 51;  /*  contours IIIIIIIIIIIIT'S BAAAAAAAAAAAAAACK CVG 8.4 */
  static int assp53 = 53;
  static int assp54 = 54;
  static int assp55 = 55;

  /* CVG 9.3 - added elevation flag */
  static int el0    = 0;
  static int el1    = 1;

    /*  future improvement: could use ComboBox to list filenames currently in  */
    /*                      preferences but permit other values to be entered */
    /*  future improvement: could define a list of standard units but */
    /*                      permit using other values */
  

    d = XmCreateFormDialog(shell, "prefd", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Product Configuration Edit", NULL);

    XtVaSetValues(XtParent(d), 
        XmNmwmDecorations,    
             MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU, 
        XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,
        
        NULL); 

    pi_label = XtVaCreateManagedWidget("Product ID",
       xmLabelWidgetClass,   d,
       XmNtopAttachment,     XmATTACH_FORM,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        3,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_NONE,
       NULL);       
 
     /* set up list items - product ids.  we get the number of pids from
      * a typical set of data from the product info 
      */


    /* set up list items - product ids.  we get the number of pids from
     * the pid list 
     */   
    xmlist = malloc(sizeof(XmString)*pid_list->size);
    for(i=0; i<pid_list->size; i++) {
        sprintf(buf, "%d", pid_list->keys[i]);
        xmlist[i] = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    }

    pi_list = XmCreateScrolledList(d, "id_list", NULL, 0);
    XtVaSetValues(XtParent(pi_list),
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         pi_label,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        3,
       XmNbottomAttachment,  XmATTACH_FORM,
       XmNbottomOffset,      10,
       XmNrightAttachment,   XmATTACH_NONE,
       XmNwidth,             70,
       NULL);

    XtVaSetValues(pi_list,
       XmNselectionPolicy,   XmSINGLE_SELECT,
       XmNvisibleItemCount,  36,
       XmNitems,             xmlist,
       XmNitemCount,         pid_list->size,
       NULL);

    XtManageChild(pi_list);
    XtAddCallback(pi_list, XmNsingleSelectionCallback, 
                                        product_edit_select_callback, NULL);

       

/**************************************************************************/

   rc_b = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, d,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       XmNtopAttachment,     XmATTACH_FORM,
       XmNtopOffset,         0,
       XmNleftAttachment,    XmATTACH_WIDGET,
       XmNleftWidget,        pi_list,
       XmNleftOffset,        25,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_NONE,
       NULL);   

      but = XtVaCreateManagedWidget("Add Product", xmPushButtonWidgetClass, rc_b, 
            XmNwidth,     80, 
            XmNheight,    30,
            XmNrecomputeSize, False, 
            NULL);

      XtAddCallback(but, XmNactivateCallback, product_edit_add_callback, NULL);

      label_id = XtVaCreateManagedWidget("         Selected Product ID:", 
                      xmLabelWidgetClass, rc_b, NULL);

      id_label = XtVaCreateManagedWidget("", xmLabelWidgetClass, rc_b, 
                    XmNmarginLeft,   5,
                    NULL);



       ledg_label = XtVaCreateManagedWidget("    \n       ",
       xmLabelWidgetClass,   d,
       XmNwidth,             95,
       XmNheight,            30,
       XmNrecomputeSize,     False,
       XmNtopAttachment,     XmATTACH_FORM,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_NONE,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_FORM,
       XmNrightOffset,       5,      
       NULL);  
       



/***************************************************************************/

    rc_a = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, d,
       XmNpacking,           XmPACK_COLUMN,
       XmNnumColumns,        2,
       XmNisAligned,         True,
       XmNentryAlignment,    XmALIGNMENT_BEGINNING,
       XmNmarginWidth,       0,
       XmNmarginHeight,      0,
       XmNspacing,           0,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         rc_b,
       XmNtopOffset,         0,
       XmNleftAttachment,    XmATTACH_WIDGET,
       XmNleftWidget,        pi_list,
       XmNleftOffset,        0,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_NONE,
       NULL);

/*    rc_el = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, d,
       XmNpacking,           XmPACK_COLUMN,
       XmNnumColumns,        2,
       XmNisAligned,         True,
       XmNentryAlignment,    XmALIGNMENT_BEGINNING,
       XmNmarginWidth,       0,
       XmNmarginHeight,      0,
       XmNspacing,           0,
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         rc_b,
       XmNtopOffset,         0,
       XmNleftAttachment,    XmATTACH_WIDGET,
       XmNleftWidget,        pi_list,
       XmNleftOffset,        0,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_NONE,
       NULL);*/

    /* the First Column of rc_a ------------------------------------------- */



/*1:1*/  label_mt = XtVaCreateManagedWidget(
                      "          Product Message Type:", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:2*/ /* CVG 9.1 - added selection for packet 1 coordinate override in geographic products */
         label_pk = XtVaCreateManagedWidget(
                      "          Packet 1 Coord Override (Geographic):", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:3*/  label_pr = XtVaCreateManagedWidget(
                      "          Product Resolution:", 
                      xmLabelWidgetClass, rc_a, NULL);
         


/*1:4*/ /* CVG 9.1 - added override of colors for non-2d array packets */
         label_o_pal = XtVaCreateManagedWidget(
                      "          Overriding Palette: ________", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:5*/ /* CVG 9.1 - added override of colors for non-2d array packets */
         label_o_pkt = XtVaCreateManagedWidget(
                      "          Packet with overridden colors:", 
                      xmLabelWidgetClass, rc_a, NULL);



/*1:6*/  /* CVG 9.1 - added separator widget */
         rc_sub1 = XtVaCreateManagedWidget("rc_layout",
                 xmRowColumnWidgetClass, rc_a,
                 XmNpacking,           XmPACK_TIGHT,
                 XmNorientation,       XmHORIZONTAL,
                 NULL);  

         label_ = XtVaCreateManagedWidget("     LEGEND CONFIGURATION  ", 
                                          xmLabelWidgetClass, rc_sub1, NULL);
         lpsep_ = XtVaCreateManagedWidget("lpsep_",xmSeparatorWidgetClass,
                                         rc_sub1, XmNsensitive,  True, NULL);
         
         

/*1:7*/  label_df = XtVaCreateManagedWidget(
                      "          Legend File Type:", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:8*/  label_l1 = XtVaCreateManagedWidget(
                      "          Configured Legend File:", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:9*/  label_l2 = XtVaCreateManagedWidget(
                      "               Legend File 2 (Dig Vel only):", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:10*//*  label_ = XtVaCreateManagedWidget("", xmLabelWidgetClass, rc_a, */
        /*                                  XmNheight     , 1, NULL);      */
          

/*1:11*/  label_p1 = XtVaCreateManagedWidget(
                      "          Configured Color Palette:", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:12*/  label_p2 = XtVaCreateManagedWidget(
                      "               Color Palette 2 (Dig Vel only):", 
                      xmLabelWidgetClass, rc_a, NULL);

/*1:13*/ label_pt = XtVaCreateManagedWidget(
                      "          Associated Packet / Component:", 
                      xmLabelWidgetClass, rc_a, NULL);

         /* CVG 9.1 - remove label */
/*1:14*/ /*label_ = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL);*/


/*1:15*/ label_um = XtVaCreateManagedWidget("          Unit of Measure:", 
                      xmLabelWidgetClass, rc_a, 
                      XmNmarginBottom,    7,
                      NULL);


/*1:16*//* label_ = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL);*/
         /* CVG 9.3 - added elevation flag */
         label_el = XtVaCreateManagedWidget("          Elevation-based?:", 
                      xmLabelWidgetClass, rc_a, 
                      XmNmarginBottom,    4,
                      NULL);

         label_ = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL);

/*1:17*/ rc_sub = XtVaCreateManagedWidget("rc_layout",
            xmRowColumnWidgetClass, rc_a,
            XmNpacking,           XmPACK_COLUMN,
            XmNorientation,       XmHORIZONTAL,
            XmNmarginWidth,       5,
            XmNspacing,           25,
/*             XmNresizeWidth,       False, */
            NULL);
        but = XtVaCreateManagedWidget("Undo Edits", xmPushButtonWidgetClass, rc_sub, 
            XmNwidth,     80, 
            XmNheight,    15,
            XmNrecomputeSize, False, 
            NULL);
        XtAddCallback(but, XmNactivateCallback, product_edit_revert_callback, NULL);
        but = XtVaCreateManagedWidget("Apply Edits", xmPushButtonWidgetClass, rc_sub, 
            XmNwidth,     80, 
            XmNheight,    15,
            XmNrecomputeSize, False, 
            NULL);
        XtAddCallback(but, XmNactivateCallback, product_edit_commit_callback, NULL);



        XtVaSetValues(label_mt, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_pr, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_df, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   False,   NULL); 
        XtVaSetValues(label_pt, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_um, XmNsensitive,   False,   NULL);

    /* end first column ------------------------------------------- */



    /* the Second Column of rc_a ------------------------------------------- */


         
/*2:1*/ 
        msgtype_opt = XmCreateOptionMenu(rc_a, "msgtype_opt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        msgtype_menu = XmCreatePulldownMenu(rc_a, "msgtype_menu", NULL, 0);
        
        msgt_but0 = XtVaCreateManagedWidget("Geographic Product (0)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but0, XmNactivateCallback, msgt_cb, (XtPointer)&msgt0);
        msgt_but1 = XtVaCreateManagedWidget("Non-Geographic Product (1)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but1, XmNactivateCallback, msgt_cb, (XtPointer)&msgt1);
        msgt_but2 = XtVaCreateManagedWidget("Stand alone Tabular (2)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but2, XmNactivateCallback, msgt_cb, (XtPointer)&msgt2);
        msgt_but3 = XtVaCreateManagedWidget("Radar Coded Message (3)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but3, XmNactivateCallback, msgt_cb, (XtPointer)&msgt3);
        msgt_but4 = XtVaCreateManagedWidget("Text Message (4)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but4, XmNactivateCallback, msgt_cb, (XtPointer)&msgt4);
        msgt_but999 = XtVaCreateManagedWidget("Unknown Message (999)", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but999, XmNactivateCallback, msgt_cb, 
                                                             (XtPointer)&msgt999);
        msep = XtVaCreateManagedWidget("msep",xmSeparatorWidgetClass,
                                         msgtype_menu, NULL);
        msgt_but1neg = XtVaCreateManagedWidget("Parameter Not Configured (-1)  ", 
                xmPushButtonWidgetClass, msgtype_menu, NULL);
        XtAddCallback(msgt_but1neg, XmNactivateCallback, msgt_cb, 
                                                             (XtPointer)&msgt1neg);
                        
        XtVaSetValues(msgtype_opt,  XmNsubMenuId, msgtype_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(msgtype_opt);


/*2:2*/ /* CVG 9.1 - added selection for packet 1 coordinate override in geographic products */
        packet_1_opt = XmCreateOptionMenu(rc_a, "packet_1_opt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        packet_1_menu = XmCreatePulldownMenu(rc_a, "packet_1_menu", NULL, 0);
        
        pkt1_but0 = XtVaCreateManagedWidget("Default: 1/4 km radar coord (0)", 
                xmPushButtonWidgetClass, packet_1_menu, NULL);
        XtAddCallback(pkt1_but0, XmNactivateCallback, pkt1_cb, (XtPointer)&pkt10);
        pkt1_but1 = XtVaCreateManagedWidget("Switch to pixel screen coord (1)", 
                xmPushButtonWidgetClass, packet_1_menu, NULL);
        XtAddCallback(pkt1_but1, XmNactivateCallback, pkt1_cb, (XtPointer)&pkt11);
        
        XtVaSetValues(packet_1_opt,  XmNsubMenuId, packet_1_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(packet_1_opt);

         
/*2:3*/ 
        resindex_opt = XmCreateOptionMenu(rc_a, "resindexopt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        resindex_menu = XmCreatePulldownMenu(rc_a, "resindex_menu", NULL, 0);
        
        resi_but0 = XtVaCreateManagedWidget("N/A (0)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but0, XmNactivateCallback, resi_cb, (XtPointer)&resi0);
        resi_but1 = XtVaCreateManagedWidget("0.08 nm ( 150 m) TDWR (1)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but1, XmNactivateCallback, resi_cb, (XtPointer)&resi1);
        resi_but2 = XtVaCreateManagedWidget("0.13 nm ( 250 m) WSR-88D (2)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but2, XmNactivateCallback, resi_cb, (XtPointer)&resi2);
        resi_but3 = XtVaCreateManagedWidget("0.16 nm ( 300 m) TDWR (3)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but3, XmNactivateCallback, resi_cb, (XtPointer)&resi3);
        resi_but4 = XtVaCreateManagedWidget("0.25 nm ( 463 m) ARSR-4 (4)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but4, XmNactivateCallback, resi_cb, (XtPointer)&resi4);
        resi_but5 = XtVaCreateManagedWidget("0.27 nm ( 500 m) WSR-88D (5)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but5, XmNactivateCallback, resi_cb, (XtPointer)&resi5);
        resi_but6 = XtVaCreateManagedWidget("0.32 nm ( 600 m) TDWR (6)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but6, XmNactivateCallback, resi_cb, (XtPointer)&resi6);
        resi_but7 = XtVaCreateManagedWidget("0.50 nm ( 926 m) ASR-11 (7)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but7, XmNactivateCallback, resi_cb, (XtPointer)&resi7);
        resi_but8 = XtVaCreateManagedWidget("0.54 nm (1000 m) WSR-88D (8)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but8, XmNactivateCallback, resi_cb, (XtPointer)&resi8);
        resi_but9 = XtVaCreateManagedWidget("1.1  nm (2000 m) WSR-88D (9)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but9, XmNactivateCallback, resi_cb, (XtPointer)&resi9);
        resi_but10 = XtVaCreateManagedWidget("2.2 nm (4000 m) WSR-88D (10)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but10, XmNactivateCallback, resi_cb, (XtPointer)&resi10);
        resi_but11 = XtVaCreateManagedWidget("Unknown (11)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but11, XmNactivateCallback, resi_cb, (XtPointer)&resi11);
        resi_but12 = XtVaCreateManagedWidget("Overlay Product (12)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but12, XmNactivateCallback, resi_cb, (XtPointer)&resi12);
        resi_but13 = XtVaCreateManagedWidget("Generic Radial (13)", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but13, XmNactivateCallback, resi_cb, (XtPointer)&resi13);
        rsep = XtVaCreateManagedWidget("rsep",xmSeparatorWidgetClass,
                                         resindex_menu, NULL);
        resi_but1neg = XtVaCreateManagedWidget("Parameter Not Configured (-1)  ", 
                xmPushButtonWidgetClass, resindex_menu, NULL);
        XtAddCallback(resi_but1neg, XmNactivateCallback, resi_cb, 
                                                              (XtPointer)&resi1neg);
                                
        XtVaSetValues(resindex_opt,  XmNsubMenuId, resindex_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(resindex_opt);



/*2:4*/ /* CVG 9.1 - added override of colors for non-2d array packets */
    rc_sub2 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

        override_palette_text = XtVaCreateManagedWidget("", 
                   xmTextFieldWidgetClass, rc_sub2, 
                   XmNcolumns, 29,
                   XmNheight,  15,
                   NULL);


/*2:5*/ /* CVG 9.1 - added override of colors for non-2d array packets */
        overridepacket_opt = XmCreateOptionMenu(rc_a, "overridepacket_opt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        overridepacket_menu = XmCreatePulldownMenu(rc_a, "overridepacket_menu", NULL, 0);
        
        over_but0 = XtVaCreateManagedWidget("No Packet Selected (0)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but0, XmNactivateCallback, overp_cb, (XtPointer)&over0);
        
        ovsep = XtVaCreateManagedWidget("ovsep",xmSeparatorWidgetClass,
                                         overridepacket_menu, NULL);
                                         
        /*  possibly add 2 - Text Symb SYMBOL no value */
        over_but4 = XtVaCreateManagedWidget("  4 - Wind Barb Symbol (4)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but4, XmNactivateCallback, overp_cb, (XtPointer)&over4);
        over_but6 = XtVaCreateManagedWidget("  6 - Linked Vector No Val (6)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but6, XmNactivateCallback, overp_cb, (XtPointer)&over6);
        /*  possibly add 7 - Unlinked Vector no value */
        over_but8 = XtVaCreateManagedWidget("  8 - Text/Symb TEXT (8)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but8, XmNactivateCallback, overp_cb, (XtPointer)&over8);
        over_but9 = XtVaCreateManagedWidget("  9 - Linked Vector (9)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but9, XmNactivateCallback, overp_cb, (XtPointer)&over9);
        over_but10 = XtVaCreateManagedWidget("10 - Unlinked Vector (10)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but10, XmNactivateCallback, overp_cb, (XtPointer)&over10);
        over_but20 = XtVaCreateManagedWidget("20 - Point Feature Data (20)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but20, XmNactivateCallback, overp_cb, (XtPointer)&over20);
        over_but43 = XtVaCreateManagedWidget("Generic Area Component (43)  ", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but43, XmNactivateCallback, overp_cb, (XtPointer)&over43);
   /*  perhaps never a need to override colors for 44 and 45: */
   /*      over_but44 = XtVaCreateManagedWidget("Generic Text Component (44)",  */
   /*                 xmPushButtonWidgetClass, overridepacket_menu, NULL); */
   /*      XtAddCallback(over_but44,XmNactivateCallback,overp_cb,(XtPointer)&over44); */
   /*         over_but45 = XtVaCreateManagedWidget("Generic Table Component (45)",  */
   /*                 xmPushButtonWidgetClass, overridepacket_menu, NULL); */
   /*      XtAddCallback(over_but45,XmNactivateCallback,overp_cb,(XtPointer)&over45); */
        over_but51 = XtVaCreateManagedWidget("0E03x - Contour Data (51)", 
                xmPushButtonWidgetClass, overridepacket_menu, NULL);
        XtAddCallback(over_but51, XmNactivateCallback, overp_cb, (XtPointer)&over51);


        XtVaSetValues(overridepacket_opt,  XmNsubMenuId, overridepacket_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(overridepacket_opt);




         
/*2:6*/  /* CVG 9.1 - changed to separator widget */
         /*label = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL);*/
         lpsep = XtVaCreateManagedWidget("lpsep",xmSeparatorWidgetClass,
                                         rc_a, XmNsensitive,  False, NULL);
         
/*2:7*/ 
        digflag_opt = XmCreateOptionMenu(rc_a, "digflag_opt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        digflag_menu = XmCreatePulldownMenu(rc_a, "digflag_menu", NULL, 0);
        
        digf_but0 = XtVaCreateManagedWidget("Legend File Not Used   (0)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but0, XmNactivateCallback, digf_cb, (XtPointer)&digf0);
        digf_but1 = XtVaCreateManagedWidget("Digital Legend (calculated) (1)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but1, XmNactivateCallback, digf_cb, (XtPointer)&digf1);
        digf_but2 = XtVaCreateManagedWidget("Digital Legend (defined) (2)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but2, XmNactivateCallback, digf_cb, (XtPointer)&digf2);
        digf_but3 = XtVaCreateManagedWidget("Vel Digital Legend (defined) (3)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but3, XmNactivateCallback, digf_cb, (XtPointer)&digf3);

        fsep = XtVaCreateManagedWidget("fsep",xmSeparatorWidgetClass,
                                         digflag_menu, NULL);
        fsep2 = XtVaCreateManagedWidget("fsep2",xmSeparatorWidgetClass,
                                         digflag_menu, NULL);
                                         
        digf_but4 = XtVaCreateManagedWidget("Generic Signed Integer G(4)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but4, XmNactivateCallback, digf_cb, (XtPointer)&digf4);
        digf_but5 = XtVaCreateManagedWidget("Generic UnSigned Integer G(5)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but5, XmNactivateCallback, digf_cb, (XtPointer)&digf5);
        digf_but6 = XtVaCreateManagedWidget("Generic Real Data Type G(6)", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but6, XmNactivateCallback, digf_cb, (XtPointer)&digf6);
        
        fsep = XtVaCreateManagedWidget("fsep",xmSeparatorWidgetClass,
                                         digflag_menu, NULL);
        fsep2 = XtVaCreateManagedWidget("fsep2",xmSeparatorWidgetClass,
                                         digflag_menu, NULL);
                                         
        digf_but1neg = XtVaCreateManagedWidget("Parameter Not Configured (-1)  ", 
                xmPushButtonWidgetClass, digflag_menu, NULL);
        XtAddCallback(digf_but1neg, XmNactivateCallback, digf_cb, 
                                                           (XtPointer)&digf1neg);

        XtVaSetValues(digflag_opt,  XmNsubMenuId, digflag_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(digflag_opt);

        /*  FUTURE TO DO - ADD SUPPORT FOR METHODS 4 AND 6 */
        XtVaSetValues(digf_but4, XmNsensitive,   False,   NULL);
        XtVaSetValues(digf_but6, XmNsensitive,   False,   NULL);
        
/*  future improvement: could use ComboBox to list filenames currently in  */
/*                      preferences but permit other values to be entered */
        
/*2:8*/  
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

       diglegfile_text = XtVaCreateManagedWidget("", 
                   xmTextFieldWidgetClass, rc_sub3, 
                   XmNcolumns, 29,
                   XmNheight,  15,
                   NULL);
       label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, 
                            rc_sub3, NULL);

         
/*2:9*/  
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

       diglegfile2_text = XtVaCreateManagedWidget("-.-", 
                   xmTextFieldWidgetClass, rc_sub3, 
                   XmNcolumns, 29, 
                   XmNheight,  15,
                   NULL);
       label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, 
                            rc_sub3, NULL);


/*2:10*/ /* label = XtVaCreateManagedWidget("", xmLabelWidgetClass, rc_a, */
         /*                                      XmNheight     , 1, NULL);*/
          
          
/*2:11*/  
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

       confpalette_text = XtVaCreateManagedWidget("", 
                   xmTextFieldWidgetClass, rc_sub3, 
                   XmNcolumns, 29,
                   XmNheight,  15,
                   NULL);
       label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, 
                            rc_sub3, NULL);


         
/*2:12*/  
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

       confpalette2_text = XtVaCreateManagedWidget("-.-", 
                   xmTextFieldWidgetClass, rc_sub3, 
                   XmNcolumns, 29, 
                   XmNheight,  15,
                   NULL);
       label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, 
                            rc_sub3, NULL);

/*2:13*/ 
        /* CVG 9.1 -  reorganized buttons */
        assocpacket_opt = XmCreateOptionMenu(rc_a, "assocpacketopt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        assocpacket_menu = XmCreatePulldownMenu(rc_a, "assocpacket_menu", NULL, 0);
        /* CVG 9.1 - changed from APPLY TO ALL PACKETS to No packet Selected */
        assp_but0 = XtVaCreateManagedWidget("No Packet Selected (0)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but0, XmNactivateCallback, assp_cb, (XtPointer)&assp0);
        
        psep2 = XtVaCreateManagedWidget("psep2a",xmSeparatorWidgetClass,
                                         assocpacket_menu, NULL);

        assp_but41 = XtVaCreateManagedWidget("Generic Radial (41)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but41, XmNactivateCallback, assp_cb, (XtPointer)&assp41);
        /*  future support 42: */
        assp_but42 = XtVaCreateManagedWidget("Generic Grid (42)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but42, XmNactivateCallback, assp_cb, (XtPointer)&assp42);

/* ****************************************************************************** */
        
        assp_but16 = XtVaCreateManagedWidget("16 - Digital Radial Data (16)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but16, XmNactivateCallback, assp_cb, (XtPointer)&assp16);

        assp_but53 = XtVaCreateManagedWidget("AF1Fx - Radial Data (53)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but53, XmNactivateCallback, assp_cb, (XtPointer)&assp53);
        assp_but54 = XtVaCreateManagedWidget("BA07x - Raster Data (54)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but54, XmNactivateCallback, assp_cb, (XtPointer)&assp54);
        assp_but55 = XtVaCreateManagedWidget("BA0Fx - Raster Data (55)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but55, XmNactivateCallback, assp_cb, (XtPointer)&assp55);
        assp_but17 = XtVaCreateManagedWidget("17 - Digital Precip Data (17)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but17, XmNactivateCallback, assp_cb, (XtPointer)&assp17);
        /* cvg 9.0 - removed */
/*        assp_but18 = XtVaCreateManagedWidget("18 - Precip Rate Data (18)",      */
/*            xmPushButtonWidgetClass, assocpacket_menu, NULL);                   */
/*       XtAddCallback(assp_but18,XmNactivateCallback,assp_cb,(XtPointer)&assp18);*/

        psep = XtVaCreateManagedWidget("psep1a",xmSeparatorWidgetClass,
                                         assocpacket_menu, NULL);
        psep2 = XtVaCreateManagedWidget("psep2b",xmSeparatorWidgetClass,
                                         assocpacket_menu, NULL);
        pseplabel = XtVaCreateManagedWidget("Non-Legend Packets: ", 
                                            xmLabelWidgetClass, 
                                            assocpacket_menu, NULL);
        psep2 = XtVaCreateManagedWidget("psepc2",xmSeparatorWidgetClass,
                                         assocpacket_menu, NULL);

        /*  possibly add 2 - Text Symb SYMBOL no value */
        assp_but4 = XtVaCreateManagedWidget("  4 - Wind Barb Symbol (4)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but4, XmNactivateCallback, assp_cb, (XtPointer)&assp4);
        assp_but6 = XtVaCreateManagedWidget("  6 - Linked Vector No Val (6)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but6, XmNactivateCallback, assp_cb, (XtPointer)&assp6);
        /*  possibly add 7 - Unlinked Vector no value */
        assp_but8 = XtVaCreateManagedWidget("  8 - Text/Symb TEXT (8)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but8, XmNactivateCallback, assp_cb, (XtPointer)&assp8);
        assp_but9 = XtVaCreateManagedWidget("  9 - Linked Vector (9)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but9, XmNactivateCallback, assp_cb, (XtPointer)&assp9);
        assp_but10 = XtVaCreateManagedWidget("10 - Unlinked Vector (10)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but10, XmNactivateCallback, assp_cb, (XtPointer)&assp10);

        assp_but20 = XtVaCreateManagedWidget("20 - Point Feature Data (20)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but20, XmNactivateCallback, assp_cb, (XtPointer)&assp20);
        assp_but43 = XtVaCreateManagedWidget("Generic Area Component (43)  ", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but43, XmNactivateCallback, assp_cb, (XtPointer)&assp43);
        assp_but51 = XtVaCreateManagedWidget("0E03x - Contour Data (51)", 
                xmPushButtonWidgetClass, assocpacket_menu, NULL);
        XtAddCallback(assp_but51, XmNactivateCallback, assp_cb, (XtPointer)&assp51);

/*  perhaps never a need to override colors for 44 and 45: */
/*      assp_but44 = XtVaCreateManagedWidget("Generic Text Component (44)",  */
/*                 xmPushButtonWidgetClass, assocpacket_menu, NULL); */
/*      XtAddCallback(assp_but44,XmNactivateCallback,assp_cb,(XtPointer)&assp44); */
/*         assp_but45 = XtVaCreateManagedWidget("Generic Table Component (45)",  */
/*                 xmPushButtonWidgetClass, assocpacket_menu, NULL); */
/*      XtAddCallback(assp_but45,XmNactivateCallback,assp_cb,(XtPointer)&assp45); */

        
        XtVaSetValues(assocpacket_opt,  XmNsubMenuId, assocpacket_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(assocpacket_opt);

        XtVaSetValues(assp_but42, XmNsensitive,   False,   NULL);

         /* CVG 9.1 - remove label */
/*2:14*/ /*label = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL); */



/*2:15*/
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc_a,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);  

       unit_text = XtVaCreateManagedWidget("", xmTextFieldWidgetClass, rc_sub3, 
            XmNcolumns, 30, 
            XmNheight,      15,
            NULL);
  /*     label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, 
                            rc_sub3, NULL);*/
        /*  future improvement: could define a list of standard units but */
        /*  permit using other values */



/*2:16*/ /* label = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc_a, NULL);*/
         /* CVG 9.3 - added elevation flag */
        elflag_opt = XmCreateOptionMenu(rc_a, "elflagopt", NULL, 0);
        elflag_menu = XmCreatePulldownMenu(rc_a, "elflag_menu", NULL, 0);
        elf_but0 = XtVaCreateManagedWidget("Not Elevation-Based (0)", 
                xmPushButtonWidgetClass, elflag_menu, NULL);
        XtAddCallback(elf_but0, XmNactivateCallback, elflag_cb, (XtPointer)&el0);
        elf_but1 = XtVaCreateManagedWidget("Elevation-Based (1)", 
                xmPushButtonWidgetClass, elflag_menu, NULL);
        XtAddCallback(elf_but1, XmNactivateCallback, elflag_cb, (XtPointer)&el1);
        XtVaSetValues(elflag_opt,  XmNsubMenuId, elflag_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);
        XtManageChild(elflag_opt);

      
/*2:17*/ rc_sub = XtVaCreateManagedWidget("rc_layout",
            xmRowColumnWidgetClass, rc_a,
            XmNpacking,           XmPACK_COLUMN,
            XmNorientation,       XmHORIZONTAL,
            XmNalignment, XmALIGNMENT_CENTER,
            XmNmarginWidth,       40,
            XmNspacing,           25,
            NULL);



         but = XtVaCreateManagedWidget("Save Changes", 
            xmPushButtonWidgetClass, rc_sub, 
            XmNwidth,     90, 
            XmNheight,    15,
            XmNrecomputeSize, False, 
            NULL);
         XtAddCallback(but, XmNactivateCallback, product_edit_save_callback, NULL);

    /* end second column ------------------------------------------- */



   pref_legend_frame = XtVaCreateManagedWidget ("preflegendframe",
     xmFrameWidgetClass,             d,
     XmNwidth,                       sidebarwidth-2,
     XmNheight,                      height+75,
     XmNshadowType,                  XmSHADOW_IN,
     XmNtopAttachment,               XmATTACH_WIDGET,
     XmNtopWidget,                   rc_b,
     XmNtopOffset,                   0,
     XmNleftAttachment,              XmATTACH_WIDGET,
     XmNleftWidget,                  rc_a,
     XmNleftOffset,                  0,     
     XmNbottomAttachment,            XmATTACH_NONE,
     XmNrightAttachment,             XmATTACH_FORM,
     XmNrightOffset,                 5,
     NULL);
   
    pref_legend_draw = XtVaCreateWidget("drawing_area",
     xmDrawingAreaWidgetClass, pref_legend_frame,
     XmNwidth,                 sidebarwidth-2,
     XmNheight,                height+75,
     NULL);

    XtManageChild(pref_legend_draw);
    XtAddCallback(pref_legend_draw, XmNexposeCallback, 
                                                      pref_legend_expose_cb, NULL);


    XtAddCallback(XtParent(d), XmNdestroyCallback, prod_edit_kill_cb, NULL);


    XtManageChild(d);
 
     /*  NOTE: on Linux, the shell (parent of d) must be realized (managed) */
     /*        before attempting to create the pixmap */
        
     pref_legend_pix = XCreatePixmap(XtDisplay(pref_legend_draw), 
                   XtWindow(pref_legend_draw), sidebarwidth, sidebarheight, 
                   XDefaultDepthOfScreen(XtScreen(XtParent(d))) );

    pref_legend_grey_pixmap();
    pref_legend_show_pixmap();

    /* now that we've displayed stuff, we can free up some memory, since
     * the list keeps a copy of the strings
     */
    for(i=0; i<pid_list->size; i++)
        XmStringFree(xmlist[i]);
    free(xmlist);
    
    
    
} /*  end product_info_edit_window_callback() */




void prod_edit_kill_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    XFreePixmap(XtDisplay(pref_legend_draw), pref_legend_pix);


}





/* similar expose callback for the legend window */
void pref_legend_expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
  

  pref_legend_show_pixmap();


}





/* copies the offscreen copy of the legend into the viewable area */
void pref_legend_show_pixmap()
{
  
  
      XCopyArea(XtDisplay(pref_legend_draw), pref_legend_pix, 
                   XtWindow(pref_legend_draw), gc, 0, 0, 
                   sidebarwidth, sidebarheight, 0, 0);

}




void pref_legend_clear_pixmap()
{
  /* clear the pixmap to black */
  

    XSetForeground(XtDisplay(pref_legend_draw), gc, black_color);
    XFillRectangle(XtDisplay(pref_legend_draw), pref_legend_pix, gc, 0, 0, 
           sidebarwidth, sidebarheight); 


}



void pref_legend_grey_pixmap()
{
  /* clear the pixmap to black */
  

    XSetForeground(XtDisplay(pref_legend_draw), gc, grey_color);
    XFillRectangle(XtDisplay(pref_legend_draw), pref_legend_pix, gc, 0, 0, 
           sidebarwidth, sidebarheight); 


}













/* ******************************************************************* */
/* ******************************************************************* */


/* window for graphic editing of site specific data */
void site_info_edit_window_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, rc, label;
    Widget rc_sub, but, rc_sub2, rc_sub3;
    Widget isep;

    XmString *xmlist;
    int i;
    char buf[200];


  static int rdrt0 = 0;
  static int rdrt4 = 4;
/*  possible future radar types:   */
   static int rdrt1 = 1; /* ARSR-4 */
   static int rdrt2 = 2; /* ASR-9  */
   static int rdrt3 = 3; /* ASR-11 */
  



    d = XmCreateFormDialog(shell, "prefd", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Site Specific Info Edit", NULL);

    XtVaSetValues(XtParent(d), 
        XmNmwmDecorations,    
             MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU, 
        XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,
        
        NULL); 

    label = XtVaCreateManagedWidget("Site ID",
       xmLabelWidgetClass,   d,
       XmNtopAttachment,     XmATTACH_FORM,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        5,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_NONE,
       NULL);       
 
    /* set up list items - site ids.  we get the number of sids from
     * a typical set of data from the site info 
     */
    xmlist = malloc(sizeof(XmString)*icao_list->size);

    for(i=0; i<icao_list->size; i++) {
        sprintf(buf, "%d", icao_list->keys[i]);
        xmlist[i] = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    }

    si_list = XmCreateScrolledList(d, "id_list", NULL, 0);
    XtVaSetValues(XtParent(si_list),
       XmNtopAttachment,     XmATTACH_WIDGET,
       XmNtopWidget,         label,
       XmNtopOffset,         5,
       XmNleftAttachment,    XmATTACH_FORM,
       XmNleftOffset,        5,
       XmNbottomAttachment,  XmATTACH_FORM,
       XmNbottomOffset,      10,
       XmNrightAttachment,   XmATTACH_NONE,
       XmNrightOffset,       5,
       XmNwidth,             70,
       NULL);

    XtVaSetValues(si_list,
       XmNselectionPolicy,   XmSINGLE_SELECT,
       XmNvisibleItemCount,  11,
       XmNitems,             xmlist,
       XmNitemCount,         icao_list->size,
       NULL);

    XtManageChild(si_list);
    XtAddCallback(si_list, XmNsingleSelectionCallback, 
                                           site_edit_select_callback, NULL);



    rc = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, d,
       XmNpacking,           XmPACK_COLUMN,
       XmNnumColumns,        2,
       XmNisAligned,         True,
       XmNentryAlignment,    XmALIGNMENT_BEGINNING,
       XmNtopAttachment,     XmATTACH_FORM,
       XmNtopOffset,         0,
       XmNleftAttachment,    XmATTACH_WIDGET,
       XmNleftWidget,        si_list,
       XmNleftOffset,        5,
       XmNbottomAttachment,  XmATTACH_NONE,
       XmNrightAttachment,   XmATTACH_FORM,
       XmNrightOffset,       0,
       NULL);


    /* the First Column of rc ------------------------------------------- */

/* 1:1 */
    rc_sub2 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);    
    but = XtVaCreateManagedWidget("Add Site", xmPushButtonWidgetClass, rc_sub2, 
            XmNheight,    15,
            NULL);
    XtAddCallback(but, XmNactivateCallback, site_edit_add_callback, NULL); 
    label = XtVaCreateManagedWidget("   Selected Site:", 
                                        xmLabelWidgetClass, rc_sub2, NULL);
                                        
 
 
/* 1:2 */                                        
    label = XtVaCreateManagedWidget("               Radar Type:", 
                                        xmLabelWidgetClass, rc, NULL);
                                        
/* 1:3 */                                        
    label = XtVaCreateManagedWidget("                         ICAO:", 
                                        xmLabelWidgetClass, rc, 
                                        XmNmarginBottom,    7, 
                                        NULL);


/* 1:4 */ label = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc, NULL);

                                        
/* 1:5 */
    rc_sub = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);

    but = XtVaCreateManagedWidget("Undo Edits", xmPushButtonWidgetClass, rc_sub, 
            XmNheight,    15,
            NULL);
    XtAddCallback(but, XmNactivateCallback, site_edit_revert_callback, NULL);
    but = XtVaCreateManagedWidget("Apply Edits", xmPushButtonWidgetClass, rc_sub, 
            XmNheight,    15,
            NULL);
    XtAddCallback(but, XmNactivateCallback, site_edit_commit_callback, NULL);

    /* end first column ------------------------------------------- */



    /* the Second Column of rc ------------------------------------------- */



/* 2:1 */
    site_id_label = XtVaCreateManagedWidget("", xmLabelWidgetClass, rc, 
            XmNmarginTop,    11,        
            XmNmarginLeft,   10,       
            NULL);
 
    
/* 2:2 */        
        rdrtype_opt = XmCreateOptionMenu(rc, "rdrtype_opt", NULL, 0);
        /*  note: XmNentryAlignment, XmALIGNMENT_CENTER cannot be overridden */
        rdrtype_menu = XmCreatePulldownMenu(rc, "rdrtype_menu", NULL, 0);
        
        rdrt_but0 = XtVaCreateManagedWidget("WSR-88D (0)", 
                xmPushButtonWidgetClass, rdrtype_menu, NULL);
        XtAddCallback(rdrt_but0, XmNactivateCallback, rdrt_cb, (XtPointer)&rdrt0);
        rdrt_but4 = XtVaCreateManagedWidget("TDWR (4)", 
                xmPushButtonWidgetClass, rdrtype_menu, NULL);
        XtAddCallback(rdrt_but4, XmNactivateCallback, rdrt_cb, (XtPointer)&rdrt4);
        
/*  possible future radar types: */
        isep = XtVaCreateManagedWidget("isep0",xmSeparatorWidgetClass,
                                          rdrtype_menu,NULL); 
        rdrt_but1 = XtVaCreateManagedWidget("ARSR-4 (1)",  
                xmPushButtonWidgetClass, rdrtype_menu, NULL); 
        XtAddCallback(rdrt_but1, XmNactivateCallback, rdrt_cb, (XtPointer)&rdrt1); 
        rdrt_but2 = XtVaCreateManagedWidget("ASR-9 (2)  ",  
                xmPushButtonWidgetClass, rdrtype_menu, NULL); 
        XtAddCallback(rdrt_but2, XmNactivateCallback, rdrt_cb, (XtPointer)&rdrt2); 
        rdrt_but3 = XtVaCreateManagedWidget("ASR-11 (3)",  
                xmPushButtonWidgetClass, rdrtype_menu, NULL); 
        XtAddCallback(rdrt_but3, XmNactivateCallback, rdrt_cb, (XtPointer)&rdrt3); 
        
                        
        XtVaSetValues(rdrtype_opt,  XmNsubMenuId, rdrtype_menu,
            XmNorientation, XmVERTICAL,  XmNspacing, 0,  
            NULL);  
        
        XtManageChild(rdrtype_opt);

/* 2:3 */
    rc_sub3 = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       NULL);    

    icao_text = XtVaCreateManagedWidget("", xmTextFieldWidgetClass, rc_sub3, 
            XmNcolumns,     5, 
            XmNwidth,       50,
            XmNheight,      15,
            NULL);
    label = XtVaCreateManagedWidget("  ", xmLabelWidgetClass, rc_sub3, NULL);


/* 2:4 */ label = XtVaCreateManagedWidget(" ", xmLabelWidgetClass, rc, NULL);


/* 2:5 */
    rc_sub = XtVaCreateManagedWidget("rc_layout",
       xmRowColumnWidgetClass, rc,
       XmNpacking,           XmPACK_TIGHT,
       XmNorientation,       XmHORIZONTAL,
       XmNmarginWidth,       30,
       NULL);
    
    but = XtVaCreateManagedWidget("Save Changes", xmPushButtonWidgetClass, 
                                                                     rc_sub, NULL);
    XtAddCallback(but, XmNactivateCallback, site_edit_save_callback, NULL);


    /* end second column ------------------------------------------- */



    XtManageChild(d);



    /* now that we've displayed stuff, we can free up some memory, since
     * the list keeps a copy of the strings
     */
    for(i=0; i<icao_list->size; i++)
        XmStringFree(xmlist[i]);
    free(xmlist);
    
    
    
}

















/* ***************************************************************************** */
/* ***************************************************************************** */


void area_comp_opt_window_callback(Widget w, XtPointer client_data, 
                                                              XtPointer call_data)
{
    Widget d, label_radio_label, label_radiobox;
    Widget l1_radio, l2_radio, l3_radio, l4_radio;
    Widget symbol_radio_label, symbol_radiobox;
    Widget s0_radio, s1_radio, s2_radio, s3_radio, s4_radio;
    Widget line_radio_label, line_radiobox;
    Widget  i1_radio, i2_radio, i3_radio, i4_radio;
    Widget line_point_toggle;
    Widget form;
    
    Widget area_prev_frame;
       
    XmString oklabel;
    Arg al[20];
    Cardinal ac;   
 

  static int lbl_none = AREA_LBL_NONE;
  static int lbl_mne = AREA_LBL_MNEMONIC;
  static int lbl_num = AREA_LBL_COMP_NUM;
  static int lbl_both = AREA_LBL_BOTH;
   
  static int sym_point = AREA_SYM_POINT;            
  static int sym_circle = AREA_SYM_CIRCLE;
  static int sym_seg_cir = AREA_SYM_SEGMENTED_CIRCLE;
  static int sym_diamond = AREA_SYM_DIAMOND;
  static int sym_square = AREA_SYM_SQUARE;
  
  static int line_solid = AREA_LINE_SOLID;
  static int line_dash_clr = AREA_LINE_DASH_CLEAR;
  static int line_dash_blk = AREA_LINE_DASH_BLACK;
  static int line_dotted = AREA_LINE_DOTTED;
  
  /* here we make the dialog and ensure it has the buttons we need */
    oklabel = XmStringCreateLtoR("OK", XmFONTLIST_DEFAULT_TAG);
/*     cnxlabel = XmStringCreateLtoR("Cancel", XmFONTLIST_DEFAULT_TAG); */
/*     helplabel = XmStringCreateLtoR("Help", XmFONTLIST_DEFAULT_TAG);   */
  
    ac = 0;
    XtSetArg(al[ac], XmNokLabelString, oklabel);  ac++;	
/*     XtSetArg(al[ac], XmNcancelLabelString, cnxlabel);  ac++; */
/*     XtSetArg(al[ac], XmNhelpLabelString, helplabel);  ac++;   */
 
       XtSetArg(al[ac], XmNdialogType,    XmDIALOG_WORKING); ac++;
 
    d = XmCreateTemplateDialog(shell, "area_comp", al, ac);


    XtVaSetValues(XtParent(d), XmNtitle, "Area Component Display Options", NULL);

    XtVaSetValues(XtParent(d), 
        XmNmwmDecorations,    
             MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU, 
        XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,
        NULL); 

    XtAddCallback(d, XmNokCallback, area_ok_callback, NULL);
/*     XtAddCallback(d, XmNcancelCallback, ts_pref_cancel_callback, NULL);  */
/*     XtAddCallback(d, XmNhelpCallback, help_window_callback, helpfile); */

  form = XtVaCreateManagedWidget("areaform", xmFormWidgetClass, d,
			XmNresizable,  False,
			NULL);    
			
 /* --- BEGIN RADIO BOX --- */
  
  label_radio_label = XtVaCreateManagedWidget("Label Format: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  label_radiobox = XmCreateRadioBox(form, "labelselect", NULL, 0);

  XtVaSetValues(label_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        label_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       label_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);

  XtManageChild(label_radiobox);


  /* create the toggle buttons within the radio box */
 
  l1_radio = XtCreateManagedWidget("None", xmToggleButtonWidgetClass,
                   label_radiobox, 
                   NULL, 0);
  XtAddCallback(l1_radio, XmNvalueChangedCallback, a_label_radio_callback, 
                                                           (XtPointer) &lbl_none );
  
  l2_radio = XtCreateManagedWidget("Product Mnemonic", xmToggleButtonWidgetClass,
                   label_radiobox, 
                   NULL, 0);
  XtAddCallback(l2_radio, XmNvalueChangedCallback, a_label_radio_callback, 
                                                            (XtPointer) &lbl_mne );
  
  l3_radio = XtCreateManagedWidget("Component Number", xmToggleButtonWidgetClass,
                   label_radiobox, 
                   NULL, 0);
  XtAddCallback(l3_radio, XmNvalueChangedCallback, a_label_radio_callback, 
                                                            (XtPointer) &lbl_num );

  l4_radio = XtCreateManagedWidget("Both", xmToggleButtonWidgetClass,
                   label_radiobox, 
                   NULL, 0);
  XtAddCallback(l4_radio, XmNvalueChangedCallback, a_label_radio_callback, 
                                                           (XtPointer) &lbl_both );

  if(area_label==AREA_LBL_NONE)
      XtVaSetValues( l1_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_label==AREA_LBL_MNEMONIC)
      XtVaSetValues( l2_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_label==AREA_LBL_COMP_NUM)
      XtVaSetValues( l3_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_label==AREA_LBL_BOTH)
      XtVaSetValues( l4_radio,
          XmNset, TRUE,
          NULL);

  /* --- END RADIO BOX --- */



 /* --- BEGIN RADIO BOX --- */
  
  symbol_radio_label = XtVaCreateManagedWidget("Point Symbol: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       200,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  symbol_radiobox = XmCreateRadioBox(form, "symbolselect", NULL, 0);

  XtVaSetValues(symbol_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        symbol_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       symbol_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
  XtManageChild(symbol_radiobox);


  /* create the toggle buttons within the radio box */

  s0_radio = XtCreateManagedWidget("White Dot", xmToggleButtonWidgetClass,
                   symbol_radiobox, 
                   NULL, 0);
  XtAddCallback(s0_radio, XmNvalueChangedCallback, a_symbol_radio_callback, 
                                                        (XtPointer) &sym_point );
 
  s1_radio = XtCreateManagedWidget("Circle", xmToggleButtonWidgetClass,
                   symbol_radiobox, 
                   NULL, 0);
  XtAddCallback(s1_radio, XmNvalueChangedCallback, a_symbol_radio_callback, 
                                                       (XtPointer) &sym_circle );
  
  s2_radio = XtCreateManagedWidget("Segmented Circle", xmToggleButtonWidgetClass,
                   symbol_radiobox, 
                   NULL, 0);
  XtAddCallback(s2_radio, XmNvalueChangedCallback, a_symbol_radio_callback, 
                                                      (XtPointer) &sym_seg_cir );
  
  s3_radio = XtCreateManagedWidget("Diamond", xmToggleButtonWidgetClass,
                   symbol_radiobox, 
                   NULL, 0);
  XtAddCallback(s3_radio, XmNvalueChangedCallback, a_symbol_radio_callback, 
                                                      (XtPointer) &sym_diamond );

  s4_radio = XtCreateManagedWidget("Square", xmToggleButtonWidgetClass,
                   symbol_radiobox, 
                   NULL, 0);
  XtAddCallback(s4_radio, XmNvalueChangedCallback, a_symbol_radio_callback, 
                                                        (XtPointer) &sym_square );  

  if(area_symbol==AREA_SYM_POINT)
      XtVaSetValues( s0_radio,
          XmNset, TRUE,
          NULL); 
 else if(area_symbol==AREA_SYM_CIRCLE)
      XtVaSetValues( s1_radio,
          XmNset, TRUE,
          NULL);   
  else if(area_symbol==AREA_SYM_SEGMENTED_CIRCLE)
      XtVaSetValues( s2_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_symbol==AREA_SYM_DIAMOND)
      XtVaSetValues( s3_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_symbol==AREA_SYM_SQUARE)
      XtVaSetValues( s4_radio,
          XmNset, TRUE,
          NULL);

  /* --- END RADIO BOX --- */


 /* --- BEGIN RADIO BOX --- */
  
  line_radio_label = XtVaCreateManagedWidget("Line Format: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       400,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  line_radiobox = XmCreateRadioBox(form, "lineselect", NULL, 0);

  XtVaSetValues(line_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        line_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       line_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);

  XtManageChild(line_radiobox);


  /* create the toggle buttons within the radio box */
 
  i1_radio = XtCreateManagedWidget("Solid", xmToggleButtonWidgetClass,
                   line_radiobox, 
                   NULL, 0);
  XtAddCallback(i1_radio, XmNvalueChangedCallback, a_line_radio_callback, 
                                                     (XtPointer) &line_solid );
  
  i2_radio = XtCreateManagedWidget("Dashed (clear)", xmToggleButtonWidgetClass,
                   line_radiobox, 
                   NULL, 0);
  XtAddCallback(i2_radio, XmNvalueChangedCallback, a_line_radio_callback, 
                                                   (XtPointer) &line_dash_clr );

  i3_radio = XtCreateManagedWidget("Dashed (black)", xmToggleButtonWidgetClass,
                   line_radiobox, 
                   NULL, 0);
  XtAddCallback(i3_radio, XmNvalueChangedCallback, a_line_radio_callback, 
                                                   (XtPointer) &line_dash_blk );
  
  i4_radio = XtCreateManagedWidget("Dotted", xmToggleButtonWidgetClass,
                   line_radiobox, 
                   NULL, 0);
  XtAddCallback(i4_radio, XmNvalueChangedCallback, a_line_radio_callback, 
                                                     (XtPointer) &line_dotted );


  if(area_line_type==AREA_LINE_SOLID)
      XtVaSetValues( i1_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_line_type==AREA_LINE_DASH_CLEAR)
      XtVaSetValues( i2_radio,
          XmNset, TRUE,
          NULL);  
  else if(area_line_type==AREA_LINE_DASH_BLACK)
      XtVaSetValues( i3_radio,
          XmNset, TRUE,
          NULL); 
  else if(area_line_type==AREA_LINE_DOTTED)
      XtVaSetValues( i4_radio,
          XmNset, TRUE,
          NULL);  
 

  /* --- END RADIO BOX --- */



    line_point_toggle = XtVaCreateManagedWidget("Include Symbols with Lines",
       xmToggleButtonWidgetClass, form,
       XmNtopAttachment,       XmATTACH_WIDGET,
       XmNtopWidget,           line_radiobox,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_OPPOSITE_WIDGET,
       XmNleftWidget,           line_radiobox,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       NULL);

    XtAddCallback(line_point_toggle, XmNvalueChangedCallback, 
                                                 a_line_points_cb, NULL );

    if(include_points_flag==TRUE)
        XtVaSetValues( line_point_toggle,
          XmNset, TRUE,
          NULL);



  /* --- PREVIEW DISPLAY FRAME ------------*/
  
     area_prev_frame = XtVaCreateManagedWidget ("area_prevframe",
         xmFrameWidgetClass,             form,
         XmNwidth,                       400,
         XmNheight,                      100,
         XmNshadowType,                  XmSHADOW_IN,
         XmNtopAttachment,               XmATTACH_WIDGET,
         XmNtopWidget,                   line_point_toggle,
         XmNtopOffset,                   10,
         XmNleftAttachment,              XmATTACH_NONE,   
         XmNbottomAttachment,            XmATTACH_FORM,
         XmNrightAttachment,             XmATTACH_FORM,
         XmNrightOffset,                 100,
         NULL);
   
     area_prev_draw = XtVaCreateWidget("drawing_area",
         xmDrawingAreaWidgetClass, area_prev_frame,
         XmNwidth,                 400,
         XmNheight,                100,
         NULL);

    XtManageChild(area_prev_draw);
    XtAddCallback(area_prev_draw, XmNexposeCallback, area_prev_expose_cb, NULL);


    XtAddCallback(XtParent(d), XmNdestroyCallback, area_opt_kill_cb, NULL);


    XtManageChild(d);
    
     /*  NOTE: on Linux, the shell (parent of d) must be realized (managed) */
     /*        before attempting to create the pixmap */
        
     area_prev_pix = XCreatePixmap(XtDisplay(area_prev_draw), 
                   XtWindow(area_prev_draw), 400, 100, 
                   XDefaultDepthOfScreen(XtScreen(shell)));


    area_prev_grey_pixmap();
    display_area(0, PREFS_FRAME);
    area_prev_show_pixmap(); 
 
    
           
} /*  end area_comp_opt_callback */




void area_opt_kill_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    XFreePixmap(XtDisplay(area_prev_draw), area_prev_pix);


}






/* similar expose callback for the legend window */
void area_prev_expose_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
  

  area_prev_show_pixmap();


}





/* copies the offscreen copy of the legend into the viewable area */
void area_prev_show_pixmap()
{
  
  
      XCopyArea(XtDisplay(area_prev_draw), area_prev_pix, 
                   XtWindow(area_prev_draw), gc, 0, 0, 
                   400, 100, 0, 0);

}




void area_prev_clear_pixmap()
{
  /* clear the pixmap to black */
  

    XSetForeground(XtDisplay(area_prev_draw), gc, black_color);
    XFillRectangle(XtDisplay(area_prev_draw), area_prev_pix, gc, 0, 0, 
           400, 100); 


}



void area_prev_grey_pixmap()
{
  /* clear the pixmap to black */
  

    XSetForeground(XtDisplay(area_prev_draw), gc, grey_color);
    XFillRectangle(XtDisplay(area_prev_draw), area_prev_pix, gc, 0, 0, 
           400, 100); 


}




/* ***************************************************************************** */
/* ***************************************************************************** */

void table_comp_opt_window_callback(Widget w, XtPointer client_data, 
                                                               XtPointer call_data)
{
    ;
}
/*  FUTURE DEVELOPMENT */





/* ***************************************************************************** */
/* ***************************************************************************** */

void map_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, form; 
    
/* /// FOR FUTURE TESTING */
/* /    Widget MAP_rowcol; */
/* /    char    buf [256]; */

    Widget road_radio_label, road_radiobox;
    Widget rd1_radio, rd2_radio, rd3_radio;
    Widget rail_radio_label, rail_radiobox;
    Widget rr1_radio, rr2_radio;
    Widget county_radio_label, county_radiobox;
    Widget co1_radio, co2_radio;
    
       
    XmString oklabel;
    Arg al[20];
    Cardinal ac; 
    


    static int rd_none = ROAD_NONE;
    static int rd_major = ROAD_MAJOR;
    static int rd_more = ROAD_MORE;
    static int rr_none = RAIL_NONE;
    static int rr_major = RAIL_MAJOR;
    static int co_no = FALSE;
    static int co_yes = TRUE;
        
    /* here we make the dialog and ensure it has the buttons we need */
    oklabel = XmStringCreateLtoR("OK", XmFONTLIST_DEFAULT_TAG);
/* /    cnxlabel = XmStringCreateLtoR("Cancel", XmFONTLIST_DEFAULT_TAG); */
/* /    helplabel = XmStringCreateLtoR("Help", XmFONTLIST_DEFAULT_TAG);   */
  
    ac = 0;
    XtSetArg(al[ac], XmNokLabelString, oklabel);  ac++;	
/* /    XtSetArg(al[ac], XmNcancelLabelString, cnxlabel);  ac++; */
/* /    XtSetArg(al[ac], XmNhelpLabelString, helplabel);  ac++;   */
 
       XtSetArg(al[ac], XmNdialogType,    XmDIALOG_WORKING); ac++;
 
    d = XmCreateTemplateDialog(shell, "map_opt", al, ac);


    XtVaSetValues(XtParent(d), XmNtitle, "Map Display Options", NULL);

    XtVaSetValues(XtParent(d), 
        XmNmwmDecorations,    
              MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU, 
        XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,    
        NULL); 

    XtAddCallback(d, XmNokCallback, map_ok_callback, NULL);
/* /    XtAddCallback(d, XmNcancelCallback, ts_pref_cancel_callback, NULL);  */
/* /    XtAddCallback(d, XmNhelpCallback, help_window_callback, helpfile); */

    form = XtVaCreateManagedWidget("areaform", xmFormWidgetClass, d,
			XmNresizable,  False,
			NULL);    

 
  
  
/* --- ROAD RADIO BOX --- */
  
  road_radio_label = XtVaCreateManagedWidget("Highways Displayed: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  road_radiobox = XmCreateRadioBox(form, "roadselect", NULL, 0);

  XtVaSetValues(road_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        road_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       road_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
  XtManageChild(road_radiobox);


  /* create the toggle buttons within the radio box */
 
  rd1_radio = XtCreateManagedWidget("None", xmToggleButtonWidgetClass,
                   road_radiobox, 
                   NULL, 0);
  XtAddCallback(rd1_radio, XmNvalueChangedCallback, road_radio_callback, 
                                                       (XtPointer) &rd_none );
  
  rd2_radio = XtCreateManagedWidget("Major Only", xmToggleButtonWidgetClass,
                   road_radiobox, 
                   NULL, 0);
  XtAddCallback(rd2_radio, XmNvalueChangedCallback, road_radio_callback, 
                                                      (XtPointer) &rd_major );
  
  rd3_radio = XtCreateManagedWidget("More", xmToggleButtonWidgetClass,
                   road_radiobox, 
                   NULL, 0);
  XtAddCallback(rd3_radio, XmNvalueChangedCallback, road_radio_callback, 
                                                       (XtPointer) &rd_more );

 /*  SHOULD THESE BE ACTIVATION CALLBACKS????    */

  if(road_d==ROAD_NONE)
      XtVaSetValues( rd1_radio,
          XmNset, TRUE,
          NULL);  
  else if(road_d==ROAD_MAJOR)
      XtVaSetValues( rd2_radio,
          XmNset, TRUE,
          NULL);  
  else if(road_d==ROAD_MORE)
      XtVaSetValues( rd3_radio,
          XmNset, TRUE,
          NULL);  
  
  /* --- END RADIO BOX --- */
  
  

  
 /* --- RAIL RADIO BOX --- */
  
  rail_radio_label = XtVaCreateManagedWidget("Rail Roads Displayed: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       road_radio_label,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  rail_radiobox = XmCreateRadioBox(form, "railselect", NULL, 0);

  XtVaSetValues(rail_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        rail_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       rail_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
  XtManageChild(rail_radiobox);


  /* create the toggle buttons within the radio box */
 
  rr1_radio = XtCreateManagedWidget("None", xmToggleButtonWidgetClass,
                   rail_radiobox, 
                   NULL, 0);
  XtAddCallback(rr1_radio, XmNvalueChangedCallback, rail_radio_callback, 
                                                        (XtPointer) &rr_none );
  
  rr2_radio = XtCreateManagedWidget("Major", xmToggleButtonWidgetClass,
                   rail_radiobox, 
                   NULL, 0);
  XtAddCallback(rr2_radio, XmNvalueChangedCallback, rail_radio_callback, 
                                                       (XtPointer) &rr_major );
 
 /* CVG 9.0 */
 /*  SHOULD THESE BE ACTIVATION CALLBACKS????    */
   
   if(rail_d==RAIL_NONE)
      XtVaSetValues( rr1_radio,
          XmNset, TRUE,
          NULL);  
  else if(rail_d==RAIL_MAJOR)
      XtVaSetValues( rr2_radio,
          XmNset, TRUE,
          NULL);  

 
 
   
 /* --- COUNTY RADIO BOX --- */
  
  county_radio_label = XtVaCreateManagedWidget("County Lines Displayed: ", 
        xmLabelWidgetClass,  form,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        15,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       rail_radio_label,
        XmNleftOffset,       10,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  

  county_radiobox = XmCreateRadioBox(form, "countyselect", NULL, 0);

  XtVaSetValues(county_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopWidget,        county_radio_label,
        XmNtopOffset,        0,
        
/*  change when additional widgets are added         */
        XmNbottomAttachment, XmATTACH_NONE,        
        XmNleftAttachment,   XmATTACH_OPPOSITE_WIDGET,
        XmNleftWidget,       county_radio_label,
        XmNleftOffset,       8,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
 
  XtManageChild(county_radiobox);


  /* create the toggle buttons within the radio box */
 
  co1_radio = XtCreateManagedWidget("No", xmToggleButtonWidgetClass,
                   county_radiobox, 
                   NULL, 0);
  XtAddCallback(co1_radio, XmNvalueChangedCallback, county_radio_callback, 
                                                           (XtPointer) &co_no );
  
  co2_radio = XtCreateManagedWidget("Yes", xmToggleButtonWidgetClass,
                   county_radiobox, 
                   NULL, 0);
  XtAddCallback(co2_radio, XmNvalueChangedCallback, county_radio_callback, 
                                                         (XtPointer) &co_yes );

 

 /*  SHOULD THESE BE ACTIVATION CALLBACKS????    */

  if(co_d==FALSE)
      XtVaSetValues( co1_radio,
          XmNset, TRUE,
          NULL);  
  else if(co_d==TRUE)
      XtVaSetValues( co2_radio,
          XmNset, TRUE,
          NULL);  

  /* --- END RADIO BOX --- */
  
  

 
  

/* FOR FUTURE TESTING                                     */
/* MAP_rowcol = XtVaCreateWidget ("MAP_rowcol",           */
/*                xmRowColumnWidgetClass, form,           */
/*                XmNpacking,     XmPACK_COLUMN,          */
/*                XmNnumColumns,      2,                  */
/*                XmNorientation,     XmVERTICAL,         */
/*                XmNtopAttachment,    XmATTACH_WIDGET,   */
/*                XmNtopOffset,        10,                */
/*                  XmNtopWidget,       road_radiobox,    */
/*                XmNbottomAttachment, XmATTACH_NONE,     */
/*                XmNleftAttachment,   XmATTACH_FORM,     */
/*                XmNleftOffset,       0,                 */
/*                XmNrightAttachment,  XmATTACH_NONE,     */
/*                NULL);                                  */

    /* first column label widgets --------------------------------------- */
/* label_A = XtVaCreateManagedWidget (                                    */
/*                "Admin Detail: (97 All,  21 Least,  20 None)",          */
/*                xmLabelGadgetClass, MAP_rowcol,                         */
/*                NULL);                                                  */

    /* second column text widgets --------------------------------------- */
 
/*   text_A  = XtVaCreateManagedWidget ("admin_text",                     */
/*                xmTextWidgetClass,  MAP_rowcol,                         */
/*                NULL);                                                  */
/*  sprintf (buf,"%02d", admin_d);                                        */
/*  XmTextSetString (text_A, buf);                                        */
/*  XtAddCallback (text_A,                                                */
/*      XmNactivateCallback, admin_callback, ( XtPointer ) NULL);         */
/*  XtAddCallback (text_A,                                                */
/*      XmNlosingFocusCallback, admin_callback, ( XtPointer ) NULL);      */

/*  XtManageChild (MAP_rowcol);                                           */



    
    XtManageChild(d);
    
}





void map_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    if(screen_1 != NULL)
        replot_image(SCREEN_1);
    if(screen_2 != NULL)
        replot_image(SCREEN_2);
    if(screen_3 != NULL)
        replot_image(SCREEN_3);
    
    
    XtUnmanageChild(w);
}



/*   ======================================================================== */
/*   ======================================================================== */
  
  /*  eventually move to prefs_edit.c */
  
void road_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *road_selected = (int *)client_data;  

    road_d = *road_selected;

}



void rail_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *rail_selected = (int *)client_data;  

    rail_d = *rail_selected;

}



void county_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *county_selected = (int *)client_data;  

    co_d = *county_selected;

}


/* FOR FUTURE TESTING  ----------------------------------------------------   */
/* void admin_callback (Widget w, XtPointer client_data, XtPointer call_data) */
/* {                                                                          */

/*     set_admin();                                                           */

/* }                                                                          */

/* void set_admin()                                                           */
/* {                                                                          */
/*     char    *text;                                                         */

/*     text = XmTextGetString (text_A);                                       */
/*     sscanf (text,"%d",&admin_d);                                           */
/*     XtFree (text);                                                         */
/* TEMP     */
/*     if ( (admin_d > 97) || (admin_d < 21) ) { */
/*        fprintf(stderr,"Admin Detail: %d is not within limits (21 to 97)\n",admin_d); */
/*        XmTextSetString(text_A, "Not within limits (21 to 97)");            */
/*        bad_val = TRUE;                                                     */
/*     }                                                                      */

/* }                                                                          */




