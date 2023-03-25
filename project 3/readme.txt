Keep track of cached time which will be used by the LRU

total data 500
hot 50
cold 450


hot data item access probability 0.33 (33%)


For the cold data items, we need to choose an item uniformly. [ use the uniform() function]



Data update is done in the server side. How frequently it will be updated will be mentioned later by professor.



Result graphs:

1. 
y axis=cache hit
t_update = x-axis
t_update 5 attemts
t-query fixed to 10s


2. 
y axis=query delay
t_query = x-axis
t_query 5 attemts
t_update-fixed to 10s

3. 
y axis=query delay
t_update = x-axis
t_update 5 attemts
t-query fixed to 10s



number of query will be counted by each nodes.
if query is answered by local cache, we have a cachehit and it can be counted by each node.







Every client will have 1 process
Server should have a process

client will generate query based on the frequency of T_QUERY (5, 10, 15 .... ) taken as an input.
The program needs to be ran multiple times with different T_QUERY time


based on query probability we need to choose will that be a hot or cold data item (see slides how to determine hot/cold data item)

After that the client will send a request/check message to the server.
1. client will generate a query
2. [ message id, time stamp, message type ] while generating the query
3. The client will check the cache. If there is something there, it will send a check message. Or else it will send a request.
4. The server returns, a confirm(or check) message if the cache is valid in the client side
5. If the server find the cached copy to be invalid, it will send the data.
6. Based on the received data, the client will do cache management. If the cache is filled up, it will conduct cache replacement policy (LRU- least recently used).
7. 




message types:
1. request (from client to server)
2. check message (client to server)
3. confirm message (server to client)
4. data (server to client)


-----------------
T_UPDATE update interval (server side):


x = uniform (0, 1)
x > 0.33 -> cold data item
x < 0.33 -> hot data item

-----------------

hot/cold data item query:

x1 = uniform (0, 1)
x1 > 0.8 -> cold data item
x1 <= 0.8 -> hot data item

if hot data then, y1= uniform (1, 50), y1 is the item that needs to be queried. 


cached data should have a timestamp for lastupdate time stamp.



------------------
for the reuwest message, check and confirm message, it does not include any data. Data item id will be included. 
The size of these control messages are very small

There must be a transmission delay based on the given bandwidth.

descrive processing delay based on imagination on report

once a client generate a query and send  message and do not do anything unless receive a reply from the server.




node 1: we need to sum up all query delays and divide it with number of queries.
node 2: we need to sum up all query delays and divide it with number of queries.
node n: we need to sum up all query delays and divide it with number of queries.

calculate average query delay

----cache hit
when the client receives aconfirm message fro mthe server, the client determines cache hit.

cache miss = 1- cache hit

for client 1, 2 , ..., n each will ahve its average cache hit. We need to average that too.

Initially each client cache is empty. First 50 (atleast) query for each client will be a cache miss. 

