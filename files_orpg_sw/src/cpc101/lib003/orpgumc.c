/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/27 14:41:40 $
 * $Id: orpgumc.c,v 1.51 2012/09/27 14:41:40 steves Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */ 

/***********************************************************************

    Description: product User Message Conversion functions.

***********************************************************************/
 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <infr.h>
#include <a309.h>
#include <basedata.h>
#include <orpg.h>
#include <prod_user_msg.h>

#include "orpgumc_def.h"

/* MESSAGE TYPE            MESSAGE LENGTH in sizeof halfword */
#define MSG_PROD_REQUEST_LEN		16
#define MSG_GEN_STATUS_LEN		100
#define MSG_REQ_RESPONSE_LEN 		24
#define MSG_MAX_CON_DISABLE_LEN    	14
#define MSG_ALERT_PARAMETER_LEN      	6
#define MSG_ALERT_REQUEST_LEN           13
#define MSG_ALERT_CAT_LEN               3
#define MSG_ALERT_BOX_LEN               4 * 58
#define MSG_PROD_LIST_LEN       	13
#define PROD_LIST_ENTRY_LEN     	7
#define MSG_ALERT_LEN           	25

/* Macro definitions for Bias Table type of Environmental Data */
#define MSG_BIAS_TABLE_BLOCK_ID          1
#define MSG_BIAS_TABLE_ROW_LEN          10
#define MSG_BIAS_TABLE_HDR_LEN          21

#define MSG_SIGN_ON_LEN             	18
#define MSG_PROD_REQ_CANCEL_LEN     	25
#define MSG_HEADER_LEN          	9
#define MSG_PRODUCT_LEN          	PHEADLNG
#define MSG_SYM_LEN          		5
#define MSG_GRA_LEN          		5
#define MSG_TAB_LEN          		4

#define TO_IEEE754			0
#define FROM_IEEE754			1

static int Prod_request_from_icd 
		(void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Max_con_disable_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Alert_request_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Sign_on_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Prod_list_req_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Environmental_data_from_icd
                (void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int External_data_from_icd
                (void *icd_msg, int icd_size, int hd_size, void **c_msg);
static int Gen_status_to_icd (void *c_msg, halfword * icd_msg);
static int Request_response_to_icd (void *c_msg, halfword * icd_msg);
static int Product_list_to_icd
		(void *c_msg, int c_size, int hd_size, void **icd_msg);
static int Alert_params_to_icd
		(void *c_msg, int c_size, int hd_size, void **icd_msg);
static int Msg_hd_block_to_icd (void *c_msg, int len, halfword *icd_mhb);

#ifdef LITTLE_ENDIAN_MACHINE
static int  Process_display_data_packets (unsigned short *block, 
                                          int tlen, int pcode);
static int  Validate_display_data_packets (unsigned short *block, 
                                           int tlen, int pcode);
static void Process_wx_alert_message (halfword *msg);
static void Process_graphic_product_message (halfword *msg, int size,
                                             int pcode, int to_icd);
static void Process_stand_alone_tabular_message (halfword *msg, int size,
                                                 int to_icd);
static void Process_rcm_tabular_product_message (halfword *msg);
#endif

static void Msg_hd_block_from_icd (halfword *icd_mhb, int len, void *c_msg);

#ifdef LITTLE_ENDIAN_MACHINE
static int  Validate_block_id( int block_id );
static void Dump_product_data(halfword *msg, int size);
static void Print_data (halfword *msg, int size);
#endif

/***************************************************************************

    Description: Converts a product user message from the ICD format to ORPG
		C structure format. Memory segment for the output message is
		allocated here. It must be freed by the caller. The caller
		can specify a reserved space in front of the output message.

    Inputs:	icd_msg - pointer to the ICD message
		icd_size - size of the ICD message
                hd_size - reserved size (bytes) in front of the output msg

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
int UMC_from_ICD (void *icd_msg, int icd_size, int hd_size, void **c_msg){
    halfword       *spt;
    int code;

    /* If size of ICD message must be at least the size of a message header. */
    if( icd_size < SIZE_OF_PD_MSG_HEADER) {
	LE_send_msg (GL_ERROR, 
			"ORPGUMC: Unknown user message (size %d)", icd_size);
	return UMC_UNKNOWN_TYPE;
    }

    spt = (halfword *) icd_msg;
    code = spt[0];
#ifdef LITTLE_ENDIAN_MACHINE
    code = SHORT_BSWAP (spt[0]);
#endif
    switch (code) {			/* msg_code. */

	case MSG_PROD_REQUEST:		/* prod. request. */
	case MSG_PROD_REQ_CANCEL:
	return Prod_request_from_icd (icd_msg, icd_size, hd_size, c_msg);

	case MSG_MAX_CON_DISABLE:	/* Max connection disable req. */
	return Max_con_disable_from_icd (icd_msg, icd_size, hd_size, c_msg);

        case MSG_EXTERNAL_DATA:	        /* External data message */
        return External_data_from_icd (icd_msg, icd_size, hd_size, c_msg);

	case MSG_ALERT_REQUEST:		/* Alert req. */
	return Alert_request_from_icd (icd_msg, icd_size, hd_size, c_msg);

	case MSG_SIGN_ON:		/* Sign on  msg. */
	return Sign_on_from_icd (icd_msg, icd_size, hd_size, c_msg);

	case MSG_PROD_LIST:		/* product list request msg. */
	return Prod_list_req_from_icd (icd_msg, icd_size, hd_size, c_msg);

        case MSG_ENVIRONMENTAL_DATA:	/* Environmental data message */
        return Environmental_data_from_icd (icd_msg, icd_size, hd_size, c_msg);

	default:
	LE_send_msg (GL_ERROR, 
			"ORPGUMC: Unknown user message (code %d)", code);
	return UMC_UNKNOWN_TYPE;
    }
}

/***************************************************************************

    Description: Converts a product request user message from the ICD format 
		to ORPG C structure format. 

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Prod_request_from_icd 
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    unsigned short *spt;
    void *msg, *buf;
    Pd_request_products *req;
    int length, num_blocks, i;

    spt = (unsigned short *) icd_msg;

    num_blocks = spt[8];
#ifdef LITTLE_ENDIAN_MACHINE
    num_blocks = SHORT_BSWAP (spt[8]);
#endif
    num_blocks--;

    if (icd_size != (MSG_HEADER_LEN + num_blocks * MSG_PROD_REQUEST_LEN) *
	    				((int)sizeof (halfword))) {
	LE_send_msg (GL_ERROR, "UMC: The incoming PROD_REQUEST len: %d expecting %d\n",
		   icd_size, (MSG_HEADER_LEN + num_blocks *
			      MSG_PROD_REQUEST_LEN) *
		   ((int)sizeof (halfword)));
	return UMC_MSG_LEN_ERROR;
    }

    length = ALIGNED_SIZE((int)sizeof (Pd_msg_header)) +
			num_blocks * ((int)sizeof (Pd_request_products));

    buf = (void *)malloc (hd_size + length);
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }
    msg = (void *) ((char *) buf + hd_size);

    Msg_hd_block_from_icd (icd_msg, length, msg);

    MISC_short_swap ((unsigned short *)icd_msg + MSG_HEADER_LEN, 
			(icd_size / sizeof (halfword)) - MSG_HEADER_LEN);

    req = (Pd_request_products *) ((char *) msg +
				 ALIGNED_SIZE((int)sizeof (Pd_msg_header)));
    spt = (unsigned short *)icd_msg + MSG_HEADER_LEN;

    for (i = 0; i < num_blocks; i++) {
	int j;
	int pid;
	unsigned int tm;

	req[i].divider = *spt++;
	req[i].length = *spt++;
	req[i].prod_id = *spt++;
	req[i].flag_bits = *spt++;
	req[i].seq_number = *spt++;
	req[i].num_products = *spt++;
	req[i].req_interval = *spt++;
	req[i].VS_date = *spt++;
	tm = *spt++;
	tm = (tm << 16) | *spt++;
	req[i].VS_start_time = tm;

	pid = ORPGPAT_get_prod_id_from_code (req[i].prod_id);

        /* For all legacy products ..... */
	if( (pid >= 0) && (pid <= LEGACY_MAX_BUFFERNUM) ){

           for (j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++) {
	      int prpc;
	      short pvalue;

	      prpc = Prod_req_param_conv[pid][j];
	      pvalue = *spt++;
	      switch (prpc) {
		case COPY:
		    req[i].params[j] = pvalue;
		    break;
		case TOALG:
		    if (pvalue == -1)
			req[i].params[j] = ALG;
		    else
			req[i].params[j] = pvalue;
		    break;
		case PTOALG:
		    if (req[i].params[j - 1] == ALG)
			req[i].params[j] = ALG;
		    else
			req[i].params[j] = pvalue;
		    break;
		default:
		    req[i].params[j] = prpc;
		    break;
              }

           }

        }
        else{

           /* For all non-legacy products ..... */
           int ind, num_params;

           /* Initialize all product dependent parameters to "unused" */
           for( j = 0; j < NUM_PROD_DEPENDENT_PARAMS; j++ )
              req[i].params[j] = PARAM_UNUSED;

           /* Do For All defined parameters. */
           num_params = ORPGPAT_get_num_parameters( pid );
           for (j = 0; j < num_params; j++){
  
              ind = ORPGPAT_get_parameter_index( pid, j );
              if( ind >= 0 )
                 req[i].params[ind] = *(spt+ind);
  
           }

           /* Increment msg pointer by number of dependent parameters. */
           spt += NUM_PROD_DEPENDENT_PARAMS;

        }

        /* Special request processing for CFC product and ORDA. */
        if( pid == CFCPROD ){

           if( ORPGRDA_get_rda_config( NULL ) == ORPGRDA_ORDA_CONFIG ){

              /* Mask off the channel information. */
              int ind = ORPGPAT_get_parameter_index( pid, 0 );
              if( ind >= 0 )
                 req[i].params[ind] &= 0xfffe;

           }

        } /* End of CLUTPROD special request processing. */

    }

    *c_msg = buf;

    return length;
}

/***************************************************************************

    Description: Processes an alert request user message in the ICD format
		to ORPG C structure format. Because this message will be
		passed to a ported program, we define the C structure format 
		as identical to the ICD format.

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Alert_request_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    unsigned short *spt;
    void *msg, *buf;
    int n_blocks, esize, block_len, i;

    spt = (unsigned short *)icd_msg;
    n_blocks = spt[8];			/* number of blocks (always 2) */
#ifdef LITTLE_ENDIAN_MACHINE
    n_blocks = SHORT_BSWAP (spt[8]);
#endif

    spt += MSG_HEADER_LEN;
    esize = MSG_HEADER_LEN;		/* expected size */
    for (i = 0; i < n_blocks - 1; i++) {
	int b_size, n_cat;

	n_cat = spt[3];			/* number of categories */
#ifdef LITTLE_ENDIAN_MACHINE
	n_cat = SHORT_BSWAP (spt[3]);
#endif
	b_size = 4 + n_cat * MSG_ALERT_CAT_LEN + MSG_ALERT_BOX_LEN;
	esize += b_size;
	spt += b_size;
    }
    if (icd_size != esize * sizeof (halfword)) {
	LE_send_msg (GL_ERROR, 
		"UMC: bad ALERT_REQUEST len (%d, expecting %d)\n",
		  				 icd_size, esize);
	return UMC_MSG_LEN_ERROR;
    }
    spt = (unsigned short *)icd_msg;
    spt += MSG_HEADER_LEN;
    block_len = spt[1];
#ifdef LITTLE_ENDIAN_MACHINE
	block_len = SHORT_BSWAP (spt[1]);
