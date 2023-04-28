#include "stdio.h"
#include "csim.h"
#include "string.h"

double SIMTIME = 15000.0;




#define NUM_NODES 5L
#define MSG_CONFIRM 2L
#define MSG_DATA 8L
#define MSG_REQUEST 11L
#define MSG_CHECK 3L
#define DB_SIZE 500
#define CACHE_SIZE 100
#define HOT_DATA_ITEM_SIZE 50
#define ITEM_COLD 6L
#define ITEM_HOT 5L




double T_LOAD_DELAY = 2.3; 
double T_MSG_DELAY = 0.6; 
double UPDATE_TIME;
double QUERY_TIME;


struct item
{
	int item_id;
	TIME last_updated_time;
	long data;
	int item_type;
} ;

struct item server_node_database[DB_SIZE];

typedef struct msg *msg_t;


struct msg
{
	long to;
	long from;
	msg_t link;
	TIME time_stamp;
	long type;
	struct item itemx;
};

msg_t msg_queue;

struct clnt
{
	double average_q_delay;
	TIME used_time[CACHE_SIZE];
	int cache_hit;
	struct item node_cache[CACHE_SIZE];
	int size_of_cache;
	MBOX mbox;
	int n_query;
	int is_cold_state;
};
	

struct clnt nodes[NUM_NODES];


