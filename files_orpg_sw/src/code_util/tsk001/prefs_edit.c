/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/28 16:04:45 $
 * $Id: prefs_edit.c,v 1.9 2014/03/28 16:04:45 jeffs Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/* prefs_edit.c */
/* editing preference data */

#include "prefs_edit.h"




/* 12/29/05 TJG - separated editing preferences into a separate module */




int temp_msg_type;
/* CVG 9.1 - add packet 1 coord override for geographic products */
int temp_pkt1_flag;

int temp_resindex;
int temp_digflag;
int temp_assocpacket;

/* CVG 9.1 - add added override of colors for non-2d array packets */
int temp_overridepacket;

int temp_rdr_type;

/* CVG 9.3 - added elevation falg */
int temp_elflag;

/* //////////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////////// */






/* ////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////// */



void msgt_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *msg_type = (int *)client_data; 
    
    temp_msg_type = *msg_type;

    set_sensitivity();    
    
}


/* CVG 9.1 - added packet 1 coord override for geographic products */
void pkt1_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    int *pkt1_flag = (int *)client_data; 
    
    temp_pkt1_flag = *pkt1_flag;
    
}




void resi_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *res_index = (int *)client_data; 
    
    temp_resindex = *res_index;
    
    
}




/* CVG 9.1 - added override of colors for non-2d array packets */
void overp_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *override_pkt = (int *)client_data; 
    
    temp_overridepacket = *override_pkt;
        
}






void digf_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *dig_flag = (int *)client_data; 
    
    temp_digflag = *dig_flag;

    set_sensitivity();

    
    
}


void assp_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *assoc_pkt = (int *)client_data; 
    
    temp_assocpacket = *assoc_pkt;
        
}


/* CVG 9.3 - added elevation flag */
void elflag_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *elflag = (int *)client_data; 
    
    temp_elflag = *elflag;
        
}


void set_sensitivity()
{

char *buf;

        XtVaSetValues(label_mt, XmNsensitive,   True,   NULL);
        /* CVG 9.1 - added packet 1 coord override for geographic products */
        XtVaSetValues(label_pk, XmNsensitive,   True,   NULL);
        
        XtVaSetValues(label_pr, XmNsensitive,   True,   NULL);
        
        /* CVG 9.1 - added override of colors for non-2d array packets */
        XtVaSetValues(label_o_pal, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_o_pkt, XmNsensitive,   True,   NULL);
        
        XtVaSetValues(label_df, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_l1, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_p1, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   True,   NULL); 
        XtVaSetValues(label_pt, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_um, XmNsensitive,   True,   NULL);

        /* CVG 9.3 - added elevation flag */
        XtVaSetValues(label_el, XmNsensitive,   True,   NULL);     
        
/*  get value of message type //     */
    /* CVG 9.1 - added packet 1 coord override for geographic products */
    if (temp_msg_type!=0 ) {
        XtVaSetValues(label_pk, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_el, XmNsensitive,   False,   NULL);     
    }

    if( temp_msg_type==0 || temp_msg_type==1 || 
        temp_msg_type==2 ) {
        
        ;
    
    } else if( temp_msg_type==3 || temp_msg_type==3 ||
               temp_msg_type==4 || temp_msg_type==999 ) {
        XtVaSetValues(label_pr, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_df, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   False,   NULL); 
        XtVaSetValues(label_pt, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_um, XmNsensitive,   False,   NULL); 
    }


/*  get value of digital legend flag   */
    if(temp_digflag==0) {
        XtVaSetValues(label_l1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   False,   NULL);        
        
    } else if(temp_digflag==1 || temp_digflag==2 
           || temp_digflag==4 || temp_digflag==5 || temp_digflag==6 ) {
        XtVaSetValues(label_l1, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   False,   NULL);        
        
    } else if(temp_digflag==3) {
        XtVaSetValues(label_l1, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   True,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   True,   NULL);
       
    } else if(temp_digflag==-1) {
        XtVaSetValues(label_l1, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_l2, XmNsensitive,   False,   NULL);
        XtVaSetValues(label_p2, XmNsensitive,   False,   NULL);    
    }

    /*  get value of configured palette // */
    XtVaGetValues(confpalette_text, XmNvalue, &buf, NULL);
    XtVaSetValues(confpalette_text, XmNvalue, buf, NULL);
    if( (strcmp(buf, ".") == 0) ||    /*  old default palette entry */
        (strcmp(buf, ".plt") == 0) )
         XtVaSetValues(label_pt, XmNsensitive,   False,   NULL);
    else 
         XtVaSetValues(label_pt, XmNsensitive,   True,   NULL);
    free(buf);
    
}





/* when a product ID is selected from the list, fill the editing areas with the data */
void product_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
/* char *sbuf; */
/* DEBUG */
/* fprintf(stderr,"DEBUG - position of selected item is %d, length is %d, number selected is %d\n", */
/*    cbs->item_position, cbs->item_length, cbs->selected_item_count);  */
/*  */
/*  */
/*   sbuf = (char *) XmStringUnparse(cbs->item, XmFONTLIST_DEFAULT_TAG, */
/*                                   XmCHARSET_TEXT, XmCHARSET_TEXT, */
/*                                   NULL, 0, */
/*                                   XmOUTPUT_ALL);   */
/* DEBUG */
/* fprintf(stderr,"DEBUG - selected item is %s, length is %d\n", */
/*    sbuf, strlen(sbuf));  */
/* fprintf(stderr,"DEBUG - selected item is '%s'\n", */
/*    sbuf);   */
/*   sscanf(sbuf, "%d", &msg_num);  */
/*   free(sbuf);   */

