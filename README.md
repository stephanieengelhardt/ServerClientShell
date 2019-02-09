**Overview:** 
This project allows multiple clients to connect to a server. The clients send encrypted commands to the server
and then the server decrypts the command and then the server sends the response back to the client. All normal
command line commands can be run, in addition to these special commands: 
    1. History: This will list the history of the commands. It will list up to the last 10 commands. 
        It starts indexing from 0, where 0 is the earliest command run, and the 9th command is
        the latest command ran. 
    2. !!: This runs the last command. 
    3. !N: This will run the Nth command in the History list. Note that the history list starts at 0. 
    4. jobs: this lists all the current child processes connected to the server. 
You can also send multiple commands at once by seperating each command with a semi-colon. 


**How to compile the program:**
Within the same folder as the .c files, type in "make" on the command line. The make file will compile server.c 
to server and client.c to client.


**How to run the program:**
First, start the server, by typing in ./server after compiling the program. Second, start the client, by typing in ./client
<ip_address_of_the_server>. For example: 
    ./client 127.0.0.1
Then you will be prompted to by the client to enter a command. Enter a command and hit enter. For example:
    ls -l
    History
    jobs
    date;!!;History
    date; ls
**PLEASE NOTE:** When running a special command, it is SPACE sensitive. So, if you are running multiple commands 
with a special command, you cannot have a space in between the commands. If all commands are not special within a 
multi-command input, there can be spaces.  Special commands are NOT case sensitive.


**Exiting the program:**
To exit the client, type in "quit" or ctrl + c.
To exit the server, you have to do ctrl + c.


**Details:**
Connections: 
The server is listening / sending on port 5558.  When the server gets a client trying to request, it will spin 
off a child process to execute the command and continue listening for further commands. Stderr is piped to stdout in
order to handle any errors. The parent process is then able to close the connection between the parent and the client
and listen for more clients.  If there is no input read when a command is performed, it will tell the client there was no 
input read. 

Encryption: 
The client sends the command encrypted (by converting each character to the ASCII value and adding 8). 
The server then decrypts the command, by subtracting 8 and converting the ASCII value to a character.

Jobs command: 
When the server gets the special command, jobs, it runs "ps" to get the PIDs of all of the current threads. It then
gets all the PIDs of the threads running the server program. Then, it takes away the parent ID (because only the
child process IDs are sent). It sends over each PID to the client one at a time. 

Running a historical/ History  command: 
The client, before sending over the command, checks to see if it is a previous command or a history command. If so,
it handles the command on the client side. The history of the last commands are kept in a char pointer array on the client 
side. When running a specific command, it adds whatever command it ends up being into the history. For example, if the 
history was:
0. ls
1. date
And you call !!, then the last command, #1, would be added to the history. This is  the same for !N commands. To print the 
history, it simply prints out a new line per array element. 

Quitting and zombie processes:
Once the client types in quit, before exiting the program, the client sends over quit, which will terminate the child
process. However, if the client just types in control + C, the server collects the child process in a different way. Instead,
there is a signal handler the parent calls, which is a function to handle signal from the child that it has terminated. The 
funciton loops through all of the child IDs, without blocking, by calling: waitpid((pid_t) (-1), 0, WNOHANG). WNOHANG 
stands for wait no hang (meaning do not block). The pid of -1 means all of the children PIDs. 
