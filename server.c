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
	#define MAXTHREADS 10
	#define NUMUSERS 12

void run(int* new_fd);
char *readFile();
int authenticate(int* new_fd, char* send_data, char* recv_data);
int authenticateUser(char* input);
int authenticatePass(char* input, int lineNo);
void printLeaderboard(int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id);
void initilizeStruct(int line, char* player);
void showMainMenu(int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id);
void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id);
int guessedAlready(char* guessedString, int charTwo);

//thread pool array
pthread_t tids[MAXTHREADS];

struct scoreBoard {
	char *player;
	int gamesWon;
	int gamesPlayed;
}u[NUMUSERS];

//create input values for thread function
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

	printf("TCP Server waiting for client\n");//, htons(my_addr.sin_port));

	//create default thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//main block
    while(1) {
		
		//create array of values for each thread
		struct thread_values thread_args[MAXTHREADS];

		for (i = 0; i < MAXTHREADS; i++) {
			//wait for connection
			sin_size = sizeof(struct sockaddr_in);

			if ((thread_args[i].new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
				perror("accept");
				continue;
			} else {

				//create a new thread if connection is made
				if (pthread_create(&tids[i], NULL, (void *(*)(void*))run, &thread_args[i]) == -1) {
					printf("ERROR creating thread");
					return EXIT_FAILURE;
				}
			}
		}
    }
	
    return 0;
}

//main function
void run(int* new_fd) {
	int userID;
	//store current thread id
	pthread_t id = pthread_self();

	//store send and receive data here
	char send_data[MAXDATASIZE], recv_data[MAXDATASIZE];
	
	//authenticate, main menu
	userID = authenticate(new_fd, send_data, recv_data);
	showMainMenu(new_fd, send_data, recv_data, userID, id);
}

//code to authenticate users
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
		//create memory space for the name char pointer
		name = malloc(strlen(recv_data)+1);
		memcpy(name, recv_data, strlen(recv_data)+1);		
		
		//return the line that the username is seen on
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

			//get password from that user line, check if same
			if (authenticatePass(recv_data, userLine) == 1) {
				printf("User %s attempted to log in and was successful\n", name);
				userID = userLine - 1;	 
				initilizeStruct(userID, name);		
				auth = 0;
			} else {
				printf("User %s attempted to log in and was unsuccessful\n", name);
				message = "password does not match\n";
				if (send(*new_fd, message, strlen(message) ,0) == -1) {
					perror("send");
				}
			}
		}
	}
	return userID; 

}

//check if entered username exists, return the line number it's found on or 0
int authenticateUser(char* input) {
	static const char* filename = "authentication.txt";
	FILE *file = fopen(filename, "r");
	
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	int inputLength = strlen(input);
	char substring[inputLength];
	static int lineNumber, lineMatch = 0;

	//find matching username
	while((read = getline(&line, &len, file)) != -1) {
		strncpy(substring, line, inputLength);
		substring[inputLength] = '\0';
		
		//return username if found
		if (strcmp(substring, input) == 0) {
			lineMatch = lineNumber;
		}
		lineNumber++;
	}

	lineNumber = 0;
	
	fclose(file);	
	return lineMatch;
}

//get password from the same line as username
int authenticatePass(char* input, int lineNo) {

	static const char* filename = "authentication.txt";
	FILE *file = fopen(filename, "r"); //open file

	int inputLength = 6;
	char* substring;
	int lineNumber, passMatch = 0;
	
	char* line = NULL;
	size_t len = 0;
	ssize_t read;

	//keep reading until EOF
	while((read = getline(&line, &len, file)) != -1) {

		//if line number matches, check password
		if (lineNumber == lineNo) {	
			if (line[strlen(line) - 1] == '\n') {
				substring = &line[strlen(line) - inputLength - 1];
				substring[strlen(substring) - 1] = '\0';
			} else {
				substring = &line[strlen(line) - inputLength];
			}

			//change return value
			if (strcmp(substring, input) == 0) {
				passMatch = 1;
				break;
			}
		} else {
			lineNumber++;
		}
	}

	//return 1 or 0
	return passMatch;

}

//main menu code
void showMainMenu(int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id) {
	char* message;
	int choice = 0;
	char *p;
	char *wordOne;
	char *wordTwo;
	
	//repeat until there is a valid choice
	while (choice == 0) {
		message = "\n===========HANGMAN=========== \n Please choose an option:\n 1) Play Hangman \n 2) See Scoreboard \n 3) Exit \n\nSelection: ";
		if (send(*new_fd, message, strlen(message) ,0) == -1) {
			perror("send");
		}

		memset(recv_data, 0, sizeof(recv_data));

		recv(*new_fd, recv_data, sizeof(recv_data), 0);

		if (strcmp(recv_data, "1") == 0) {
			choice = 1;
		} else if (strcmp(recv_data, "2") == 0) {
			choice = 2;
		} else if (strcmp(recv_data, "3") == 0) {
			choice = 3;
		} else {
			message = "Please enter 1, 2, or 3 as a selection.\n";
			if (send(*new_fd, message, strlen(message) ,0) == -1) {
				perror("send");
			}
		}
	}
	
	//play game if choice == 1
	if (choice == 1) {
		
		//game block
		char *combinedWords = readFile();
		
		//Code to separate words	
		p = strtok(combinedWords, ",");
		if (p) { wordOne = p; }
		p = strtok(NULL, ",");
		if (p) { wordTwo = p; }

		game(wordOne, wordTwo, new_fd, send_data, recv_data, userID, id); //testing game function
	
	//leaderboard block if choice == 2
	} else if (choice == 2) {
		printLeaderboard(new_fd, send_data, recv_data, userID, id);

	//send exit signal
	} else {
		message = "EXITNOW";
		if (send(*new_fd, message, strlen(message) ,0) == -1) {
			perror("send");
		}
		
		//cancel current thread
		if (pthread_cancel(id) != 0) {
			printf("There was an error canelling thread\n");
		}
	}
}

