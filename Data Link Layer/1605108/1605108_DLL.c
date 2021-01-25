#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdbool.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
/*struct msg
{
    char data[4];
};
*/
/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    char payload[4];
};

struct frm
{
    int type;
    int seqnum;
    int acknum;
    unsigned long long int checksum;
    char payload[4];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer1(int AorB, struct frm frame);
void tolayer3(int AorB, char data[4]);
//void tolayer1(int AorB, struct frm frame);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define TIME_OUT 15

bool okToSendFromA = true;
bool okToSendFromB = true;
//struct pkt pcktToSend;
char divisor[5]="1101";
struct frm frameToSendAtoB;
struct frm frameToSendBtoA;
int aSequenceNumber = 0;
int bSequenceNumber = 0;
int bAcknowledgeNumber = 1;
int aAcknowledgeNumber = 1;
int outStandingACKa = 0;
int outStandingACKb = 0;
bool enablePiggyBack;
char a[80];
char msg[80];
bool enableCRC;

unsigned long long int convertToDec(char arr[])
{
    unsigned long long int number = 0;
    int size = strlen(arr);
    int i = 0;
    while(i<size)
    {
        number = number << 1;
        if (arr[i] == '1')
        {
            number = number | 1;
        }
        i++;
    }
    return number;
}

char* convertToBinary(unsigned long long int number)
{
    int i=63;
    while(i>=0)
    {
        a[i] = (number & 1) ? '1' : '0';
        number = number >> 1;
        i--;
    }
    return a;
}

char* createMsg(char data[],long long int ackNum,long long int seqNum)
{
    int k=0;
    for(int i=0; i<strlen(data); i++)
    {
        char *msgch = convertToBinary((int)data[i]);
        int l = 56;
        while(l<64)
        {
            msg[k] = msgch[l];
            k++;
            l++;
        }
    }
    char *ackch = convertToBinary(ackNum);
    char *seqch = convertToBinary(seqNum);
    int i = 54;
    while(i<64)
    {
        msg[k] = ackch[i];
        i++;
        k++;
    }
    int j = 54;
    while(j<64)
    {
        msg[k] = ackch[j];
        j++;
        k++;
    }
    msg[k]=0;
    return msg;
}

char* CalcCRC(char* data, char* generator)
{
    unsigned long long int word = convertToDec(data);
    unsigned long long int dividendVal = word << (strlen(generator)-1);
    unsigned long long int divisorVal = convertToDec(generator);

    int shiftAmount = (int) ceill(log2l(dividendVal+1)) - strlen(generator);

    unsigned long long int remndr,value=1;

    while ((shiftAmount >= 0) || (dividendVal >= divisorVal))
    {
        unsigned long long int temp1 = (dividendVal >> shiftAmount);
        remndr = (temp1^divisorVal);
        unsigned long long int temp2 = (remndr << shiftAmount);
        dividendVal = ((dividendVal & ((value << shiftAmount) - value)) |temp2 );
        shiftAmount = (int) ceill(log2l(dividendVal + 1)) - strlen(generator);
    }

    unsigned long long int codedWord = ((word << (strlen(generator) - 1)) | (dividendVal));

    return convertToBinary(dividendVal);

}

bool isCorrupt(unsigned long long int codedDec)
{
    return (codedDec == 0) ? 0 : 1;
}

/*int CalcCheckSum(struct frm frame)
{
    int value = 0;
    int i;
    value = value + frame.acknum;
    value = value + frame.seqnum;

    for(i = 0; i<4; ++i)
    {
        value = value + frame.payload[i];
    }
    return value;
}*/

struct frm ackGen(int ackNum)
{
    struct frm newFrame;

    newFrame.type = 1;
    newFrame.seqnum = 0;
    newFrame.acknum = ackNum;
    /* char fillup[4];
     for(int i = 0; i<4; i++)
     {
         fillup[i] = '0';
     }
     strcpy(newFrame.payload,fillup);
     newFrame.checksum = CalcCheckSum(newFrame);*/