typedef struct srvr
{
	MBOX mbox;
	
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
void proc_server_response() ;
int check_in_cache(msg_t m, long n);
int run_LRU(long n) ;

void sim(int argc, char *argv[] )
{
	printf("Please enter T update:\n");
	scanf("%lf", &UPDATE_TIME);
	printf("Please enter T query:\n");
	scanf("%lf", &QUERY_TIME);

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
	
	max_facilities(NUM_NODES * NUM_NODES + 1);
	max_servers(NUM_NODES * NUM_NODES);
	max_mailboxes(NUM_NODES + 1);
	max_events(4 * NUM_NODES );
	resp_tm = table("msg rsp tm");
	msg_queue = NIL;


	for (i = 0; i < NUM_NODES; i++)
	{
		sprintf(str, "mbox %d", i);
		nodes[i].mbox = mailbox(str);

		nodes[i].n_query = 0;
		nodes[i].cache_hit = 0;
		nodes[i].size_of_cache = 0;
		nodes[i].average_q_delay = 0.0;
		nodes[i].is_cold_state = 0;
		
	}
	sprintf(str, "mboxsrvr");
	server_main.mbox = mailbox(str);

	int i_db;
	for (i_db = 0; i_db <  DB_SIZE; i_db++){
		server_node_database[i_db].data = 663;
		server_node_database[i_db].item_id = i_db;

		server_node_database[i_db].last_updated_time = clock;

		if (i_db < HOT_DATA_ITEM_SIZE){
			server_node_database[i_db].item_type = ITEM_HOT;
		} else {
			server_node_database[i_db].item_type = ITEM_COLD;
		}
		hold(0.001);

	}
	
	for (i = 0; i < NUM_NODES; i++)
	{
		
		proc_client(i);
	}

	proc_server_update_item();
	proc_server_response();
}


int check_in_cache(m, n)
msg_t m; 
long n; {
	int i;
	int found = -1;
	for (i = 0; i < CACHE_SIZE; i++){

		if (nodes[n].node_cache[i].item_id == m->itemx.item_id){
			found = i;
			break;
		}
	}

	return found;
}


int run_LRU(n) long n;{
	TIME oldestTime = nodes[n].used_time[0];
	long oldestIndex = 0;
	int i ;
	for (i=0; i < CACHE_SIZE; i ++){

		if (nodes[n].used_time[i] < oldestTime){
			oldestTime = nodes[n].used_time[i];
			oldestIndex = i;
		}
	}
	return oldestIndex;
	
}

void calculate_q_delay(n, query_time) long n; long query_time;{
	TIME current_time = clock;
	double query_delay = current_time - query_time;
	nodes[n].average_q_delay = ((nodes[n].average_q_delay * ( nodes[n].n_query -1 )) + query_delay) / nodes[n].n_query;
}
void create_query(n) long n;{
	msg_t m;
	long t;
	m = client_query(n);
	TIME query_time = clock;
	int c_check = check_in_cache(m, n);
	if (c_check == -1){
		send_msg(m);
	} else {
		m->type = MSG_CHECK;
		m->itemx.last_updated_time = nodes[n].node_cache[c_check].last_updated_time;
		send_msg(m);
	}
	nodes[n].n_query = nodes[n].n_query + 1;
	receive(nodes[n].mbox, &m);
	t = m->type;
		switch (t)
		{
		case MSG_CONFIRM:

			hold(0.001);

			nodes[n].used_time[c_check] = clock;
			nodes[n].cache_hit = nodes[n].cache_hit + 1;
			calculate_q_delay(n, query_time);
			break;

		case MSG_DATA:

			if (c_check == -1){
				int size_of_cache = nodes[n].size_of_cache;

				if (size_of_cache < CACHE_SIZE){

					nodes[n].node_cache[size_of_cache].item_id = m->itemx.item_id;
					nodes[n].node_cache[size_of_cache].last_updated_time = m->itemx.last_updated_time;
					nodes[n].node_cache[size_of_cache].data = m->itemx.data;
					nodes[n].node_cache[size_of_cache].item_type = m->itemx.item_type;

					nodes[n].used_time[size_of_cache] = clock;
					calculate_q_delay(n, query_time);
					nodes[n].size_of_cache = nodes[n].size_of_cache + 1;
					hold(0.001);
					
				} else {

					int c_idx_replacement = run_LRU(n);
					nodes[n].node_cache[c_idx_replacement].item_id = m->itemx.item_id;
					nodes[n].node_cache[c_idx_replacement].last_updated_time = m->itemx.last_updated_time;
					nodes[n].node_cache[c_idx_replacement].data = m->itemx.data;
					nodes[n].node_cache[c_idx_replacement].item_type = m->itemx.item_type;

					nodes[n].used_time[c_idx_replacement] = clock;

					calculate_q_delay(n, query_time);
					hold(0.001);

					if (nodes[n].is_cold_state == 0){

						nodes[n].is_cold_state = 1;
						nodes[n].cache_hit = 0;
						nodes[n].n_query = 0;
						nodes[n].average_q_delay = 0.0;
					} 
				}

			} else {
				nodes[n].node_cache[c_check].item_id = m->itemx.item_id;
				nodes[n].node_cache[c_check].last_updated_time = m->itemx.last_updated_time;
				nodes[n].node_cache[c_check].data = m->itemx.data;
				nodes[n].node_cache[c_check].item_type = m->itemx.item_type;
				nodes[n].used_time[c_check] = clock;
				calculate_q_delay(n, query_time);
				hold(0.001);
			}
			break;

		default:
			break;
		}
}
void update_cold_data_item(){
	int rand_idx = random (HOT_DATA_ITEM_SIZE, DB_SIZE);
	if (server_node_database[rand_idx].item_type == ITEM_COLD){
		server_node_database[rand_idx].data = 456;
		server_node_database[rand_idx].last_updated_time = clock;
		hold(0.001);
	}
}

void update_hot_data_item(){

	int rand_idx = random (0, HOT_DATA_ITEM_SIZE);

	if (server_node_database[rand_idx].item_type == ITEM_HOT){
		server_node_database[rand_idx].data = 123;
		server_node_database[rand_idx].last_updated_time = clock;
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
		hold(exponential(UPDATE_TIME));	
	}
}

void proc_server_response() 
{
	create("serverprocresponse");
	while (clock < SIMTIME)
	{
		msg_t m;
		long s, t;
		receive(server_main.mbox, &m);
		if (m->type == MSG_REQUEST){
			int item_id = m->itemx.item_id;
			m->type = MSG_DATA;
			m->itemx.item_id = server_node_database[item_id].item_id;
			m->itemx.last_updated_time = server_node_database[item_id].last_updated_time;
			m->itemx.data = server_node_database[item_id].data;
			m->itemx.item_type = server_node_database[item_id].item_type;
			from_reply(m);
			send_msg(m);

		} else if (m->type == MSG_CHECK){
			int item_id = m->itemx.item_id;
			if (server_node_database[item_id].last_updated_time >  m->itemx.last_updated_time){
				m->type = MSG_DATA;
				m->itemx.item_id = server_node_database[item_id].item_id;
				m->itemx.last_updated_time = server_node_database[item_id].last_updated_time;
				m->itemx.data = server_node_database[item_id].data;
				m->itemx.item_type = server_node_database[item_id].item_type;
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
		hold(exponential(QUERY_TIME));
	}
}

void send_msg(m)
	msg_t m;
{
	if (m->type == MSG_DATA){
		hold(exponential(T_LOAD_DELAY));
	} else {
		hold(exponential(T_MSG_DELAY));
	}

	long from, to;
	from = m->from;
	to = m->to;
	if (to == -1){
		send(server_main.mbox, m);	
	} else {
		send(nodes[to].mbox, m);	
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
		m->itemx.item_type = ITEM_HOT;
		int y1= random (0, HOT_DATA_ITEM_SIZE);
		m->itemx.item_id = y1;

	} else {
		m->itemx.item_type = ITEM_COLD;
		int y2 = random(HOT_DATA_ITEM_SIZE, DB_SIZE);
		m->itemx.item_id = y2;
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
	int total_node_queries = 0;
	int total_hit = 0;
	double query_delay = 0.0;

	int i;
	for (i = 0; i < NUM_NODES; i++)
	{
		query_delay = query_delay + nodes[i].average_q_delay;
		total_node_queries = total_node_queries + nodes[i].n_query;
		total_hit = total_hit + nodes[i].cache_hit;
	}
	double avg_hit = total_hit/NUM_NODES;
	double avg_queries = total_node_queries/NUM_NODES;
	double avg_delay = query_delay/NUM_NODES;

	printf("Avg delay: %lf \n", avg_delay);
	printf("Avg queries: %lf \n", avg_queries);
	printf("Avg cache hit: %lf \n", avg_hit);
	
}


