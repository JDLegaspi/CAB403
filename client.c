#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

    #define MAXDATASIZE 100

int main (int argc, char* argv[]) {

    int sockfd, numbytes, i = 0;
    char buf[MAXDATASIZE];
    struct hostent* he;
    struct sockaddr_in their_address;

    //make sure all inputs are satisfied
    if (argc != 3) {
        fprintf(stderr, "usage: client_hostname port_number\n");
        exit(1);
    }

    //get hostname from first input
    if ((he = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname\n");
        exit(1);
    }

    //generate socket and ensure no error
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket\n");
        exit(1);
    }

    //generate endpoint I think?
    their_address.sin_family = AF_INET;
    their_address.sin_port = htons(atoi(argv[2])); //store port number and change byte order
    their_address.sin_addr = *((struct in_addr* )he->h_addr);
    bzero(&(their_address.sin_zero), 8); //I don't understand what this line does

    //connect to ip and socket specified
    if (connect(sockfd, (struct sockaddr* )&their_address, sizeof(struct sockaddr)) == -1) {
        perror("connect\n");
        exit(1);
    } else {
        printf("wassup foo\n");
    }
    
    return 0;
}
    