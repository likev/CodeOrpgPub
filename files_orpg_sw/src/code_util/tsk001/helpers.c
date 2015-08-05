/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:14:47 $
 * $Id: helpers.c,v 1.8 2009/08/19 15:14:47 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
/****************************************************************************************
Module:     helpers.c
Description:    General support routines for error handling and pretty printing.

Author:     Bradford T. Ulery, Mitretek Systems

*****************************************************************************************/

#include "helpers.h"

#define SECS_PER_DAY    86400


/****************************************************************************************
Function:   _88D_orpg_date
Description:    Get ORPG date from Unix time_t.
Returns:    ORPG date (days since 1/1/70, plus 1)
Globals:    none
*****************************************************************************************/
short _88D_orpg_date (
    time_t  seconds)            /* time_t (seconds since 1/1/70)    */
{
    return (seconds/SECS_PER_DAY + 1);
}


/****************************************************************************************
Function:   _88D_orpg_time
Description:    Get ORPG "time" (milliseconds past midnight) from Unix time_t.
Returns:    ORPG "time" (milliseconds past midnight)
Globals:    none
*****************************************************************************************/
short _88D_orpg_time (
    time_t  seconds)            /* seconds since 1/1/70         */
{
    return (seconds % SECS_PER_DAY * 1000);
}


/****************************************************************************************
Function:   _88D_unix_time
Description:    Convert ORPG date and time to Unix time_t.
Returns:    Seconds since 1/1/70
Globals:    none
*****************************************************************************************/
time_t _88D_unix_time (
    short   date,               /* days since January 1, 1970       */
                            /* (1/1/70 is represented as 1, not 0)  */
    int     time)               /* milliseconds past midnight       */
{
    return ((time_t) ((date-1)*SECS_PER_DAY + time/1000));
}


/************************************************************************************
Function:   _88D_calendar_date
Description:    Convert modified julian date to day, month, and year integer values.
Returns:    day, month, year (via parameters)
Globals:    none
Notes:      calendar_date was copied directly from ~/src/cpc001/tsk001/decode_prod.c
*************************************************************************************/

void calendar_date (
    short   date,               /* days since January 1, 1970       */
    int     *dd,                /* OUT:  day                */
    int     *dm,                /* OUT:  month              */
    int     *dy )               /* OUT:  year               */
{

    int         l,n, julian;

    /* Convert modified julian to type integer */
    julian = date;

    /* Convert modified julian to year/month/day */
    julian += 2440587;   /*  convert to plain Julian */
    l = julian + 68569;
    n = 4*l/146097;
    l = l -  (146097*n + 3)/4;
    *dy = 4000*(l+1)/1461001;
    l = l - 1461*(*dy)/4 + 31;
    *dm = 80*l/2447;
    *dd= l -2447*(*dm)/80;
    l = *dm/11;
    *dm = *dm+ 2 - 12*l;
    *dy = 100*(n - 49) + *dy + l;
    *dy = *dy - 1900;

    return;
}


/******************************************************************************
Function:  julian_date
Description:  Converts day, month, and year to modified julian date
Returns: modified julian date (via parameters)
Notes: from RPGCS_date_to_julian() in ~/src/cpc101/lib004/rpgcs_time_functs.c */
/******************************************************************************/
void julian_date( 
    int year, 
    int month, 
    int day, 
    int *julian_date )     /* OUT: days since January 1, 1970 */
{

   *julian_date = ( 1461*(year + 4800 + (month - 14)/12))/4 +
                  ( 367*(month - 2 - 12 * ( (month - 14)/12 )))/12 -
                  ( 3*((year + 4900 + (month - 14)/12 )/100))/4 +
                    day - 32075;

   /* Must subtract base year to convert from Julian date to Modified
      Julian date. */
   *julian_date -= 2440587;

/* End of RPGCS_date_to_julian() */
}



/****************************************************************************************
Function:   _88D_LatLon_to_DdotD
Description:    Given an angle (e.g., a latitude or longitude) in thousandths of degrees,
        return a string representation as DDD.DDD (decimal degrees).
Returns:    angle formatted as a string (DDD.DDD)
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_LatLon_to_DdotD (
    int     LatLon)             /* Angle in degrees         */
{
    static char     LatLon_str[10];

    (void) sprintf(LatLon_str, "%3.3f\"", LatLon / 1000.0);
    return(LatLon_str);
}


