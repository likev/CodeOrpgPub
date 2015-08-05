#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int read_in_data(float Z_data[1124],float V_data[1124],float Zdr_data[1124],float RhoHV_data[1124],float PhiDP_data[1124],
            float SNR_data[1124],float KDP1_data[1124],float STDZ_data[1124],float STDPhiDP_data[1124]) {

FILE *inputfile;
char line[180];

char aString[80];
char name[80];
int num_bin;
int total_num_bin;
int i;
float elev, az;
float KDP2[1124];
float SPW[1124];

/* bin  elev    Z    V    SPW   Zdr   RhoHV   PhiDP   SNR   KDP1   KDP2   STD_Z  STD_PhiDP*/

inputfile = fopen("PreProOut_030514060440.txt", "r");


while(fgets(line, 180, inputfile) != NULL){


  sscanf(line, "%3s", aString);

  if(strcmp(aString, "Az:") == 0){


    /*fprintf(stderr, "%s\n", aString);*/
    sscanf(line,"%3s%f%9s%d",aString, &az, name,&total_num_bin);
    /*fprintf(stderr, "az=%f, %d\n", az,total_num_bin);*/
    
    for(i=0;i<total_num_bin;i++) {
     if(fgets(line, 180, inputfile) != NULL){
      sscanf(line,"%d%f%f%f%f%f%f%f%f%f%f%f%f",&num_bin,&elev,&Z_data[i],&V_data[i],&SPW[i],&Zdr_data[i],
                &RhoHV_data[i],&PhiDP_data[i],&SNR_data[i],&KDP1_data[i],&KDP2[i],&STDZ_data[i],&STDPhiDP_data[i]);
      /*fprintf(stderr, "%d, %f, %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", num_bin,elev,Z_data[i],V_data[i],SPW[i],Zdr_data[i],
                RhoHV_data[i],PhiDP_data[i],SNR_data[i],KDP1_data[i],KDP2[i],STDZ_data[i],STDPhiDP_data[i]);*/
    }
  }
  break;
 }
}
fclose(inputfile);

}


