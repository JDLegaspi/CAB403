#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

void /*idk what this should return yet*/ readFile();
int authenticate();
int updateLeaderboard(int score, char* name);

int main (int argc, char* argv[]) {

    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_sizel
    int i = 0;

    //get port number
    if (argc != 2) {
        fprintf(stderr, "Please specify client port number");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0) == -1)) {
        perror("socket");
        exit(1);
    }

    //generate the end point
    my_addr.sin_family = AF_INET; //host byte order
    my_addr.sin_port = htons(atoi(argv[1])); //network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket to endpoint
    if (bind(sockfd, struct sockaddr *)&my_addr, sizeof(struct sockaddr) == -1) {
        perror("bind");
        exit(1);
    }

    //listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    //variable = readFile();

    return 0;
}

void /*idk what this should return yet*/ readFile() {

}

int authenticate() {

}

int updateLeaderboard(int score, char* name) {
    //check all scores - put current score in correct place
    //maybe use a struct for name/score pairs
    //maybe if scores are the same, place in alphabetical order
}