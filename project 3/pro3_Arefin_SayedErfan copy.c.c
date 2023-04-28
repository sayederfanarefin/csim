
/* Project 3; Arefin, Sayed Erfan */

#include "csim.h"
#include "stdio.h"
#include "string.h"

// atleast 5000 SIMTIME. It is recommended to double or triple it 15000.0
double SIMTIME = 50000.0;
#define NUM_CLIENTS 5L
// #define NUM_CLIENTS 5L

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


double T_UPDATE;
double T_QUERY;
double T_DELAY_LOAD = 2.4192; // 1 sec for data transfer // 2.4192  (16000 + (1024 * 8))/10000
double T_DELAY_MSG = 0.8197; // 0.0001 sec for just message transfer  // 0.8197   (5 + (1024 * 8))/10000
// 0 for server
// FACILITY network[NUM_CLIENTS];


long item_hot_updated_count;
long item_cold_updated_count;

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
	// FACILITY cpu;
	MBOX input;
	int numberOfQuery;
	int cache_hit;
	double average_query_delay;

	struct item client_cache[CACHE_SIZE];
	TIME usedTime[CACHE_SIZE];
	int cacheSize;
	int coldState; // cold state 0=false, 1 = true
};
	

struct clnt client[NUM_CLIENTS];


typedef struct srvr
{
	// FACILITY cpu;
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
msg_t clientQuery();
void createQuery(long n);
void updateColdDataItem();
void updateHotDataItem();
void procServerUpdateItem();
void procClient(long n);
void procServerReply() ;


void sim(int argc, char *argv[] )
{
	printf("-----------======================-------------\n");

	// param 1: T_UPDATE;
	// param 2: T_QUERY;

	if( argc == 3 ) {
		
		sscanf(argv[1], "%lf", &T_UPDATE);
		sscanf(argv[2], "%lf", &T_QUERY);

		if (T_UPDATE > 0.1 ){
			
			if ( T_QUERY > 0.1){
				// // printf("Starting..");
				create("sim");
				init();
				hold(SIMTIME);
				my_report();

				free(msg_queue);

			} else {
				printf("T_QUERY should be greater than 0.1.\n");
			}

		} else {
			printf("T_UPDATE should be greater than 0.1.\n");
		}
	}
	else if( argc > 3 ) {
		printf("Too many arguments supplied.\n");
	}
	else {
		printf("Two arguments expected.\n");
	}
}

void init()
{

	item_hot_updated_count = 0;
    item_cold_updated_count = 0;


	long i, j;
	char str[24];
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	
	 max_events(NUM_CLIENTS * NUM_CLIENTS * 100 + NUM_CLIENTS);
    max_mailboxes(NUM_CLIENTS * NUM_CLIENTS * 100 + NUM_CLIENTS);                     
    max_messages(NUM_CLIENTS * NUM_CLIENTS * 100 + NUM_CLIENTS);

	// max_facilities(NUM_CLIENTS * NUM_CLIENTS + 1);
	// max_servers(NUM_CLIENTS * NUM_CLIENTS);
	// max_mailboxes(NUM_CLIENTS + 1);
	// max_events(4 * NUM_CLIENTS );
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;


	for (i = 0; i < NUM_CLIENTS; i++)
	{
		// sprintf(str, "cpu %d", i);
		// client[i].cpu = facility(str);
		sprintf(str, "input %d", i);
		client[i].input = mailbox(str);

		client[i].numberOfQuery = 0;
		client[i].cache_hit = 0;
		client[i].cacheSize = 0;
		client[i].average_query_delay = 0.0;
		client[i].coldState = 0;
		
	}

	// for (i = 0; i < NUM_CLIENTS; i++)
	// {
	// 		sprintf(str, "nt %d", i);
	// 		network[i] = facility(str);
	// }

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
		// printf ("db[%d]= %lld\n", i_db, serverDatabase[i_db].updated_time );

		
		if (i_db < HOT_DATA_ITEM_SIZE){
			serverDatabase[i_db].item_type = ITEM_HOT;
		} else {
			serverDatabase[i_db].item_type = ITEM_COLD;
		}
		hold(2.0);

	}
	

	// printf ("Server data base check %lld\n", clock);
	// printf ("Server data base check %lf\n", clock);
	// printf ("simtime %lf\n", simtime());
	// TIME yy = clock ;
	// printf ("clock TIME var %lld\n", yy);


	// TIME xx = simtime();
	// printf ("simtime TIME var %lld\n", xx);


	// double x = simtime();
	// printf ("simtime var %lf\n", x);


	// for (i_db = 0; i_db <  DB_SIZE; i_db++){
	// 	printf ("db[%d]= %lld\n", i_db, serverDatabase[i_db].updated_time );
	// }

	// printf("Running client processes\n");
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		procClient(i);
	}

	procServerUpdateItem();
	procServerReply();
}


