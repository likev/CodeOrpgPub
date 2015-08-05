/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 16:42:19 $
 * $Id: map_cvg.c,v 1.4 2014/03/18 16:42:19 jeffs Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/************************************************************************
 *                                  *
 *  Module: map_cvg.c - This module provides a GUI to extract  *
 *      regions/elements from the USGS data file.           *
 *                                  *
 *
 *  The previous CVG map program "map" from map.c created multiple
 *  map files each containing a category of vectors (stream perennial,
 *  county boarders, etc.  The first version of map_cvg works on a single
 *  source file already containing the desired features.
 *
 *  map_cvg addes a command line version that can be driven by a script to
 *  automate map creation.
 *
 *  The file decode.c creates a single reagion file from several region
 *  files each containing a type of data.  Contatenation scripts are used
 *  to create input data files for map_cvg, one for the contiguous 48 states,
 *  one for Alaska, and one for Hawaii.
 *
 *  The number of features contained in the USGS data cd files has been
 *  reduced by decode.c to remove unneeded features including: many small
 *  lakes in water bodies and other intermittent features, all but major 
 *  rivers in streams, and minor highways.and ferry routs in roads
 ************************************************************************/
 
 /*  TO DO */
 
/*  PROBLEMS */

/*  EHNANCEMENTS */




#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>
#include <Xm/Form.h>

#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleB.h>


#include "global.h" 

/* #define _GNU_SOURCE */

#define PI 3.141592654
#define CKM_TO_NM  0.53996

#define STD_RANGE 250


XColor  color, unused;
Pixel   bg_color;
Pixel   button_color;
Colormap    cmap;
/* /int Switch=0; */


/*  global parameters */
char    infile [256];
char    outfile [128];
char    radID[5];
float   radar_lat;
float   radar_long; 
int     range = STD_RANGE;
int     use_rad_ID = TRUE;
int     bad_val = FALSE;

/*  other globals could be made local */
FILE    *fd_in;


/*  global widgets */
Widget  label_I, label_D, label_LA, label_LO, label_R, label_M;
Widget  text_I, text_D, text_LA, text_LO, text_R, text_M;
Widget create_button;

XmString readyXmstr, workingXmstr;

/*  prototypes */
void option1_callback (Widget w, XtPointer client_data, XtPointer call_data);
void option2_callback (Widget w, XtPointer client_data, XtPointer call_data);

void outfile_callback (Widget w, XtPointer client_data, XtPointer call_data);
void set_outfile();

void input_filename_callback (Widget w, XtPointer client_data, XtPointer call_data);
void set_input_filename();

void radID_callback (Widget w, XtPointer client_data, XtPointer call_data);
void set_radID();

    void    radar_lat_callback ();
void set_radar_lat();

    void    radar_long_callback ();
void set_radar_long();

    void    range_callback ();
void set_range();

void create_maps_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* /    void    switch_callback (); */
void quit_callback (Widget w, XtPointer client_data, XtPointer call_data);

void    CommandLine(int argc, char **argv);
int     check_args(int argc,char *argv[],char *param,int startval);
int     lookup_lat_long();
int     check_for_directory_existence(char *dirname);

void    Usage();
void    BuildMap();
void    ProcessVectors();
/*  COULD ELIMINATE THIS: */
FILE    *OpenInput(char *infile);



