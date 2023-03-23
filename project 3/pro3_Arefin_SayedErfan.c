
/* Project 3; Arefin, Sayed Erfan */

#include "csim.h"
#include "stdio.h"
// #include "stdlib.h"
#include "string.h"

// atleast 5000 SIMTIME. It is recommended to double or triple it
#define SIMTIME 15000.0
#define NUM_CLIENTS 5L

// message types:
#define MSG_REQUEST 1L
#define MSG_CHECK 2L
#define MSG_CONFIRM 3L
#define MSG_DATA 4L


// item type
#define ITEM_COLD 5L
#define ITEM_HOT 6L

#define DB_SIZE 500
#define CACHE_SIZE 100
#define HOT_DATA_ITEM_SIZE 450



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

long T_UPDATE;

typedef struct msg *msg_t;


struct msg
{
	long type;
	long to;
	long from;
	TIME time_stamp;
	msg_t link;
	
};

msg_t msg_queue;



FACILITY network[NUM_CLIENTS][NUM_CLIENTS];


struct item
{
	int item_id;
	TIME updated_time;

	// size of long is 8 bytes. For a data to be 1024 bytes there should be 128 long array
	// long data[128];
	long data;
	int item_type;
};

struct item serverDatabase[DB_SIZE];


struct clnt
{
	FACILITY cpu;
	MBOX input;
	int numberOfQuery;
	int cache_hit;
	double average_query_delay;

	struct item client_cache[CACHE_SIZE];
	int cacheSize;
};

struct clnt client[NUM_CLIENTS];


struct srvr
{
	FACILITY cpu;
	MBOX input;
	
};



TABLE resp_tm;
FILE *fp;


void init();
void my_report();
void send_msg();
void from_reply();
void decode_msg();
void return_msg();
msg_t new_msg();


void createQuery(long n);
void updateColdDataItem();
void update_hot_data_item();
void procServerUpdateItem();
void procClient(long n);
void procServerReply() ;


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

				free(msg_queue);

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
	long i, j;
	char str[24];
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	max_facilities(NUM_CLIENTS * NUM_CLIENTS + NUM_CLIENTS);
	max_servers(NUM_CLIENTS * NUM_CLIENTS + NUM_CLIENTS);
	max_mailboxes(NUM_CLIENTS);
	max_events(2 * NUM_CLIENTS * NUM_CLIENTS);
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;



	for (i = 0; i < NUM_CLIENTS; i++)
	{
		sprintf(str, "cpu %d", i);
		client[i].cpu = facility(str);
		printf(str, "input %d", i);
		client[i].input = mailbox(str);

		client[i].numberOfQuery = 0;
		client[i].cache_hit = 0;
		client[i].cacheSize = 0;
		client[i].average_query_delay = 0.0;
		
		// client[i].client_cache
	}

	for (i = 0; i < NUM_CLIENTS; i++)
	{
		for (j = 0; j < NUM_CLIENTS; j++)
		{
			sprintf(str, "nt %d %d", i, j);
			network[i][j] = facility(str);
		}
	}

	for (i = 0; i < NUM_CLIENTS; i++)
	{
		procClient(i);
	}


	// server:


	sprintf(str, "cpu srvr");
	srvr.cpu = facility(str);
	printf(str, "input srvr");
	srvr.input = mailbox(str);


	// initialize database
		//hot 50
	// cold 450
	int i_db, i_dta;
	for (i_db = 0; i_db <  DB_SIZE; i_db++){
		item item_o = (item)do_malloc(sizeof(struct item));
		item_o.item_id = i_db;
		item_o.updated_time =  clock;
		if (i_db < HOT_DATA_ITEM_SIZE){
			item_o.item_type = ITEM_HOT;
		} else {
			item_o.item_type = ITEM_COLD;
		}

		// for (i_dta =0; i_dta < 128; i_dta++){
		// 	item_o.data[i_dta] = 2147483646;
		// }
		item_o.data[i_dta] = 2143646;

		serverDatabase[i_db] = item_o;
	}



	procServerUpdateItem();
	procServerReply();
}


void checkCache(){

}

void createQuery(long n){

	msg_t m;
	long t;
	m = new_msg(n);
	decode_msg("sends hello", m, n);		
	send_msg(m);
	client[n].numberOfQuery = client[n].numberOfQuery + 1;

	receive(client[n].input, &m);
	t = m->type;
		switch (t)
		{
		case MSG_CONFIRM:
			from_reply(m);
			decode_msg("replies a hello_ack", m, n);
			
			send_msg(m);
			break;

		case MSG_DATA:
			decode_msg("receives a hello_ack", m, n);
			client[n].cache_hit = client[n].cache_hit + 1;
			record(clock - m->time_stamp, resp_tm);
			return_msg(m);
			break;

		default:
			decode_msg("***unexpected type", m, n);
			break;
		}




}


