#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

int B_seq_num;
struct msg message_buffer[2000];
static int start_buffer_pos;
static int end_buffer_pos;

struct Sender
{
    bool ready_to_send;
    int seq;
    float estimated_rtt;
    struct pkt last_packet;
}  A;

int Calculate_Checksum(struct pkt *packet);

void Send_Packet_From_A(struct msg message)
{
    struct pkt packet;
    memset(&packet,0,sizeof(packet));
    packet.seqnum = A.seq;
    //Copy message data to the packet
    memmove(packet.payload, message.data, 20);
    packet.checksum = Calculate_Checksum(&packet);
    A.last_packet = packet;
    A.ready_to_send = false;
    tolayer3(0, packet);
    //Starting timer after sending packet
    starttimer(0, A.estimated_rtt);
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
    if (A.ready_to_send != true)
    {
        printf("A_output: still waiting for ACK. Add message to buffer: %s\n", message.data);
        message_buffer[end_buffer_pos] = message;
        end_buffer_pos++;
        return;
    }
    printf("A_output: send packet: %s\n", message.data);
    Send_Packet_From_A(message);
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
    if (A.ready_to_send != false)
    {
        printf("A_input: Unidirectional only!! drop.\n");
        return;
    }
    if (packet.checksum != Calculate_Checksum(&packet))
    {
        printf("A_input: packet corrupted!! drop.\n");
        return;
    }

    if (packet.acknum != A.seq)
    {
        printf("A_input: not the expected ACK!! drop.\n");
        return;
    }
    printf("A_input: ACK received.\n");
    stoptimer(0);
    A.seq = (A.seq == 1) ? 0 : 1;
    //Check if there's something in the buffer to send
    if (start_buffer_pos < end_buffer_pos)
    {
        Send_Packet_From_A(message_buffer[start_buffer_pos]);
        start_buffer_pos++;
    }
    else
    {
        A.ready_to_send = true;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    if (A.ready_to_send == true)
    {
        printf("A_timerinterrupt: A not waiting ACK. ignore event.\n");
        return;
    }
    printf("A_timerinterrupt: resend last packet: %s.\n", A.last_packet.payload);
    tolayer3(0, A.last_packet);
    starttimer(0, A.estimated_rtt);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    A.ready_to_send = true;
    A.seq = 0;
    A.estimated_rtt = 15;
    memset(&message_buffer,0,sizeof(message_buffer));
    start_buffer_pos = 0;
    end_buffer_pos = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

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

void B_SendAcknowledgement(int ACK_seq_num)
{
    struct pkt packet;
    memset(&packet,0,sizeof(packet));
    packet.acknum = ACK_seq_num;
    packet.checksum = Calculate_Checksum(&packet);
    tolayer3(1, packet);
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
    if (packet.checksum != Calculate_Checksum(&packet))
    {
        printf("Checksum mismatch at B: Packet corrupted!!\n");
        B_SendAcknowledgement((B_seq_num == 1) ? 0 : 1);
        return;
    }

    //Continuing as the checksum is verified
    //Checking for sequence number
    if (packet.seqnum != B_seq_num)
    {
        printf("Sequence number mismatch at B: Not the expected seq num!!\n");
        B_SendAcknowledgement((B_seq_num == 1) ? 0 : 1);
        return;
    }

    //Continuing to send the packet to upper layer as error cases are already considered
    printf("Sending message to upper layer at B: %s\n", packet.payload);
    printf("Sending ACK to A\n");
    B_SendAcknowledgement(B_seq_num);
    tolayer5(1, packet.payload);
    //Changing expected seq num to receive the next packet
    B_seq_num = (B_seq_num == 1) ? 0 : 1;
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_seq_num = 0;
}
