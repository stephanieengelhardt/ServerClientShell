#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#define PORT 5558
#define ADDITION 8

/**
 * Handles when the child processes become zombies
 **/
void sigHandler(int sig){
    int pid;
    //Tries to see if any child id is terminated, if so, collect it
    //WNOHANG makes it so it doesn't wait on each pid & -1 is for all of the child ids
    while((pid = waitpid((pid_t) (-1), 0, WNOHANG)) > 0);
}

/**
 * Main method
 */
int main(int argc, char *argv[]){
	int server_file_desc, l_socket;
	struct sockaddr_in server; //Server socket that is started in this program
    char buff[2048]; //Buffer used to read inputs
	int addrlen = sizeof(server);
	int opt = 1;
	server_file_desc = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(server_file_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	server.sin_family = AF_INET; //Use internet domain protocols
	server.sin_addr.s_addr = INADDR_ANY; 
	server.sin_port = htons(PORT); //Listen on defined port, 5558
	//Try to bind to the socket
	if(bind(server_file_desc, (struct sockaddr *) &server, sizeof(server)) < 0){
		printf("Bind failure \n");
		exit(0);
	}
	//Try to listen for requests to connect
	if(listen(server_file_desc, 3) < 0){
		printf("Listen failure \n");
		exit(0);
	}
	//Let user know that is running
	printf("Server is up and listening on port %d \n", PORT);
	while(1){
        int id;
		//Parent process should always be trying to accept a new connection request
		if((l_socket = accept(server_file_desc, (struct sockaddr *) &server, (socklen_t *) &addrlen)) < 0){
			perror("Couldn't accept the connection from the client");
			exit(1);
		}
        signal(SIGCHLD, sigHandler);
		if((id=fork()) == 0){ //Child process should handle the new client
            
			while(1){
                char answer[1024];
                //Reset the buffer so old comments don't end up in the buffer
                memset(buff, 0, 1024 * sizeof(char));
				//Read the command coming in from the client
                read(l_socket, buff, sizeof(buff));
                //Then convert the numbers from ASCII to the actual command
                char realCommand[20];
                memset(realCommand, 0, 20* sizeof(char));
                char *temp;
                int position = 0;
                temp = strtok(buff, " ");
                while(temp != NULL){
                    //Remove what was added on the client side
                    int asciiValue = (atoi(temp)) - ADDITION;
                    //Get the char from the ascii value
                    char character = (char) asciiValue;
                    //Add the character to the command
                    realCommand[position] = character;
                    position++;
                    //Get the next ascii value
                    temp = strtok(NULL, " ");
                }
                FILE *command;
                if(strcmp(realCommand, "quit")==0){
                    close(l_socket);
                    exit(0);
                }
                else if(strcmp(realCommand, "jobs")==0 || strcmp(realCommand, "Jobs") == 0){
                    //Run ps to get all the executing pids
                    command = popen("ps", "r");
                    //You only want the pids that are running the server program
                    char *findString = "server";
                    int lengthOfServer = strlen(findString);
                    //get response to executing ps
                    while((fgets(answer, 1024, command)) > 0){
                        int currentLength = strlen(answer);
                        int i;
                        int j;
                        //see if the line is for a process running server
                        for(i = 0; i<= currentLength - lengthOfServer; i++){
                            for(j = 0; j <lengthOfServer; j++){
                                if(answer[i+j] != findString[j]){
                                    //doesn't include server, break
                                    break;
                                }
                            }
                            //found the entire word "server"
                            if(j == lengthOfServer){
                                int parentID = getppid();
                                //add a space
                                char *sendOver = strtok(answer, " ");
                                int foundID = atoi(sendOver);
                                //only print the child processes, not the parent process
                                if(parentID != foundID){
                                    send(l_socket, strcat(sendOver, "\n"), 1024, 0);
                                }
                            }
                        }
                    }
                }
                //Use popen and fgets to perform the command and then send the output to the client
                //Adding "2>&1" sends all the stderr to stdout to be printed
                else if((command = popen(strcat(realCommand, " 2>&1"), "r")) != NULL){
                    while((fgets(answer, 1024, command)) > 0){
                        send(l_socket, answer, 1024, 0);
                    }
                    pclose(command);
                }
               
                //If there was an error, nothing will be printed out, it'll just get the command done. When there wasn't an error, this indicates that output is done
                send(l_socket, "DONE", sizeof("DONE"), 0);
				//Reset the buffer so old comments don't end up in the buffer
				memset(buff, 0, 1024 * sizeof(char));
                memset(realCommand, 0, 20* sizeof(char));
			}
		}
		else{ //Parent process is done handling the client, the child process will continue to do that
			close(l_socket);
		}
	}
	return 0;
}
