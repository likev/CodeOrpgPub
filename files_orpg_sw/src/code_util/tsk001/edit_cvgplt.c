/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:16:44 $
 * $Id: edit_cvgplt.c,v 1.1 2009/08/19 15:16:44 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* edit_cvgplt.c */
/* this program lets one graphically edit color palettes */

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Scale.h>
#include <Xm/DrawnB.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
/* CVG 9.1 */
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/FileSB.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
/* CVG 9.1 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include "global2.h"


/* cvg 9.1 */
#define VARIABLE_SIZE 1
#define FIXED_SIZE 2

int resize_mode = VARIABLE_SIZE;

int max_scroll_wid_height, scroll_wid_height; /* used with small display screens */
Widget scroll_widget=NULL; /* used with small display screens */

Widget shell, swatch, red_slider, green_slider, blue_slider;
Widget toggles[256], current_toggle;
/*CVG 9.1 */
Widget palettefile_text;

/* CVG 9.1 - added prev_palette_size and new_palette_size */
int palette_size=0, new_palette_size=0, prev_palette_size=0;
int dynamic_color;
int current_index;
char palette_filename[512];
XColor display_colors[256];
XColor unassigned_colors[256]; /* CVG 9.1 - ADDED */
Colormap cmap;
/* CVG 9.1 - moved to file global */
Pixel snow_color, light_grey_color;
FILE *palette_file=NULL;
Widget panel;
int num_cols;

/*  adding fallback resources */
String fallbacks [] = 
{ 
"*redslider*troughColor:    red",
"*greenslider*troughColor:  green",
"*blueslider*troughColor:   blue",
    NULL
};


/* CVG 9.1 */
char titleString[100];
char config_dir[255];
int toggles_needed=0;
int unsaved_edits;


/* prototypes */
void read_palette(FILE *the_palette_file);
void select_color_callback(Widget w, XtPointer client_data, XtPointer call_data);
void red_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data);
void green_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data);
void blue_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data);
void exit_program_callback(Widget w, XtPointer client_data, XtPointer call_data);
void save_palette_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.1 */
void exit_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void exit_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void exit_program();

void save_palette_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void save_palette_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);

void open_palette_callback(Widget w, XtPointer client_data, XtPointer call_data);
void open_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void open_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void screen_setup_scroll_bars();
void remove_toggles(int destroy_all_toggles);
void set_toggles_insensitive();
void create_toggles();
void create_all_toggles();
void set_toggles_sensitive();
void color_toggles();

void fileselect_help_callback(Widget w, XtPointer client_data, XtPointer call_data);

void save_palette_as_callback(Widget w, XtPointer client_data, XtPointer call_data);
void save_as_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void save_as_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void new_palette_callback(Widget w, XtPointer client_data, XtPointer call_data);
void new_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void new_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data); /* */
void size_set_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void size_set_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void close_callback(Widget w, XtPointer client_data, XtPointer call_data);
void close_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void close_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void close_plt();

void write_to_palette(FILE *the_palette_file);
void write_my_color_to_palette(FILE *plt_file, short r_c, short g_c, short b_c);
void get_config_dir();
int check_args(int argc,char *argv[],char *param,int startval);




