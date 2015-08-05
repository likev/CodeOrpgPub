/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:51 $
 * $Id: png_output.c,v 1.8 2009/05/15 17:52:51 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/* png_output.c */

#include "png_output.h"

int color_found;

/* pops up a dialog to choose the location of an output file */
void png_output_file_select_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, e;
    XmString xmdir, xmstr;
    static char *helpfile = HELP_FILE_PNG_OUT;


    if( check_png_libs(standalone_flag) == FALSE ) {
        
       e = XmCreateInformationDialog(XtParent(XtParent(XtParent(XtParent(XtParent(w))))), 
                        "Missing Libraries", NULL, 0);
       xmstr = XmStringCreateLtoR(
           "At least one of the three libraries required for this function are missing.\n"\
           "     libgd.so, libpng.so and libz.so must be installed in /usr/lib on Linux \n"
           "     or installed in /usr/local/lib on Solaris\n\n"
           "See terminal window for the identity of the missing libraries.",  
           XmFONTLIST_DEFAULT_TAG);
       XtVaSetValues(XtParent(e), XmNtitle, "Required Libraries Not Installed", NULL);
       XtVaSetValues(e, XmNmessageString, xmstr, NULL);
       XmStringFree(xmstr);
       XtUnmanageChild(XmMessageBoxGetChild(e, XmDIALOG_HELP_BUTTON));
       XtUnmanageChild(XmMessageBoxGetChild(e, XmDIALOG_CANCEL_BUTTON));
       XtManageChild(e);
       
       return;
               
    }




    if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell1) {
        psd = sd1;
        sd = sd1;       

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell2) {
        psd = sd2;
        sd = sd2;        

    } else if(XtParent(XtParent(XtParent(XtParent(XtParent(w))))) == dshell3) {
        psd = sd3;
        sd = sd3;        
    }




/* don't try to export an empty or cleared screen */

if(psd->layers == NULL || psd->history == NULL)
     return;

    d = XmCreateFileSelectionDialog(w, "choose_pngfile_dialog", NULL, 0);
    
    XtVaSetValues(XtParent(d), XmNtitle, "Choose PNG Output File...", 
         XmNwidth,                560,
         XmNheight,               380,
         XmNallowShellResize,     FALSE,
         NULL);
         
    XtVaSetValues(d, 
         XmNdialogStyle,     XmDIALOG_FULL_APPLICATION_MODAL, 
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

    /* if we've saved a directory that we've loaded something from,
     * use that as the current directory
     */
    if(disk_last_filename[0] != '\0') {
        xmdir = XmStringCreateLtoR(disk_last_filename, XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNdirectory, xmdir, NULL);
        XmStringFree(xmdir);
    }


    XtAddCallback(d, XmNokCallback, png_output_file_ok_callback, NULL);
    XtAddCallback(d, XmNcancelCallback, png_output_file_cancel_callback, NULL);
    XtAddCallback(d, XmNhelpCallback, help_window_callback, helpfile);

    XtManageChild(d);
    
}



  /* check presence of required libraries    */
  /* function has some dependency on cvg.mak */
int check_png_libs(int install_type)
{

  
struct stat st;

char png_filename[256];
char z_filename[256];
/* CVG 9.0 */
char gd_filename[256];

int all_available = TRUE;    

/* // Currently there is no difference between the standalone and */
/* // normal installations, both use the same dynamic libraries */
/* //    if(install_type == TRUE) { // STANDALONE   */
/* //       */
/* //      ; */
/* //        */
/* //    }  else { // NORMAL LOCAL INSTALLATION */

      /*  the needed filenames are dependent upon the cvg.mak file */
#ifdef LITTLE_ENDIAN_MACHINE
      sprintf(png_filename, "/usr/lib/libpng.so");
      sprintf(z_filename, "/usr/lib/libz.so"); 
      /* CVG 9.0 */
      sprintf(gd_filename, "/usr/lib/libgd.so");

#else
      sprintf(png_filename, "/usr/local/lib/libpng.so");
      sprintf(z_filename, "/usr/local/lib/libz.so"); 
      /* CVG 9.0 */
      sprintf(gd_filename, "/usr/local/lib/libgd.so"); 

#endif

    
      if(stat(png_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR\n");
          fprintf(stderr, " Required library %s is not present.\n", png_filename);
          all_available = FALSE;
      }
      
      if(stat(z_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR*\n");
          fprintf(stderr, " Required library %s is not present.\n", z_filename);
          all_available = FALSE;
      }
      
      if(stat(gd_filename, &st) < 0) {
          fprintf(stderr, " ERROR*****************ERROR************ERROR*\n");
          fprintf(stderr, " Required library %s is not present.\n", gd_filename);
          all_available = FALSE;
      }
 

      
    if(all_available == TRUE) {
         return TRUE;
          
    } else {
          return FALSE;
    }    
    
}










void png_output_file_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}




/* pass on the selected output file name, doing any necessary preprocessing */
void png_output_file_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
    char *filename;
    int len; 

    
    /* get the selected file name */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename);

    /* save the directory where this file is in */
    len = strlen(filename);
    while( (filename[len] != '/') && (len > 0) ) len--;
    strncpy(disk_last_filename, filename, len+2);  /* include the '/' */
    disk_last_filename[len+1] = '\0';


    /* output the image to a disk file */
    output_image_to_png(filename);


    XtUnmanageChild(w);

}




