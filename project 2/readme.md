

in the project  we are going to have 5 mail boxes


sender will choose the receiver randomly


we need to include to and from (node numbers) in the hello message


dealing with time out -> re transmission. keep a count of how many times it was re transmitted (tracked by sender). After 2 times, you can stop resending. 


we can have two type of messages: hello and ack. For project 2, type is not required.



in the node struct, facility is included. Based on professor's experience, we can ignore that.


with malloc he has allocated memory. This needs to be cleared with most probably "clear()" or else, it will use up all the memory


you need to write my_report() function:
1. After running the number of successfull transmissions and failed transmissions should be displayed. Becasue of packet loss probablity, it needs to be changed 5 times and run the whole simulation 5 times.


It is not required to generate different stream of random number



For report, you can use a global variable to record successful or failed requests. Best idea is to have local variables for al the individual nodes to record that.




packet loss probabulity implementation:


uniform(0, 1) 

sender generates a random number. Lets say it is 0.15. if it is less than the given packetloss_probabiliity. 


it can be implemented on the receiver end too. 