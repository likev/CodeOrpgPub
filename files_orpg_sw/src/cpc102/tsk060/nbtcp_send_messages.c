/********************************************************************************

     File: send_messages.c
           This file contains the routines that builds narrowband user messages
           and sends them to the socket to be written to the rpg.

 ********************************************************************************/


/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/01/28 18:30:07 $
 * $Id: nbtcp_send_messages.c,v 1.24 2011/01/28 18:30:07 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <inttypes.h>

#include <infr.h>
#include <orpg_def.h>
#include <nbtcp.h>
#include <prod_user_msg.h>
#include <orpgmisc.h>


   /* process level globals */

extern int    PVC0_sock;
extern int    PVC1_sock;
extern int    PIDS[MAXRPS];
extern char   MNE[MAXRPS][4];
extern char   Password[10];
extern char   Userpass[10];
extern char   Portpass[10];
extern u_short User_id;
extern int    Otrs_pending;  /* number of OTRs remaining to be received */



   /* file scope globals */

 static char *Msg_buf;    /* temp buffer used to construct msgs throughout 
                             this file - it is defined globally for 
                             convenience so buffer space does not have to
                             be malloc'ed and freed in every routine */
static char Tool_data_dir [FILENAME_MAX];
static int  Byte_swap_required = FALSE;    /* endian byte swap flag */

   /* local function prototypes */

static short Get_msg_seq_num ();
static int   Read_alert_cats (int num_cats, ushort *cats, ushort *codes, 
                              ushort *req_flag);
static int   Read_bias_rows  (int n_rows, Memory_span_t *mem_span_blk);
static int   Read_params     (int *params);
static void  Update_msg_hdr  (msg_header *hdr, short msg_code, int msg_len, 
                              short number_blks);
static void  Write_tcp_hdr   (int id, int msg_type, int msg_len, int pvc);
static int   Pack_ushorts_with_value ( void *loc, void *value );


/***********************************************************************

    Description: This function generates a alert request message and 
                 sends it to the p_server.
          Input:

         Output:

         Return: 0 on success; -1 on error

***********************************************************************/


#define NUMBER_FIELDS_PER_CAT         3
#define NUMBER_BYTES_PER_SHORT        2
#define NUMBER_ROWS_PER_ALERT_GRID   58
#define NUMBER_BYTES_PER_ALERT_ROW    8