int checkCache(m, n)
msg_t m; 
long n; {
	// reply 0 if not present in cache
	// reply item index if present in cache

	int i;
	int found = -1;
	for (i = 0; i < CACHE_SIZE; i++){

		if (client[n].client_cache[i].item_id == m->itemm.item_id){
			found = i;
			break;
		}
	}

	return found;
}

int printCache(){

	printf("--------------------- printing all cache -------------------------\n");
	printf("------------------------------------------------------------------\n");

	int x ;
	for (x = 0; x < NUM_CLIENTS; x++){

		printf("Node: %lld\n", x);

		int i;
		for (i=0; i < CACHE_SIZE ; i++){

			printf ("cache[%d]=%d | ", i, &client[x].client_cache[i].item_id );
		}
		printf("\n");

	}
	printf("------------------------------------------------------------------\n");
	printf("------------------------------------------------------------------\n");
}

int printUsedTime(){

	printf("--------------------- printing used time -------------------------\n");
	printf("------------------------------------------------------------------\n");

	int x ;
	for (x = 0; x < NUM_CLIENTS; x++){

		printf("Node: %lld\n", x);

		int i;
		for (i=0; i < CACHE_SIZE ; i++){

			printf ("usedTime[%d]=%lld | ", i, &client[x].usedTime[i] );
		}
		printf("\n");

	}
	printf("------------------------------------------------------------------\n");
	printf("------------------------------------------------------------------\n");
}


int checkLRU(n) long n;{
	// get the index of the least recently used 
	// long currentTime = clock;
	TIME oldestTime = clock;
	long oldestIndex = -1;
	int i ;
	// printf("client %d, ------------------------------------\n",n);
	// printf ("client %d, Checking LRU index: \n",n);
	// printf ("client %d, init: Oldest time (current time): %lld \n",n, oldestTime);

	for (i=0; i < CACHE_SIZE; i ++){

		if ((client[n].usedTime[i] != 0) && (client[n].usedTime[i] < oldestTime)){
			//printf ("client %d, loop: Oldest time (current time): %lld, client cache %d used time: %lld \n",n, oldestTime, i, &client[n].usedTime[i]);
			oldestTime = client[n].usedTime[i];
			
			oldestIndex = i;
			
		}
		// printf ("client %d, loop: i-> %d, cache-size-> %d | Oldest time: %lld, last used time %lld\n", n, i ,CACHE_SIZE,oldestTime, client[n].usedTime[i]);
		
	}
	// printf("client %d, finally selected cache idx: %d and oldest time: %lld \n",n, oldestIndex, oldestTime);

	// printf("client %d, ------------------------------------\n",n);
	

	return oldestIndex;
	
}

void queryDelay (n, queryTime) long n; long queryTime;{
	TIME currentTime = clock;
	long queryDelay = currentTime - queryTime;
	client[n].average_query_delay = ((client[n].average_query_delay * ( client[n].numberOfQuery -1 )) + queryDelay) / client[n].numberOfQuery;

}