#endif
    if (icd_size != block_len + 18) {
	LE_send_msg (GL_ERROR, 
		"UMC: bad block length (%d, expecting %d)\n",
		  				 block_len, icd_size - 18);
	return UMC_MSG_LEN_ERROR;
    }

    buf = (void *)malloc (hd_size + icd_size);
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }

    msg = (void *) ((char *)buf + hd_size);

    Msg_hd_block_from_icd (icd_msg, icd_size, msg);

    MISC_short_swap ((unsigned short *)icd_msg + MSG_HEADER_LEN, 
			(icd_size / sizeof (halfword)) - MSG_HEADER_LEN);
    memcpy ((short *)msg + MSG_HEADER_LEN, (short *)icd_msg + MSG_HEADER_LEN,
			icd_size - MSG_HEADER_LEN * sizeof (halfword));

    *c_msg = buf;
    return icd_size;
}

/***************************************************************************

    Description: Converts a maximum connection time disable user message 
		from the ICD format to ORPG C structure format. 

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Max_con_disable_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    unsigned short *spt;
    void *buf;
    Pd_max_conn_time_disable_msg *max_conn;

    if (icd_size != MSG_MAX_CON_DISABLE_LEN * ((int)sizeof (halfword))) {
	LE_send_msg (GL_ERROR, "UMC: The incoming MAX_CON len: %d expecting %d\n",
		   icd_size, MSG_MAX_CON_DISABLE_LEN *
		   ((int)sizeof (halfword)));
	return UMC_MSG_LEN_ERROR;
    }

    buf = (void *)malloc (hd_size +
				(int)sizeof (Pd_max_conn_time_disable_msg));
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }

    max_conn = (Pd_max_conn_time_disable_msg *) ((char *) buf + hd_size);
    Msg_hd_block_from_icd (icd_msg, 
		(int)sizeof (Pd_max_conn_time_disable_msg), max_conn);

    MISC_short_swap ((unsigned short *)icd_msg + MSG_HEADER_LEN, 
			(icd_size / sizeof (halfword)) - MSG_HEADER_LEN);

    spt = (unsigned short *)icd_msg;
    max_conn->divider = spt[9];
    max_conn->length = spt[10];
    max_conn->add_conn_time = spt[11];
    max_conn->spare0 = spt[12];
    max_conn->spare1 = spt[13];

    *c_msg = buf;

    return ((int)sizeof (Pd_max_conn_time_disable_msg));
}

/***************************************************************************

    Description: Converts an environmental data message from the ICD format 
		to ORPG C structure format. 

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Environmental_data_from_icd 
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    unsigned short *spt;
    void *msg, *buf;
    int n_blocks = 0, esize = 0, block_len = 0, length = 0, i;

    spt = (unsigned short *)icd_msg;
    n_blocks = spt[8];			/* number of blocks (assume always 2) */
#ifdef LITTLE_ENDIAN_MACHINE
    n_blocks = SHORT_BSWAP (spt[8]);
#endif

    if (n_blocks != 2 ){
       LE_send_msg (GL_ERROR, 
		"UMC: Bad Environmental Data Message (More than 2 Blocks)\n" );
       return UMC_CONVERSION_FAILED;
    }

    spt += MSG_HEADER_LEN;
    esize = MSG_HEADER_LEN;		/* expected size */
    for (i = 0; i < n_blocks - 1; i++) {

       int block_id, b_size = 0;

       block_id = spt[1];
#ifdef LITTLE_ENDIAN_MACHINE
       block_id = SHORT_BSWAP (spt[1]);
#endif
       switch( block_id ){

          case MSG_BIAS_TABLE_BLOCK_ID:
          {

  	     int n_rows;

       	     n_rows = spt[20];
#ifdef LITTLE_ENDIAN_MACHINE
	     n_rows = SHORT_BSWAP (spt[20]);
#endif
	     b_size = MSG_BIAS_TABLE_HDR_LEN + (n_rows*MSG_BIAS_TABLE_ROW_LEN);
	     esize += b_size;
             length = ALIGNED_SIZE((int)sizeof (Pd_msg_header)) + (b_size*sizeof(halfword));

             break;

          }
           
          default:
          {
             LE_send_msg( GL_ERROR, "Unknown Block ID (%d) For Environmental Data\n",
                          block_id );
             break;

          }

      }

      spt += b_size;

        
    }
    if (icd_size != esize * sizeof (halfword)) {
	LE_send_msg (GL_ERROR, 
		"UMC: Bad Environmental Data Len (%d, Expecting %d)\n",
		  			 icd_size, (esize*sizeof(halfword)));
	return UMC_MSG_LEN_ERROR;
    }

    spt = (unsigned short *)icd_msg;
    spt += MSG_HEADER_LEN;
    block_len = spt[3];
#ifdef LITTLE_ENDIAN_MACHINE
	block_len = SHORT_BSWAP (spt[3]);
#endif
    if (icd_size != (block_len + MSG_HEADER_LEN*2)) {
	LE_send_msg (GL_ERROR, 
		"UMC: Bad Block Length (%d, Expecting %d)\n",
		  			 block_len, (icd_size - MSG_HEADER_LEN));
	return UMC_MSG_LEN_ERROR;
    }

    buf = (void *)malloc (hd_size + length);
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }

    msg = (void *) ((char *)buf + hd_size);
    Msg_hd_block_from_icd (icd_msg, length, msg);

    MISC_short_swap ((unsigned short *)icd_msg + MSG_HEADER_LEN, 
			(icd_size / sizeof (halfword)) - MSG_HEADER_LEN);
    memcpy ((short *)msg + (ALIGNED_SIZE((int)sizeof (Pd_msg_header))/sizeof(short)),
            (short *)icd_msg + MSG_HEADER_LEN, 
            icd_size - MSG_HEADER_LEN * sizeof (halfword));

    *c_msg = buf;
    return length;

}

/***************************************************************************

    Description: Converts an external data message from the ICD format 
		 to ORPG C structure format. 

    Inputs:	 icd_msg - pointer to the ICD message
		 icd_size - size of the ICD message (in bytes)
		 hd_size - reserved space (bytes) in front of the output msg

    Output:	 c_msg - pointer to the area for the output message. The 
		 ORPG C structure message starts at c_msg + hd_size.

    Return:	 The size of the output data in C struct format (excluding 
		 the hd_size) on success or a negative error code (described 
		 in umc manpages).

***************************************************************************/
static int External_data_from_icd (void *icd_msg, int icd_size, int hd_size,
   void **c_msg){

    void           *buf     = NULL; /* output buffer to hold entire adjusted and
                                       aligned output message plus extra hdr */
    void           *msg     = NULL;
    int            length   = 0;    /* aligned, adjusted length to return */
    int            block_id = 0;    /* msg block identifier */
    unsigned short *spt     = NULL; /* short pointer to icd msg */

    /* cast short pointer to incoming icd message and increment past msg hdr */
    spt = (unsigned short *)icd_msg;
    spt += MSG_HEADER_LEN;

    block_id = spt[1];
#ifdef LITTLE_ENDIAN_MACHINE
    block_id = SHORT_BSWAP (spt[1]);
#endif

    /* determine the block id and take the appropriate action */
    switch( block_id ){

       case RUC_MODEL_DATA_BLOCK_ID:
       {
          /* The exact format of the RUC Model Data External Data message is TBD.
             For now we just need to allocate sufficient space to hold the msg.
             NOTE: the line index is added to the end of the msg header.  We 
             allocate space for it here (via sizeof Pd_msg_header). */

          length = ALIGNED_SIZE((int)sizeof (Pd_msg_header)) +
             (icd_size - (MSG_HEADER_LEN * sizeof(short)));

          /* allocate buffer to hold entire aligned message in orpg format */
          buf = (void *) malloc (hd_size + length);
          if (buf == NULL) {
	      return UMC_MALLOC_FAILED;
          }

          /* cast msg ptr so we can byte swap the msg if necessary */
          msg = (void *) ((char *) buf + hd_size);

          /* convert the message header. note - i don't believe we need to
             convert the rest of the message because it will be taken care of
             in the XDR decoding functions */
          Msg_hd_block_from_icd (icd_msg, length, msg);

          /* copy icd msg to buffer */
          memcpy ((short *)msg +
             (ALIGNED_SIZE((int)sizeof (Pd_msg_header))/sizeof(short)),
             (short *)icd_msg + MSG_HEADER_LEN, icd_size -
             (MSG_HEADER_LEN * sizeof(halfword)));

          /* point the input buffer to the buffer we created */
          *c_msg = buf;

          break;
       }
           
       default:
       {
          LE_send_msg( GL_ERROR, "Unknown Block ID (%d) For External Data\n",
             block_id );
          break;
       }
    }

    return length;

} /* end External_data_from_icd() */


/***************************************************************************

    Description: Converts a sign-on user message from the ICD format to 
		ORPG C structure format. 

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Sign_on_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    void *buf;
    unsigned short *spt;
    Pd_sign_on_msg *sign;
    char *cpt;

    if (icd_size != ((int)sizeof (halfword)) * MSG_SIGN_ON_LEN) {
	LE_send_msg (GL_ERROR, "UMC: The incoming SIGN_ON len: %d expecting %d\n",
		   icd_size, ((int)sizeof (halfword)) * MSG_SIGN_ON_LEN);
	return UMC_MSG_LEN_ERROR;
    }

    buf = (void *)malloc (hd_size + sizeof (Pd_sign_on_msg));
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }
    sign = (Pd_sign_on_msg *)((char *)buf + hd_size);

    Msg_hd_block_from_icd (icd_msg, sizeof (Pd_sign_on_msg), sign);

    spt = (unsigned short *)icd_msg;
#ifdef LITTLE_ENDIAN_MACHINE
    sign->divider = SHORT_BSWAP (spt[9]);
    sign->length = SHORT_BSWAP (spt[10]);
    sign->disconn_override_flag = SHORT_BSWAP (spt[16]);
    sign->spare = SHORT_BSWAP (spt[17]);
#else
    sign->divider = spt[9];
    sign->length = spt[10];
    sign->disconn_override_flag = spt[16];
    sign->spare = spt[17];
#endif

    memcpy(sign->user_passwd, spt + 11, 6);
    sign->user_passwd[6] = '\0';
    cpt = sign->user_passwd + 5;	/* removing trailing spaces */
    while (*cpt == ' ' && cpt >= sign->user_passwd) {
	*cpt = '\0';
	cpt--;
    }
    memcpy(sign->port_passwd, spt + 14, 4);
    sign->port_passwd[4] = '\0';
    cpt = sign->port_passwd + 3;	/* removing trailing spaces */
    while (*cpt == ' ' && cpt >= sign->port_passwd) {
	*cpt = '\0';
	cpt--;
    }

    *c_msg = buf;

    return sizeof (Pd_sign_on_msg);
}