int SM_alert_request_msg ()
{
   int        i, j, k;
   int        err;
   char       data_file [FILENAME_MAX];
   char       alerts [2][128];
   char       buf[128];
   msg_header *msg_hdr;
   ushort     *spt;
   ushort     msg_code;
   int        msg_id;         /* TCP msg id */
   int        msg_type;       /* TCP msg type */
   int        msg_len;        /* msg length */
   ushort     num_blks;
   ushort     *pt;
   char       log_msg [256];
   int        ret;
   ushort     alert_area;
   ushort     number_cats;
   ushort     alert_cat [10];
   ushort     threshold_code[10];
   ushort     prod_req_flag[10];
   int        temp;
   int        row;
   char       tmp_buf[128];

   strncpy (&data_file[0], "", 1);

   strcpy (data_file, Tool_data_dir);
   strcat (data_file, "alert_req.msg");

   CS_cfg_name (data_file);

   /* Re-read the CS file in the event it has changed. */
   CS_control(CS_UPDATE);

   if ((ret = CS_level (CS_TOP_LEVEL)) < 0) {
       printf ("CS_TOP_LEVEL failed (ret: %d)\n", ret);
       return (-1);
   }

   strcpy (alerts[0], "Alert_area_1");
   strcpy (alerts[1], "Alert_area_2");

   msg_hdr = (msg_header *)Msg_buf;
   spt = (ushort *)Msg_buf;
   spt += (MSG_HDR_LEN / sizeof (ushort));

      /* write a msg for each alert area */

   for (i = 0; i < 2; i++) {

       CS_control (CS_KEY_OPTIONAL);

       if( (CS_entry (alerts[i], 0, 128, buf) <= 0) 
                          ||
           (CS_level( CS_DOWN_LEVEL) < 0) ){
           CS_cfg_name ("");
           return -1;
       }

       if (CS_entry ("alert_area", 1 | CS_SHORT, 0, (void *)&alert_area) <= 0 ||
           CS_entry ("number_cats", 1 | CS_SHORT, 0, (void *)&number_cats) <= 0) {

/*           printf ("read alert request msg failed\n"); */
           CS_cfg_name ("");
           return -1;
       }

       if (Read_alert_cats (number_cats, alert_cat, threshold_code, prod_req_flag)) {

          printf ("parsing alert categories failed\n");
          CS_cfg_name ("");
          return -1;
       }

         /* update the msg buffer (halfword increments) */

       spt[0] = -1;      /* block divider */
       msg_len = ((ushort)(number_cats * NUMBER_FIELDS_PER_CAT * NUMBER_BYTES_PER_SHORT) + 
                  (NUMBER_ROWS_PER_ALERT_GRID * NUMBER_BYTES_PER_ALERT_ROW) + 8); 
       spt[1] = msg_len;
       spt[2] = alert_area;      /* alert area number */
       spt[3] = number_cats;     /* number categories */

          /* byte swap if needed */

       if (Byte_swap_required == TRUE) {
          if ((ret = SMIA_bswap_output ("alert_area_t", spt, 
                                         sizeof (alert_area_t))) < 0) {
             sprintf (log_msg, 
                   "SMIA_bswap_output for\"alert_area_t\" failed: ret : %d\n", ret);
             MA_printlog(log_msg);
             MA_abort ("Byte swap routine failed\n");
          }
       }

       pt = &spt[4];

         /* add the alert categories to the msg buffer */

      for (j = 0; j < number_cats; j++) {
         pt[0] = alert_cat[j];
         pt[1] = threshold_code [j];
         pt[2] = prod_req_flag [j];

            /* byte swap if needed */

         if (Byte_swap_required == TRUE) {
            if ((ret = SMIA_bswap_output ("Pd_alert_category_def_msg", pt, 
                                           sizeof (Pd_alert_category_def_msg))) < 0) {
               sprintf (log_msg, 
                     "SMIA_bswap_output for\"alert_area_t\" failed: ret : %d\n", ret);
               MA_printlog(log_msg);
               MA_abort ("Byte swap routine failed\n");
            }
         }
         pt += (ushort) NUMBER_FIELDS_PER_CAT;
      }

         /* copy the alert grid to the buffer (grid matrix: 58 rows x 4 columns) */

      if ((err = CS_entry ("alert_grid", 0, 128, tmp_buf)) <= 0)  {
             sprintf (log_msg, 
                  "Error reading \"alert_grid\" key for alert area %d (err: %d)\n", alert_area, err);
             MA_printlog(log_msg);
             return -1;
      }

      for (j = 0; j < NUMBER_ROWS_PER_ALERT_GRID; j++) {

         if ((err = CS_entry(CS_NEXT_LINE, 0 | CS_INT, 0, (void *)&row)) <= 0)  {
            sprintf (log_msg, 
                    "Error accessing alert grid row %d in alert area %d (err: %d; index: %d)...populating row with fixed pattern\n",
                     row, alert_area, err, j);
            MA_printlog(log_msg);

            if (alert_area == 1) {
               pt [0] = 0xffff;
               pt [1] = 0xfff8;
               pt [2] = 0x0000;
               pt [3] = 0x0000;
            }
            else {  /* assign row pattern for alert area 2 */
               pt [0] = 0x0000;
               pt [1] = 0x0007;
               pt [2] = 0xffff;
               pt [3] = 0xffc0;
            }
        
         } else {   

            for (k = 1; k <= 4; k++) {
               if ((err = CS_entry(CS_THIS_LINE, k | CS_HEXINT, 5, (void *) &temp)) <= 0) {
                  sprintf (log_msg, 
                           "Error reading column %d in row %d, alert area %d (err: %d)...populating column with xffff\n", 
                            k, row, alert_area, err);
                  MA_printlog(log_msg);
                  temp = 0xffff;
               }
               pt[k-1] = (short) temp;
            }
         }

            /* byte swap if needed */

         if (Byte_swap_required == TRUE) {
            if ((ret = SMIA_bswap_output ("Pd_alert_box_msg", pt, 
                                          sizeof (Pd_alert_box_msg))) < 0) {
               sprintf (log_msg, 
                     "SMIA_bswap_output for\"alert_area_t\" failed: ret : %d\n", ret);
               MA_printlog(log_msg);
               MA_abort ("Byte swap routine failed\n");
            }
         }
         pt += (ushort) (NUMBER_BYTES_PER_ALERT_ROW / NUMBER_BYTES_PER_SHORT);
      }

      msg_len += MSG_HDR_LEN; 

         /* update msg hdr and write msg */

      msg_code = 7;
      num_blks = 2;

      Update_msg_hdr (msg_hdr, msg_code, msg_len, num_blks);

      if (MA_get_interface() == NATIVE_INTERFACE) {

            /* write TCP header */

         msg_id = 0;      /* no ack expected */
         msg_type = DATA; /* message type = 2 */

         Write_tcp_hdr (msg_id, msg_type, msg_len, PVC1_sock);

         if (SOC_sock_write(PVC1_sock, Msg_buf, msg_len) != msg_len) {
             printf ("Error sending Alert Request Msg");
             return (-1);
         }

      } else {  /* write msg using the cm_tcp interface */

         if (CMT_write (CM_WRITE, MA_get_link_number(), Msg_buf, msg_len) < 0) {
            printf ("Error sending Alert Request Msg");
            return (-1);
         }
      }

      CS_level (CS_UP_LEVEL);
   }
   return (0);
}


