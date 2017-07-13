#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

#define SERVER_PORT 57111
#define MAX_LINE 256

/* struct used for the registration and leaving of a client */
struct registration_packet {
  short type;
  short group;
  char user[MAX_LINE];
};
/* struct used for passing of the data to and from the server */
struct data_packet {
  short group;
  char user[MAX_LINE];
  char data[MAX_LINE];
};

/* global varibles since they are used in the thread functions and the main */
struct registration_packet packet_reg;
struct data_packet packet_data;
char* user;
int group;
int s;

/* function called by a thread to handle the sending of the data_packet to the server */
void *sendData() {
  char input[MAX_LINE];
  /* loop infinitely so that the user is able to constantly type new messages */
  while (1) {
    /* get input from the user */
    gets(input);
    //printf("data: %s", input);
    packet_data.group = htons(group);
    strcpy(packet_data.user, user);
    strcpy(packet_data.data, input);
    /* send the data packet to the server */
    if(send(s, &packet_data, sizeof(packet_data), 0) < 0) {
      printf("\n ---Send failed \n");
      exit(1);
    }
  }
}

/* function called by a thread to handles the receiving of the data_packet from the server */
void *receiveData() {
  /* loop infinitely to receive data packets from the server multicaster */
  while (1) {
    if(recv(s, &packet_data, sizeof(packet_data), 0) < 0) {
      printf("\n ---Could not receive data packet \n");
      exit(1);
    }
    /* print the name and data from the multicaster */
    printf("[%s]: %s\n", packet_data.user, packet_data.data);
  }
}

int main(int argc, char* argv[]) {
  /* declaring variables */
  pthread_t threads[2];
  struct hostent *hp;
	struct sockaddr_in sin;
  char* host;

  /* get the arguments for running the client */
  if (argc == 4) {
    host = argv[1];
    user = argv[2];
    group = atoi(argv[3]);
  }
  else {
  		fprintf(stderr, "usage:newclient server\n");
  		exit(1);
  }

  /* translate host name into peer's IP address */
	hp = gethostbyname(host);
	if(!hp){
		fprintf(stderr, "unkown host: %s\n", host);
		exit(1);
	}

  /* create a new socket */
  if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpclient: socket");
		exit(1);
	}

  /* build address data structure */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

  /* connect the socket to the server */
	if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: connect");
		close(s);
		exit(1);
	}

  /* setting the type for the registration packet */
  packet_reg.type = htons(121);
  packet_reg.group = htons(group);
  strcpy(packet_reg.user, user);

  /* setting necessary information for the data packet */
  packet_data.group = htons(group);
  strcpy(packet_data.user, user);

  /* sending the registration packet to the server */
  printf("\n Sending REGISTRATION Request \n");
  if(send(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
    printf("\n ---Send failed \n");
    exit(1);
  }

  /* loop while the recieved packet doesn't match the correct type */
  while (ntohs(packet_reg.type) != 221) {
    /* receives the registartion packet from the server */
    if(recv(s, &packet_reg, sizeof(packet_reg), 0) < 0) {
      printf("\n ---Could not receive confirmation packet \n");
      exit(1);
    }
  }
  printf("\n Received REGISTRATION Confirmation \n");
  /* set the socket to the new socket created by the server so that
  /* create and call two threads that handle the sending and receiving of data packet */
  pthread_create(&threads[0], NULL, sendData, NULL);
  pthread_create(&threads[1], NULL, receiveData, NULL);

  while(1) {}
}
