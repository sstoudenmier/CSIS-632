#Project 4
---
In order to run this Assignment you will first need to start up the server with the following:

./server

After the server is running you can connect up to 30 clients at one time. To start a client you will have to use the following:

./client "server_name" "user_name" "group_number"

In the line above the server name is the IP Address of the server, the user name is a string (without spaces) that will be used to represent a clients message to other clients in a group. The group number is an integer that is used to group the clients together.

Below is a full example of how to start up the server and run two clients on group 3 with user names of Seth and Bob. Additionally, there will be a third user added, Larry, who will be in a separate group. Larry will be able to see his own messages, but not Seth and Bob’s since he is in his own group.

**Example**

Server:
	./server

Client1:
	./client 192.168.0.16 Seth 3

Client2:
	./client 192.168.0.16 Bob 3

Client3:
	./client 192.168.0.16 Larry 8