/********************************************************************************

    Description: This routine sets the byte swap required flag. Byte swapping
                 is dependent on the endianess of the local host.

          Input:

         Output:

         Return: 

 ********************************************************************************/

void SM_set_byte_swap_flag ()
{
   if (MISC_i_am_bigendian() == 0) {  /* 0 means little endian machine */
      Byte_swap_required = TRUE;
/* printf("Byte swapping is required\n"); */
      SMIA_set_smi_func (SWAp_bytes); /* function SWAp_bytes located in ..._smipp.c */
   }
   else {
      Byte_swap_required = FALSE;
/* printf("Byte swapping is not required\n"); */
   }

/* sleep(1); */
   return;
}


/********************************************************************************

    Description: This routine initalizes the path where the tool's message files
                 are located

          Input:

         Output:

         Return: pointer to the tool's data directory

 ********************************************************************************/

#define BUF_SIZE   20000

char *SM_init_data_dir ()
{
   char *dir_ptr;
   char log_msg [256];

   strncpy (&Tool_data_dir [0], "", 1);

   dir_ptr = getenv ("TOOLS_DIR");

   if (!dir_ptr)
   {
      dir_ptr = getenv ("CFG_DIR");

      if (!dir_ptr) {
      
         printf ("\n\nNeither the TOOLS_DIR nor CFG_DIR environment variable is setup in\n");
         printf ("your environment. Please set one of the environment variables to\n");
         printf ("the directory where the tool's message files are located, then\n");
         printf ("restart the tool\n\n");

         exit (3);
      }
   }

   strcpy (Tool_data_dir, dir_ptr);
   strcat (Tool_data_dir, "/\0");

/*   printf ("tool data directory: %s\n", Tool_data_dir); */

      /* malloc the temp msg buffer space here */

   if ((Msg_buf = malloc (BUF_SIZE)) == NULL) {
      sprintf (log_msg, 
           " SM_init_data_dir: Msg_buf malloc failed (errno: %d)\n", errno);
      MA_printlog(log_msg);
      MA_abort ("malloc failed...program aborting \n");
   }

   return (Tool_data_dir);
}

/***********************************************************************

    Description: This function generates a bias table message

          Input:

         Output:

         Return: 0 on success; -1 on error

***********************************************************************/

