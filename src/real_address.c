#include "real_address.h"

#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char * real_address(const char * address, struct sockaddr_in6 * rval){
    int s;
	rval -> sin6_family = AF_INET6;

	s = inet_pton(AF_INET6, address, &(rval -> sin6_addr));

    //cas ou il s'agit d'une addresse IP
	if( s == 1 ){
		return NULL;
    }
    //cas ou il s'agit d'u nom de domaine
    struct addrinfo *result;
 	struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
 	hints.ai_family = AF_INET6;
 	hints.ai_socktype = SOCK_DGRAM;

    s = getaddrinfo(address, NULL, &hints, &result);
    if (s != 0) {
        return "Error : getaddrinfo\n";
    }

    struct sockaddr *ai_addr = result -> ai_addr;
    memcpy(rval,ai_addr,sizeof(struct sockaddr_in6));
	free(result);
    return NULL;
}