/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:56 $
 * $Id: symbology_block.c,v 1.12 2009/05/15 17:52:56 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/* symbology_block.c */

#include "symbology_block.h"


static int accept_block_error = FALSE;


/* Accomplishes a consistency check for block lengths with product length,
 * a bytswap of the product header (MHB & PDB), then parses the rest of the 
 * product and a) byteswaps as necessary and b) creates a list of the layers
 * and packets / generic components present in the product.
 *
 * Returns -3 with unrecoverable PARSE ERROR, -2 with BLOCK LENGTH ERROR,
 * +1 for GOOD_LOAD (SUCCESS)
 */
int parse_packet_numbers(char *buffer) 
{
    time_t tval;
    int l_idx=0, j, k, offset, result;

    int num_sym_layers;
    Prod_header *hdr;
    Graphic_product *gp;
    int  *type_ptr, msg_type;

    Widget dialog, d_block, accept_box;
    XmString xmstr;

    int layer_index=0;/*  counts total number of packets plus number of layers */
                      /*  beginning with 0!  This is used to set the list index */
                      /*  of the layer in the packet / component selection */
                      /*  dialog.  First layer is at position 0! */


 
    /* first make sure that the buffer had data in it */
    if(buffer==NULL) {
      fprintf(stderr,"Summary Info ERROR: The product buffer is empty\n");
      return PARSE_ERROR;
    }

    /* load the Pre-ICD Header */
    hdr=(Prod_header*)buffer;

   /* load the Message Header block and Product Description Block */
    gp=(Graphic_product*)(buffer+96);


    /* do some checks to make sure that this is a valid ICD product */
    /*  THE TEST FOR ICD MOVED TO ALL OF THE PRODUCT LOAD FUNCTIONS!!!  */



    if((result=check_offset_length(buffer+96, FALSE))==FALSE) {
 
         fprintf(stderr,"PARSING PACKETS - Error in Block Length Test\n");

        if(accept_block_error == FALSE) {
            
            xmstr = XmStringCreateLtoR(
             "The consistency check for the length of major product blocks has \n"
             "failed.  This could be a product not yet supported by CVG or     \n"
             "there may be an error in this product.                           \n\n"
                
             "See terminal output for details.                                 \n\n"
                
             "Attempting to display this product may cause CVG to crash.  If   \n"
             "you wish to attempt display of this product check the box below. \n"
             "and reselect the product for display.                            \n\n"
                
             "VAD, STP, OHP, and THP have a minor error in number of blocks.   \n",
                XmFONTLIST_DEFAULT_TAG);
    
            d_block = XmCreateErrorDialog(shell, "block_test", NULL, 0);
            XtVaSetValues(XtParent(d_block), XmNtitle, "CAUTION", NULL);
            XtVaSetValues(d_block, XmNmessageString, xmstr, NULL);
            XtUnmanageChild(XmMessageBoxGetChild(d_block, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(d_block, XmDIALOG_CANCEL_BUTTON));
            XtManageChild(d_block);
    
            accept_box = XtVaCreateManagedWidget("Accept this error.            "
                                            "                                   "
                                         "                                  \n\n"
                                         "Then reattempt to display the product.", 
                 xmToggleButtonWidgetClass,  d_block,
                 XmNset,                     XmUNSET,
                 NULL);
            XtAddCallback(accept_box, XmNvalueChangedCallback, 
                                                      accept_block_error_cb, NULL);
            
            XmStringFree(xmstr);
            
        } /*  end if accept_block is FALSE */

        if(accept_block_error == FALSE) {
            return BLK_LEN_ERROR;
            
        } else {
            accept_block_error = FALSE;
        }/*  end if accept_block is FALSE */
 

    } /*  end if block length test fails */




/* ///////////////////////////////////////////////////////////////////////// */
/*  BEGINNING PRODUCT BYTE-SWAPPING HERE */

/*LINUX change - swaps the mhb and pdb in place */
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_short_swap(gp,60);
    gp->msg_time=INT_SSWAP(gp->msg_time);
    gp->msg_len=INT_SSWAP(gp->msg_len);
    gp->latitude=INT_SSWAP(gp->latitude);
    gp->longitude=INT_SSWAP(gp->longitude);
    gp->gen_time=INT_SSWAP(gp->gen_time);
    gp->sym_off=INT_SSWAP(gp->sym_off);
    gp->gra_off=INT_SSWAP(gp->gra_off);
    gp->tab_off=INT_SSWAP(gp->tab_off);
#endif
    
    /* begin to output specific product information */
    if(verbose_flag) {
        fprintf(stderr,"\nPRODUCT SUMMARY INFORMATION\n");
        fprintf(stderr,"-------------------------------------\n");

        fprintf(stderr,"Message/Product Code:\t\t\t\t%hd\n",gp->msg_code);
        fprintf(stderr,"Buffer/Product ID:\t\t\t\t%hd\n",hdr->g.prod_id);

        if(hdr->g.len==0) { /*  no internal Pre-ICD header */
            fprintf(stderr,
                "Product Binary did not include the internal 96-byte header\n\n");
            fprintf(stderr,
                "----- Summary Information Not Printed -----\n\n");
        } else {
            tval=(time_t)hdr->g.vol_t;
            fprintf(stderr,"Volume Scan Start Time\t\t\t\t%s",ctime(&tval));
            fprintf(stderr,
               "Total Product Length (bytes)\t\t\t%d (96 byte hdr not included)\n",
                                                                    hdr->g.len-96);
            fprintf(stderr,"Volume Scan Sequence Number\t\t\t%d\n",hdr->g.vol_num);
            fprintf(stderr,"Elevation Count\t\t\t\t\t%hd\n",hdr->elev_cnt);
            fprintf(stderr,"Elevation Index\t\t\t\t\t%hd\n",
                                       get_elev_ind(buffer,orpg_build_i));
            fprintf(stderr,"Weather Mode\t\t\t\t\t%hd\n",hdr->wx_mode);
            fprintf(stderr,"VCP Number\t\t\t\t\t%hd\n",hdr->vcp_num);
            fprintf(stderr,"-------------------------------------\n\n");
    
        } /*  end else g.len not == 0 */
    
    } /*  end verbose_flag  */
  
  
  /* Read the 16 threshold level fields and 10 product dependent */
  /* parameters into structures containing variables of several  */
  /* types for future custom decoding                            */
  
  
  
  
    /* if there's info we're not storing laying around, 
     * we can free it without problem */
     
    if(sd->layers != NULL) {
        delete_layer_info(sd->layers, sd->num_layers);
    }
    sd->layers = NULL;


    /* get info about the type of product this is */
    type_ptr = assoc_access_i(msg_type_list, hdr->g.prod_id);
    if(type_ptr == NULL) {
        msg_type = GEOGRAPHIC_PRODUCT; /* default */
    } else {
        msg_type = *type_ptr;
    }

/* ///////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */
    /* there're a couple special cases to handle first*/
    if(msg_type == STANDALONE_TABULAR) {
    /* for a standalone tabular alphanumeric product,  
     * This structure is 
     * not identical to a TAB and needs individual treatment as a 
     * STAND_ALONE_TAB_ALPHA_BLOCK. It is byte-swapped as required and 
     * loaded here.  Product 49 Storm Structure (code 62) also includes
     * unique data after the initial SATAP block which is ignored.
     */
    
        int offset=0;

        short divtemp=0;
        short pages=0, numchars=0, pagecount=0;
                
        sd->num_layers = 1;
           
        offset = 96 + 120; /* set offset to beginning of SATAP block */ 
        /* read SATAP header (divider and number of pages */
        divtemp = read_half_flip(buffer, &offset);
        pages = read_half_flip(buffer, &offset);        
                

        for(pagecount=0;pagecount<pages;pagecount++) {
            numchars=read_half_flip(buffer,&offset);
            while(numchars>0) { /* if it's -1 it goes to the next page */
                offset+=numchars;
                numchars=read_half_flip(buffer,&offset);
            }
        }
        
        /* old logic to WAS treat as a TAB */   
        sd->layers = (layer_info *)malloc(sizeof(layer_info)*(sd->num_layers));
        sd->layers[0].codes = (int *)malloc(sizeof(int));
    
        sd->layers[0].codes[0] = STAND_ALONE_TAB_ALPHA_BLOCK;
        sd->layers[0].offsets = (int *)malloc(sizeof(int));
    
        sd->layers[0].offsets[0] = 96 + 120;
        sd->layers[0].num_packets = 1;
        sd->layers[0].index = 0;
    
        return GOOD_LOAD;
    
    } else if(msg_type == RADAR_CODED_MESSAGE) {
        /* we don't display radar coded messages yet, so tell the
         * user and warn them off 
         */
        dialog = XmCreateInformationDialog(shell, "no_rcm", NULL, 0);
        xmstr = XmStringCreateLtoR(
             "CVG does not currently support radar coded messages for display.", 
                                                         XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(dialog, XmNmessageString, xmstr, NULL);
        XtVaSetValues(XtParent(dialog), XmNtitle, "Unsupported Product", NULL); 
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));  
        XtManageChild(dialog);
        XmStringFree(xmstr);
        return PARSE_ERROR;
    
/* ///////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */
    } else if(msg_type == TEXT_MESSAGE) {
        /* we don't display text messages yet, so tell the
         * user and warn them off 
         */
        dialog = XmCreateInformationDialog(shell, "no_FTM", NULL, 0);
        xmstr = XmStringCreateLtoR(
             "CVG does not currently support free text messages for display.", 
                                                         XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(dialog, XmNmessageString, xmstr, NULL);
        XtVaSetValues(XtParent(dialog), XmNtitle, "Unsupported Product", NULL); 
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));  
        XtManageChild(dialog);

        XmStringFree(xmstr); 
        return PARSE_ERROR;
    
/* ///////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */
    } else if(msg_type == UNKNOWN_MESSAGE) {
        /* we don't display things like alert messages or intermediate products
         * this catches them, just in case
         */
        dialog = XmCreateInformationDialog(shell, "unk", NULL, 0);
        xmstr = XmStringCreateLtoR(
             "CVG does not currently support this type of message for display.", 
                                                          XmFONTLIST_DEFAULT_TAG);
        XtVaSetValues(dialog, XmNmessageString, xmstr, NULL);
        XtVaSetValues(XtParent(dialog), XmNtitle, "Unsupported Product", NULL);  
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON)); 
        XtManageChild(dialog);

        XmStringFree(xmstr);
        return PARSE_ERROR;
    }
    