/****************************************************************************************
Function:   _88D_LatLon_to_DDDMMSS
Description:    Given an angle (e.g., a latitude or longitude) in thousandths of degrees,
        return a string representation as DDD MM' SS" (degrees, minutes, seconds).
Returns:    angle formatted as a string (DDD MM' SS")
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_LatLon_to_DDDMMSS (
    int     LatLon)             /* Angle in degrees         */
{
    int         degrees;
    int         minutes;
    int         seconds;
    int         remainder;
    static char     LatLon_str[20];

    degrees = LatLon / 1000;
    remainder   = LatLon % 1000;

    minutes = remainder / 60;
    remainder   = remainder % 60;

    seconds = remainder / 60;

    (void) sprintf(LatLon_str, "%3d %2d' %2d\"", degrees, minutes, seconds);
    return(LatLon_str);
}


/****************************************************************************************
Function:   _88D_msecs_to_string
Description:    Given time in milliseconds since midnight, return HH:MM:SS.SSS string.
Returns:    time formatted as a string
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_msecs_to_string (
    int     time)               /* milliseconds since midnight      */
{
    int         h, m, s, frac;
    static char     stime[20];

    frac =  time - 1000*(time/1000);
    time =  time/1000;
    h =     time/3600;
    time =  time - h*3600;
    m =     time/60;
    s =     time - m*60;

    (void) sprintf(stime, "%2d:%02d:%02d.%03d", h, m, s, frac);
    return(stime);
}


/****************************************************************************************
Function:   _88D_secs_to_string
Description:    Given time in seconds since midnight, return HH:MM:SS string.
Returns:    time formatted as a string
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_secs_to_string (
    int     time)               /* seconds since midnight       */
{
    int         h, m, s;
    static char     stime[20];

    h =     time/3600;
    time =  time - h*3600;
    m =     time/60;
    s =     time - m*60;

    (void) sprintf(stime, "%02d:%02d:%02d", h, m, s);
    return(stime);
}


/****************************************************************************************
Function:   _88D_date_to_string
Description:    Format modified julian date -- return as string (Month DD, YYYY).
Returns:    formatted date string (Month DD, YYYY)
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_date_to_string (
    short   date)               /* days since January 1, 1970       */
{
    int         dd, dm, dy;
    char        *month = "UNDEFINED";
    int         year;
    static char     sdate[30];

    calendar_date( date, &dd, &dm, &dy );
    switch (dm) {
    case 1:     month = "January";  break;
    case 2:     month = "February"; break;
    case 3:     month = "March";    break;
    case 4:     month = "April";    break;
    case 5:     month = "May";      break;
    case 6:     month = "June";     break;
    case 7:     month = "July";     break;
    case 8:     month = "August";   break;
    case 9:     month = "September";    break;
    case 10:    month = "October";  break;
    case 11:    month = "November"; break;
    case 12:    month = "December"; break;
    default:    (void) sprintf(sdate, "** %02d/%02d/%02d **\n", dm, dd, dy);
    }
    year = 1900 + dy;
    (void) sprintf(sdate, "%s %d, %d", month, dd, year);
    return(sdate);
}



/****************************************************************************************
Function:   _88D_errno_to_string
Description:    Convert errno to a string.
Returns:    string mnemonic for the errno
Globals:    none
Notes:      
*****************************************************************************************/
char *_88D_errno_to_string (
    int     local_errno)            /* errno, defined by POSIX [see errno.h]*/
{
    switch (local_errno) {
    case EACCES:        return("EACCES");
    case EINTR:     return("EINTR");
    case EISDIR:        return("EISDIR");
    case ELOOP:     return("ELOOP");
    case EMFILE:        return("EMFILE");
    case ENAMETOOLONG:  return("ENAMETOOLONG");
    case ENFILE:        return("ENFILE");
    case ENOENT:        return("ENOENT");
    case ENOSPC:        return("ENOSPC");
    case ENOTDIR:       return("ENOTDIR");
    case ENXIO:     return("ENXIO");
    case EOVERFLOW:     return("EOVERFLOW");
    case EROFS:     return("EROFS");
    case EINVAL:        return("EINVAL");
    case ENOMEM:        return("ENOMEM");
    case ETXTBSY:       return("ETXTBSY");
    default:        fprintf(stderr, "errno = %d", local_errno);
                return("** UNEXPECTED VALUE FOR ERRNO **");
    }
}

    /*  DESCRIPTION OF RADIAL STATUS FLAG VARIABLE                  */
    /*  NAMES: INCLUDING RADIAL EVALUATION (GOOD OR BAD)                */
    /*                                      */
    /*  GOODBEL - GOOD BEGINNING OF ELEVATION CUT               */
    /*  BADBEL - BAD BEGINNING OF ELEVATION CUT                 */
    /*  GOODBVOL - GOOD BEGINNING OF VOLUME SCAN                */
    /*  BADBVOL - BAD BEGINNING OF VOLUME SCAN                  */
    /*  GENDEL - GOOD END OF ELEVATION CUT                  */
    /*  BENDEL - BAD END OF ELEVATION CUT                   */
    /*  GENDVOL - GOOD END OF VOLUME SCAN                   */
    /*  BENDVOL - BAD END OF VOLUME SCAN                    */
    /*  GOODINT - GOOD INTERMEDIATE RADIAL                  */
    /*  BADINT - BAD INTERMEDIATE RADIAL                    */
    /*  PGENDEL - PSUEDO GOOD END OF ELEVATION CUT              */
    /*  PGENDVOL - PSUEDO GOOD END OF VOLUME SCAN               */
    /*  PBENDEL - PSUEDO BAD END OF ELEVATION CUT               */
    /*  PBENDVOL - PSUEDO BAD END OF VOLUME SCAN                */
    /*  GOODTHRLO - VALUE OF LOWEST GOOD STATUS                 */
    /*  GOODTHRHI - VALUE OF HIGHEST GOOD STATUS                */