/* ///////////////////////////////////////////////////////////////// */
int main (int argc,char    **argv)
{
    
char *charval=NULL;
char map_data[255]; /*  same as CVG's map_dir */

    Widget  MAP_widget;
    
    Widget option_radio_label, option_radiobox, o1_radio, o2_radio;
    
    Widget  MAP_rowcol, MAP_rowcol2;
    Widget  rowcol;
    Widget  button;

/* /    Widget  toggle_rowcol = NULL; */
    Widget  form1;
    Widget  form2 = NULL;
    Widget  form3;
    XtAppContext    control_app;
    ArgList args=NULL;

struct stat comprfile_stat;
char testfile[255];
char uncompress_cmd[265];
int ret;

char    buf [256];
int     i;


    /*  initialize radar ID */
    for(i=0; i<4; i++) {
        radID[i] = ' ';
    }
    radID[4] = '\0';


    /* If argc > 1 then call the command line function  */
    /*                  and exit before widget creation */
    if (argc> 1) {
        
        CommandLine(argc, argv);

        BuildMap();
        
        exit(0);
    }



    /*  set the input data file to the us file in default location */
/*     charval = getenv("HOME"); */
    if((getenv("CVG_DEF_PREF_DIR")) == NULL) {
        sprintf(infile,  "/usr/local/share/cvg_map/us_map.dat");
        sprintf(map_data,"/usr/local/share/cvg_map");
        
    } else { 
        charval = getenv("CVG_DEF_PREF_DIR");
        
        /* variable is set, create default map path */
/*         sprintf(infile, "%s/tools/cvg_map/us_map.dat", charval); */
/*         sprintf(map_data, "%s/tools/cvg_map" , charval); */
        sprintf(infile,  "%s/cvg_map/us_map.dat", charval);
        sprintf(map_data,"%s/cvg_map" , charval);
       
    } /*  end else not NULL  */
    
    /*  ensure ~/tools/cvg_map directory exists */
    if(check_for_directory_existence(map_data) == FALSE) {
        /* tell user if directory does not exist */
        fprintf(stderr,"NOTE:  The cvg map data files are not installed \n");
        fprintf(stderr,"       in their standard location: %s/\n",map_data);
    }


    /* ensure compressed data files were uncompressed */
    sprintf(testfile, "%s/us_map.dat.bz2", map_data);
    if(stat(testfile, &comprfile_stat) == 0) { /*  still compressed */
        sprintf(uncompress_cmd, "bunzip2 %s", testfile);
        ret = system(uncompress_cmd);
        if(ret<0) {
            fprintf(stderr, "ERROR decompressing %s\n", testfile);
        }
    }
    
    sprintf(testfile, "%s/ak_map.dat.bz2", map_data);
    if(stat(testfile, &comprfile_stat) == 0) { /*  still compressed */
        sprintf(uncompress_cmd, "bunzip2 %s", testfile);
        ret = system(uncompress_cmd);
        if(ret<0) {
            fprintf(stderr, "ERROR decompressing %s\n", testfile);
        }
    }
    
    sprintf(testfile, "%s/hi_map.dat.bz2", map_data);
    if(stat(testfile, &comprfile_stat) == 0) { /*  still compressed */
        sprintf(uncompress_cmd, "bunzip2 %s", testfile);
        ret = system(uncompress_cmd);
        if(ret<0) {
            fprintf(stderr, "ERROR decompressing %s\n", testfile);
        }
    }



/*  Initialize Xt toolkit and create control window widget
 */

    XtSetLanguageProc (NULL, (XtLanguageProc)NULL, NULL);

    MAP_widget = XtAppInitialize (&control_app, "Map Generator",
        NULL, 0, &argc, argv, NULL, args, 0);

    rowcol = XtVaCreateWidget ("MAP_row$Gcol",
        xmRowColumnWidgetClass, MAP_widget,
        XmNpacking,     XmPACK_TIGHT,
        XmNnumColumns,      1,
        XmNorientation,     XmVERTICAL,
        NULL);

    form1 = XtVaCreateManagedWidget ("form1",
        xmFormWidgetClass,  rowcol,
        NULL);
        
        
        
    /*  radiobox here to select mode         */
   option_radio_label = XtVaCreateManagedWidget("Option:", 
        xmLabelWidgetClass,  form1,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        10,
        XmNbottomAttachment, XmATTACH_NONE,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       5,
        XmNrightAttachment,  XmATTACH_NONE,          
        NULL);  
  
   option_radiobox = XmCreateRadioBox(form1, "optionselect", NULL, 0);

   XtVaSetValues(option_radiobox,
        XmNorientation,      XmVERTICAL,
        XmNpacking,          XmPACK_TIGHT,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNtopOffset,        5,
        XmNbottomAttachment, XmATTACH_NONE,     
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       option_radio_label,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE,
        NULL);
   XtManageChild(option_radiobox);


   /* create the toggle buttons within the radio box */
  

        
   o1_radio = XtVaCreateManagedWidget(
                     "Use Radar ID to set radar Lat/Long, range, & map filename", 
                     xmToggleButtonWidgetClass, option_radiobox, 
                     XmNset, TRUE, 
                     NULL);
/* CVG 8.2  XtAddCallback(o1_radio, XmNvalueChangedCallback, option1_callback, NULL); */
   XtAddCallback(o1_radio, XmNarmCallback, option1_callback, NULL);
           

   o2_radio = XtVaCreateManagedWidget(
                     "Manually enter Lat/Long, range, & map filename", 
                     xmToggleButtonWidgetClass, option_radiobox, 
/*                      XmNset, True, */
                     NULL);
/* CVG 8.2  XtAddCallback(o2_radio, XmNvalueChangedCallback, option2_callback, NULL); */
   XtAddCallback(o2_radio, XmNarmCallback, option2_callback, NULL);       
   

    /*  a first second level rowcol ----------------- */
    MAP_rowcol = XtVaCreateWidget ("MAP_rowcol",
        xmRowColumnWidgetClass, form1,
        XmNpacking,     XmPACK_COLUMN,
        XmNnumColumns,      2,
        XmNorientation,     XmVERTICAL,
        XmNtopAttachment,    XmATTACH_WIDGET,
        XmNtopOffset,        10,
        XmNtopWidget,       option_radiobox,
        XmNbottomAttachment, XmATTACH_NONE,     
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,       0,
        XmNrightAttachment,  XmATTACH_NONE,
        
        NULL);

    XtVaGetValues (MAP_rowcol, XmNcolormap, &cmap, NULL);
    XAllocNamedColor (XtDisplay (MAP_rowcol), cmap, "SteelBlue", &color, &unused);
    bg_color = color.pixel;

    XtVaSetValues (MAP_rowcol,
        XmNbackground,      bg_color,
        NULL);

    XAllocNamedColor (XtDisplay (MAP_rowcol), cmap, "white", &color, &unused);
    button_color = color.pixel;

/*
 *  Define label widgets now - Column 1 of MAP_rowcol contains the labels
 */

/* L1 */
    label_I = XtVaCreateManagedWidget ("CODE Source Data Filename: (e.g., us_map.dat)",
        xmLabelGadgetClass, MAP_rowcol,
        NULL);

/* L2 */
        label_D = XtVaCreateManagedWidget ("Radar 4 Letter ID: (e.g., kmlb)",
                xmLabelGadgetClass,     MAP_rowcol,
                NULL);

/* L3 */
    label_LA = XtVaCreateManagedWidget ("Radar Latitude: thousands of a deg (NN.NNN)",
        xmLabelGadgetClass, MAP_rowcol,
        NULL);

/* L4 */
    label_LO = XtVaCreateManagedWidget ("Radar Longtitude: thousands of a deg (-NN.NNN)",
        xmLabelGadgetClass, MAP_rowcol,
        NULL);
        
/* L5 */
    label_R = XtVaCreateManagedWidget ("Desired Map Range: (250 NM recommended)",
        xmLabelGadgetClass, MAP_rowcol,
        NULL);

/* L6 */
 label_M = XtVaCreateManagedWidget ("Output Map Filename:",
        xmLabelGadgetClass, MAP_rowcol,
        NULL);


/*
 *  Define text widgets now - Column 2 of MAP_rowcol contains the text boxes
 */

/* T1 */
    text_I  = XtVaCreateManagedWidget ("input_text",
        xmTextWidgetClass,  MAP_rowcol,
        NULL);
    XmTextSetString (text_I, infile);
    XtAddCallback (text_I,
        XmNactivateCallback, input_filename_callback, ( XtPointer ) NULL);
    XtAddCallback (text_I,
        XmNlosingFocusCallback, input_filename_callback, ( XtPointer ) NULL);

/* T2 */
    text_D  = XtVaCreateManagedWidget ("radID_text",
            xmTextWidgetClass,      MAP_rowcol,
            NULL);
    XmTextSetString (text_D, ""); 
    XtAddCallback (text_D,
            XmNactivateCallback, radID_callback, ( XtPointer ) NULL);
    XtAddCallback (text_D,
            XmNlosingFocusCallback, radID_callback, ( XtPointer ) NULL);

/* T3 */
    text_LA  = XtVaCreateManagedWidget ("Lat_text",
        xmTextWidgetClass,  MAP_rowcol,
        NULL);
    XmTextSetString (text_LA, "");
    XtAddCallback (text_LA,
        XmNactivateCallback, radar_lat_callback, ( XtPointer ) NULL);
    XtAddCallback (text_LA,
        XmNlosingFocusCallback, radar_lat_callback, ( XtPointer ) NULL);

/* T4 */
    text_LO  = XtVaCreateManagedWidget ("Long_text",
        xmTextWidgetClass,  MAP_rowcol,
        NULL);
    XmTextSetString (text_LO, "");
    XtAddCallback (text_LO,
        XmNactivateCallback, radar_long_callback, ( XtPointer ) NULL);
    XtAddCallback (text_LO,
        XmNlosingFocusCallback, radar_long_callback, ( XtPointer ) NULL);

/* T5 */
    text_R  = XtVaCreateManagedWidget ("range_text",
        xmTextWidgetClass,  MAP_rowcol,
        NULL);
    sprintf (buf,"%03d", range);
    XmTextSetString (text_R, buf);
    XtAddCallback (text_R,
        XmNactivateCallback, range_callback, ( XtPointer ) NULL);
    XtAddCallback (text_R,
        XmNlosingFocusCallback, range_callback, ( XtPointer ) NULL);

/* T6 */
    text_M  = XtVaCreateManagedWidget ("output_text",
        xmTextWidgetClass,  MAP_rowcol,
        NULL);
    XmTextSetString (text_M, "");
    XtAddCallback (text_M,
        XmNactivateCallback, outfile_callback, ( XtPointer ) NULL);
    XtAddCallback (text_M,
        XmNlosingFocusCallback, outfile_callback, ( XtPointer ) NULL);
        
    /*  the first second-level rowcol ----------------- */
    XtManageChild (MAP_rowcol);


  /* using radar id is the default mode */
  use_rad_ID = TRUE;
  XtSetSensitive(label_D, True);
  XtSetSensitive(text_D, True);
  XmTextSetString (text_LA, "");
  XtSetSensitive(label_LA, False);
  XtSetSensitive(text_LA, False);
  XmTextSetString (text_LO, "");  
  XtSetSensitive(label_LO, False);
  XtSetSensitive(text_LO, False);
  sprintf (buf,"%03d", STD_RANGE);
  XmTextSetString (text_R, buf); 
  XtSetSensitive(label_R, False);
  XtSetSensitive(text_R, False);
  XmTextSetString (text_M, "");  
  XtSetSensitive(label_M, False);
  XtSetSensitive(text_M, False);

  /* manually entering custom parameters is default mode */
  /*   use_rad_ID = FALSE; */
  /*   XmTextSetString (text_D, "");    */
  /*   XtSetSensitive(label_D, False); */
  /*   XtSetSensitive(text_D, False); */
  /*   XtSetSensitive(label_LA, True); */
  /*   XtSetSensitive(text_LA, True); */
  /*   XtSetSensitive(label_LO, True); */
  /*   XtSetSensitive(text_LO, True); */
  /*   XtSetSensitive(label_R, True); */
  /*   XtSetSensitive(text_R, True); */
  /*   XtSetSensitive(label_M, True); */
  /*   XtSetSensitive(text_M, True); */
  


    XtVaCreateManagedWidget ("_sep",
        xmSeparatorGadgetClass,rowcol,
        NULL);



    form3 = XtVaCreateManagedWidget ("form3",
        xmFormWidgetClass,  rowcol,
        XmNtopWidget,       form2,
        XmNtopAttachment,   XmATTACH_WIDGET,
        NULL);


        readyXmstr   = XmStringCreateLtoR("  Create Maps  ", XmFONTLIST_DEFAULT_TAG);
        workingXmstr = XmStringCreateLtoR(" - WAIT - ", XmFONTLIST_DEFAULT_TAG);
        
    /*  another second-level rowcol ----------------- */
    MAP_rowcol2 = XtVaCreateWidget ("MAP_rowcol",
        xmRowColumnWidgetClass, form3,
        XmNpacking,     XmPACK_COLUMN,
        XmNnumColumns,      2,
        XmNorientation,     XmVERTICAL,
        NULL);

    create_button = XtVaCreateManagedWidget ("Create Maps",
        xmPushButtonWidgetClass,    MAP_rowcol2, 
        XmNmultiClick,              XmMULTICLICK_DISCARD,
        XmNfillOnArm,               True,
        XmNarmColor,                bg_color,
        XmNlabelString,             readyXmstr,
        NULL);
    XtAddCallback (create_button,
        XmNactivateCallback, create_maps_callback, ( XtPointer ) NULL);

    button = XtVaCreateManagedWidget ("   Quit   ",
        xmPushButtonWidgetClass,    MAP_rowcol2,        
        NULL);
    XtAddCallback (button,
        XmNactivateCallback, quit_callback, ( XtPointer ) NULL);

    XtManageChild (MAP_rowcol2);

    XtManageChild (rowcol);
    
    XtRealizeWidget (MAP_widget);

    XtAppMainLoop (control_app);

    exit (0);

}  /*  end main() */




