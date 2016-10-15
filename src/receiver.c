#include "packet_interface.h"
/* Library used to get the address */
#include "real_address.h"
/* Library used to create teh socket */
#include "create_socket.h"
/* Standard input output library */
#include <stdio.h>
/* Standard general utilities library */
#include <stdlib.h>
/* getop funciton -> agruments */
#include <getopt.h>
/* Sockets gestion */
#include <sys/socket.h>
#include <sys/types.h>

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

#define WINDOW_SIZE 8

int writeOnAFile = FALSE;
FILE *fileToWrite;

// The socket to listen
int sock;

// Buffers for the packet and create ack
char bufferPkt[MAX_PAYLOAD_SIZE+12];
char bufferAck[12];

// Window for packets
pkt_t *windowPkt[WINDOW_SIZE+1];

// Status code used on ultiple places
pkt_status_code codePkt;


/* 	Method used to open a file
		If (the file doesn't exists) -> create an empty file for writing 
		Else (the file already exists) -> erase it and create a new empty file in place  
	At the end of this method 
		- writeONAFile is set to TRUE
		- fileToWrite is set or the program is stopped
	Error 10 */
void openFile(char* fileToOpen){
	writeOnAFile = TRUE;
	if (fopen(fileToOpen,"w")==NULL){
		fprintf(stderr, "Error receiver (10) : The file you try to use is not valid ( %s ) \n", fileToOpen);
		exit(10);
	}
	printf("File %s successfully opened\n",fileToOpen);
}

/* Method used to fill the window of pkt with new empty pkt */
void fillWindow(){
	printf("fillWindow\n");
	int i;
	for (i =0;i<WINDOW_SIZE;i++){
		windowPkt[i] = pkt_new();
	}
}

/* Loop reading a socket and printing to stdout,
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: the packet
 */
ssize_t readPkt(const int sfd){
	fd_set rfds;
    struct timeval tv;
    int retval;
    ssize_t rec=0;
	socklen_t lengthSock = sizeof(sock);

    FD_ZERO(&rfds);

    /* Pendant 5 secondes maxi */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

	FD_SET(sfd, &rfds);
    	
    retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
	/* Considérer tv comme indéfini maintenant ! */

    if (retval == -1){
        fprintf(stderr, "Error receiver (20): select\n");
    }
    else if (retval) {
        rec = recvfrom(sfd, bufferPkt, sizeof(bufferPkt), 
				   0, (struct sockaddr *)&sfd, &lengthSock);
	    if(rec ==-1){
    		fprintf(stderr,"Error receiver (21) : Connection failed\n");
    		exit(21);
	    }
    }
    else{
        fprintf(stderr, "No data for 5 secodnes\n");
        exit(0);
    }

    return rec;


} 


int main (int argc, char * argv[]){
	char c;
	char* ip = NULL;
	int port = 0;
	char* portErrorPtr;
	int keepListening = TRUE;

	if (argc != 3 && argc !=5 ){
		fprintf(stderr,"Use %s [-f X] hostname port (X is the name of where the file will be sent)\n",argv[0]);
		return -1;
	}
	while ((c = getopt (argc, argv, "f:")) != -1){
		switch (c){
			case 'f':
				// Open file 
				openFile(optarg);
				break;
			default:
				fprintf(stderr,"Use %s [-f X] hostname port (X is the name of where the file will be sent)\n",argv[0]);
            	return -1;
		}
	}

	//Address Ip resolv
	if(argv[optind]!=NULL){
        ip = argv[optind];
    }
    //Port resolv
    if(argv[optind+1]!=NULL){
        port = (int) strtod(argv[optind+1],&portErrorPtr);
        if(*portErrorPtr != 0){
        	fprintf(stderr,"Error receiver (11) : The port used seems not to be an int - %s\n",argv[optind+1]);
        	exit (11);
        }
    }


	struct sockaddr_in6 addr;
	const char* error = NULL;
	//Real address treatment
    error = real_address(ip,&addr); 
    if(error != NULL){
    	fprintf(stderr, "Error receiver (12) : %s\n", error);
    	exit(12);
    }
    printf("Récap : \n - File exists : %d\n - Filename %s\n - ip : %s\n - port :%d\nNote : The filename is set to the port if -f option isn't used.\n", writeOnAFile, argv[2],ip,port);

    //Both last args are useless because we won't use them
    sock = create_socket(&addr,port,NULL,-1);

    //First we create every packets in the window 
    fillWindow();

    while(keepListening){
        /*Reset suite eu trigger du select*/ 
    	pkt_t *pktForThisLoop = pkt_new();
    	ssize_t lengthRead = readPkt(sock);
    	codePkt = pkt_decode(bufferPkt,lengthRead,pktForThisLoop);

    	if(codePkt != PKT_OK){
    		//TODO traiter packets
    		fprintf(stderr, "Error pkt_decode\n");
    	}else{

    	}
    }
}

//TODO NUM NEXT_SEQ_NUM_EXPECTED

/* Receiver while (true){
		reçoit 
		décode 
			si error discard ack num next expected
		Check si num seq expected 
			si non : ack avec numéro expected + SIZE SLIDING WINDOW ATUELLE
			si oui : Slide window et envoie next num expected 
		
		écrit le payload du/des packets
	}

*/