/***************************************************************************

    Description: Converts a product list request user message 
		from the ICD format to ORPG C structure format. 

    Inputs:	icd_msg - pointer to the ICD message.
		icd_size - size of the ICD message.
		hd_size - reserved space size in front of the output message.

    Output:	c_msg - pointer to the area for the output message. The 
			ORPG C structure message starts at c_msg + hd_size.

    Return:	The size of the output data in C struct format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Prod_list_req_from_icd
		(void *icd_msg, int icd_size, int hd_size, void **c_msg){

    void *buf;

    if (icd_size != MSG_HEADER_LEN * ((int)sizeof (halfword))) {
	LE_send_msg (GL_ERROR, 
		"UMC: The incoming prod_list_req len: %d expecting %d\n",
		   icd_size, MSG_HEADER_LEN * ((int)sizeof (halfword)));
	return UMC_MSG_LEN_ERROR;
    }

    buf = (void *)malloc (hd_size + sizeof (Pd_msg_header));
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }

    Msg_hd_block_from_icd (icd_msg, 
		sizeof (Pd_msg_header), (char *)buf + hd_size);

    *c_msg = buf;

    return (sizeof (Pd_msg_header));
}

/***************************************************************************

    Description: Converts the message header block from ICD.

    Inputs:	icd_mhb - pointer to the ICD message header block.
		len - length, in number of bytes, of the C message.

    Output:	c_msg - pointer to Pd_msg_header.

***************************************************************************/
static void Msg_hd_block_from_icd (halfword *icd_mhb, int len, void *c_msg){

    Pd_prod_header *hd;
    time_t utime;

    MISC_short_swap (icd_mhb, MSG_HEADER_LEN);

    hd = (Pd_prod_header *)c_msg;

    hd->msg_code = icd_mhb[0];
    hd->date = icd_mhb[1];
    utime = UNIX_SECONDS_FROM_RPG_DATE_TIME 
	((int)icd_mhb[1], (int)(icd_mhb[3] | ((int)icd_mhb[2] << 16)) * 1000);
    ORPGMISC_pack_ushorts_with_value( &hd->timem, &utime );  
    ORPGMISC_pack_ushorts_with_value( &hd->lengthm, &len );
    hd->src_id = icd_mhb[6];
    hd->dest_id = icd_mhb[7];
    hd->n_blocks = icd_mhb[8];

    return;
}

/***************************************************************************

    Description: Converts a product user message from the ORPG C structure 
		format to the ICD format. Memory segment for the output 
		message is allocated here. It must be freed by the caller. 
		The caller can specify a reserved space in front of the 
		output message.

    Inputs:	c_msg - pointer to the ORPG C structure message.
		c_size - size of the ORPG C structure message.
		hd_size - reserved space size in front of the output message.

    Output:	icd_msg - pointer to the area for the output message. The 
			ICD message starts at icd_msg + hd_size.

    Return:	The size of the output data in ICD format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
int UMC_to_ICD (void *c_msg, int c_size, int hd_size, void **icd_msg){

    halfword       *icd;
    int             msg_size;
    Pd_msg_header   hdr;

    if (c_size < (int)sizeof (Pd_msg_header)) {
	return UMC_MSG_LEN_ERROR;
    }
    memcpy(&hdr, c_msg, (int)sizeof (Pd_msg_header));

    switch (hdr.msg_code) {

    case MSG_GEN_STATUS:
	if (c_size < (int)sizeof (Pd_general_status_msg)) {
	    LE_send_msg (GL_ERROR, "UMC: Incoming len: %d expecting %d\n",
		   c_size, (int)sizeof (Pd_general_status_msg));
	    return UMC_MSG_LEN_ERROR;
	}
	icd = (halfword *)malloc (hd_size +
				MSG_GEN_STATUS_LEN * sizeof (halfword));
	if (icd == NULL) {
	    return UMC_MALLOC_FAILED;
	}
	msg_size = Gen_status_to_icd (c_msg,
				(halfword *) ((char *) icd + hd_size));
	*icd_msg = icd;
	return msg_size;
	break;

    case MSG_REQ_RESPONSE:
	if (c_size <
	    (int)sizeof (Pd_request_response_msg)) {
	    LE_send_msg (GL_ERROR, "UMC: Incoming len: %d expecting %d\n",
		   c_size, (int)sizeof (Pd_request_response_msg));
	    return UMC_MSG_LEN_ERROR;
	}
	icd = (halfword *)malloc (hd_size +
				MSG_REQ_RESPONSE_LEN * sizeof (halfword));
	if (icd == NULL) {
	    return UMC_MALLOC_FAILED;
	}
	msg_size = Request_response_to_icd (c_msg, 
				(halfword *) ((char *) icd + hd_size));
	*icd_msg = icd;
	return msg_size;

    case MSG_ALERT_PARAMETER:
	return Alert_params_to_icd (c_msg, c_size, hd_size, icd_msg);
	break;

    case MSG_PROD_LIST:
	return Product_list_to_icd (c_msg, c_size, hd_size, icd_msg);
	break;

    default:
	if ((131 >= hdr.msg_code &&
	     hdr.msg_code >= 16) ||
	    (-16 >= hdr.msg_code &&
	     hdr.msg_code >= -131) ||
	    hdr.msg_code == 9) {

	    LE_send_msg (GL_ERROR, "UMC: No conversion is needed for products\n");
	} 
        return UMC_UNKNOWN_TYPE;
    }
}

/***************************************************************************

    Description: Converts a general status message from the ORPG C structure 
		format to the ICD format. 

    Inputs:	c_msg - pointer to the ORPG C structure message.

    Output:	icd_msg - pointer to the ICD message. 

    Return:	The size of the output data in ICD format on success or a 
		negative error code (described in umc manpages).

***************************************************************************/
static int Gen_status_to_icd (void *c_msg, halfword *icd_msg){

    Pd_general_status_msg *msg;
    unsigned short *spt;
    int nitems;

    spt = (unsigned short *)icd_msg;
    spt += Msg_hd_block_to_icd (c_msg, MSG_GEN_STATUS_LEN, icd_msg);

    msg = (Pd_general_status_msg *)c_msg;
    nitems = MSG_GEN_STATUS_LEN - MSG_HEADER_LEN;
    memcpy ((char *)spt, (char *)&(msg->divider), nitems * sizeof (halfword));

    MISC_short_swap (spt, nitems);

    return MSG_GEN_STATUS_LEN * ((int)sizeof (halfword));
}

/***************************************************************************

    Description: Converts a product list message from the ORPG C structure 
		format to the ICD format. Memory segment for the output 
		message is allocated here. It must be freed by the caller. 
		The caller can specify a reserved space in front of the 
		output message.

    Inputs:	c_msg - pointer to the ORPG C structure message.
		c_size - size of the ORPG C structure message.
		hd_size - reserved space size in front of the output message.

    Output:	icd_msg - pointer to the area for the output message. The 
			ICD message starts at icd_msg + hd_size.

    Return:	The size of the output data in ICD format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Product_list_to_icd
		(void *c_msg, int c_size, int hd_size, void **icd_msg){

    int             i, length, n_prods;
    Pd_prod_list_entry *entry;
    Pd_prod_list_msg *list;
    void           *buf;
    halfword       *icd;
    unsigned short *spt;

    list = (Pd_prod_list_msg *)c_msg;
    n_prods = list->num_products;
    if (c_size != (int)sizeof (Pd_prod_list_msg) +
			n_prods * sizeof (Pd_prod_list_entry)) {
	return UMC_MSG_LEN_ERROR;
    }
    length = MSG_PROD_LIST_LEN + n_prods * PROD_LIST_ENTRY_LEN;
    buf = (halfword *)malloc (hd_size + length * sizeof (halfword));
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }
    icd = (halfword *) ((char *)buf + hd_size);
    icd += Msg_hd_block_to_icd (c_msg, length, icd);
    spt = (unsigned short *)icd;

    *icd++ = -1;
    *icd++ = list->length;
    *icd++ = n_prods;
    *icd++ = list->distribution_method;

    entry = (Pd_prod_list_entry *) ((char *) c_msg +
			ALIGNED_SIZE (sizeof (Pd_prod_list_msg)));

    for( i = 0; i < n_prods; i++ ){

        int pcode, k, m;

	pcode = ORPGPAT_get_code ((int)entry[i].prod_id);
	*icd++ = pcode;
	
	if( pcode > 0 ){

	    if( pcode < MAX_LIST_PROD_CODE ){

                for( k = 0; k < 5; k++ ){

	            if( List_params_index [pcode][k] >= 0){

		        *icd = entry[i].params[List_params_index [pcode][k]];
                        if( (entry[i].prod_id == SRMRVMAP)
                                           ||
                            (entry[i].prod_id == SRMRVREG) ){

                            if( *icd == PARAM_ALG_SET )
                                *icd = -1;

                        }

                    }
                    else
                        *icd = 0;

	            icd++;

                }

            }
	    else{

                int el_ind = ORPGPAT_elevation_based( (int) entry[i].prod_id );

                /* For product codes > MAX_LIST_PROD_CODE, check if the 
                   product is elevation-based and has an elevation parameter.
                   If yes, get this parameter and store at index k = 0. */
                if( el_ind >= 0 )
                    *icd = entry[i].params[el_ind];

                else
                    *icd = 0;

                icd++;

                /* Store the remaining product dependent parameters.  Don't store 
                   the elevation slice more than once. */
                m = 0;
                for( k = 0; k < 4; k++ ){

                    if( m == el_ind )
                        m++;

                    *icd = entry[i].params[m];
	            icd++;
                    m++;

                }

	    }

        }

	*icd++ = entry[i].distribution_class;
    }

    MISC_short_swap (spt, 4 + PROD_LIST_ENTRY_LEN * n_prods);

    *icd_msg = buf;

    return (length * sizeof (halfword));
}

/***************************************************************************

    Description: Converts an alert adaptation parameter message from 
		the ORPG C structure format to the ICD format. Memory 
		segment for the output 
		message is allocated here. It must be freed by the caller. 
		The caller can specify a reserved space in front of the 
		output message.

    Inputs:	c_msg - pointer to the ORPG C structure message.
		c_size - size of the ORPG C structure message.
		hd_size - reserved space size in front of the output message.

    Output:	icd_msg - pointer to the area for the output message. The 
			ICD message starts at icd_msg + hd_size.

    Return:	The size of the output data in ICD format (excluding 
		the hd_size) on success or a negative error code (described 
		in umc manpages).

***************************************************************************/
static int Alert_params_to_icd
		(void *c_msg, int c_size, int hd_size, void **icd_msg){

    Pd_alert_adaptation_parameter_msg *aap;
    Pd_alert_adaptation_parameter_entry *entry;
    void           *buf;
    halfword       *icd;
    int n_cat, icd_size, i, cnt;

    aap = (Pd_alert_adaptation_parameter_msg *)c_msg;
    entry = (Pd_alert_adaptation_parameter_entry *)
		((char *)aap + 
		ALIGNED_SIZE (sizeof (Pd_alert_adaptation_parameter_msg)));
    n_cat = (aap->length - 4) / 
		sizeof (Pd_alert_adaptation_parameter_entry);
    if (c_size != n_cat * sizeof (Pd_alert_adaptation_parameter_entry) + 
		ALIGNED_SIZE (sizeof (Pd_alert_adaptation_parameter_msg)) ||
	n_cat < 1) {
	LE_send_msg (GL_ERROR, "UMC: bad alert params msg\n");
	return UMC_MSG_LEN_ERROR;
    }
    icd_size = MSG_HEADER_LEN + 2 + n_cat * 10;

    buf = (halfword *)malloc (hd_size + icd_size * sizeof (halfword));
    if (buf == NULL) {
	return UMC_MALLOC_FAILED;
    }
    icd = (halfword *) ((char *)buf + hd_size);
    icd += Msg_hd_block_to_icd (&(aap->mhb), icd_size, icd);
    
    memcpy (icd, &(aap->divider), 2 * sizeof (halfword));
    cnt = 2;
    for (i = 0; i < n_cat; i++) {
	memcpy (icd + cnt, entry, 10 * sizeof (halfword));
	entry++;
	cnt += 10;
    }

    MISC_short_swap (icd, icd_size - MSG_HEADER_LEN);

    *icd_msg = buf;

    return (icd_size * sizeof (halfword));
}

