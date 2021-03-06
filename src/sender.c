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

#define DELTA_TIMEOUT 3700000 // 3,7 secondes

int readFromFile = FALSE;
int fdToRead = -1;

int windowSize = 1;
pkt_t *windowPkt[MAX_WINDOW_SIZE+1];

int currentSeqnum = 0;
int maxSeqnum = 0;
int indicePkt = 0;

int lastPacketGenerated = FALSE;

fd_set rfds;

char buffReadGen[MAX_PAYLOAD_SIZE];


/* 	Method used to open a file
		If (the file doesn't exists) -> create an empty file for writing
		Else (the file already exists) -> erase it and create a new empty file in place
	At the end of this method
		- readFromFile is set to TRUE
		- fdToRead is set or the program is stopped
	Error 10 */
void openFile(char* fileToOpen){
	readFromFile = TRUE;
	if ((fdToRead = open(fileToOpen, O_RDONLY)) == -1){
		fprintf(stderr, "Error receiver (10) : The file you try to use is not valid ( %s ) \n", fileToOpen);
		exit(10);
	}
	fprintf(stderr,"File %s successfully opened\n",fileToOpen);
}


/* Used to send every packets
	if tab[x] is null read and create a packet

	*/
int sendPkts(int sock){

	int indice = indicePkt;

	int numPktSent = 0;
	while(numPktSent<windowSize){
	 	pkt_t *pktTmp = windowPkt[indice];
		if(pktTmp == NULL || pktTmp == 0){
			int size = read(fdToRead,buffReadGen,MAX_PAYLOAD_SIZE);
		    if(size == 0 && lastPacketGenerated == FALSE){
		    	//fprintf(stderr, "ZERO \n");
		    	lastPacketGenerated = TRUE;
		    	close(fdToRead);
		    }
		    pktTmp = pkt_new();
		    pkt_set_type(pktTmp,PTYPE_DATA);
		    pkt_set_window(pktTmp,windowSize);
		    //fprintf(stderr, "Je gen le packet %d\n",maxSeqnum);
		    pkt_set_seqnum(pktTmp,maxSeqnum);
		    maxSeqnum = (maxSeqnum+1)%(256);
		    if(lastPacketGenerated == FALSE){
		    	pkt_set_length(pktTmp,size);
			    pkt_set_payload(pktTmp,buffReadGen,size);
		    }else{
		    	//fprintf(stderr, "Je gen un pkt vide \n");
		    	pkt_set_length(pktTmp,0);
		    }
		    pkt_set_timestamp(pktTmp,0);
			windowPkt[indice] = pktTmp;

		}

		char bufTmp[pkt_get_length(pktTmp) + 12];
		size_t len = sizeof(bufTmp);
		//fprintf(stderr,"indice du pkt que je vais envoyer : %d\n",indice);
		struct timeval tv;
    	gettimeofday(&tv, NULL);

    	uint32_t now = tv.tv_sec % 1000 + tv.tv_usec;
		uint32_t tsmpPkt = pkt_get_timestamp(pktTmp);


		//Not resend if timeout isn't out
		//fprintf(stderr,"now: %d\n",now);
		//fprintf(stderr,"tsmpPkt: %d\n",tsmpPkt);
		//fprintf(stderr,"DELTA_TIMEOUT: %d\n",DELTA_TIMEOUT);
		if(tsmpPkt == 0 || ((now - tsmpPkt ) > DELTA_TIMEOUT)){

			//fprintf(stderr,"J'envoie le pkt : %d\n",pkt_get_seqnum(pktTmp));
			pkt_set_timestamp(pktTmp,now);
			if(pkt_encode(pktTmp,bufTmp,&len) == PKT_OK){
				if((write(sock,bufTmp,len)) == -1){
			        fprintf(stderr, "Error : write(1)\n");
			    }
				numPktSent++;
			}else{
				fprintf(stderr, "Erreur sender (30) : encode\n");
				exit(30);
			}
			indice = (indice + 1) % (MAX_WINDOW_SIZE+1);
			numPktSent++;
		}else{
			//fprintf(stderr, "JE RENVOI PAS\n");
			numPktSent++;
			//fprintf(stderr, "TIMER Not out\n");
		}
		if(lastPacketGenerated == TRUE){
			return 0;

		}
		//pkt_del(pktTmp);
	}

	return 1;
}
/* Method ued to delete every useless pkt
* used after reception of an ack
* */
void updateWindow(int nextSeqNum){
	while(1){
		//Check if there a pkt
		if(windowPkt[indicePkt] != NULL && windowPkt[indicePkt]!= 0){
			pkt_t *pktTmp = windowPkt[indicePkt];
			if(pkt_get_seqnum(pktTmp) == nextSeqNum){
				return;
			}else if(pkt_get_seqnum(pktTmp) < nextSeqNum){
				//fprintf(stderr, "Je delete le packet : %d\n",pkt_get_seqnum(pktTmp));
				pkt_del(pktTmp);
				windowPkt[indicePkt] = 0;
			}else if(nextSeqNum >=0 && nextSeqNum <=31 && pkt_get_seqnum(pktTmp) >=255-32){
				//fprintf(stderr, "Je delete le packet : %d\n",pkt_get_seqnum(pktTmp));
				pkt_del(pktTmp);
				windowPkt[indicePkt] = 0;
			}
			indicePkt = (indicePkt+1) % (MAX_WINDOW_SIZE+1);

		}else{
			return;
		}
	}
}
/* Read for an ack
* While there is ack on the fd it keep reading
* If there is no ack for x usec we check if there no pkt to resend due to timeout
*/
int readForAck(int sfd){
    struct timeval tv;
    int retval;
    ssize_t rec=0;

    tv.tv_sec = 0;
    tv.tv_usec = 1;

    FD_ZERO(&rfds);
	FD_SET(sfd, &rfds);

    //Le timeout petit permet de vider le buffer d'ack
    retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
    //fprintf(stderr, "ON READ REREAD \n");

    if (retval == -1){
        fprintf(stderr, "Error receiver (20): select\n");
    }
    else if (retval) {
		char bufferAck[12];
		rec = read(sfd,bufferAck,sizeof(bufferAck));
	    if(rec ==-1){
    		fprintf(stderr,"Error receiver (21) : Connection failed\n");
    		exit(21);
	    }
	    if(rec== 0){
	    	//fprintf(stderr, "REC == 0\n");
	    	return 0;
	    }

    	pkt_t *ack = pkt_new();
    	pkt_status_code codePkt = pkt_decode(bufferAck,rec,ack);

		if(codePkt != PKT_OK){

    		fprintf(stderr, "Error receiver (22) : decode failed\n");
    		//exit(22);
    	}else if( pkt_get_type(ack) == PTYPE_ACK){
    		windowSize = pkt_get_window(ack);
    		//fprintf(stderr, "WINDOW ACK : %d\n",windowSize);
    		//fprintf(stderr, "pkt_get_seqnum ACK : %d\n",pkt_get_seqnum(ack));
    		//Delete useless pkt
    		currentSeqnum = pkt_get_seqnum(ack);
    		updateWindow(pkt_get_seqnum(ack));

    		//Treatment when window equals zero
    		// We reopen the window only if we got an ack that told us to
    		if(windowSize == 0){
    			readForAck(sfd);
    		}
    	}
    	//Not a ack type packet ==> ignored
    	else{
    		// Here we can implements a corruption rate

    	}

			pkt_del(ack);
    }
    else{
    	//fprintf(stderr, "ON READ TIMEOUT \n");
        sendPkts(sfd);
    }

    return rec;
}
/*
* Methode used to delete every pkt ta still left in the window
* Called at the end of the program
*/
void freeLastPkt(){
	int i;
	for (i = 0;i<MAX_WINDOW_SIZE+1;i++){
		if(windowPkt[i] != NULL && windowPkt[i] != 0){
			pkt_del(windowPkt[i]);
		}
	}
}