int main(int argc, char* argv[])
{
    /* CVG 9.1 - moved panel to file global, added frame2 */
    Widget top_form, form, frame, frame2, sliders, quitbut, savebut;
    
    /* CVG 9.1 */
    Widget save_asbut, openbut, newbut, closebut;
    Widget mode_label, mode_value_lbl;
    Widget panel_parent=NULL;
    int disp_width, disp_height;
    int file_sel_wid_space;
    XmString  mode_xtmr;

    
    /* CVG 9.1 - moved num_cols to file global */
    int  i, value;
    
    XtAppContext app;
    Pixel pixels[256], red_color, green_color, blue_color;
    XColor color, unused; 
    Visual *vis;





    /* CVG 9.1 */
    palette_filename[0] = ' '; 
    palette_filename[1] = '\0'; 

    /* CVG 9.1 */
    get_config_dir();



/*     shell = XtAppInitialize(&app,"Cvg_color_edit",NULL,0,&argc,argv,NULL,NULL,0); */
    shell = XtAppInitialize(&app,"Cvg_color_edit.RESOURCE",NULL,0,&argc,argv,fallbacks,NULL,0);
    
    XtVaSetValues(shell, XmNallowShellResize, True, NULL);

    XtVaSetValues(shell, XmNtitle, "Editor for CVG Color Palettes - ", NULL);


/* CVG 9.1 - get display screen size */
    disp_width = DisplayWidth(XtDisplay(shell),
            XScreenNumberOfScreen(XtScreen(shell)));
    disp_height = DisplayHeight(XtDisplay(shell),
            XScreenNumberOfScreen(XtScreen(shell)));
    fprintf (stderr,"display screen width=%d, display screen height=%d\n", 
            disp_width, disp_height);  



    /* CVG 9.1 - CONTINUE if no arguments provided on the command line */ 
    if(argc == 1) {  

        ;
        
    } else { /* there are command line parameters */
        
        /* CVG 9.1 - ADD CAPABILITY TO READ -f -v FLAGS */
        if( (value=check_args(argc,argv,"-v",0))>0 ) {
                 resize_mode = VARIABLE_SIZE; 
        } 
        
        if( (value=check_args(argc,argv,"-f",0))>0 ) {
                 resize_mode = FIXED_SIZE; 
        } 
        
        if( (value=check_args(argc,argv,".plt",0))>0 ) {
                 
            strncpy(palette_filename, argv[value], 512);
            /* CVG 9.1 - why is this done? */
            palette_filename[511] = '\0'; 
            
            /* CVG 9.1 moved inside conditional */
            if( (palette_file = fopen(palette_filename, "r")) == NULL ) {
                fprintf(stderr, "Could not open file %s\n", palette_filename);
                exit(0);
            }
            /* read in palette color info*/
            read_palette(palette_file);
            fclose(palette_file);
        
        } /* end check_args ".plt" */
        
    } /* end else there are command line parameters */
    
    

    /* CVG 9.1 - also set in remove_toggles() and set_toggles_insensitive() */
    prev_palette_size = palette_size;


    sprintf(titleString,"Editor for CVG Color Palettes - %s", palette_filename);

    XtVaSetValues(shell, XmNtitle, titleString, NULL);
    
    
    /* get some named colors first off */
    cmap = DefaultColormap(XtDisplay(shell), 0); 
    XAllocNamedColor(XtDisplay(shell), cmap, "red", &color, &unused);
    red_color = color.pixel;

    XAllocNamedColor(XtDisplay(shell), cmap, "blue", &color, &unused);
    blue_color = color.pixel;
    
    XAllocNamedColor(XtDisplay(shell), cmap, "green", &color, &unused);
    green_color = color.pixel;



    /* start creating the UI */
    top_form = XtVaCreateManagedWidget("form", xmFormWidgetClass, shell, NULL);


    /* CVG 9.1 - add mode radiobox, text widget palette file */
    /***********************************************************************/

   file_sel_wid_space = 26;
   
   max_scroll_wid_height = disp_height - 300;

   
   /*****************************************************************/
   
    mode_label = XtVaCreateManagedWidget("Size Mode:", 
       xmLabelWidgetClass,  top_form,
       XmNtopAttachment,       XmATTACH_FORM,
       XmNtopOffset,           10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          file_sel_wid_space,
       XmNbottomAttachment,    XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       NULL);  
  
    mode_value_lbl = XtVaCreateManagedWidget("Variable", 
       xmLabelWidgetClass,  top_form,
       XmNrecomputeSize,    False,
       XmNwidth,            52,
       XmNtopAttachment,    XmATTACH_FORM,
       XmNtopOffset,        10,
       XmNleftAttachment,   XmATTACH_WIDGET,
       XmNleftWidget,       mode_label,
       XmNleftOffset,       0,
       XmNbottomAttachment, XmATTACH_NONE,
       XmNrightAttachment,  XmATTACH_NONE,
       NULL);  

   
    if(resize_mode == FIXED_SIZE) {
        mode_xtmr = XmStringCreateLtoR("Fixed", XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(mode_value_lbl, XmNlabelString, mode_xtmr, NULL);
        XmStringFree(mode_xtmr);
    }


    palettefile_text = XtVaCreateManagedWidget("palette file",
       xmTextFieldWidgetClass, top_form,
       XmNvalue,                 palette_filename,
       XmNcolumns,               63,
       XmNmaxLength,             255,
       XmNeditable,              False,
       XmNcursorPositionVisible, False,
       XmNtopAttachment,         XmATTACH_FORM,
       XmNtopOffset,             5,
       XmNleftAttachment,        XmATTACH_WIDGET,
       XmNleftWidget,            mode_value_lbl,
       XmNleftOffset,            file_sel_wid_space,
       XmNbottomAttachment,      XmATTACH_NONE,
       XmNbottomOffset,          5,
       XmNrightAttachment,       XmATTACH_FORM,
       XmNrightOffset,           file_sel_wid_space,
       NULL);

   
   /***********************************************************************/

    /* CVG 9.1 - added an "New" button*/
    newbut = XtVaCreateManagedWidget("New",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,       XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNleftAttachment,       XmATTACH_FORM,
       XmNleftOffset,           10,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                70,
       XmNheight,               25,
       NULL);
    XtAddCallback(newbut, XmNactivateCallback, new_palette_callback, NULL);


    /* CVG 9.1 - added an "Open" button*/
    openbut = XtVaCreateManagedWidget("Open",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,       XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_NONE,
       XmNleftAttachment,       XmATTACH_WIDGET,
       XmNleftWidget,           newbut,
       XmNleftOffset,           15,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                70,
       XmNheight,               25,
       NULL);
    XtAddCallback(openbut, XmNactivateCallback, open_palette_callback, NULL);

    
    savebut = XtVaCreateManagedWidget("Save",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,        XmATTACH_NONE,
       XmNrightAttachment,      XmATTACH_NONE,
       XmNleftAttachment,       XmATTACH_WIDGET,
       XmNleftWidget,           openbut,
       XmNleftOffset,           15,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                70,
       XmNheight,               25,
       NULL);
    XtAddCallback(savebut, XmNactivateCallback, save_palette_callback, NULL);


/* CVG 9.1 - add a "Save As" button */
    save_asbut = XtVaCreateManagedWidget("Save As",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,        XmATTACH_NONE,
       XmNrightAttachment,      XmATTACH_NONE,
       XmNleftAttachment,       XmATTACH_WIDGET,
       XmNleftWidget,           savebut,
       XmNleftOffset,           15,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                70,
       XmNheight,               25,
       NULL);
    XtAddCallback(save_asbut, XmNactivateCallback, save_palette_as_callback, NULL);


/* CVG 9.1 - add a "Close" button */
    closebut = XtVaCreateManagedWidget("Close",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,        XmATTACH_NONE,
       XmNrightAttachment,      XmATTACH_NONE,
       XmNleftAttachment,       XmATTACH_WIDGET,
       XmNleftWidget,           save_asbut,
       XmNleftOffset,           15,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                70,
       XmNheight,               25,
       NULL);
    XtAddCallback(closebut, XmNactivateCallback, close_callback, NULL);


    /* CVG 9.1 - changed "Quit" to "Exit" */
    quitbut = XtVaCreateManagedWidget("Exit",
       xmPushButtonWidgetClass, top_form,
       XmNtopAttachment,        XmATTACH_NONE,
       XmNrightAttachment,      XmATTACH_FORM,
       XmNrightOffset,          10,
       XmNleftAttachment,       XmATTACH_NONE,
       XmNbottomAttachment,     XmATTACH_FORM,
       XmNbottomOffset,         10,
       XmNwidth,                80,
       XmNheight,               25,
       NULL);
    XtAddCallback(quitbut, XmNactivateCallback, exit_program_callback, NULL);

    /* create swatch and sliders */
    form = XtVaCreateManagedWidget("controls", 
       xmFormWidgetClass, top_form,
       XmNtopAttachment,       XmATTACH_NONE,
       XmNrightAttachment,     XmATTACH_FORM,
       XmNrightOffset,         10,
       XmNleftAttachment,      XmATTACH_FORM,
       XmNleftOffset,          10,
       XmNbottomAttachment,    XmATTACH_WIDGET,
       XmNbottomWidget,        savebut,
       XmNbottomOffset,        0,
       NULL);

    frame = XtVaCreateManagedWidget("frame",
       xmFrameWidgetClass, form,
       XmNtopAttachment,       XmATTACH_FORM,
       XmNtopOffset,           0,
       XmNrightAttachment,     XmATTACH_FORM,
       XmNrightOffset,         20,
       XmNbottomAttachment,    XmATTACH_FORM,
       XmNbottomOffset,        10,
       XmNleftAttachment,      XmATTACH_NONE,
       NULL);

    XtVaCreateManagedWidget("Current Color", xmLabelWidgetClass, frame, 
                XmNchildType, XmFRAME_TITLE_CHILD, NULL);

    swatch = XtVaCreateManagedWidget("display", 
       xmDrawnButtonWidgetClass, frame,

       XmNwidth,     90,
       XmNheight,    90,
       XmNcolormap,  cmap,
       NULL);

    sliders = XtVaCreateManagedWidget("sliderpanel",
       xmRowColumnWidgetClass, form,
       XmNtopAttachment,      XmATTACH_FORM,
       XmNbottomAttachment,   XmATTACH_NONE,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNleftOffset,         20,
       XmNrightAttachment,    XmATTACH_WIDGET,
       XmNrightWidget,        swatch,
       XmNrightOffset,        20,
       NULL);

    red_slider = XtVaCreateManagedWidget("redslider", 
       xmScaleWidgetClass, sliders,
       XmNminimum,         0,
       XmNmaximum,         255,
       XmNshowValue,       TRUE,
       XmNorientation,     XmHORIZONTAL,
       XmNtroughColor,     "red",
       NULL);
    XtAddCallback(red_slider, XmNvalueChangedCallback, red_slider_change_callback, NULL);
    XtAddCallback(red_slider, XmNdragCallback, red_slider_change_callback, NULL);

    green_slider = XtVaCreateManagedWidget("greenslider", 
       xmScaleWidgetClass, sliders,
       XmNminimum,         0,
       XmNmaximum,         255,
       XmNshowValue,       TRUE,
       XmNorientation,     XmHORIZONTAL,
       NULL);
    XtAddCallback(green_slider, XmNvalueChangedCallback, green_slider_change_callback, NULL);
    XtAddCallback(green_slider, XmNdragCallback, green_slider_change_callback, NULL);

    blue_slider = XtVaCreateManagedWidget("blueslider", 
       xmScaleWidgetClass, sliders,
       XmNminimum,         0,
       XmNmaximum,         255,
       XmNshowValue,       TRUE,
       XmNorientation,     XmHORIZONTAL,
       NULL);
    XtAddCallback(blue_slider, XmNvalueChangedCallback, blue_slider_change_callback, NULL);
    XtAddCallback(blue_slider, XmNdragCallback, blue_slider_change_callback, NULL);



    frame2 = XtVaCreateManagedWidget("frame2",
       xmFrameWidgetClass, top_form,

       XmNtopAttachment,      XmATTACH_FORM,
       XmNtopOffset,          35,
       XmNrightAttachment,    XmATTACH_FORM,
       XmNrightOffset,        0,
       XmNleftAttachment,     XmATTACH_FORM,
       XmNleftOffset,         0,
       XmNbottomAttachment,   XmATTACH_WIDGET,
       XmNbottomWidget,       form,
       NULL);

    XtVaCreateManagedWidget("Palette", xmLabelWidgetClass, frame2,
                XmNchildType, XmFRAME_TITLE_CHILD, NULL);


    /* CVG 9.1 - added scrolled window widget to enable scroll bars */
    scroll_widget = XtVaCreateManagedWidget ("scrolledwindow",
        xmScrolledWindowWidgetClass,    frame2,
        XmNscrollingPolicy,             XmAUTOMATIC,
        /* XmNwidth,                       width, */
        /* XmNheight,                      height, */
        XmNtopAttachment,               XmATTACH_WIDGET,
        XmNtopWidget,                   frame2,
        XmNtopOffset,                   0,
        XmNleftAttachment,              XmATTACH_WIDGET,
        XmNleftWidget,                  frame2,
        XmNleftOffset,                  0,
        XmNbottomAttachment,            XmATTACH_WIDGET,
        XmNbottomWidget,                frame2,
        XmNbottomOffset,                0,     
        XmNrightAttachment,             XmATTACH_WIDGET,
        XmNrightWidget,                 frame2,
        XmNrightOffset,                 0,
        NULL);
    
    
    if(resize_mode == VARIABLE_SIZE)              
        scroll_wid_height = (2 * 28) + 9;         
    else                                          
        scroll_wid_height = (22 * 28) + 9;        
                                                  
    if(scroll_wid_height > max_scroll_wid_height) 
        scroll_wid_height = max_scroll_wid_height;

    XtVaSetValues(scroll_widget, XmNheight,  scroll_wid_height, NULL);
        
        
    /* CVG 9.1 -  make each row consist of up to 12 buttons */
    /* NOTE: with XmNorientation = XmHORIZONTAL, XmNnumColumns */
    /*       and num_cols is actually the number of rows.      */
   
    panel_parent = scroll_widget;
    
    panel = XtVaCreateManagedWidget("colorpanel",

       xmRowColumnWidgetClass, panel_parent,
       XmNradioBehavior,   TRUE,
       XmNnumColumns,      2,
       XmNpacking,         XmPACK_COLUMN,
       XmNadjustLast,      FALSE,
       /* must remain horizontal*/
       XmNorientation,     XmHORIZONTAL, 
       NULL);
       
       

/* CVG 9.1 - the current toggles are destroyed and recreated in */
/*           open_ok_callback                               */
/****************************************************************************/


    if(resize_mode == VARIABLE_SIZE) {    
        if(palette_size == 0) {           
            prev_palette_size = 0;        
        } else {                          
            toggles_needed = palette_size;
            create_toggles();             
        }                                 
                                          
    } else {  /* FIXED_SIZE */ 
        create_all_toggles();             
        set_toggles_sensitive();          
                                          
    }  /* end FIXED_SIZE */
  
    
    /* finish set up and activate */
    current_toggle = NULL;    /* start by having nothing selected */

/****************************************************************************/

    XtRealizeWidget(shell);




    /* we need to make our own colormap... */
    vis = XDefaultVisualOfScreen(XtScreen(shell));
    cmap = XCreateColormap(XtDisplay(shell), XtWindow(shell), vis, AllocNone);
    
    if( (vis->class == PseudoColor) || (vis->class == DirectColor) )
        dynamic_color = TRUE;
    else
        dynamic_color = FALSE;

    
    fprintf(stderr, "dynamic color = %d\n", dynamic_color);

    /* ...so that we can allocate read/writeable color cells.
     * we handle color palettes of up to 256 colors
     */
    if(dynamic_color) {
        if(!XAllocColorCells(XtDisplay(shell), cmap, TRUE, NULL, 0, pixels, 256)) {
           printf("Could not allocate color cells!\n");
           exit(0);
        }

        /* save which colors are associated with which pixels */
        for(i=0; i<256; i++) {
            display_colors[i].pixel = pixels[i];
        }
     } /* end if dynamic_color */


     color_toggles();


    /* set the colormap for the swatch correctly as well */
    XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel,
                          XmNforeground, display_colors[0].pixel, /* should be black */
                          XmNcolormap,   cmap,
                          NULL);

/****************************************************************************/

    XtAppMainLoop(app);

    return 0;
    
} /* end main */


