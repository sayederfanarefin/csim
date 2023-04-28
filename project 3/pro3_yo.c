#include "csim.h"
#include "stdio.h"
#include "string.h"

double SIMTIME = 15000.0;
#define NUM_CLIENTS 5L

#define MSG_REQUEST 11L
#define MSG_CHECK 3L
#define MSG_CONFIRM 2L
#define MSG_DATA 8L


#define ITEM_COLD 6L
#define ITEM_HOT 5L

#define DB_SIZE 500
#define CACHE_SIZE 100
#define HOT_DATA_ITEM_SIZE 50


double T_UPDATE;
double T_QUERY;
double T_DELAY_LOAD = 2.3; 
double T_DELAY_MSG = 0.6; 

struct item
{
	int item_id;
	TIME updated_time;
	long data;
	int item_type;
} ;

struct item serverDatabase[DB_SIZE];

typedef struct msg *msg_t;


struct msg
{
	TIME time_stamp;
	long type;
	long to;
	long from;
	msg_t link;
	struct item itemm;
	
};

msg_t msg_queue;

struct clnt
{

	TIME usedTime[CACHE_SIZE];
	MBOX input;
	int numberOfQuery;
	int cache_hit;
	double average_query_delay;
	struct item client_cache[CACHE_SIZE];
	int cacheSize;
	int coldState;
};
	

struct clnt nodes[NUM_CLIENTS];


typedef struct srvr
{
	MBOX input;
	
} server_i;

server_i server_main;

TABLE resp_tm;
FILE *fp;


void init();
void my_report();
void send_msg();
void from_reply();
void return_msg();
msg_t client_query();
void create_query(long n);
void update_cold_data_item();
void update_hot_data_item();
void proc_server_update_item();
void proc_client(long n);
void proc_server_reply() ;


void sim(int argc, char *argv[] )
{
	printf("Please enter T update:\n");
	scanf("%lf", &T_UPDATE);
	printf("Please enter T query:\n");
	scanf("%lf", &T_QUERY);

	create("simproc");
	init();
	hold(SIMTIME);
	my_report();
	free(msg_queue);

}

void init()
{

	long i, j;
	char str[24];
	fp = fopen("output.out", "w");
	set_output_file(fp);
	
	max_facilities(NUM_CLIENTS * NUM_CLIENTS + 1);
	max_servers(NUM_CLIENTS * NUM_CLIENTS);
	max_mailboxes(NUM_CLIENTS + 1);
	max_events(4 * NUM_CLIENTS );
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;


	for (i = 0; i < NUM_CLIENTS; i++)
	{
		sprintf(str, "input %d", i);
		nodes[i].input = mailbox(str);

		nodes[i].numberOfQuery = 0;
		nodes[i].cache_hit = 0;
		nodes[i].cacheSize = 0;
		nodes[i].average_query_delay = 0.0;
		nodes[i].coldState = 0;
		
	}
	sprintf(str, "inputsrvr");
	server_main.input = mailbox(str);

	int i_db;
	for (i_db = 0; i_db <  DB_SIZE; i_db++){
		serverDatabase[i_db].data = 663;
		serverDatabase[i_db].item_id = i_db;

		serverDatabase[i_db].updated_time = clock;

		if (i_db < HOT_DATA_ITEM_SIZE){
			serverDatabase[i_db].item_type = ITEM_HOT;
		} else {
			serverDatabase[i_db].item_type = ITEM_COLD;
		}
		hold(0.001);

	}
	
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		
		proc_client(i);
	}

	proc_server_update_item();
	proc_server_reply();
}


int checkCache(m, n)
msg_t m; 
long n; {
	int i;
	int found = -1;
	for (i = 0; i < CACHE_SIZE; i++){

		if (nodes[n].client_cache[i].item_id == m->itemm.item_id){
			found = i;
			break;
		}
	}

	return found;
}


