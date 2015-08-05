/*
 * RCS info
 * $Author: cmn $
 * $Date: 2002/06/26 20:49:22 $
 * $Locker:  $
 * $Id: superob_initialize.c,v 1.2 2002/06/26 20:49:22 cmn Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/************************************************************************
 *      Module:  superob_initialize.c                                    *
 *                                                                       *
 *      Description:  This module is used by the SuperOb to initialize   *
 *                      a few arrays to be zearo                         *
 *      Input:        superob: mean wind of each cell			 *
 *                    superob_std: standard deviation of mean wind       *
 *                    superob_encoded: encoded value of mean wind        *
 *		      rangebar: mean radial range of cell                *
 *                    azimuthbar: mean azimuth of cell                   *
 *                    tiltsbar: mean tilt angle of cell                  *
 * 		      timebar:  time deviation from base time            *
 *                    icount:   num of bins considered to form a cell    *
 *                    naz:    num of cell azimuths                       *
 *                    nr:     num of cells along the radial              *
 *                    ntilts: num of elevations                          *
 *     Output:	      same as input with values cleared to zero          *
 *     Return:        none						 *
 *                                                                       *
 ************************************************************************/


void
superob_initialize( 
                   double ***superob, double ***superob_std, double ***superob_square,
                   double ***rangebar,
                   double ***azimuthbar,double ***tiltsbar,double ***timebar,
                   int   ***icount, int naz, int nr, int *ntilts, float elev_table[])
{ 

 /* local variable in this function  */
 int i, j, k;   /*loop index */

 for(i=0;i<naz;i++)
      for(j=0;j<nr;j++)
       for(k=0;k<(*ntilts);k++)
         {
         superob[i][j][k]=0.0;
         superob_std[i][j][k]=0.0;
         superob_square[i][j][k]=0.0;
         rangebar[i][j][k]=0.0;
         azimuthbar[i][j][k]=0.0;
         tiltsbar[i][j][k]=0.0;
         timebar[i][j][k]=0.0;
         icount[i][j][k]=0;
         }
 /* reintialize the elevation table     */
 for(k=0; k< (*ntilts); k++)
  {
   elev_table[k] = -999.0;
  }
 /* reset num of elevation angles to 0   */
  (*ntilts) = 0;

}