#include <a309.h>
/****************************************************************************************
Function:   _88D_elev_print_status
Description:    Convert status to a string.
Returns:    string mnemonic for the status
Globals:    none
Notes:      
*****************************************************************************************/
void _88D_elev_print_status (
    FILE    *fp,
    int     status)
{
    switch (status) {
    case GOODBEL:   fprintf(fp, "GOODBEL");     break;
    case BADBEL:    fprintf(fp, "BADBEL");      break;
    case GOODBVOL:  fprintf(fp, "GOODBVOL");    break;
    case BADBVOL:   fprintf(fp, "BADBVOL");     break;
    case GENDEL:    fprintf(fp, "GENDEL");      break;
    case BENDEL:    fprintf(fp, "BENDEL");      break;
    case GENDVOL:   fprintf(fp, "GENDVOL");     break;
    case BENDVOL:   fprintf(fp, "BENDVOL");     break;
    case GOODINT:   fprintf(fp, "GOODINT");     break;
    case BADINT:    fprintf(fp, "BADINT");      break;
    case PGENDEL:   fprintf(fp, "PGENDEL");     break;
    case PGENDVOL:  fprintf(fp, "PGENDVOL");    break;
    case PBENDEL:   fprintf(fp, "PBENDEL");     break;
    case PBENDVOL:  fprintf(fp, "PBENDVOL");    break;
    default:    fprintf(fp, "*** UNEXPECTED VALUE (%d) ***\n", status);
    }
}


    /*  DESCRIPTION OF RADIAL SATUS FLAG VARIABLE               */
    /*  NAMES: STATUS (I.E. SEQUENCE IN SCAN) ONLY              */
    /*                                      */
    /*  BEG_ELEV - BEGINNING OF ELEVATION SCAN                  */
    /*  INT_ELEV - WITHIN ELEVATION SCAN                    */
    /*  END_ELEV - END OF ELEVATION SCAN                    */
    /*  BEG_VOL - BEGINNING OF VOLUME SCAN                  */
    /*  END_VOL - END OF VOLUME SCAN                        */
    /*  PSEND_ELEV - PSEUDO END OF ELEVATION SCAN               */
    /*  PSEND_VOL   - PSEUDO END OF VOLUME SCAN             */


/****************************************************************************************
Function:   _88D_rad_print_status
Description:    Convert status to a string.
Returns:    string mnemonic for the status
Globals:    none
Notes:      
*****************************************************************************************/
void _88D_rad_print_status (
    FILE    *fp,
    int status)
{
    switch (status) {
    case BEG_ELEV:      fprintf(fp, "BEG_ELEV");    break;
    case INT_ELEV:      fprintf(fp, "INT_ELEV");    break;
    case END_ELEV:      fprintf(fp, "END_ELEV");    break;
    case BEG_VOL:       fprintf(fp, "BEG_VOL");     break;
    case END_VOL:       fprintf(fp, "END_VOL");     break;
    case PSEND_ELEV:    fprintf(fp, "PSEND_ELEV");  break;
    case PSEND_VOL:     fprintf(fp, "PSEND_VOL");   break;
    default:        fprintf(fp, "*** UNEXPECTED VALUE (%d) ***\n", status);
    }
}





