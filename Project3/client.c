#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

#define SERVER_PORT 57120
#define MAX_LINE 256
#define MAX_NAME 100

/* structure of the registration packet */
struct registration_packet {
	short type;
	char data[MAX_NAME];
	int sockid;
};

/* structure of the data packet */
struct data_packet {
	short header;
	char data[MAX_NAME];
};

int main(int argc, char* argv[])
{
	struct hostent *hp;
	struct sockaddr_in sin;
	struct registration_packet packet_reg;
	struct data_packet packet;
	char *host;
	char clientName[MAX_NAME];
	int s, i, packetLimit, temp;

	if(argc == 3){
		host = argv[1];
		packetLimit = strtol(argv[2], NULL, 10);
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

	/* data for the registration packet; only need to set once */
	gethostname(clientName, sizeof(clientName));
	strcpy(packet_reg.data, clientName);
	/******* handle the registration of the client to the server *****************/
	// there are three registration packets so this loops for all three of those
	while(1) {
		// setting the registration packet values
		packet_reg.type = htons(121);
		for (i = 0; i < 3; i++) {
			// send the registration packet
			printf("\n Sending REGISTRATION Request \n");
			if(send(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
				printf("\n ---Send failed \n");
				exit(1);
			}
			/* sleep to allow server time before sending next request */
			sleep(1);
		}
		/* sleep to see if the ACK was sent */
		//sleep(1);
		/* client receives the packet from the server */
		if(recv(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
			printf("\n ---Could not receive confirmation packet \n");
			exit(1);
		}

		/* check the type to validate confirmation packet */
		if(ntohs(packet_reg.type) == 221) {
			printf("\n Received REGISTRATION Confirmation \n");
			break;
		}
	}

	/***** handle the receiving of the data packet; send the leave request *********/
	/* wait on addr response from the server */
	while(packetLimit != 0) {
		/* set temp to packet.header; if temp is different from the recieved packet.header then
		it is known that it is a different message */
		temp = packet.header;
		/* client receives the packet from the server */
		if(recv(s, &packet, sizeof(packet), 0) < 0) {
			printf("\n ---Could not receive data packet \n");
			exit(1);
		}
		if(temp != packet.header) {
			packetLimit--;
		}
		printf("\n %d %s",packet.header, packet.data);
	}
	/* packet limit was reached so now it is time to leave the multicaster */
	packet_reg.type = htons(321);

	/* close the socket and create a new connection */
	close(s);

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}
	
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

	if(send(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
		printf("\n ---Send failed \n");
		exit(1);
	}

	printf("\n Sending LEAVE Request \n");

	// close the socket since it is done
	close(s);
	/*****************************************************************************/
}
