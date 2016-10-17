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
int currentSeqnum = -1;

// Buffers for the packet and create ack
char bufferPkt[MAX_PAYLOAD_SIZE+12];
char bufferAck[12];

// Window for packets
pkt_t *windowPkt[WINDOW_SIZE];

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
	if ((fileToWrite = fopen(fileToOpen,"w"))==NULL){
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
		windowPkt[i] = 0;
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

	struct sockaddr_in6 from;
	socklen_t taille = sizeof(from);
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
				   0, (struct sockaddr *)&from, &taille);


//TODO Faire que une seule fwa
        if (connect(sfd, (struct sockaddr *) &from, 
                sizeof(struct sockaddr_in6)) != 0){
    		fprintf(stderr, "Error receiver (23) : connect\n");
        	return -1;
    	}

	    if(rec ==-1){
    		fprintf(stderr,"Error receiver (21) : Connection failed\n");
    		exit(21);
	    }
    }
    else{
        fprintf(stderr, "No data for 5 seconds\n");
        if(writeOnAFile == TRUE){
    		fclose(fileToWrite);
    	}
        exit(0);
    }

    return rec;
} 

void sendAck(const int sfd){
	fprintf(stderr, "ACK SENT %d\n", currentSeqnum);
	pkt_t *ack = pkt_new();
	pkt_set_type(ack,PTYPE_ACK);
	pkt_set_window(ack,WINDOW_SIZE);
	pkt_set_seqnum(ack,(currentSeqnum)%WINDOW_SIZE);
	pkt_set_length(ack,0);
	char bufTmp[12];
	size_t len = sizeof(bufTmp); 
	if(pkt_encode(ack,bufTmp,&len) == PKT_OK){
		fprintf(stderr, "ACK : %d\n",(currentSeqnum)%WINDOW_SIZE);
		if((write(sfd,bufTmp,len)) == -1){
            fprintf(stderr, "Error : write(5)\n");
       	}
	}else{
		fprintf(stderr, "Erreur receiver (30) : encode\n");
		exit(30);
	}
   	pkt_del(ack);
}


/* Method used to write pkts, write every valid pkt 
	Return 0 if the length of pkt == 0 (Last pkt) */
int writePkt(int currentPkt){
	int i;
	int ret=1;
	for(i = currentPkt;i<WINDOW_SIZE;i++){

		if(windowPkt[i]!=0){
			fprintf(stderr, "B1 currentPkt : %d\n",i);
			if(writeOnAFile == FALSE){
				if(write(fileno(stdout),windowPkt[i],pkt_get_length(windowPkt[i])) == -1){
                    fprintf(stderr, "Error : write(1)\n");
				}
			}else{
	            if(fwrite(pkt_get_payload(windowPkt[i]),pkt_get_length(windowPkt[i]),1,fileToWrite) == 0){
    	            fprintf(stderr, "Error : write(2)\n");
        	    }
			}
			if(pkt_get_length(windowPkt[i])==0){
				ret=0;
			}
    		currentSeqnum++;
    		//sendAck(sock);
			pkt_del(windowPkt[i]);
			windowPkt[i] = 0;
		}else{
			return 1;
		}
	}
	
	for(i = 0;i<currentPkt;i++){
		if(windowPkt[i]!=0){
			fprintf(stderr, "B2 currentPkt : %d\n",i);
			if(writeOnAFile == FALSE){
				if(write(fileno(stdout),windowPkt[i],pkt_get_length(windowPkt[i])) == -1){
                    fprintf(stderr, "Error : write(1)\n");
				}
			}else{
	            if(fwrite(pkt_get_payload(windowPkt[i]),pkt_get_length(windowPkt[i]),1,fileToWrite) == 0){
    	            fprintf(stderr, "Error : write(2)\n");
        	    }
			}
			if(pkt_get_length(windowPkt[i])==0){
				ret=0;
			}
    		currentSeqnum++;
    		//sendAck(sock);
			pkt_del(windowPkt[i]);
			windowPkt[i] = 0;
		}else{
			return 1;
		}
	}
	return ret;
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
    		//TODO traiter erreurs une à une
    		fprintf(stderr, "Error pkt_decode\n");
    	}else if( pkt_get_type(pktForThisLoop) == PTYPE_DATA){
    		int seqNumReceived = pkt_get_seqnum(pktForThisLoop);
    		if(currentSeqnum==-1){
    			currentSeqnum = seqNumReceived; 
    		}
			//fprintf(stderr, "currentSeqnum %d\n", currentSeqnum);
			//fprintf(stderr, "seqNumReceived %d\n", seqNumReceived);
    		if(currentSeqnum == seqNumReceived){
    			//fprintf(stderr, "seqNumReceived - WINDOW_SIZE %d\n", seqNumReceived%WINDOW_SIZE);
            	windowPkt[seqNumReceived%WINDOW_SIZE] = pktForThisLoop;
    			if(writePkt(seqNumReceived%WINDOW_SIZE) == 0){
	    			keepListening = FALSE;
	    		}
	    		sendAck(sock);
    		}
        	if(currentSeqnum >= 256){
        		currentSeqnum = 0;
        	}
    	}
    	//Not a data type packet ==> ignored
    	else{

    	}
    }

    fprintf(stderr, "END OF FILE \n");

    if(writeOnAFile == TRUE){
    	fclose(fileToWrite);
    }
    return 0;
}