/* function reads elev_ind from 96 byte pre-ICD header */
short get_elev_ind(char *bufptr, int orpg_build) {

    Prod_header_b5 *hdr5; /* structure Build 5 and earlier */
    Prod_header_b6 *hdr6; /* structure Build 6 and later   */
    
    if(orpg_build >= 6) {
        hdr6=(Prod_header_b6 *) bufptr; 
        return (hdr6->g.elev_ind);    
     } else {
        hdr5=(Prod_header_b5 *) bufptr; 
        return (hdr5->elev_ind);          
     } 
}



/* function to test for a valid icd product */
/* for CVG, generally used to distinguish from an intermediat product */
/* could also be used as a diagnostic */
/* input args must have been swapped to local Endian */
int test_for_icd(short div, short ele, short vol, int silent) {

  int pdb_divider_found=FALSE;
  int elev_ind_found=FALSE;
  int vol_num_found=FALSE;


    if(div == -1)
        pdb_divider_found = TRUE;       
    if((ele >= 0) && (ele <= 25))
        elev_ind_found = TRUE;    
    
    if((vol >= 1) && (vol <= 80))
        vol_num_found = TRUE;

        
    if(pdb_divider_found && elev_ind_found && vol_num_found) {
        
        return TRUE;
/* CVG 9.1 */
/*    }    */
    } else {

        if(silent==FALSE) {
            fprintf(stderr,"ERROR - Product header contents do not match ICD Format.\n");
            if(pdb_divider_found == FALSE)
                 fprintf(stderr,"PDB HW 10 - Block Divider value (%d) is not -1\n",div);
            if(vol_num_found == FALSE)
                 fprintf(stderr,"PDB HW 20 - Volume number (%d) is not 1 - 80.\n",vol);
            if(elev_ind_found == FALSE)
                 fprintf(stderr,"PDB HW 29 - Elevation index (%d) is not 0 - 25.\n",ele);
         } 
         
        return FALSE;
    
    } /* end else */


}




/**********************************************************************/
int _88D_Round ( float r      /* a float number */) {

    if ((double)r >= 0.)
        return ((int)(r + .5));
    else
        return (-(int)((-r) + .5));

/* End of Round() */
}





/* CVG 9.0 - adjust scale factor for CVG screen vice PUP screen dimensions */
/*             and center on larger image. This is specifically for          */
/*             non-geographic products                                       */
void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust)

{
    /* the following may need to be modified for proper display */
    *x_scale_adjust = 640.0 / (float)PUP_WIDTH;
    
    *y_scale_adjust = 640.0 / (float)PUP_HEIGHT;

    *center_x_adjust = screen_width/2 - 640/2;
    *center_y_adjust = screen_height/2 - 640/2;
        

    
} /* end non_geo_screen_scale_adjustment */






/* CVG 9.0 - adjust scale factor for CVG screen vice screen zoom factor, */
/*             product resolution and base product resolution              */
/*             and calculate screen center via screen dimensions and if not   */
/*             centered on the radar via screen zoom factor and offset of the */
/*             screen center. This is specifically for geographic packet that */
/*             can be overlaid.                                               */
void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust)
{
        *center_x_adjust = screen_width/2;  /* initialized to center of canvas */
        *center_y_adjust = screen_height/2; /* initialized to center of canvas */

        /* correct center pixel for the off_center value (if any) */
        *center_x_adjust = *center_x_adjust - 
                                   (int) (screen_x_center * screen_zoom);  
        *center_y_adjust = *center_y_adjust - 
                                   (int) (screen_y_center * screen_zoom); 

        /* This looks kludgy, however the Legacy rescale factor assumed a 0.54 NM*/
        /*   product displayed on a PUP                                          */
        /* In order to be able to display over other product resolutions : */
        
        /* if conconfigured as an overlay and used as an overlay */
        if((product_resolution == 999) &&  (base_resolution != 999)) {
            /* adjust to resolution of base product */       
            *x_scale_adjust = (0.54 / base_resolution) * RESCALE_FACTOR * screen_zoom; 
        } else { /* use the inherent resolution of the packet */ 
            *x_scale_adjust = RESCALE_FACTOR * screen_zoom; 
        }
        
        *y_scale_adjust = -*x_scale_adjust;
        
        
} /* end geo_scale_and_center */




/* reads in from file char by char until we get to the end of the line */
/* we use it mostly to read in titles, which contain spaces */
void read_to_eol(FILE *list_file, char *buf)
{
    int i=0;

    do {
        fread(&(buf[i]), 1, 1, list_file);  /* read in one char */
    } while((buf[i++] != '\n') && 
        ((ferror(list_file)) == 0) && 
        ((feof(list_file)) == 0));

    buf[--i] = '\0';


    if((ferror(list_file)) != 0) {
        fprintf(stderr, "Error reading stream: %d\n", ferror(list_file));
    }

}






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