int checkLRU(n) long n;{
	TIME oldestTime = nodes[n].usedTime[0];
	long oldestIndex = 0;
	int i ;
	for (i=0; i < CACHE_SIZE; i ++){

		if (nodes[n].usedTime[i] < oldestTime){
			oldestTime = nodes[n].usedTime[i];
			oldestIndex = i;
		}
	}
	return oldestIndex;
	
}

void queryDelay (n, queryTime) long n; long queryTime;{
	TIME currentTime = clock;
	double queryDelay = currentTime - queryTime;
	nodes[n].average_query_delay = ((nodes[n].average_query_delay * ( nodes[n].numberOfQuery -1 )) + queryDelay) / nodes[n].numberOfQuery;
}
void create_query(n) long n;{
	msg_t m;
	long t;
	m = client_query(n);
	TIME queryTime = clock;
	int cacheCheck = checkCache(m, n);
	if (cacheCheck == -1){
		send_msg(m);
	} else {
		m->type = MSG_CHECK;
		m->itemm.updated_time = nodes[n].client_cache[cacheCheck].updated_time;
		send_msg(m);
	}
	nodes[n].numberOfQuery = nodes[n].numberOfQuery + 1;
	receive(nodes[n].input, &m);
	t = m->type;
		switch (t)
		{
		case MSG_CONFIRM:

			hold(0.001);

			nodes[n].usedTime[cacheCheck] = clock;
			nodes[n].cache_hit = nodes[n].cache_hit + 1;
			queryDelay (n, queryTime);
			break;

		case MSG_DATA:

			if (cacheCheck == -1){
				int cacheSize = nodes[n].cacheSize;

				if (cacheSize < CACHE_SIZE){

					nodes[n].client_cache[cacheSize].item_id = m->itemm.item_id;
					nodes[n].client_cache[cacheSize].updated_time = m->itemm.updated_time;
					nodes[n].client_cache[cacheSize].data = m->itemm.data;
					nodes[n].client_cache[cacheSize].item_type = m->itemm.item_type;

					nodes[n].usedTime[cacheSize] = clock;
					queryDelay (n, queryTime);
					nodes[n].cacheSize = nodes[n].cacheSize + 1;
					hold(0.001);
					
				} else {

					int cacheIndexCanBeReplaced = checkLRU(n);
					nodes[n].client_cache[cacheIndexCanBeReplaced].item_id = m->itemm.item_id;
					nodes[n].client_cache[cacheIndexCanBeReplaced].updated_time = m->itemm.updated_time;
					nodes[n].client_cache[cacheIndexCanBeReplaced].data = m->itemm.data;
					nodes[n].client_cache[cacheIndexCanBeReplaced].item_type = m->itemm.item_type;

					nodes[n].usedTime[cacheIndexCanBeReplaced] = clock;

					queryDelay (n, queryTime);
					hold(0.001);

					if (nodes[n].coldState == 0){

						nodes[n].coldState = 1;
						nodes[n].cache_hit = 0;
						nodes[n].numberOfQuery = 0;
						nodes[n].average_query_delay = 0.0;
					} 
				}

			} else {
				nodes[n].client_cache[cacheCheck].item_id = m->itemm.item_id;
				nodes[n].client_cache[cacheCheck].updated_time = m->itemm.updated_time;
				nodes[n].client_cache[cacheCheck].data = m->itemm.data;
				nodes[n].client_cache[cacheCheck].item_type = m->itemm.item_type;
				nodes[n].usedTime[cacheCheck] = clock;
				queryDelay (n, queryTime);
				hold(0.001);
			}
			break;

		default:
			break;
		}
}
void update_cold_data_item(){
	int randomItemToUpdate = random (HOT_DATA_ITEM_SIZE, DB_SIZE);
	if (serverDatabase[randomItemToUpdate].item_type == ITEM_COLD){
		serverDatabase[randomItemToUpdate].data = 456;
		serverDatabase[randomItemToUpdate].updated_time = clock;
		hold(0.001);
	}
}

