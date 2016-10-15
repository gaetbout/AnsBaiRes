/*Standard input output library*/
#include <stdio.h>
/*Standard general utilities library*/
#include <stdlib.h>
/*getop funciton -> agruments*/
#include <getopt.h>

#define BAD_USE 1


int main (int argc, char * argv[]){
	char c;

	if (argc != 3 && argc !=5 ){
		fprintf(stderr,"Use %s hostname port [-f X] (X is the name of the file to be sent)\n",argv[0]);
		return BAD_USE;
	}
	while ((c = getopt (argc, argv, "f:")) != -1){
	        //fprintf(stderr,"Curr %c\n",(char) c);
	        //fprintf(stderr,"OptA %s\n\n",optarg);
		switch (c){
			case 'f':
				fprintf(stdout,"f detected\n");
				//test si fichier existe :D
				break;
			default:
    	        fprintf(stderr,"Error : Use %s hostname port [-f X] (X is the name of the file to be sent)\n",argv[0]);
            	return BAD_USE;
		}
	}	
}
