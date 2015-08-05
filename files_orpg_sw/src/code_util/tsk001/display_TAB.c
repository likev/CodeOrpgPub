/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:54 $
 * $Id: display_TAB.c,v 1.9 2008/03/13 22:45:54 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* display_TAB.c */


#include "display_TAB.h"

/* the following are  file scope because they are not part of call data */
char tab_buf[1500];
int num_tab_pages=0;  
int tab_msg_type;

int tabx, taby;

static tab_info *call_info1=NULL, *call_info2=NULL;
static tab_info *call_info3=NULL;


  
void display_TAB(int offset) 
{
  /* display information contained within the
   Tabular Alphanumeric Block */

  XmString xmstr;

  Widget tab_text, prev_but, next_but, tabform;

  int begin_offset = offset;
  
  tab_info *call_info=NULL;
    
  int  *type_ptr;  
  int ret; 
  
  int prod_screen; 
  screen_data *tab_sd;
 
  Prod_header *hdr; 
  Graphic_product *gp=NULL;


    if(sd==sd1) {
        prod_screen=1;
        tab_sd=sd1;
    } else if(sd==sd2) {
        prod_screen=2;
        tab_sd=sd2;
    } else if(sd==sd3) {
        prod_screen=3;
        tab_sd=sd3;
    } else {
/*  CVG 8.5 */
        fprintf(stderr,"ERROR Display TAB - sd has invalid value\n");
        return;
    }

/*  CVG 8.5 */
    if(tab_sd->icd_product==NULL) {
        fprintf(stderr,"ERROR Display TAB - no produict loaded\n");
        return;
    }
    
    /* load the Pre-ICD Header */
    hdr=(Prod_header*)tab_sd->icd_product;
    /* is this a TAB or SATAP ?*/
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);    
/*  CVG 8.5 */
    if(type_ptr==NULL) {
        fprintf(stderr,"ERROR Display TAB, unable to read message type for ID %d\n",
                                                                    hdr->g.prod_id);
        fprintf(stderr,"                   assuming type 0 and continuing!\n");
        tab_msg_type = 0;
    } else {
        tab_msg_type = *type_ptr;
    }




  /* start with page 1 */
  ret = get_tab_page(offset,1,prod_screen);
  
  if(ret==FALSE) {
      fprintf(stderr," ERROR - Unable to retrieve the first TAB Page.\n");
      return;
  }




/* ----------------------- OPEN NEW TAB WINDOW ---------------------- */
    if(tab_sd->tab_window == NULL) {         
      /* create the window, and add a couple buttons for paging */
      tab_sd->tab_window = XtVaCreatePopupShell("Tabular Alphanumeric Block", 
            topLevelShellWidgetClass, shell,
            XmNwidth,            tabwidth,
            XmNheight,           tabheight,
            
/*  BUG FIX CVG 7.1.1 */
/*            XmNmwmDecorations,    MWM_DECOR_RESIZEH, */
           XmNmwmDecorations,    MWM_DECOR_BORDER^MWM_DECOR_MINIMIZE^MWM_DECOR_TITLE^MWM_DECOR_MENU,
/*  limit available window functions */
           XmNmwmFunctions,      MWM_FUNC_CLOSE^MWM_FUNC_MINIMIZE^MWM_FUNC_MOVE,
                                
            NULL);

          if(prod_screen==1) {
/*               if( (taby=715+45) > (disp_height-tabheight-10) ) {                */
/*                 taby=disp_height-tabheight-10;                 */
/*               } */
/*               tabx=50; */
            
              XtVaSetValues(tab_sd->tab_window,
                  XmNx,         tab1x,
                  XmNy,         tab1y,
                  NULL);
              if(tab_msg_type == STANDALONE_TABULAR) 
                  XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Screen 1",
                     NULL);
              else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Screen 1",
                     NULL);
                     
          } else if(prod_screen==2) { 
/*               if( (taby=715+45) > (disp_height-tabheight-10) ) {                */
/*                 taby=disp_height-tabheight-10;                 */
/*               } */
/*               if( (tabx=615+20+10+30) > (disp_width-tabwidth-10) ) {                */
/*                 tabx=disp_width-tabwidth-10;                 */
/*               } */
              
              XtVaSetValues(tab_sd->tab_window, 
                 XmNx,         tab2x,
                 XmNy,         tab2y,    
                 NULL);
             if(tab_msg_type == STANDALONE_TABULAR)
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Screen 2",
                     NULL);
             else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Screen 2",
                     NULL);
                     
          } else if(prod_screen==3) { 
/*               if( (taby=715+45) > (disp_height-tabheight-10) ) {                */
/*                 taby=disp_height-tabheight-10;                 */
/*               } */
/*               if( (tabx=615+20+10+30) > (disp_width-tabwidth-10) ) {                */
/*                 tabx=disp_width-tabwidth-10;                 */
/*               } */
              
              XtVaSetValues(tab_sd->tab_window, 
                 XmNx,         tab3x,
                 XmNy,         tab3y,    
                 NULL);
             if(tab_msg_type == STANDALONE_TABULAR)
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Aux Screen",
                     NULL);
             else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Aux Screen",
                     NULL);
          }
      
      
      