/* /////////////////////////////////////////////////////////// */
void quit_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
    
XmPushButtonCallbackStruct  *cbs = (XmPushButtonCallbackStruct*)call_data;

    if (cbs->reason == XmCR_ACTIVATE) {

        printf ("QUIT pushed\n");
        exit (0);

    }

} /*  quit_callback() */






void option1_callback(Widget w, XtPointer client_data, 
                                   XtPointer call_data)
{

char buf[4];

  use_rad_ID = TRUE;

  XtSetSensitive(label_D, True);
  XtSetSensitive(text_D, True);

  XmTextSetString (text_LA, "");
  XtSetSensitive(label_LA, False);
  XtSetSensitive(text_LA, False);

  XmTextSetString (text_LO, "");  
  XtSetSensitive(label_LO, False);
  XtSetSensitive(text_LO, False);
  
  sprintf (buf,"%03d", STD_RANGE);
  XmTextSetString (text_R, buf); 
  XtSetSensitive(label_R, False);
  XtSetSensitive(text_R, False);

  XmTextSetString (text_M, "");  
  XtSetSensitive(label_M, False);
  XtSetSensitive(text_M, False);

}




void option2_callback(Widget w, XtPointer client_data, 
                                   XtPointer call_data)
{
  use_rad_ID = FALSE;
    
  XmTextSetString (text_D, "");   
  XtSetSensitive(label_D, False);
  XtSetSensitive(text_D, False);
  
  XtSetSensitive(label_LA, True);
  XtSetSensitive(text_LA, True);

  XtSetSensitive(label_LO, True);
  XtSetSensitive(text_LO, True);

  XtSetSensitive(label_R, True);
  XtSetSensitive(text_R, True);

  XtSetSensitive(label_M, True);
  XtSetSensitive(text_M, True);

}