int SM_send_bias_table_msg ()
{
    int i;
    char             buf[128];
    char             log_msg [256];
    char             data_file [FILENAME_MAX];
    ushort           msg_code;
    ushort           num_blks;
    int              ret;
    int              msg_id;                 /* TCP msg id */
    int              msg_type;               /* TCP msg type */
    int              msg_len;                /* msg length */
    int              n_rows;                 /* # mem span rows */
    bias_table_msg_base_t bt_msg_base;       /* bias table msg seg */
    Block_id_t       bias_block;
    Memory_span_t    mem_span[12];
    ushort           *ptr;                   /* msg buf ptr for constructing ICD msg */  
    msg_header       *msg_hdr;               /* msg header ptr */
    int              bias_msg_base_len = 34; /* ICD msg base len in bytes */
    int              mem_span_blk_len = 20;  /* ICD mem span block len in bytes */
    int              bias_block_len = 8;     /* ICD block len in bytes */

    strncpy (&data_file[0], "", 1);
    strcpy (data_file, Tool_data_dir);
    strcat (data_file, "bias_table.msg");

    CS_cfg_name (data_file);

    /* Re-read the CS file in the event it has changed. */
    CS_control(CS_UPDATE);

    if ((ret = CS_level (CS_TOP_LEVEL)) < 0) {
       printf ("CS_TOP_LEVEL failed (ret: %d)\n", ret);
       return (-1);
    }

      /* initialize msg ptrs */

    msg_hdr = (msg_header *)Msg_buf;
    ptr = (ushort *)Msg_buf;
    ptr += (MSG_HDR_LEN / sizeof (ushort));

    if( (CS_entry ("Bias_table", 0, 128, buf ) <= 0 )
                       ||
        (CS_level( CS_DOWN_LEVEL ) < 0) ){ 
        CS_cfg_name ("");
        return -1;
    }

    if (CS_entry ("site_id", 1, 4, (void *)&bt_msg_base.awips_id[0]) <= 0 ||
        CS_entry ("radar_id", 1, 4, (void *)&bt_msg_base.radar_id[0]) <= 0 ||
        CS_entry ("obs_year", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_yr) <= 0 ||
        CS_entry ("obs_month", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_mon) <= 0 ||
        CS_entry ("obs_day", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_day) <= 0 ||
        CS_entry ("obs_hour", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_hr) <= 0 ||
        CS_entry ("obs_min", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_min) <= 0 ||
        CS_entry ("obs_sec", 1 | CS_SHORT, 0, (void *)&bt_msg_base.obs_sec) <= 0 ||
        CS_entry ("gen_year", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_yr) <= 0 ||
        CS_entry ("gen_month", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_mon) <= 0 ||
        CS_entry ("gen_day", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_day) <= 0 ||
        CS_entry ("gen_hour", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_hr) <= 0 ||
        CS_entry ("gen_min", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_min) <= 0 ||
        CS_entry ("gen_sec", 1 | CS_SHORT, 0, (void *)&bt_msg_base.gen_sec) <= 0 ||
        CS_entry ("n_rows", 1 | CS_SHORT, 0, (void *)&bt_msg_base.n_rows) <= 0 ){

        printf ("read bias table failed\n");
        CS_cfg_name ("");
        return -1;
    }

    n_rows = bt_msg_base.n_rows;

    if (Read_bias_rows (n_rows, mem_span) != 0) {

       printf ("parsing bias table rows failed\n");
       CS_cfg_name ("");
       return -1;

    }

    if (CS_level (CS_UP_LEVEL) < 0){

/*       printf("read bias table rows failed\n" ); */
       CS_cfg_name ("");
       return -1;

    }

       /* update the msg block info */

    bias_block.divider = -1;
    bias_block.block_id = 1;
    bias_block.version = 0;
    bias_block.length = bias_msg_base_len + bias_block_len + 
                      (n_rows * mem_span_blk_len); 

      /* byte swap the bias table msg block segment if needed */

   if (Byte_swap_required == TRUE) {
      if ((ret = SMIA_bswap_output 
                 ("Block_id_t", &bias_block, bias_block_len)) < 0) {
         sprintf (log_msg, "Byte swapping \"bias_block\" failed (ret: %d)\n", ret);
         MA_printlog(log_msg);
         MA_abort ("Byte swap routine failed\n");
      }
   }

      /* copy block info to msg buffer and update msg buffer ptr */

   memcpy (ptr, &bias_block, bias_block_len);
   ptr += (bias_block_len / sizeof (ushort));

      /* byte swap the bias table msg segment if needed */

   if (Byte_swap_required == TRUE) {
      if ((ret = SMIA_bswap_output 
                 ("bias_table_msg_base_t", &bt_msg_base, bias_msg_base_len)) < 0) {
         sprintf (log_msg, "Byte swapping \"bias_table_msg_base\" failed (ret: %d)\n", ret);
         MA_printlog(log_msg);
         MA_abort ("Byte swap routine failed\n");
      }
   }

      /* copy msg base info to msg buffer and update msg buffer ptr */

   memcpy (ptr, &bt_msg_base, bias_msg_base_len);
   ptr += (bias_msg_base_len / sizeof (ushort));

      /* copy memory span data for all rows defined to msg buffer and update 
         msg buffer ptr */

   for ( i = 0; i < n_rows; i++ ) {

         /* byte swap the mem span data if needed */

      if (Byte_swap_required == TRUE) {
         if ((ret = SMIA_bswap_output 
                ("Memory_span_t", &mem_span[i], mem_span_blk_len)) < 0) {
            sprintf (log_msg, "Byte swapping \"bias_table_msg\" failed (ret: %d)\n", ret);
            MA_printlog(log_msg);
            MA_abort ("Byte swap routine failed\n");
         }
      }
      memcpy (ptr, &mem_span[i], mem_span_blk_len);
      ptr += (mem_span_blk_len / sizeof (ushort));
   }

   CS_cfg_name ("");

      /* write TCP hdr */
 
   msg_len = MSG_HDR_LEN + bias_msg_base_len + bias_block_len +
             (n_rows * mem_span_blk_len);

   msg_code = 15;
   num_blks = 2;

      /* update msg hdr and write msg */

   Update_msg_hdr (msg_hdr, msg_code, msg_len, num_blks);

   if (MA_get_interface() == NATIVE_INTERFACE) {

      msg_id = 0;      /* no ack expected */
      msg_type = DATA; /* message type = 2 */

      Write_tcp_hdr (msg_id, msg_type, msg_len, PVC1_sock);

      if (SOC_sock_write(PVC1_sock, Msg_buf, msg_len) != msg_len) {
         printf ("Error: Bias Table Msg write failed");
         return (-1);
      }
   } else {  /* write msg using the cm_tcp interface */

      if (CMT_write (CM_WRITE, MA_get_link_number(), Msg_buf, msg_len) < 0) {
         printf ("Error: Bias Table Msg write failed");
         return (-1);
      }
   }

   return 0;
}

/***********************************************************************

    Description: This function sends a class 2 sign on message

          Input:

         Output:

         Return: 0 on success; -1 on error

***********************************************************************/

#define SIGN_ON_BLOCK_LEN 18  /* in bytes */

