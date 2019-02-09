#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#define PORT 5558
#define MAXLENGTH 80
#define ADDITION 8

//Array to hold the last 10 most recent commands, up to 19 letters in length
char* historyCommands[10];
int numberOfCommands = -1;
char* command;
int l_socket;

/**
 * Adds the command to history, and shifts down the commands
 * if there have been more than 10 commands run. Commands are
 * added after it has been run.
 */
static void addToHistory(const char *command){
    if(numberOfCommands == 10){
        //shift up the number of commands, the oldest is at 0
        historyCommands[0] = historyCommands[1];
        historyCommands[1] = historyCommands[2];
        historyCommands[2] = historyCommands[3];
        historyCommands[3] = historyCommands[4];
        historyCommands[4] = historyCommands[5];
        historyCommands[5] = historyCommands[6];
        historyCommands[6] = historyCommands[7];
        historyCommands[7] = historyCommands[8];
        historyCommands[8] = historyCommands[9];
        historyCommands[9] = strdup(command);
    }
    else{
        numberOfCommands++;
        historyCommands[numberOfCommands]= strdup(command);
    }
}

/**
 * Encrypts the command using ascii values and adding
 * the last digit of my school id, 8, and then sends the command
 */
static void sendEncryptedCommand(const char *command){
    //Add the last command to the history array
    addToHistory(command);
    int i;
    int encrypted[strlen(command)];
    for(i =0; i<strlen(command); i++){
        encrypted[i] = ((int) command[i]) + ADDITION;
    }
    int position = 0;
    char str[(strlen(command)+1)*4];
    for(i = 0; i<strlen(command); i++){
        position += sprintf(&str[position], "%d", encrypted[i]);
        if(i+1 < strlen(command)){
            position += sprintf(&str[position], "%s", " ");
        }
    }
    char buffer[1024];
    int valread;
    //send the command
    send(l_socket, str, sizeof(str), 0);
    //Read the information coming in from the server
    //Use to see if anything was read, if nothing was read, it is due to an error
    int amountRead=0;
    while((valread = recv(l_socket, buffer, 1024, 0)) > 0){
        if(strcmp(buffer, "DONE") == 0){
            break;
        }
        //print what the server said
        printf("%s", buffer);
        amountRead++;
        
    }
    if(amountRead == 0){
        printf("    Nothing was read.\n");
    }
    
}

/**
 * Prints the history of the commands. Says no commands
 * in history if there haven't been any commands.
 */
static void printHistory(){
    //Add the last command to the history array
    addToHistory("History");
    if(numberOfCommands == 0){
        printf("No commands in history\n");
    }
    else{
        int i=0;
        while(i < numberOfCommands){
            printf("%d. %s\n", i, historyCommands[i]);
            i++;
        }
    }
}

/**
 * If the user enters "!n" then the nth command will
 * be run. If the user enters !!, then the last command
 * will be run. 
 */
static void runSpecificHistory(int com){
    if(historyCommands[com] == NULL){
        printf("No %dth command in history\n", com);
    }
    else{
        if(strcmp(historyCommands[com], "History") == 0 || strcmp(historyCommands[com], "history") == 0){
            printHistory();
        }
        //Run specific previous command
        else if(historyCommands[com][0] == '!'){
                runSpecificHistory((int) historyCommands[com][1]);
        }
        else{
            sendEncryptedCommand(historyCommands[com]);
        }
    }
}

/**
 * Main method
 */
int main(int argc, char *argv[]){
	struct sockaddr_in address, server_address;
    l_socket = 0;
	int shouldrun = 1; //Flag for when the program should exit
	l_socket = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_address, '0', sizeof(server_address));
	server_address.sin_family = AF_INET; //Binds to all local interfaces
	server_address.sin_port = htons(PORT); //Port to send and listen on, 5558
	//make sure that there is enough information entered to start the program
	if(argc < 2){
		printf("To run this program, please put input of the ip address of the server");
		exit(0);
	}
	else{
		//argv[1] should be the ip address of the server
		inet_pton(AF_INET, argv[1], &server_address.sin_addr);
	}
	//Try to connect to the server
	if(connect(l_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0){
		//If you can't connect, it is likely that the server program wasn't up
		printf("Please start the server first \n");
		exit(0);
	}
	//Run as long as the user doesn't enter "quit"
	while(shouldrun){
        //Handle the next command
        printf("Enter the next command (or type quit):");
        char str2[1024];
        //Get the command from the user
        fgets(str2, 1024, stdin);
        //carry it into the command string
        command = malloc(sizeof(str2) + 1);
        strcpy(command, str2);
        if(strlen(command) > 0){
            //if the string isn't empty, take out the new line
            strtok(command, "\n");
        }
        //Divide into multiple commands
        char *commands = strtok(command, ";");
        char firstCharacter; //First character of command (to help with running historical commands)
        while(commands != NULL){
            firstCharacter = commands[0];
            //User wants to quit the program
            if(strcmp(commands, "quit") == 0 || strcmp(commands, "Quit") == 0){
                send(l_socket, "quit", sizeof("quit"), 0);
                printf("Exiting the program... \n");
                shouldrun=0;
                exit(0);
            }
            //Run last command
            else if(strcmp(commands, "!!") == 0){
                runSpecificHistory(numberOfCommands);
            }
            //Run specific previous command
            else if(firstCharacter == '!'){
                if(isdigit(commands[1]) && strlen(commands) == 2){
                    char *temp = &commands[1];
                    runSpecificHistory(atoi(temp));
                }
                else{
                    //treat it as a normal command
                    sendEncryptedCommand(commands);
                }
            }
            else if(strcmp(commands, "History") == 0 || strcmp(commands, "history") == 0){
                printHistory();
            }
            else{
                sendEncryptedCommand(commands);
            }
            
            commands = strtok(NULL, ";");
        }
        
	}
    free(command);
	return 0;
}