/***************************************************************************

    Description: Converts a request response message from the ORPG C structure 
		format to the ICD format. 

    Inputs:	c_msg - pointer to the ORPG C structure message.

    Output:	icd_msg - pointer to the ICD message.

    Return:	The size of the output data in ICD format on success or a 
		negative error code (described in umc manpages).

***************************************************************************/
static int Request_response_to_icd (void *c_msg, halfword *icd_msg){

    Pd_request_response_msg *msg;
    int err_code, nitems;
    unsigned short *spt;

    spt = (unsigned short *)icd_msg;
    spt += Msg_hd_block_to_icd (c_msg, MSG_REQ_RESPONSE_LEN, icd_msg);
    msg = (Pd_request_response_msg *)c_msg;

    spt[0] = msg->divider;
    spt[1] = msg->length;

    err_code = msg->error_code;
    spt[2] = (err_code >> 16) & 0xffff;
    spt[3] = err_code & 0xffff;

    nitems = MSG_REQ_RESPONSE_LEN - MSG_HEADER_LEN;
    memcpy ((char *)(spt + 4), (char *)&(msg->seq_number), 
				(nitems - 4) * sizeof (halfword));

    MISC_short_swap (spt, nitems);

    return MSG_REQ_RESPONSE_LEN * ((int)sizeof (halfword));
}

/***************************************************************************

    Description: Returns the product length.

    Inputs:	prod - pointer to the legacy product data.

    Return:	The product length.

***************************************************************************/
int UMC_product_length (void *prod){

    unsigned short *spt;
    unsigned int len;

    spt = (unsigned short *) prod;
    spt = spt + 4;

    len = (spt[0] << 16) | (spt[1] & 0xffff);

    return ( (int) len );

}

/***************************************************************************

    Description: Returns the product length.

    Inputs:	phd - pointer to the product header block in local endian
		format.

    Return:	The product length.

***************************************************************************/
int UMC_get_product_length (void *phd){

    unsigned short *spt;
    int code;

    spt = (unsigned short *)phd;
    code = spt[0];
    if (code < 0 || code >= 16 || code == 9)
        return ((0xffff0000 & (spt[4] << 16)) | (spt[5] & 0xffff));
    else {
        Pd_msg_header *hd;

        hd = (Pd_msg_header *)phd;
        return (hd->length);
    }

}

/***************************************************************************

    Description: This function performs the byte swap for the message header 
		block. It can be used for converting to and from the
		ICD format.

    Inputs:	prod - pointer to the message header block.

***************************************************************************/
void UMC_message_header_block_swap (void *mhb){

#ifdef LITTLE_ENDIAN_MACHINE
    MISC_short_swap ((short *)mhb, MSG_HEADER_LEN);
#endif

    return;
}

/***************************************************************************

    Description: This function performs the byte swap for the message header 
		block and product description block. It can be used for 
                converting to and from the ICD format.

    Inputs:	prod - pointer to the message header block.

***************************************************************************/
void UMC_msg_hdr_desc_blk_swap (void *mhb){

#ifdef LITTLE_ENDIAN_MACHINE
    MISC_short_swap ((short *)mhb, MSG_PRODUCT_LEN );
#endif

    return;
}

/***************************************************************************

    Description: This is the same as UMC_product_from_icd.

    Inputs:	pmsg - pointer to the product message.
		size - size of the message.

    Output:	pmsg - the converted message.

    Return:	The message size on success or a negative error code 
		(described in umc manpages).

***************************************************************************/
int UMC_icd_to_product (void *pmsg, int size){

    return (UMC_product_from_icd (pmsg, size));
}

/***************************************************************************

    Description: Converts a big endian and Concurrent float ICD product to
		local endian and IEEE float format. Byte swaps are performed 
		as short array for little endian machines. The conversion
		is performed in-place. 


    Inputs:	pmsg - pointer to the product message.
		size - size of the message, in bytes.

    Output:	pmsg - the converted message.

    Return:	The message size on success or a negative error code 
		(described in umc manpages).

***************************************************************************/
int UMC_product_from_icd (void *pmsg, int size){

    halfword *msg, message_code;
    int pcode, ll, lm, length;

    msg = (halfword *)pmsg;

    /* Extract message (product) code */
    message_code = msg[MESCDOFF];
    ll = msg[LGLSWOFF];
    lm = msg[LGMSWOFF];
#ifdef LITTLE_ENDIAN_MACHINE
    message_code = SHORT_BSWAP (msg[MESCDOFF]);
    ll = SHORT_BSWAP (msg[LGLSWOFF]);
    lm = SHORT_BSWAP (msg[LGMSWOFF]);
#endif
    pcode = (int)message_code;
    length = (lm << 16) | ll;

    if (length != size) {
	LE_send_msg (GL_ERROR, "UMC: Incoming product len: %d expecting %d\n",
		   				size, length);
	return UMC_MSG_LEN_ERROR;
    }

#ifdef LITTLE_ENDIAN_MACHINE
    /* Swap the bytes within the message. */
    {

	MISC_short_swap (msg, ((size + 1)/ sizeof(short)));		/* swap all as shorts */

        /* Special processing for each product. */
        if (pcode == MSG_CODE_USER_ALERT ||
            pcode == MSG_CODE_STORM_STRUCTURE ||
            pcode == MSG_CODE_FREE_TEXT || 
            pcode == MSG_CODE_SUPP_PRECIP_DATA)
	    Process_stand_alone_tabular_message (msg, ((size + 1)/ sizeof(short)), 0);

        /* RCM product requires special processing. */
        else if ( pcode == MSG_CODE_RCM )
            Process_rcm_tabular_product_message (msg);

        /* weather alert product. */
        else if (pcode == MSG_CODE_ALERT){

            Process_wx_alert_message (msg);
            return size;

        }
        else
	    Process_graphic_product_message (msg, size, pcode, 0);

    }
#endif

    return size;
}

/***************************************************************************

    Description: Converts a product from the local endian and IEEE float ICD 
		format to big endian and Concurrent float ICD product format. 
		Byte swaps are performed as short array for little endian 
		machines. The conversion is performed in-place. 

    Inputs:	pmsg - pointer to the product message.
		size - size of the message, in shorts.

    Output:	pmsg - the converted message.

    Return:	The message size on success or a negative error code 
		(described in umc manpages).

***************************************************************************/
int UMC_product_back_to_icd (void *pmsg, int size){

    halfword *msg;
    int pcode;

    msg = (halfword *)pmsg;

    /* Extract message (product) code */
    pcode = (int)msg[MESCDOFF];

    /* Special handling required for pre-edit version of RCM product. */

#ifdef LITTLE_ENDIAN_MACHINE
    /* Swap the bytes within the message. */
    {
        /* Special processing for each product. */
        if (pcode == MSG_CODE_USER_ALERT ||
            pcode == MSG_CODE_STORM_STRUCTURE ||
            pcode == MSG_CODE_FREE_TEXT || 
            pcode == MSG_CODE_SUPP_PRECIP_DATA)
	    Process_stand_alone_tabular_message (msg, ((size+1)/sizeof(short)), 1);

        /* RCM product requires special processing. */
        else if ( pcode == MSG_CODE_RCM )
            Process_rcm_tabular_product_message (msg); 

        /* weather alert product. */
        else if (pcode == MSG_CODE_ALERT){

            Process_wx_alert_message (msg);
            return size;

        }
        else
	    Process_graphic_product_message (msg, size, pcode, 1);

	MISC_short_swap (msg, ((size+1)/sizeof(short)));	/* swap all as shorts */
    }
#endif

    return size;
}
/***************************************************************************

    Description: Converts a ORPG product message as generate by legacy
		programs to the ICD format. Field and byte swaps are 
		performed for little endian machines. Field swaping depends
		on particular produts. The conversion is performed in-place.

    Inputs:	pmsg - pointer to the product message.
		size - size of the message, in shorts.

    Output:	pmsg - the converted message.

    Return:	The message size on success or a negative error code 
		(described in umc manpages).

***************************************************************************/
int UMC_product_to_icd (void *pmsg, int size){

    halfword *msg;
    int pcode, pid;

    msg = (halfword *)pmsg;
    pcode = (int)msg[MESCDOFF];

    /* weather alert (alert message) product (this product does not have a product
       description block) */
    if (pcode == MSG_CODE_ALERT){
 
#ifdef LITTLE_ENDIAN_MACHINE
        Process_wx_alert_message (msg);
        MISC_short_swap (msg, ((size+1)/sizeof(short)));	/* swap all as shorts */
#endif
        return size;

    }

    pid = ORPGPAT_get_prod_id_from_code (pcode);

#ifdef LITTLE_ENDIAN_MACHINE
    /* It is more convenient to first adjust the halfword elements and then
	do the byte swap for each halfword */
    {
	if (pid == ALRTPROD || pid == STRUCDAT || pid == FTXTMSG || 
            pid == HYSUPPLE)
	    Process_stand_alone_tabular_message (msg, ((size+1)/sizeof(short)), 1);

        /* RCM product requires special processing. */
        else if ( pcode == MSG_CODE_RCM )
            Process_rcm_tabular_product_message (msg);

        else
	    Process_graphic_product_message (msg, size, pcode, 1);
      
	MISC_short_swap (msg, ((size+1)/sizeof(short)));	/* swap all as shorts */
        
    }
#endif

    return size;
}

#ifdef LITTLE_ENDIAN_MACHINE