int SM_send_sign_on_msg ( )
{
   msg_header   *msg_hdr;
   Sign_on_msg_t sign_on_block;
   ushort        msg_code;
   ushort       *ptr;            /* msg buf ptr for constructing ICD msg */  
   int           msg_id;         /* TCP msg id */
   int           msg_type;       /* TCP msg type */
   int           msg_len;        /* msg length */
   ushort        num_blks;

   /* send a sign-on request */

   /* initialize msg ptrs */

   msg_hdr = (msg_header *)Msg_buf;
   ptr = (ushort *)Msg_buf;
   ptr += (MSG_HDR_LEN / sizeof (ushort));

   /* send the TCP hdr */

   msg_id = 0;      /* no ack expected */
   msg_type = DATA; /* message type = 2 */

   msg_len = MSG_HDR_LEN + SIGN_ON_BLOCK_LEN; 

   /* write the TCP header */

   Write_tcp_hdr (msg_id, msg_type, msg_len, PVC0_sock);

   /* update the msg hdr and block info then send the msg */

   msg_code = 11;
   num_blks = 2;
   sign_on_block.divider = -1;
   sign_on_block.length  = 18;
   sign_on_block.disconn_override_flag = 0; 
   strncpy(sign_on_block.user_passwd, Userpass, USER_PASSWD_LEN);
   strncpy(sign_on_block.port_passwd, Portpass, PORT_PASSWD_LEN);
   memcpy (ptr, &sign_on_block, SIGN_ON_BLOCK_LEN);

   Update_msg_hdr (msg_hdr, msg_code, msg_len, num_blks);

   if(SOC_sock_write(PVC0_sock, Msg_buf, msg_len) != msg_len)
       MA_abort("Sign-on write failed");

   return (0);
}

/***********************************************************************

    Description: This function generates a product request message

          Input:

         Output:

         Return: 0 on success; -1 on error

***********************************************************************/

int SM_send_product_request_msg (int pvc)
{
    char       data_file [FILENAME_MAX];
    msg_header *msg_hdr;
    ushort     *spt;
    int        cnt;
    ushort     msg_code;
    ushort     num_blks;
    int        msg_id;        /* TCP msg id */
    int        msg_type;      /* TCP msg type */
    int        msg_len;       /* msg length */
    char       log_msg [128];
    int        ret;
    int        ret1 = 0;

    strncpy (&data_file[0], "", 1);

    strcpy (data_file, Tool_data_dir);
    strcat (data_file, "one_time_req.msg");

    CS_cfg_name (data_file);

    /* Re-read the CS file in the event it has changed. */
    CS_control(CS_UPDATE);

    if ((ret = CS_level (CS_TOP_LEVEL)) < 0) {
       printf ("CS_TOP_LEVEL failed (ret: %d)\n", ret);
       return (-1);
    }

    if ( (ret1 = CS_level (CS_DOWN_LEVEL)) < 0) {
         printf ("CS_level (ret: %d) failed\n", ret1);
         return (-1);
    }
    msg_hdr = (msg_header *)Msg_buf;
    spt = (ushort *)Msg_buf;
    spt += (MSG_HDR_LEN / sizeof (ushort));

    cnt = 0;

    while (1) {
       int prod_code, flag, num_prods, interval; 
       int vol_date, vol_time;
       int params[6];
       ushort *pt;

       if (CS_entry ("prod_code", 1 | CS_INT, 0, (void *)&prod_code) <= 0 ||
           CS_entry ("flag", 1 | CS_HEXINT, 0, (void *)&flag) <= 0 ||
           CS_entry ("num_prods", 1 | CS_INT, 0, (void *)&num_prods) <= 0 ||
           CS_entry ("interval", 1 | CS_INT, 0, (void *)&interval) <= 0 ||
           CS_entry ("vol_date", 1 | CS_INT, 0, (void *)&vol_date) <= 0 ||
           CS_entry ("vol_time", 1 | CS_INT, 0, (void *)&vol_time) <= 0) {
           printf ("read product message fields failed\n"); 
           CS_cfg_name ("");
           return -1;
       }

       if (Read_params (params) != 0) {
          printf ("read product parameters failed\n"); 
          CS_cfg_name ("");
          return -1;
       }

       if (CS_level (CS_UP_LEVEL) < 0)
          continue;

       pt = spt + (cnt * 16);      /* block ptr */
       pt[0] = -1;                 /* block divider */
       pt[1] = 32;                 /* block length */
       pt[2] = prod_code;          /* product code */
       pt[3] = flag;               /* flag bits */
       pt[4] = Get_msg_seq_num (); /* msg sequence number */
       pt[5] = num_prods;          /* number of products */
       pt[6] = interval;           /* request interval */
       pt[7] = vol_date;           /* volume date */
       memcpy( &pt[8], &vol_time, sizeof(int) ); /* volume time */
       pt[10] = params[0];         /* product parameters */
       pt[11] = params[1];
       pt[12] = params[2];
       pt[13] = params[3];
       pt[14] = params[4];
       pt[15] = params[5];

          /* byte swap if needed */

       if (Byte_swap_required == TRUE) {
          if ((ret = SMIA_bswap_output 
                     ("Pd_request_products", pt, sizeof (Pd_request_products))) < 0) {
             sprintf (log_msg, "SMIA_bswap_output failed: ret : %d\n", ret);
             MA_printlog(log_msg);
             MA_abort ("Byte swap routine failed\n");
          }
       }

       cnt++;

       if (CS_entry (CS_NEXT_LINE, 0, 0, NULL) < 0)
           break;

       if (CS_level (CS_DOWN_LEVEL) < 0)
         continue;
    }

    CS_cfg_name ("");

    msg_len = MSG_HDR_LEN + ((cnt * 16) * 2);  /* message length in bytes */

    msg_code = 0;
    num_blks = cnt + 1;
    Otrs_pending = cnt;
    printf("%d products requested.\n",cnt); sleep(1);

       /* update msg hdr and write msg */

    Update_msg_hdr (msg_hdr, msg_code, msg_len, num_blks);

   if (MA_get_interface() == NATIVE_INTERFACE) {

         /* update and write TCP header */

       msg_id = 0;     /* no ack expected */
       msg_type = DATA; /* message type = 2 */

       Write_tcp_hdr (msg_id, msg_type, msg_len, pvc);

       if (SOC_sock_write(pvc, Msg_buf, msg_len) != msg_len) {
          printf ("Error: Onetime Request Msg write failed");
          return (-1);
       }
   } else {  /* write msg using the cm_tcp interface */

      if (CMT_write (CM_WRITE, MA_get_link_number(), Msg_buf, msg_len) < 0) {
          printf ("Error: Onetime Request Msg write failed");
          return (-1);
      }    
   }
   
   return 0;
}