/*******************************************************************************/
/*                              Other Functions                                */
/*******************************************************************************/


/* CVG 9.1 */
int check_args(int argc,char *argv[],char *param,int startval) {
  /* check the command line arguments for input parameters.
     If the parameter IS in the list, then return the positive
     index of the parameter, otherwise return 0 */
  int TEST=FALSE;
  int i;

  if(TEST)
    fprintf(stderr,"inside check_args: input string=%s  strlen=%i st=%i end=%i\n",
     param,strlen(param),startval,argc);

  if(strcmp(param, ".plt")==0) { /* search for a palette name *.plt */
    
    for(i=startval;i<argc;i++) {
          /* using a substring compare was a problem, could conflict with filenames  */
          if(strstr(argv[i],".plt")!=NULL){
             return(i);
          }
      
      } /* end i loop */
    
    
  } else { /* search for other flags */
  
      for(i=startval;i<argc;i++) {
          /* using a substring compare was a problem, could conflict with filenames  */
          if(strcmp(argv[i],param)==0){
             return(i);
          }
      
      } /* end i loop */
      
  } /* end search for other flags */
  
  if(TEST) fprintf(stderr,"returning false\n");
  
  return(FALSE);
  
} /* end check_args() */




/* CVG 9.1 */
void get_config_dir()
{

  char *home_dir;
  struct stat confdirstat;

  /*  The following errors should never occur because this program is not */
  /*  forked until after the preferences are initialized by CVG */
  
  /* if the HOME directory is not defined, EXIT */
  if((home_dir = getenv("HOME")) == NULL) {
      fprintf(stderr, "     ERROR*****************ERROR************ERROR*\n");
      fprintf(stderr, "     Environmental variable HOME not set.\n");
      fprintf(stderr, "          CVG launch aborted \n");
      fprintf(stderr, "     *********************************************\n");
      exit(0);
  }

  sprintf(config_dir, "%s/.%s", home_dir,CVG_PREF_DIR_NAME);

  /* make sure the local directory exists                        */
  /*    if the local configuration directory exists, do nothing  */
  /*                                                             */
  /*    if the directory does not exist, error message           */

  if(stat(config_dir, &confdirstat) < 0) {

      fprintf(stderr, "\n");
      fprintf(stderr, "     ERROR****************************************\n"); 
      fprintf(stderr, "     * CVG local config directory does not exist  \n"); 
      fprintf(stderr, "     * Cannot edit files for this version of CVG  \n"); 
      fprintf(stderr, "     *********************************************\n");
      fprintf(stderr, "\n");   
      
      sprintf(config_dir, "%s", home_dir);
   }


} /*  end get_config_dir() */



/**************************************************************************/



void read_palette(FILE *the_palette_file)
{
    int i, in_color;
  
    /* get number of colors */
    fscanf(the_palette_file, "%d", &palette_size);

    /* read in color table. First all R values, then all G, then all B */
    for(i=0; i<palette_size; i++) {
        display_colors[i].flags = DoRed | DoGreen | DoBlue;
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
            (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }
        display_colors[i].red   = ((unsigned short)in_color)<<8;
    }

    for(i=0; i<palette_size; i++) {
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
            (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }
        display_colors[i].green = ((unsigned short)in_color)<<8;
    }

    for(i=0; i<palette_size; i++) {
        if( (fscanf(the_palette_file, "%d", &in_color) != 1) || 
            (ferror(the_palette_file) != 0) ) {
            fprintf(stderr, "Error reading palette file.\n");
        }
        display_colors[i].blue  = ((unsigned short)in_color)<<8;
    }
    
} /* end read_palette() */