    return newFrame;
}
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct pkt packet)
{
    if(!okToSendFromA)
    {
        return;
    }
    okToSendFromA = false;

    frameToSendAtoB.type = 0;
    frameToSendAtoB.acknum = 0;
    strcpy(frameToSendAtoB.payload,packet.payload);
    frameToSendAtoB.seqnum = aSequenceNumber;
    //frameToSendAtoB.checksum = CalcCheckSum(frameToSendAtoB);


    if(outStandingACKa == 1)
    {
        frameToSendAtoB.acknum = aAcknowledgeNumber;
        frameToSendAtoB.type = 2;
        outStandingACKa = 0;
    }


    char* dataA = createMsg(frameToSendAtoB.payload,frameToSendAtoB.acknum,frameToSendAtoB.seqnum);

    char* codedMsgA = CalcCRC(dataA,divisor);

    unsigned long long int chcksm = convertToDec(codedMsgA);

    frameToSendAtoB.checksum = chcksm;

    tolayer1(0,frameToSendAtoB);

    starttimer(0,TIME_OUT);

    printf(" A -->  Sending Message : %s, A_output : type = %d\n",packet.payload,frameToSendAtoB.type);

    if(enableCRC)
    {
        printf(" Remainder Sent From Side A to Check : %lld\n",frameToSendAtoB.checksum);
    }
}

