#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
#define SEQNUM 0;
#define ACK 1;
#define NAK 0;
/*buffer的运用：array of pkt， 然后一个数字记录头的位置，一个数字记录尾巴的位置，每次call A_output时尾巴移一位，每次确认到头的ACK时就移一位头的位置*/

int seq = 0; /*正在传输的packet的seqnum*/
int SEQ = 0; /*会随着buffer的增加一直在走的seqnum*/
int seqB = 0;
struct pkt buffer[1000];
int processing = 0; /*1 neans there is a measage sending and reciving, 0 means no meassage are processing*/
int head = 0;
int tail = 0;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  /*everytime send packet to B we set acknum to 1 so that each time if they check correct they can direct sand back this packet*/
  struct pkt packet;
  strcpy(packet.payload, message.data);
  packet.seqnum = SEQ;
  packet.acknum = ACK;
  int checksum = 0;
  checksum = checksum + ACK + SEQ;
  for (int i = 0; i < 20; i++){
    checksum = checksum + message.data[i];
  }
  packet.checksum = checksum;
  if (processing == 0){
    processing = 1;
    buffer[tail].seqnum = SEQ;
    buffer[tail].acknum = ACK;
    buffer[tail].checksum = checksum;
    strcpy(buffer[tail].payload, message.data);
    if (tail + 1 < 1000){
      tail = tail + 1;
    }
    else {
      tail = 0;
    }
    tolayer3(0,packet);
    printf("success send packet to B\n");
    seq = packet.seqnum;
    /*开始timer*/
    starttimer(0,30.0f);
  }
  else {
    buffer[tail].seqnum = SEQ;
    buffer[tail].acknum = ACK;
    buffer[tail].checksum = checksum;
    strcpy(buffer[tail].payload, message.data);
    if (tail + 1 < 1000){
      tail = tail + 1;
    }
    else {
      tail = 0;
    }
  }
  SEQ = (SEQ + 1) % 2;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{ 
  struct msg message;
  int seqnum = packet.seqnum;
  int acknum = packet.acknum;
  strcpy(message.data, packet.payload);
  int checksum = packet.checksum;
  /*calculate the checksum*/
  int sum = 0;
  sum = sum + seqnum + acknum;
  for (int i = 0; i < 20; i++){
    sum = sum + message.data[i];
  }
  if (seqnum == seq && sum == checksum && acknum == 1){
    stoptimer(0);
    if (head + 1 < 1000){
      head = head + 1;
    }
    else {
      head = 0;
    }
    if (buffer[head].seqnum != -1){
      tolayer3(0, buffer[head]);
      starttimer(0,30.0f);
      seq = buffer[head].seqnum;
      printf("recive ACK from B and send next packet in buffer to B\n");
    }
    else {
      processing = 0;
    }
  }
  if (seqnum == seq && sum == checksum && acknum == 0){
    stoptimer(0);
    tolayer3(0, buffer[head]);
    starttimer(0,30.0f);
    printf("recive NAK from B and resend this packet to B\n");
  }
}
/* called when A's timer goes off */
void A_timerinterrupt()
{
    tolayer3(0, buffer[head]);
    starttimer(0,30.0f);
    printf("timeout! resend packet to B\n");
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
/*modify all buffer seq set them to -1, so that can figure out which position is empty in A_input*/
  seq = 0;
  SEQ = 0;
  processing = 0;
  head = 0;
  tail =0;
  for (int i = 0; i < 1000; i++){
    buffer[i].seqnum = -1;
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  struct msg message;
  int seqnum = packet.seqnum;
  int acknum = packet.acknum;
  strcpy(message.data, packet.payload);
  int checksum = packet.checksum;
  if (seqnum == seq){
    int sum = 0;
    sum = sum + seqnum + acknum;
    for (int i = 0; i < 20; i++){
      sum = sum + message.data[i];
    }
    /*Check whether checksum is the sum of above three number*/
    /*If sum and checksum are same send ACK to A, if not send NAK to A*/
    if (sum == checksum){
      tolayer3(1,packet);
      tolayer5(1,message.data);
      printf("succsee recive packet from A and send to layer5\n");
    }
    else {
      struct pkt NAKpacket;
      NAKpacket.seqnum = seqnum;
      NAKpacket.acknum = NAK;
      strcpy(NAKpacket.payload, message.data);
      int nak_checksum = 0;
      nak_checksum = nak_checksum + seqnum + NAK;
      for (int i = 0; i < 20; i++){
	nak_checksum = nak_checksum + message.data[i];
      }
      NAKpacket.checksum = nak_checksum;
      tolayer3(1,NAKpacket);
      printf("recive the packet from A but the check sum is wrong, so sendback a NAK packet\n");
    }
  }
  /* when waiting for seq 1 but recive seq 0*/
  else{
    tolayer3(1,packet);
    printf("recive packet from A, but the wrong seq number, sendback this packet to A\n");
  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  seqB = 0;
}