/***************************************************************************

    Description: Processes the Graphic Product Message.

    Inputs:	msg - pointer to the message.
                size - size of the message, in bytes.
                pcode - product code.

***************************************************************************/
static void Process_graphic_product_message (halfword *msg, 
                                             int size, int pcode, int to_icd){

    int i, ret;
    int sym_off, gra_off, tab_off, block_id;

    sym_off = msg[OPRMSWOFF] << 16 | (msg[OPRLSWOFF] & 0xffff);
    gra_off = msg[OGMSWOFF] << 16 | (msg[OGLSWOFF] & 0xffff);
    tab_off = msg[OTADMSWOFF] << 16 | (msg[OTADLSWOFF] & 0xffff);

    if (sym_off > 0) {		/* product symbology block */
	unsigned short *sym, *layer;

        /* Short pointer to start of symbology block. */
	sym = (unsigned short *)msg + sym_off;
        
        /* Check block ID to insure this is really the symbology
           block. */
        if( (block_id = sym[1]) == 1 ){

	   layer = sym + 5;

	   for (i = 0; i < sym[4]; i++) {	/* for each layer */

	      int len;

	      len = (layer[1] << 16) | (layer[2] & 0xffff);

              /* Check product data against ICD. */
              if( to_icd )
                 Validate_display_data_packets (layer + 3, len, pcode );

              /* Perform necessary byte-swapping. */
	      if( Process_display_data_packets (layer + 3, len, pcode) < 0 )
                 Dump_product_data( msg, size );

              /*Check product data against ICD. */
              if( !to_icd )
                 Validate_display_data_packets (layer + 3, len, pcode );

	      layer += 3 + ((len + 1) / 2);

           }
	}
        else{

           /* Validate the block id. */
           if( (ret = Validate_block_id( block_id )) == 0 )
              Dump_product_data( msg, size );

        }

    }

    if (gra_off > 0) {			/* graphic alphanumeric block */
	unsigned short *gra, *page;

        /* Short pointer to start of graphic alphanumeric block. */
	gra = (unsigned short *)msg + gra_off;

        /* Check block ID to insure this is really the graphic
           alphanumeric block. */
        if( (block_id = gra[1]) == 2 ){

	   page = gra + 5;
	   for (i = 0; i < gra[4]; i++) {	/* for each page */
	      int len;

	      len = page[1];

              /* Check product data against ICD. */
              if( to_icd )
                 Validate_display_data_packets (page + 2, len, pcode );
 
              /* Perform necessary byte-swapping. */
	      if( Process_display_data_packets (page + 2, len, pcode) < 0 )
                 Dump_product_data( msg, size );

              /* Check product data against ICD. */
              if( !to_icd )
                 Validate_display_data_packets (page + 2, len, pcode );

	      page += 2 + (len / 2);

           }
	}
        else{

           /* Validate the block id. */
           if( (ret = Validate_block_id( block_id )) == 0 )
              Dump_product_data( msg, size );

        }
    }

    if (tab_off > 0) {			/* tabular alphanumeric block */
	unsigned short *tab, *cpt;

        /* Short pointer to start of tabular alphanumeric block. */
	tab = (unsigned short *)msg + tab_off;

        /* Check block ID to insure this is really the tabular
           alphanumeric block. */
        if( (block_id = tab[1]) == 3 ){

#ifdef DEBUG_TAB
            Tabular_alpha_block *alp = (Tabular_alpha_block *) &tab[0];
            char str[120];
            int cnt = 0;

            LE_send_msg( GL_INFO, "Tabular Block .....\n" );
            LE_send_msg( GL_INFO, "--->tab->divider:         %d\n", alp->divider );
            LE_send_msg( GL_INFO, "--->tab->block_id:        %d\n", alp->block_id );
            LE_send_msg( GL_INFO, "--->tab->block_len:       %d\n", alp->block_len );

            /* Clear the string. */
            memset( str, 0, 120 );
#endif
           /* set the pointer to the start of the tabular data. */
	   tab += PHEADLNG + 4;
	   cpt = tab + 2;

#ifdef DEBUG_TAB
           LE_send_msg( GL_INFO, "--->Process each page\n" );
#endif
	   for (i = 0; i < tab[1]; i++) {	/* for each page */
	      int len, k;

	      while (1) {

	   	 if (cpt[0] == 0xffff) {
#ifdef DEBUG_TAB
                    LE_send_msg( GL_INFO, "--->cpt[0] == 0xffff: %d\n", cpt[0] );
#endif
		    cpt++;
		    break;
		 }

	 	 len = (cpt[0] + 1) / 2;
#ifdef DEBUG_TAB
                 LE_send_msg( GL_INFO, "Length of Line (shorts): %d\n", len );
                 if( len == 0 )
                    exit(0);
#endif
	  	 cpt++;
		 for (k = 0; k < len; k++) {
#ifdef DEBUG_TAB
                    memcpy( &str[cnt], cpt, sizeof(short) );
                    cnt += sizeof(short);
#endif
		    *cpt = SHORT_BSWAP(*cpt);
		    cpt++;
		 }

#ifdef DEBUG_TAB
                 LE_send_msg( GL_INFO, "Done Swapping Line\n" );
                 str[cnt] = '\0';
                 LE_send_msg( GL_INFO, "Line: %s\n", str );
                 cnt = 0;
                 memset(str,0,120);
#endif

              }
           }
	}
        else{

           /* Validate the block id. */
           if( (ret = Validate_block_id( block_id )) == 0 )
              Dump_product_data( msg, size );

        }
    }
    return;
}

/***********************************************************************

   Description:
      Validates the block id.

   Inputs:
      block_id - Block ID to validate.

   Returns:
      0 - if block ID is invalid.
      1 - if block ID is valid.
   
***********************************************************************/
static int Validate_block_id( int block_id ){

   /* Currently the only validate block IDs are 1, 2, and 3. */
   if( block_id < 1 || block_id > 3 ) {

      LE_send_msg( GL_ERROR, "Block ID Not Valid: %d\n", block_id );
      return 0;

   }
   else
      return 1;

}

/***************************************************************************

    Description: Processes the weather alert message.

    Inputs:     msg - pointer to the message.

***************************************************************************/
static void Process_wx_alert_message (halfword *msg){

    short *spt;

    spt = (short *)msg;

    spt = msg + ALERT_CELL_ID_OFF;
    *spt = SHORT_BSWAP (*spt);

    return;
}

/***************************************************************************

    Description: Processes the RCM Tabular Product Message.

    Inputs:     msg - pointer to the message.

***************************************************************************/

static void Process_rcm_tabular_product_message (halfword *msg)
{
    int sym_off, len;

    len = UMC_get_product_length (msg);

    sym_off = msg[OPRMSWOFF] << 16 | (msg[OPRLSWOFF] & 0xffff);

    if (sym_off > 0) {                  /* tabular alphanumeric block */
        unsigned short *cpt;
        int k;

        cpt = (unsigned short *)msg + sym_off;

        /* Calculate the length, in shorts, of the rcm tabular data. */
        len = len / 2 - sym_off;

        /* Byte-swap the tabular data. */
        for (k = 0; k < len; k++) {
           *cpt = SHORT_BSWAP(*cpt);
           cpt++;
        }

    }
    return;
}

/***************************************************************************

    Description: Processes the Stand Alone Tabular Alphanumeric Product 
		Message.

    Inputs:	msg - pointer to the message.
                size - size of the message, in shorts
                to_icd - 1 = internal to ICD, 0 = ICD to internal

***************************************************************************/
static void Process_stand_alone_tabular_message (halfword *msg, int size,
                                                 int to_icd){

    int i, pcode;
    unsigned short *cpt, *tab;

    tab = (unsigned short *)msg + PHEADLNG;
    cpt = tab + 2;

    for (i = 0; i < tab[1]; i++) {	/* for each page */
	int len, k;

	while (1) {
	    if (cpt[0] == 0xffff) {
		cpt++;
		break;
	    }
	    len = (cpt[0] + 1) / 2;
	    cpt++;
	    for (k = 0; k < len; k++) {
		*cpt = SHORT_BSWAP(*cpt);
		cpt++;
	    }
	}
    }

    /* Special processing required for product 62.  The trend data
       immediately follows the tabular data. */
    pcode = msg[MESCDOFF];
    if( pcode == MSG_CODE_STORM_STRUCTURE ){

       int shorts_remaining; 

       /* Calculate the number of bytes remaining in the product.
          The remaining part of product is the trend data. */
       shorts_remaining = size - (int) (cpt - (unsigned short *)msg);

       /* Check product data against ICD. */
       if( to_icd )
          Validate_display_data_packets (cpt, shorts_remaining*2, pcode );

       /* Perform necessary byte-swapping. */
       if( Process_display_data_packets (cpt, shorts_remaining*2, pcode) < 0 )
          Dump_product_data( msg, size ); 

       /* Check product data against ICD. */
       if( !to_icd )
          Validate_display_data_packets (cpt, shorts_remaining*2, pcode );

    } 

    return;
}

/* Macro definitions for specific packet types. */
/* Packet code 27 - Superob */
#define SUPEROB_CELL_SIZE_BYTES    18

/* Used for multiple packets. */
#define MIN_PIXEL			-2048
#define MAX_PIXEL			 2048

/* Used in Write Text Packet (Uniform Value - code 8). */
#define MIN_COLOR			    0
#define MAX_COLOR			   15

/* Used in AF1F Packet (Radial RLE) */
#define MAX_FIRST_RNG_BIN		  460
#define MAX_NUM_RNG_BINS		  460
#define MIN_SCALE_FACTOR		    1
#define MAX_SCALE_FACTOR		 8000
#define MAX_NUM_RADIALS			  400
#define MIN_NUM_RLE_HW			    1
#define MAX_NUM_RLE_HW			  230

/* Used in Digital Data Array Packet (code 16) */
#define MAX_DIG_FIRST_RNG_BIN		  230
#define MAX_DIG_NUM_RNG_BINS		 1840
#define MIN_DIG_SCALE_FACTOR		    1
#define MAX_DIG_SCALE_FACTOR		 1000
#define MAX_DIG_NUM_RADIALS		  720

/* Used in multiple packets. */
#define MAX_RAD_START_ANG		 3599
#define MAX_RAD_DELTA		 	   20

/* Used in Hail Packet. */
#define HAIL_FLAG			 -999
#define MAX_HAIL_SIZE                       4
#define MAX_HAIL_PROB                     100

/* Used in STI Radius Packet. */
#define MAX_STI_RADIUS			  512

/* Used in the Special Symbol Packet. */
#define MAX_FEATURE_TYPE		   11

/* Used in Vector Arrow Packet. */
#define MAX_ARROW_DIR			  359
#define MAX_ARROW_LEN			  512
#define MAX_ARROW_HEAD_LEN		  512

/* Used in Wind Barb Packet. */
#define MAX_WIND_DIR			  359
#define MAX_WIND_SPEED			  195
#define MAX_WIND_BARB_COLOR		    5

/* Used in Cell Trends Packet. */
#define MAX_TREND_CODE			    8
#define MAX_TREND_VOLUMES   		   10
#define MAX_TREND_VOLUME_TIME  		 1439
#define MIN_TREND_PIXEL			-4096
#define MAX_TREND_PIXEL			 4095

/* Used in Set Color Levels for Contours Packet. */
#define MAX_CONTOUR_COLOR		   15

/* Used in  Linked Contour Vectors. */
#define MIN_NUM_VECTORS			    4
#define MAX_NUM_VECTORS			32764

/* Used in Raster Data Packet. */
#define MAX_SCALE_INT			   67
#define MAX_NUM_ROWS			  464

/* Used in Digital Precipitation Data Array. */
#define NUM_LFM_BOXES_ROW_17		  131
#define NUM_ROWS_17			  131

/* Used in Digital Precipitation Rate Data Array. */
#define NUM_LFM_BOXES_ROW_18		   13
#define NUM_ROWS_18			   13