void updateColdDataItem(){
	int ii;
	for (ii = HOT_DATA_ITEM_SIZE; ii < DB_SIZE; ii++){
		if (serverDatabase[ii].item_type == ITEM_COLD){
			serverDatabase[ii].data = zipf(ii);
			serverDatabase[ii].updated_time = clock;
		}
		
	}
}

void update_hot_data_item(){
	int ii;
	for (ii =0; ii < HOT_DATA_ITEM_SIZE; ii++){
		if (serverDatabase[ii].item_type == ITEM_HOT){
			serverDatabase[ii].data = zipf(ii);
			serverDatabase[ii].updated_time = clock;
		}
		
	}

}
void procServerUpdateItem()
{
	
	create("procServerUpdateItem");
	while (clock < SIMTIME)
	{

		double x = uniform (0, 1);
		if (x > 0.33){
			// update -> cold data item
			updateColdDataItem();
		} else {
			// update -> hot data item
			update_hot_data_item();
		}
		
		hold(T_UPDATE);

		// createQuery(n, holdTime);
		
		
	}
}

void procServerReply() 
{
	create("procServerReply");
	while (clock < SIMTIME)
	{
		msg_t m;
		long s, t;
		receive(srvr.input, &m); 
		
		if (m->type == MSG_REQUEST){

		} else if (m->type == MSG_CHECK){

		}

		else{

		}
		
	}

}

void procClient(n) long n;
{
	
	create("procClient");
	while (clock < SIMTIME)
	{

		// hold(TIME_OUT);
		// float holdTime = exponential(SEND_HELLO_PERIOD);
		// hold(holdTime);

		createQuery(n);
		
		
	}
}

void send_msg(m)
	msg_t m;
{

	long from, to;
	from = m->from;
	to = m->to;
	use(client[from].cpu, T_DELAY);
	reserve(network[from][to]);
		
	if (m->type == REQUEST){
		 hold(TRANS_TIME);
		send(client[to].input, m);	
		
	}else {
		hold(LOCAL_PROCESSING_DELAY);
		send(client[to].input, m);
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
		i = random(01, NUM_CLIENTS - 1);
	} while (i == from);

	m->to = i;
	m->from = from;
	m->type = REQUEST;
	m->time_stamp = clock;
	// m->resent = 0;
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
	// printf("%6.3f client %2ld: %s - msg: type = %s, from = %ld, to = %ld\n", clock, n, str, (m->type == REQUEST) ? "req" : "rep", m->from, m->to);
	if ((m->to == n) && (m->type == REQUEST))
	{
		// printf( "client.%2ld: %s from client.%ld  at %6.3f seconds\n", n, str, m->from, clock);
		printf("client.%2ld: %s from client.%ld at %6.3f seconds\n", m->to, str, m->from, clock);
	}
	else if ((m->from == n) && (m->type == REQUEST))
	{
		printf("client.%2ld: %s to client.%ld at %6.3f seconds\n", n, str, m->to, clock);
	}
	else if ((m->from == n) && (m->type != REQUEST))
	{
		printf("client.%2ld: %s to client.%ld at %6.3f seconds\n", n, str, m->to, clock);
	}
	else
	{
		//	 printf( "client.%2ld: %s from client.%ld  at %6.3f seconds\n", n, str, m->from, clock);
	}
}

void my_report()
{
	int totalHelloSent = 0;
	int totalHelloAckReceived = 0;

	int i;
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		totalHelloSent = totalHelloSent + client[i].numberOfQuery;
		totalHelloAckReceived = totalHelloAckReceived + client[i].cache_hit;
		
	}

	// printf("Total Hello Message sent: %d \n", totalHelloSent);
	// printf("Total Hello Ack Message received: %d \n", totalHelloAckReceived);

	int totalMessageFailed = totalHelloSent - totalHelloAckReceived;
	double averageSuccess = totalHelloAckReceived/NUM_CLIENTS;
	double averageFailed = totalMessageFailed/NUM_CLIENTS;

	printf("Average number of successful transmissions: %lf \n", averageSuccess);
	printf("Average number of failed transmissions: %lf \n", averageFailed);
	printf("Packet loss probability: %lf \n", packetloss_probabiliity);

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
		i = random(01, NUM_CLIENTS - 1);
	} while (i == from);

	m->to = i;
	m->from = from;
	m->type = REQUEST;
	m->time_stamp = clock;
	// m->resent = 0;
	return (m);
}