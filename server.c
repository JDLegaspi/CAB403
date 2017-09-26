#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

    #define BACKLOG 10

void /*idk what this should return yet*/ readFile();
int authenticate();
int updateLeaderboard(int score, char* name);

int main (int argc, char* argv[]) {

    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    int i = 0;

    char send_data[1024], recv[1024];

    //get port number
    if (argc != 2) {
        fprintf(stderr, "Please specify client port number\n"); //configure so we have a default port
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    //generate the end point
    my_addr.sin_family = AF_INET; //host byte order
    my_addr.sin_port = htons(atoi(argv[1])); //network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket to endpoint
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    //listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("TCP Server waiting for client on port %d\n", htons(my_addr.sin_port)); //configure so it shows actual port number (not working properly)

    while(1) {
        
        int state = 0;

        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        
        //main menu block 
        while (1) {
            char *message = "\n=======WASSUPPP======= \n Please choose an option:\n 1) Play Hangman \n 2) See Scoreboard \n 3) Exit \n";
            if (send(new_fd, message, sizeof((char*)message) ,0) == -1) {
                perror("send");
            }
        

        }

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