/* DEBUG */
/* fprintf(stderr,"DEBUG TAB %d X IS %d, Y IS %d \n", prod_screen, tabx, taby); */

      
      tabform = XtVaCreateManagedWidget("form", 
                    xmFormWidgetClass, tab_sd->tab_window, NULL);
      
      prev_but = XtVaCreateManagedWidget("Previous Page",
             xmPushButtonWidgetClass, tabform,
         XmNtopAttachment,     XmATTACH_FORM,
             XmNtopOffset,         2,
             XmNleftAttachment,    XmATTACH_FORM,
             XmNleftOffset,        2,
             XmNrightAttachment,   XmATTACH_NONE,
             XmNbottomAttachment,  XmATTACH_NONE,
         NULL);
      next_but = XtVaCreateManagedWidget("Next Page",
             xmPushButtonWidgetClass, tabform,
         XmNtopAttachment,     XmATTACH_FORM,
             XmNtopOffset,         2,
             XmNleftAttachment,    XmATTACH_WIDGET,
         XmNleftWidget,        prev_but,
             XmNleftOffset,        5,
             XmNrightAttachment,   XmATTACH_NONE,
             XmNbottomAttachment,  XmATTACH_NONE,
        NULL);
      tab_text = XtVaCreateManagedWidget("",
         xmLabelWidgetClass,  tabform,
         XmNalignment,        XmALIGNMENT_BEGINNING,
             XmNtopAttachment,    XmATTACH_WIDGET,
         XmNtopWidget,        prev_but,
             XmNtopOffset,        5,
             XmNleftAttachment,   XmATTACH_FORM,
             XmNleftOffset,       5,
             XmNrightAttachment,  XmATTACH_NONE,
             XmNbottomAttachment, XmATTACH_NONE,
         NULL);
      
      /* to the callback, we need to give the widget to display its
       * data on, and something to keep track of what page we're on,
       * a copy of the icd product for safekeeping, and screen number
       */
      call_info = (tab_info *)malloc(sizeof(tab_info));
      call_info->label = tab_text;
      call_info->cur_page = (int *)malloc(sizeof(int));
      *(call_info->cur_page) = 1;
      call_info->offset = begin_offset;
      call_info->tabscreen = prod_screen;
      /*  NOTE: Since we close the TAB window when the product screen is  */
      /*        closed and cleared, there is only one reason for making  */
      /*        a copy of the product: */
      /*        A user can display a TAB of the current product and then */
      /*        overlay another product, and still page through the TAB  */
      gp = (Graphic_product *)(tab_sd->icd_product + 96);
      call_info->product = malloc(96+gp->msg_len);
      if(call_info->product != NULL)
      memcpy(call_info->product, tab_sd->icd_product, 96+gp->msg_len);


      /* create a handle so an external callback can destroy structure */
      if(prod_screen==1) {
          call_info1 = call_info;
            XtAddCallback(prev_but, XmNactivateCallback, tab_prev_callback, 
                (XtPointer)call_info1);         
            XtAddCallback(next_but, XmNactivateCallback, tab_next_callback, 
                (XtPointer)call_info1);  
            XtAddCallback(sd->tab_window, XmNdestroyCallback, tab_destroy_callback,
            (XtPointer)call_info1);     
      } 
         
      else if(prod_screen==2) {
          call_info2 = call_info;
            XtAddCallback(prev_but, XmNactivateCallback, tab_prev_callback, 
                (XtPointer)call_info2);         
            XtAddCallback(next_but, XmNactivateCallback, tab_next_callback, 
                (XtPointer)call_info2); 
            XtAddCallback(sd->tab_window, XmNdestroyCallback, tab_destroy_callback,
            (XtPointer)call_info2);       
      }
      
      else if(prod_screen==3) {
          call_info3 = call_info;
            XtAddCallback(prev_but, XmNactivateCallback, tab_prev_callback, 
                (XtPointer)call_info3);         
            XtAddCallback(next_but, XmNactivateCallback, tab_next_callback, 
                (XtPointer)call_info3); 
            XtAddCallback(sd->tab_window, XmNdestroyCallback, tab_destroy_callback,
            (XtPointer)call_info3);       
      }
      
      
      /* reset call_info for use when displaying a new TAB in this window */
      call_info=NULL;
     

      /* now, display the page of TAB info by setting it as the text for a label */
        xmstr = XmStringCreateLtoR(tab_buf, "tabfont"); 

      if(prod_screen==1)
          XtVaSetValues(call_info1->label,XmNlabelString,xmstr,NULL);
      else if(prod_screen==2)
          XtVaSetValues(call_info2->label,XmNlabelString,xmstr,NULL); 
      else if(prod_screen==3)
          XtVaSetValues(call_info3->label,XmNlabelString,xmstr,NULL); 
    
      XmStringFree(xmstr);
      
      /* now, open the window and clean up */
      XtRealizeWidget(tab_sd->tab_window);
      XtPopup(tab_sd->tab_window, XtGrabNone);
     
    } /*  end IF NULL opened new window to display the TAB */