/* this function uses the display colors created via the toggle callbacks */
/* CVG 9.1 */
void write_to_palette(FILE *the_palette_file)
{
int i;
    
    
    /* first, we output the number of colors in the palette */
    fprintf(the_palette_file, "%d\n", palette_size);

    /* then we print out all the R values for each color sequentially, then
     * the G values and lastly the B values.  we only seperate each value 
     * by a space here, so if you want to edit the palette file with a text
     * editor, use one with line wrapping.  it doesn't really matter, though,
     * what whitespace is used - any will do
     */
    fprintf(stderr,"\n"); 
    fprintf(stderr,"red:\n");
    fprintf(stderr,"'");
    for(i=0; i<palette_size; ++i) {
        /* here we first have to update the stored RGB values
     * in the global array
     */
        display_colors[i].flags = DoRed | DoGreen | DoBlue;
        XQueryColor(XtDisplay(shell), cmap, &display_colors[i]);

        fprintf(the_palette_file, " %3d", display_colors[i].red>>8);
        fprintf(stderr," %3d", display_colors[i].red>>8);
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(the_palette_file, "\n");
    fprintf(stderr,"'\n");
    fprintf(stderr,"green:\n");
    fprintf(stderr,"'");
    
    for(i=0; i<palette_size; ++i) {

        fprintf(the_palette_file, " %3d", display_colors[i].green>>8);
        fprintf(stderr," %3d", display_colors[i].green>>8);
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(the_palette_file, "\n");
    fprintf(stderr,"'\n");
    fprintf(stderr,"blue:\n");
    fprintf(stderr,"'");
    
    for(i=0; i<palette_size; ++i) {

        fprintf(the_palette_file, " %3d", display_colors[i].blue>>8);
        fprintf(stderr," %3d", display_colors[i].blue>>8);
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(the_palette_file, "\n");
    fprintf(stderr,"'\n");
    
    
    
} /* end write_to_palette() */




/* this function requires that a defined color be created with */
/* red, green, and blue values */
/* CVG 9.1 */
void write_my_color_to_palette(FILE *plt_file, short r_c, short g_c, short b_c)
{

int i;
    
    
    /* first, we output the number of colors in the palette */
    fprintf(plt_file, "%d\n", palette_size);

    /* then we print out all the R values for each color sequentially, then
     * the G values and lastly the B values.  we only seperate each value 
     * by a space here,  doesn't really matter, though,
     * what whitespace is used - any will do
     */
    fprintf(stderr,"\n"); 
    fprintf(stderr,"red:\n");
    fprintf(stderr,"'");
    for(i=0; i<palette_size; ++i) {
        if(i==0) {
            fprintf(plt_file, " %3d", 0);
            fprintf(stderr," %3d", 0);
        } else {
            fprintf(plt_file, " %3d", r_c);
            fprintf(stderr," %3d", r_c);
        }
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(plt_file, "\n");
    fprintf(stderr,"'\n");
    fprintf(stderr,"green:\n");
    fprintf(stderr,"'");
    
    for(i=0; i<palette_size; ++i) {
        if(i==0) {
            fprintf(plt_file, " %3d", 0);
            fprintf(stderr," %3d", 0);
        } else {
            fprintf(plt_file, " %3d", g_c);
            fprintf(stderr," %3d", g_c);
        }
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(plt_file, "\n");
    fprintf(stderr,"'\n");
    fprintf(stderr,"blue:\n");
    fprintf(stderr,"'");
    
    for(i=0; i<palette_size; ++i) {
        if(i==0) {
            fprintf(plt_file, " %3d", 0);
            fprintf(stderr," %3d", 0);
        } else {
            fprintf(plt_file, " %3d", b_c);
            fprintf(stderr," %3d", b_c);
        }
        if((i+1)%20 == 0) fprintf(stderr,"\n");
    }
    
    fprintf(plt_file, "\n");
    fprintf(stderr,"'\n");
    


} /* end write_my_color_to_palette() */




/* CVG 9.1 */
void open_palette_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmfile, xmdir;
    char dirname[255];
    
    char color_dir[255];
    int i;
    
    d = XmCreateFileSelectionDialog(w, "choose_palette_dialog", NULL, 0);

    XtVaSetValues(XtParent(d), XmNtitle, "Choose File...", 
          XmNwidth,                560,
          XmNheight,               380,
          XmNx,                    10,
          XmNy,                    10,
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


    /* set the currently selected file to the current palette file, and the
     * current directory to the directory that it is in
     */
     
    /* CVG 9.1 - if no file previously selected, use the config_dir */
    if( (palette_filename == NULL) || (strcmp(palette_filename, " ") == 0 ) ) {
        
        sprintf(color_dir,"%s/colors",config_dir);
        xmfile = XmStringCreateLtoR(" ", XmFONTLIST_DEFAULT_TAG); 
        xmdir = XmStringCreateLtoR(color_dir, XmFONTLIST_DEFAULT_TAG);
        
    } else {
 
        xmfile = XmStringCreateLtoR(palette_filename, XmFONTLIST_DEFAULT_TAG);
        i = strlen(palette_filename) - 1;
        while( (palette_filename[i] != '/') && (i > 0) ) 
            i--;
        
        strncpy(dirname, palette_filename, i+2);  /* include the '/' */
        dirname[i+1] = '\0';
        xmdir = XmStringCreateLtoR(dirname, XmFONTLIST_DEFAULT_TAG);
    }

   
    XtVaSetValues(d, 
          XmNdirSpec,          xmfile,
          XmNdirectory,        xmdir,
          NULL);
    XmStringFree(xmdir);
    XmStringFree(xmfile);
    
    XtAddCallback(d, XmNokCallback, open_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, open_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, fileselect_help_callback, NULL); 

    XtManageChild(d);
    
} /* end open_palette_callback() */



/**************************************************************************/



/* CVG 9.1 */
/* set the pdlb file field to whatever was selected */
void open_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct *cbs = 
                                 (XmFileSelectionBoxCallbackStruct *)call_data;
    char *filename;
    
    int j;
    char palette_name[60];
    
    

    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
    

    if( (palette_file = fopen(filename, "r")) == NULL ) {
        fprintf(stderr, "Could not open file: '%s'\n", filename);
        if(filename!=NULL)
           free(filename);
        return;
    }
    
    strcpy(palette_filename, filename); 
    
    /* read in palette color info*/
    read_palette(palette_file);
    fclose(palette_file);
    

    j = strlen(palette_filename) - 1;
    while( (palette_filename[j] != '/') && (j > 0) ) 
        j--;
    strcpy(palette_name, (palette_filename + j+1) );
    sprintf(titleString,"Editor for CVG Color Palettes - %s", palette_name);

    XtVaSetValues(shell, XmNtitle, titleString, NULL);

    XtVaSetValues(palettefile_text, XmNvalue, filename, NULL);
    XtUnmanageChild(w);
    free(filename);



    if(resize_mode == VARIABLE_SIZE) {
                                      
        remove_toggles(FALSE);        
                                      
        create_toggles();             
                                      
        color_toggles();              
                                      
    } else { /* FIXED SIZE */
                                      
        set_toggles_insensitive();    
                                      
        set_toggles_sensitive();      
                                      
        color_toggles();              
                                      
    }   /* end else FIXED_SIZE */

    unsaved_edits = FALSE;
    
} /* end open_ok_callback() */



/* CVG 9.1 */
/* if canceled, we just don't do anything but kill the window */
void open_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
    
}


/****************************************************************************/


/* CVG 9.1 - used in variable size mode */
void remove_toggles(int destroy_all_toggles)
{
int i;
Pixel bg;

    XtVaSetValues(panel, XmNresizeHeight, False, NULL);
    XtVaSetValues(panel, XmNresizeWidth, False, NULL); 
    XtVaSetValues(shell, XmNallowShellResize, False, NULL);
    XtRealizeWidget(shell);
    
    
    /* CVG 9.1 - ADD filling out the palette color buttons */
    XtVaGetValues(shell, XmNbackground, &bg, NULL);
    
    
    if(destroy_all_toggles==TRUE) {
        
        for(i=0; i<prev_palette_size; i++) {
            if(toggles[i] != NULL) {
                XtVaSetValues(toggles[i], XmNbackground, bg, NULL);
            }
        }
        
        for(i=0; i<prev_palette_size; i++) {
            if(toggles[i] != NULL) {
                XtDestroyWidget(toggles[i]);
            }
        }
        
    } else if(prev_palette_size == palette_size) {

        for(i=0; i<prev_palette_size; i++) {
            if(toggles[i] != NULL) {
                XtVaSetValues(toggles[i], 
                      XmNbackground, bg, 
                      XmNset,        False, 
                      NULL);
            }
        }
        toggles_needed = 0;
        
    } else if(prev_palette_size > palette_size) {
        
        for(i=0; i<prev_palette_size; i++) {
            if(toggles[i] != NULL) {
                XtVaSetValues(toggles[i], 
                    XmNbackground, bg, 
                    XmNset,        False,
                    NULL);
            }
        }
        
        for(i=prev_palette_size-1; i>palette_size-1; i--) {
            if(toggles[i] != NULL)
                XtDestroyWidget(toggles[i]);
        }
        toggles_needed = 0;
        
    } else { /* palette_size > prev_palette_size */
        
        for(i=0; i<prev_palette_size; i++) {
            if(toggles[i] != NULL) {
                XtVaSetValues(toggles[i], 
                    XmNbackground, bg, 
                    XmNset,        False,
                    NULL);
            }
        }
        toggles_needed = palette_size - prev_palette_size;
        
    }


    prev_palette_size = palette_size;
 
 
    /* set the colormap for the swatch correctly as well */
    XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel,
                          XmNforeground, display_colors[0].pixel, /* should be black */
                          XmNcolormap,   cmap,
                          NULL);
    
    XtVaSetValues(red_slider,   XmNvalue,   0,  NULL);
    XtVaSetValues(green_slider, XmNvalue,   0,  NULL);
    XtVaSetValues(blue_slider,  XmNvalue,   0,  NULL);
 
 
 
    
    XtVaSetValues(panel, XmNresizeHeight, True, NULL);
    XtVaSetValues(panel, XmNresizeWidth, True, NULL);
    XtVaSetValues(shell, XmNallowShellResize, True, NULL);
    XtRealizeWidget(shell);
    
} /* end remove_toggles() */




/* CVG 9.1 - used in fixed size mode*/
void set_toggles_insensitive()
{
    
int i;
Pixel bg;



    current_toggle = NULL;
    
    
    XtVaGetValues(shell, XmNbackground, &bg, NULL);
    
    for(i=0; i<prev_palette_size; i++) {

        XtVaSetValues(toggles[i],
                     XmNbackground,              bg,
                     XmNset,                     False,
                     XmNsensitive,               False,
                     XmNvisibleWhenOff,          False,
                     NULL);
        
    } /* end for */;
    
    prev_palette_size = palette_size;


    /* set the colormap for the swatch correctly as well */
    XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel,
                          XmNforeground, display_colors[0].pixel, /* should be black */
                          XmNcolormap,   cmap,
                          NULL);
    
    XtVaSetValues(red_slider,   XmNvalue,   0,  NULL);
    XtVaSetValues(green_slider, XmNvalue,   0,  NULL);
    XtVaSetValues(blue_slider,  XmNvalue,   0,  NULL);
        
    
} /* end set_toggles_insensitive */




/* CVG 9.1 - used in variable size mode */
void create_toggles()
{
char buf[20];
int i;

int first_needed;

    if(palette_size < 1) {
        num_cols = 1;
    
    } else {
        /* CVG 9.1 */
        /*num_cols = ceil(sqrt(palette_size)); */
        num_cols = (int) ceil( (double) palette_size / 12.0 );
    
    }
    
    XtVaSetValues(panel, XmNnumColumns, num_cols, NULL);


    scroll_wid_height = (num_cols * 28) + 9;
    
    if(scroll_wid_height > max_scroll_wid_height)
        scroll_wid_height = max_scroll_wid_height;


    XtVaSetValues(scroll_widget, XmNheight,  scroll_wid_height, NULL);
        

    
    if(toggles_needed > 0) {
        
            
        first_needed = palette_size - toggles_needed;
 
        for(i=first_needed; i<palette_size; i++) {
            sprintf(buf, "%3d", i);
    
            toggles[i] = XtVaCreateManagedWidget(buf,
                             xmToggleButtonWidgetClass,  panel,
                             NULL);
    
            XtAddCallback(toggles[i], XmNvalueChangedCallback, select_color_callback, NULL);
            
        } /* end for */
        
    } /* end if toggles_needed */

    current_toggle = NULL;



    XtVaSetValues(panel, XmNresizeHeight, True, NULL);
    XtVaSetValues(panel, XmNresizeWidth, True, NULL);
    XtVaSetValues(shell, XmNallowShellResize, True, NULL);
    XtRealizeWidget(shell);
    
} /* end create_toggles() */




/* CVG 9.1 - used in fixed size mode*/
void create_all_toggles()
{

char buf[20];
int i;
Pixel bg;





    num_cols = (int) ceil( (double) 256 / 12.0 );                     
                                                                      
    XtVaSetValues(panel, XmNnumColumns, num_cols, NULL);              
                                                                      
                                                                      
    scroll_wid_height = (num_cols * 28) + 9;                          
                                                                      
    if(scroll_wid_height > max_scroll_wid_height)                     
        scroll_wid_height = max_scroll_wid_height;                    
                                                                      
    XtVaSetValues(scroll_widget, XmNheight,  scroll_wid_height, NULL);
        

    
    
    XtVaGetValues(shell, XmNbackground, &bg, NULL);

    for(i=0; i<256; i++) {
        sprintf(buf, "%3d", i);

        toggles[i] = XtVaCreateManagedWidget(buf,
                         xmToggleButtonWidgetClass,  panel,
                         XmNbackground,              bg,
                         XmNsensitive,               False,
                         XmNvisibleWhenOff,          False,
                         NULL);

        XtAddCallback(toggles[i], XmNvalueChangedCallback, select_color_callback, NULL);
        
    } /* end for */;
    
    current_toggle = NULL;
    
    
} /* end create_all_toggles() */




/* CVG 9.1 - used in fixed size mode*/
void set_toggles_sensitive()
{
    
int i;
Pixel bg;


    XtVaGetValues(shell, XmNbackground, &bg, NULL);

    for(i=0; i<palette_size; i++) {

        XtVaSetValues(toggles[i],
                     XmNbackground,              bg,
                     XmNsensitive,               True,
                     XmNvisibleWhenOff,          True,
                     NULL);
        
    } /* end for */;
    
    current_toggle = NULL;
    
} /* end set_toggles_sensitive */




/* CVG 9.1 */
void color_toggles()
{
int i;

    /* CVG 9.1 - need to set colors for the toggle buttons */
    for(i=0; i<palette_size; i++) {

       if(dynamic_color)
            XStoreColor(XtDisplay(shell), cmap, &display_colors[i]);
       else
            XAllocColor(XtDisplay(shell), cmap, &display_colors[i]);
    
    }

    /* now, set each toggle to be the correct color */
    for(i=0; i<palette_size; i++) {
        XtVaSetValues(toggles[i], XmNbackground, display_colors[i].pixel,
                          XmNforeground, display_colors[0].pixel, /* should be black */
                          XmNcolormap,   cmap,
                                  NULL);
    }

    
} /* end color_toggles */



/****************************************************************************/



/* when a toggle is selected, update which color the swatch is displaying */
void select_color_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor color;
    Pixel  bg;
    XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *)call_data;

    /* if the toggle is being unset, we can ignore it */
    if(cbs->set == FALSE)
        return;

    /* we need to keep track of the last selected widget so that we can
     * change its value
     */
    current_toggle = w;

    /* also need to keep track of which color is selected */
    for(current_index=0; current_index<palette_size; ++current_index)
        if(current_toggle == toggles[current_index])
            break;
    
    /* update the swatch's color */
    XtVaGetValues(w, XmNbackground, &bg, NULL);
    XtVaSetValues(swatch, XmNbackground, bg, NULL);

    /* get the details about the current color */
    color.flags = DoRed | DoGreen | DoBlue;
    color.pixel = bg;
    XQueryColor(XtDisplay(w), cmap, &color);

    /* set the sliders based on the current color - need to scale it back since
     * we (like most reasonable people) let each part have 256 values - 8bits
     * 24bit color total.  X lets each part have 65563 - 16bits.  wierd.
     */
    XtVaSetValues(red_slider, XmNvalue, color.red>>8, NULL);
    XtVaSetValues(green_slider, XmNvalue, color.green>>8, NULL);
    XtVaSetValues(blue_slider, XmNvalue, color.blue>>8, NULL);
    
} /* end select_color_callback() */