/***************************************************************************

    Description: Values in the packets are validated against the RPG to 
                 Class 1 ICD.

    Inputs:	 block - pointer to the display data packets.
		 tlen - total bytes of the display data packets.
                 pcode - product code

***************************************************************************/
static int Validate_display_data_packets (unsigned short *block, 
                                          int tlen, int pcode){

    unsigned short *end = block + (tlen / 2);
    unsigned short *start = block; 
    short value = 0;
    unsigned short u_value = 0, temp;
    char c_value = 0;

    int i, j, n_volumes = 0, n_vectors = 0, n_radials = 0;
    int n_rows = 0, n_cells = 0, n_rng_bins = 0, sum_runs = 0;
    unsigned int len;

    static char process_name[LE_NAME_LEN] = "";

    /* If the process_name is blank, get the process name.   The process
       name is needed for any ICD error message. */
   if( strlen( process_name ) == 0 )
      LE_get_process_name( process_name, LE_NAME_LEN );

    /* Parse the product. */
    while (block < end) {

	switch (block[0]) {


	    case 1:
		/* Write Text (No Value) */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Starting Point. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Starting Point in Text Packet (code 1): %d\n", 
                                process_name, value );

                /* Check the J Starting Point. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Starting Point in Text Packet (code 1): %d\n", 
                                process_name, value );


		block += 4 + len;
		break;

	    case 2:
		/* Write Special Symbols (No Value) */
		len = (block[1] + 1) / 2;

                /* Check the I Starting Point. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Starting Point in Text Packet (code 2): %d\n", 
                                process_name, value );

                /* Check the J Starting Point. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Starting Point in Text Packet (code 2): %d\n", 
                                process_name, value );

		block += 2 + len;
		break;

	    case 3:
  		/* Mesocyclone (3) */
		len = (block[1] + 1) / 2;	/* length of packet, in shorts */

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Mesocyclone Packet (code 3): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Mesocyclone Packet (code 3): %d\n", 
                                process_name, value );

                /* Check the Radius of Mesocyclone. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Radius of Mesocyclone in Mesocyclone Packet (code 3): %d\n", 
                                process_name, value );

		block += 2 + len;
		break;

	    case 4:
		/* Wind Barb */

                /* Check the Wind Barb Color. */
                value = (short) block[2];
                if( (value < 1) || (value > MAX_WIND_BARB_COLOR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Wind Barb Color in Wind Barb Packet (code 4): %d\n", 
                                process_name, value );

                /* Check the I Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Wind Barb Packet (code 4): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Wind Barb Packet (code 4): %d\n", 
                                process_name, value );

                /* Check the Wind Direction. */
                value = (short) block[5];
                if( (value < 0) || (value > MAX_WIND_DIR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Wind Direction in Wind Barb Packet (code 4): %d\n", 
                                process_name, value );

                /* Check the Wind Speed. */
                value = (short) block[6];
                if( (value < 0) || (value > MAX_WIND_SPEED) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Wind Speed in Wind Barb Packet (code 4): %d\n", 
                                process_name, value );


		block += 7;
                break;

	    case 5:
		/* Vector Arrow */

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Vector Arrow Packet (code 5): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Vector Arrow Packet (code 5): %d\n", 
                                process_name, value );

                /* Check the Arrow Direction. */
                value = (short) block[4];
                if( (value < 0) || (value > MAX_ARROW_DIR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Arrow Direction in Vector Arrow Packet (code 5): %d\n", 
                                process_name, value );

                /* Check the Arrow Length. */
                value = (short) block[5];
                if( (value < 1) || (value > MAX_ARROW_LEN) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Arrow Length in Vector Arrow Packet (code 5): %d\n", 
                                process_name, value );

                /* Check the Arrow Direction. */
                value = (short) block[6];
                if( (value < 1) || (value > MAX_ARROW_HEAD_LEN) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Arrow Head Length in Vector Arrow Packet (code 5): %d\n", 
                                process_name, value );


		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 6:
		/* Linked Vector (No Value) */

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector. */
                   value = (short) block[2+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                         "%s: Invalid Vector Coordinate in Linked Vector (No Value) Packet (code 6): %d\n", 
                         process_name, value );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 7:
		/* Unlinked Vector (No Value) */

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector. */
                   value = (short) block[2+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                         "%s: Invalid Vector Coordinate in Unlinked Vector (No Value) Packet (code 7): %d\n", 
                         process_name, value );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 8:
		/* Write Text (Uniform Value) */
		len = (block[1] - 6 + 1) / 2;

                /* Check the Text Color Value. */
                value = (short) block[2];
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Color in Text Packet (code 8): %d\n", 
                                process_name, value );

                /* Check the I Starting Point. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Starting Point in Text Packet (code 8): %d\n", 
                                process_name, value );

                /* Check the J Starting Point. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Starting Point in Text Packet (code 8): %d\n", 
                                process_name, value );

		block += 5 + len;
		break;

	    case 9:
		/* Linked Vector (Uniform Value) */

                /* Check the Text Color Value. */
                value = (short) block[2];
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Color in Linked Vector (Uniform Value) Packet (code 9): %d\n", 
                                process_name, value );

		n_vectors = (block[1] + 1) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector Coordinate. */
                   value = (short) block[3+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                                "%s: Invalid Vector Coordinate in Linked Vector (Uniform Value) Packet (code 9): %d\n", 
                                process_name, value );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 10:
		/* Unlinked Vector (Uniform Value) */

                /* Check the Text Color Value. */
                value = (short) block[2];
                if( (value < MIN_COLOR) || (value > MAX_COLOR) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Color in Unlinked Vector (Uniform Value) Packet (code 10): %d\n", 
                                process_name, value );

                /* The number of vectors is the number of bytes in block less 2 bytes for color level
                   divided by 2 since each vector coordinate is 2 bytes. */
		n_vectors = (block[1] - 2) / 2;
                for( i = 0; i < n_vectors; i++ ){

                   /* Check Vector Coordinate. */
                   value = (short) block[3+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                         "%s: Invalid Vector Coordinate in Unlinked Vector (Uniform Value) Packet (code 10): %d\n", 
                         process_name, value );

                }

		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 11:
  		/* 3DC Shear (11) */

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in 3DC Shear Packet (code 11): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in 3DC Shear Packet (code 11): %d\n", 
                                process_name, value );

                /* Check the Radius of Mesocyclone. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Radius of 3DC Shear in 3DC Shear Packet (code 11): %d\n", 
                                process_name, value );

		block += 2 + (block[1] + 1) / 2;	
		break;

	    case 12:
		/* TVS Symbol */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in TVS Packet (code 12): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in TVS Packet (code 12): %d\n", 
                                process_name, value );


		block += 4 + len;
		break;

	    case 13:
	    case 14:
		/* Hail Positive (13) and Hail Probable (14) */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Hail Packet (code 13 or 14): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Hail Packet (code 13 or 14): %d\n", 
                                process_name, value );

		block += 4 + len;
		break;

	    case 15:
                /* Storm ID Packet. */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Storm ID Packet (code 15): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Storm ID  Packet (code 15): %d\n", 
                                process_name, value );

                /* Check the First Character in Cell ID.  Note:  Since this is a Little Endian
                   machine, the lower address is the letter, the upper address the number. */
                c_value = (block[4] & 0x00ff);
                if( (c_value < 'A') || (c_value > 'Z') )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid First Character Cell ID in Storm ID Packet (code 15): %c\n", 
                                process_name, c_value );

                /* Check the Last Character in Cell ID. */
                c_value = (block[4] & 0xff00) >> 8;
                if( (c_value < '0') || (c_value > '9') )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Second Character Cell ID in Storm ID Packet (code 15): %c\n", 
                                process_name, c_value );


		block += 4 + len;
		break;

	    case 16:
		/* Digital Radial Data Array */
		n_radials = block[6];

                /* Check the Index of First Range Bin. */
                value = (short) block[1];
                if( (value < 0) || (value > MAX_DIG_FIRST_RNG_BIN) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid First Range Bin in Packet (code 16): %d\n", 
                                process_name, value );

                /* Check the number of range bins. */
                value = (short) block[2];
                if( (value < 0) || (value > MAX_DIG_NUM_RNG_BINS) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid # of Range Bins in Packet (code 16): %d\n", 
                                process_name, value );

                /* Check the I Center of Sweep. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Center of Sweep in Packet (code 16): %d\n", 
                                process_name, value );

                /* Check the J Center of Sweep. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid J Center of Sweep in Packet (code 16): %d\n", 
                                process_name, value );

                /* Check the Scale Factor. */
                value = (short) block[5];
                if( (value < MIN_DIG_SCALE_FACTOR) || (value > MAX_DIG_SCALE_FACTOR) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Scale Factor in Packet (code 16): %d\n", 
                                process_name, value );

                /* Check the Number of Radials. */
                if( (n_radials < 1) || (n_radials > MAX_DIG_NUM_RADIALS) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Number of Radials in Packet (code 16): %d\n", 
                                process_name, n_radials );
#ifdef DETAILED_OUTPUT
                fprintf( stderr, "Packet 16 (Product Code: %d):\n", pcode );
                fprintf( stderr, "--->First Rng Bin:  %5d\n", (short) block[1] );
                fprintf( stderr, "---># Rng Bins:     %5d\n", (short) block[2] );
                fprintf( stderr, "--->I Center:       %5d\n", (short) block[3] );
                fprintf( stderr, "--->J Center:       %5d\n", (short) block[4] );
                fprintf( stderr, "--->Scale Factor:   %5d\n", (short) block[5] );
                fprintf( stderr, "---># Radials:      %5d\n", (short) block[6] );
#endif

		block += 7;
		for (i = 0; i < n_radials; i++) {

                    /* Check the Number of Bytes In Radial. */
                    value = (short) block[0];
                    if( (value < 1) || (value > MAX_DIG_NUM_RNG_BINS) )
                    LE_send_msg( GL_ERROR,
                                 "%s: Invalid Number of Range Bins in Radial (code 16): %d\n", 
                                 process_name, value );

                    /* Check the Radial Start Angle. */
                    value = (short) block[1];
                    if( (value < 0) || (value > MAX_RAD_START_ANG) )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Radial Start Angle (code 16): %d\n", 
                                    process_name, value );

                    /* Check the Radial Angle Delta. */
                    value = (short) block[2];
                    if( (value < 0) || (value > MAX_RAD_DELTA) )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Radial Delta Angle (code 16): %d\n", 
                                    process_name, value );

#ifdef DETAILED_OUTPUT
                    fprintf( stderr, "------>Radial: %3d, Rng Bins: %3d, Start Angle: %4d, Delta Angle: %2d\n",
                             i, (short) block[0], (short) block[1], (short) block[2] );
