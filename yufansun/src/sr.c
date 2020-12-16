#include "../include/simulator.h"
#include <stddef.h>
#include <stdio.h>

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

int base; //当前正在等候的seqnum
int exseqnum; //the seq is B side waiting for
int nextseqnum; //窗口中下一个要发送的包裹的seqnum
int seq; //一直在滚动的seqnum，记录下每一个包裹该填的seqnum
struct pkt buffer[1000];
struct pkt Bbuffer[1000];
int head;
int tail; //everyone call A_output will add this packet to buffer
int winsize;
int N;
struct Node {
  int seqnum;
  float end;
  struct Node* next;
  struct Node* tail;
};
float t = 30.0f;

struct Node* list;

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

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
    if (nextseqnum < base + winsize){ //when windows size not full, add to window and send packet to B
    buffer[tail] = makepacket(message, seq, seq);
    tolayer3(0, buffer[tail]);
    printf("Aside: the packet in window and already send to B\n");
    if (tail % 1000 != 0){
      tail = tail + 1;
    }
    else{
      tail = 0;
    }
    if (base == nextseqnum){
      starttimer(0, t);
    }
    if (list -> tail == NULL){
      list = (struct Node*)malloc(sizeof(struct Node));
      list -> seqnum = 0;
      list -> end = get_sim_time() + t;
      list -> next = NULL;
      list -> tail = list;
    }
    else {
      list -> tail -> next = (struct Node*)malloc(sizeof(struct Node)); 
      list -> tail -> next -> seqnum = base;
      list -> tail -> next -> end = get_sim_time() + t;
      list -> tail -> next -> next = NULL;
      list -> tail -> next = list -> tail;
    }
    nextseqnum++;
    seq++;
  }
  else{ // when windows size is full, add this packet to buffer, and will send it later
    buffer[tail] = makepacket(message, seq, seq);
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
  if (check_checksum(packet) == 1 && packet.seqnum >= base && packet.seqnum < base + winsize){
    buffer[packet.acknum].acknum = -1;
    printf("Aside: recive ACK from B and mark this seq is all set\n");
    if (packet.seqnum == base){ //当收到packet为窗口的第一位时
      if (packet.seqnum == list -> seqnum){ //当收到的packet为在timer list里第一个也就是正常开启的计时器时
	stoptimer(0);
	for (struct Node* l = list; l -> next != NULL; l = l -> next){
	  if (buffer[l -> next -> seqnum].acknum != -1){
	    starttimer(0, list -> next -> end - get_sim_time());
	    list -> next -> tail = list -> tail;
	    list = list -> next;
	    break;
	  }
	}
      }
      int transition = base;
      for (int i = transition; i < transition + winsize; i++){ //窗口的移动
	if (buffer[i % 1000].acknum == -1){
	  base++;
	}
	else{
	  break;
	}
      }
      int count = 0;
      for (int a = nextseqnum; a < base + winsize; a++){ //发送新窗口中为发送的分组
	if (buffer[a].seqnum != -1){
	  tolayer3(0, buffer[a]);
	  printf("Aside: already move window and send message from new window which haven't send yet\n");
	}
	if (list -> tail == NULL){
	  list = (struct Node*)malloc(sizeof(struct Node));
	  list -> seqnum = 0;
	  list -> end = get_sim_time() + t;
	  list -> next = NULL;
	  list -> tail = list;
	}
	else {
	  list -> tail -> next = (struct Node*)malloc(sizeof(struct Node)); 
	  list -> tail -> next -> seqnum = base;
	  list -> tail -> next -> end = get_sim_time() + t;
	  list -> tail -> next -> next = NULL;
	  list -> tail -> next = list -> tail;
	}
	count++;
      }
      nextseqnum = nextseqnum + count;
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  for (int i = base; i < base + winsize; i++){
    if (buffer[i].acknum != -1){
      tolayer3(0, buffer[i]);
      printf("Aside: resend packet to B\n");
      if (list -> tail == NULL){
	list = (struct Node*)malloc(sizeof(struct Node));
	list -> seqnum = 0;
	list -> end = get_sim_time() + t;
	list -> next = NULL;
	list -> tail = list;
      }
      else {
	list -> tail -> next = (struct Node*)malloc(sizeof(struct Node)); 
	list -> tail -> next -> seqnum = base;
	list -> tail -> next -> end = get_sim_time() + t;
	list -> tail -> next -> next = NULL;
	list -> tail -> next = list -> tail;
      }
      for (struct Node* l = list; l -> next != NULL; l = l -> next){
	if (buffer[l -> next -> seqnum].acknum != -1){
	  starttimer(0, list -> next -> end - get_sim_time());
	  list -> next -> tail = list -> tail;
	  list = list -> next;
	  break;
	}
      }
    }
    break;
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
  winsize = getwinsize();
  list = (struct Node*)malloc(sizeof(struct Node));
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  struct msg message;
  if (check_checksum(packet) == 1 && packet.seqnum >= exseqnum && packet.seqnum < exseqnum + N){
    for (int a = 0; a < 20; a++){
      message.data[a] = packet.payload[a];
    }
    Bbuffer[packet.seqnum] = makepacket(message, packet.seqnum, packet.seqnum);
    tolayer3(1, packet);
    printf("Bside: check everything good and send back ACKpacket to A\n");
    if (packet.seqnum == exseqnum){
      for (int i = exseqnum; i < exseqnum + N; i++){
	if (Bbuffer[i % 1000].seqnum != -1){
	  tolayer5(1, Bbuffer[i % 1000].payload);
	  printf("Bside: everything check right and send message to layer5\n");
	  exseqnum++;
	}
	else {
	  break;
	}
      }
    }
  }
  //  if (check_checksum(packet) == 1 && packet.seqnum >= exseqnum - N && packet.seqnum < exseqnum){
  // tolayer3(1, packet);
  // printf("Bside: The message is in previous window and send back to A\n");
  // }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  for (int i = 0; i < 1000; i++){
    Bbuffer[i].seqnum = -1;
  }
  exseqnum = 0;
  N = getwinsize();
}
