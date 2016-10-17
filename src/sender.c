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
    int num = 0;
    while(keepListening){
	    char ttmp[MAX_PAYLOAD_SIZE];
	    int size = read(fdToRead,ttmp,MAX_PAYLOAD_SIZE);
	    if(size == 0){
	    	close(fdToRead);
	    	break;
	    }
	    pkt_t* pktToSend = pkt_new();
	    pkt_set_type(pktToSend,PTYPE_DATA);
	    pkt_set_window(pktToSend,1);
	    pkt_set_seqnum(pktToSend,num);
	    pkt_set_length(pktToSend,size);
	    pkt_set_payload(pktToSend,ttmp,size);
		fprintf(stderr, "SEND : %d\n",num);

	    char bufTmp[size + 12];
		size_t len = sizeof(bufTmp); 
	    if(pkt_encode(pktToSend,bufTmp,&len) == PKT_OK){
	    	if((write(sock,bufTmp,len)) == -1){
	            fprintf(stderr, "Error : write(1)\n");
	        }else{
	        	num++;
	        	if(num == 256){
	        		num = 0;
	        	}
	        }
			
		}else{
			fprintf(stderr, "Erreur sender (30) : encode\n");
			exit(30);
		}

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

// ICi y'aura sliding window 
// Num last envoyé

//TODO : 