/* ////////////////////////////////////////////////////////// */
void input_filename_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
    
/* XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct*)call_data; */

   set_input_filename();

}  /*  end input_filename_callback() */

/*  FUTURE IMPROVEMENT */
/*  remove trailing spaces from filename */
void set_input_filename()
{
char    *text;

    text = XmTextGetString (text_I);
    strcpy (infile, text);
    XtFree (text);
    
} /*  end set_input_filename() */



/*  if radar_ID used, this is a shortcut to (1) look up the radar */
/*             Lat-Long, (2) use a standard map output file name, and */
/*             (3) use a standard range of 250 miles */
/*  FUTURE IMPROVEMENT */
/*  remove trailing spaces from radar ID */
/* ///////////////////////////////////////////////////////////// */
void radID_callback (Widget w, XtPointer client_data, XtPointer call_data)
{

/* XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct*)call_data; */

    set_radID();

}  /*  end radID_callback() */

void set_radID()
{
char    *text;
char    *ch_ptr;
int lookup_error;

    text = XmTextGetString (text_D);
/* DEBUG */
/* fprintf(stderr,"DEBUG - setting radar ID to '%s'\n", text);  */
    strcpy (radID, text);
    XtFree (text);
    
    if(strlen(radID) != 4) {
        fprintf(stderr,"Error - Radar ID must be 4 character string\n");
        XmTextSetString(text_D, "ERROR - Radar ID must be 4 character string");
        bad_val = TRUE;
        return;
    }
    
    /* as a convenience, change upper case entry to lower case */
    for(ch_ptr = radID; *ch_ptr != '\0'; ch_ptr++)
        *ch_ptr = tolower(*ch_ptr);
    XmTextSetString(text_D, radID);

/* DEBUG */
/* fprintf(stderr, "DEBUG - radID is '%s'\n", radID); */
        
    /*  set range and output map filename */
    range = STD_RANGE;
/*  SET MAPFILE NAME  */
#ifdef LITTLE_ENDIAN_MACHINE
        sprintf(outfile, "%s_cvg%d_lnux.map",radID, range); 
#endif
#ifndef LITTLE_ENDIAN_MACHINE
        sprintf(outfile, "%s_cvg%d_slrs.map",radID, range);
#endif

/* DEBUG */
/* fprintf(stderr, "DEBUG - outfile is '%s'\n", outfile); */
    
    /*  lookup and set radar Lat Long  */
    lookup_error = lookup_lat_long();

    if(lookup_error == TRUE) {    
        fprintf(stderr,"Error looking up site data for ID '%s'\n",radID);
        XmTextSetString(text_D, "ERROR - bad ID or problem with site data file");
        bad_val = TRUE;
    }     
    
    
} /*  end set_rad_ID() */


