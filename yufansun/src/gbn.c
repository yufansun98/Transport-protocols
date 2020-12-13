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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int check_checksum(struct pkt packet);
struct pkt makepacket(struct msg message, int seqnum, int acknum);
//int processing;
int base; //当前正在等候的seqnum
int exseqnum; //the seq is B side waiting for
int nextseqnum; //窗口中下一个要发送的包裹的seqnum
int seq; //一直在滚动的seqnum，记录下每一个包裹该填的seqnum
struct pkt buffer[1000];
int head;
int tail; //everyone call A_output will add this packet to buffer
int winsize;
//int N; 


int check_checksum(struct pkt packet){
  int sum = 0;
  sum = sum + packet.seqnum + packet.acknum;
  for (int i = 0; i < 20; i++){
    sum = sum + packet.payload[i];
  }
  if (sum == packet.checksum){
    return 1;
  }
  else{
    return 0;
  }
}

struct pkt makepacket(struct msg message, int seqnum, int acknum){
  struct pkt packet;
  for (int a = 0; a < 20; a++){
    packet.payload[a] = message.data[a];
  }
  packet.seqnum = seqnum;
  packet.acknum = acknum;
  int checksum = 0;
  checksum = checksum + acknum + seqnum;
  for (int i = 0; i < 20; i++){
    checksum = checksum + packet.payload[i];
  }
  packet.checksum = checksum;
  return packet;
}

//struct msg unpack(struct pkt packet){
//
//}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  //struct pkt packet;
  //packet = makepacket(message, nextseqnum, 1);
  if (nextseqnum < base + winsize){ //when windows size not full, add to window and send packet to B
    //processing = 1;
    buffer[tail] = makepacket(message, seq, 1);
    tolayer3(0, buffer[tail]);
    if (tail % 1000 != 0){
      tail = tail + 1;
    }
    else{
      tail = 0;
    }
    if (base == nextseqnum){
      starttimer(0, 20.0f);
    }
    nextseqnum++;
    //N++;
    seq++;
  }
  else{ // when windows size is full, add this packet to buffer, and will send it later
    buffer[tail] = makepacket(message, seq, 1);
    if (tail % 1000 != 0){
      tail = tail + 1;
    }
    else{
      tail = 0;
    }
    seq++;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  if (packet.seqnum == base && check_checksum(packet) == 1){
    base = packet.seqnum + 1;
    //N--;
    if (base == nextseqnum){
      stoptimer(0);
    }
    else {
      starttimer(0, 20.0f);
      for (int i = nextseqnum; i < base + winsize; i++){
	if (buffer[i].seqnum != -1){
	  tolayer3(0, buffer[i]);
	}
	else {
	  break;
	}
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  starttimer(0, 20.0f);
  for (int i = base; i < nextseqnum; i = (i + 1) % 1000){
    tolayer3(0, buffer[i]);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  base = 0;
  nextseqnum = 0;
  seq = 0;
  for (int i = 0; i < 1000; i++){
    buffer[i].seqnum = -1;
  }
  head = 0;
  tail = 0;
  //N = 0;
  winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  if (check_checksum(packet) == 1 && packet.seqnum == exseqnum){
    tolayer5(1, packet.payload);
    tolayer3(1, packet);
    exseqnum++;
  }
  if (check_checksum(packet) == 1 && packet.seqnum != exseqnum){
    struct pkt lasttime_packet;
    struct msg message;
    for (int a = 0; a < 20; a++){
      message.data[a] = packet.payload[a];
    }
    lasttime_packet = makepacket(message, exseqnum, 1);
    tolayer3(1, lasttime_packet);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  
  exseqnum = 0;
}