void update_hot_data_item(){

	int randomItemToUpdate = random (0, HOT_DATA_ITEM_SIZE);

	if (serverDatabase[randomItemToUpdate].item_type == ITEM_HOT){
		serverDatabase[randomItemToUpdate].data = 123;
		serverDatabase[randomItemToUpdate].updated_time = clock;
		hold(0.001);
	}
}
void proc_server_update_item()
{
	create("serverprocupdate");
	while (clock < SIMTIME)
	{
		double x = uniform (0, 1);
		if (x > 0.33){
			update_hot_data_item();
			
		} else {
			update_cold_data_item();
		}
		hold(exponential(T_UPDATE));	
	}
}

void proc_server_reply() 
{
	create("serverprocresponse");
	while (clock < SIMTIME)
	{
		msg_t m;
		long s, t;
		receive(server_main.input, &m);
		if (m->type == MSG_REQUEST){
			int item_id = m->itemm.item_id;
			m->type = MSG_DATA;
			m->itemm.item_id = serverDatabase[item_id].item_id;
			m->itemm.updated_time = serverDatabase[item_id].updated_time;
			m->itemm.data = serverDatabase[item_id].data;
			m->itemm.item_type = serverDatabase[item_id].item_type;
			from_reply(m);
			send_msg(m);

		} else if (m->type == MSG_CHECK){
			int item_id = m->itemm.item_id;
			if (serverDatabase[item_id].updated_time >  m->itemm.updated_time){
				m->type = MSG_DATA;
				m->itemm.item_id = serverDatabase[item_id].item_id;
				m->itemm.updated_time = serverDatabase[item_id].updated_time;
				m->itemm.data = serverDatabase[item_id].data;
				m->itemm.item_type = serverDatabase[item_id].item_type;
				from_reply(m);
				send_msg(m);

			} else {
				m->type = MSG_CONFIRM;
				from_reply(m);
				send_msg(m);
			}
		}

		else{
		}
		
	}

}
void proc_client(n) long n;
{
	create("clientp");
	while (clock < SIMTIME)
	{
		create_query(n);
		hold(exponential(T_QUERY));
	}
}

void send_msg(m)
	msg_t m;
{
	if (m->type == MSG_DATA){
		hold(exponential(T_DELAY_LOAD));
	} else {
		hold(exponential(T_DELAY_MSG));
	}

	long from, to;
	from = m->from;
	to = m->to;
	if (to == -1){
		send(server_main.input, m);	
	} else {
		send(nodes[to].input, m);	
	}
}

msg_t client_query(from)
long from;
{
	msg_t m;
	long to = -1; 
	if (msg_queue == NIL)
	{
		m = (msg_t) do_malloc(sizeof(struct msg));
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
	double x1 = uniform (0, 1);
	if (x1 > 0.2){
		m->itemm.item_type = ITEM_HOT;
		int y1= random (0, HOT_DATA_ITEM_SIZE);
		m->itemm.item_id = y1;

	} else {
		m->itemm.item_type = ITEM_COLD;
		int y2 = random(HOT_DATA_ITEM_SIZE, DB_SIZE);
		m->itemm.item_id = y2;
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


void my_report()
{
	int totalQueries = 0;
	int totalCacheHit = 0;
	double queryDelay = 0.0;

	int i;
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		totalQueries = totalQueries + nodes[i].numberOfQuery;
		totalCacheHit = totalCacheHit + nodes[i].cache_hit;
		queryDelay = queryDelay + nodes[i].average_query_delay;
	}
	double averageTotalQueries = totalQueries/NUM_CLIENTS;
	double averageTotalCacheHit = totalCacheHit/NUM_CLIENTS;
	double averageQueryDelay = queryDelay/NUM_CLIENTS;

	printf("Average number of total queries: %lf \n", averageTotalQueries);
	printf("Average number of total cache hit: %lf \n", averageTotalCacheHit);
	printf("Average query delay: %lf \n", averageQueryDelay);
	
}


