
/* Project 3; Arefin, Sayed Erfan */

#include "csim.h"
#include "stdio.h"
// #include "stdlib.h"
#include "string.h"

// atleast 5000 SIMTIME. It is recommended to double or triple it 15000.0
#define SIMTIME 1000.0
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
#define HOT_DATA_ITEM_SIZE 50



// #define TIME_OUT 2.0
// #define T_DELAY 0.2
// #define TRANS_TIME 0.1
// #define LOCAL_PROCESSING_DELAY 0.1
// #define REQUEST 1L
// #define REPLY 2L
// #define TRACE 1
// #define MAX_RESENT_ALLOWED 1


// #define SEND_HELLO_PERIOD 5

double packetloss_probabiliity;

long T_UPDATE;
long T_QUERY;

// #define T_QUERY 0.01


// 0 for server
FACILITY network[NUM_CLIENTS];


struct item
{
	int item_id;
	TIME updated_time;

	// size of long is 8 bytes. For a data to be 1024 bytes there should be 128 long array
	// long data[128];
	long data;
	int item_type;
} ;

struct item serverDatabase[DB_SIZE];


typedef struct msg *msg_t;


struct msg
{
	long type;
	long to;
	long from;
	TIME time_stamp;
	msg_t link;
	struct item itemm;
	
};

msg_t msg_queue;

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


typedef struct srvr
{
	FACILITY cpu;
	MBOX input;
	
} server_i;

server_i server_main;

TABLE resp_tm;
FILE *fp;


void init();
void my_report();
void send_msg();
void from_reply();
void decode_msg();
void return_msg();
msg_t clientQuery();


void createQuery(long n);
void updateColdDataItem();
void update_hot_data_item();
void procServerUpdateItem();
void procClient(long n);
void procServerReply() ;


void sim(int argc, char *argv[] )
{
	// // printf("0. Starting..");
	if( argc == 2 ) {
		
		sscanf(argv[1], "%lf", &packetloss_probabiliity);

		if (packetloss_probabiliity < 1.0 ){
			
			if ( packetloss_probabiliity > 0.0){
				// // printf("Starting..");
				create("sim");
				init();
				hold(SIMTIME);
				my_report();

				free(msg_queue);

			} else {
				// printf("Packet loss probability should be greater than 0.\n");
			}

		} else {
			// printf("Packet loss probability should be less than 1.\n");
		}
	}
	else if( argc > 2 ) {
		// printf("Too many arguments supplied.\n");
	}
	else {
		// printf("One argument expected.\n");
	}
}

void init()
{
	T_UPDATE = 0.003;
	T_QUERY = 0.01;
	// // printf("init..");
	long i, j;
	char str[24];
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	// // printf("setting max..");
	max_facilities(NUM_CLIENTS * NUM_CLIENTS + 1);
	max_servers(NUM_CLIENTS * NUM_CLIENTS);
	max_mailboxes(NUM_CLIENTS + 1);
	max_events(4 * NUM_CLIENTS );
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;


	// printf("client init loop..\n");


	for (i = 0; i < NUM_CLIENTS; i++)
	{
		sprintf(str, "cpu %d", i);
		client[i].cpu = facility(str);
		sprintf(str, "input %d", i);
		client[i].input = mailbox(str);

		client[i].numberOfQuery = 0;
		client[i].cache_hit = 0;
		client[i].cacheSize = 0;
		client[i].average_query_delay = 0.0;
		
		// client[i].client_cache
	}

	// printf("Creating facilities\n");
	for (i = 0; i < NUM_CLIENTS; i++)
	{
			sprintf(str, "nt %d", i);
			network[i] = facility(str);
	}

	// server:
	// printf("Server creation\n");
	sprintf(str, "cpusrvr");
	server_main.cpu = facility(str);
	sprintf(str, "inputsrvr");

	server_main.input = mailbox(str);


	// initialize database
	//hot 50
	// cold 450

	// printf("Server Database\n");

	int i_db;
	for (i_db = 0; i_db <  DB_SIZE; i_db++){
		serverDatabase[i_db].data = 2143646;
		serverDatabase[i_db].item_id = i_db;
		serverDatabase[i_db].updated_time = clock;
		if (i_db < HOT_DATA_ITEM_SIZE){
			serverDatabase[i_db].item_type = ITEM_HOT;
		} else {
			serverDatabase[i_db].item_type = ITEM_COLD;
		}

	}

	// printf("Running client processes\n");
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		procClient(i);
	}

	
	procServerUpdateItem();

	
	procServerReply();
}