/* need be completed only for extra credit */
void B_output(struct pkt packet)
{
    if(!okToSendFromB)
    {
        return;
    }
    okToSendFromB = false;

    frameToSendBtoA.type = 0;
    frameToSendBtoA.acknum = 0;
    strcpy(frameToSendBtoA.payload,packet.payload);
    frameToSendBtoA.seqnum = bSequenceNumber;
    //frameToSendBtoA.checksum = CalcCheckSum(frameToSendBtoA);


    if(outStandingACKb == 1)
    {
        frameToSendBtoA.acknum = bAcknowledgeNumber;
        frameToSendBtoA.type = 2;
        outStandingACKb = 0;
    }


    char* dataB = createMsg(frameToSendBtoA.payload,frameToSendBtoA.acknum,frameToSendBtoA.seqnum);

    char* codedMsgB = CalcCRC(dataB,divisor);

    unsigned long long int chcksm = convertToDec(codedMsgB);

    frameToSendBtoA.checksum = chcksm;

    tolayer1(1,frameToSendBtoA);
    starttimer(1,TIME_OUT);

    printf(" B -->  Sending Message : %s, B_output : type = %d\n",packet.payload,frameToSendBtoA.type);

    if(enableCRC)
    {
        printf(" Remainder Sent From Side B to Check : %lld\n",frameToSendBtoA.checksum);
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct frm frame)
{
    if(frame.type == 1)
    {
        int length = strlen(divisor) - 1;
        char* dataAin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataAin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordA = (msgValue << length |valueToAppend);
        char* codeWordAinBin = convertToBinary(codeWordA);
        char* codedWordAin = CalcCRC(codeWordAinBin,divisor);

        if(aSequenceNumber == frame.acknum && !isCorrupt(convertToDec(codedWordAin)))
        {
            printf(" Valid Acknowledgment Frame received by A\n");
            okToSendFromA = true;
            aSequenceNumber = (1+aSequenceNumber)%2;
            stoptimer(0);
        }
        else
        {
            printf(" Corrupt Acknowledgment Frame received by A\n");
        }

        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideA = convertToDec(codedWordAin);
            printf(" Remainder Received on Side A to Verify : %lld\n", remndrRcvOnSideA);
        }
    }
    else if(frame.type == 0)
    {
        int length = strlen(divisor) - 1;
        char* dataAin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataAin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordA = (msgValue << length |valueToAppend);
        char* codeWordAinBin = convertToBinary(codeWordA);
        char* codedWordAin = CalcCRC(codeWordAinBin,divisor);


        bool expOutput = false;
        if((1-aAcknowledgeNumber) == frame.seqnum)
        {
            expOutput = true;
        }

        if(expOutput && !isCorrupt(convertToDec(codedWordAin)))
        {
            printf(" Valid Frame received by A, message : %s \n", frame.payload);
            aAcknowledgeNumber = (1+aAcknowledgeNumber)%2;
            tolayer3(0,frame.payload);
            if(enablePiggyBack)
            {
                outStandingACKa = 1;
            }
        }
        else
        {
            if(isCorrupt(convertToDec(codedWordAin)))
            {
                printf(" Corrupted Frame received by A, message : %s \n", frame.payload);
            }
            else
            {
                printf(" Repeated Frame received by A, message : %s \n", frame.payload);
                if(enablePiggyBack)
                {
                    struct frm ackFromAtoB = ackGen(aAcknowledgeNumber);
                    strcpy(ackFromAtoB.payload,frame.payload);
                    char* dataA = createMsg(ackFromAtoB.payload,ackFromAtoB.acknum,ackFromAtoB.seqnum);
                    char* codeA = CalcCRC(dataA,divisor);
                    ackFromAtoB.checksum = convertToDec(codeA);
                    if(enableCRC)
                    {
                        printf(" Remainder Sent From Side A to Check : %lld\n",ackFromAtoB.checksum);
                    }
                    outStandingACKa = 0;
                    tolayer1(0,ackFromAtoB);
                }
            }
        }

        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideA = convertToDec(codedWordAin);
            printf(" Remainder Received on Side A to Verify : %lld\n", remndrRcvOnSideA);
        }

        if(!enablePiggyBack)
        {
            struct frm ackFromAtoB = ackGen(aAcknowledgeNumber);
            strcpy(ackFromAtoB.payload,frame.payload);
            char* dataA = createMsg(ackFromAtoB.payload,ackFromAtoB.acknum,ackFromAtoB.seqnum);
            char* codeA = CalcCRC(dataA,divisor);
            ackFromAtoB.checksum = convertToDec(codeA);
            if(enableCRC)
            {
                printf(" Remainder Sent From Side A to Check : %lld\n",ackFromAtoB.checksum);
            }
            tolayer1(0,ackFromAtoB);
        }
    }
    else if(frame.type == 2)
    {
        int length = strlen(divisor) - 1;
        char* dataAin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataAin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordA = (msgValue << length |valueToAppend);
        char* codeWordAinBin = convertToBinary(codeWordA);
        char* codedWordAin = CalcCRC(codeWordAinBin,divisor);

        if(aSequenceNumber == frame.acknum && !isCorrupt(convertToDec(codedWordAin)))
        {
            printf(" Valid Acknowledgment Frame received by A\n");
            okToSendFromA = true;
            aSequenceNumber = (1+aSequenceNumber)%2;
            stoptimer(0);
        }
        else
        {
            printf(" Corrupt Acknowledgment Frame received by A\n");
        }

        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideA = convertToDec(codedWordAin);
            printf(" Remainder Received on Side A to Verify : %lld\n", remndrRcvOnSideA);
        }

        bool expOutput = false;
        if((1-aAcknowledgeNumber) == frame.seqnum)
        {
            expOutput = true;
        }

        if(expOutput && !isCorrupt(convertToDec(codedWordAin)))
        {
            printf(" Valid Frame received by A, message : %s \n", frame.payload);
            aAcknowledgeNumber = (1+aAcknowledgeNumber)%2;
            tolayer3(0,frame.payload);
            outStandingACKa = 1;
        }
        else
        {
            if(isCorrupt(convertToDec(codedWordAin)))
            {
                printf(" Corrupted Frame received by A, message : %s \n", frame.payload);
            }
            else
            {
                printf(" Repeated Frame received by A, message : %s \n", frame.payload);
                struct frm ackFromAtoB = ackGen(aAcknowledgeNumber);
                strcpy(ackFromAtoB.payload,frame.payload);
                char* dataA = createMsg(ackFromAtoB.payload,ackFromAtoB.acknum,ackFromAtoB.seqnum);
                char* codeA = CalcCRC(dataA,divisor);
                ackFromAtoB.checksum = convertToDec(codeA);
                if(enableCRC)
                {
                    printf(" Remainder Sent From Side A to Check : %lld\n",ackFromAtoB.checksum);
                }
                outStandingACKa = 0;
                tolayer1(0,ackFromAtoB);
            }
        }
    }
}
/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    printf(" A_timerinterrupt : Sending message again : %s \n", frameToSendAtoB.payload);
    frameToSendAtoB.type = 0;
    tolayer1(0,frameToSendAtoB);
    starttimer(0,TIME_OUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct frm frame)
{
    // bAcknowledgeNumber = 1 - bAcknowledgeNumber;
    if(frame.type == 1)
    {
        int length = strlen(divisor) - 1;
        char* dataBin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataBin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordB = (msgValue << length |valueToAppend);
        char* codeWordBinBinary = convertToBinary(codeWordB);
        char* codedWordBin = CalcCRC(codeWordBinBinary,divisor);


        if(bSequenceNumber == frame.acknum && !isCorrupt(convertToDec(codedWordBin)))
        {
            printf(" Valid Acknowledgment Frame received by B\n");
            okToSendFromB = true;
            bSequenceNumber = (1+bSequenceNumber)%2;
            stoptimer(1);
        }
        else
        {
            printf(" Corrupt Acknowledgment Frame received by B\n");
        }
        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideB = convertToDec(codedWordBin);
            printf(" Remainder Received on Side B to Verify : %lld\n", remndrRcvOnSideB);
        }
    }
    else if(frame.type == 0)
    {
        int length = strlen(divisor) - 1;
        char* dataBin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataBin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordB = (msgValue << length |valueToAppend);
        char* codeWordBinBinary = convertToBinary(codeWordB);
        char* codedWordBin = CalcCRC(codeWordBinBinary,divisor);

        bool expOutput = false;
        if((1-bAcknowledgeNumber) == frame.seqnum)
        {
            expOutput = true;
        }

        if(expOutput && !isCorrupt(convertToDec(codedWordBin)))
        {
            printf(" Valid Frame received by B, message : %s \n", frame.payload);
            bAcknowledgeNumber = (1+bAcknowledgeNumber)%2;
            tolayer3(1,frame.payload);
            if(enablePiggyBack)
            {
                outStandingACKb = 1;
            }
        }
        else
        {
            if(isCorrupt(convertToDec(codedWordBin)))
            {
                printf(" Corrupted Frame received by B, message : %s \n", frame.payload);
            }
            else
            {
                printf(" Repeated Frame received by B, message : %s \n", frame.payload);
                if(enablePiggyBack)
                {
                    struct frm ackFromBtoA = ackGen(bAcknowledgeNumber);
                    strcpy(ackFromBtoA.payload,frame.payload);
                    char* dataB = createMsg(ackFromBtoA.payload,ackFromBtoA.acknum,ackFromBtoA.seqnum);
                    char* codeB = CalcCRC(dataB,divisor);
                    ackFromBtoA.checksum = convertToDec(codeB);
                    if(enableCRC)
                    {
                        printf(" Remainder Sent From Side B to Check : %lld\n",ackFromBtoA.checksum);
                    }
                    outStandingACKb = 0;
                    tolayer1(1,ackFromBtoA);
                }
            }
        }

        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideB = convertToDec(codedWordBin);
            printf(" Remainder Received on Side B to Verify : %lld\n", remndrRcvOnSideB);
        }

        if(!enablePiggyBack)
        {
            struct frm ackFromBtoA = ackGen(bAcknowledgeNumber);
            strcpy(ackFromBtoA.payload,frame.payload);
            char* dataB = createMsg(ackFromBtoA.payload,ackFromBtoA.acknum,ackFromBtoA.seqnum);
            char* codeB = CalcCRC(dataB,divisor);
            ackFromBtoA.checksum = convertToDec(codeB);
            if(enableCRC)
            {
                printf(" Remainder Sent From Side B to Check : %lld\n",ackFromBtoA.checksum);
            }
            tolayer1(1,ackFromBtoA);
        }
    }
    else if(frame.type == 2)
    {
        int length = strlen(divisor) - 1;
        char* dataBin = createMsg(frame.payload,frame.acknum,frame.seqnum);
        unsigned long long int msgValue = convertToDec(dataBin);
        unsigned long long int valueToAppend = frame.checksum;
        unsigned long long int codeWordB = (msgValue << length |valueToAppend);
        char* codeWordBinBinary = convertToBinary(codeWordB);
        char* codedWordBin = CalcCRC(codeWordBinBinary,divisor);

        if(bSequenceNumber == frame.acknum && !isCorrupt(convertToDec(codedWordBin)))
        {
            printf(" Valid Acknowledgment Frame received by B\n");
            okToSendFromB = true;
            bSequenceNumber = (1+bSequenceNumber)%2;
            stoptimer(1);
        }
        else
        {
            printf(" Corrupt Acknowledgment Frame received by B\n");
        }

        bool expOutput = false;
        if((1-bAcknowledgeNumber) == frame.seqnum)
        {
            expOutput = true;
        }

        if(expOutput && !isCorrupt(convertToDec(codedWordBin)))
        {
            printf(" Valid Frame received by B, message : %s \n", frame.payload);
            bAcknowledgeNumber = (1+bAcknowledgeNumber)%2;
            tolayer3(1,frame.payload);
            outStandingACKb = 1;
        }
        else
        {
            if(isCorrupt(convertToDec(codedWordBin)))
            {
                printf(" Corrupted Frame received by B, message : %s \n", frame.payload);
            }
            else
            {
                printf(" Repeated Frame received by B, message : %s \n", frame.payload);
                struct frm ackFromBtoA = ackGen(bAcknowledgeNumber);
                strcpy(ackFromBtoA.payload,frame.payload);
                char* dataB = createMsg(ackFromBtoA.payload,ackFromBtoA.acknum,ackFromBtoA.seqnum);
                char* codeB = CalcCRC(dataB,divisor);
                ackFromBtoA.checksum = convertToDec(codeB);
                if(enableCRC)
                {
                    printf(" Remainder Sent From Side B to Check : %lld\n",ackFromBtoA.checksum);
                }
                outStandingACKb = 0;
                tolayer1(1,ackFromBtoA);
            }
        }
        if(enableCRC)
        {
            unsigned long long int remndrRcvOnSideB = convertToDec(codedWordBin);
            printf(" Remainder Received on Side B to Verify : %lld\n", remndrRcvOnSideB);
        }
    }
}
/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    frameToSendBtoA.type = 0;
    printf(" B_timerinterrupt : Sending message again %s \n",frameToSendBtoA.payload);
    tolayer1(1,frameToSendBtoA);
    starttimer(1,TIME_OUT);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct frm *frmptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER3 1