void createQuery(n) long n;{

	msg_t m;
	long t;
	m = clientQuery(n);

	TIME queryTime = clock;
	
	int cacheCheck = checkCache(m, n);

	if (cacheCheck == -1){
		send_msg(m);
		//printf("Client %d: send MSG_REQUEST\n", n);
	} else {

		m->type = MSG_CHECK;
		m->itemm.updated_time = client[n].client_cache[cacheCheck].updated_time;
		
		send_msg(m);
		//printf("Client %d: send MSG_CHECK\n", n);
	}

	client[n].numberOfQuery = client[n].numberOfQuery + 1;
	

	receive(client[n].input, &m);
	t = m->type;
		switch (t)
		{
		case MSG_CONFIRM:
			
			//printf("Client %d: received MSG_CONFIRM\n", n);
			
			// update recently used time
			client[n].usedTime[cacheCheck] = clock;
			// printf("");

			client[n].cache_hit = client[n].cache_hit + 1;

			queryDelay (n, queryTime);
			// if (n==1){
			// 	printf("Reusing cache. Client: %d.\n", n);
			// }
			break;

		case MSG_DATA:

			if (cacheCheck == -1){
				// MSG_DATA for MSG_REQUEST
				//printf("Client %d: received MSG_DATA\n", n);
				int cacheSize = client[n].cacheSize;
				if (cacheSize < CACHE_SIZE){
					client[n].client_cache[cacheSize].item_id == m->itemm.item_id;
					client[n].client_cache[cacheSize].updated_time == m->itemm.updated_time;
					client[n].client_cache[cacheSize].data == m->itemm.data;
					client[n].client_cache[cacheSize].item_type == m->itemm.item_type;
					// update recently used time
					client[n].usedTime[cacheCheck] = clock;
					queryDelay (n, queryTime);
					client[n].cacheSize = client[n].cacheSize + 1;
					// if (n==1){
					// 	printf("Client:%d -> Not present in cache, storing in cache, cache size: %d\n", n, cacheSize);

					// }

				} else {
					
			
					//run LRU
					int cacheIndexCanBeReplaced = checkLRU(n);
					client[n].client_cache[cacheIndexCanBeReplaced].item_id == m->itemm.item_id;
					client[n].client_cache[cacheIndexCanBeReplaced].updated_time == m->itemm.updated_time;
					client[n].client_cache[cacheIndexCanBeReplaced].data == m->itemm.data;
					client[n].client_cache[cacheIndexCanBeReplaced].item_type == m->itemm.item_type;
					// update recently used time
					client[n].usedTime[cacheIndexCanBeReplaced] = clock;
					queryDelay (n, queryTime);
					// if (n==1){
					// 	printf("LRU -> cache index least used: %d.\n", cacheIndexCanBeReplaced);
					// }

					if (client[n].coldState == 0){
						// cold state will be when the cache is filled up.
						// The number of queries and cache hits should be re-initialized to zero when you remove the cold state.  
			
						client[n].coldState = 1;
						client[n].cache_hit = 0;
						client[n].numberOfQuery = 0;
						client[n].average_query_delay = 0.0;
						// if (n==1){
						// 	printf("Cold state removed for client %d, case size of the clients is %d\n", n, cacheSize);
						// }

						//printUsedTime();
						//printCache();
					}

				}

			} else {
				// MSG_DATA for invalid data
				client[n].client_cache[cacheCheck].item_id == m->itemm.item_id;
				client[n].client_cache[cacheCheck].updated_time == m->itemm.updated_time;
				client[n].client_cache[cacheCheck].data == m->itemm.data;
				client[n].client_cache[cacheCheck].item_type == m->itemm.item_type;
				// update recently used time
				client[n].usedTime[cacheCheck] = clock;
				queryDelay (n, queryTime);
			}
			break;

		default:
			printf("***unexpected type");
			break;
		}

}

void updateColdDataItem(){

    item_cold_updated_count = item_cold_updated_count + 1;

	int randomItemToUpdate = random (HOT_DATA_ITEM_SIZE, DB_SIZE);

	if (serverDatabase[randomItemToUpdate].item_type == ITEM_COLD){
		serverDatabase[randomItemToUpdate].data = 345345345;
		serverDatabase[randomItemToUpdate].updated_time = clock;
		// printf ("-------------------> %lld\n", serverDatabase[randomItemToUpdate].updated_time);
		// printf ("--------------------------> %lld\n", clock);
	}

	// int ii;
	// for (ii = HOT_DATA_ITEM_SIZE; ii < DB_SIZE; ii++){
	// 	if (serverDatabase[ii].item_type == ITEM_COLD){
	// 		serverDatabase[ii].data = zipf(ii);
	// 		serverDatabase[ii].updated_time = clock;
	// 	}
	// }
}