//read string from random line of hangman_text.txt
char* readFile(){
	int lineNumber;
	static const char filename[] = "hangman_text.txt";
	FILE *file = fopen(filename, "r");
	int count = 1;
	char line[288];
	char *guessWord;
	lineNumber = rand() % 289;
	//return a pointer of the randomly selected words
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
}

//game code
void game(char* wordOne, char* wordTwo, int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id) {
	
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

	//board is the string representation of what the user sees, eg: _____ ____
	//create memory space for board and string of both words
	board = malloc(wordOneLen + wordTwoLen + 2);
	combinedWords = malloc(wordOneLen + wordTwoLen + 2);

	//combine words
	combinedWords[0] = '\0';
	strcat(combinedWords, wordOne);
	strcat(combinedWords, " ");
	strcat(combinedWords, wordTwo);

	printf("Game started: %s", combinedWords);

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

		//send UI to client
		if (send(*new_fd, message, strlen(message), 0) == -1 || send(*new_fd, guessesMadeMessage, strlen(guessesMadeMessage), 0) == -1 || send(*new_fd, guessesMadeStr, strlen(guessesMadeStr), 0) == -1 || send(*new_fd, board, strlen(board), 0) == -1 || send(*new_fd, "\n", 1, 0) == -1 || send(*new_fd, "\nNew Guess: ", 13, 0) == -1) {
			perror("send");
		}

		memset(recv_data, 0, sizeof(recv_data));
		recv(*new_fd, recv_data, sizeof(recv_data), 0);

		printf("Guess made: %s\n", recv_data);

		//ensure input is alpha character and length is 1
		if (isalpha(recv_data[0]) && strlen(recv_data) == 1) {
			
			//check if letter has been used already, if not, see if it matches
			for (i = 0; i < strlen(combinedWords); i++) {
				if (recv_data[0] == combinedWords[i]) {				
					printf("Match!\n");
					if (!guessedAlready(guessesMadeStr, recv_data[0])) {
						board[i + 1] = combinedWords[i];
						charsCorrect++;
					}
				}
			}

			//add char to string of already made guesses
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

		//check if player ran out of guesses
		if (guessesNo <= 0) {
			gameStatus = 2;
		}

		//check if player won game
		if (charsCorrect == totalChars) {
			gameStatus = 3;
		}
	}

	//Game Win
	if (gameStatus == 3) {
		u[userID].gamesWon += 1;
		u[userID].gamesPlayed++;
		
		if (send(*new_fd, board, strlen(board), 0) == -1 || send(*new_fd, "\n", 1, 0) == -1) {
			perror("send");
		}
		statusMessage = "Yay! You beat the Hangman!\n";
		if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
			perror("send");		
		}
			
	}
	//Game Lose
	if (gameStatus == 2) {
		u[userID].gamesPlayed++;		
		statusMessage = "You Lose! The Hangman caught you!\n";
		if (send(*new_fd, statusMessage, strlen(statusMessage), 0) == -1) {
			perror("send");
		}
		
	}
	printf("Game Over: %s", combinedWords);
	//Display Main Menu
	showMainMenu(new_fd, send_data, recv_data, userID, id);
}

//check if char in included in string 'guessedString'
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

//initilise the structure based on the user's textfile line and name
void initilizeStruct(int line, char* player){
	u[line].player = player;
	u[line].gamesPlayed;
	u[line].gamesWon;
}

//code to print leaderboard
void printLeaderboard(int* new_fd, char* send_data, char* recv_data, int userID, pthread_t id) {
	struct scoreBoard b[NUMUSERS];
	struct scoreBoard temp;
	int swapped = 0;
	int unsorted = 1;
	char message[1048];
	int count = 0;
	//Put all user structs into another group of structs so variables are consistant to the user
	for(int k = 0; k < NUMUSERS; k++){
		b[k] = u[k];
	}		

	while(unsorted){
		swapped = 0;
		for( int j = 0; j < 3; j++){		
			//order by wins, then percentage of wins, the alphabetically
			if(b[j].gamesWon > b[j+1].gamesWon){
				struct scoreBoard temp = b[j];
				b[j] = b[j+1];
				b[j+1] = temp;
				swapped = 1;	
			}
		
			else if(b[j].gamesWon == b[j+1].gamesWon){
				float diff = (float)b[j].gamesWon/(float)b[j].gamesPlayed;
				float diff2 = (float)b[j+1].gamesWon/(float)b[j+1].gamesPlayed;
				if(diff > diff2){
					temp = b[j];
					b[j] = b[j+1];
					b[j+1] = temp;
					swapped = 1;
				}
				else if(diff == diff2){
					if(strcmp(b[j].player, b[j+1].player)<0){
					temp = b[j];
					b[j] = b[j+1];
					b[j+1] = temp;
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
	
	//Print only users that have played a game
	for( int l = 0; l < NUMUSERS; l++){
		if(b[l].gamesPlayed > 0){
			snprintf( message, sizeof message, "\n Player - %s\n Number of games won - %d \n Number of games played - %d \n=============================", b[l].player, b[l].gamesWon, b[l].gamesPlayed);
		
			if (send(*new_fd, message, strlen(message), 0) == -1) {
				perror("send");
			}
		}else{count++;}
	}
	//Print only if no user has played a game
	if(count == NUMUSERS){
		title = "\nBe the first one on the leaderboard. Play a game now!!!";
		if (send(*new_fd, title, strlen(title), 0) == -1){
			perror("send");
		}
	}
	showMainMenu(new_fd, send_data, recv_data, userID, id);
}