/* ////////////////////////////////////////////////////////////// */
void
radar_lat_callback (w, data, cbs)
/*  TO DO:  THESE SHOULD BE DEFINED IN A PROTOTYPE, NOT HERE!!!! */
Widget      w;
XtPointer   data;
XmAnyCallbackStruct *cbs;
{

    set_radar_lat();

} /*  end radar_lat_callback() */

void set_radar_lat()
{
    char    *text;

    text = XmTextGetString (text_LA);
/* DEBUG */
/* fprintf(stderr,"DEBUG - setting latitude to '%s'\n", text);  */
    sscanf (text,"%f",&radar_lat);
    XtFree (text);
    
    if ( (radar_lat < 15.0) || (radar_lat > 70.0) ) {
        fprintf(stderr,"Illegal Radar Latitude Specified: %f\n",radar_lat);
        XmTextSetString(text_LA, "Not within limits (15.0 to 70.0)");
        bad_val = TRUE;
    }    
    
} /*  end set_radar_lat() */



/* /////////////////////////////////////////////////////////// */
void
radar_long_callback (w, data, cbs)
/*  TO DO:  THESE SHOULD BE DEFINED IN A PROTOTYPE, NOT HERE!!!! */
Widget      w;
XtPointer   data;
XmAnyCallbackStruct *cbs;
{

    set_radar_long();

} /*  end radar_long_callback() */

void set_radar_long()
{
    char    *text;

    text = XmTextGetString (text_LO);
/* DEBUG */
/* fprintf(stderr,"DEBUG - setting longitude to '%s'\n", text);  */
    sscanf (text,"%f",&radar_long);
    XtFree (text);
    
    if ( (radar_long > -65.0) || (radar_long < -170.0) ) {
        fprintf(stderr,"Illegal Radar Longitude Specified: %f\n",radar_long);
        XmTextSetString(text_LO, "Not within limits (-65.0 to -170.0)");
        bad_val = TRUE;
    }    
    
} /*  end set_radar_long() */



/* /////////////////////////////////////////////////////////// */
void
range_callback (w, data, cbs)
/*  TO DO:  THESE SHOULD BE DEFINED IN A PROTOTYPE, NOT HERE!!!! */
Widget      w;
XtPointer   data;
XmAnyCallbackStruct *cbs;
{

    set_range();

} /*  end range_callback() */

void set_range()
{
    char    *text;

    text = XmTextGetString (text_R);
/* DEBUG */
/* fprintf(stderr,"DEBUG - setting range to '%s'\n", text);  */
    sscanf (text,"%d",&range);
    XtFree (text);
    
    if ( (range > 500) || (range < 0) ) {
        fprintf(stderr,"Map Range: %d is not within limits (0 to 500)\n",range);
        XmTextSetString(text_R, "Not within limits (0 to 500)");
        bad_val = TRUE;
    }    
    
} /*  end set_range() */


/*  FUTURE IMPROVEMENT */
/*  remove trailing spaces from filename */
/* ///////////////////////////////////////////////////////////// */
void outfile_callback (Widget w, XtPointer client_data, XtPointer call_data)
{

/* XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct*)call_data; */

    set_outfile();

} /*  end outfile_callback() */

void set_outfile()
{
    char    *text;

    text = XmTextGetString (text_M);
/* DEBUG */
/* fprintf(stderr,"DEBUG - setting output filename to '%s'\n", text);     */
    strcpy (outfile, text);
    XtFree (text);    
    
    
}






/*  this is the interactive version */
/*  if the main function is called with no arguments */
/*  this function is executed when clicking the Create Maps button */
/*  calls ProcessVectors()  */
/* ///////////////////////////////////////////////////////////// */
void create_maps_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

/* XmPushButtonCallbackStruct  *cbs = (XmPushButtonCallbackStruct*)call_data; */
char text_buf[128];

    /*  debug */
    /* fprintf(stderr,"DEBUG entering create_map_callback()\n"); */

    /*  ensures all appropriate text widgets are read before building map */
    if(use_rad_ID == FALSE)  {
       set_input_filename();
       set_radar_lat();
       set_radar_long();
       set_range();
       set_outfile();
        
    } else {
       set_input_filename();
       set_radID();
        
    }

    /*  check presence of the input data file */
    if ((fd_in  = fopen(infile, "r")) == (FILE *) NULL) {
       fprintf (stderr,"ERROR: unable to open '%s'\n", infile);
       sprintf (text_buf,"Unable to open '%s'", infile);
       XmTextSetString (text_I, text_buf);
       bad_val = TRUE;
    }
    
    /*  if any of the values are not within range, do nothing! */
    /*  note: ability to create the output file is not checked */
    if(bad_val == TRUE) {
        bad_val = FALSE;
        if(fd_in != NULL)
            fclose(fd_in);
        return;
    }

/*  does not work */
/*     XtSetSensitive(create_button, False); */
    XtVaSetValues(create_button,
/*          XmNsensitive,     False, */
         XmNlabelString,   workingXmstr,
         NULL);
    
    ProcessVectors();
    
/*     XtSetSensitive(create_button, True);  */
    XtVaSetValues(create_button,
/*          XmNsensitive,     True, */
         XmNlabelString,   readyXmstr,
         NULL);
    
    if(fd_in != NULL)
        fclose(fd_in);


fprintf (stderr,"Finished create_map_callback()\n");


} /*  end create_maps_callback() */




/* //////////////////////////////////////////////////////////////////////////////////// */
void CommandLine(int argc, char **argv)