/* -------------------- USE EXISTING TAB WINDOW ---------------------- */
    else {
        

      /* now, display the page of TAB info by setting it as the text for a label */
        xmstr = XmStringCreateLtoR(tab_buf, "tabfont"); 
        if(prod_screen==1) {
            call_info=call_info1;
              if(tab_msg_type == STANDALONE_TABULAR) 
                  XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Screen 1",
                     NULL);
              else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Screen 1",
                     NULL);

        } else if(prod_screen==2) {
            call_info=call_info2;
                if(tab_msg_type == STANDALONE_TABULAR) 
                  XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Screen 2",
                     NULL);
              else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Screen 2",
                     NULL);

        } else if(prod_screen==3) {
            call_info=call_info3;
                if(tab_msg_type == STANDALONE_TABULAR) 
                  XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Standalone Tabular Alpha Product - Aux Screen",
                     NULL);
              else
                 XtVaSetValues(tab_sd->tab_window,
                     XmNtitle,    "Tabular Alphanumeric Block - Aux Screen",
                     NULL);
        }



        XtVaSetValues(call_info->label,XmNlabelString,xmstr,NULL);
        XmStringFree(xmstr);
        
        *(call_info->cur_page) = 1; 
         call_info->offset = begin_offset;
         call_info->tabscreen = prod_screen;

        gp = (Graphic_product *)(tab_sd->icd_product + 96);
        if(call_info->product!=NULL) free(call_info->product);
        call_info->product = malloc(96+gp->msg_len);
        if(call_info->product != NULL)
            memcpy(call_info->product, tab_sd->icd_product, 96+gp->msg_len);

    } /*  end use existing tab window */

} /*  end display_TAB */


/* the TAB display window has two buttons - one to display the next page
 * and one to display the previous one.  they work by reparsing the TAB block
 * and simply loading whatever page they need and redisplaying it
 */
void tab_next_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    tab_info *call_info = (tab_info *)client_data; /* info needed to redisplay */

    int offset = call_info->offset;
    XmString xmstr;
    int *cur_page;

    int ret;
    
                   
    cur_page = call_info->cur_page;
    
    /* print out some of the client data*/
    if(verbose_flag)
        printf("page number = %d  tab offset = %d  tab screen %d\n", 
                     *cur_page, call_info->offset,call_info->tabscreen);


    /* increment page number! */
    (*cur_page)++;
    if(*cur_page > num_tab_pages)
        *cur_page = 1;
       
    if(verbose_flag)
        printf("New page = %d\n", *cur_page);

    /* GET NEXT PAGE */
    ret = get_tab_page(offset,*cur_page,call_info->tabscreen);
  
    if(ret==FALSE) {
        fprintf(stderr," ERROR - Unable to retrieve the next TAB Page.\n");
        return;
    }


      if(ret==TRUE) {  
        /* set new string for display */
       /* xmstr = XmStringCreateLtoR(tab_buf, XmFONTLIST_DEFAULT_TAG); */
        xmstr = XmStringCreateLtoR(tab_buf, "tabfont");
        XtVaSetValues(call_info->label, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);
        return;
    }

} /*  end tab_next_callback */