/* dumps the contents of the global variable pixmap to a specified file */
void output_image_to_png(char *filename)
{
    gdImagePtr outimage;      /* temp image for output */
    int png_colors[256];      /* we can handle up to 256 colors */
    int i, j, color;
    XImage *the_image;
    FILE *pngfile;

/* CVG 9.0 diagnostic */
unsigned int color_not_found = 0;

    /* initialize the image */
    if((outimage = gdImageCreate(pwidth+barwidth, pheight)) == NULL)
         return;

    /* open an output file */
    if((pngfile = fopen(filename, "w")) == NULL) {
        printf("output file could not be opened\n");
        /* mem leak fix */
        gdImageDestroy(outimage); 
        return;
    }

    /* convert the current palette to one that can be outputted */

    if(psd==sd1) {
        
        for(i=0; i<global_palette_size_1; i++)
            png_colors[i] = gdImageColorAllocate(outimage, 
                            global_display_colors_1[i].red,
                            global_display_colors_1[i].green,
                            global_display_colors_1[i].blue);
    }

    else if(psd==sd2) {
        for(i=0; i<global_palette_size_2; i++)
            png_colors[i] = gdImageColorAllocate(outimage, 
                            global_display_colors_2[i].red,
                            global_display_colors_2[i].green,
                            global_display_colors_2[i].blue);
    }

    else if(psd==sd3) {
        for(i=0; i<global_palette_size_3; i++)
            png_colors[i] = gdImageColorAllocate(outimage, 
                            global_display_colors_3[i].red,
                            global_display_colors_3[i].green,
                            global_display_colors_3[i].blue);
    }


/* CVG 9.0 - ELIMINATE THE KLUDGE OF ADDING COLOR WHITE HERE */
/*    if(psd==sd1) {                                                                */
/*        if(global_palette_size_1<256)                                             */
/*            png_colors[global_palette_size_1] =                                   */
/*                                    gdImageColorAllocate(outimage, 255, 255, 255);*/
/*    }                                                                             */
/*                                                                                  */
/*    else if(psd==sd2) {                                                           */
/*        if(global_palette_size_2<256)                                             */
/*            png_colors[global_palette_size_2] =                                   */
/*                                    gdImageColorAllocate(outimage, 255, 255, 255);*/
/*    }                                                                             */
/*                                                                                  */
/*    else if(psd==sd3) {                                                           */
/*        if(global_palette_size_3<256)                                             */
/*            png_colors[global_palette_size_3] =                                   */
/*                                    gdImageColorAllocate(outimage, 255, 255, 255);*/
/*    }                                                                             */


    /* grab a copy of the pixmap's data */

    if((the_image = XGetImage(display, psd->pixmap, 0, 0, pwidth+barwidth, pheight,
                  AllPlanes, ZPixmap))==NULL) {
        printf("Could not get pixmap image.\n");
        /* mem leak fix */
        gdImageDestroy(outimage);
        fclose(pngfile);
        return;
    }

    /* now, copy the image pixel by pixel (yay, slow) */
    for(i=0; i<the_image->height; i++)
      for(j=0; j<the_image->width; j++) {
          /* get the index into our color arrays */
          color = global_pixel_to_color(XGetPixel(the_image, j, i), &color_found);
          if(color_found==1 || palette_size==256) {
              gdImageSetPixel(outimage, j, i, png_colors[color]);

          } else { /* we added the color white and we need to use it */
             /* CVG 9.0 - diagnostic */
             if(color_not_found == 1)
                fprintf(stderr,
                          "PNG OUTPUT - color not found, using the last color\n");
             color_not_found++;
             gdImageSetPixel(outimage, j, i, png_colors[palette_size]);

          } 
      } /* end for j */

    if(color_not_found > 0)
        fprintf(stderr,"PNG OUTPUT - color not found %d times\n", color_not_found);
    
    /* write the file out */
    gdImagePng(outimage, pngfile);

    /* cleanup */
    fclose(pngfile);
    /* mem leak fix */
    gdImageDestroy(outimage);
    XDestroyImage(the_image);


}

