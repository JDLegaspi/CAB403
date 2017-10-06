#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 10
#define BUF_SIZE ( 256 )

char *readFile();
int authenticateUser(char* input);
int authenticatePass(char* input, int lineNo);
int updateLeaderboard(int score, char* name);
void game(char* wordOne, char* wordTwo);


int main (int argc, char* argv[]) {

    srand(time(NULL));
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    int bytes_received, i = 0;


    char send_data[1024], recv_data[1024];

    //get port number
    if (argc != 2) {
        fprintf(stderr, "Port number has been set to default: 12345\n"); //configure so we have a default port
		//exit(1);
		
		//generate the end point
		my_addr.sin_family = AF_INET; //host byte order
		my_addr.sin_port = htons(12345); //network byte order
		my_addr.sin_addr.s_addr = INADDR_ANY;

    } else {

		//generate the end point
		my_addr.sin_family = AF_INET; //host byte order
		my_addr.sin_port = htons(atoi(argv[1])); //network byte order
		my_addr.sin_addr.s_addr = INADDR_ANY;

	}

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

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

	char *test = readFile(); 
	//printf("%s", test); //Testing readFile

	//Code to separate words
	char *p;
	char *wordOne;
	char *wordTwo;

	p= strtok(test, ",");

	if(p){
		wordOne = p;		
	}
	p = strtok(NULL, ",");
	
	if(p){
		wordTwo = p;	
	} // won't work in the game function otherwise segmentation dump

	//printf("%s\n", wordOne);//Test word separation
	//printf("%s", wordTwo);	

	game(wordOne, wordTwo); //testing game function

	


    printf("TCP Server waiting for client on port %d\n", htons(my_addr.sin_port)); //configure so it shows actual port number (not working properly)
	
	
	//main block
    while(1) {
        
        int state = 0;

        sin_size = sizeof(struct sockaddr_in);
		
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

		//authenticate block
		int auth = 1;
		while(auth) {

			memset(recv_data, 0, sizeof(recv_data));
			int userLine;

			char *message = "enter username: ";
			if (send(new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
				exit(1);
			}
			
			bytes_received = recv(new_fd, recv_data, sizeof(recv_data), 0);
			
			// auth username
			if (userLine = authenticateUser(recv_data) == 0) {
				char *message = "username does not match";
				if (send(new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
				}
			} else {
				memset(recv_data, 0, sizeof(recv_data));
				char *message = "enter password: ";
				if (send(new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
					exit(1);
				}

				bytes_received = recv(new_fd, recv_data, sizeof(recv_data), 0);

				if (authenticatePass(recv_data, userLine)) {
					
				} else {
					char *message = "password does not match";
					if (send(new_fd, message, strlen(message) ,0) == -1) {
						perror("send");
					}
				}

			}
		}

		//main menu block 
        //char *message = "\n=======WASSUPPP======= \n Please choose an option:\n 1) Play Hangman \n 2) See Scoreboard \n 3) Exit \n";

    }

    //variable = readFile();

	
    return 0;
}

char* readFile(){
	int lineNumber;// = 285; //change to random
	static const char filename[] = "hangman_text.txt";
	FILE *file = fopen(filename, "r");
	int count = 1;
	char line[288];
	char *guessWord;
	lineNumber = rand() % 289;

	while(fgets(line, sizeof(line), file) != NULL){

		
		if(lineNumber == count){
			guessWord = line;
			count++;
			fclose(file);
			return guessWord;			
		}
		else{
			count++;		
		}

	}
	//had return guessWord down here but returns different value
}

int authenticateUser(char* input) {
	static const char* filename = "authentication.txt";
	FILE *file = fopen(filename, "r");
	
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	int inputLength = strlen(input);
	char substring[inputLength];
	int lineNumber, lineMatch = 0;

	while((read = getline(&line, &len, file)) != -1) {
		strncpy(substring, line, inputLength);
		substring[inputLength] = '\0';
		if (strcmp(substring, input) == 0) {
			lineMatch = lineNumber;
		}
		lineNumber++;
	}

	lineNumber = 0;

	fclose(file);

	return lineMatch;
}

int authenticatePass(char* input, int lineNo) {

}

void game(char* wordOne, char* wordTwo) {
	
	
	char guess = 'a'; //tester variable
	int guessesNo = strlen(wordOne) +strlen(wordTwo) + 9;
	if( guessesNo < 26){	
	}
	else{
		guessesNo = 26;	
	}


	//printf("%d\n", guessesNo); //test number of guesses

	/*for(int i = 0; wordOne[i] != '\0'; i++){
		
		printf("Check: %c", wordOne[i]);
		printf("Guess: %c\n", guess);

		if(wordOne[i] == guess){
			printf("%c ASDASF\n", guess);
		}
		else{
			printf("%c was Incorrect\n", guess);
		}
	}/* Doesnt work yet
			


	
	/*char *p;
	char *wordOne;
	char *wordTwo;

	p= strtok(guessWords, ",");

	if(p){
		wordOne = p;		
	}
	p = strtok(NULL, ",");
	
	if(p){
		wordTwo = p;	
	}
	return wordOne;*/
}

int updateLeaderboard(int score, char* name) {
    //check all scores - put current score in correct place
    //maybe use a struct for name/score pairs
    //maybe if scores are the same, place in alphabetical order
}