/* change the red values of the currently selected colors */
void red_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor color;
    Pixel pixel;
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *)call_data;

    if(current_toggle == NULL)
        return;

    XtVaGetValues(swatch, XmNbackground, &pixel, NULL);

    if(dynamic_color) {
      color.red = cbs->value << 8; 
      color.pixel = pixel;
      color.flags = DoRed;
      XStoreColor(XtDisplay(w), cmap, &color);
 
      /* we need to set the widgets to a different color and then set them back
       * so that they reflect the color change
       */
      if(pixel == display_colors[0].pixel) {
          XtVaSetValues(swatch, XmNbackground, display_colors[1].pixel, NULL);
          XtVaSetValues(current_toggle, XmNbackground, display_colors[1].pixel, NULL);
      } else {
          XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel, NULL);
          XtVaSetValues(current_toggle, XmNbackground, display_colors[0].pixel, NULL);
      }
      
      XtVaSetValues(swatch, XmNbackground, pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, pixel, NULL);
      
    } else { /* else not dynamic_color */
      color.pixel = pixel;
      color.flags = DoRed | DoGreen | DoBlue;
      XQueryColor(XtDisplay(w), cmap, &color);
      color.red = cbs->value << 8; 
      XAllocColor(XtDisplay(w), cmap, &color);
      
      display_colors[current_index].pixel = color.pixel;

      XtVaSetValues(swatch, XmNbackground, color.pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, color.pixel, NULL);
    }
    
    unsaved_edits = TRUE;
     
    
} /* end red_slider_change_callback() */