/********************************************************************************

    Description: This routine sends an rps list to the ORPG

          Input: rps_file - RPS file name

         Output:

         Return: 0 on success; -1 on error

 ********************************************************************************/

int SM_sendrps(char *rps_file) {

   int                rpslength, i, j;
   char	             prodno[3];
   char	             *msgbuf;
   msg_header         *msghead;
   RPS_block          block;
   product_msg_info_t product;
   int                eof;
   ushort             msg_code;
   ushort             noblks;
   int                msg_id;       /* TCP msg id */
   int                msg_type;     /* TCP msg type */
   FILE               *fp;
   int                pidindex = 4;
   char               log_msg [128];
   char               data_file [FILENAME_MAX];
   int                ret;
   int                neg_one = -1;

     /* build an RPS list and send it */

    strncpy (&data_file[0], "", 1);

    strcpy (data_file, Tool_data_dir);
    strcat (data_file, rps_file);

   if ((fp = fopen (data_file, "r")) == NULL) {
         fprintf (stderr, "Error opening file %s (err: %d)\n", data_file, errno);
         return (-1);
   }

     /* alloc memory for RPS list */

   if ((msgbuf = (char *)malloc(MAXRPS * sizeof(RPS_block))) == NULL)
       MA_abort("malloc rps list msgbuf failed");
        
   j = MSG_HDR_LEN; /* offset into msgbuf to start rps list, past header */
   noblks = 0;      /* number products in rps list */
    
      /* traverse the product file and build the RPS list */
  
    while (RPL_read_product_list (fp, &product, &eof) == 0) {
         /* fill in the product request blocks */

         /* fixed stuff */

      block.divider = -1;
      block.length_of_blk = 32;
      block.flag = 0;

         /* high priority request */
      if( (short) atoi(product.prod_id) < 0 )
         block.flag |= 0x8000;

      block.no_of_prods = -1;
      block.vol_scan_date = -1;
      Pack_ushorts_with_value( &block.vol_scan_time_msw, &neg_one );
      block.req_intrvl = 1;

         /* add product information from the product file */

      block.prod_code = (short)(atoi(product.prod_code));

         /* put the product code in the pid array */

      PIDS[pidindex] = atoi(product.prod_code);
      strcpy(MNE[pidindex], product.mne);

         /* for one char names, add the product code to it */

      if(MNE[pidindex][1] == ' ') { 
         MNE[pidindex][1] = '\0';
         sprintf(prodno, "%d", PIDS[pidindex]);    
         strncat(MNE[pidindex],prodno, 2);
      } 
  
      if(MNE[pidindex][2] == ' ')MNE[pidindex][2] = '\0';

      strcat(MNE[pidindex],"\0");	    
      pidindex++;            
      block.seq_no = (ushort)(Get_msg_seq_num());

      for (i = 0; i < NUMBER_PROD_PARMS; i++) {
          block.prod_parms [i] = (short) atoi (product.params [i]);
      }

/* printf("rps product %d sent: blk #: %d\n", block.prod_code, noblks); */

         /* byte swap if needed */

      if (Byte_swap_required == TRUE) {
         if ((ret = SMIA_bswap_output ("RPS_block", &block, sizeof (RPS_block))) < 0) {
            sprintf (log_msg, "SMIA_bswap_output failed (ret: %d)\n", ret);
            MA_printlog(log_msg);
            fprintf (stderr, "Byte swap routine failed\n");
            return (-1);
         }
      }

      memcpy(&msgbuf[j], &block, sizeof(RPS_block));

       j = j + sizeof(RPS_block);
       noblks++;
   }

    PIDS[pidindex] = -1;

   if (!eof) {
       fprintf (stderr, "Error reading rps file");
       return (-1);
   }

      /* send the RPS list on pvc1 */

   rpslength = noblks * sizeof(RPS_block) + MSG_HDR_LEN; /* # prods + header */
      
   msghead = (msg_header *)msgbuf;

   msg_code = 0;  /* msg code */

      /* update the msg hdr and write msg */

   Update_msg_hdr (msghead, msg_code, rpslength, noblks + 1);

   if (MA_get_interface() == NATIVE_INTERFACE) {
 
         /* update and write the TCP hdr */
 
      msg_id = 0;       /* no ack expected */
      msg_type = DATA;

      Write_tcp_hdr (msg_id, msg_type, rpslength, PVC1_sock);

      if (SOC_sock_write(PVC1_sock, msgbuf, rpslength) != rpslength) {
          printf ("Error: RPS List write failed");
          return (-1);
      }

   } else {  /* write msg using the cm_tcp interface */

      if (CMT_write (CM_WRITE, MA_get_link_number(), msgbuf, rpslength) < 0) {
          printf ("Error: RPS List write failed");
          return (-1);
      }
   }

/* printf("RPS list sent!\n"); */

   sprintf(log_msg, "RPS List sent (number products in list: %d)\n",
           noblks);

   MA_printlog(log_msg);

   fclose (fp);
   free(msgbuf);
   return (0);
}