/* ///////////////////////////////////////////////////////////////////////// */
/* ///////////////////////////////////////////////////////////////////////// */
    /* otherwise, we know that we have a graphic product on our hands */

    num_sym_layers = sd->num_layers = 0;

    sd->this_image = NO_IMAGE;



    /* list whether or not symbology, alpha and graphicalpha blocks
     * are included */


     
    if(gp->sym_off>0) {
        /* load the symbology block header */
        Sym_hdr *sh;
    
        sh = (Sym_hdr*)(buffer + 216);
/*LINUX change - swaps the symbology block in place */
#ifdef LITTLE_ENDIAN_MACHINE
        MISC_short_swap(sh,5);
        sh->block_length=INT_SSWAP(sh->block_length);
#endif
                  
        num_sym_layers = sd->num_layers = sh->n_layers;
    

        if(num_sym_layers > 18) {
            printf("WARNING: Number of Layers (%d) is out of range (1-18).\n",
                                                              sd->num_layers);
            printf(
              "         CVG will accept up to 30 layers for development purposes.\n");
        }

        if(num_sym_layers > 30) { /*  accepting 30  */
            
            fprintf(stderr,
                           "\n==== ERROR PARSING SYMBOLOGY BLOCK =================\n"
                           "      Number of layers (%d) excessive\n"
                           "      Continuing parsing for diagnostic purposes only.\n\n"
                           "      Either a bad product or a mis-configured product.\n"
                           "====================================================\n\n",
                                                    num_sym_layers);

            num_sym_layers = 30;
        }
        
        
        if(verbose_flag) {
            fprintf(stderr,"Symbology Block Information:\n");
            fprintf(stderr,"  Number of Layers=%i\n",sh->n_layers);
        }
    
        /* calculate offset to the data packets */
        offset = 226;
        if(verbose_flag)
            fprintf(stderr,"symbology offset=%i initial offset value=%i\n",
                                                         gp->sym_off,offset);
    
       
        /* we display the GAB and TAB as layers, 
           so we need to add them to the list */
        if(gp->gra_off > 0) 
            (sd->num_layers)++;
        if(gp->tab_off > 0)
            (sd->num_layers)++;
    
        /* During animation, need to check if there are no layers */
        if(sd->num_layers == 0) {
            fprintf(stderr,"in parse_packet_numbers(): No LAYERS FOR DISPLAY\n");
        
            return GOOD_LOAD;
        }
    
        /* create enough layer infos to hold whatever we need */
        sd->layers = (layer_info *)malloc(sizeof(layer_info)*(sd->num_layers));
    

    
/* ///////////////////////////////////////////////////////////////////////// */
        for(l_idx=0; l_idx<num_sym_layers; l_idx++) {
            int length = 0, end_offset;
            unsigned short pcode;
            grow_array *pcodes, *poffsets;
            int num_packets;   /*  counts number of packets in a layer */
         
            num_packets = 0;
            
            if(verbose_flag)
                fprintf(stderr,"->begin processing for layer %i\n",l_idx);
    
            /* set up  smart arrays */
            grow_init(&pcodes);
            grow_init(&poffsets);
    
            /* set up as much of the layer block as we can */
            

            sd->layers[l_idx].index = layer_index;
            
            read_half_flip(buffer,&offset); /* advance beyond the layer divider */
            
            length=read_word_flip(buffer,&offset); /* read the layer length */
                          /* does NOT include the divider and length fields */ 

            if(length==0) {
                fprintf(stderr,
                    "CAUTION- Non standard structure layer %d is empty.\n", 
                                                                    l_idx+1);
                /***** LOGIC FOR EMPTY LAYER NOT TESTED *****/
                sd->layers[l_idx].codes = NULL;
                sd->layers[l_idx].offsets = NULL;
                sd->layers[l_idx].num_packets = 0; /*  THIS IS CRITICAL */
                sd->layers[l_idx].index = layer_index++; /*  REQUIRED */
                grow_delete(pcodes);
                grow_delete(poffsets);
                /***** LOGIC FOR EMPTY LAYER NOT TESTED *****/
                continue;
            }
            
            if(verbose_flag) {
                fprintf(stderr,"->length of data layer %d. num_packets %d\n",
                                                          length,num_packets);
                fprintf(stderr,"  Layer %2d Length=%d bytes  offset %d\n",
                   l_idx,length,grow_access(poffsets, l_idx));
            }
            /*  reading the very first packet in the layer */
            pcode=read_half_flip(buffer,&offset); /* read packet code */

            end_offset = offset + length - 4; /* offset of last HW in layer */
            if(verbose_flag) {
                fprintf(stderr,"           Packet Code=%x Hex or %d decimal\n",
                                                                  pcode,pcode);
            fprintf(stderr,"curent offset value=%d or %x hex\n",offset,offset);
            fprintf(stderr,"End offset target value=%d or %x hex\n",
                                                      end_offset,end_offset);
            }
    
            while(offset<end_offset) {
                int ser_len, ret;
                char *serial_data;  /* offset to the serialized data */
                char *my_buf;
                packet_28_t *pkt28hdr;
                RPGP_product_t *generic_product;
                RPGP_area_t **gencomps;
                int i;
                
                if(verbose_flag)
                fprintf(stderr,"*** parsing symbology packet: "
                               "current offset=%x end_offset=%x\n",
                                                  offset,end_offset);
                /*  GENERIC PRODUCT */
                if(pcode==28) { /*  A GENERIC PRODUCT PACKET */
                    /*  WE ASSUME THAT THERE IS ONLY ONE PACKET 28 IN A PRODUCT */
                    /*  save the offset to packet 28 for use by  */
                    /*  save_current_state_to_history */
                    sd->packet_28_offset = offset-2;
                    /*  call function to deserialize and parse for components */
                    my_buf = &buffer[offset-2];
                    pkt28hdr = (packet_28_t *) my_buf;
                    ser_len = pkt28hdr->num_bytes;
#ifdef LITTLE_ENDIAN_MACHINE
                    ser_len = INT_BSWAP(ser_len);
#endif
/*  DEBUG */
/* fprintf(stderr,"DEBUG PARSE PACKETS- PACKET 28 NUMBER OF BYTES IS %d\n",  */
/*                                                                 ser_len); */
                    /* use a local variable rather than offset ?? */
                    serial_data = &buffer[offset+6];
                      
                    ret = cvg_RPGP_product_deserialize(serial_data, ser_len, 
                                                (void *)&sd->generic_prod_data);
                    if(ret < 0) {
                        fprintf(stderr, 
                          "ERROR - Unable to deserialize the data in packet 28\n");
                        return PARSE_ERROR; 
                    }  
                    
                    /*  we place each component into the arrays as a packet */
                    /*     a. packet number is 2801 - 2806 corresponds to  */
                    /*        component types 1 - 6 */
                    /*     b. the offset is the index to the generic product  */
                    /*        component array rather than the byte offset into  */
                    /*        the packet */
                    
                    generic_product = (RPGP_product_t *)sd->generic_prod_data;
                    
                    
                    gencomps = (RPGP_area_t **)generic_product->components;
                    for (i = 0; i < generic_product->numof_components; i++) {
                        
                        switch (gencomps[i]->comp_type) {
                            case RPGP_RADIAL_COMP:
                                grow_append(pcodes, RPGP_RADIAL_COMP+2800);
                                sd->this_image = GENERIC_RADIAL; 
                                break;
                            case RPGP_GRID_COMP:
                                grow_append(pcodes, RPGP_GRID_COMP+2800);
                                sd->this_image = GENERIC_GRID; 
                                break;
                            case RPGP_AREA_COMP:
                                grow_append(pcodes, RPGP_AREA_COMP+2800);
                                break;
                            case RPGP_TEXT_COMP:
                                grow_append(pcodes, RPGP_TEXT_COMP+2800);
                                break;
                            case RPGP_TABLE_COMP:
                                grow_append(pcodes, RPGP_TABLE_COMP+2800);
                                break;
                            case RPGP_EVENT_COMP:
                                grow_append(pcodes, RPGP_EVENT_COMP+2800);
                                break;
                            default:
                                break;
                        } /*  end switch */
                        
                        grow_append(poffsets, i); /* the offset to the packet*/
                        num_packets++; 
                        layer_index++; /*  total layers _and_ packets we have */
                    } /*  END FOR */

                } else { /*  A TRADITIONAL PRODUCT PACKET */
                    num_packets++; 
                    grow_append(pcodes, pcode);       /* store the packet code */
                    grow_append(poffsets, offset-2); /* the offset to the packet*/
                    layer_index++;  /*  how many layers _and_ packets we have */
                }
                

                if(skip_over_packet(pcode, &offset, buffer) == FALSE) {
                    fprintf(stderr,
                        "\n==============================================\n"
                        "ERROR PARSING PACKETS IN SYMBOLOGY BLOCK\n"
                        "      Invalid packet number %d at offset %d\n\n"
                        "      Parsing packets terminated.\n"
                        "      Number of packets: %d, total layers & packets: %d\n"
                        "==============================================\n\n",
                                     pcode, offset-96, num_packets, layer_index+1);
                /*  THIS CODE HAS ONLY BEEN TESTED ON THE FIRST PACKET BAD */
                    sd->num_layers = layer_index+1 - num_packets;
                    sd->layers[sd->num_layers].codes = 
                                        (int *)malloc(sizeof(int)*num_packets);
                    sd->layers[sd->num_layers].offsets = 
                                        (int *)malloc(sizeof(int)*num_packets);
                    sd->layers[sd->num_layers].num_packets = num_packets;
                  
                    /* dump the data from the local storage to global storage */
                    memcpy(sd->layers[sd->num_layers].codes, pcodes->data, 
                                                 sizeof(int)*num_packets);
                    memcpy(sd->layers[sd->num_layers].offsets, poffsets->data, 
                                                 sizeof(int)*num_packets);
                    grow_delete(pcodes);                 
                    grow_delete(poffsets);
                    
                    return GOOD_LOAD;  /*  in order to display packets so far */
                    
                } /*  end if bad packet number */

/*  TEST ERROR HANDLING BY CALLER */
/* if(hdr->g.prod_id == 2) */
/* return PARSE_ERROR; */

                
                
                if(verbose_flag)
                    fprintf(stderr,"after skip %d: offset=%x  end_offset=%x\n",
                                                      pcode,offset,end_offset);


    
                /* if there's more data in the layer, set up for next pass */
                if((offset<end_offset) &&
                    (pcode!=28)      ) {  /* A TEMPORARY LIMITATION */
                    pcode=read_half_flip(buffer,&offset);
                    if(verbose_flag)
                        fprintf(stderr,"New packet number=%hd at offset %d\n",
                                                               pcode,offset-2);
                } else {  /* otherwise, stuff the data we have now in the 
                           * layer structure */
                  /* first, make us some memory */
                  sd->layers[l_idx].codes = 
                                        (int *)malloc(sizeof(int)*num_packets);
                  sd->layers[l_idx].offsets = 
                                        (int *)malloc(sizeof(int)*num_packets);
                  sd->layers[l_idx].num_packets = num_packets;
                  
                  /* dump the data from the local storage to global storage */
                  memcpy(sd->layers[l_idx].codes, pcodes->data, 
                                                 sizeof(int)*num_packets);
                  memcpy(sd->layers[l_idx].offsets, poffsets->data, 
                                                 sizeof(int)*num_packets);
              
                  /* output info about the layer data */
                  if(verbose_flag) {
                    fprintf(stderr,"LAYER #%d DATA:\n", l_idx);
                    fprintf(stderr,
                          "index=%d  number of packets=%d\npacket codes: ", 
                       sd->layers[l_idx].index, sd->layers[l_idx].num_packets);
                       
                    for(j=0; j<num_packets; j++)
                        fprintf(stderr,"%x ", sd->layers[l_idx].codes[j]);
                        
                    fprintf(stderr,"\npacket offsets: ");
                    for(j=0; j<num_packets; j++)
                      fprintf(stderr,"%d ", sd->layers[l_idx].offsets[j]);
                    fprintf(stderr,"\n");
                  }
                  break;
                }
            } /* end  while offset<end_offset */

           /*  the layer increment here, at the end of the for loop      */
            layer_index++;  /*  how many layers _and_ packets we have */
                            /*  increment for the next layer          */
            /* clean up */                 
            grow_delete(pcodes);                 
            grow_delete(poffsets);
                                                       
        } /* end for l_idx<num_layers */                                      
/* ///////////////////////////////////////////////////////////////////////// */
                                   
                                   
    } else {                         
        if(verbose_flag)           
        fprintf(stderr,"Symbology Block is NOT available\n");

    } /* end of symbology blocks */
                                   



                                   
    if(gp->gra_off>0) {            
                                   
#ifdef LITTLE_ENDIAN_MACHINE        
        short pagecount=0, pagelength=0, pagenum=0;
        short pagelength_running_total=0, pcode=0;
#endif                             
                                   
        int offset=0;              
        int length;                
                                   
        short pages=0;             
        /* if no symbology block, number of layers has not been counted, 
         * so increment here */
        if(gp->sym_off == 0)       
           (sd->num_layers)++;     
        /*LINUX change*/           
        offset=96 + (2 * gp->gra_off)+4; /* set offset to gab length value*/
        MISC_short_swap(buffer+offset-4,2); 
                                   
        /* if not allocated in the symbology block logic, allocate it here */
        /* THE REASON FOR 2*sizeof(layer_info) is to cover for a possible TAB */
        if(sd->layers == NULL)     
            sd->layers = (layer_info *)malloc(2*sizeof(layer_info));
        /* resize the layer array so that we can add on one more layer */
        /* for now, just add the GAB as a packet--we might want to decompose it
         * and list its contents later on */
        sd->layers[l_idx].codes = (int *)malloc(sizeof(int));
        sd->layers[l_idx].offsets = (int *)malloc(sizeof(int));
        sd->layers[l_idx].num_packets = 1;
        /* if there is no symbology block, the index must be 0 */
        if(l_idx==0)               
            sd->layers[l_idx].index = 0;
        else                       
        sd->layers[l_idx].index = sd->layers[l_idx-1].index + 
                              sd->layers[l_idx-1].num_packets + 1;
        sd->layers[l_idx].codes[0] = GRAPHIC_ALPHA_BLOCK;
        sd->layers[l_idx].offsets[0] = offset - 4;
                                   
        /*LINUX changes for the purpose of parsing the GAB to swap it.*/
        length = read_word_flip(buffer, &offset); /* read length field */ 
        pages = read_half_flip(buffer, &offset);
#ifdef LITTLE_ENDIAN_MACHINE       
        for(pagecount=0;pagecount<pages;pagecount++) {
            pagenum=read_half_flip(buffer,&offset);
            pagelength=read_half_flip(buffer,&offset);
            /* this has to account for the relative position of the offset */
            while( (offset-(96+(2*gp->gra_off)+14+pagelength_running_total)) 
                                                               < pagelength ) {
                pcode=read_half_flip(buffer,&offset);  

                if(skip_over_packet(pcode, &offset, buffer) == FALSE) {
                    fprintf(stderr,
                          "\n==============================================\n"
                          "ERROR PARSING PACKETS IN GAB\n"
                          "      Invalid packet number %d at offset %d\n\n"
                          "      Parsing packets terminated.\n"
                          "==============================================\n\n",
                                          pcode, offset-96);

          /*  THIS CODE HAS NOT BEEN TESTED WITH A BAD PRODUCT */
                    return GOOD_LOAD;  /*  in order to display packets so far */
                    
                } /*  end if bad packet number */
                
            } 
                                 
            pagelength_running_total+=pagelength;
        }                          
#endif                             
                                   
        if(verbose_flag) {         
            fprintf(stderr,"Graphic Alphanumeric Block Information:\n");
            fprintf(stderr,"current offset=%d\n", offset);
            fprintf(stderr,"  Block Length=%d\n", length);
            fprintf(stderr,"  Number of Pages: %hd\n", pages);
        }                          
                                   
        /* output info about the layer data */
        j=l_idx;                   
        if(verbose_flag) {         
            fprintf(stderr,"LAYER #%d DATA:\n", j);
            fprintf(stderr,"index=%d  number of packets=%d\npacket codes: ", 
               sd->layers[j].index, sd->layers[j].num_packets);
            for(k=0; k<sd->layers[j].num_packets; k++)
              fprintf(stderr,"%x ", sd->layers[j].codes[k]);
            fprintf(stderr,"\npacket offsets: ");
            for(k=0; k<sd->layers[j].num_packets; k++)
              fprintf(stderr,"%d ", sd->layers[j].offsets[k]);
            fprintf(stderr,"\n");  
        }                          
                                   
        /* make sure our index into the layer list is just off the end  */
            /* of the array or in position for the TAB, if it exists */
        l_idx++;      
                     
    } else {                       
        if(verbose_flag)           
        fprintf(stderr,"Graphic Alphanumeric Block is NOT available\n");
    }                              
                                   
    if(gp->tab_off>0) {            
        int offset=0;              
        int length=0;              
        Graphic_product *gp2;      
        int divtemp;               
        short pages, numchars,pagecount;
                                   
        /* if no symbology block, number of layers has not been counted, 
         * so increment here */
        if(gp->sym_off == 0)       
           (sd->num_layers)++;     
                                   
        offset = 96 + (2 * gp->tab_off) + 4; /* set offset to tab length val */
        /*LINUX changes*/          
        MISC_short_swap(buffer+offset-4,2);
        length = read_word_flip(buffer, &offset); /* read length field */
            gp2=(Graphic_product *) (buffer+offset); 
#ifdef LITTLE_ENDIAN_MACHINE       
        MISC_short_swap(gp2,60);   
        gp2->msg_time=INT_SSWAP(gp2->msg_time);
        gp2->msg_len=INT_SSWAP(gp2->msg_len);
        gp2->latitude=INT_SSWAP(gp2->latitude);
        gp2->longitude=INT_SSWAP(gp2->longitude);
        gp2->gen_time=INT_SSWAP(gp2->gen_time);
        gp2->sym_off=INT_SSWAP(gp2->sym_off);
        gp2->gra_off=INT_SSWAP(gp2->gra_off);
        gp2->tab_off=INT_SSWAP(gp2->tab_off);
#endif                             
                                   
        if(verbose_flag) {         
            fprintf(stderr,"Tabular Alphanumeric Block Information:\n");
            fprintf(stderr,"  Block Length=%d\n", length);
        }                          
                                   
        /*LINUX changes*/          
        offset+=120;/* for the mhb and pdb */
        divtemp=read_half_flip(buffer,&offset);/* read divider */
        numchars=0;                
        pages=0;                   
        pagecount=0;               
        pages=read_half_flip(buffer,&offset);
        for(pagecount=0;pagecount<pages;pagecount++) {
            numchars=read_half_flip(buffer,&offset);
            while(numchars>0) { /* if it's -1 it goes to the next page */
                offset+=numchars;  
                numchars=read_half_flip(buffer,&offset);
            }                      
        }                          
                                   
        /* if not allocated in GAB or symbology block logic, 
         * allocate it here */
        if(sd->layers == NULL)     
            sd->layers = (layer_info *)malloc(sizeof(layer_info));
                                   
        /* resize the layer array so that we can add on one more layer */
        sd->layers[l_idx].codes = (int *)malloc(sizeof(int));
        sd->layers[l_idx].offsets = (int *)malloc(sizeof(int));
        sd->layers[l_idx].num_packets = 1;
            /* if there is no symbology block or GAB, the index must be 0 */
            if(l_idx==0)           
                sd->layers[l_idx].index = 0;
            else                   
            sd->layers[l_idx].index = sd->layers[l_idx-1].index + 
                              sd->layers[l_idx-1].num_packets + 1;
        sd->layers[l_idx].codes[0] = TABULAR_ALPHA_BLOCK;
        sd->layers[l_idx].offsets[0] = 96 + gp->tab_off*2;
                                   
    } else {                       
        if(verbose_flag)           
        fprintf(stderr,"Tabular Alphanumeric Block is NOT available\n");
    }                              
                                   