int main (int argc, char * argv[]){
	char c;
	char* ip = NULL;
	int port = 0;
	char* portErrorPtr;

	// Usage check
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
	if(readFromFile == FALSE){
		fdToRead = fileno(stdin);
	}
	//Address Ip resolv
	if(argv[optind]!=NULL){
        ip = argv[optind];
    }
    //Port resolv
    if(argv[optind+1]!=NULL){
        port = (int) strtod(argv[optind+1],&portErrorPtr);
        if(*portErrorPtr != 0){
        	fprintf(stderr,"Error sender (11) : The port used seems not to be an int - %s\n",argv[optind+1]);
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
    //printf("Récap : \n - File exists : %d\n - Filename %s\n - ip : %s\n - port :%d\nNote : The filename is set to the port if -f option isn't used.\n", readFromFile, argv[2],ip,port);

    //Both last args are useless because we won't use them
    int sock = create_socket(NULL,-1 ,&addr,port);

    int keepListening = TRUE;

    while(keepListening){

    	keepListening = sendPkts(sock);
    	if(keepListening == 0){
    		//envoyer un packet vide
    		while(readForAck(sock)){
    			sendPkts(sock);
    		}
				freeLastPkt();
    		fprintf(stderr, "End, file successfully sent !\n");
    		exit(0);
    	}
    	readForAck(sock);

    }
}

//	    fprintf(stderr,"Timestamp: %d\n",(int)time(NULL));