int checkCache(m, n)
msg_t m; long n;{
	// reply 0 if not present in cache
	// reply item index if present in cache

	int i;
	int found = -1;
	for (i =0; i < CACHE_SIZE; i++){
		if (client[n].client_cache[i].item_id == m->itemm.item_id){
			found = i;
			break;
		}
	}

	return found;
	
}


void createQuery(n) long n;{

	// printf("Client %d: Create query\n", n);

	msg_t m;
	long t;
	m = clientQuery(n);

	int cacheCheck = checkCache(m, n);

	// printf("Client %d: After check cache\n", n);

	if (cacheCheck == -1){

		send_msg(m);
		printf("Client %d: send MSG_REQUEST\n", n);
	} else {
		m->type = MSG_CHECK;
		m->itemm.updated_time = client[n].client_cache[m->itemm.item_id].updated_time;
		send_msg(m);
		printf("Client %d: send MSG_CHECK\n", n);
	}

	// decode_msg("sends hello", m, n);	
	// printf("Client %d: Sending query\n", n);
	
	client[n].numberOfQuery = client[n].numberOfQuery + 1;

	receive(client[n].input, &m);
	t = m->type;
		switch (t)
		{
		case MSG_CONFIRM:
			// printf("Client %d: received MSG_CONFIRM\n", n);
			// from_reply(m);
			// decode_msg("replies a hello_ack", m, n);
			
			// send_msg(m);
			printf("Client %d: received MSG_CONFIRM\n", n);
			break;

		case MSG_DATA:
			// printf("Client %d: received MSG_DATA\n", n);
			// decode_msg("receives a hello_ack", m, n);
			// client[n].cache_hit = client[n].cache_hit + 1;
			// record(clock - m->time_stamp, resp_tm);
			// return_msg(m);
			printf("Client %d: received MSG_DATA\n", n);
			break;

		default:
			// decode_msg("***unexpected type", m, n);
			break;
		}

}


void updateColdDataItem(){
	// // printf("Updating cold data item\n");

	int ii;
	for (ii = HOT_DATA_ITEM_SIZE; ii < DB_SIZE; ii++){
		if (serverDatabase[ii].item_type == ITEM_COLD){
			serverDatabase[ii].data = zipf(ii);
			serverDatabase[ii].updated_time = clock;
		}
		
	}
}

void update_hot_data_item(){
	// // printf("Updating hot data item\n");
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
	// printf("Running server item update process\n");
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
	}
}

void procServerReply() 
{
	// printf("Running server process\n");
	create("procServerReply");
	while (clock < SIMTIME)
	{
		// printf("Inside server process loop\n");
		msg_t m;
		long s, t;
		receive(server_main.input, &m); 
		
		// printf("server received something\n");

		if (m->type == MSG_REQUEST){

			// printf("server received msg_request\n");
			
			int item_id = m->itemm.item_id;
			m->type = MSG_DATA;
			m->itemm.item_id = serverDatabase[item_id].item_id;
			m->itemm.updated_time = serverDatabase[item_id].updated_time;
			m->itemm.data = serverDatabase[item_id].data;
			m->itemm.item_type = serverDatabase[item_id].item_type;
			from_reply(m);
			send_msg(m);
			printf("Server: replied MSG_DATA\n");

		} else if (m->type == MSG_CHECK){
			// printf("server received msg_check\n");
			int item_id = m->itemm.item_id;
			TIME updated_time = m->itemm.updated_time;
			// check if data is old or not
			if (serverDatabase[item_id].updated_time > updated_time){
				m->type = MSG_DATA;
				m->itemm.item_id = serverDatabase[item_id].item_id;
				m->itemm.updated_time = serverDatabase[item_id].updated_time;
				m->itemm.data = serverDatabase[item_id].data;
				m->itemm.item_type = serverDatabase[item_id].item_type;
				from_reply(m);
				send_msg(m);
				printf("Server: replied MSG_DATA\n");

			} else {
				// printf("server returns msg_confirm\n");
				m->type = MSG_CONFIRM;
				from_reply(m);
				send_msg(m);
				printf("Server: replied MSG_CONFIRM\n");
			}
		}

		else{
			// do nothing
		}
		
	}

}
void procClient(n) long n;
{
	// printf("Client %d: creating process\n", n);
	
	create("procClient");
	// printf("Client %d: ------- \n", n);
	
	while (clock < SIMTIME)
	{
		// printf("Client %d: Inside process\n", n);
		createQuery(n);
		// printf("Client %d: done querying......... next!\n", n);
		hold(T_QUERY);
		// printf("Client %d: done querying......... next!   after hold period T_QUERY\n", n);
	}
}