#endif

                    /* NOTE:  The length needs to be halved. */
		    len = (block[0] + 1)/2;
		    block += 3 + len;
		}
		break;

	    case 17:
		/* Digital Precipitation Data Array */
		n_rows = block[4];

                /* Check the Number of LFM Boxes in Row. */
                value = (short) block[3];
                if( value != NUM_LFM_BOXES_ROW_17 )
                   LE_send_msg( GL_ERROR, 
                      "%s: Invalid # of LFM Boxes in Row in Digital Precip Data Array Packet (code 17): %d\n", 
                                process_name, value );

                /* Check the Number of Rows. */
                if( n_rows != NUM_ROWS_17 )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid # of Rows in Digital Precip Data Array Packet (code 17): %d\n", 
                                process_name, n_rows );

		block += 5;
		for (i = 0; i < n_rows; i++) {

		    len = (block[0] + 1) / 2;

                    /* Insure the number of range bins matches the sum of the run values. */
                    sum_runs = 0;
                    for( j = 0; j < len; j++ ){

                       unsigned short temp;

                       u_value = block[1+j];
                       temp = ((u_value & 0xff00) >> 8);
                       sum_runs += temp;

                    } 

                    if( sum_runs != NUM_LFM_BOXES_ROW_17 )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Number of LFM Boxes (%d != %d) in Row (code 17)\n", 
                                    process_name, sum_runs, NUM_LFM_BOXES_ROW_17 );

		    block += 1 + len;
		}
		break;

	    case 18:
		/* Digital Rate Array */
		n_rows = block[4];

                /* Check the Number of LFM Boxes in Row. */
                value = (short) block[3];
                if( value != NUM_LFM_BOXES_ROW_18 )
                   LE_send_msg( GL_ERROR, 
                      "%s: Invalid # of LFM Boxes in Row in Digital Precip Rate Data Array Packet (code 18): %d\n", 
                                process_name, value );

                /* Check the Number of Rows. */
                if( n_rows != NUM_ROWS_18 )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid # of Rows in Digital Precip Data Array Packet (code 18): %d\n", 
                                process_name, n_rows );

		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;

                    /* Insure the number of range bins matches the sum of the run values. */
                    sum_runs = 0;
                    for( j = 0; j < len; j++ ){

                       u_value = block[1+j];
                       temp = ((u_value & 0xff00) >> 8);
                       sum_runs += (temp >> 4);

                       temp = (u_value & 0x00ff);
                       sum_runs += (temp >> 4);

                    } 

                    if( sum_runs != NUM_LFM_BOXES_ROW_18 )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Number of LFM Boxes (%d != %d) in Row (code 18)\n", 
                                    process_name, sum_runs, NUM_LFM_BOXES_ROW_18 );
		    block += 1 + len;
		}
		break;

	    case 19:
		/* HDA Hail */
		len = (block[1] + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Hail Packet (code 19): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Hail Packet (code 19): %d\n", 
                                process_name, value );

                /* Check the Probability of Hail. */
                value = (short) block[4];
                if( (value != HAIL_FLAG) 
                           && 
                    ((value < 0) || (value > MAX_HAIL_PROB)) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Probability of Hail in Hail Packet (code 19): %d\n", 
                                process_name, value );

                /* Check the Probability of Severe Hail. */
                value = (short) block[5];
                if( (value != HAIL_FLAG) 
                           && 
                    ((value < 0) || (value > MAX_HAIL_PROB)) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Probability of Severe Hail in Hail Packet (code 19): %d\n", 
                                process_name, value );

                /* Check the Max Hail Size. */
                value = (short) block[6];
                if( (value < 0) || (value > MAX_HAIL_SIZE) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Max Hail Size in Hail Packet (code 19): %d\n", 
                                process_name, value );

		block += 2 + len;
		break;

	    case 20:
		/* MDA Data */

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Special Symbol Packet (code 20): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Special Symbol Packet (code 20): %d\n", 
                                process_name, value );

                /* Check the Point Feature Type. */
                value = (short) block[4];
                if( (value < 1) || (value > MAX_FEATURE_TYPE) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Feature Type in Special Symbol Packet (code 20): %d\n", 
                                process_name, value );


		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 21:
		/* Cell Trend Data */

                /* Check the First Character in Cell ID.  Note:  Since this is a Little Endian
                   machine, the lower address is the letter, the upper address the number. */
                c_value = (block[2] & 0x00ff);
                if( (c_value < 'A') || (c_value > 'Z') )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid First Character Cell ID in Trend Code Packet (code 21): %c\n", 
                                process_name, c_value );

                /* Check the Last Character in Cell ID. */
                c_value = (block[2] & 0xff00) >> 8;
                if( (c_value < '0') || (c_value > '9') )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Second Character Cell ID in Trend Code Packet (code 21): %c\n", 
                                process_name, c_value );

                /* Check the I Position. */
                value = (short) block[3];
                if( (value < MIN_TREND_PIXEL) || (value > MAX_TREND_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in Trend Code Packet (code 21): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[4];
                if( (value < MIN_TREND_PIXEL) || (value > MAX_TREND_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in Trend Code Packet (code 21): %d\n", 
                                process_name, value );

                /* Check the Trend Code. */
                value = (short) block[5];
                if( (value < 1) || (value > MAX_TREND_CODE) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Trend Code in Trend Code Packet (code 21): %d\n", 
                                process_name, value );

                /* Check the Trend Number of Volumes. */
                value = (short) block[6];
                temp = ((value & 0xff00) >> 8);
                if( (temp < 1) || (temp > MAX_TREND_VOLUMES) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Trend # Volumes in Trend Code Packet (code 21): %d\n", 
                                process_name, temp );

                /* Check the Trend Volume Pointer. */
                temp = (value & 0x00ff);
                if( (temp < 1) || (temp > MAX_TREND_VOLUMES) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Trend Volume Pointer in Trend Code Packet (code 21): %d\n", 
                                process_name, temp );

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 22:
		/* Cell Trend Volume Scan Times */

                /* Check the Trend Number of Volumes. */
                n_volumes = (block[2] & 0xff00) >> 8;
                if( (n_volumes < 1) || (n_volumes > MAX_TREND_VOLUMES) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Trend Number of Volumes in Trend Data Packet (code 22): %d\n", 
                                process_name, n_volumes );

                else{

                   /* Check the Trend Volume Pointer. */
                   value = (block[2] & 0x00ff);
                   if( (value < 1) || (value > MAX_TREND_VOLUMES) )
                      LE_send_msg( GL_ERROR, 
                                   "%s: Invalid Trend Volume Pointer in Trend Data Packet (code 22): %d\n", 
                                   process_name, n_volumes );

                   for( i = 0; i < n_volumes; i++ ){

                      /* Check the Trend Volume Times. */
                      value = (short) block[3+i];
                      if( (value < 0) || (value > MAX_TREND_VOLUME_TIME) )
                         LE_send_msg( GL_ERROR, 
                                 "%s: Invalid Trend Volume Time in Trend Code Packet (code 22): %d\n", 
                                 process_name, value );

                   }

                }

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 23:
		/* SCIT Past Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 24:
		/* SCIT Forecast Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 25:
		/* STI Circle */

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in STI Circle Packet (code 25): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in STI Circle Packet (code 25): %d\n", 
                                process_name, value );

                /* Check the Radius of STI Circle. */
                value = (short) block[4];
                if( (value < 1) || (value > MAX_STI_RADIUS) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Radius in STI Circle Packet (code 25): %d\n", 
                                process_name, value );

		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 26:
		/* ETVS Symbol */
		len = (block[1] - 4 + 1) / 2;

                /* Check the I Position. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Position in ETVS Packet (code 26): %d\n", 
                                process_name, value );

                /* Check the J Position. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Position in ETVS Packet (code 26): %d\n", 
                                process_name, value );

		block += 4 + len;
		break;

            case 27:
                /* Superob Data */
                len = ( block[1] << 16 ) | ( block[2] & 0xffff );
                n_cells = (len - sizeof(unsigned short)) / SUPEROB_CELL_SIZE_BYTES;

                /* Skip to the start of the next packet. */
                block +=  4 + (n_cells * (SUPEROB_CELL_SIZE_BYTES/sizeof(unsigned short)));
                break;

            case 28:
                /* Generic product format data. */
                len = ( block[2] << 16 ) | ( block[3] & 0xffff );
                block += 4 + ((len + 1)/2);
                break;

	    case 30:
		/* LFM Grid Adaptation Parameters (Pre-edit) */
		block += 11;
		break;

	    case 31:
		/* Radar Coded Message (Pre-edit) */
		block += 2;
		break;

	    case 32:
		/* Radar Coded Message Composite Reflectivity (Pre-edit) */
		n_rows = block[1];
		block += 2;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 0xaf1f:
		/* Radial Run-Length-Encode */
		n_radials = block[6];

                /* Check the Index of First Range Bin. */
                value = (short) block[1];
                if( (value < 0) || (value > MAX_FIRST_RNG_BIN) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid First Range Bin in Packet (code AF1F): %d\n", 
                                process_name, value );

                /* Check the number of range bins. */
                n_rng_bins = (short) block[2];
                if( (n_rng_bins < 1) || (n_rng_bins > MAX_NUM_RNG_BINS) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Starting Point in Packet (code AF1F): %d\n", 
                                process_name, n_rng_bins );

                /* Check the I Center of Sweep. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Center of Sweep in Packet (code AF1F): %d\n", 
                                process_name, value );

                /* Check the J Center of Sweep. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid J Center of Sweep in Packet (code AF1F): %d\n", 
                                process_name, value );

                /* Check the Scale Factor. */
                value = (short) block[5];
                if( (value < MIN_SCALE_FACTOR) || (value > MAX_SCALE_FACTOR) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Scale Factor in Packet (code AF1F): %d\n", 
                                process_name, value );

                /* Check the Number of Radials. */
                if( (n_radials < 1) || (n_radials > MAX_NUM_RADIALS) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Number of Radials in Packet (code AF1F): %d\n", 
                                process_name, n_radials );
#ifdef DETAILED_OUTPUT
                fprintf( stderr, "Packet AF1F: (Product Code: %d)\n", pcode );
                fprintf( stderr, "--->First Rng Bin:  %5d\n", (short) block[1] );
                fprintf( stderr, "---># Rng Bins:     %5d\n", (short) block[2] );
                fprintf( stderr, "--->I Center:       %5d\n", (short) block[3] );
                fprintf( stderr, "--->J Center:       %5d\n", (short) block[4] );
                fprintf( stderr, "--->Scale Factor:   %5d\n", (short) block[5] );
                fprintf( stderr, "---># Radials:      %5d\n", (short) n_radials );
#endif
		block += 7;

		for (i = 0; i < n_radials; i++) {

		    len = block[0];

                    /* Check the Number of RLE Halfwords. */
                    if( (len < MIN_NUM_RLE_HW) || (len > MAX_NUM_RLE_HW) )
                    LE_send_msg( GL_ERROR,
                                 "%s: Invalid Number RLE Halfwords in Radial (code AF1F): %d\n", 
                                process_name, value );

                    /* Check the Radial Start Angle. */
                    value = (short) block[1];
                    if( (value < 0) || (value > MAX_RAD_START_ANG) )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Radial Start Angle (code AF1F): %d\n", 
                                process_name, value );

                    /* Check the Radial Angle Delta. */
                    value = (short) block[2];
                    if( (value < 0) || (value > MAX_RAD_DELTA) )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Radial Delta Angle (code AF1F): %d\n", 
                                process_name, value );
    
#ifdef DETAILED_OUTPUT
                    fprintf( stderr, "------>Radial: %3d, Halfwords: %3d, Start Angle: %4d, Delta Angle: %2d\n",
                             i, len, (short) block[1], (short) block[2] );
#endif
                    /* Insure the number of range bins matches the sum of the run values. */
                    sum_runs = 0;
                    for( j = 0; j < len; j++ ){

                       u_value = block[3+j];
                       temp = ((u_value & 0xff00) >> 8);
                       sum_runs += (temp >> 4);

                       temp = (u_value & 0x00ff);
                       sum_runs += (temp >> 4);

                    } 

                    if( sum_runs != n_rng_bins )
                       LE_send_msg( GL_ERROR,
                                    "%s: Invalid Number of Range Bins (%d != %d) in Radial (code AF1F)\n", 
                                    process_name, sum_runs, n_rng_bins );

		    block += 3 + len;
		}
		break;


	    case 0x0802:
		/* Contour Value */

                /* Check the Color Value Indicator. */
                value = (short) block[1];
                if( value != 0x0002 )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Color Value Indicator Set Color Levels (code 0802): %x\n", 
                                process_name, value );

                /* Check the Value (Level) of Contour. */
                value = (short) block[2];
                if( (value < 0) || (value > MAX_CONTOUR_COLOR) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Contour Color Value (code 0802): %d\n", 
                            process_name, value );

		block += 3;
		break;

	    case 0x0e03:
		/* Linked Contour Vectors */

                /* Check the I Starting Point. */
                value = (short) block[2];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Starting Point In Linked Vector Packet (code 0x0E03): %d\n", 
                                process_name, value );

                /* Check the J Starting Point. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid J Starting Point In Linked Vector Packet (code 0x0E03): %d\n", 
                                process_name, value );

                /* Check the Length of Vectors. */
                value = (short) block[4];
                if( (value < MIN_NUM_VECTORS) || (value > MAX_NUM_VECTORS) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Length of Vectors In Linked Vector Packet (code 0x0E03): %d\n", 
                                process_name, value );

                len = (block[4] + 1)/2;
                for( i = 0; i < len; i++ ){

                   /* Check the Vector End Point. */
                   value = (short) block[5+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                                   "%s: Invalid Vector End Point In Linked Vector Packet (code 0x0E03): %d\n", 
                                   process_name, value );

                }

                block += len + 5;
		break;

	    case 0x3501:
		/* Unlinked Contour Vectors */

                /* Check the Length of Vectors. */
                value = (short) block[1];
                if( (value < MIN_NUM_VECTORS) || (value > MAX_NUM_VECTORS) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid Length of Vectors In Unlinked Vector Packet (code 0x3501): %d\n", 
                                process_name, value );

                len = (block[1] + 1)/2;
                for( i = 0; i < len; i++ ){

                   /* Check the Vector End Point. */
                   value = (short) block[2+i];
                   if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                      LE_send_msg( GL_ERROR, 
                                   "%s: Invalid Vector End Point In Unlinked Vector Packet (code 0x3501): %d\n", 
                                   process_name, value );

                }

		block += len + 2;
		break;

	    case 0xba0f:
	    case 0xba07:
		/* Raster Run-Length-Encode */

                /* Check the I Coordinate Start. */
                value = (short) block[3];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR, 
                                "%s: Invalid I Coordinate Start in Packet (code BA0F/BA07): %d\n", 
                                process_name, value );

                /* Check the J Coordinate Start. */
                value = (short) block[4];
                if( (value < MIN_PIXEL) || (value > MAX_PIXEL) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid J Coordinate Start in Packet (code BA0F/BA07)\n", 
                                process_name, value );

                /* Check the X Scale INT. */
                value = (short) block[5];
                if( (value < 1) || (value > MAX_SCALE_INT) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid X Scale INT in Packet (code BA0F/BA07)\n", 
                                process_name, value );

                /* Check the Y Scale INT. */
                value = (short) block[7];
                if( (value < 1) || (value > MAX_SCALE_INT) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Y Scale INT in Packet (code BA0F/BA07)\n", 
                                process_name, value );

		n_rows = block[9];

                /* Check the Number of Rows. */
                if( (n_rows < 1) || (n_rows > MAX_NUM_ROWS) )
                   LE_send_msg( GL_ERROR,
                                "%s: Invalid Number of Rows in Packet (code BA0F/BA07)\n", 
                                process_name, n_rows );

		block += 11;
		for (i = 0; i < n_rows; i++) {

		    len = (block[0] + 1) / 2;
		    block += 1 + len;

		}
		break;

	    default:
		LE_send_msg( GL_ERROR, "UMC: Unrecognized Display Data Packet %d in Product %d\n", 
                             (int)block[0], pcode);
                LE_send_msg( GL_ERROR, "--->block offset: %d\n", (int) (block - start) );
		return -1;

	}

    }

    /* Normal return. */
    return 0;
}