void tab_prev_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    tab_info *call_info = (tab_info *)client_data; /* info needed to redisplay */
    
    int offset = call_info->offset;
    XmString xmstr;
    int *cur_page; 

    int ret;

    
    cur_page = call_info->cur_page;
    
    /* print out some of the client data*/
    if(verbose_flag)
        printf("page number = %d  tab offset = %d   tab screen = %d\n", 
                 *cur_page, call_info->offset,call_info->tabscreen);


    /* decrement page number! */
    (*cur_page)--;
    if(*cur_page <= 0)
        *cur_page = num_tab_pages;
       
    if(verbose_flag)
        printf("New page = %d\n", *cur_page);

    /* GET NEXT PAGE */
    ret = get_tab_page(offset,*cur_page,call_info->tabscreen);
  
    if(ret==FALSE) {
        fprintf(stderr," ERROR - Unable to retrieve the previous TAB Page.\n");
        return;
    }

      if(ret==TRUE) {  

        /* set new string for display */
            /*  xmstr = XmStringCreateLtoR(tab_buf, XmFONTLIST_DEFAULT_TAG); */
        xmstr = XmStringCreateLtoR(tab_buf, "tabfont");     
        XtVaSetValues(call_info->label, XmNlabelString, xmstr, NULL);
        XmStringFree(xmstr);
        return;
    }

} /*  end tab_prev_callback */



/* gets rid of all of the TAB info associated with a particular window */
void tab_destroy_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    tab_info *call_info = (tab_info *)client_data; 

    close_tab_window(call_info->tabscreen);


}



void close_tab_window(int screen) {
tab_info *info=NULL;

/* // DEBUG */
/* fprintf(stderr," DEBUG - close_tab_window screen %d \n", screen); */

    if(screen==1) {
        info = call_info1;
        call_info1=NULL;
        sd1->tab_window = NULL;
    } else if(screen==2) {    
        info = call_info2;
        call_info2=NULL;
        sd2->tab_window = NULL;
    } else if(screen==3) {    
        info = call_info3;
        call_info3=NULL;
        sd3->tab_window = NULL;
    }
    

    if(info!=NULL) {
/* // DEBUG */
/* fprintf(stderr," DEBUG - close_tab_window clearing tab_info \n");     */
    
        if(info->cur_page!=NULL) {
/* // DEBUG */
/* fprintf(stderr," DEBUG - close_tab_window clearing current_page \n"); */
            free(info->cur_page);
            info->cur_page=NULL;
        }
        
        if(info->product!=NULL ) {
/* // DEBUG */
/* fprintf(stderr," DEBUG - close_tab_window clearing product \n"); */
            free(info->product);
            info->product=NULL;
        }

        free(info);
        info=NULL;
    }

} /*  end close_tab_window */



/*==============================================================================*/