/* ///////////////////////////////////////////////////////////////////////// */
    /* During animation, need to check if there are no layers */
    if(sd->num_layers == 0) {      
    fprintf(stderr,"in parse_packet_numbers (no symb): No LAYERS FOR DISPLAY\n");

        return GOOD_LOAD;
    }


    return GOOD_LOAD;

} /*  end parse_packet_numbers */







void accept_block_error_cb(Widget w,XtPointer client_data, XtPointer call_data)
{

    XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) call_data;
    
    if(cbs->set == XmSET) {        
        accept_block_error = TRUE;
       
    } else
        accept_block_error = FALSE;
            
    
}





  /* skip over the packet data.  since they're all essentially, or at least  
   * potentially different, we need to use different functions for each one.*/
  /* this is where required byte-swapping is accomplished on a Linux platform */
  
  /* return for bad packet code., in the future we need to 
   * modify all of the skip_packet functions to return for bad offset
   */
int skip_over_packet(int pcode, int *offset, char *buffer)
{

int retval=TRUE;

  switch(pcode) {
/*
 * case 0:
 *   packet_0_skip(buffer, offset);
 *   break;
 */
  case 1:
    packet_1_skip(buffer, offset);
    break;

  case 2:
    packet_2_skip(buffer, offset);
    break;

  case 3:
    packet_3_skip(buffer, offset);
    break;

  case 4:
    packet_4_skip(buffer, offset);
    break;       

  case 5:
    packet_5_skip(buffer, offset);
    break;

  case 6:
    packet_6_skip(buffer, offset);
    break; 

  case 7:
    packet_7_skip(buffer, offset);
    break;

  case 8:
    packet_8_skip(buffer, offset);
    break;  

  case 9:
    packet_9_skip(buffer, offset);
    break;

  case 10:
    packet_10_skip(buffer, offset);
    break;

  case 11:
    packet_11_skip(buffer, offset);
    break;

  case 12:
    packet_12_skip(buffer, offset);
    break;

/* Hail packets no longer used
 * case 13:
 *   packet_13_skip(buffer, offset);
 *   break;
 * case 14:
 *   packet_14_skip(buffer, offset);
 *   break;    
 */

  case 15:
    packet_15_skip(buffer, offset);
    break;

  case 16:
    packet_16_skip(buffer, offset);
    sd->this_image = DIGITAL_IMAGE;
    break;

  case 17:
    packet_17_skip(buffer, offset);
    sd->this_image = PRECIP_ARRAY_IMAGE;
    break;

  case 18:
    packet_18_skip(buffer, offset);
    sd->this_image = PRECIP_ARRAY_IMAGE;
    break;

  case 19:
    packet_19_skip(buffer, offset);
    break;

  case 20:
    packet_20_skip(buffer, offset);
    break; 

/* Cell Trend and Cell Trend VCP Time packets not
 *  displayed graphically, data in a TAB product
 * case 21:
 *   packet_21_skip(buffer, offset);
 *   break;  
 * case 22:
 *   packet_22_skip(buffer, offset);
 *   break; 
 */       

  case 23:
    packet_23_skip(buffer, offset);
    break;  

  case 24:
    packet_24_skip(buffer, offset);
    break;

  case 25:
    packet_25_skip(buffer, offset);
    break;

  case 26:
    packet_26_skip(buffer, offset);
    break; 

/*  case 27:
 *   SUPEROB NOT SUPPORTED
 */    

  case 28:  
    packet_28_skip(buffer, offset); 
    /*  sd->this_image set to GENERIC_RADIAL / GENERIC_GRID as appropriate */
    /*  in calling function parse_packet_numbers() */
    break;

  case 0x0802:
    packet_0802_skip(buffer, offset);
    break;     

  case 0x3501:
    packet_3501_skip(buffer, offset);
    break;

  case 0x0E03:
    packet_0E03_skip(buffer, offset);
    break;

  case 0xAF1F:
    packet_AF1F_skip(buffer, offset);
    sd->this_image = RLE_IMAGE;
    break;

  case 0xBA0F:
  case 0xBA07:
    packet_BA07_skip(buffer, offset);
    sd->this_image = RASTER_IMAGE;
    break;

/* The following is always defined as one larger than the enumerated value 
   used for the SATAP,  see packet_definitions.h  
   This packet type is not yet defined and not currently in use */
  case 59:
    digital_raster_skip(buffer, offset);
    break;

  default:
    if(verbose_flag)
        fprintf(stderr,"WARNING: Packet Code %d (%x Hex) "
                       "was not found for processing\n", pcode,pcode);

    retval = FALSE;
    
  } /* end switch */


  return retval;
  
} /*  END skip_over_packet */







