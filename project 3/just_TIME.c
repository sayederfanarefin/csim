
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

#define DB_SIZE 5
#define CACHE_SIZE 100
#define HOT_DATA_ITEM_SIZE 50


double T_UPDATE;
double T_QUERY;
double T_DELAY_LOAD = 2.4192; // 1 sec for data transfer // 2.4192  (16000 + (1024 * 8))/10000
double T_DELAY_MSG = 0.8197; // 0.0001 sec for just message transfer  // 0.8197   (5 + (1024 * 8))/10000
// 0 for server
FACILITY network[NUM_CLIENTS];



struct item
{
	int item_id;
	TIME updated_time;
};

struct item serverDatabase[DB_SIZE];

TIME update_time_2[DB_SIZE];



typedef struct srvr
{
	FACILITY cpu;
	MBOX input;
	
} server_i;

server_i server_main;

TABLE resp_tm;
FILE *fp;

void init();


void sim(int argc, char *argv[] )
{
	
	create("sim");
	init();
	hold(SIMTIME);
}

void init()
{


	long i, j;
	char str[24];
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	
	max_facilities(NUM_CLIENTS * NUM_CLIENTS + 1);
	max_servers(NUM_CLIENTS * NUM_CLIENTS);
	max_mailboxes(NUM_CLIENTS + 1);
	max_events(4 * NUM_CLIENTS );
	resp_tm = table("msg rsp tm");
	// msg_queue = NIL;



	for (i = 0; i < NUM_CLIENTS; i++)
	{
			sprintf(str, "nt %d", i);
			network[i] = facility(str);
	}

	sprintf(str, "cpusrvr");
	server_main.cpu = facility(str);
	sprintf(str, "inputsrvr");

	server_main.input = mailbox(str);

	// update_time_2[0] = clock;
	// update_time_2[1] = clock;
	// update_time_2[2] = clock;

	// int xxxxx = 0;
	// while (xxxxx < DB_SIZE){
	// 	update_time_2[xxxxx] = clock;

	// 	xxxxx = xxxxx+1;
	// 	hold(2.1);

	// }

	// printf ("db[0]=  %ld\n", update_time_2[0] );
	// printf ("db[1]=  %ld\n", update_time_2[1] );
	// printf ("db[2]=  %ld\n", update_time_2[2] );
	// printf ("db[3]=  %ld\n", update_time_2[3] );
	// printf ("db[4]=  %ld\n", update_time_2[4] );

	// int xxxxx = 0;
	// xxxxx = 0;
	// while (xxxxx < DB_SIZE){
	// 		printf ("db[%d]=  %ld\n", xxxxx, update_time_2[xxxxx] );
	// 	hold(2.0);
	// 	xxxxx = xxxxx+1;

	// }

	// int ii;
	// for (ii = 0; ii < DB_SIZE; ii++) {
    //     printf ("db[%d]=  %ld\n", ii, update_time_2[ii] );
    // }

	// update_time_2[ii] = clock;
	// update_time_2[ii] = clock;
	// update_time_2[ii] = clock;
	// update_time_2[ii] = clock;

	// TIME yy = clock ;
	// printf ("clock TIME var %lld\n", yy);

	// itemii.item_id = 230;
	// itemii.updated_time = yy;

	// printf (" %ld\n", itemii.updated_time );
	// printf ("%d\n", itemii.item_id );

	long ii;
    for (ii = 0; ii < DB_SIZE; ii++) {
        serverDatabase[ii].item_id = ii;
        serverDatabase[ii].updated_time = clock;
		hold(2.0);
		
    }


	int i_db;
	for (i_db = 0; i_db <  DB_SIZE; i_db++){
		
		// serverDatabase[i_db].item_id = i_db;
		// TIME currentTimexoxo = clock;
		// serverDatabase[i_db].updated_time = 10;
		// serverDatabase[i_db].updated_time = clock;
		printf ("db[%d]=  %ld\n", i_db, update_time_2[i_db] );
		// printf ("db[%d]=  %lf\n", i_db, update_time_2[i_db] );
		hold(2.0);
		// printf ("db[%d]=  %ld\n", i_db, &serverDatabase[i_db].updated_time );
		// printf ("db[%d]=  %ld\n", i_db, serverDatabase[i_db].updated_time );
		// printf ("db[%d]= %d\n", i_db, serverDatabase[i_db].item_id );

	}
	
}
