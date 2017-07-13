#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define SERVER_PORT 57120
#define MAX_LINE 256
#define MAX_PENDING 5
#define MAX_NAME 100

/* structure of the packet */
struct registration_packet {
	short type;
	char data[MAX_NAME];
};

/* structure of the data packet */
struct data_packet {
	short header;
	char data[MAX_NAME];
};

/* structure of the registration table */
struct global_table {
	int sockid;
	int reqno;
};

/* structure of Registration Table */
struct registrationTable {
	int port;
	char name[MAX_NAME];
};

struct global_table records[20];

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

/* join_handler functions that handles the second and third registration requests */
void *join_handler(struct global_table *rec) {
	int newsock, i;
	struct registration_packet packet_reg;
	newsock = rec->sockid;
	/* two more registration requests needed */
	while(rec->reqno != 3){
		/* receive the packet from the client */
		if(recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n Could not receive \n");
			exit(1);
		}
		/* check the type to validate registration packet */
		if(ntohs(packet_reg.type) == 121) {
			printf("\n Received REGISTRATION Request %d \n", rec->reqno +1);
			/* change so that a repeat does not occur without actual input from client */
			packet_reg.type = 0;
			pthread_mutex_lock(&my_mutex);
			rec->reqno++;
			pthread_mutex_unlock(&my_mutex);
		}
	}
	/* send the confirmation packet to the client */
	packet_reg.type = htons(221);
	if(send(newsock, &packet_reg, sizeof(packet_reg), 0) <0) {
		printf("\n Send failed\n");
		exit(1);
	}
	printf("\n Sending REGISTRATION Confirmation \n");
	/* leaving the thread */
	pthread_exit(0);
}

/* function used to handle the multicaster to the client */
void *multicaster() {
	FILE *fp;
	char *filename;
	char text[100];
	struct data_packet filedata;
	int i, s, found;
	int counter = 1;

	filename = "multicasterinput.txt";
	fp = fopen(filename, "r");

	/* loop indefinitely */
	while(1) {
		found = 0;
		/* check to see if there is a client that can receive the signal */
		for (i = 0; i < 20; i++) {
			//printf("\n%d\n", records[i].reqno);
			/* break out of for loop and handle clients if they are detected */
			if (records[i].reqno == 3) {
				found = 1;
				break;
			}
			else {
				found = 0;
			}
		}
		/* if a single client is found to be valid */
		if (found) {
			fgets(text, 100, (FILE*)fp);
			/* build the data packet */
			filedata.header = counter;
			strcpy(filedata.data, text);
			/* send the packet to each valid client */
			for (i = 0; i < 20; i++) {
				if (records[i].reqno == 3) {
					if(send(records[i].sockid, &filedata, sizeof(filedata), MSG_NOSIGNAL) <0) {
						close(records[i].sockid);
						records[i].reqno = 0;
					}
				}
			}
			counter++;
			sleep(1);
		}
	}
}

int main(int argc, char* argv[]) {

	pthread_t threads[2];
	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	struct registrationTable table[20];
	struct registration_packet packet_reg;
	struct data_packet packet;
	struct hostent *hp;
	char *address;
	int s, new_s, len, i;
	int index = 0;
	void* exit_value;
	struct global_table client_info;

	/* initialize the multicaster thread */
	pthread_create(&threads[1], NULL, multicaster, NULL);

	/* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpserver: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);

	/* wait for connection, then receive and print text */
	while(1){
		if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
			perror("tcpserver: accept");
			exit(1);
		}
		/* server receives the packet from the client */
		if(recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n Could not receive packet \n");
			exit(1);
		}
		/*************** handles the registration of the client *******************/
		/* check the type of the packet to make sure it is intended for registration */
		if(ntohs(packet_reg.type) == 121) {
			/* register the client to the registration table */
			table[index].port = ntohs(clientAddr.sin_port);
			strcpy(table[index].name, packet_reg.data);
			/* print the information from the registration request */
			printf("\n Received REGISTRATION Request 1:");
			printf("\n ---Client's port is %d", ntohs(clientAddr.sin_port));
			printf("\n ---Client's machine name is %s\n", packet_reg.data);
			/* add the client to the record table so for additional registrations */
			client_info.sockid = new_s;
			client_info.reqno = 1;
			/* create the join_handler thread to handle second/third registrations */
			pthread_create(&threads[0], NULL, (void*)join_handler, &client_info);
			pthread_join(threads[0], &exit_value);
			/* increment for the next client that attempts to register */
			pthread_mutex_lock(&my_mutex);
			records[index].sockid = client_info.sockid;
			records[index].reqno = client_info.reqno;
			pthread_mutex_unlock(&my_mutex);
			index++;
		}
	}
}
