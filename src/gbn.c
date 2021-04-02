#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>

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

int base;
int window_size;
int seq_num;
int timer_increment = 20;
int B_seq_num;
struct msg message_buffer[2000];
struct pkt resend_buffer[2000];
static int start_buffer_pos;
static int end_buffer_pos;

int Calculate_Checksum(struct pkt *packet)
{
    int checksum = 0;
    checksum += packet->seqnum;
    checksum += packet->acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet->payload[i];
    }
    return checksum;
}

void Send_Packet_From_A(int seq, struct msg message)
{
    struct pkt packet;
    memset(&packet,0,sizeof(packet));
    packet.seqnum = seq;
    //Copy message data to the packet
    memmove(packet.payload, message.data, 20);
    packet.checksum = Calculate_Checksum(&packet);

    tolayer3(0,packet);

    //Starting timer if needed after sending packet
    if (base == seq)
    {
        starttimer(0, timer_increment);
    }

    resend_buffer[seq]  = packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
    if (seq_num < base+window_size)
    {
        printf("A_output: send packet: %s\n", message.data);
        Send_Packet_From_A(seq_num, message);
        seq_num++;
        return;
    }

    //Add message to buffer
    message_buffer[end_buffer_pos] = message;
    end_buffer_pos++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
    if (packet.checksum != Calculate_Checksum(&packet))
    {
        printf("A_input: packet corrupted!! drop.\n");
        return;
    }

    base = packet.acknum + 1;

    while (seq_num < base+window_size && start_buffer_pos < end_buffer_pos)
    {
        Send_Packet_From_A(seq_num, message_buffer[start_buffer_pos]);
        start_buffer_pos++;
        seq_num++;
    }

    if (base == seq_num)
    {
        stoptimer(0);
    }
    else
    {
        starttimer(0, timer_increment);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    starttimer(0, timer_increment);
    printf ("A_timerinterrupt: Resending unACKed packets in the window\n");
    for (int i = base; i < seq_num; i++)
    {
        tolayer3(0, resend_buffer[i]);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    window_size = getwinsize();
    start_buffer_pos = 0;
    end_buffer_pos = 0;
    memset(&message_buffer,0,sizeof(message_buffer));
    base = 0;
    seq_num = 0;
}

void B_SendAcknowledgement(int ACK_seq_num)
{
    struct pkt packet;
    memset(&packet,0,sizeof(packet));
    packet.acknum = ACK_seq_num;
    packet.checksum = Calculate_Checksum(&packet);
    tolayer3(1, packet);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
    if (packet.checksum != Calculate_Checksum(&packet))
    {
        printf("Checksum mismatch at B: Packet corrupted!!\n");
        return;
    }

    //Continuing as the checksum is verified
    //Checking for sequence number
    if (packet.seqnum != B_seq_num)
    {
        printf("Sequence number mismatch at B: Not the expected seq num!!\n");
        B_SendAcknowledgement(B_seq_num-1);
        return;
    }

    //Continuing to send the packet to upper layer as error cases are already considered
    printf("Sending message to upper layer at B: %s\n", packet.payload);
    printf("Sending ACK to A\n");
    B_SendAcknowledgement(B_seq_num);
    tolayer5(1,packet.payload);
    //Changing expected seq num to receive the next packet
    B_seq_num++;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_seq_num = 0;
}
