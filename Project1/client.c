#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

#define SERVER_PORT 6990
#define MAX_LINE 256
#define MAX_NAME 100


/* structure of the packet */
struct packet {
	short type;
	char data[MAX_NAME];
};

int main(int argc, char* argv[])
{

	struct hostent *hp;
	struct sockaddr_in sin;
	struct packet packet_reg;
	char *host;
	char buf[MAX_LINE];
	char clientName[MAX_NAME];
	int s;
	int len;

	if(argc == 3){
		host = argv[1];
	}
	else{
		fprintf(stderr, "usage:newclient server\n");
		exit(1);
	}

	/* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

	/* active open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}

	/* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	/************* handle the registration of the client to the server ***********/
	/* assigning values to the registration packet */
	gethostname(clientName, sizeof(clientName));
	packet_reg.type = htons(121);
	strcpy(packet_reg.data, clientName);
	/* Send the registration packet to the server */
	printf("\n Sending REGISTRATION Request \n");
 	if(send(s, &packet_reg, sizeof(packet_reg), 0) <0) {
		printf("\n ---Send failed\n");
		exit(1);
	}
	/* wait on confirmation from the server */
	while(1) {
		/* client receives the packet from the server */
		if(recv(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n ---Could not receive confirmation packet \n");
			exit(1);
		}
		/* check the type to validate confirmation packet */
		if(ntohs(packet_reg.type) == 221) {
			printf("\n Received REGISTRATION Confirmation\n");
			break;
		}
	}
	/*****************************************************************************/

	/************* handle the ADDR Request and Response ***********/
	/* assigning values to the registration packet */
	packet_reg.type = htons(131);
	strcpy(packet_reg.data, argv[2]);
	/* Send the registration packet to the server */
	printf("\n Sending ADDR Request \n");
 	if(send(s, &packet_reg, sizeof(packet_reg), 0) <0) {
		printf("\n ---Send failed\n");
		exit(1);
	}
	/* wait on addr response from the server */
	while(1) {
		/* client receives the packet from the server */
		if(recv(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n ---Could not receive confirmation packet \n");
			exit(1);
		}
		/* check the type to validate ADDR response packet */
		if(ntohs(packet_reg.type) == 231) {
			printf("\n Received ADDR Reponse \n");
			printf("\n %s has dotted decimal address of %s \n\n", argv[2], packet_reg.data);
			break;
		}
	}
	/*****************************************************************************/


}