void updateHotDataItem(){
	item_hot_updated_count = item_hot_updated_count + 1;

	int randomItemToUpdate = random (0, HOT_DATA_ITEM_SIZE);

	if (serverDatabase[randomItemToUpdate].item_type == ITEM_HOT){
		serverDatabase[randomItemToUpdate].data = 345345345;
		serverDatabase[randomItemToUpdate].updated_time = clock;

		// printf ("-------------------> %lld\n", serverDatabase[randomItemToUpdate].updated_time);
		// printf ("--------------------------> %lld\n", clock);
	}


	// // printf("Updating hot data item\n");
	// int ii;
	// for (ii =0; ii < HOT_DATA_ITEM_SIZE; ii++){
	// 	if (serverDatabase[ii].item_type == ITEM_HOT){
	// 		serverDatabase[ii].data = zipf(ii);
	// 		serverDatabase[ii].updated_time = clock;
	// 	}
	// }

}
void procServerUpdateItem()
{
	// printf("Running server item update process\n");
	create("procServerUpdateItem");
	while (clock < SIMTIME)
	{
		// printf ("--------------------------> %lld\n", clock);

		double x = uniform (0, 1);
		if (x > 0.33){
			// update -> hot data item
			updateHotDataItem();
			
		} else {
			// update -> cold data item
			updateColdDataItem();
		}
		hold(exponential(T_UPDATE));	
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
		

		if (m->type == MSG_REQUEST){
			int item_id = m->itemm.item_id;
			m->type = MSG_DATA;
			m->itemm.item_id = serverDatabase[item_id].item_id;
			m->itemm.updated_time = serverDatabase[item_id].updated_time;
			m->itemm.data = serverDatabase[item_id].data;
			m->itemm.item_type = serverDatabase[item_id].item_type;
			from_reply(m);
			send_msg(m);
			//printf("Server: replied MSG_DATA\n");

		} else if (m->type == MSG_CHECK){
			// printf("server received msg_check\n");
			int item_id = m->itemm.item_id;
			long updated_time = m->itemm.updated_time;
			// check if data is old or not

			// printf("Server end message check: received item updated time: %ld, actual updated time: %ld\n", updated_time, serverDatabase[item_id].updated_time);

			// printf ("----------> %lld\n", updated_time);
			// printf ("-------------------> %lld\n", serverDatabase[item_id].updated_time);
			// printf ("--------------------------> %lld\n", clock);

			if (serverDatabase[item_id].updated_time > updated_time){
				m->type = MSG_DATA;
				m->itemm.item_id = serverDatabase[item_id].item_id;
				m->itemm.updated_time = serverDatabase[item_id].updated_time;
				m->itemm.data = serverDatabase[item_id].data;
				m->itemm.item_type = serverDatabase[item_id].item_type;
				from_reply(m);
				send_msg(m);
				//printf("Server: replied MSG_DATA\n");

			} else {
				// printf("server returns msg_confirm\n");
				m->type = MSG_CONFIRM;
				from_reply(m);
				send_msg(m);
				//printf("Server: replied MSG_CONFIRM\n");
			}
		}

		else{
			// do nothing
		}
		
	}

}
void procClient(n) long n;
{
	
	create("procClient");
	// printf("Client %d: ------- \n", n);
	
	while (clock < SIMTIME)
	{
		// printf("Client %d: Inside process\n", n);
		createQuery(n);
		// printf("Client %d: done querying......... next!\n", n);
		hold(exponential(T_QUERY));
		// printf("Client %d: done querying......... next!   after hold period T_QUERY\n", n);
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



// 	hot/cold data item query:

// x1 = uniform (0, 1)
// x1 > 0.8 -> cold data item
// x1 <= 0.8 -> hot data item

// if hot data then, y1= uniform (1, 50), y1 is the item that needs to be queried. 
	double x1 = uniform (0, 1);

	if (x1 > 0.2){

		// hot data item
		m->itemm.item_type = ITEM_HOT;

		int y1= random (0, HOT_DATA_ITEM_SIZE);
		m->itemm.item_id = y1;

	} else {

		// cold data item
		m->itemm.item_type = ITEM_COLD;


		// for cold data item type lets select any id, randomly
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

int file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    int is_exist = 0;
    if (fp != NULL)
    {
        is_exist = 1;
        fclose(fp); // close the file
    }
    return is_exist;
}


void my_report()
{
	printf("Total hot data item count: %d \n", item_hot_updated_count);
	printf("Total cold data item count: %d \n", item_cold_updated_count);

	int totalQueries = 0;
	int totalCacheHit = 0;
	double queryDelay = 0.0;

	int i;
	for (i = 0; i < NUM_CLIENTS; i++)
	{
		totalQueries = totalQueries + client[i].numberOfQuery;
		totalCacheHit = totalCacheHit + client[i].cache_hit;
		queryDelay = queryDelay + client[i].average_query_delay;
		
	}

	double averageTotalQueries = totalQueries/NUM_CLIENTS;
	double averageTotalCacheHit = totalCacheHit/NUM_CLIENTS;
	double averageQueryDelay = queryDelay/NUM_CLIENTS;

	printf("Total queries: %d \n", totalQueries);
	printf("Total cache hit: %d \n", totalCacheHit);
	printf("Query delay: %lf \n", queryDelay);
	printf("Average number of total queries: %lf \n", averageTotalQueries);
	printf("Average number of total cache hit: %lf \n", averageTotalCacheHit);
	printf("Average query delay: %lf \n", averageQueryDelay);

  	FILE *fp;

	char *filename = "results.csv";
	int file_result = file_exists(filename);

	fp = fopen(filename, "a+");


	if (file_result == 0){
		fprintf(fp, "t_update, t_query, avg_total_queries, avg_total_cache_hit, " "cahce_hit_ratio, avg_query_delay\n");
	}

	
	fprintf(fp, "%lf, %lf, %lf, %lf, %lf, %lf\n", T_UPDATE, T_QUERY,
			averageTotalQueries, averageTotalCacheHit,
			 (averageTotalCacheHit/ averageTotalQueries) * 100, averageQueryDelay);
	// fputs("This is testing for fputs...\n", fp);
	fclose(fp);
}


