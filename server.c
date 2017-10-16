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
#include <pthread.h>

	#define BACKLOG 10
	#define MAXDATASIZE 1024
	#define MAXTHREADS 5

void run(int* new_fd);
char *readFile();
int authenticate(int* new_fd, char* send_data, char* recv_data);
int authenticateUser(char* input);
int authenticatePass(char* input, int lineNo);
void printLeaderboard(int* new_fd);
void initilizeStruct(int line, char* player);
int showMainMenu(int* new_fd, char* recv_data);
void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, int userID);
int guessedAlready(char* guessedString, int charTwo);

struct scoreBoard {
	char *player;
	int gamesWon;
	int gamesPlayed;
}u[12];

struct thread_values {
	int new_fd;
};

int main (int argc, char* argv[]) {

    srand(time(NULL));
    int sockfd, i = 0;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
	socklen_t sin_size;

    //get port number
    if (argc != 2) {
        fprintf(stderr, "Port number has been set to default: 12345\n"); //configure so we have a default port
		
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

	//create socket
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
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_t tids[MAXTHREADS];

	//main block
    while(1) {
		
		struct thread_values thread_args[MAXTHREADS];

		for (i = 0; i < MAXTHREADS; i++) {
			sin_size = sizeof(struct sockaddr_in);
			if ((thread_args[i].new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
				perror("accept");
				continue;
			} else {

				if (pthread_create(&tids[i], NULL, (void *(*)(void*))run, &thread_args[i]) == -1) {
					printf("ERROR creating thread");
					return EXIT_FAILURE;
				}
			}
		}
    }
	
    return 0;
}

void run(int* new_fd) {
	int userID;
	char *p;
	char *wordOne;
	char *wordTwo;

	char send_data[MAXDATASIZE], recv_data[MAXDATASIZE];
	
	//authenticate, main menu
	userID = authenticate(new_fd, send_data, recv_data);
	int choice;
	choice = showMainMenu(new_fd, recv_data);
	
	if (choice == 1) {
		
		//game block
		char *test = readFile();
		
		// //Code to separate words	
		p = strtok(test, ",");
		if (p) { wordOne = p; }
		p = strtok(NULL, ",");
		if (p) { wordTwo = p; }

		game(wordOne, wordTwo, new_fd, send_data, recv_data, userID); //testing game function
	
	} else if (choice == 2) {
		//leaderboard block
		printLeaderboard(new_fd);
	} else {
		//gracefully exit
	}
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

int authenticate(int* new_fd, char* send_data, char* recv_data) {
	char* message;
	char* name;
	int userLine, userID;

	int auth = 1;
	while(auth) {

		memset(recv_data, 0, sizeof(recv_data));

		message = "enter username: ";
		if (send(*new_fd, message, strlen(message) ,0) == -1) {
			perror("send");
			exit(1);
		}
		
		recv(*new_fd, recv_data, sizeof(recv_data), 0);
		name = recv_data; //need to grab name but this doesnt work
		// auth username
		if ((userLine = authenticateUser(recv_data)) == 0) {
			message = "username does not match\n";
			if (send(*new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
			}
		} else {
			
			memset(recv_data, 0, sizeof(recv_data));
			message = "enter password: ";
			if (send(*new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
				exit(1);
			}

			recv(*new_fd, recv_data, sizeof(recv_data), 0);

			if (authenticatePass(recv_data, userLine) == 1) {
				userID = userLine - 1;	 
				initilizeStruct(userID, name);		
				auth = 0;
			} else {
				message = "password does not match\n";
				if (send(*new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
				}
			}
		}
	}

	return userID;

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

int showMainMenu(int* new_fd, char* recv_data) {
	char* message;
	
	while (1) {
		message = "\n=======WELCOME======= \n Please choose an option:\n 1) Play Hangman \n 2) See Scoreboard \n 3) Exit \n\nSelection: ";
		if (send(*new_fd, message, strlen(message) ,0) == -1) {
			perror("send");
		}

		memset(recv_data, 0, sizeof(recv_data));

		recv(*new_fd, recv_data, sizeof(recv_data), 0);

		if (strcmp(recv_data, "1") == 0) {
			return 1;
		} else if (strcmp(recv_data, "2") == 0) {
			return 2;
		} else if (strcmp(recv_data, "3") == 0) {
			return 3;
		} else {
			message = "Please enter 1, 2, or 3 as a selection.\n";
			if (send(*new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
			}
		}
	}

}

void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, int userID) {
	
	char* board;
	char* combinedWords;
	char* statusMessage;
	int i, j, totalChars, charsCorrect;
	char message[1048];
	char guessesMadeStr[100];
	guessesMadeStr[0] = '\0';
	char* guessesMadeMessage;

	int wordOneLen = strlen(wordOne);
	int wordTwoLen = strlen(wordTwo);
	int guessesNo = wordOneLen + wordTwoLen + 9;
	if( guessesNo < 26){
	}
	else{
		guessesNo = 26;	
	}

	board = malloc(wordOneLen + wordTwoLen + 2);
	combinedWords = malloc(wordOneLen + wordTwoLen + 2);

	//combine words
	combinedWords[0] = '\0';
	strcat(combinedWords, wordOne);
	strcat(combinedWords, " ");
	strcat(combinedWords, wordTwo);

	printf("%s\n", combinedWords);

	//create guesses UI
	board[0] = '\0';
	for (j = 0; j < wordOneLen + 1; j++) {
		if (j == 0) {
			strcat(board, "\n");
		} else {
			strcat(board, "_");
		}
	}
	strcat(board, " ");
	for (j = 0; j < wordTwoLen - 1; j++) { 
		strcat(board, "_");
	}

	totalChars = wordOneLen + wordTwoLen - 1;
	charsCorrect = 0;

	//1 = playing, 2 = lose, 3 = win
	int gameStatus = 1;
	while (gameStatus == 1) {

		snprintf(message, sizeof message, "\nGuesses Left: %d\n", guessesNo);
		guessesMadeMessage = "Guesses Made: ";

		if (send(*new_fd, message, strlen(message), 0) == -1 || send(*new_fd, guessesMadeMessage, strlen(guessesMadeMessage), 0) == -1 || send(*new_fd, guessesMadeStr, strlen(guessesMadeStr), 0) == -1 || send(*new_fd, board, strlen(board), 0) == -1 || send(*new_fd, "\n", 1, 0) == -1 || send(*new_fd, "\nNew Guess: ", 13, 0) == -1) {
			perror("send");
		}

		memset(recv_data, 0, sizeof(recv_data));
		recv(*new_fd, recv_data, sizeof(recv_data), 0);

		printf("Guess made: %s\n", recv_data);

		if (isalpha(recv_data[0]) && strlen(recv_data) == 1) {
			
			for (i = 0; i < strlen(combinedWords); i++) {
				if (recv_data[0] == combinedWords[i]) {				
					printf("Match!\n");
					if (!guessedAlready(guessesMadeStr, recv_data[0])) {
						board[i + 1] = combinedWords[i];
						charsCorrect++;
					}
				}
			}

			if (!guessedAlready(guessesMadeStr, recv_data[0])) {
				guessesNo--;
				strcat(guessesMadeStr, recv_data);
			} else {
				statusMessage = "Guess has already been made\n";
				if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
					perror("send");
				}
			}			
		
		} else {
			statusMessage = "Not a valid input. Please enter a lower-case letter\n";
			if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
				perror("send");
			}
		}

		if (guessesNo <= 0) {
			gameStatus = 2;
			
		}
		if (charsCorrect == totalChars) {
			gameStatus = 3;
		}

	}

	//do stuff based on game status
	if (gameStatus == 3) {
		u[userID].gamesWon += 1;
		u[userID].gamesPlayed++;

		if (send(*new_fd, board, strlen(board), 0) == -1 || send(*new_fd, "\n", 1, 0) == -1) {
			perror("send");
		}

		statusMessage = "Yay!\n";
		if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
			perror("send");
		}
		//printLeaderboard(new_fd);//using to test leaderboard
			
	}
	if (gameStatus == 2) {
		u[userID].gamesPlayed++;		
		statusMessage = "You Lose!\n";
		if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
			perror("send");
		}
	}
}

int guessedAlready(char* guessedString, int charTwo) {
	int guessWasMade = 0;
	int j;
	for (j = 0; j < strlen(guessedString); j++) {
		if (guessedString[j] == charTwo) {
			guessWasMade = 1;
		}
	}
	return guessWasMade;
}

void initilizeStruct(int line, char* player){
	u[line].player = player;
	u[line].gamesPlayed = 0;
	u[line].gamesWon = 0;
}


void printLeaderboard(int* new_fd) {
	struct scoreBoard test[12];
	struct scoreBoard temp;
	int swapped = 0;
	int unsorted = 1;
	char message[1048];
	
		
	for(int k = 0; k < 12; k++){
		test[k] = u[k];
	}		

	while(unsorted){
	swapped = 0;
	for( int j = 0; j < 3; j++){
		
		//order by wins, then percentage of wins, the alphabetically
		if(test[j].gamesWon < test[j+1].gamesWon){
			struct scoreBoard temp = test[j];
			test[j] = test[j+1];
			test[j+1] = temp;
			swapped = 1;	
		}
		
		else if(test[j].gamesWon == test[j+1].gamesWon){
			float test1 = (float)test[j].gamesWon/(float)test[j].gamesPlayed;
			float test2 = (float)test[j+1].gamesWon/(float)test[j+1].gamesPlayed;
			if(test1 < test2){
			temp = test[j];
			test[j] = test[j+1];
			test[j+1] = temp;
			swapped = 1;
		}
		else if(test1 == test2){
			if(strcmp(test[j].player, test[j+1].player)>0){
			temp = test[j];
			test[j] = test[j+1];
			test[j+1] = temp;
			swapped = 1;
		}						
	}
		
	}	
		if(swapped == 0){
		unsorted = 0;		
		}		
	}

	}
	
	char * title = "\n=========Leaderboard=========";
	if (send(*new_fd, title, strlen(title), 0) == -1){
				perror("send");
	}
 
	for( int l = 0; l < 4; l++){
		if(test[l].gamesPlayed > 0){
		snprintf( message, sizeof message, "\n Player - %s\n Number of games won - %d \n Number of games played - %d \n=============================\n", test[l].player, test[l].gamesWon, test[l].gamesPlayed);
		
		if (send(*new_fd, message, strlen(message), 0) == -1) {
			perror("send");
		}

		}
	}
	
}