/* change the green values of the currently selected colors */
void green_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor color;
    Pixel pixel;
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *)call_data;

    if(current_toggle == NULL)
        return;

    XtVaGetValues(swatch, XmNbackground, &pixel, NULL);
    color.green = cbs->value << 8;
    color.pixel = pixel;
    color.flags = DoGreen;
    if(dynamic_color) {
      XStoreColor(XtDisplay(w), cmap, &color);

      /* we need to set the widgets to a different color and then set them back
       * so that they reflect the color change
       */
      if(pixel == display_colors[0].pixel) {
        XtVaSetValues(swatch, XmNbackground, display_colors[1].pixel, NULL);
        XtVaSetValues(current_toggle, XmNbackground, display_colors[1].pixel, NULL);
      } else {
        XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel, NULL);
        XtVaSetValues(current_toggle, XmNbackground, display_colors[0].pixel, NULL);
      }
      XtVaSetValues(swatch, XmNbackground, pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, pixel, NULL);
      
    } else { /* else not dynamic_color */
      color.pixel = pixel;
      color.flags = DoRed | DoGreen | DoBlue;
      XQueryColor(XtDisplay(w), cmap, &color);
      color.green = cbs->value << 8; 
      XAllocColor(XtDisplay(w), cmap, &color);
      
      display_colors[current_index].pixel = color.pixel;

      XtVaSetValues(swatch, XmNbackground, color.pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, color.pixel, NULL);
    }
    
    unsaved_edits = TRUE;
     
    
} /* end green_slider_change_callback()  */



/* change the blue values of the currently selected colors */
void blue_slider_change_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor color;
    Pixel pixel;
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *)call_data;

    if(current_toggle == NULL)
        return;

    XtVaGetValues(swatch, XmNbackground, &pixel, NULL);
    color.blue = cbs->value << 8;
    color.pixel = pixel;
    color.flags = DoBlue;
    if(dynamic_color) {
      XStoreColor(XtDisplay(w), cmap, &color);

      /* we need to set the widgets to a different color and then set them back
       * so that they reflect the color change
       */
      if(pixel == display_colors[0].pixel) {
        XtVaSetValues(swatch, XmNbackground, display_colors[1].pixel, NULL);
        XtVaSetValues(current_toggle, XmNbackground, display_colors[1].pixel, NULL);
      } else {
        XtVaSetValues(swatch, XmNbackground, display_colors[0].pixel, NULL);
        XtVaSetValues(current_toggle, XmNbackground, display_colors[0].pixel, NULL);
      }
      XtVaSetValues(swatch, XmNbackground, pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, pixel, NULL);
      
    } else { /* else not dynamic_color */
      color.pixel = pixel;
      color.flags = DoRed | DoGreen | DoBlue;
      XQueryColor(XtDisplay(w), cmap, &color);
      color.blue = cbs->value << 8; 
      XAllocColor(XtDisplay(w), cmap, &color);
      
      display_colors[current_index].pixel = color.pixel;

      XtVaSetValues(swatch, XmNbackground, color.pixel, NULL);
      XtVaSetValues(current_toggle, XmNbackground, color.pixel, NULL);
    }
    
    unsaved_edits = TRUE;
     
     
} /* end blue_slider_change_callback */



/**************************************************************************/



/* quit the program */

void exit_program_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget d;
  char buf[120];
  XmString xmstr, oklabel, cnxlabel;
  
  if(unsaved_edits==TRUE) {
  
      sprintf(buf, "\nThere are edits to this palette that have not been saved.\n"
                   "\nDo you want to Exit Program without saving?\n ");
      xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
      oklabel = XmStringCreateLtoR("YES", XmFONTLIST_DEFAULT_TAG);
      cnxlabel = XmStringCreateLtoR(" NO ", XmFONTLIST_DEFAULT_TAG);
      
      d = XmCreateQuestionDialog(w, " ", NULL, 0);
      XtVaSetValues(XtParent(d), XmNtitle, " ", NULL);
      XtVaSetValues(d, XmNmessageString, xmstr, 
                        XmNokLabelString,     oklabel,
                        XmNcancelLabelString, cnxlabel,
                        XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
                        NULL);
   
      
      XtAddCallback(d, XmNcancelCallback, exit_cancel_callback, NULL);
      XtAddCallback(d, XmNokCallback, exit_ok_callback, NULL);
      
      XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
      
      XmStringFree(xmstr);
      XmStringFree(oklabel);
      XmStringFree(cnxlabel);
  
      XtManageChild(d);

  } else {
    
      exit_program();
    
  }

            
} /*  end close_callback() */



void exit_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

   ;
   
    
}


/* exit program  */
void exit_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

  exit_program();
  
    
} /* exit_ok_callback() */


void exit_program() {
    
   XtDestroyWidget(shell);
    exit(0);
    
    
} /* end exit_program() */






/**************************************************************************/





