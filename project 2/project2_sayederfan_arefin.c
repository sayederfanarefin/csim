
/* Project 2; Sayed Erfan, Arefin */

#include "csim.h"
#include "stdio.h"
// #include "stdlib.h"
#include "string.h"

#define SIMTIME 1000.0
#define NUM_NODES 5L
#define TIME_OUT 2.0
#define T_DELAY 0.2
#define TRANS_TIME 0.1
#define LOCAL_PROCESSING_DELAY 0.1
#define REQUEST 1L
#define REPLY 2L
#define TRACE 1
#define MAX_RESENT_ALLOWED 1


#define SEND_HELLO_PERIOD 5

double packetloss_probabiliity;

typedef struct msg *msg_t;

struct msg
{
	long type;
	long to;
	long from;
	TIME start_time;
	msg_t link;
	int resent;
};

msg_t msg_queue;

struct nde
{
	FACILITY cpu;
	MBOX input;

	int sentHello;
	int receivedHelloAck;

};

struct nde node[NUM_NODES];
FACILITY network[NUM_NODES][NUM_NODES];

TABLE resp_tm;
FILE *fp;


void init();
void my_report();
void proc();
void send_msg();
void from_reply();
void decode_msg();
void return_msg();
msg_t new_msg();

void sim(int argc, char *argv[] )
{
	if( argc == 2 ) {
		
		sscanf(argv[1], "%lf", &packetloss_probabiliity);

		if (packetloss_probabiliity < 1.0 ){
			
			if ( packetloss_probabiliity > 0.0){
				create("sim");
				init();
				hold(SIMTIME);
				my_report();
			} else {
				printf("Packet loss probability should be greater than 0.\n");
			}

		} else {
			printf("Packet loss probability should be less than 1.\n");
		}
	}
	else if( argc > 2 ) {
		printf("Too many arguments supplied.\n");
	}
	else {
		printf("One argument expected.\n");
	}


}

void init()
{
	// printf("inside init\n");
	long i, j;
	char str[24];
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	max_facilities(NUM_NODES * NUM_NODES + NUM_NODES);
	max_servers(NUM_NODES * NUM_NODES + NUM_NODES);
	max_mailboxes(NUM_NODES);
	max_events(2 * NUM_NODES * NUM_NODES);
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;
	for (i = 0; i < NUM_NODES; i++)
	{
		sprintf(str, "cpu %d", i);
		node[i].cpu = facility(str);
		printf(str, "input %d", i);
		node[i].input = mailbox(str);

		node[i].sentHello = 0;
		node[i].receivedHelloAck = 0;
	}

	for (i = 0; i < NUM_NODES; i++)
	{
		for (j = 0; j < NUM_NODES; j++)
		{
			sprintf(str, "nt %d %d", i, j);
			network[i][j] = facility(str);
		}
	}

	for (i = 0; i < NUM_NODES; i++)
	{
		proc(i);
	}
}

void createSendReceive(long n){

	msg_t m;
	long s, t;
	m = new_msg(n);
		decode_msg("sends hello", m, n);		
		send_msg(m);
		node[n].sentHello = node[n].sentHello + 1;

		s = timed_receive(node[n].input, &m, TIME_OUT);

		switch (s)
		{
		case TIMED_OUT:
			//  m = new_msg(n);

			if (m->resent < MAX_RESENT_ALLOWED){
				m->resent = m->resent + 1;
				decode_msg("re sends hello", m, n);
					
				send_msg(m);
				
			} 

			break;
		case EVENT_OCCURRED:
			
			t = m->type;
			switch (t)
			{
			case REQUEST:
				from_reply(m);
				decode_msg("replies a hello_ack", m, n);
				hold(LOCAL_PROCESSING_DELAY);
				send_msg(m);
				break;
			case REPLY:
				decode_msg("receives a hello_ack", m, n);
				node[n].receivedHelloAck = node[n].receivedHelloAck + 1;
				record(clock - m->start_time, resp_tm);
				return_msg(m);
				break;
			default:
				decode_msg("***unexpected type", m, n);
				break;
			}
			break;

		default:
				break;

		}
}

void proc(n) long n;
{
	
	create("proc");
	while (clock < SIMTIME)
	{

		// hold(SEND_HELLO_PERIOD);
		hold(exponential(SEND_HELLO_PERIOD));

		createSendReceive(n);
		
		
	}
}

void send_msg(m)
	msg_t m;
{
	// packet loss probability

	double uniformResult = uniform(0, 1);


	if (uniformResult < packetloss_probabiliity){
		long from, to;
		from = m->from;
		to = m->to;
		use(node[from].cpu, T_DELAY);
		reserve(network[from][to]);
		hold(TRANS_TIME);
		send(node[to].input, m);
		release(network[from][to]);
	}

}




msg_t new_msg(from)
long from;
{
	msg_t m;
	long i;
	if (msg_queue == NIL)
	{
		m = (msg_t)do_malloc(sizeof(struct msg));
	}
	else
	{
		m = msg_queue;
		msg_queue = msg_queue->link;
	}

	do
	{
		i = random(01, NUM_NODES - 1);
	} while (i == from);

	m->to = i;
	m->from = from;
	m->type = REQUEST;
	m->start_time = clock;
	m->resent = 0;
	return (m);
}

void return_msg(m)
	msg_t m;
{
	m->link = msg_queue;
	msg_queue = m;
}

void from_reply(m)
	msg_t m;
{
	long from, to;
	from = m->from;
	to = m->to;
	m->from = to;
	m->to = from;
	m->type = REPLY;
}

void decode_msg(str, m, n) char *str;
msg_t m;
long n;
{
	// printf("%6.3f node %2ld: %s - msg: type = %s, from = %ld, to = %ld\n", clock, n, str, (m->type == REQUEST) ? "req" : "rep", m->from, m->to);
	if ((m->to == n) && (m->type == REQUEST))
	{
		// printf( "node.%2ld: %s from node.%ld  at %6.3f seconds\n", n, str, m->from, clock);
		printf("node.%2ld: %s from node.%ld at %6.3f seconds\n", m->to, str, m->from, clock);
	}
	else if ((m->from == n) && (m->type == REQUEST))
	{
		printf("node.%2ld: %s to node.%ld at %6.3f seconds\n", n, str, m->to, clock);
	}
	else if ((m->from == n) && (m->type != REQUEST))
	{
		printf("node.%2ld: %s to node.%ld at %6.3f seconds\n", n, str, m->to, clock);
	}
	else
	{
		//	 printf( "node.%2ld: %s from node.%ld  at %6.3f seconds\n", n, str, m->from, clock);
	}
}

void my_report()
{
	int totalHelloSent = 0;
	int totalHelloAckReceived = 0;

	int i;
	for (i = 0; i < NUM_NODES; i++)
	{
		totalHelloSent = totalHelloSent + node[i].sentHello;
		totalHelloAckReceived = totalHelloAckReceived + node[i].receivedHelloAck;
		
	}

	// printf("Total Hello Message sent: %d \n", totalHelloSent);
	// printf("Total Hello Ack Message received: %d \n", totalHelloAckReceived);

	int totalMessageFailed = totalHelloSent - totalHelloAckReceived;
	double averageSuccess = totalHelloAckReceived/NUM_NODES;
	double averageFailed = totalMessageFailed/NUM_NODES;

	printf("Average number of successful transmissions: %lf \n", averageSuccess);
	printf("Average number of failed transmissions: %lf \n", averageFailed);
	printf("Packet loss probability: %lf \n", packetloss_probabiliity);

}