{

  int value;
  int input_error = FALSE;
  char *ch_ptr;


    /* get Input data file name */
    value=check_args(argc,argv,"-I",0);
    if(value <= 0) {
        fprintf(stderr,"ERROR, parameter '-I' for Input data filename is missing\n");
        input_error = TRUE;
        
    } else { /*  get and set input data filename */
        strcpy(infile, argv[value+1]);
    }

/*  debug */
/* fprintf(stderr,"DEBUG CL - input file is %s\n", infile); */

    /* get radar iD */
    /*  this is a shortcut to (1) look up the radar Lat-Long,  */
    /*                        (2) use a standard map output file name, and */
    /*                        (3) use a standard range of 250 miles */
    value=check_args(argc,argv,"-D",0);
    
    if(value <= 0) { /*  MODE IS NOT USING RADAR ID ----------------------- */
        
        
        /*  initialize radar ID */
        radID[0] = '\0'; /*  empty string */
    
        /*  look for radar Lat Long, map filename, and range */
        
        /* get Map output file name */
        value=check_args(argc,argv,"-M",0);
        if(value <= 0) {
            fprintf(stderr,"ERROR, parameter '-M' for Map output filename is missing\n");
            input_error = TRUE;
            
        } else { /*  get and set Map output filename */
            strcpy(outfile, argv[value+1]);
        }    
        
        /* get radar LAtitude */
        value=check_args(argc,argv,"-LA",0);
        if(value <= 0) {
            fprintf(stderr,"ERROR, parameter '-LA' for radar LAtitude is missing\n");
            input_error = TRUE;
            
        } else { /*  get and set radar LAtitude */
            radar_lat = atof(argv[value+1]);
            if ( (radar_lat < 5.0) || (radar_lat > 85.0) ) {
                fprintf(stderr,"Illegal Radar Latitude Specified: %f\n",radar_lat);
                input_error = TRUE;
            }
        }  
    
        /* get radar LOngitude*/
        value=check_args(argc,argv,"-LO",0);
        if(value <= 0) {
            fprintf(stderr,"ERROR, parameter '-LO' for radar LOngitude is missing\n");
            input_error = TRUE;
            
        } else { /*  get and set radar LOngitude */
            radar_long = atof(argv[value+1]);
            if ( (radar_long > -10.0) || (radar_long < -170.0) ) {
                fprintf(stderr,"Illegal Radar Longitude Specified: %f\n",radar_long);
                input_error = TRUE;
            }
        }  
    
        /* get desired map Range*/
        value=check_args(argc,argv,"-R",0);
        if(value <= 0) {
            fprintf(stderr,"ERROR, parameter '-R' for desired map Range is missing\n");
            input_error = TRUE;
            
        } else { /*  get and set map Range */
            range = atoi(argv[value+1]);
            if ( (range > 500) || (range < 0) ) {
                fprintf(stderr,"Map Range: %d is not within limits (0 - 500)\n",range);
                input_error = TRUE;
            }
        }  
            

    } else { /*  MODE IS USING RADAR ID ----------------------------------- */
        strcpy(radID, argv[value+1]);

        /* as a convenience, change upper case entry to lower case */
        for(ch_ptr = radID; *ch_ptr != '\0'; ch_ptr++)
            *ch_ptr = tolower(*ch_ptr);
        
        /*  set range and output map filename */
        range = STD_RANGE;
/*  SET MAPFILE NAME */
#ifdef LITTLE_ENDIAN_MACHINE
        sprintf(outfile, "%s_cvg%d_lnux.map",radID, range); 
#endif
#ifndef LITTLE_ENDIAN_MACHINE
        sprintf(outfile, "%s_cvg%d_slrs.map",radID, range);
#endif        
        /*  lookup and set radar Lat Long  */
        input_error = lookup_lat_long();
        
            
        
    } /*  end MODE USING RADAR ID ----------------------------------- */



    if(input_error == TRUE) {
        Usage();
        exit(-1);
    }

    
    return;

} /*  end CommandLine() */





int check_args(int argc,char *argv[],char *param,int startval) {
  /* check the command line arguments for input parameters.
     If the parameter IS in the list, then return the positive
     index of the parameter, otherwise return 0 */
  int TEST=FALSE;
  int i;

    if(TEST)
        fprintf(stderr,"inside check_args: input string=%s  strlen=%i st=%i end=%i\n",
	             param,strlen(param),startval,argc);

    for(i=startval;i<argc;i++) {

        if(strcmp(argv[i],param)==0) {
	        if(TEST) 
	            fprintf(stderr,"return double i=%i\n",i);
	        return(i);
        }
        
    } /* end i loop */
    
    return(FALSE);
    
} /*  end check_args() */





