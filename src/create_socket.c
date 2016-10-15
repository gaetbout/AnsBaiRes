#include "create_socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int create_socket(struct sockaddr_in6 * source_addr, 
                  int src_port, struct sockaddr_in6 * dest_addr, int dst_port){
    //fprintf(stderr,"create_socket\n");
    int sfd = fileno(stdin);

	//bind la source car socket --> Local point
	if(source_addr != NULL && src_port > 0) {
		sfd = socket(source_addr->sin6_family, SOCK_DGRAM,0);
		if(sfd == -1) {
			fprintf(stderr, "Error : create socket source\n");
			return EXIT_FAILURE;
		}

		source_addr -> sin6_port = htons(src_port);
		if (bind(sfd, (struct sockaddr *) source_addr, sizeof(struct sockaddr_in6)) 
            == -1) {
			fprintf(stderr, "Error : bind\n");
			exit(errno);
		}

	}
	//connect vers la destination car socket --> Remote point
	if(dest_addr != NULL && dst_port > 0) {
		sfd = socket(dest_addr->sin6_family, SOCK_DGRAM,0);
		if(sfd == -1) {
			fprintf(stderr, "Error : create socket destination\n");
			return EXIT_FAILURE;
		}

		dest_addr -> sin6_port = htons(dst_port);
		if (connect(sfd, (struct sockaddr *) dest_addr,
			sizeof(struct sockaddr_in6)) == -1) {
			fprintf(stderr, "Error : connect\n");
			exit(errno);
		}
	}
	return sfd;
}