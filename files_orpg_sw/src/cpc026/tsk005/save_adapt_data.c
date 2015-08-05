/**************************************************************************

      Module: save_adapt_data.c

 Description:

        Write Fortran adaptation data into ORPG Adaptation Data Linear
	Buffer (LB) file.

 Changes:

 Assumptions:

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 18:39:22 $
 * $Id: save_adapt_data.c,v 1.6 2005/12/27 18:39:22 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifdef LINUX
#define save_adapt_data_ save_adapt_data__
#define save_adapt_data_gen_ save_adapt_data_gen__
#define process_argv_ process_argv__
#define init_le_services_ init_le_services__
#endif

#ifdef HPUX
#define save_adapt_data_ save_adapt_data
#define save_adapt_data_gen_ save_adapt_data_gen
#define process_argv_ process_argv
#define init_le_services_ init_le_services
#endif

/*
 * System Include Files/Local Include Files
 */
#include <stdio.h>
#include <stdlib.h>            /* getopt()                                */

#include <lb.h>
#include <rpg_port.h>
#include <orpg.h>
#include <rdacnt.h>

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
#define AVERAGE_MSG_SIZE 0	/* average RPG adaptation message size */
#define MAXN_MESSAGES 20	/* maximum number of adaptation messages */
#define ARG_SIZE 64
#define N_ARGS 20
/*
  Define VCP data constants and offsets.
*/
#define PFNHW             0
#define PFELANG           0
#define PFS1AZAN         11
#define PFS2AZAN         15
#define PFS3AZAN         19
#define DEG_TO_BAM        0.0439453f



/*
 * Static Globals
 */
static int Argc = 0;            /* the argc variable */
static char *Argv [N_ARGS];     /* the argv array */
static char Arg_buffer [ARG_SIZE * N_ARGS] = "";
                                /* buffer for argv array */
static int Arg_buf_off = 0;     /* offset of free space in Arg_buffer */
static Rdacnt *Rdacnt_info;	/* RDACNT data structure. */

/* Function Prototypes. */
static int Convert_vcp_to_icd_format( Vcp_struct *vcp );

/****************************************************************

   Description: This function receives the command line arguments
                from the FORTRAN main subbroutine and rebuilds the
                the C Argv list.

   Input:      argv - one argv string
               len - max length of the argv buffer

   Returns:    return value is never used.

*****************************************************************/
int process_argv_ (char *argv, int *len){
  
   int i;
  
   for (i = 0; i < *len; i++) {
      if ((int)argv [i] <= 32) {
         argv [i] = '\0';
         break;
      }
   }
   argv [*len - 1] = '\0';
 
   if (Argc >= N_ARGS || Arg_buf_off + *len >= ARG_SIZE * N_ARGS) {

      LE_send_msg( GL_INFO, "Too many command line options specified\n");
      exit(1) ;

   }
  
   Argv [Argc] = Arg_buffer  + Arg_buf_off;
   strcpy (Argv [Argc], argv);
   Arg_buf_off += strlen (argv) + 1;
   Argc++;

   return (0);

/*END of process_argv_()*/
}

/********************************************************************

   Function initializes the LE services. 

********************************************************************/
int init_le_services_( char *argv ){

   ORPGMISC_init( 1, &argv, 100, 0, -1, 0 );
   return (0);

/* End of init_le_services() */
}

/********************************************************************

	Function writing site specific adaptation data to the LB. 

	It returns 0 on success or -1 on failure through argument 
	"err".

********************************************************************/

int save_adapt_data_ (int *rdacnt_first, int *rdacnt_last, int *err)
{
    int len, ret;
    int vcp_ind;
    Vcp_struct *vcp;

    *err = -1;
    Rdacnt_info = (Rdacnt *) rdacnt_first;

    /* Adjust pointer to point to VCP data. */
    vcp = (Vcp_struct *) Rdacnt_info->rdcvcpta[0];

    /* Verify that the selection VCP is a valid VCP. */
    for( vcp_ind = 0; vcp_ind < VCP_MAXN_VCPS; vcp_ind++ ){

      if( vcp[vcp_ind].vcp_num > 0 )
         Convert_vcp_to_icd_format( &vcp[vcp_ind] );

    }

    /* write adaptation data */
    len = (rdacnt_last - rdacnt_first + 1) * sizeof (int);
    if ((ret = ORPGDA_write (ORPGDAT_ADAPTATION, (char *)rdacnt_first, 
	len, RDACNT)) != len) {
	LE_send_msg( GL_INFO, "Writing RDACNT Failed (%d)\n", ret);
	exit (RDACNT);
    }
    LE_send_msg ( GL_INFO, "RDACNT Adaptation Written (%d bytes)\n", len);

    *err = 0;
    return (0);
}


/********************************************************************

	Function writing generic (site independent) adaptation data 
	to the LB. 

	It returns 0 on success or -1 on failure through argument 
	"err".

********************************************************************/

int save_adapt_data_gen_ (
	int *colrtbl_first, int *colrtbl_last, 
	int *environ_first, int *environ_last, 
	int *err)
{
    int len, ret;


    *err = -1;

    /* write adaptation data */
    len = (colrtbl_last - colrtbl_first + 1) * sizeof (int);
    if ((ret = ORPGDA_write (ORPGDAT_ADAPTATION, (char *)colrtbl_first, 
	len, COLRTBL)) != len) {
	LE_send_msg (GL_INFO, "Writing COLRTBL Failed (%d)\n", ret);
	exit (COLRTBL);
    }
    LE_send_msg (GL_INFO, "COLRTBL Adaptation Written (%d bytes)\n", len);

    /* write adaptation data */
    len = (environ_last - environ_first + 1) * sizeof (int);
    if ((ret = ORPGDA_write (ORPGDAT_ADAPTATION, (char *)environ_first, 
	len, ENVIRON)) != len) {
	LE_send_msg (GL_INFO, "Writing ENVIRON Failed (%d)\n", ret);
	exit (ENVIRON);
    }
    LE_send_msg (GL_INFO, "ENVIRON Adaptation Written (%d bytes)\n", len);

    *err = 0;
    return (0);

}

/***********************************************************************
  
   Description:  
      The vcp elevation and PRF sector azimuths are converted to i
      RPG/RDA ICD format.
  
*******************************************************************/
static int Convert_vcp_to_icd_format( Vcp_struct *vcp ){

   short number_elevations, *rpg_vcp_data;
   int i, j;


   /* Transfer the Elevation Cut information. */
   number_elevations = vcp->n_ele;

   for( i = 0; i < number_elevations; i++ ){

      rpg_vcp_data = (short *) &vcp->vcp_ele[i][0];

      /* For all shorts in elevation cut data. */
      for( j = 0; j < EL_ATTR; j++ ){ 

         if( j == PFELANG ){

            /* Elevation angle data needs to be converted to BAMS. */
            rpg_vcp_data[j] =  (short) (((float) rpg_vcp_data[j]/10) * 
                                         (8.0/DEG_TO_BAM)) + 0.5;
         }

      /* End of "for" loop. */
      }

   }

   return ( 0 );

/* End of Convert_vcp_to_icd_format() */
}
