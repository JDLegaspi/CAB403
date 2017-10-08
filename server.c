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
#include <ctype.h>

	#define BACKLOG 10
	#define BUF_SIZE ( 256 )
	#define MAXDATASIZE 1024

char *readFile();
int authenticateUser(char* input);
int authenticatePass(char* input, int lineNo);
int updateLeaderboard(int score, char* name);
void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, char* message);


int main (int argc, char* argv[]) {

    srand(time(NULL));
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    int bytes_received, i = 0;


    char send_data[MAXDATASIZE], recv_data[MAXDATASIZE];

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

	printf("TCP Server waiting for client on port %d\n", htons(my_addr.sin_port)); //configure so it shows actual port number (not working properly)
	
	
	//main block
    while(1) {
        
        int state = 0;

        sin_size = sizeof(struct sockaddr_in);
		
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

		char *message;
		int userLine;

		//authenticate block
		int auth = 1;
		while(auth) {

			memset(recv_data, 0, sizeof(recv_data));

			message = "enter username: ";
			if (send(new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
				exit(1);
			}
			
			bytes_received = recv(new_fd, recv_data, sizeof(recv_data), 0);
			
			// auth username
			if ((userLine = authenticateUser(recv_data)) == 0) {
				message = "username does not match\n";
				if (send(new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
				}
			} else {
				memset(recv_data, 0, sizeof(recv_data));
				message = "enter password: ";
				if (send(new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
					exit(1);
				}

				bytes_received = recv(new_fd, recv_data, sizeof(recv_data), 0);

				if (authenticatePass(recv_data, userLine) == 1) {
					auth = 0;
				} else {
					message = "password does not match\n";
					if (send(new_fd, message, strlen(message) ,0) == -1) {
						perror("send");
					}
				}
			}
		}

		memset(recv_data, 0, sizeof(recv_data));

		//main menu block 
		int choice = 0;
		while (choice == 0) {
			message = "\n=======WASSUPPP======= \n Please choose an option:\n 1) Play Hangman \n 2) See Scoreboard \n 3) Exit \n\nSelection: ";
			if (send(new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
			}

			bytes_received = recv(new_fd, recv_data, sizeof(recv_data), 0);

			if (strcmp(recv_data, "1") == 0) {
				choice = 1;
			} else if (strcmp(recv_data, "2") == 0) {
				choice = 2;
			} else if (strcmp(recv_data, "3") == 0) {
				choice = 3;
			} else {
				message = "Please enter 1, 2, or 3 as a selection.\n";
				if (send(new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
				}
			}
		}

		if (choice == 1) {
			//game block

			char *test = readFile();
			test[strlen(test)] = '\0';
			printf("%s", test); //Testing readFile
		
			// //Code to separate words
			char *p;
			char *wordOne;
			char *wordTwo;
		
			// p = strtok(test, ",");
		
			// if (p) {
			// 	wordOne = p;		
			// }
			
			// p = strtok(NULL, ",");
			
			// if (p) {
			// 	wordTwo = p;
			// } // won't work in the game function otherwise segmentation dump	

			//printf("%s, %s\n", wordOne, wordTwo);
		
			wordOne = "hello";
			wordTwo = "sir";

			//game(wordOne, wordTwo, &new_fd, send_data, recv_data, message); //testing game function

		} else if (choice == 2) {
			//leaderboard block
		} else {
			//gracefully exit
		}

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
	static int lineNumber, lineMatch = 0;

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

	static const char* filename = "authentication.txt";
	FILE *file = fopen(filename, "r");

	//int inputLength = strlen(input);
	int inputLength = 6;
	char* substring;
	int lineNumber, passMatch = 0;
	
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	while((read = getline(&line, &len, file)) != -1) {

		printf("lineNo: %d	currentLine: %d\n", lineNo, lineNumber);

		if (lineNumber == lineNo) {	
			if (line[strlen(line) - 1] == '\n') {
				substring = &line[strlen(line) - inputLength - 1];
				substring[strlen(substring) - 1] = '\0';
			} else {
				substring = &line[strlen(line) - inputLength];
			}

			printf("entered: %s	password: %s\n", input, substring);

			if (strcmp(substring, input) == 0) {
				passMatch = 1;
				break;
			}
		} else {
			lineNumber++;
		}
	}

	return passMatch;

}

void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, char* message) {
	
	char guess = 'a'; //tester variable
	char* board;
	char* combinedWords;
	int bytes_received, i;
	char* guessesMadeStr = "Guesses Made: ";

	int wordOneLen = strlen(wordOne);
	int wordTwoLen = strlen(wordTwo);
	int guessesNo = wordOneLen + wordTwoLen + 9;
	if( guessesNo < 26){
	}
	else{
		guessesNo = 26;	
	}

	board = malloc(wordOneLen + wordTwoLen + 3);
	combinedWords = malloc(wordOneLen + wordTwoLen + 2);

	//combine words
	combinedWords[0] = '\0';
	strcat(combinedWords, wordOne);
	strcat(combinedWords, " ");
	strcat(combinedWords, wordTwo);

	//create guesses UI <---------------------------------NEED TO FIX THIS
	int j = 0;
	//board[wordOneLen + wordTwoLen + 3] = '\0';
	board[0] = '\n';
	while (board[j] != '\0') {
		if (!(&board[j] == " " || &board[j] == "\n")) {
			putchar('_');
		}
	}

	printf("%s\n", combinedWords);

	int gameStatus = 1;
	// while (gameStatus == 1) {

	// 	if (guessesNo <= 0) {
	// 		//exit game
	// 	}

	// 	sprintf(message, "\nGuesses Left: %d\n", guessesNo);
		
	// 	if (send(*new_fd, message, strlen(message), 0) == -1 || send(*new_fd, guessesMadeStr, strlen(guessesMadeStr), 0) == -1) {
	// 		perror("send");
	// 	}

	// 	if (send(*new_fd, board, strlen(board), 0) == -1) {
	// 		perror("send");
	// 	}

	// 	bytes_received = recv(*new_fd, recv_data, sizeof(recv_data), 0);

	// 	if (isalpha(recv_data) || strlen(recv_data) == 1) { //also find a way to check if input is uppercase
	// 		for (i = 0; i < strlen(combinedWords); i++) {
	// 			if (strcmp(recv_data, &combinedWords[i]) == 0) {
	// 				board[i + 1] = combinedWords[i];
	// 			}
	// 		}
	// 		guessesNo--;
	// 	} else {
	// 		message = "Not a valid input. Please enter a lower-case letter\n";
	// 		if (send(*new_fd, message, strlen(message), 0) == -1) {
	// 			perror("send");
	// 		}
	
	// 		bytes_received = recv(*new_fd, recv_data, sizeof(recv_data), 0);
	// 	}

	// }


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