/***************************************************************************

    Description: Swaps bytes in shorts for all byte fields in display data 
		packets and layers.

    Inputs:	block - pointer to the display data packets.
		tlen - total bytes of the display data packets.
                pcode - product code

    Output:	block - modified display data packets.

    NOTES:	This function contains product specific processing.  In
		the future, we will want to remove this.	

***************************************************************************/
static int Process_display_data_packets (unsigned short *block, 
                                         int tlen, int pcode){

    unsigned short *end = block + (tlen / 2);
    unsigned short *start = block; 

    while (block < end) {

	switch (block[0]) {
	    int i, n_radials, n_rows, n_cells;
	    unsigned int len;

	    case 1:
		/* Write Text (No Value) */
		len = (block[1] - 4 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 2:
		/* Write Special Symbols (No Value) */
		len = (block[1] + 1) / 2;
		block += 2 + len;
		break;

	    case 3:
  		/* Mesocyclone (3) */
		len = (block[1] + 1) / 2;	/* length of packet, in shorts */
		block += 2 + len;
		break;

	    case 4:
		/* Wind Barb */
		block += 7;
                break;

	    case 5:
		/* Vector Arrow */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 6:
		/* Linked Vector (No Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 7:
		/* Unlinked Vector (No Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 8:
		/* Write Text (Uniform Value) */
		len = (block[1] - 6 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap (block + 5, len);
		block += 5 + len;
		break;

	    case 9:
		/* Linked Vector (Uniform Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 10:
		/* Unlinked Vector (Uniform Value) */
		block += 2 + ((block[1] + 1) / 2);
                break;

	    case 11:
  		/* 3DC Shear (11) */
		block += 2 + (block[1] + 1) / 2;	
		break;

	    case 12:
		/* TVS Symbol */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 13:
	    case 14:
		/* Hail Positive (13) and Hail Probable (14) */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

	    case 15:
                /* Storm ID Packet. */
		len = (block[1] - 4 + 1) / 2;

		/* Swap text data. */
		MISC_short_swap( block + 4, len );
		block += 4 + len;
		break;

	    case 16:
		/* Digital Radial Data Array */
		n_radials = block[6];
		block += 7;
		for (i = 0; i < n_radials; i++) {

                    /* NOTE:  The length needs to be halved. */
		    len = (block[0] + 1)/2;
                    MISC_short_swap( block + 3, len );
		    block += 3 + len;
		}
		break;

	    case 17:
		/* Digital Precipitation Data Array */
		n_rows = block[4];
		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 18:
		/* Digital Rate Array */
		n_rows = block[4];
		block += 5;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 19:
		/* HDA Hail */
		len = (block[1] + 1) / 2;
		block += 2 + len;
		break;

	    case 20:
		/* MDA Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 21:
		/* Cell Trend Data */

		/* Swap cell ID. */
		MISC_short_swap( block + 2, 1 );
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 22:
		/* Cell Trend Volume Scan Times */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 23:
		/* SCIT Past Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 24:
		/* SCIT Forecast Data */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 25:
		/* STI Circle */
		block += 2 + ((block[1] + 1) / 2);
		break;

	    case 26:
		/* ETVS Symbol */
		len = (block[1] - 4 + 1) / 2;
		MISC_short_swap (block + 4, len);
		block += 4 + len;
		break;

            case 27:
                /* Superob Data */
                len = ( block[1] << 16 ) | ( block[2] & 0xffff );
                n_cells = (len - sizeof(unsigned short)) / SUPEROB_CELL_SIZE_BYTES;

                /* Skip to the start of the next packet. */
                block +=  4 + (n_cells * (SUPEROB_CELL_SIZE_BYTES/sizeof(unsigned short)));
                break;

            case 28:
                /* Generic product format data. */
                len = ( block[2] << 16 ) | ( block[3] & 0xffff );
		MISC_short_swap (block + 4, len/2);
                block += 4 + ((len + 1)/2);
                break;

	    case 30:
		/* LFM Grid Adaptation Parameters (Pre-edit) */
		SHORT_SSWAP ( (block + 1) );
		SHORT_SSWAP ( (block + 3) );
		SHORT_SSWAP ( (block + 5) );
		SHORT_SSWAP ( (block + 7) );
		block += 11;
		break;

	    case 31:
		/* Radar Coded Message (Pre-edit) */
		block += 2;
		break;

	    case 32:
		/* Radar Coded Message Composite Reflectivity (Pre-edit) */
		n_rows = block[1];
		block += 2;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    case 0xaf1f:
		/* Radial Run-Length-Encode */
		n_radials = block[6];
		block += 7;

		for (i = 0; i < n_radials; i++) {
		    len = block[0];

                    /* SPECIAL CASE: Product Code 137 (ULR) */
                    if( pcode == MSG_CODE_US_LAYER_CR ){

                       /* Only swap the RLE data, not the RLE header. */
                       MISC_short_swap( block + 3, len ); 

                    }
		    block += 3 + len;
		}
		break;


	    case 0x0802:
		/* Contour Value */
		block += 3;
		break;

	    case 0x0e03:
		/* Linked Contour Vectors */
                len = (block[4] + 1)/2;
                block += len + 5;
		break;

	    case 0x3501:
		/* Unlinked Contour Vectors */
                len = (block[1] + 1)/2;
		block += len + 2;
		break;

	    case 0xba0f:
	    case 0xba07:
		/* Raster Run-Length-Encode */
		n_rows = block[9];
		block += 11;
		for (i = 0; i < n_rows; i++) {
		    len = (block[0] + 1) / 2;
		    block += 1 + len;
		}
		break;

	    default:
		LE_send_msg( GL_ERROR, "UMC: Unrecognized Display Data Packet %d in Product %d\n", 
                             (int)block[0], pcode);
                LE_send_msg( GL_ERROR, "--->block offset: %d\n", (int) (block - start) );
		return -1;

	}

    }

    /* Normal return. */
    return 0;
}

#endif

/***************************************************************************

    Description: Converts the message header block to ICD.

    Inputs:	c_msg - pointer to Pd_msg_header.

    Output:	icd_mhb - pointer to the ICD message header block.
		len - length, in number of halfwords, of the ICD message.

    Return:	The size (number of halfwords) of the icd_mhb.

***************************************************************************/
static int Msg_hd_block_to_icd (void *c_msg, int len, halfword *icd_mhb){

    Pd_msg_header *hd;
    int tmp_time, length;

    hd = (Pd_msg_header *)c_msg;

    icd_mhb[0] = hd->msg_code;
    icd_mhb[1] = hd->date;
    tmp_time = RPG_TIME_IN_SECONDS (hd->time);
    icd_mhb[2] = (tmp_time >> 16) & 0xffff;
    icd_mhb[3] = tmp_time & 0xffff;

    length = len * sizeof (halfword);
    icd_mhb[4] = (length >> 16) & 0xffff;
    icd_mhb[5] = length & 0xffff;

    icd_mhb[6] = hd->src_id;
    icd_mhb[7] = hd->dest_id;
    icd_mhb[8] = hd->n_blocks;

    MISC_short_swap (icd_mhb, MSG_HEADER_LEN);

    return (MSG_HEADER_LEN);
}


#ifdef LITTLE_ENDIAN_MACHINE

/***********************************************************************

   Description:
      This function is the driver module for writing product data to
      a product file.  Called when message conversion fails.

   Inputs:
      msg - product data.
      size - size of the product, in bytes.

   Notes:
      It is assumed that this module is called after the message header
      and description blocks have been byte swapped.

***********************************************************************/
static void Dump_product_data( halfword *msg, int size ){

   int pcode;

    pcode = msg[0];
    LE_send_msg( GL_ERROR, "Writing Product Data To File prod.%d\n", pcode );

#ifdef LITTLE_ENDIAN_MACHINE
    /* Must swap product code because the Print_data routine assumes it is
       not swapped. */
    msg[0] = SHORT_BSWAP(msg[0]);
#endif

    Print_data( msg, size );
    ORPGTASK_exit(GL_ERROR);

}

/*************************************************************************

   Description: 
      This function is used to print product dump for x86 porting.

*************************************************************************/

static void Print_data (halfword *msg, int size){

    static FILE *hdl[200];
    static int wcnt[200];
    unsigned char *cmsg;
    
    int pcode, i, in_line;

    pcode = msg[0];
#ifdef LITTLE_ENDIAN_MACHINE
    pcode = SHORT_BSWAP(msg[0]);
#endif
    if (pcode < 0 || pcode > ORPGPAT_MAX_PRODUCT_CODE) {
	LE_send_msg (GL_ERROR, "UMC: bad pcode %d\n", pcode);
	return;
    }

    if (hdl[pcode] == NULL) {
	char name[128];

	sprintf (name, "prod.%d", pcode);
	hdl[pcode] = fopen (name, "w");
	if (hdl[pcode] == NULL) {
	    LE_send_msg (GL_ERROR, "UMC: open file %s failed\n", name);
	    return;
	}
	wcnt[pcode] = -1;
    }
    else {
	ftruncate (fileno (hdl[pcode]), 0);
	rewind (hdl[pcode]);
    }

    wcnt[pcode]++;
    cmsg = (unsigned char *)msg;

    fprintf (hdl[pcode], "Product write (%d)\n", wcnt[pcode]);
    in_line = 0;
    for (i = 0; i < size; i = i+2) {

       if( (i+1) < size )
          fprintf( hdl[pcode], "%.2x%.2x ", 
                   (unsigned int)cmsg[i], (unsigned int)cmsg[i+1]);
       else
          fprintf( hdl[pcode], "%.2x ", (unsigned int)cmsg[i]);
          
       in_line += 2;
       if ( (in_line % 16) == 0){

	  fprintf (hdl[pcode], "\n");
          in_line = 0;

       }

    }

    fprintf (hdl[pcode], "\n");
    fflush (hdl[pcode]);

    return;
}

#endif