int lookup_lat_long() 
{
  FILE *site_fd;
  char site_data_file[255];
  
  char cfg_dir[128]; /*  CONSTRAINT: must be the same as CVG's config_dir */
  char *charval;
  char data[128], temp_str[64];
  int  len, lat_start, long_start;
  
  char icao[6];
  float rlat=0.0, rlong=0.0;
  
    /*  lookup and set radar Lat Long  */
    charval = getenv("HOME");
  
    if(charval == NULL) { /* variable not set, default is blank */
        printf("ERROR: (map_cvg) environmental variable for HOME is not set\n");
        return (TRUE);
    } else { /* variable is set */
/*          sprintf(site_data_file, "%s/tools/data/change_radar.dat", charval); */
/*          sprintf(cfg_dir, "%s/tools/data" , charval); */
        sprintf(cfg_dir, "%s/.%s", charval,CVG_PREF_DIR_NAME);
        sprintf(site_data_file, "%s/rdr_loc.dat", cfg_dir);
         /*  ensure ~/tools/data directory exists */
         if(check_for_directory_existence(cfg_dir) == FALSE) {
             /* tell user if file does not exist */
             fprintf(stderr,"NOTE: (map_cvg) The default location for site data, the \n");
             fprintf(stderr,"      ~/cvgN.N directory, does not exist.\n");
             return (TRUE);
         }
    }
    
    
    if ((site_fd  = fopen(site_data_file, "r")) == (FILE *) NULL) {
        fprintf (stderr,"ERROR: unable to open %s\n", site_data_file);
        return (TRUE);
    }
  
  
    while(1) {        
        if( fgets(data,61,site_fd) == NULL) {  /*  read 60 chars         */
            if(feof(site_fd) != 0)
                printf("Encountered end of input file\n");
            if(ferror(site_fd) != 0)
                printf("Error reading input file\n");
/*             clearerr(infile); */
            fprintf(stderr,"Radar ID %s not found in data file %s\n.", 
                                                radID, site_data_file);
            if(site_fd != NULL)
                fclose(site_fd);
            return (TRUE);
        }
  
        len = strcspn(data, ",");
        strncpy(icao, data, len);
        icao[len] = '\0';
        lat_start = len + 1;

        len = strcspn(data+lat_start, ",");
        strncpy(temp_str, data+lat_start, len);
        temp_str[len] = '\0';
        rlat = atof(temp_str);
        long_start = lat_start + len + 1;
        
        len = strcspn(data+long_start, ",");
        strncpy(temp_str, data+long_start, len);
        temp_str[len] = '\0';       
        rlong = atof(temp_str);
        
/*  debug */
/* fprintf(stderr,"DEBUG LL lookup - icao is '%s', Lat is '%f', Long is '%f'\n",  */
/*                                                             icao, rlat, rlong); */
        if( strncmp(radID, icao, 4) == 0) {
            radar_lat = rlat / 1000.0;
            radar_long = rlong / 1000.0;
            if(site_fd != NULL)
                fclose(site_fd);            
            return (FALSE);
        }
  
    } /*  end while */
  
    if(site_fd != NULL)
        fclose(site_fd);
  
} /*  end lookup_lat_long() */






int check_for_directory_existence(char *dirname) 
{
    /* if the directory exists return TRUE else FALSE */
    FILE *fp;
   
    fp=fopen(dirname,"rt");
    if(fp==NULL) {
    return FALSE;

    } else {
        fclose(fp);

    return TRUE;

    }  

}













/* /////////////////////////////////////////////////////////////// */
void Usage() 
{
fprintf(stderr,"\nIn Usage\n");

char *long_sample=" Input for Sterling Radar (KLWX)\n\nmap_cvg -I new_maps/east_map.dat -LA 38.875 -LO -77.478 -R 250  -M test.map\n";

char *long_commInputs=                                "map_cvg    Infilename                Lat        Lon      Range     Mapfilename\n\
\n\twhere\
\n\t-I (Infilename) = Input data filename (complete / relative pathname)\
\n\t-LA (Lat) = Latitude of the radar\
\n\t-LO (Lon) = Longitude of the radar (all entries assumed negative)\
\n\t-R (Range) = Cutoff range (in NM) for map image, 250 NM  recommended\
\n\t-M (Mapfilename) = Output map filename (complete / relative pathname)\n";

char *short_sample=" Input for Sterling Radar (KLWX)\n\nmap_cvg -I new_maps/east_map.dat -D KLWX\n";

char *short_commInputs=                                 "map_cvg    Infilename               ID  \n\
\n\twhere\
\n\t-I (Infilename) = Input data filename (complete / relative pathname)\
\n\t-D (ID) = Radar ID (e.g. KLWX) - uses std site Lat/Long, 250 range, std map name\n";


        fprintf(stderr,"\nExample:\n%s\n", long_sample);
        fprintf(stderr,"%s\n", long_commInputs);

        fprintf(stderr,"OR use the radar ID which uses the site std Lat/Long, 250 NM range, and map filename\n");
  
        fprintf(stderr,"\nExample:\n%s\n", short_sample);
        fprintf(stderr,"%s\n", short_commInputs);

        fprintf(stderr,"OR\n\tOnly type 'map_cvg' to use the X-Window GUI\n");

} /*  end Usage() */






/*  this is the command line version,  */
/*  if the main function is called with arguments */
/*  this function is executed, calls ProcessVectors() */
/*  using the argument list from the command line */
/* ///////////////////////////////////////////////////////////////// */
void BuildMap ()

{
/*     fprintf(stderr,"\nEntering BuildMap\n"); */
    
    if(radID[0] == '\0') {
        fprintf(stderr,"Creating map '%s' with range %d \n", outfile, range);
        fprintf(stderr,"for location Latitude %f,   Longitude %f \n", 
                                                      radar_lat, radar_long);
        
    } else {
        fprintf(stderr,"Creating standard map for radar ID  %s \n", radID);
    }

    /*  debug */
    /* fprintf(stderr,"Input parameters for building map\n"); */
    /* fprintf(stderr,"-D Radar ID (currently not used) '%s'\n", radID); */
    /* fprintf(stderr,"-LA Radar Latitude '%f'\n", radar_lat); */
    /* fprintf(stderr,"-LO Radar Longitude '%f'\n", radar_long); */
    /* fprintf(stderr,"-R Range of Map image '%d'\n", range); */
    /* fprintf(stderr,"-I Input data filename '%s'\n", infile); */
    /* fprintf(stderr,"-M Output map filename '%s'\n", outfile); */

    if ((fd_in  = fopen(infile, "r")) == (FILE *) NULL) {
       printf ("ERROR: unable to open %s\n", infile);
        exit (2);
    }
    
    
    
    
    ProcessVectors();
    
    fprintf(stderr,"Map file: '%s' finished.\n", outfile);
    
    if(fd_in != NULL)
        fclose(fd_in);

} /*  end BuildMap() */





