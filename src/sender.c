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

/* open */
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/time.h>
/* read */
#include <unistd.h>
#include <time.h>
#include <string.h>

#define FALSE 0
#define TRUE 1


int writeOnAFile = FALSE;
int fdToRead = -1;

int windowSize = 1;
pkt_t *windowPkt[MAX_WINDOW_SIZE];
int currentSeqnum = 0;


char buffReadGen[MAX_PAYLOAD_SIZE];


/* 	Method used to open a file
		If (the file doesn't exists) -> create an empty file for writing 
		Else (the file already exists) -> erase it and create a new empty file in place  
	At the end of this method 
		- writeONAFile is set to TRUE
		- fdToRead is set or the program is stopped
	Error 10 */
void openFile(char* fileToOpen){
writeOnAFile = TRUE;
	if ((fdToRead = open(fileToOpen, O_RDONLY)) == -1){
		fprintf(stderr, "Error receiver (10) : The file you try to use is not valid ( %s ) \n", fileToOpen);
		exit(10);
	}
	printf("File %s successfully opened\n",fileToOpen);
}

void fillWindow(){
	printf("fillWindow\n");
	int i;
	for (i =0;i<MAX_WINDOW_SIZE;i++){
	    int size = read(fdToRead,buffReadGen,MAX_PAYLOAD_SIZE);
	    if(size == 0){
	    	close(fdToRead);
	    	break;
	    }
	    pkt_t* pktTmp = pkt_new();
	    pkt_set_type(pktTmp,PTYPE_DATA);
	    pkt_set_window(pktTmp,windowSize);
	    pkt_set_seqnum(pktTmp,currentSeqnum);
	    pkt_set_length(pktTmp,size);
	    pkt_set_payload(pktTmp,buffReadGen,size);
		windowPkt[i] = pktTmp;
	}
}

void sendPkt(int numToSend, int sock){
	pkt_t *pktTmp = windowPkt[numToSend%MAX_WINDOW_SIZE];
	char bufTmp[pkt_get_length(pktTmp) + 12];
	size_t len = sizeof(bufTmp); 
    if(pkt_encode(pktTmp,bufTmp,&len) == PKT_OK){
    	if((write(sock,bufTmp,len)) == -1){
            fprintf(stderr, "Error : write(1)\n");
        }else{
        	currentSeqnum++;
        	if(currentSeqnum == 256){
        		currentSeqnum = 0;
        	}
        }
		
	}else{
		fprintf(stderr, "Erreur sender (30) : encode\n");
		exit(30);
	}
}

int sendPkts(int sock){
	printf("sendPkts\n");
	int i;
	
	//Va do current seqnum et lit la size window
	for (i = currentSeqnum; i<currentSeqnum+windowSize;i++){
		pkt_t *pktTmp = windowPkt[i];
		if(pktTmp != 0){
			sendPkt(i,sock);
		}else{
			return 0;
		}
	}
	return 1;
}

int readForAck(int sfd){
	fd_set rfds;
    struct timeval tv;
    int retval;
    ssize_t rec=0;
	socklen_t lengthSock = sizeof(sfd);

    FD_ZERO(&rfds);

    /* Pendant 5 secondes maxi */
    tv.tv_sec = 5;
    tv.tv_usec = 0;


	FD_SET(sfd, &rfds);
    	
    retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
	/* Considérer tv comme indéfini maintenant ! */

	printf("ds\n");

    if (retval == -1){
        fprintf(stderr, "Error receiver (20): select\n");
    }
    else if (retval) {
		printf("dssdq\n");
		char bufferAck[12];
		printf("dssdq\n");
        rec = recvfrom(sfd, bufferAck, sizeof(bufferAck), 
				   0, (struct sockaddr *)&sfd, &lengthSock);
		printf("dsqsd\n");

	    if(rec ==-1){
    		fprintf(stderr,"Error receiver (21) : Connection failed\n");
    		exit(21);
	    }

    	pkt_t *ack = pkt_new();
    	pkt_status_code codePkt = pkt_decode(bufferAck,rec,ack);


		if(codePkt != PKT_OK){
    		//TODO traiter erreurs une à une
    		fprintf(stderr, "Error pkt_decode\n");
    	}else if( pkt_get_type(ack) == PTYPE_ACK){
			fprintf(stderr, "currentSeqnum %d\n", pkt_get_seqnum(ack));
    		
    	}
    	//Not a data type packet ==> ignored
    	else{

    	}

    }
    else{
		printf("ezrzr\n");
        sendPkt(currentSeqnum,sfd);
    }

    return rec;
}

int main (int argc, char * argv[]){
	char c;
	char* ip = NULL;
	int port = 0;
	char* portErrorPtr;

	fdToRead = fileno(stdin);

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
    int sock = create_socket(NULL,-1 ,&addr,port);

    int keepListening = TRUE;
	
	fillWindow();

    while(keepListening){
	    

    	keepListening = sendPkts(sock);

    	readForAck(sock);
	    

	 	struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = 500000;

	   if(nanosleep(&tim , &tim2) < 0 )   
	   {
	      printf("Nano sleep system call failed \n");
	      return -1;
	   }



    }
}