/* DEBUG */
/*fprintf(stderr,"DEBUG - entering product_edit_select_callback\n");*/

    XtVaSetValues(id_label, XmNlabelString, cbs->item, NULL);
    

    product_edit_fill_fields();
    
}


/* does not work after pressing the commit button */
void product_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    
    product_edit_fill_fields();
    
}



/* fills the fields by reading entries in the global arrays */
void product_edit_fill_fields()
{
    XmString xmstr, preview_xtr;
    char *str, *datastr, buf[200], preview_str[40];
    int   pid, *dataptr, data;
    
    /* first, find the product ID of the current product */
    XtVaGetValues(id_label, XmNlabelString, &xmstr, NULL);
    XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &str);
    pid = atoi(str);
    free(str);
    XmStringFree(xmstr);


/*  DEBUG */
/* fprintf(stderr,"DEBUG - entering product_edit_fill_fields \n"); */
    
    if( !((pid >=0) && (pid <= 1999)) )
        fprintf(stderr,"ERROR in Product ID (%d) This value is outside of the\n"
                       "      range of values produced by the radar (1-1999).\n", 
                                                pid);

    /* now, fill the rest of the fields */
    
    /****************************************************************/
    dataptr = assoc_access_i(msg_type_list, pid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = -1;
        

    if(data==0) {
        msgt_but_set = msgt_but0;
        temp_msg_type = 0;
        
    } else if(data==1) {
        msgt_but_set = msgt_but1;
        temp_msg_type = 1;
        
    } else if(data==2) {
        msgt_but_set = msgt_but2;
        temp_msg_type = 2;
        
    } else if(data==3) {
        msgt_but_set = msgt_but3;
        temp_msg_type = 3;
  
    } else if(data==4) {
        msgt_but_set = msgt_but4;
        temp_msg_type = 4;
       
    } else if(data==999) {
        msgt_but_set = msgt_but999;
        temp_msg_type = 999;
  
    } else if(data==-1) {
        msgt_but_set = msgt_but1neg;
        temp_msg_type = -1;
        
    } else {
        fprintf(stderr,"ERROR message type (%d) for ID %d is not\n"
                       "      valid, reset to default value '-1'.\n", 
                                                data, pid);
        msgt_but_set = msgt_but1neg;
        temp_msg_type = -1;
    }

    XtVaSetValues(msgtype_opt, XmNmenuHistory, msgt_but_set, NULL);



    /* CVG 9.1 - added packet 1 coord override for geographic products */
    /****************************************************************/
/* DEBUG */
/*fprintf(stderr,"DEBUG product_edit_fill_fields - reading pkt1 flag / " */
/*               "setting temp_pkt1_flag, pid is %d\n", pid); */
               
    dataptr = assoc_access_i(packet_1_geo_coord_flag, pid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = 0;
        

    if(data==0) {
        pkt1_but_set = pkt1_but0;
        temp_pkt1_flag = 0;
        
    } else if(data==1) {
        pkt1_but_set = pkt1_but1;
        temp_pkt1_flag = 1;
        
    } else {
        fprintf(stderr,"ERROR packet 1 override  (%d) for ID %d is not\n"
                       "      valid, reset to default value '0'.\n", 
                                                data, pid);
        pkt1_but_set = pkt1_but0;
        temp_pkt1_flag = 0;
    }


    
    XtVaSetValues(packet_1_opt, XmNmenuHistory, pkt1_but_set, NULL);
    

/* DEBUG */
/*fprintf(stderr,"DEBUG product_edit_fill_fields - reading product res / " */
/*               "setting resi_but_set\n");                                */
               
    /****************************************************************/
    dataptr = assoc_access_i(product_res, pid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = -1;
        

    if(data==0) {
        resi_but_set = resi_but0;
        temp_resindex = 0;
    } else if(data==1) {
        resi_but_set = resi_but1;
        temp_resindex = 1;
    } else if(data==2) {
        resi_but_set = resi_but2;
        temp_resindex = 2;
    } else if(data==3) {
        resi_but_set = resi_but3;
        temp_resindex = 3;
    } else if(data==4) {
        resi_but_set = resi_but4;
        temp_resindex = 4;
    } else if(data==5) {
        resi_but_set = resi_but5;
        temp_resindex = 5;
    } else if(data==6) {
        resi_but_set = resi_but6;
        temp_resindex = 6;
    } else if(data==7) {
        resi_but_set = resi_but7;
        temp_resindex = 7;
    } else if(data==8) {
        resi_but_set = resi_but8;
        temp_resindex = 8;
    } else if(data==9) {
        resi_but_set = resi_but9;
        temp_resindex = 9;
    } else if(data==10) {
        resi_but_set = resi_but10;
        temp_resindex = 10;
    } else if(data==11) {
        resi_but_set = resi_but11;
        temp_resindex = 11;
    } else if(data==12) {
        resi_but_set = resi_but12;
        temp_resindex = 12;
    } else if(data==13) {
        resi_but_set = resi_but13;
        temp_resindex = 13;
    } else if(data==-1) {
        resi_but_set = resi_but1neg;
        temp_resindex = -1;
        
    } else {
        fprintf(stderr,"ERROR product resolution (%d) for ID %d is not\n"
                       "      valid, reset to default value '-1'.\n", 
                                                               data, pid);
        resi_but_set = resi_but1neg;
        temp_resindex = -1;
    }
    
    XtVaSetValues(resindex_opt, XmNmenuHistory, resi_but_set, NULL);







    /**************************************************************************/
    /* CVG 9.1 - added override of colors for non-2d array packets */
    datastr = assoc_access_s(override_palette, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, ".plt");
    if(check_filename("plt", buf)==FALSE) {
        fprintf(stderr,"ERROR palette filename (%s) for ID %d is not\n"
                       "      valid, reset to default value '.plt'\n", 
                                              buf, pid);
        strcpy(buf, ".plt");
    }
    XtVaSetValues(override_palette_text, XmNvalue, buf, NULL);


    /***************************************************************************/
    /* CVG 9.1 - added override of colors for non-2d array packets */
    dataptr = assoc_access_i(override_packet, pid);
    
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = 0;


    if(data==0) {
        over_but_set = over_but0;
        temp_overridepacket = 0;
     } else if(data==4) {
        over_but_set = over_but4;
        temp_overridepacket = 4;
    } else if(data==6) {
        over_but_set = over_but6;
        temp_overridepacket = 6;
    } else if(data==8) {
        over_but_set = over_but8;
        temp_overridepacket = 8;
    } else if(data==9) {
        over_but_set = over_but9;
        temp_overridepacket = 9;
    } else if(data==10) {
        over_but_set = over_but10;
        temp_overridepacket = 10;
    } else if(data==20) { /* SHOULD THIS BE INCLUDED? */
        over_but_set = over_but20;
        temp_overridepacket = 20;

    } else if(data==43) {
        over_but_set = over_but43;
        temp_overridepacket = 43;
    } else if(data==51) {   /*  contour IT'S BACK */
        over_but_set = over_but51;
        temp_overridepacket = 51;
    } else {
        fprintf(stderr,"ERROR associated packet (%d) for ID %d is not\n"
                       "      valid/supported, reset to default value '0'.\n", 
                                                             data, pid);
        over_but_set = over_but0;
        temp_overridepacket = 0;
    }
    XtVaSetValues(overridepacket_opt, XmNmenuHistory, over_but_set, NULL);    



/* DEBUG */
/*fprintf(stderr,"DEBUG product_edit_fill_fields - setting override_palette_text ,%s, "*/
/*               "and temp_overridepacket %d \n",                                      */
/*        buf, temp_overridepacket);                                                   */





    /****************************************************************/
    dataptr = assoc_access_i(digital_legend_flag, pid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = -1;
        

    if(data==0) {
        digf_but_set = digf_but0;
        temp_digflag = 0;       
        
    } else if(data==1) {
        digf_but_set = digf_but1;
        temp_digflag = 1;
        
    } else if(data==2) {
        digf_but_set = digf_but2;
        temp_digflag = 2;
        
    } else if(data==3) {
        digf_but_set = digf_but3;
        temp_digflag = 3;
 
    } else if(data==4) {
        digf_but_set = digf_but4;
        temp_digflag = 4;
        
    } else if(data==5) {
        digf_but_set = digf_but5;
        temp_digflag = 5;
        
    } else if(data==6) {
        digf_but_set = digf_but6; 
        temp_digflag = 6;
     
    } else if(data==-1) {
        digf_but_set = digf_but1neg;
        temp_digflag = -1;

        
    } else {
        fprintf(stderr,"ERROR legend flag (%d) for ID %d is not\n"
                       "      valid, reset to default value '-1'.\n", 
                                                  data, pid);
        digf_but_set = digf_but1neg;
        temp_digflag = -1;
    }

/* DEBUG */
/* fprintf(stderr,"DEBUG product_edit_fill_fields - setting dig flag button\n"); */
    
    XtVaSetValues(digflag_opt, XmNmenuHistory, digf_but_set, NULL);


    /****************************************************************/
    datastr = assoc_access_s(digital_legend_file, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, ".lgd");
    if(check_filename("lgd", buf)==FALSE) {
        fprintf(stderr,"ERROR legend filename (%s) for ID %d is not\n"
                       "      valid, reset to default value '.lgd'\n", 
                                              buf, pid);
        strcpy(buf, ".lgd");
    }
    XtVaSetValues(diglegfile_text, XmNvalue, buf, NULL);     


    /****************************************************************/
    datastr = assoc_access_s(dig_legend_file_2, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, ".lgd");
    if(check_filename("lgd", buf)==FALSE) {
        fprintf(stderr,"ERROR legend filename (%s) for ID %d is not\n"
                       "      valid, reset to default value '.lgd'\n", 
                                              buf, pid);
        strcpy(buf, ".lgd");
    }
    XtVaSetValues(diglegfile2_text, XmNvalue, buf, NULL);     


    /****************************************************************/
    datastr = assoc_access_s(configured_palette, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, ".plt");
    if(check_filename("plt", buf)==FALSE) {
        fprintf(stderr,"ERROR palette filename (%s) for ID %d is not\n"
                       "      valid, reset to default value '.plt'\n", 
                                              buf, pid);
        strcpy(buf, ".plt");
    }
    XtVaSetValues(confpalette_text, XmNvalue, buf, NULL);


    /****************************************************************/
    datastr = assoc_access_s(config_palette_2, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, ".plt");
    if(check_filename("plt", buf)==FALSE) {
        fprintf(stderr,"ERROR palette filename (%s) for ID %d is not\n"
                       "      valid, reset to default value '.plt'\n", 
                                              buf, pid);
        strcpy(buf, ".plt");
    }
    XtVaSetValues(confpalette2_text, XmNvalue, buf, NULL);


    /****************************************************************/
    dataptr = assoc_access_i(associated_packet, pid);
    
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = 0;


    if(data==0) {
        assp_but_set = assp_but0;
        temp_assocpacket = 0;
    } else if(data==4) {
        assp_but_set = assp_but4;
        temp_assocpacket = 4;
    } else if(data==6) {
        assp_but_set = assp_but6;
        temp_assocpacket = 6;
    } else if(data==8) {
        assp_but_set = assp_but8;
        temp_assocpacket = 8;
    } else if(data==9) {
        assp_but_set = assp_but9;
        temp_assocpacket = 9;
    } else if(data==10) {
        assp_but_set = assp_but10;
        temp_assocpacket = 10;
    } else if(data==16) {
        assp_but_set = assp_but16;
        temp_assocpacket = 16;
    } else if(data==17) {
        assp_but_set = assp_but17;
        temp_assocpacket = 17;
/* cvg 9.0 - removed */
/*    } else if(data==18) {         */
/*        assp_but_set = assp_but18;*/
/*        temp_assocpacket = 18;    */
    } else if(data==20) {
        assp_but_set = assp_but20;
        temp_assocpacket = 20;
    } else if(data==41) {
        assp_but_set = assp_but41;
        temp_assocpacket = 41;
    } else if(data==42) {
        assp_but_set = assp_but42;
        temp_assocpacket = 42;
    } else if(data==43) {
        assp_but_set = assp_but43;
        temp_assocpacket = 43;
    } else if(data==51) {   /*  contour IIIIIIIIT'S BAAAAAAAAAACK */
        assp_but_set = assp_but51;
        temp_assocpacket = 51;
    } else if(data==53) {
        assp_but_set = assp_but53;
        temp_assocpacket = 53;
    } else if(data==54) {
        assp_but_set = assp_but54;
        temp_assocpacket = 54;
    } else if(data==55) {
        assp_but_set = assp_but55;
        temp_assocpacket = 55;
    } else {
        fprintf(stderr,"ERROR associated packet (%d) for ID %d is not\n"
                       "      valid/supported, reset to default value '0'.\n", 
                                                             data, pid);
        assp_but_set = assp_but0;
        temp_assocpacket = 0;
    }
    XtVaSetValues(assocpacket_opt, XmNmenuHistory, assp_but_set, NULL);    
    
    
    /****************************************************************/
    datastr = assoc_access_s(legend_units, pid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, "");
        
    XtVaSetValues(unit_text, XmNvalue, buf, NULL);   
    
    set_sensitivity();    

    pref_legend_grey_pixmap();
    pref_legend_show_pixmap(); 

    /* CVG 9.1 */
    /* update the sample legend display if assocpaket is a 2-d array */
    if(temp_assocpacket == 16 || temp_assocpacket == 17 ||
       temp_assocpacket == 41 || temp_assocpacket == 42 || 
       temp_assocpacket == 53 || temp_assocpacket == 54 ||
       temp_assocpacket == 55) {
        
        if(temp_digflag != 0)
            pref_legend_clear_pixmap();
        display_legend_blocks(pref_legend_pix, 5, 5, TRUE, PREFS_FRAME);
        pref_legend_show_pixmap();
        
        if( (temp_digflag == 1) || (temp_digflag == 2) || (temp_digflag == 3) ) 
            sprintf(preview_str, "Digital Legend   \nPreview");
        else if( (temp_digflag==4) || (temp_digflag==5) || (temp_digflag==6) )
            sprintf(preview_str, "Generic Legend   \nPreview");
        else  /*  not digital / generic product */
            sprintf(preview_str, "Color Palette   \nPreview");
            
    } else { /* end if assoc packet is a 2-d array */   
        sprintf(preview_str, "       \n       ");
    }
     
    preview_xtr = XmStringCreateLtoR(preview_str, 
                                            XmFONTLIST_DEFAULT_TAG); 
    XtVaSetValues(ledg_label, XmNlabelString, preview_xtr, NULL);
    XmStringFree(preview_xtr);

    /* CVG 9.3 - added elevation flag */        
    /****************************************************************/
    dataptr = assoc_access_i(elev_flag, pid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = -1;
        

    if(data==0) {
        elf_but_set = elf_but0;
        temp_elflag = 0;       
        
    } else if(data==1) {
        elf_but_set = elf_but1;
        temp_elflag = 1;
        
    } else {
        fprintf(stderr,"ERROR elevation flag (%d) for ID %d is not\n"
                       "      valid, reset to default value '0'.\n", 
                                                  data, pid);
        elf_but_set = elf_but0;
        temp_elflag = 0;
    }

/* DEBUG */
/* fprintf(stderr,"DEBUG product_edit_fill_fields - setting elevation flag\n"); */
    
    XtVaSetValues(elflag_opt, XmNmenuHistory, elf_but_set, NULL);


/*  DEBUG */
/* fprintf(stderr,"DEBUG - leaving product_edit_fill_fields \n"); */
    
} /*  end product_edit_fill_fields() */




/* sets the values of the product specific prefs from what's in the fields */
/* and updates the sample legend display if appropriate */
void product_edit_commit_callback(Widget w, XtPointer client_data, 
                                                             XtPointer call_data)
{
    XmString xmstr;
    char *str = NULL,  *buf;
    int   pid;
    int   the_msgtype, the_resindex, the_digflag, the_asocpacket, the_elflag;
    /* CVG 9.1 - added packet 1 coord override for geographic products */
    int   the_pkt1_flag;  
    /* CVG 9.1 - added override of colors for non-2d array packets */
    int   the_overridepacket;
    
    /* first, find the product ID of the current product */
    XtVaGetValues(id_label, XmNlabelString, &xmstr, NULL);
    XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &str);

    XmStringFree(xmstr);
    
    /* if there's nothing currently being edited, we don't have to do anything */
    if(str == NULL || str[0] == '\0')
        return;
    else
    /* this is reasonably safe since this value originally comes from preferences */
    /* would be better to use isspace and isdigit to determine if really a number */
        pid = atoi(str);
        
    free(str);

    
     /* legend filenames must have the ".lgd" extension
      * and palette filenames must have the ".plt" extension.
      * If a user enters the original default (not configured) filename
      *  ".", it is converted to the new defaults ".lgd" and ".plt".
      */
    
    
    errno = 0;

    the_msgtype = temp_msg_type;
    
    /* CVG 9.1 - added packet 1 coord override for geographic products */
    the_pkt1_flag = temp_pkt1_flag;

    the_resindex = temp_resindex;
    
    /* CVG 9.1 - added ovrride of colors for non-2d array packets */
    the_overridepacket = temp_overridepacket;

    the_digflag = temp_digflag;

    the_asocpacket = temp_assocpacket;

    /* CVG 9.3 - added elevation flag */
    the_elflag = temp_elflag;

    /* now, fill the data from whatever's in the fields right now */

    assoc_insert_i(msg_type_list, pid, the_msgtype);
    
    /* CVG 9.1 - added packet 1 coord override for geographic products */
    assoc_insert_i(packet_1_geo_coord_flag, pid, the_pkt1_flag);
    
    assoc_insert_i(product_res, pid, the_resindex);
    
    assoc_insert_i(override_packet, pid, the_overridepacket);
    
    assoc_insert_i(digital_legend_flag, pid, the_digflag);

    assoc_insert_i(associated_packet, pid, the_asocpacket);

    /* CVG 9.3 - added elevation flag */
    assoc_insert_i(elev_flag, pid, the_elflag);    

    /* CVG 9.1 - added packet 1 coord override for geographic products */
      XtVaGetValues(override_palette_text, XmNvalue, &buf, NULL);
      if(check_filename("plt", buf)==FALSE) { /*  also prevents blank entry */
          assoc_insert_s(override_palette, pid, ".plt");
      } else
        /*  convert original default "." entry to the new default ".plt" */
          if((strcmp(buf, ".")==0)) 
              assoc_insert_s(override_palette, pid, ".plt");
          else
              assoc_insert_s(override_palette, pid, buf); 
      free(buf);

      /* CVG 9.1 - added packet 1 coord override for geographic products */
      XtVaGetValues(diglegfile_text, XmNvalue, &buf, NULL);
      if(check_filename("lgd", buf)==FALSE) { /*  also prevents blank entry */
          assoc_insert_s(digital_legend_file, pid, ".lgd");
          /* BUG - WHY DOESN'T THE FOLOWING WORK? - called fill_fields() instead */
          XtVaSetValues(diglegfile_text, XmNvalue, ".lgd", NULL);
      } else
        /*  convert original default "." entry to the new default ".lgd" */
          if((strcmp(buf, ".")==0)) 
              assoc_insert_s(digital_legend_file, pid, ".lgd");
          else
              assoc_insert_s(digital_legend_file, pid, buf); 
      free(buf);


      XtVaGetValues(diglegfile2_text, XmNvalue, &buf, NULL);
      if(check_filename("lgd", buf)==FALSE) { /*  also prevents blank entry */
          assoc_insert_s(dig_legend_file_2, pid, ".lgd");
      } else
        /*  convert original default "." entry to the new default ".lgd" */
          if((strcmp(buf, ".")==0)) 
              assoc_insert_s(dig_legend_file_2, pid, ".lgd");
          else
              assoc_insert_s(dig_legend_file_2, pid, buf); 
      free(buf);


      XtVaGetValues(confpalette_text, XmNvalue, &buf, NULL);
      if(check_filename("plt", buf)==FALSE) { /*  also prevents blank entry */
          assoc_insert_s(configured_palette, pid, ".plt");
      } else
        /*  convert original default "." entry to the new default ".plt" */
          if((strcmp(buf, ".")==0)) 
              assoc_insert_s(configured_palette, pid, ".plt");
          else
              assoc_insert_s(configured_palette, pid, buf); 
      free(buf);


      XtVaGetValues(confpalette2_text, XmNvalue, &buf, NULL);
      if(check_filename("plt", buf)==FALSE) { /*  also prevents blank entry */
          assoc_insert_s(config_palette_2, pid, ".plt");
      } else
        /*  convert original default "." entry to the new default ".plt" */
          if((strcmp(buf, ".")==0)) 
              assoc_insert_s(config_palette_2, pid, ".plt");
          else
              assoc_insert_s(config_palette_2, pid, buf); 
      free(buf);


      XtVaGetValues(unit_text, XmNvalue, &buf, NULL);
      /*  future improvement: could define a list of standard units but */
      /*  permit using other values */
      assoc_insert_s(legend_units, pid, buf); 
      free(buf);
      
      /*  only reason for this is to enter corrected filename entries */
      product_edit_fill_fields();


} /*  end product_edit_commit_callback() */




int check_filename(const char *type_file, char *buffer)
{
    char substring[10];

/*     fprintf(stderr,"DEBUG - checking for 'filename.%s' in %s\n",  */
/*         type_file, buffer);                                       */

    /* the filename should be either "." or contain ".typefile" */
    sprintf(substring, ".%s", type_file);
    if( (strcmp(buffer,".")!=0) &&     /*  the original default filename */
        (strstr(buffer, substring)==NULL) ) {
/*         fprintf(stderr,"ERROR - 'filename.%s' or empty filename '.' not entered\n", */
/*             type_file);                                                             */
        return(FALSE);
    } else {
/*         fprintf(stderr,"DEBUG - 'filename.%s' or '.' found:%s\n", */
/*             type_file, buffer);                                   */
        return(TRUE);
    }
           
}






/* save all of the current product specific prefs in memory to disk */
void product_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    char filename[256], *datastr;
    FILE *list_file;
    int i, *dataptr, data, pid;

    /* open the data file */
    sprintf(filename, "%s/prod_config", config_dir);
    if((list_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "Could not open product info data\n");
        exit(0);
    }


    /* again, we use the message type list as our "typical list" */
    
    
    for(i=0; i<msg_type_list->size; i++) {
        /* get the product id */
        pid = msg_type_list->keys[i];

        /* for each element, we get the value, give it a default 
         * if needed, then print it out */
        /****************************************************************/
        dataptr = assoc_access_i(msg_type_list, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, "%3d %3d", pid, data);
    
        /* CVG 9.1 - added packet 1 coord override for geographic products */
        /****************************************************************/
        dataptr = assoc_access_i(packet_1_geo_coord_flag, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %d", data);
        
        /****************************************************************/
        dataptr = assoc_access_i(product_res, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %2d", data);
        


        /* CVG 9.1 - added override of colors for non-2d array packets */
        /****************************************************************/
        datastr = assoc_access_s(override_palette, pid);
        if(datastr != NULL)
          fprintf(list_file, " %s", datastr);
        else
          fprintf(list_file, " .");

        /* CVG 9.1 - added override of colors for non-2d array packets */
        /****************************************************************/
        dataptr = assoc_access_i(override_packet, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %2d ", data);

/* DEBUG */
/*fprintf(stderr,"DEBUG product_edit_save_callback - writing override packet %d to file\n",*/
/*              data  );                                                                   */

        /****************************************************************/
        dataptr = assoc_access_i(digital_legend_flag, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %d", data);
    
        /****************************************************************/
        datastr = assoc_access_s(digital_legend_file, pid);
        if(datastr != NULL)
          fprintf(list_file, " %s", datastr);
        else
          fprintf(list_file, " .");
    
        /****************************************************************/
        datastr = assoc_access_s(dig_legend_file_2, pid);
        if(datastr != NULL)
          fprintf(list_file, " %s", datastr);
        else
          fprintf(list_file, " .");

        /****************************************************************/
        datastr = assoc_access_s(configured_palette, pid);
        if(datastr != NULL)
          fprintf(list_file, " %s", datastr);
        else
          fprintf(list_file, " .");

        /****************************************************************/
        datastr = assoc_access_s(config_palette_2, pid);
        if(datastr != NULL)
          fprintf(list_file, " %s", datastr);
        else
          fprintf(list_file, " .");
    
        /****************************************************************/
        dataptr = assoc_access_i(associated_packet, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %d ", data);

        /****************************************************************/
        dataptr = assoc_access_i(elev_flag, pid);
        if(dataptr != NULL)
          data = *dataptr;
        else
          data = 0;
        fprintf(list_file, " %d ", data);
    
        /****************************************************************/
        datastr = assoc_access_s(legend_units,pid);
        if(datastr != NULL)
          fprintf(list_file, "%s", datastr);
    
        /* end of record means end of line */
        fprintf(list_file, "\n");
    }

    fclose(list_file);
}




/* add another entry of product specific data by entering its product id */
void product_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmstr;


    d = XmCreatePromptDialog(w, "pid_add", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Add Product ID", NULL);
    xmstr = XmStringCreateLtoR("Product ID to add:", XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(d, XmNselectionLabelString, xmstr, 
             XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);
    XtAddCallback(d, XmNcancelCallback, product_edit_add_cancel_callback, NULL);
    XtAddCallback(d, XmNokCallback, product_edit_add_ok_callback, NULL);
    XmStringFree(xmstr);

    XtManageChild(d);
}



void product_edit_add_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    XtUnmanageChild(w);
}




/* if the product add is okayed, then we check to see if the input is
 * good, and if it is, we bring it up for editing
 */
void product_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
    char *new_pid_str, buf[20];
    int i, new_pid, list_size;
    Widget d;
    XmString xmstr;
    
    char sub[25];
    int k, j;

    /* get the new pid in string form */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &new_pid_str);



    /* find a number and complain if it isn't one */
    /* skip over any initial spaces */
    k=0;
    while(new_pid_str[k] == ' ') k++;

    /* read in an integer (the prod id) */
    j=0;
    while(isdigit((int)(new_pid_str[k]))) 
        sub[j++] = new_pid_str[k++];
    sub[j] = '\0';
    free(new_pid_str);
    
    /* check if we got digits */
    if(j==0) {
        d = XmCreateErrorDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR("The entered value is not a number.\nPlease try again.",
                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
        
    } else 
        new_pid = atoi(sub);


    /* check to see if the number exists already.  if it does, complain */
    for(i=0; i<msg_type_list->size; i++)
        if(new_pid == msg_type_list->keys[i]) {
        d = XmCreateInformationDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR("The specified product ID already exists.\nEnter another number.",
                       XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
    }
    
    /* check to see if the number is within the range produced by the radar */
    if( !((new_pid >=0) && (new_pid <= 1999)) ) {
        d = XmCreateInformationDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR("The specified product ID is beyond the valid range.\nEnter another number (1-1999).",
                       XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
    }
        

    /* if not, add default values for the pid in to the preference fields */
    assoc_insert_i(product_res, new_pid, -1);
    assoc_insert_i(digital_legend_flag, new_pid, -1);
    assoc_insert_s(digital_legend_file, new_pid, ".lgd");
    assoc_insert_s(dig_legend_file_2, new_pid, ".lgd");
    assoc_insert_s(configured_palette, new_pid, ".plt");
    assoc_insert_s(config_palette_2, new_pid, ".plt");
    assoc_insert_i(associated_packet, new_pid, 0);

    assoc_insert_i(msg_type_list, new_pid, -1);
    /* CVG 9.1 - added packet 1 coord override for geographic products */
    assoc_insert_i(packet_1_geo_coord_flag, new_pid, 0);
    
    /* CVG 9.1 - added override of colors for non-2d array packets */
    assoc_insert_s(override_palette, new_pid, ".plt");
    assoc_insert_i(override_packet, new_pid, 0);
    
    /* CVG 9.3 - added elevation flag */
    assoc_insert_i(elev_flag, new_pid, 0);

    assoc_insert_s(legend_units, new_pid, "units"); 
    
    /* then set it up to be edited */
    sprintf(buf, "%d", new_pid);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    

    XtVaSetValues(id_label, XmNlabelString, xmstr, NULL);
     
    XtVaGetValues(pi_list, XmNitemCount, &list_size, NULL);  
    XmListAddItem(pi_list, xmstr, list_size+1);   
    XmListSelectItem(pi_list, xmstr, True);     
    XtVaSetValues(XtParent(pi_list), XmNwidth, 150, NULL);    
    XtUnmanageChild(w); 
XmStringFree(xmstr);
  
}




/* --- start site info stuff --- */
/* /////////////////////////////////////////////////////////////////// */
/* /////////////////////////////////////////////////////////////////// */





void rdrt_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *rdr_type = (int *)client_data; 
    
    temp_rdr_type = *rdr_type;   
    
}



/* when a site ID is selected from the list, fill the editing areas with the data */
void site_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;

    XtVaSetValues(site_id_label, XmNlabelString, cbs->item, NULL);
    site_edit_fill_fields();
}



void site_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    site_edit_fill_fields();
}



void site_edit_fill_fields()
{
    XmString xmstr;
    char *str, *datastr, buf[200];
    int   sid, *dataptr, data;
    
    /* first, find the site ID of the current site */
    XtVaGetValues(site_id_label, XmNlabelString, &xmstr, NULL);
    XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &str);
    sid = atoi(str);


    /* now, fill the rest of the fields */
    dataptr = assoc_access_i(radar_type_list, sid);
    if(dataptr != NULL)
        data = *dataptr;
    else
        data = 0;


    if(data==0) {        /* WSR-88D */
        rdrt_but_set = rdrt_but0;
        temp_rdr_type = 0;
    /* possible future radar types         */
    } else if(data==1) { /* ARSR-4 */
        rdrt_but_set = rdrt_but1;
        temp_rdr_type = 1;
    } else if(data==2) { /* ASR-9 */
        rdrt_but_set = rdrt_but2;
        temp_rdr_type = 2;
    } else if(data==3) { /* ASR-11 */
        rdrt_but_set = rdrt_but3;
        temp_rdr_type = 3;
    } else if(data==4) { /* TDWR */
        rdrt_but_set = rdrt_but4;
        temp_rdr_type = 4;

    }
    XtVaSetValues(rdrtype_opt, XmNmenuHistory, rdrt_but_set, NULL);



    datastr = assoc_access_s(icao_list, sid);
    if(datastr != NULL)
        strcpy(buf, datastr);
    else
        strcpy(buf, "");
    XtVaSetValues(icao_text, XmNvalue, buf, NULL);       
}




/* sets the values of the site specific prefs from what's in the fields */
void site_edit_commit_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmString xmstr;
    char *str = NULL, *buf;
    int   sid;
    int   the_radartype;
    
    /* first, find the site ID of the current site */
    XtVaGetValues(site_id_label, XmNlabelString, &xmstr, NULL);
    XmStringGetLtoR(xmstr, XmFONTLIST_DEFAULT_TAG, &str);
    XmStringFree(xmstr);
    
    /* if there's nothing currently being edited, we don't have to do anything */
    if(str == NULL || str[0] == '\0')
        return;
    else
        sid = atoi(str);

    if(sid == 0)
        return;
      
    free(str);

    /* now, doublecheck the field values to make sure that they're
     * reasonably kosher (e.g. not all letters when we want numbers)
     */
     /* FUTURE ENHANCEMENT */
    /* NOTE: the following checks do not work because of the 
     * implementation of strtol().  New checks parsing the string
     * with isdigit() and isspace() should be developed.  
     */
    errno = 0;


    the_radartype = temp_rdr_type;

    /* now, fill the data from whatever's in the fields right now */
     
    assoc_insert_i(radar_type_list, sid, the_radartype);

    XtVaGetValues(icao_text, XmNvalue, &buf, NULL);
    assoc_insert_s(icao_list, sid, buf);
    free(buf);

}



/* save all of the current site specific prefs in memory to disk */
void site_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    char filename[256], *datastr;
    FILE *list_file;
    int i, *dataptr, data, sid;

    /* open the data file */
    sprintf(filename, "%s/site_data", config_dir);
    if((list_file=fopen(filename, "w"))==NULL) {
        fprintf(stderr, "Could not open site info data\n");
        exit(0);
    }


    /* again, we use the icao list as our "typical list" */
    for(i=0; i<icao_list->size; i++) {
        /* get the site id */
        sid = icao_list->keys[i];

        /* for each element, we get the value, give it a default 
         * if needed, then print it out */
            dataptr = assoc_access_i(radar_type_list, sid);
        if(dataptr != NULL)
            data = *dataptr;
        else
            data = 0;
        fprintf(list_file, "%d %d", sid, data);
    
        datastr = assoc_access_s(icao_list, sid);
        if(datastr != NULL)
            fprintf(list_file, " %s", datastr);
        else
            fprintf(list_file, " ");
    
        /* end of record means end of line */
        fprintf(list_file, "\n");
    }

    fclose(list_file);
}



/* add another entry of site specific data by entering its site id */
void site_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget d;
    XmString xmstr;

    d = XmCreatePromptDialog(w, "sid_add", NULL, 0);
    XtVaSetValues(XtParent(d), XmNtitle, "Add Site ID", NULL);
    xmstr = XmStringCreateLtoR("Site ID to add:", XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(d, XmNselectionLabelString, xmstr, 
             XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL, NULL);
    XtAddCallback(d, XmNcancelCallback, site_edit_add_cancel_callback, NULL);
    XtAddCallback(d, XmNokCallback, site_edit_add_ok_callback, NULL);
    XmStringFree(xmstr);

    XtManageChild(d);
}



void site_edit_add_cancel_callback(Widget w, XtPointer client_data, 
                                                                 XtPointer call_data) 
{
    XtUnmanageChild(w);
}


/* if the site add is okayed, then we check to see if the input is
 * good, and if it is, we bring it up for editing
 */
void site_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data) 
{
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *)call_data;
    char *new_sid_str, buf[20];
    int i, new_sid, list_size;
    Widget d;
    XmString xmstr;
    
    char sub[25];
    int k, j;

    /* get the new pid in string form */
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &new_sid_str);


    /* find a number and complain if it isn't one */
    /* skip over any initial spaces */
    k=0;
    while(new_sid_str[k] == ' ') k++;

    /* read in an integer (the prod id) */
    j=0;
    while(isdigit((int)(new_sid_str[k]))) 
        sub[j++] = new_sid_str[k++];
    sub[j] = '\0';
    free(new_sid_str);
    
    /* check if we got digits */
    if(j==0) {
        d = XmCreateErrorDialog(w, "Error", NULL, 0);
        xmstr = XmStringCreateLtoR("The entered value is not a number.\nPlease try again.",
                   XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(d, XmNmessageString, xmstr, NULL);
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
        XtManageChild(d);
        XmStringFree(xmstr);
        return;
        
    } else 
        new_sid = atoi(sub);



    /* check to see if the number exists already.  if it does, complain */
    for(i=0; i<icao_list->size; i++)
        if(new_sid == icao_list->keys[i]) {
            d = XmCreateInformationDialog(w, "Error", NULL, 0);
            xmstr = XmStringCreateLtoR("The specified site ID number already exists.",
                           XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            return;
        }

    /* check to see if the number is within a defined range */
    if( !((new_sid >=0) && (new_sid <= 999)) &&  /*  WSR-88D */
        !((new_sid >=3000) && (new_sid <= 3050)) ) { /*  TDWR */
            d = XmCreateInformationDialog(w, "Error", NULL, 0);
            xmstr = XmStringCreateLtoR("The specified site ID is not in a defined range.\n 0-999 for WSR-88D; 3000-3050 for TDWR",
                           XmFONTLIST_DEFAULT_TAG);
            XtVaSetValues(d, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d);
            return;
    }

    /* if not, add default values for the pid in to the preference fields */
    assoc_insert_i(radar_type_list, new_sid, 0);
    assoc_insert_s(icao_list, new_sid, " ");    
    
    /* then set it up to be edited */
    sprintf(buf, "%d", new_sid);
    xmstr = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
    
    XtVaSetValues(site_id_label, XmNlabelString, xmstr, NULL);
    
    XtVaGetValues(si_list, XmNitemCount, &list_size, NULL);
    XmListAddItem(si_list, xmstr, list_size+1);
    XmListSelectItem(si_list, xmstr, True);
    XtVaSetValues(XtParent(si_list), XmNwidth, 130, NULL);
    XtUnmanageChild(w);
}



/* ***************************************************************************** */
/* ***************************************************************************** */







void area_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}



void a_label_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *label_selected = (int *)client_data;  

    area_label = *label_selected;

    area_prev_grey_pixmap();
    display_area(0, PREFS_FRAME);
    area_prev_show_pixmap();

}



void a_symbol_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    int *symbol_selected = (int *)client_data;  

    area_symbol = *symbol_selected;

    area_prev_grey_pixmap();
    display_area(0, PREFS_FRAME);
    area_prev_show_pixmap();

}



void a_line_radio_callback(Widget w, XtPointer client_data, XtPointer call_data)

{
    int *line_selected = (int *)client_data;  

    area_line_type = *line_selected;

    area_prev_grey_pixmap();
    display_area(0, PREFS_FRAME);
    area_prev_show_pixmap();
            
}



void a_line_points_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)call_data;

  if(cbs->set) {
    include_points_flag = TRUE;  
  } else {
    include_points_flag = FALSE;
  }
  
  area_prev_grey_pixmap();
  display_area(0, PREFS_FRAME);
  area_prev_show_pixmap();
            
}
   

