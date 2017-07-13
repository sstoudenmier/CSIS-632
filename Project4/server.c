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
#include<sys/select.h>

#define SERVER_PORT 57111
#define MAX_LINE 256
#define MAX_PENDING 5
#define MAX_CLIENTS 30

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
/* struct used to stored the user, group, and sockid of each client */
struct group_table {
  int sockid;
  int group;
  char user[MAX_LINE];
};

/* global sockid so that the threads can create new sockid using received data */
struct group_table records[MAX_CLIENTS];
fd_set readfds;
int s, maxFd;

/* function called by the join handler thread to handle registration of the client */
void *joinHandler() {
  struct registration_packet packet_reg;
  struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
  int newsock, i, len;

  /* accepting input on the main socket */
  if((newsock = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
    perror("tcpserver: accept");
    exit(1);
  }
  maxFd = newsock;
  /* server receives the packet from the client */
  if(recv(newsock, &packet_reg, sizeof(packet_reg), 0) < 0) {
    printf("\n Could not receive packet \n");
    exit(1);
  }
  /* check to make sure that the type matches up for a registration request */
  if (ntohs(packet_reg.type) == 121) {
    /* inform that the confirmation was received */
    printf("\n Recieved REGISTRATION Request with Group: %d and User: %s\n", htons(packet_reg.group), packet_reg.user);
    /* find the first spot that is not currently being used by a client */
    for (i = 0; i < MAX_CLIENTS; i++) {
      if (records[i].sockid <= 0) {
        /* update the group table for the client's unique sockid */
        records[i].sockid = newsock;
        records[i].group = htons(packet_reg.group);
        strcpy(records[i].user, packet_reg.user);
        /* add the socket to the set */
        FD_SET(newsock, &readfds);
        /* send the confirmation packet to the client */
        packet_reg.type = htons(221);
        if(send(newsock, &packet_reg, sizeof(packet_reg), 0) <0) {
          printf("\n Send failed\n");
          exit(1);
        }
        /* inform that the REGISTRATION was confirmed */
        printf("\n Sending REGISTRATION Confirmation\n");
        /* found the first opening; time to leave */
        break;
      }
    }
  }
  /* leave the thread because the client is now registered */
  pthread_exit(0);
}

/* function called by the multicaster thread to handle sending of messages */
void *multicaster(struct data_packet *temp) {
  struct data_packet packet_data;
  struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
  int i;

  // convert the data_packet pointer to a data_packet
  packet_data = *(temp);
  printf("\n Received DATA PACKET from Group: %d and User: %s \n", htons(packet_data.group), packet_data.user);

  /* loop through the records to determine who the data should go to */
  for (i = 0; i < MAX_CLIENTS; i++) {
    /* no need to look further if the sockid is not valid */
    if (records[i].sockid <= 0) {
      break;
    }
    /* time to check who the data goes to */
    if (records[i].group == htons(packet_data.group)) {
      if(send(records[i].sockid, &packet_data, sizeof(packet_data), MSG_NOSIGNAL) <0) {
        FD_CLR(records[i].sockid, &readfds);
        close(records[i].sockid);
      } else {
        printf("\n Sending DATA PACKET for Group: %d and User: %s \n", records[i].group, records[i].user);
      }
    }
  }
  /* exit the thread and join back */
  pthread_exit(0);
}

/* main function */
int main(int argc, char* argv[]) {
  pthread_t threads[2];
  struct data_packet packet_data;
  struct sockaddr_in sin;
	struct sockaddr_in clientAddr;
  struct hostent *hp;
	char *address;
  int len, i, result, temp;

  /* setup passive open */
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("tcpserver: socket");
		exit(1);
	}
	/* setting up the main sockid to listen for the clients */
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
		perror("tcpclient: bind");
		exit(1);
	}
	listen(s, MAX_PENDING);
  /* setting the max descriptor for use in select() */
  maxFd = s;
  /* loop infinitely while listening to the descriptor */
  while(1) {
    /* initialize descriptor for select() and add main socket */
    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    /* add all of the currently existing sockid to the sest */
    for (i = 0; i < MAX_CLIENTS; i++) {
      temp = records[i].sockid;
      if (temp <= 0) {
        break;
      } else {
        FD_SET(temp, &readfds);
      }
    }
    /* listen on the select */
    result = select(maxFd + 1, &readfds, NULL, NULL, NULL);
    /* something happened on one of the set sockets so see which one */
    if (result == -1) {
      printf("\n Error on select() \n");
    } else {
      /* look for input on the main socket */
      if (FD_ISSET(s, &readfds)) {
        pthread_create(&threads[0], NULL, joinHandler, NULL);
        pthread_join(threads[0], NULL);
      }
      /* must be in one of the other sockets */
      else {
        for (i = 0; i < MAX_CLIENTS; i++) {
          temp = records[i].sockid;
          if (temp <= 0) {
            break;
          }
          if (FD_ISSET(temp, &readfds)) {
            /* server receives the packet from the client */
            if(recv(temp, &packet_data, sizeof(packet_data), 0) < 0) {
              printf("\n Could not receive packet \n");
              exit(1);
            }
            pthread_create(&threads[1], NULL, multicaster, &packet_data);
            pthread_join(threads[1], NULL);
          }
        }
      }
    }
  }
}