/*  FOLLOWING PORTED FROM CVT */
/*  A. Must use read_word_swap_result (new function) */
/*     and read_half_swap_result (new function) in place */
/*     of read_word and read_half */
/*  B. Set verbose IAW verbose_flag. */
/*  C. Replace "CVT" with "CVG" */
/* checks block offsets, block lengths, and product length  */
/* prints message concerning suspected problem found        */
/* returns FASLE if unexpected value is found               */
/* Inputs: bufptr - pointer to beginning of product         */
/*         verbose - if TRUE provides more info w/ no error */
int check_offset_length(char *bufptr, int verbose) {

  Graphic_product *gp;

  int sum_len=0;     /* sum of layer / page lengths */
  int calc_length=0; /* calculated length of sym block / GAB */
  int symb_length=0; /* length of symbology block */   
  int gab_length=0;  /* length of gab */ 
  int tab_length=0;  /* length of tab */
  int calc_prod_length;   /* calculated product length */
  int message_length; 
  int opt_blocks;  /* number of optional blocks: symb, GAB, TAB */

  int symb_present = FALSE;
  int gab_present = FALSE;
  int tab_present = FALSE;
   
  /* the following valuses are set: -1 FALSE, 0 UNK or N/A, 1 TRUE */  
  int good_symb_offset=0; 
  int good_gab_offset=0;
  int good_tab_offset=0;  
  int good_symb_length=0; 
  int good_gab_length=0; 

  int symb_off, gab_off, tab_off;
  short n_blocks, divider; 
  int offset;  
  short symb_id, gab_id, tab_id;
  
  
  int retval=TRUE;  /* SET TO FALSE WITH ANY ERROR */


    fprintf(stderr, "\nPerforming Consistency Check of Product Length with\n");
    fprintf(stderr, "Block Offsets and Block Lengths\n");    
    
    if(verbose_flag == TRUE)
        verbose = TRUE;
    
    gp = (Graphic_product*)bufptr;

    symb_off = gp->sym_off;
    gab_off = gp->gra_off;
    tab_off = gp->tab_off;
    message_length = gp->msg_len;
    n_blocks = gp->n_blocks;

/* LINUX changes */
#ifdef LITTLE_ENDIAN_MACHINE
    symb_off = INT_BSWAP(symb_off);
    gab_off = INT_BSWAP(gab_off);
    tab_off = INT_BSWAP(tab_off);
    message_length = INT_BSWAP(message_length);
    n_blocks = SHORT_BSWAP(n_blocks);
#endif
    

/* TEST */
/* return FALSE; */
    
    opt_blocks = 0; 
    
    if(symb_off > 0) {
        opt_blocks++;
        symb_present = TRUE;
    }
    if(gab_off > 0) {
        opt_blocks++;
        gab_present = TRUE;
    }
    if(tab_off > 0) {
        opt_blocks++;
        tab_present = TRUE;
    }



           /*  a. the Block ID is 1 if there is only one page to the SATAP */
           /*  b. some SATAP have the GAB offset for some reason */
      /* IF SYMB PRESENT AND TAB IS NOT PRESENT           */
      /*   AND IF DIVIDER != -1                           */
      /*   THEN  REPORT RESULTS FOR SATAP AND QUIT.       */ 
    if((symb_present == TRUE) && (tab_present != TRUE)) {    
        short divider,pages,blockID; 
        int offset; /* offset in bytes */

        int characters, i;
        
        
         offset = (120);
         
         /* first optional block */
         divider = read_half_swap_result(bufptr,&offset); 
         
         /* block ID (if a symbology block) */
         blockID = read_half_swap_result(bufptr,&offset); 
         
         offset = (120+10);
         /* first layer (if a symbology block) */
         divider = read_half_swap_result(bufptr,&offset); 

        if( (divider != -1)) {  /*  no symb block layer divider */
        
            fprintf(stderr,"\n");
            fprintf(stderr, "--- Stand Alone Tabular Alphanumeric Product "
                            "(SATAP) Detected ---\n");
            if(symb_off != 60) {
                 printf("Error - Offset to SATAP (%d) is Incorrect\n",symb_off);
                 retval=FALSE;
            }
           offset = (120+2);
           pages = read_half_swap_result(bufptr,&offset);

           characters = read_half_swap_result(bufptr,&offset);
   
           if(verbose)
                fprintf(stderr,"    SATAP has %d pages\n",pages);
           if(pages<1 || pages>24) {
               printf("Warning - Number of Pages (%d) in the Stand Alone\n",pages);
               printf("         Alphanumeric product is out of range (1-24).\n");
               retval=FALSE;
            }

            if(retval != FALSE) {
                offset = (120 + 4);
                for(i=1; i<=pages; i++) {
                    characters = read_half_swap_result(bufptr,&offset);
                    if(characters<0 || characters>80) {
                        printf("Warning - Number of Characters (%d) in page %d\n",
                                                                    characters, i);
                        printf("          of the SATAP is out of range (0-80).\n");
                        retval=FALSE;
                    } /*  end if characters */
                    
                    while( (retval != FALSE) && (offset < (message_length)) ) {
                        divider = read_half_swap_result(bufptr,&offset);
                        
                        if(divider == -1) {
                            break;
                        }
                        
                    } /*  end while */
                    
                    if( offset > (message_length) ) {
                        printf("Warning - Page divider for page %d not found.\n",i);
                        printf("          Terminating SATAP check.\n");
                        retval=FALSE;
                        break;
                    }
                
               
                } /*  end for pages */
            
            } /*  end if retval not FALSE */
            
            fprintf(stderr,"\nFinished Consistency Check\n\n");
            return (retval);    
        }   /*  end if no layer 2 divider */
    } /*  end if symb_present and not tab_present */
  
    /* check number of blocks with non-zero block offsets */  
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Checking number of blocks vs. block offset values ---\n");

    /* Several existing products do not have a consistent value for the */
    /* number of blocks field: VAD, STP. OHP, and THP.  Because display */
    /* devices are not currently affected, this ERROR was modified      */
    /* to be a WARNING.                                                 */ 
    if((n_blocks - 2) == opt_blocks) { /* we are OK */ 
        if(verbose) {
             fprintf(stderr, "    Product has %d blocks:  the MHB and the PDB plus\n",
                 n_blocks); 
             if(symb_present == TRUE)
                 fprintf(stderr, "    the Symbology Block  ");
             if(gab_present == TRUE)
                 fprintf(stderr, " the GAB  ");
             if(tab_present == TRUE)
                 fprintf(stderr, " the TAB ");
             fprintf(stderr, "\n");   
        }  
    } else {
        fprintf(stderr, 
             "Warning - The number of Blocks %d (HW 9) does not agree with the\n",
                       n_blocks);
        fprintf(stderr, 
             "        optional blocks present as determined by the block offsets.\n");
        fprintf(stderr, "     Symbology Block Offset is %d\n",symb_off );
        fprintf(stderr, "     GAB Offset is %d\n",gab_off );
        fprintf(stderr, "     TAB Offset is %d\n",tab_off );
        fprintf(stderr, 
            "        which indicate the total number of blocks to be %d.\n",
                       (opt_blocks+2));
        fprintf(stderr, 
             "Several products (VAD, STP, OHP, and THP) have this error but \n");
        fprintf(stderr, "this has not affected current display systems. \n");
        
    } 
   
     
    /* check offsets to blocks present in this product*/
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Checking block offsets with block identifiers ---\n");  
    if(symb_present == TRUE) {
        offset = (2*symb_off);
        divider = read_half_swap_result(bufptr,&offset);
        
        symb_id = read_half_swap_result(bufptr,&offset);
        
        good_symb_offset = 1;
        if(divider != -1) {
            fprintf(stderr,
                 "Error - Symbology Block Divider is %d using offset %d (HW 55 & 56)\n",
                            divider, symb_off);  
            good_symb_offset = -1;
            retval=FALSE;
        }  
        if(symb_id != 1) {
            fprintf(stderr,
                 "Error - Symbology Block ID is %d using offset %d (HW 55 & 56)\n",
                            symb_id, symb_off);
            good_symb_offset = -1; 
            retval=FALSE;               
        }
        if(good_symb_offset == 1) {
            symb_length = read_word_swap_result(bufptr,&offset);
            
            if(verbose) 
                fprintf(stderr, 
                     "    Symbology Block Offset is %d halfwords (%d bytes)\n",
                           symb_off,(2*symb_off));
        } else 
            fprintf(stderr, 
                 "Possible Error in the Symbology Block Offset Detected\n");
    }
        
    if(gab_present == TRUE) {
        offset = (2*gab_off);
        divider = read_half_swap_result(bufptr,&offset);
        
        gab_id = read_half_swap_result(bufptr,&offset);
        
        good_gab_offset = 1;
        
        if(divider != -1) {
            fprintf(stderr,
                 "Error - GAB Divider is %d using GAB offset %d (HW 57 & 58)\n",
                            divider, gab_off);  
            good_gab_offset = -1;
            retval=FALSE;
        }  
        if(gab_id != 2) {
            fprintf(stderr,
                 "Error - GAB ID is %d using GAB offset %d (HW 57 & 58)\n",
                            gab_id, gab_off);
            good_gab_offset = -1; 
            retval=FALSE;               
        }
        if(good_gab_offset == 1) {   
            gab_length = read_word_swap_result(bufptr,&offset);
            
            if(verbose) 
                fprintf(stderr, "    GAB Offset is %d halfwords (%d bytes)\n",
                            gab_off,(2*gab_off)); 
        } else
            fprintf(stderr, "Possible Error in the GAB Offset Detected\n"); 
    }
      
    if(tab_present == TRUE) { 
        offset = (2*tab_off);
        divider = read_half_swap_result(bufptr,&offset);
        
        tab_id = read_half_swap_result(bufptr,&offset);
        
        good_tab_offset = 1;
        if(divider != -1) {
            fprintf(stderr,
                 "Error - TAB Divider is %d using TAB offset %d (HW 59 & 60)\n",
                            divider, tab_off);  
            good_tab_offset = -1; 
            retval=FALSE;
        }  
        if(tab_id != 3) {
            fprintf(stderr,
                 "Error - TAB ID is %d using TAB offset %d (HW 59 & 60)\n",
                            tab_id, tab_off);
            good_tab_offset = -1; 
            retval=FALSE;               
        }
        if(good_tab_offset == 1) {   
            tab_length = read_word_swap_result(bufptr,&offset);
            
            if(verbose) 
                fprintf(stderr, "    TAB Offset is %d halfwords (%d bytes)\n",
                            tab_off,(2*tab_off)); 
        } else
            fprintf(stderr, "Possible Error in the TAB Offset Detected\n");    
    }     
  
  
      
    /* IF SYMB PRESENT */
    /* parse through symbology block layers and check layer lengths */
    if(symb_off>0 && symb_off<=400000) {
      /* load the symbology block header */
      
      int i;
      short s_id;     
      short divider;  
      short layers;
      int offset; /* offset in bytes */

      fprintf(stderr,"\n");
      fprintf(stderr, "--- Checking Symbology Block layer lengths ---\n");

        offset = (2*symb_off) + 2; 

        s_id = read_half_swap_result(bufptr,&offset);

        if(s_id != 1)
            fprintf(stderr,
                 "Error - Not in Symbology Block as expected (block id is %d),\n"
                 "        check offset (HW 55 & 56)\n", s_id);

        else { /* continue  checking Symbology Block layer lengths */
            
            offset+=4;
            layers = read_half_swap_result(bufptr,&offset);
      
            if(verbose) 
                fprintf(stderr,"    Symbology Block Length is %d.\n",symb_length);     

            if(layers>18 || layers<1) {
                 fprintf(stderr,
                     "Warning - Number of Layers (%d) is out of range (1-18).\n",layers);
                 fprintf(stderr,
                     "          CVG will accept up to 30 layers for development purposes.\n");
            } else if(verbose) 
                 fprintf(stderr, "    Symbology block has %d layers.\n",layers);
    
            /* offset of the first symb layer divider */
            offset = (2 * symb_off) + 10;
            sum_len=0; 
            for(i=1;i<=layers;i++) {
                int length=0;
                         
                divider=read_half_swap_result(bufptr,&offset); /* read layer divider */
                
                if(divider != -1) {
                   fprintf(stderr,
                        "Error - Symbology Block Layer Divider Not Found\n");
                   fprintf(stderr,
                        "        Divider for Layer %d is '%d' rather than '-1'.\n",
                                         i, divider);
                }
                
                length=read_word_swap_result(bufptr,&offset); /*read the layer length */

                if(length==0)
                    fprintf(stderr,"CAUTION- Layer %2d Length of 0 bytes is not "
                                   "standard product construcion. -CAUTION\n", i);
                else
                    if(verbose) 
                        fprintf(stderr,"    Layer %2d Length = %d bytes\n",i,length);
                
                sum_len+=length; /* sum up length of symbology block  */
                
                offset+=length; /* advance pointer to beginning of next block */
            } /* end for */
        
            /* size of sym block is sum of layer lengths plus 6 bytes  */
            /* for each layer header and 10 bytes for the block header */
            calc_length = sum_len + (6 * layers) + 10;
            if (calc_length != symb_length) {
               fprintf(stderr,
                    "Error - Symbology Block Length (HW 63 and 64) does not agree\n");
               fprintf(stderr,
                    "        with the total length of the %d layer(s).\n",layers);
            }
            /* if GAB present, check for divider and ID */
            /* else if TAB present, check for divider and ID */
            
            if(gab_present == TRUE) {
                offset = (2*symb_off) + symb_length;
                divider = read_half_swap_result(bufptr,&offset);
                
                gab_id = read_half_swap_result(bufptr,&offset);
                
                good_symb_length = 1;
                if(divider != -1) {
                    fprintf(stderr,
                         "Error - GAB Divider is %d using Symb Block Length of %d\n",
                                    divider, symb_length);  
                    good_symb_length = -1;
                    retval=FALSE;
                }  
                if(gab_id != 2) {
                    fprintf(stderr,
                         "Error - GAB ID is %d using Symb Block Length of %d\n",
                                    gab_id, symb_length);
                    good_symb_length = -1; 
                    retval=FALSE;               
                }
                if(good_symb_length == -1)
                    fprintf(stderr,
                         "Possible Error in the Length of the Symbology Block Detected\n");
            
            } else if(tab_present == TRUE) {
                offset = (2*symb_off) + symb_length;
                divider = read_half_swap_result(bufptr,&offset);
                
                tab_id = read_half_swap_result(bufptr,&offset);
                
                good_symb_length = 1;
                if(divider != -1) {
                    fprintf(stderr,
                         "Error - TAB Divider is %d using Symb Block Length of %d\n",
                                    divider, symb_length);  
                    good_symb_length = -1;
                    retval=FALSE;
                }  
                if(tab_id != 3) {
                    fprintf(stderr,
                         "Error - TAB ID is %d using Symb Block Length of %d\n",
                                    tab_id, symb_length);
                    good_symb_length = -1; 
                    retval=FALSE;               
                }
                if(good_symb_length == -1)
                    fprintf(stderr,
                         "Possible Error in the Length of the Symbology Block Detected\n");            
            }
  
        } /* end continue checking layer lengths */
  
    } else if(symb_off>400000) {
        printf("Warning - Symbology Block offset is out of range\n");
        retval = FALSE;
        
    } /* end if symbology block present */
  
  
    /* parse through GAB pages and check page lengths */
  
    if(gab_off>0 && gab_off<=400000) {
        int offset=0;
        int length=0;
        short pages=0;
        short page_n;
        short g_id;    
        int i;

        fprintf(stderr,"\n");
        fprintf(stderr, "--- Checking GAB Page lengths ---\n"); 
        
        if(verbose) fprintf(stderr,"    GAB Length is %d.\n",gab_length);   
                     
        offset = (2*gab_off) + 2; 
        
        g_id = read_half_swap_result(bufptr,&offset);
        
        if(g_id != 2)        
            fprintf(stderr,"Error - Not in GAB as expected (block id is %d),\n"
                           "        check offset to GAB\n", g_id);

        else { /* continue  checking GAB page lengths */
            offset+=4;
            pages = read_half_swap_result(bufptr,&offset);
            
            if(verbose) fprintf(stderr,"    Number of GAB pages is %d\n",pages);
            sum_len=0;
            for(i=1;i<=pages;i++) {
                page_n = read_half_swap_result(bufptr,&offset);
                
                if(i != page_n)
                    fprintf(stderr,
                         "Error - Page Number for GAB Page %d is %d\n",i,page_n);
                length=read_half_swap_result(bufptr,&offset); /*read page length */
                
                if(verbose) 
                    fprintf(stderr,"    Page %2d Length = %d bytes\n",i,length);
                    
                sum_len+=length; /* sum up length of GAB pages  */         
                offset+=length; /* advance pointer to beginning of next page */ 
            }
            /* size of the GAB is sum of page lengths plus 4 bytes  */
            /* for each page header and 10 bytes for the GAB header */
            calc_length = sum_len + (4 * pages) + 10;
            if (calc_length != gab_length) {
               fprintf(stderr,
                    "Error - Graphic Alphanumeric Block Length does not agree\n");
               fprintf(stderr,
                    "        with the total length of the %d page(s).\n",pages);
               retval = FALSE;
            }            
        
            /* if TAB present check for beginning of TAB */
            if(tab_present == TRUE) {
               offset = (2*gab_off) + gab_length;
               divider = read_half_swap_result(bufptr,&offset);
               
               tab_id = read_half_swap_result(bufptr,&offset);
               
               good_gab_length = 1;
               if(divider != -1) {
                   fprintf(stderr,
                        "Error - TAB Divider is %d using GAB Length of %d\n",
                                   divider, gab_length);  
                   good_gab_length = -1;
                   retval=FALSE;
               }  
               if(tab_id != 3) {
                   fprintf(stderr,"Error - TAB ID is %d using GAB Length of %d\n",
                                   tab_id, gab_length);
                   good_gab_length = -1; 
                   retval=FALSE;               
               }
               if(good_gab_length == -1)
                   fprintf(stderr,
                        "Possible Error in the Length of the GAB Detected\n");  
            }
         
        }   /* end continue checking GAB page lengths */      

    } /* end if GAB present */
  
  
    /* parse through TAB pages and check page lengths */
    /* checked during the display of the TAB          */
   
  
    if(tab_off>0 && tab_off<=400000) {
        int offset=0;
        short pages=0;
        short t_id;     /*  cvg 8.2 added */

        fprintf(stderr,"\n");
        fprintf(stderr, "--- Checking TAB Page lengths ---\n"); 
        
        if(verbose) fprintf(stderr,"    TAB Length is %d.\n",tab_length);
                     
        offset = (2*tab_off) + 2; 
        
        t_id = read_half_swap_result(bufptr,&offset);
        
        if(t_id != 3)        
            fprintf(stderr,"Error - Not in TAB as expected (block id is %d),\n"
                           "        check offset to TAB\n", t_id);

        else { /* continue  checking TAB page lengths */
            offset+=126; /* skip block length, a MHB, a PDB, and a divider */
            pages = read_half_swap_result(bufptr,&offset);
            
            if(verbose) 
                fprintf(stderr,"    Number of TAB pages is %d\n",pages);
            if(verbose) 
                fprintf(stderr,
                     "TAB Page consistancy is checked when displaying the TAB.\n");
                            
        } /* end continue checking TAB pages */
        
    }  /* end if TAB present */
    
    
    
    /* check block lengths with product length */
    /* only accomplish if block offsets are correct */  
    fprintf(stderr,"\n");
    fprintf(stderr, "--- Comparing block lengths with product message length.\n");
    calc_prod_length = 120;  /* the length of the MHB and PDB */
    if(symb_present == TRUE)
        calc_prod_length+=symb_length;
    if(gab_present == TRUE)
        calc_prod_length+=gab_length;
    if(tab_present == TRUE)
        calc_prod_length+=tab_length;
    if(calc_prod_length != message_length) {
        fprintf(stderr,
             "ERROR - The product length (%d bytes) does not agree with the sum of the\n",
                   message_length);
        fprintf(stderr,
             "        block lengths plus 120 bytes for the MHB and PDB (%d bytes)\n",
                   calc_prod_length);
        fprintf(stderr,"\n");
        fprintf(stderr,
             "First address any errors reported in the individual block lengths\n");
        retval = FALSE;
    } else
        if(verbose) {
            fprintf(stderr, "    Product Message Length is %d bytes\n",message_length);
            fprintf(stderr, "    Standard headers (MHB & PDB): 120 bytes\n");
            if(symb_present == TRUE)
                fprintf(stderr, "    Symbology Block Length is %d bytes",symb_length);
            if(gab_present == TRUE)
                fprintf(stderr, "\n    GAB Length is %d bytes",gab_length);
            if(tab_present == TRUE)
            fprintf(stderr, "\n    TAB Length is %d bytes",tab_length);
            fprintf(stderr,"\n");
        } 
    fprintf(stderr,"\nFinished Consistency Check\n\n");
         
    return (retval);

} /* end check_offset_length */










