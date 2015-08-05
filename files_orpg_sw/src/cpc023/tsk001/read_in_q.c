#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int read_in_q(float q_data[1125][6]) {

FILE *inputfile;
char line[180];

char aString[80];
char name[80];
int num_bin;
int total_num_bin;
int i;
float elev, az;
float q0,q1,q2,q3,q4,q5;

inputfile = fopen("SimpleQ_030514060440.txt", "r");

q_data[0][0] = 0.98;

while(fgets(line, 180, inputfile) != NULL){


  sscanf(line, "%3s", aString);

  if(strcmp(aString, "Az:") == 0){


    sscanf(line,"%3s%f%9s%d",aString, &az, name,&total_num_bin);

    for(i=0;i<total_num_bin;i++) {
     if(fgets(line, 180, inputfile) != NULL){
         /*fprintf(stderr, "**%s\n", line); */
      sscanf(line,"%d%f%f%f%f%f%f%f",&num_bin,&elev,&q0,&q1,&q2,&q3,
                &q4, &q5);
      q_data[i][0] = q0;
      q_data[i][1] = q1;
      q_data[i][2] = q2;
      q_data[i][3] = q3;
      q_data[i][4] = q4;
      q_data[i][5] = q5;
    }
  }
  break;
 }
}
fclose(inputfile);

}
