/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/11 22:35:43 $
 * $Id: help.c,v 1.3 2004/03/11 22:35:43 cheryls Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* help.c */
/* Functions that deal with online help */

#include "help.h"

/* Opens a dialog box containing a scrolled text field to display
 * the contents of a helpful text file.  The file name given 
 * should be contained in a malloc'ed string
 */
void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d, text, scroll;
    char *help_str = NULL, fullfilename[400], *filename = (char *)client_data;
    FILE *help_file;
    int in_size, total_size = 0;
    
struct stat st;

    /* figure out the path to the help file */
    sprintf(fullfilename, "%s/help/%s", config_dir, filename);

    if(verbose_flag)
        fprintf(stderr,"help file: %s  (%s)\n", fullfilename, filename);

    /* attempt to read in the help text */
    if((help_file = fopen(fullfilename, "r")) == NULL) {
        fprintf(stderr,"Could not open help file\n");
	return;
    }

    /* GET SIZE OF FILE */
    stat(fullfilename,&st);
    total_size=st.st_size;
    
    help_str = (char *)malloc(sizeof(char)*(total_size+1));
    in_size = fread(help_str, 1, total_size, help_file);
    
    if( (feof(help_file) != 0) || (in_size < total_size) )  {
        fprintf(stderr,"ERROR - Did not read complete helpfile. Bytes read is %d\n",
                       in_size);
        fprintf(stderr,"        Expected filesize is %d\n", total_size);
    }

    help_str[in_size] = '\0';

   
    /* make dialog */
    d = XmCreateFormDialog(w, "help_dialog", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Help", NULL);

    text = XmCreateScrolledText(d, "help_text", NULL, 0);
    XtVaSetValues(XtParent(text),
		  XmNwidth,             650,
		  XmNheight,            400,
		  XmNtopAttachment,     XmATTACH_FORM,
		  XmNleftAttachment,    XmATTACH_FORM,
		  XmNrightAttachment,   XmATTACH_FORM,
		  XmNbottomAttachment,  XmATTACH_FORM,
		  NULL);
    XtVaSetValues(text, XmNeditMode,  XmMULTI_LINE_EDIT,
		        XmNeditable,  False,
		        XmNvalue,     help_str,
		        NULL);
    free(help_str);

    XtVaGetValues(XtParent(text), XmNverticalScrollBar, &scroll, NULL);
    XtManageChild(scroll);

    XtManageChild(text);
    XtManageChild(d);

 
}