/* ///////////////////////////////////////////////////////////// */

int read_word(char *buffer,int *offset)
 {
  /* read a word (4 bytes) from the buffer then
  imcrement the offset pointer */
 int d1;
 memcpy(&d1,buffer+*offset,4);
 
 *offset+=4;  /* increment offset counter */

  return(d1);
}




int read_word_swap_result(char *buffer,int *offset)
 {
  /* read a word (4 bytes) from the buffer then
  imcrement the offset pointer */
 int d1;
 memcpy(&d1,buffer+*offset,4);

#ifdef LITTLE_ENDIAN_MACHINE
d1=INT_BSWAP(d1);
#endif
 
 *offset+=4;  /* increment offset counter */

  return(d1);
}



/* swaps a word (4 bytes) in place (if required) and returns
 * the swapped value, increments offset pointer
 */
int read_word_flip(char *buffer,int *offset)
 {
  /* read a word (4 bytes) from the buffer then
  imcrement the offset pointer and swap if necessary */

  int d2;
  
#ifdef LITTLE_ENDIAN_MACHINE 
  int *d1=(int *)(buffer+*offset);
   *d1=INT_BSWAP(*d1);
#endif  
  memcpy(&d2,buffer+(*offset),4);
  *offset+=4;   /*increment offset counter */
  return(d2);
}