/*  this function does all the work, it reads the input file and  */
/*  creates the cvg map file. */
/*  PROBLEMS */

/*  EHNANCEMENTS */
/*         a. Reduced the size of the data files by writing */
/*            output in scaled integers or floats., floats */
/*            chosen because fewer transformations & smoother lines */

/* ///////////////////////////////////////////////////////////////// */
void ProcessVectors()
{
    float   minimum_latitude;
    float   minimum_longitude;
    float   maximum_latitude;
    float   maximum_longitude;
    
    float        read_lat;
    float        read_long;

unsigned int map_type;

    float        flag_lat=90.4444;
    float        flag_long=900.4444;
    
    int     flag_not_written=TRUE;

	double kmdeglat; /*  KM / deg  Latitude  */
	double kmdeglon; /*  KM / deg  Longigude */
	double radlat;   /*  radar Latitude in radians */

    int     val =(int)NULL;
    FILE    *fd_out=NULL;



/*     fprintf(stderr,"Entering ProcessVectors\n"); */

    if(outfile == NULL) {
       fprintf(stderr, "ERROR - output filename NULL\n");
       return;
    }
       
    if ((fd_out = fopen(outfile, "w")) == (FILE *) NULL) {
        fprintf (stderr,"ERROR: unable to open '%s'\n", outfile);
        return;
    }

/* ////////////////////////////////////////////////////////////////////////// */
/*  new logic */

    /*  NOTE: THE INPUT USGS 'graphic' DATA IS ALWAYS positive Lat-Long so  */
    /*        the radar Lat-Long must always be positive for the calculation  */
    /*        of min and max Lat Longs */
    if(radar_lat < 0.0)
        radar_lat = -1.0 * radar_lat;
    if(radar_long < 0.0)
        radar_long = -1.0 * radar_long;
        
    /*  calculate Latitude / Longitude limits from range input and radar location */
	radlat=(PI/180.0)*(double)radar_lat;
	kmdeglat=111.13209-0.56605*cos(2.*radlat)+0.00012*cos(4.*radlat)
	    -0.000002*cos(6.*radlat);
        kmdeglon=111.41513*cos(radlat)-0.09455*cos(3.*radlat)
      	    +0.00012*cos(5.*radlat);

    maximum_latitude=(float) ( radar_lat + ((float)range * 1/(kmdeglat*CKM_TO_NM)) );
    minimum_latitude=(float) ( radar_lat - ((float)range * 1/(kmdeglat*CKM_TO_NM)) );
    maximum_longitude=(float) ( radar_long + ((float)range * 1/(kmdeglon*CKM_TO_NM)) );
    minimum_longitude=(float) ( radar_long - ((float)range * 1/(kmdeglon*CKM_TO_NM)) );


    /*  write the indicator that this is a new type CVG map file */
   /*      writing a scaled integer value for  */
   /*      825,307,441 is 0x31313131 or '1' '1' '1' '1'   */
   /*      used by CVG to detect new map format (bytewsapping not needed)  */
   map_type = (unsigned int) 825307441;
   fwrite((void *)&map_type, (size_t)1, sizeof(unsigned int), fd_out);   
   
   /*  we write an integer with value of map version number.  This is also used */
   /*  by CVG to detect a map made on an other Endian platform */
   map_type = (unsigned int) 1;
   fwrite((void *)&map_type, (size_t)1, sizeof(unsigned int), fd_out); 

    /*  the following must permit those portions of a vector that are within */
    /*  the desired map limits to be in the output file.  This could divide  */
    /*  a single vector into multiple vectors and each of these must begin with */
    /*  the same vector beginning flag to be processed correctly by cvg_map */
    while( !feof(fd_in) ) {
        
        val = fscanf(fd_in, "%f %f\n", &read_lat, &read_long);
        if(val != 2)
            continue;
        
        if(read_lat > 90.0) { /*  a beginning vector flag */
            
            flag_lat = read_lat;
            flag_long = read_long;
            flag_not_written = TRUE;
            
        } else { /*  a real lat-long point */
            
            if(read_lat >= minimum_latitude &&
               read_lat <= maximum_latitude &&
               read_long >= minimum_longitude &&
               read_long <= maximum_longitude ) { /*  within map limits */
            
                if(flag_not_written == TRUE) {
/*  output */
/* / float version */
                    fwrite((void *)&flag_lat, (size_t)1, sizeof(float), fd_out);
                    fwrite((void *)&flag_long, (size_t)1, sizeof(float), fd_out);                    
                    flag_not_written = FALSE;
                }
                

/* / float version */
                fwrite((void *)&read_lat, (size_t)1, sizeof(float), fd_out);
                fwrite((void *)&read_long, (size_t)1, sizeof(float), fd_out);
                    
            } else { /*  point not within map limits */
                
                flag_not_written = TRUE; 
                
            }
        
        } /*  end a real lat-long point */
        
    } /*  end while */

    if(fd_out != NULL)
        fclose(fd_out);


} /*  end ()ProcessVectors */