#define FROM_LAYER1 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float time = 0.000;

float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer1;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    struct event *eventptr;
    struct pkt pkt2give;
    struct frm frm2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer3 ");
            else
                printf(", fromlayer1 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER3)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 4; i++)
                    pkt2give.payload[i] = 97 + j;
                pkt2give.payload[3] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 4; i++)
                        printf("%c", pkt2give.payload[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(pkt2give);
                else
                    B_output(pkt2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER1)
        {
            frm2give.type = eventptr->frmptr->type;
            frm2give.seqnum = eventptr->frmptr->seqnum;
            frm2give.acknum = eventptr->frmptr->acknum;
            frm2give.checksum = eventptr->frmptr->checksum;
            for (i = 0; i < 4; i++)
                frm2give.payload[i] = eventptr->frmptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(frm2give); /* appropriate entity */
            else
                B_input(frm2give);
            free(eventptr->frmptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer3\n",
        time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d",&nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f",&lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f",&corruptprob);
    printf("Enter average time between messages from sender's layer3 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);
    printf("Enter ENABLE PIGGYBACKING:");
    scanf("%d",&enablePiggyBack);
    printf("Enter ENABLE CRC STEPS:");
    scanf("%d",&enableCRC);
    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer1 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER3;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer1(int AorB, struct frm frame)
{
    struct frm *myfrmptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer1++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER1: frame being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    myfrmptr = (struct frm *)malloc(sizeof(struct frm));
    myfrmptr->type = frame.type;
    myfrmptr->seqnum = frame.seqnum;
    myfrmptr->acknum = frame.acknum;
    myfrmptr->checksum = frame.checksum;
    for (i = 0; i < 4; i++)
        myfrmptr->payload[i] = frame.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER1: seq: %d, ack %d, check: %d ", myfrmptr->seqnum,
               myfrmptr->acknum, myfrmptr->checksum);
        for (i = 0; i < 4; i++)
            printf("%c", myfrmptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER1;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->frmptr = myfrmptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER1 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            myfrmptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            myfrmptr->seqnum = 999;
        else
            myfrmptr->acknum = 999;
        if (TRACE > 0)
            printf("          TOLAYER1: frame being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER1: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer3(int AorB, char data[4])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOLAYER3: packet received: ");
        for (i = 0; i < 4; i++)
            printf("%c", data[i]);
        printf("\n");
    }
}
