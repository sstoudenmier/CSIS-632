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

#define SERVER_PORT 6990
#define MAX_LINE 256
#define MAX_PENDING 5
#define MAX_NAME 100

/* structure of the packet */
struct packet {
	short type;
	char data[MAX_NAME];
};

/* structure of Registration Table */
struct registrationTable {
	int port;
	char name[MAX_NAME];
};

int main(int argc, char* argv[]) {

	struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
	struct registrationTable table[10];
	struct packet packet_reg;
	struct hostent *hp;
	char *address;
	int s, new_s;
	int len;
	int index = 0;

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
			/* loop while the client is running */
			while(1) {
			/* server receives the packet from the client */
			if(recv(new_s, &packet_reg, sizeof(packet_reg), 0) < 0) {
	 			printf("\n Could not receive packet \n");
	 			exit(1);
	 		}
			/******************** REGISTERATION OF THE CLIENT **************************/
			/* check the type of the packet to make sure it is intended for registration */
			if(ntohs(packet_reg.type) == 121) {
				/* register the client to the registration table */
				table[index].port = ntohs(clientAddr.sin_port);
				strcpy(table[index].name, packet_reg.data);
				/* print the information from the registration request */
				printf("\n Received REGISTRATION request from Client:");
				printf("\n ---Client's port is %d", ntohs(clientAddr.sin_port));
				printf("\n ---Client's machine name is %s\n", packet_reg.data);
				printf("\n Sending confirmation to client \n");
				/* set the type of the packet to match that of a confirmation */
				packet_reg.type = htons(221);
				/* send the confirmation packet to the client */
			 	if(send(new_s, &packet_reg, sizeof(packet_reg), 0) <0) {
					printf("\n Send failed\n");
					exit(1);
				}
				/* increment for the next client that attempts to register */
				index++;
			}
			/***************************************************************************/
			/******************** ADDR Request From the Client *************************/
			/* check the type of the packet to make sure it is intended for ADDR Request */
			else if(ntohs(packet_reg.type) == 131) {
				/* notify that request was recieved */
				printf("\n Received ADDR Request\n");
				/* pull the machine name to look up dotted decimal address for */
				hp = gethostbyname(packet_reg.data);
				address = inet_ntoa(*(struct in_addr *) hp->h_addr);
			  //address = inet_ntoa(*(struct in_addr *)hp->h_name);
				strcpy(packet_reg.data, address);
				/* change the type to match the ADDR Response */
				packet_reg.type = htons(231);
				/* send the ADDR response packet to the client */
				printf("\n Sending ADDR Response\n");
			 	if(send(new_s, &packet_reg, sizeof(packet_reg), 0) <0) {
					printf("\n Send failed\n");
					exit(1);
				}
				/* the client has finished so the server is done communicating with it */
				printf("\n*****************************************\n");
				break;
			}
		}
		/***************************************************************************/

		/* close the socket that received the signal */
		close(new_s);
	}
}