int get_tab_page(offset,tab_page,tabscreen)
{
  int i,page,numpages,buf_len=0;

  int num_lines =0;  /* used to catch page parsing errors */
    
  /*LINUX change.. made them shorts*/
  short divider, bid=0;
  int  blen=-1; 
  int page_parse_error = FALSE; 
  
  screen_data *tab_sd=NULL;

/* */ /*DEBUG*/
/* fprintf(stderr,"DEBUG - entering get_tab_page offset is %d, desired page is %d\n", */
/*            offset, tab_page); */
  
 
  if(tabscreen==1)
      tab_sd=sd1;
  else if(tabscreen==2)
      tab_sd=sd2; 
  else if(tabscreen==3)
      tab_sd=sd3; 
  
  if(tab_sd->icd_product==NULL) {
    if(tab_msg_type == STANDALONE_TABULAR) {
      fprintf(stderr,"ERROR Reading SATAP, Product Data is NULL\n"); 
    } else {
      fprintf(stderr,"ERROR Reading TAB, Product Data is NULL\n");
    }
    return(FALSE);   
  } 
  
    /* read in block header */
    divider = read_half(tab_sd->icd_product, &offset);
  if(tab_msg_type != STANDALONE_TABULAR) {  
/* */ /*DEBUG*/
/* fprintf(stderr,"DEBUG - get_tab_page - reading ID and Length for TAB\n");      */
    bid = read_half(tab_sd->icd_product, &offset);
    blen = read_word(tab_sd->icd_product, &offset);
  }
  
/* */ /*DEBUG*/
/* fprintf(stderr,"DEBUG - get_tab_page - Message Type is %d\n",tab_msg_type); */
/* fprintf(stderr,"DEBUG - STANDALONE_TABULAR is '2'\n"); */
  
  if(tab_msg_type == STANDALONE_TABULAR) {
      if(verbose_flag) {
        printf("\nStand-Alone Tabular Alphanumeric Product\n");
        printf("Block Divider =   %hd\n", divider);
      }
  } else {  /* else a regular TAB */
      if(verbose_flag) {
        printf("\nTabular Alphanumeric Block\n");
        printf("Block Divider =   %hd\n", divider);
        printf("Block ID =        %hd\n", bid);
        printf("Length of Block = %d bytes\n", blen);
      }
  }
  
  if(tab_msg_type == STANDALONE_TABULAR) {
      if((divider != -1)) {
         printf( "\nSATAP  ERROR   DISPLAY_TAB   SATAP  ERROR\n");
         printf( "ERROR Entering SATAP Block, Either entry offset\n");
         printf( " is incorrect or SATAP divider is Incorrect\n");
         return(FALSE);
      }
  } else { /* else a regular TAB */
      if((divider != -1) || (bid != 3)) {
         printf( "\nTAB  ERROR   DISPLAY_TAB   TAB  ERROR\n");
         printf( "ERROR Entering TAB Block, Either entry offset\n");
         printf( " is incorrect or TAB divider and ID are Incorrect\n");
         return(FALSE);
      }
  }    

  /* advance offset pointer beyond the message header block and the
   * product description block contained in the TAB */
  if(tab_msg_type != STANDALONE_TABULAR)
      offset += 120;

  /* read in TAB header, only read number of pages for a SATAP */
  if(tab_msg_type != STANDALONE_TABULAR) 
      divider = read_half(tab_sd->icd_product, &offset);
  num_tab_pages = numpages = read_half(tab_sd->icd_product, &offset);
  
  if(verbose_flag) {
      printf("new offset = %d\n", offset); 
      printf("Block Divider =   %hd\n", divider);    
      printf("Number of Pages:  %hd  Current Page:  %hd\n", numpages, tab_page);
  }


  /* go through each page to find the one we want to display */
  for(page=1 ;page<=numpages; page++) {
      if(verbose_flag)
      fprintf(stderr, "\nTabular Alphanumeric Block - Page: %d\n",page);

        num_lines=0;
    
      /* loop to read a page's worth of data in */
      for(;;) {
      unsigned char c;
      int num = read_half(tab_sd->icd_product, &offset); /* num of chars in current line */
            if(num > 80) {
              printf( "\nTAB  ERROR   DISPLAY_TAB   TAB  ERROR\n");
              printf( "ERROR PARSING TAB PAGE Number %d\n", page);
              printf( " Number of Characters Exceed 80 on Line %d\n",
                      num_lines+1);
              page_parse_error = TRUE;
              break;
            }

           /* if a max size page we exit after checking for divider */     
            if(num_lines==MAX_NUM_LINES) {
              if(num==-1) 
                break; 
              else {
                printf( "\nTAB  ERROR   DISPLAY_TAB   TAB  ERROR\n");
                printf( "ERROR PARSING TAB PAGE Number %d\n", page);
                printf( " or Number of Lines Exceed Limit of 17\n");
                printf( "Did Not Find End-Of-Page (-1) Divider\n");
                page_parse_error = TRUE;
                break;
              }
            }   

      /* stop if end of page flag is reached */
      if(num == -1) break;  /* catch a short page */

      if(page == tab_page) {
          /* if we've reached the page we want, then read the page in character
           * by character and store it in a buffer for later display
           */
          for(i=0; i<num; i++) {
          c = read_byte(tab_sd->icd_product, &offset);
          tab_buf[buf_len++] = c;
          }
          tab_buf[buf_len++] = '\n';
      } else {   /* otherwise, skip over the page */
          offset += num;
      }
      
        num_lines++;
              
      } /* end for(;;) */
      
      if(page_parse_error==TRUE)
            return(FALSE);      
            
        
      /* if we managed to find the page we were looking for, pop up a window
       * and display the page in it
       */
      if(page == tab_page)
      tab_buf[buf_len++] = '\0';  /* make sure the TAB string is null-terminated */
     
   }
   return(TRUE);
} /*  end get_tab_page */
