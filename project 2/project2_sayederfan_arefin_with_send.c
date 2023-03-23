
/* Project 2; Sayed Erfan, Arefin */

#include "csim.h"
#include "stdio.h"
#include "string.h"

#define SIMTIME 1000.0
#define NUM_NODES 5L
#define TIME_OUT 2.0
#define T_DELAY 0.2
#define TRANS_TIME 0.1
#define REQUEST 1L
#define REPLY 2L
#define TRACE 1


#define SEND_HELLO_PERIOD 5


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
	int receivedHello;
	int sentHelloAck;
	int receivedHelloAck;
	int resentHello;
	int failedHello;
	int failedHelloAck;

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
// void return_msg();
msg_t new_msg();

void sim()
{
	// printf("Inside sim \n");
	create("sim");
	init();
	hold(SIMTIME);
	my_report();
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
		node[i].receivedHello = 0;
		node[i].sentHelloAck = 0;
		node[i].receivedHelloAck = 0;
		node[i].resentHello = 0;
		node[i].failedHello = 0;
		node[i].failedHelloAck = 0;
		
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


void createSendReceive(n) long n; {
	msg_t m;
	// long s, t;
	m = new_msg(n);
	decode_msg("sends hello", m, n);		
	send_msg(m, n);
	node[n].sentHello = node[n].sentHello + 1;

	// 	s = timed_receive(node[n].input, &m, TIME_OUT);

	// 	switch (s)
	// 	{
	// 	case TIMED_OUT:
	// 		//  m = new_msg(n);

	// 		if (m->resent < 1){
	// 			m->resent = 1;
	// 			decode_msg("re sends hello", m, n);
					
	// 			send_msg(m);
	// 			node[n].resentHello = node[n].resentHello + 1;

	// 		} else {
	// 			// failed message
	// 			node[n].failedHello = node[n].failedHello + 1;
	// 		}
	// 		break;
	// 	case EVENT_OCCURRED:
	// 		node[n].receivedHello = node[n].receivedHello + 1;
			
	// 		t = m->type;
	// 		switch (t)
	// 		{
	// 		case REQUEST:
	// 			from_reply(m);
	// 			decode_msg("replies a hello_ack", m, n);
	// 			node[n].sentHelloAck = node[n].sentHelloAck + 1;
	// 			send_msg(m);
	// 			break;
	// 		case REPLY:
	// 			decode_msg("receives a hello_ack", m, n);
	// 			node[n].receivedHelloAck = node[n].receivedHelloAck + 1;
	// 			record(clock - m->start_time, resp_tm);
	// 			return_msg(m);
	// 			break;
	// 		default:
	// 			decode_msg("***unexpected type", m, n);
	// 			break;
	// 		}
	// 		break;

	// 	default:
	// 			decode_msg("-------------", m, n);
	// 			break;

	// 	}
}

void proc(n) long n;
{
	
	create("proc");
	while (clock < SIMTIME)
	{
		hold(exponential(SEND_HELLO_PERIOD));
		createSendReceive(n);
	}
}


void send_ack(){
	if (m->type == REQUEST){
				from_reply(m);
				decode_msg("replies a hello_ack", m, n);
				node[n].sentHelloAck = node[n].sentHelloAck + 1;
				send_msg(m);

			} else if (m->type == REPLY){
				decode_msg("receives a hello_ack", m, n);
				node[n].receivedHelloAck = node[n].receivedHelloAck + 1;
				record(clock - m->start_time, resp_tm);
				// return_msg(m);
			} else{
				printf("----------- else ------------");
			}
}

void send_msg(m, n)
	msg_t m; long n;
{
	long from, to;
	long st;
	from = m->from;
	to = m->to;
	use(node[from].cpu, T_DELAY);
	reserve(network[from][to]);
	hold(TRANS_TIME);
	// send(node[to].input, m);
		 
	st = timed_synchronous_send(node[to].input, m, TIME_OUT);

	if (st == TIMED_OUT){



	} else {
		if (m->type == REQUEST){
				if (m->resent < 1){
					m->resent = 1;
					decode_msg("re sends hello", m, n);
						
					send_msg(m);
					node[n].resentHello = node[n].resentHello + 1;

				} 
			} 
	}


	
	

	release(network[from][to]);
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

// void return_msg(m)
// 	msg_t m;
// {
// 	m->link = msg_queue;
// 	msg_queue = m;
// }

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
	int totalHelloReceived = 0;
	int totalHelloAckSent = 0;
	int totalHelloResent = 0;
	int totalHelloFailed = 0;
	int totalHelloAckFailed = 0;

	int i;
	for (i = 0; i < NUM_NODES; i++)
	{
		totalHelloSent = totalHelloSent + node[i].sentHello;
		totalHelloAckReceived = totalHelloAckReceived + node[i].receivedHelloAck;
		totalHelloReceived = totalHelloReceived + node[i].receivedHello;
		totalHelloAckSent = totalHelloAckSent + node[i].sentHelloAck;
		totalHelloResent = totalHelloResent + node[i].resentHello;
		totalHelloFailed = totalHelloFailed + node[i].failedHello;
		totalHelloAckFailed = totalHelloAckFailed + node[i].failedHelloAck;
	}


	printf("Total Hello Message sent: %d \n", totalHelloSent);
	printf("Total Hello Message received: %d \n", totalHelloReceived);
	printf("Total Hello Message resent: %d \n", totalHelloResent);

	printf("Total Hello Ack Message sent: %d \n", totalHelloAckSent);
	printf("Total Hello Ack Message received: %d \n", totalHelloAckReceived);

	printf("Total Hello Failed: %d \n", totalHelloFailed);
	printf("Total Hello Ack Failed: %d \n", totalHelloAckFailed);

	// double successfulTransmission = (totalHelloAckReceived / totalHelloSent) * 100;
	// printf("Average number of successful transmissions: %lf \n", successfulTransmission);
}