void save_palette_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget d;
  char buf[120];
  XmString xmstr, oklabel, cnxlabel;
  
  
    sprintf(buf, "\nDo you want to save the existing changes to the file:\n"
                 "\n%s\n ", palette_filename);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    oklabel = XmStringCreateLtoR("YES", XmFONTLIST_DEFAULT_TAG);
    cnxlabel = XmStringCreateLtoR(" NO ", XmFONTLIST_DEFAULT_TAG);
    
    d = XmCreateQuestionDialog(w, " ", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, " ", NULL);
    XtVaSetValues(d, XmNmessageString, xmstr, 
                      XmNokLabelString,     oklabel,
                      XmNcancelLabelString, cnxlabel,
                      XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
                      NULL);
 
    
    XtAddCallback(d, XmNcancelCallback, save_palette_cancel_callback, NULL);
    XtAddCallback(d, XmNokCallback, save_palette_ok_callback, NULL);
    
    XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
    
    XmStringFree(xmstr);
    XmStringFree(oklabel);
    XmStringFree(cnxlabel);

    XtManageChild(d);

            
} /*  end save_palette_callback() */



void save_palette_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    
}



/* save the palette back to disk */
void save_palette_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    FILE *palette_file; 


    /* open output file */
    if( (palette_file = fopen(palette_filename, "w")) == NULL ) {
        fprintf(stderr, "Could not open file %s\n", palette_filename);
        return;
    }

    write_to_palette(palette_file);

    fclose(palette_file);
    
    unsaved_edits = FALSE;
    
    
} /* save_palette_ok_callback() */







/* CVG 9.1 */
void save_palette_as_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    /* This dialog selects the new filename */
    /* All of the work is done by the save_as_ok_callback */

    Widget d;
    XmString xmfile, xmdir;
    char dirname[255];
    
    int i;
    
    
    
    if( (palette_filename == NULL) || (strcmp(palette_filename, " ") == 0 ) ) {
        
        fprintf(stderr,"NOTE **********************************************\n"
                       "*    Must Have a palette opened before saving     *\n"
                       "***************************************************\n");
        return;
    }
    
    
    d = XmCreateFileSelectionDialog(w, "save_palette_as_dialog", NULL, 0);

    XtVaSetValues(XtParent(d), XmNtitle, "Save File As...", 
          XmNwidth,                560,
          XmNheight,               380,
          XmNx,                    10,
          XmNy,                    10,
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


    /* set the currently selected file to the current palette file, and the
     * current directory to the directory that it is in
     */
    xmfile = XmStringCreateLtoR(palette_filename, XmFONTLIST_DEFAULT_TAG);
    i = strlen(palette_filename) - 1;
    while( (palette_filename[i] != '/') && (i > 0) ) 
        i--;
    
    strncpy(dirname, palette_filename, i+2);  /* include the '/' */
    dirname[i+1] = '\0';
    xmdir = XmStringCreateLtoR(dirname, XmFONTLIST_DEFAULT_TAG);
    

   
    XtVaSetValues(d, 
          XmNdirSpec,          xmfile,
          XmNdirectory,        xmdir,
          NULL);
    XmStringFree(xmdir);
    XmStringFree(xmfile);
    
    XtAddCallback(d, XmNokCallback, save_as_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, save_as_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, fileselect_help_callback, NULL); 

    XtManageChild(d);

    
} /* end save_palette_as_callback() */




void save_as_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

XmFileSelectionBoxCallbackStruct *cbs = 
                                 (XmFileSelectionBoxCallbackStruct *)call_data;
                                 
char *filename;
char palette_name[60];

 FILE *palette_file;
int j;



    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
    

    strcpy(palette_filename, filename);


    if( (palette_file = fopen(palette_filename, "w")) == NULL ) {
        fprintf(stderr, "Could not open file: '%s'\n", filename);
        if(filename!=NULL)
           free(filename);
        return;
    }

    
    j = strlen(palette_filename) - 1;
    while( (palette_filename[j] != '/') && (j > 0) ) 
        j--;
    strcpy(palette_name, (palette_filename + j+1) );
    sprintf(titleString,"Editor for CVG Color Palettes - %s", palette_name);

    XtVaSetValues(shell, XmNtitle, titleString, NULL);

    XtVaSetValues(palettefile_text, XmNvalue, filename, NULL);
    XtUnmanageChild(w);
    free(filename);


    /* open output file */
    if( (palette_file = fopen(palette_filename, "w")) == NULL ) {
        fprintf(stderr, "Could not open file %s\n", palette_filename);
        return;
    }
    
    write_to_palette(palette_file);

    fclose(palette_file);
    
    unsaved_edits = FALSE;
    

} /* end save_as_ok_callback() */




void save_as_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
    
}


/**************************************************************************/



/* CVG 9.1 */
void new_palette_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

    /* This dialog selects the new filename */
    /* All of the work is done by the new_ok_callback */    

    Widget size_d;
    XmString xmstr;

    

    /* PROMPT USER FOR THE PALETTE SIZE HERE */

   size_d = XmCreatePromptDialog(w, "set_plt_size", NULL, 0);
    XtVaSetValues(XtParent(size_d), XmNtitle, "Palette Size", NULL);
    xmstr = XmStringCreateLtoR("1-256:", XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(size_d, 
             XmNselectionLabelString, xmstr, 
             XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, 
             XmNx,           20,
             XmNy,           5,
             NULL);
    XtAddCallback(size_d, XmNcancelCallback, size_set_cancel_callback, NULL);
    XtAddCallback(size_d, XmNokCallback, size_set_ok_callback, NULL);
    
    XtUnmanageChild(XmSelectionBoxGetChild(size_d, XmDIALOG_HELP_BUTTON));
    
    XmStringFree(xmstr);

    XtManageChild(size_d);


    /* OPENING FILE SELECT DIALOG is in size_set_ok_callback */    


} /* end new_palette_callback() */




/* CVG 9.1 */
void new_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

XmFileSelectionBoxCallbackStruct *cbs = 
                                 (XmFileSelectionBoxCallbackStruct *)call_data;
                                 


char *filename;
char palette_name[60];

FILE *palette_file;
int j;

short my_red, my_green, my_blue;



    my_red = 245;
    my_green = 245;
    my_blue = 245;


    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);
    

    strcpy(palette_filename, filename);

    palette_size = new_palette_size;


    /* open output file */
    if( (palette_file = fopen(palette_filename, "w")) == NULL ) {
        fprintf(stderr, "Could not open file %s\n", palette_filename);
        return;
    }
    /* first, we output the number of colors in the palette */
    write_my_color_to_palette(palette_file, my_red, my_green, my_blue);
    
    fclose(palette_file);

    /* open output file */
    if( (palette_file = fopen(palette_filename, "r")) == NULL ) {
        fprintf(stderr, "Could not open file %s\n", palette_filename);
        return;
    }
    
    read_palette(palette_file);
    fclose(palette_file);

    
    j = strlen(palette_filename) - 1;
    while( (palette_filename[j] != '/') && (j > 0) ) 
        j--;
    strcpy(palette_name, (palette_filename + j+1) );
    sprintf(titleString,"Editor for CVG Color Palettes - %s", palette_name);

    XtVaSetValues(shell, XmNtitle, titleString, NULL);

    XtVaSetValues(palettefile_text, XmNvalue, filename, NULL);
    XtUnmanageChild(w);
    free(filename);
    


    if(resize_mode == VARIABLE_SIZE) { 
                                       
        remove_toggles(FALSE);         
                                       
        create_toggles();              
                                       
        color_toggles();               
                                       
    } else { /* FIXED SIZE */
                                       
        set_toggles_insensitive();     
                                       
        set_toggles_sensitive();       
                                       
        color_toggles();               
                                       
    }  /* end else FIXED_SIZE */

    unsaved_edits = FALSE;
    
    
} /* end new_ok_callback */




/* CVG 9.1 */
void new_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
 
 
    XtUnmanageChild(w);
    
}