void send_msg(m)
	msg_t m;
{

// printf("--- send message\n");
	long from, to;
	from = m->from;
	to = m->to;
	// use(client[from].cpu, T_DELAY);

	// check if to is server
	if (to == -1){
		// printf("to server");
		// reserve(network[from]);
		send(server_main.input, m);	
		// release(network[from]);

	} else if (from == -1 ){
		// printf("from server");
		// reserve(network[to]);
		send(client[to].input, m);	
		// release(network[to]);
	}

// printf("--- done send message\n");
}




msg_t clientQuery(from)
long from;
{
	msg_t m;
	long to = -1; // -1 indicates server
	if (msg_queue == NIL)
	{
		m = (msg_t)do_malloc(sizeof(struct msg));
	}
	else
	{
		m = msg_queue;
		msg_queue = msg_queue->link;
	}

	m->to = to;
	m->from = from;
	m->type = MSG_REQUEST;
	m->time_stamp = clock;



// 	hot/cold data item query:

// x1 = uniform (0, 1)
// x1 > 0.8 -> cold data item
// x1 <= 0.8 -> hot data item

// if hot data then, y1= uniform (1, 50), y1 is the item that needs to be queried. 
	double x1 = uniform (0, 1);
	if (x1 > 0.8){
		// cold data item
		m->itemm.item_type = ITEM_COLD;

		// for cold data item type lets select any id, randomly
		int y2 = random(50, DB_SIZE);
		m->itemm.item_id = y2;

	} else {
		// hot data item
		m->itemm.item_type = ITEM_HOT;

		int y1= uniform (0, 49);
		m->itemm.item_id = y1;
	}
	
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
}

void decode_msg(str, m, n) char *str;
msg_t m;
long n;
{
	// // printf("%6.3f client %2ld: %s - msg: type = %s, from = %ld, to = %ld\n", clock, n, str, (m->type == REQUEST) ? "req" : "rep", m->from, m->to);
	// if ((m->to == n) && (m->type == REQUEST))
	// {
	// 	// printf( "client.%2ld: %s from client.%ld  at %6.3f seconds\n", n, str, m->from, clock);
	// 	// printf("client.%2ld: %s from client.%ld at %6.3f seconds\n", m->to, str, m->from, clock);
	// }
	// else if ((m->from == n) && (m->type == REQUEST))
	// {
	// 	// printf("client.%2ld: %s to client.%ld at %6.3f seconds\n", n, str, m->to, clock);
	// }
	// else if ((m->from == n) && (m->type != REQUEST))
	// {
	// 	// printf("client.%2ld: %s to client.%ld at %6.3f seconds\n", n, str, m->to, clock);
	// }
	// else
	// {
	// 	//	 printf( "client.%2ld: %s from client.%ld  at %6.3f seconds\n", n, str, m->from, clock);
	// }
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

	// // printf("Total Hello Message sent: %d \n", totalHelloSent);
	// // printf("Total Hello Ack Message received: %d \n", totalHelloAckReceived);

	int totalMessageFailed = totalHelloSent - totalHelloAckReceived;
	double averageSuccess = totalHelloAckReceived/NUM_CLIENTS;
	double averageFailed = totalMessageFailed/NUM_CLIENTS;

	// printf("Average number of successful transmissions: %lf \n", averageSuccess);
	// printf("Average number of failed transmissions: %lf \n", averageFailed);
	// printf("Packet loss probability: %lf \n", packetloss_probabiliity);

}