/********************************************************************************

     Description: This routine returns the next msg sequence number

           Input:

          Output:

          Return: the next msg sequence number

 ********************************************************************************/

short Get_msg_seq_num ()
{
   static short sequence_number = 100;

   sequence_number++;

   return (sequence_number & 0x7fff);
}


/********************************************************************************

    Description: This routine reads the paired alert categories, threshold 
                 codes, and product request flags for each defined alert 
                 category.

          Input: num_cats - number of alert categories to process

         Output: cat      - the alert category
                 code     - the threshold code that triggers the alert
                 req_flag - product required flag (specifies if a product
                            is requested for the alert when the threshold is met

         Return: 0 on success; -1 on error

 ********************************************************************************/

static int Read_alert_cats (int num_cats, ushort *cat, ushort *code, 
                            ushort *req_flag)
{
   int i;

   for (i = 0; i < num_cats; i++) {
      if (CS_entry ("alert_cat", (i + 1) | CS_SHORT, 0, 
          (void *) &cat[i]) <= 0) 
	      return (-1);
   }

   for (i = 0; i < num_cats; i++) {
      if (CS_entry ("threshold_code", (i + 1) | CS_SHORT, 0, 
                    (void *) &code[i]) <= 0) 
         return (-1);
   }

   for (i = 0; i < num_cats; i++) {
      if (CS_entry ("prod_req_flag", (i + 1) | CS_SHORT, 0, 
                    (void *) &req_flag[i]) <= 0) 
         return (-1);
   }

   return (0);
}


/**************************************************************************

    Description: This function reads and parses the rows in the bias table.

              Input: n_rows - number of bias table rows.

          IN/Output: mem_span_blk  - memory span block structure

             Return: 0 on success or -1 on failure

**************************************************************************/

static int Read_bias_rows (int n_rows, Memory_span_t *mem_span_blk)
{
    int temp, i;

    for (i = 0; i < n_rows; i++) {

       if (CS_entry ("mem_span", (i + 1) | CS_INT, 0, (void *) &temp) <= 0) 
          return (-1);

       Pack_ushorts_with_value (&mem_span_blk[i].mem_span_msw, &temp); 
    }

   for (i = 0; i < n_rows; i++) {

      if (CS_entry ("n_pairs", (i + 1) | CS_INT, 0, (void *) &temp) <= 0) 
         return (-1);

      Pack_ushorts_with_value (&mem_span_blk[i].n_pairs_msw, &temp); 
   }

   for (i = 0; i < n_rows; i++) {

      if (CS_entry ("avg_gage", (i + 1) | CS_INT, 0, (void *) &temp) <= 0) 
         return (-1);

      Pack_ushorts_with_value (&mem_span_blk[i].avg_gage_msw, &temp); 
   }

   for (i = 0; i < n_rows; i++) {

      if (CS_entry ("avg_radar", (i + 1) | CS_INT, 0, (void *) &temp) <= 0) 
          return (-1);

      Pack_ushorts_with_value (&mem_span_blk[i].avg_radar_msw, &temp); 
   }

   for (i = 0; i < n_rows; i++) {

      if (CS_entry ("bias", (i + 1) | CS_INT, 0, (void *) &temp) <= 0) 
          return (-1);

      Pack_ushorts_with_value (&mem_span_blk[i].bias_msw, &temp); 
   }
   return (0);
}


