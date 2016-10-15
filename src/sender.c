#include "packet_interface.h"
/*Standard input output library*/
#include <stdio.h>
/*Standard general utilities library*/
#include <stdlib.h>
/*getop funciton -> agruments*/
#include <getopt.h>

#define BAD_USE 1

#define FALSE 0
#define TRUE 1


int writeOnAFile = FALSE;
FILE *fileToWrite;

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

int main (int argc, char * argv[]){
	char c;
	char* ip = 0;
	int port;
	char* portErrorPtr;

	if (argc != 3 && argc !=5 ){
		fprintf(stderr,"Use %s [-f X] hostname port (X is the name of where the file will be sent)\n",argv[0]);
		return BAD_USE;
	}
	while ((c = getopt (argc, argv, "f:")) != -1){
		switch (c){
			case 'f':
				// Open file 
				openFile(optarg);
				break;
			default:
				fprintf(stderr,"Use %s [-f X] hostname port (X is the name of where the file will be sent)\n",argv[0]);
            	return BAD_USE;
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
        	fprintf(stderr,"Error sender (11) : The port used seems not to be an int - %s\n",argv[optind+1]);
        	exit (11);
        }
    }	
    printf("Récap : \n - File exists : %d\n - Filename %s\n - ip : %s\n - port :%d\nNote : The filename is set to the port if -f option isn't used.\n", writeOnAFile, argv[2],ip,port);

    
}

// ICi y'aura sliding window 
// Num last envoyé

//TODO : 