void size_set_ok_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{ 
  XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;

  char *size_str;
  int size_int;
  Widget d;
  XmString xmstr;
  
  char sub[25];
  int k, j;

  Widget file_d;
  XmString xmfile, xmdir;
  char color_dir[255];



    /* get the new pid in string form */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &size_str);
    
    
        /* find a number and complain if it isn't one */
    /* skip over any initial spaces */
    k=0;
    while(size_str[k] == ' ') k++;

    /* read in an integer (the prod id) */
    j=0;
    while(isdigit((int)(size_str[k]))) 
        sub[j++] = size_str[k++];
    sub[j] = '\0';
    free(size_str);
    
    /* check if we got digits */
    if(j==0) {
        d = XmCreateErrorDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR(
                   "The entered value is not a number.\nPlease try again.",
                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
        
    } else 
        size_int = atoi(sub);

    
    /* check to see if the number is within the range produced by the radar */
    if( !((size_int >0) && (size_int <= 256)) ) {
        d = XmCreateInformationDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR(
                       "The specified product ID is beyond the valid range.\n"
                       "Enter another number (1-256).",
                       XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
    }    
    
    new_palette_size = size_int;
    
    
    XtUnmanageChild(w);
    
    
    file_d = XmCreateFileSelectionDialog(w, "create_new_palette_dialog", NULL, 0);

    XtVaSetValues(XtParent(file_d), XmNtitle, "Create New File...", 
          XmNwidth,                560,
          XmNheight,               380,
          XmNx,                    10,
          XmNy,                    10,
          XmNallowShellResize,     FALSE,      
          NULL);

    XtVaSetValues(file_d, 
    
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


    /* set the currently selected file to the current palette file, and the
     * current directory to the directory that it is in
     */
    
    sprintf(color_dir,"%s/colors",config_dir);
    xmfile = XmStringCreateLtoR(" ", XmFONTLIST_DEFAULT_TAG); 
    xmdir = XmStringCreateLtoR(color_dir, XmFONTLIST_DEFAULT_TAG);
     

   
    XtVaSetValues(file_d, 
          XmNdirSpec,          xmfile,
          XmNdirectory,        xmdir,
          NULL);
    XmStringFree(xmdir);
    XmStringFree(xmfile);
    
    XtAddCallback(file_d, XmNokCallback, new_ok_callback, NULL);
    XtAddCallback(file_d, XmNcancelCallback, new_cancel_callback, NULL);
    XtAddCallback(file_d, XmNhelpCallback, fileselect_help_callback, NULL); 

    XtManageChild(file_d);
    
    
    
} /* end palette_size_set_ok_callback() */




void size_set_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    XtUnmanageChild(w);
    
}



/**************************************************************************/



void close_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget d;
  char buf[120];
  XmString xmstr, oklabel, cnxlabel;
  
  if(unsaved_edits==TRUE) {
  
      sprintf(buf, "\nThere are edits to this palette that have not been saved.\n"
                   "\nDo you want to Close without saving?\n ");
      xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
      oklabel = XmStringCreateLtoR("YES", XmFONTLIST_DEFAULT_TAG);
      cnxlabel = XmStringCreateLtoR(" NO ", XmFONTLIST_DEFAULT_TAG);
      
      d = XmCreateQuestionDialog(w, " ", NULL, 0);
      XtVaSetValues(XtParent(d), XmNtitle, " ", NULL);
      XtVaSetValues(d, XmNmessageString, xmstr, 
                        XmNokLabelString,     oklabel,
                        XmNcancelLabelString, cnxlabel,
                        XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
                        NULL);
   
      
      XtAddCallback(d, XmNcancelCallback, close_cancel_callback, NULL);
      XtAddCallback(d, XmNokCallback, close_ok_callback, NULL);
      
      XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
      
      XmStringFree(xmstr);
      XmStringFree(oklabel);
      XmStringFree(cnxlabel);
  
      XtManageChild(d);

  } else {
    
      close_plt();
    
  }

            
} /*  end close_callback() */



void close_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

   ;
   
    
} /* end close_cancel_callback() */


/* close the palette  */
void close_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{

  close_plt();
  
    
} /* close_ok_callback() */


void close_plt() {
    
int destroy_all = TRUE;
    

    if(resize_mode == VARIABLE_SIZE) { 
                                       
        remove_toggles(destroy_all);   
                                       
    } else { /* FIXED SIZE */
                                       
        set_toggles_insensitive();     
                                       
    }  /* end else FIXED_SIZE */

    
    palette_filename[0] = ' '; 
    palette_filename[1] = '\0'; 
    
    palette_size=0;
    new_palette_size=0;
    prev_palette_size=0;
    
    
    sprintf(titleString,"Editor for CVG Color Palettes - ");

    XtVaSetValues(shell, XmNtitle, titleString, NULL);
    
    XtVaSetValues(palettefile_text, XmNvalue, " ", NULL);

    unsaved_edits=TRUE;
    
    
}  /* end close_plt()  */




/**************************************************************************/


/* CVG 9.1 */
void fileselect_help_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, text, scroll;


/*--------------------------------------------------------------------------*/
char *help_str = 
"                                                                   \n"
"-------------- COLOR PALETTE FILE SELECTION DIALOG ----------------\n"
"                                                                   \n"
"This dialog box allows selection of a palette file to be edited    \n"
"from the choose file or the new file dialog or the name of the     \n"
"file to saved to from the save as dialog.                          \n"
"                                                                   \n"
"                                                                   \n"
"FILTER EDIT BOX                                                    \n"
"                                                                   \n"
"Controls the files listed in the dialog box.  A simple regular     \n"
"expression is used to filter the file names in the current         \n"
"directory.  For example, '/long/current/path/name/*.plt' will only \n"
"display the palette files in the current directory.                \n"
"                                                                   \n"
"                                                                   \n"
"DIRECTORIES SELECTION LIST                                         \n"
"                                                                   \n"
"Displays the subdirectories in the current directory.              \n"
"                                                                   \n"
"                                                                   \n"
"FILES SELECTION LIST                                               \n"
"                                                                   \n"
"Displays the files in the current directory, filtered by the       \n"
"expression in the Filter field.                                    \n"
"                                                                   \n"
"                                                                   \n"
"SELECTION EDIT BOX                                                 \n"
"                                                                   \n"
"The currently selected file or directory.  This text field can     \n"
"also be edited by hand, if desired.                                \n"
"                                                                   \n"
"                                                                   \n"
"OK BUTTON                                                          \n"
"                                                                   \n"
"Selects the file whose full pathname is specified by the contents  \n"
"of the selection edit box.                                         \n"
"                                                                   \n"
"                                                                   \n"
"FILTER BUTTON                                                      \n"
"                                                                   \n"
"Redisplays the contents of the files selection list using the      \n"
"expression specified in the filter edit box.                       \n"
"                                                                   \n"
"                                                                   \n"
"CANCEL BUTTON                                                      \n"
"                                                                   \n"
"Closes the dialog without selecting a file.                        \n"
"                                                                   \n";

/*--------------------------------------------------------------------------*/


    d = XmCreateFormDialog(w, "help_dialog", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Help", NULL);

    text = XmCreateScrolledText(d, "help_text", NULL, 0);
    XtVaSetValues(XtParent(text),
          XmNwidth,             650,
          XmNheight,            400,
          XmNx,                    30,
          XmNy,                    30,
          XmNtopAttachment,     XmATTACH_FORM,
          XmNleftAttachment,    XmATTACH_FORM,
          XmNrightAttachment,   XmATTACH_FORM,
          XmNbottomAttachment,  XmATTACH_FORM,
          NULL);
    XtVaSetValues(text, XmNeditMode,  XmMULTI_LINE_EDIT,
                XmNeditable,  False,
                XmNvalue,     help_str,
                NULL);

    XtVaGetValues(XtParent(text), XmNverticalScrollBar, &scroll, NULL);
    XtManageChild(scroll);

    XtManageChild(text);
    XtManageChild(d);

 
} /* end fileselect_help_callback() */


