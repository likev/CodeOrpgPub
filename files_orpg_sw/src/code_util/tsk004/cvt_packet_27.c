/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:19:56 $
 * $Id: cvt_packet_27.c,v 1.2 2003/02/06 18:19:56 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
 
/* cvt_packet_27.c */

#include "cvt_packet_27.h"


void packet_27(char *buffer,int *offset) {
  /* display information contained within packet 27 */
  short spare1,spare2;
  int i; /* j; */
  short length_msw,length_lsw;
  int packet_length=0;
  short elev_angle;
  int latitude,longitude;
  int num_packets;
  short avg_vel, height,std,time_dev,avg_az;
  
  printf("Packet 27: SuperOb Wind Data Packet\n");
  length_msw=read_half(buffer,offset);
  length_lsw=read_half(buffer,offset);
  packet_length=((int)(length_msw << 16) | (int)(length_lsw & 0xffff));
  printf("Packet Length = %d bytes or %X hex\n",packet_length,packet_length);
  num_packets=packet_length/18;
  printf("Number of Packets to Decode = %d\n",num_packets);
  elev_angle=read_half(buffer,offset);
  printf("Elevation Angle = %hd\n",elev_angle);
 
  for(i=0;i<num_packets;i++) {
   
    /* read latitude */
    spare1=read_half(buffer,offset);
    spare2=read_half(buffer,offset);
    latitude=((int)(spare1 << 16) | (int)(spare2 & 0xffff));

    /* read longitude */
    spare1=read_half(buffer,offset);
    spare2=read_half(buffer,offset);
    longitude=((int)(spare1 << 16) | (int)(spare2 & 0xffff));

    /* read the height */
    height=read_half(buffer,offset);
    
    /* read the average radial velocity */
    avg_vel=read_half(buffer,offset);
    
    /* read the standard dev of the avg radial velocity */
    std=read_half(buffer,offset);
    
    /* read the time deviation */
    time_dev=read_half(buffer,offset);
    
    /* read the average azimuth */
    avg_az=read_half(buffer,offset);

    printf("Lat: %7.3f  Lon: %7.3f  Hgt: %5hd  Avg Vel: %4hd  STDev: %4hd  TimeDev: %5hd  AvgAZ: %3hd\n",
      latitude/1000.0, longitude/1000.0, height, avg_vel, std, time_dev, avg_az);

    }

  printf("\n");
  printf("Message 27 Complete\n");
  }


