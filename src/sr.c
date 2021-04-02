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
int timer_increment = 40;
int B_seq_num;
struct msg message_buffer[3000];
struct pkt resend_buffer[3000];
static int start_buffer_pos;
static int end_buffer_pos;
static float resend_timer[3000];
static int resend_ack[3000];
struct pkt rec_buffer[3000];
static int rec_buffer_valid[3000];

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
    resend_timer[seq] = get_sim_time() + timer_increment;
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

    if(packet.acknum < base || packet.acknum >= base + window_size)
        return;

    resend_timer[packet.acknum]=0;
    resend_ack[packet.acknum]=1;

    if(base==packet.acknum)
    {
        for(int i = base; i < seq_num; i++)
        {
            if(resend_ack[i] != 1)
                break;
            base++;
        }
    }

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
}

float find_min_time()
{
    float  min_time  = get_sim_time() + timer_increment;
    for (int i= base; i <seq_num; i++)
    {
        if((resend_ack[i]==0) && (resend_timer[i]< min_time))
        {
            min_time = resend_timer[i];
        }
    }

    return (min_time);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf ("A_timerinterrupt: Resending unACKed packets in the window\n");
    for (int i = base; i < seq_num; i++)
    {
        if (resend_timer[i] == get_sim_time() && resend_ack[i]==0)
        {
            tolayer3(0, resend_buffer[i]);
            resend_timer[i] = get_sim_time() + timer_increment;
            break;
        }
    }
    if (find_min_time() > 0)
        starttimer(0, find_min_time() - get_sim_time());
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    memset(&message_buffer,0,sizeof(message_buffer));
    memset(&resend_buffer,0,sizeof(resend_buffer));
    memset(&resend_timer,0,sizeof(resend_timer));
    memset(&resend_ack,0,sizeof(resend_ack));

    start_buffer_pos = 0;
    end_buffer_pos = 0;
    base = 0;
    seq_num = 0;
    window_size =getwinsize();
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
    if(packet.seqnum >= B_seq_num + window_size || packet.seqnum < B_seq_num - window_size)
        return;

    B_SendAcknowledgement(packet.seqnum);

    if (packet.seqnum != B_seq_num)
    {
        rec_buffer[packet.seqnum] = packet;
        rec_buffer_valid[packet.seqnum] = 1;
    }
    else
    {
        int n = B_seq_num + window_size;
        tolayer5(1, packet.payload);
        B_seq_num++;

        for (int i = B_seq_num; i < n; i++)
        {
            if (rec_buffer_valid[i] == 0)
                break;

            tolayer5(1, rec_buffer[i].payload);
            B_seq_num++;
        }
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    memset(&rec_buffer , 0, sizeof(rec_buffer));
    memset(&rec_buffer_valid , 0, sizeof(rec_buffer_valid));
    window_size = getwinsize();
    B_seq_num = 0;
}