/**************************************************************************

    Description: This function reads and parses the six product 
                 dependent parameters.
                 This is adapted from a similar function in ipi.c.

          Input:

         Output: params: The parameter array.

         Return: 0 on success or -1 on failure.

**************************************************************************/

#define TBUF_SIZE     16


static int Read_params (int *params)
{
    char tmp[TBUF_SIZE];
    int i;

    for (i = 0; i < 6; i++) {
        if (CS_entry ("params", i + 1, TBUF_SIZE, (void *)tmp) > 0) {
            int v;

            if (sscanf (tmp, "%d", &v) == 1)
                params[i] = v;
            else if (strcmp (tmp, "UNU") == 0)
                params[i] = PARAM_UNUSED;
            else if (strcmp (tmp, "ANY") == 0)
                params[i] = PARAM_ANY_VALUE;
            else if (strcmp (tmp, "ALG") == 0)
                params[i] = PARAM_ALG_SET;
            else if (strcmp (tmp, "ALL") == 0)
                params[i] = PARAM_ALL_VALUES;
            else if (strcmp (tmp, "EXS") == 0)
                params[i] = PARAM_ALL_EXISTING;
            else
                return (-1);
        }
        else
            return (-1);
    }
    return (0);
}


/********************************************************************************

    Description: This routine updates the msg header for all msgs.

          Input: hdr         - the msg header
                 msg_code    - the msg code
                 msg_len     - length of the msg
                 number_blks - the number of blocks in the msg

         Output:

         Return: 

 ********************************************************************************/

static void Update_msg_hdr (msg_header *hdr, short msg_code, int msg_len, 
                            short number_blks)
{
   time_t tm, utime;
   char   log_msg [256];
   int    ret;

   tm = time (NULL);

   hdr->msgcode = msg_code;
   hdr->date_of_msg = (short) (RPG_JULIAN_DATE (tm));
   utime = (int) (RPG_TIME_IN_SECONDS (tm));
   Pack_ushorts_with_value( &hdr->time_of_msg_msw, &utime );
   Pack_ushorts_with_value( &hdr->length_of_msg_msw, &msg_len );
   hdr->src_id = (short) MA_get_user_id ();
   hdr->dst_id = (short) 0x208;
   hdr->no_of_blocks = (short) number_blks;

      /* byte swap the msg hdr if needed */

   if (Byte_swap_required == TRUE) {
      if ((ret = SMIA_bswap_output ("msg_header", hdr, sizeof (msg_header))) < 0) {
         sprintf (log_msg, "Byte swapping \"msg_header\" failed (ret: %d)\n", ret);
         MA_printlog(log_msg);
         MA_abort ("Byte swap routine failed\n");
      }
   }

   return;
}


/********************************************************************************

    Description: This routine updates the TCP header and writes it to the
                 socket.

          Input: id       - id of the msg
                 msg_type - msg type being processed
                 msg_len  - length of msg being processed

         Output:

         Return: 

 ********************************************************************************/

static void Write_tcp_hdr (int id, int msg_type, int msg_len, int pvc)
{
   tcp_header hdr;
   char *buf;

   buf = (char *)&hdr;

   hdr.type = htonl (msg_type);
   hdr.id = htonl (id);
   hdr.length = htonl (msg_len);

      /* write the TCP header */
    
   if (SOC_sock_write(pvc, buf, TCP_HDR_LEN) != TCP_HDR_LEN)
      MA_abort("TCP header write failed");

   return;
}

/****************************************************************
                                                                                                                   
   Description:
      Packs 4 bytes pointed to by "value" into 2 unsigned shorts.
      "value" can be of any type.  The address where the 4 bytes
      starting at "value" will be stored starts @ "loc".  
                                                                                                                   
      The Most Significant 2 bytes (MSW)  of value are stored at 
      the byte addressed by "loc", the Least Significant 2 bytes 
      (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where to store value. 
      value - pointer to data value.
                                                                                                                   
   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.
                                                                                                                   
   Notes:

****************************************************************/
static int Pack_ushorts_with_value( void *loc, void *value ){

   unsigned int   fw_value = *((unsigned int *) value);
   unsigned short hw_value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   hw_value = (unsigned short) (fw_value >> 16) & 0xffff;
   *msw = hw_value;

   hw_value = (unsigned short) (fw_value & 0xffff);
   *lsw = hw_value;

   return 0;

/* End of Pack_ushorts_with_value() */
}