/* ///////////////////////////////////////////////////////////// */

short read_half(char *buffer,int *offset) 
{
  /* read a half word (2 bytes) from buffer then
  increment the offset pointer */
  short d1;
  memcpy(&d1,buffer+*offset,2);
   
  /*printf("read word value=%d\n",d1);*/
  *offset+=2;  /* increment offset counter */
  return(d1);
}





short read_half_swap_result(char *buffer,int *offset) 
{
  /* read a half word (2 bytes) from buffer then
  increment the offset pointer */
  short d1;
  memcpy(&d1,buffer+*offset,2);

#ifdef LITTLE_ENDIAN_MACHINE
d1=SHORT_BSWAP(d1);
#endif

  /*printf("read word value=%d\n",d1);*/
  *offset+=2;  /* increment offset counter */
  return(d1);
}



/* swaps a short (2 bytes) in place (if required) and returns
 * the swapped value, increments offset pointer
 */
short read_half_flip(char *buffer,int *offset)
{
  /*reads a half word (2 bytes) from buffer then increments 
   * the offset pointer, and swaps the halfword it read */

  short d2;
  
#ifdef LITTLE_ENDIAN_MACHINE
  short *d1=(short *)(buffer+*offset);
   *d1=SHORT_BSWAP(*d1);
#endif
  memcpy(&d2,buffer+(*offset),2);
  *offset+=2;
  return(d2);
}



/* ///////////////////////////////////////////////////////////// */

unsigned char read_byte(char *buffer,int *offset) 
{
  /* read a byte from buffer then
  increment the offset pointer */
  unsigned char c1;
  
  c1=(unsigned char)buffer[*offset];
  /*printf("byte=%c offset=%d\n",c1,*offset);*/
   
  *offset+=1;  /* increment offset counter */
  return(c1);
}

