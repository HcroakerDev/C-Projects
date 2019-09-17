#include "duplicate.h"
#include "command_handler.h"
#include "server_connector.h"
#include "file_handler.h"
#include "common.h"

/*
 * Function: ZombieKill
 * --------------------
 * Kills any zombie process's.
 *
 * int sig: Passed to waitpid which stores information about the exit status child process's
 *
 * Returns: NIL
 */
void ZombieKill(int sig){
  if(waitpid(-1, &sig, WNOHANG)<0)
  printf("Killed me!\n");
}

/*
 * Function: cmp
 * --------------------
 * Compares two strings by checking character by character, similar functionality to strcmp()
 *
 * char str1: string 1
 * char str2: string 2
 *
 * Returns: 1 if the two strings are the same, else 0.
 */
int cmp(char *str1,char *str2){
  int s = 0;

  // Loop until terminating null.
  while(str1[s]!='\0'){
    s++;
    if(str1[s]!=str2[s]){
      return 0;
    }
  }

  if(s!=strlen(str2)){
    return 0;
  }
  return 1;
}

/*
 * Function: count_words
 * --------------------
 * Counts the number of words in a given string seperated by ' ' & '\n' using strtok.
 *
 * char str[BUFFSIZE]: string that has words
 *
 * Returns: Number of words.
 */
int count_words(char str[BUFFSIZE]){

  // Create a copy so the original is not modified.
  char strCpy[BUFFSIZE];
  strcpy(strCpy,str);

  char *s = " \n";
  char *token;

  /* get the first token */
  token = strtok(strCpy, s);
  int x = 0;

  /* walk through other tokens */
  while( token != NULL ) {
    x++;
    token = strtok(NULL, s);
  }
  return x;
}

/*
 * Function: split
 * --------------------
 * Splits a given string into an array of strings based on words seperated by spaces.
 *
 * char str[BUFFSIZE]: string that needs to be split
 * int size: number of words, use to init newStr
 * char newstrs[size][50]: a pointer to an array of strings, where the array can hold size strings of length 50.
 *
 * Returns: NIL
 */
void split(char str[BUFFSIZE], int size, char newStrs[size][50]){

  int i,j,ctr;

  j=0; ctr=0;
  for(i=0;i<=(strlen(str));i++)
  {
      // if space or NULL found, assign NULL into newStrs[ctr]
      if(str[i]==' '|| str[i]=='\0' || str[i]=='\n')
      {
          newStrs[ctr][j]='\0';
          ctr++;  //for next word
          j=0;    //for next word, init index to 0
      }
      else
      {
          newStrs[ctr][j]=str[i];
          j++;
      }
  }
}

/*
 * Function: handle_commands
 * --------------------
 * Handles commands and delegates tasks to functions based on input commands
 *
 * Returns: NIL
 */
void handle_commands(char *address){

  // Kill Zombies |0_o|
  signal(SIGCHLD, ZombieKill);
  printf("Welcome please enter a command, type help for commands or quit to exit");

  // Handle messages
  while (1) {

    char command[BUFFSIZE];
    int pid;

    printf("\n$ ");
    fgets(command, BUFFSIZE, stdin);
    command[strlen(command)-1]='\0';

    // Count the amount of words entered and split into array of strings
    int size = count_words(command);
    char strings[size][50];
    split(command,size,strings);

    if(strncmp(command,"quit",strlen("quit"))==0){
      printf("Goodbye :)\n");
      break;
    }
      
    if(strncmp(command,"help",strlen("help"))==0){
        printHelp();
        continue;
    }
    

    // Duplicate using fork()
    pid = duplicate();
    if (pid == -1)
    {
        printf("Failed\n");
        continue;
    }
    else if(pid > 0)
    {
        /* Wait for the child to die if the command is get or list as conflicts can happen
           when printing 40 lines at a time.
        */
        if(cmp(strings[0],"get") || cmp(strings[0],"list")){
          wait(NULL);
        }
        // Continue and ask for next command -> creates non blocking client.
        continue;
    }
    else if(pid == 0)
    {
        // Handle the command within a child process
        int socket_desc;
        int s = connect_server(&socket_desc,address); // Connect to server
        if(s==-1){
          perror("\nConnection error\n");
          return;
        }

        clock_t start = clock(); // Start the clock

        // See what the first command in
        if(cmp(strings[0], "put")){
          handlePutCommand(socket_desc, command, size, strings, start);
        }else if(cmp(strings[0],"get")){
          handleGetCommand(socket_desc, command, size, start);
        }else if (cmp(strings[0],"run")){
          handleRunCommand(socket_desc, command, start, size, strings);
        }else if(cmp(strings[0],"list")){
          handleListCommand(socket_desc, command, start);
        }else if(cmp(strings[0],"sys")){
          generalCommand(socket_desc, command, start);
        }else{
          // Still try and send it in case the server api has been updated without updating the client...
          generalCommand(socket_desc, command, start);
        }
        killChild(1,1);
    }

  }
  return;
}

/*
 * Function: handlePutCommand
 * --------------------
 * Handles the put command by sending each file, then sending 'Finished'.
 *
 * int socket_desc: the socket description
 * char *command: the entered command
 * int size: size of args
 * char strings: array of args
 * clock_t start: start of clock
 *
 * Returns: NIL
 */
void handlePutCommand(int socket_desc, char *command, int size, char strings[size][50], clock_t start){
  char server_reply[BUFFSIZE];

  // Ensure that the command follows standards
  if(size < 3){
    printf("\n\nError: No program name and sources files provided\n");
    killChild(1,1);
  }

  // Create list of file paths
  char *file_names[50];
  int file_count = 0;
  for(int i=2;i<size;i++){
    if(cmp(strings[i], "-f")!=1){
      file_names[file_count++]= strings[i];
    }
  }

  // Ensure source files exists
  if(files_exist(file_names, file_count)){
    printf("\n\nSending to server...\n");

    // Tell server its about to receive a file
    sendMessage(command, socket_desc);
    receiveMessage(server_reply, socket_desc);
    // Ensure the server is ready, could send back error
    if(cmp(server_reply, "Ready")){
      // For each file, send the file.
      for(int i = 0;i<file_count;i++){
        send_file(file_names[i],socket_desc); // Sends a file line by line
        sendMessage("Done", socket_desc); // Done sending one file
        receiveMessage(server_reply, socket_desc);
        printf("\nFile Sent\n"); // Feedback to user
      }
      sendMessage("Finished", socket_desc); // Send finishing command
      receiveMessage(server_reply, socket_desc);
      printf("\nAll files complete\n\n");
      printResponseTime(start); // Prints respons time
      killChild(1,1);
    }
    else{
      // Print error, ie already exists
      printf("\nRequest: %s\nReply: %s\n",command,server_reply);
      printResponseTime(start);
      killChild(1,1);
    }
  }
  else{
    printf("\n\nError: Please enter valid source files\n");
    killChild(1,1);
  }
}

/*
 * Function: handleGetCommand
 * --------------------
 * Handles the get command by receiving each line of a file until 'done'.

 * int socket_desc: Socket desc
 * char *command: Command entered
 * int size: number of words
 * clock_t start: Start time
 *
 * Returns: NIL
 */
void handleGetCommand(int socket_desc, char *command, int size, clock_t start){

  char server_reply[BUFFSIZE];

  if(size!=3){
    printf("\n\nError: Please enter a program name and a source file\n");
    killChild(1, 1);
  }
  printf("\nGetting source file from server...\n\n");

  // Tell server we are about to request a file
  sendMessage(command, socket_desc);
  int count = 0;

  // Recieve line by line the file and print to screen 40 at a time.
  while(1){
    receiveMessage(server_reply, socket_desc);
    if(strncmp(server_reply, "Error", strlen("Error"))==0){
      // Print error
      printf("\nRequest: %s\nReply: %s\n",command,server_reply);
      printResponseTime(start);
      killChild(1, 0);
    }else if(strncmp(server_reply, "Done", strlen("Done"))==0){
      // Send back Success
      printf("\nFile finished\n\n");
      sendMessage("Success", socket_desc);
      printResponseTime(start);
      killChild(1, 0);
    }else{
      // Show to screen 40 at a time
      printf("%s",server_reply);
      count++;
      if (count>=40) {
        // Wait for return to be pressed, any key would require altering terminal behaviour.
        printf("\nPress return to continue...\n");
        getchar();
        count=0;
      }
      sendMessage("Ok", socket_desc); // Received
    }
  }

}

/*
 * Function: generalCommand
 * --------------------
 * Handles general commands as well as sys.

 * int socket_desc: Socket desc
 * char *command: Command entered
 * clock_t start: Start time
 *
 * Returns: NIL
 */
void generalCommand(int socket_desc, char *command, clock_t start){
  char server_reply[BUFFSIZE];
  sendMessage(command, socket_desc);
  receiveMessage(server_reply, socket_desc);
  printf("\n\nServer reply: \n\n%s\n",server_reply);
  printResponseTime(start);
}

/*
 * Function: handleListCommand
 * --------------------
 * Handles general commands as well as sys.

 * int socket_desc: Socket desc
 * char *command: Command entered
 * clock_t start: Start time
 *
 * Returns: NIL
 */
void handleListCommand(int socket_desc, char *command, clock_t start){

  char server_reply[BUFFSIZE];

  sendMessage(command, socket_desc);
  printf("\nServer List:\n");
  int count = 0;
  while(1){
    receiveMessage(server_reply, socket_desc);
    if(cmp(server_reply, "Done")){
      sendMessage("Success", socket_desc);
      printf("\nList finished\n\n");
      printResponseTime(start);
      killChild(1, 0);
    }
    else if(strncmp(server_reply, "Error", strlen("Error"))==0){
      printf("\n%s\n",server_reply);
      printResponseTime(start);
      killChild(1, 0);
    }
    count++;
    if (count>=40) {
      printf("\nPress return to continue...\n");
      getchar();
      count=0;
    }
    printf("%s",server_reply);
    sendMessage("Ok", socket_desc);
  }
}

/*
 * Function: handleRunCommand
 * --------------------
 * Handles the run command with args by receiving each line of a output until 'done' Writes to localfile if specified.
 * run progname sourcefile [args] [[-f][-w localfile]].
 * Output could include compile errors or runtime and jus regular output.
 *
 * int socket_desc: Socket desc
 * char *command: Command entered
 * clock_t start: Start time
 * int size: number of words
 * char strings[size][50]: array of strings
 *
 * Returns: NIL
 */
void handleRunCommand(int socket_desc, char *command, clock_t start, int size, char strings[size][50]){

  char server_reply[BUFFSIZE];
  int write_local=cmp(strings[size-2], "-w");
  int force = cmp(strings[size-3], "-f");

  // If the user specified to write local
  if(write_local){
    char *files_names[50];
    files_names[0]=strings[size-1];

    // Check the the localfile doesnt exist or force is specified
    if((!files_exist(files_names, 1)) || force){
      // Can write to file
      // printf("Can write to local file %s\n",strings[size-1]);

      sendMessage(command, socket_desc); // Ensures progname and sourcefile exists
      receiveMessage(server_reply, socket_desc);
      if(strncmp(server_reply,"Error",strlen("Error"))==0){
        // Print error
        printf("%s\n", server_reply);
        printResponseTime(start);
        killChild(1,1);
      }
      fclose(fopen(strings[size-1],"w")); // Clear the file contents

      FILE *fptr;
      fptr = fopen(strings[size-1],"a");

      if(fptr == NULL){
        printf("\nFile: %s could not be opened \n", strings[size-1]);
        sendMessage("Not ready", socket_desc);
        return;
      }
      sendMessage("Ready", socket_desc); // Ready to receive

      // Receive file line by line and write to file until 'done'.
      while(1){
        receiveMessage(server_reply, socket_desc);
        // printf("%s",server_reply);
        if(cmp(server_reply, "Done")){
          printf("\nExecute finished and output saved to file: %s\n",strings[size-1]);
          printResponseTime(start);
          break;
        }
        fputs(server_reply, fptr);
        sendMessage("Ok", socket_desc);
      }
      fclose(fptr);
    }else{
      printf("\nError: Local File %s already exits.\n",strings[size-1]);
      killChild(1,1);
    }
  }
  else{

    // Print execution output to screen
    printf("\n\nExecution output:\n");
    sendMessage(command, socket_desc); // Ensure it exists
    receiveMessage(server_reply, socket_desc);

    // print error
    if(strncmp(server_reply,"Error",strlen("Error"))==0){
      printf("%s\n", server_reply);
      printResponseTime(start);
      killChild(1,1);
    }
    sendMessage("Ready", socket_desc);
    while(1){
      receiveMessage(server_reply, socket_desc);
      if(cmp(server_reply, "Done")){
        printf("\nExecute finished\n");
        printResponseTime(start);
        break;
      }
      printf("%s",server_reply);
      sendMessage("Ok", socket_desc);
    }
  }
}

/*
 * Function: printResponseTime
 * --------------------
 * Prints reponse time of server based on start time
 *
 * clock_t start: Start time
 *
 * Returns: NIL
 */
void printResponseTime(clock_t start){
  printf("Server Response Time: %lfs\n",(double)(clock() - start)/CLOCKS_PER_SEC);
}

/*
 * Function: killChild
 * --------------------
 * Kills a child process and prints '$' if specified.
 *
 * int exitStatus: exitStatus
 * int printEnter: If '$' should be printed, on wait commands it shouldn't
 *
 * Returns: NIL
 */
void killChild(int exitStatus, int printEnter){

  if(printEnter)
  printf("\n$ ");

  exit(exitStatus);
}

/*
 * Function: send_file
 * --------------------
 * Sends a file line by line to the server.
 *
 * char *file_name: filename,
 * int socket_desc: socket desc
 *
 * Returns: NIL
 */
int send_file(char *file_name, int socket_desc){

  char server_reply[BUFFSIZE];
  printf("\nSending File: %s\n",file_name);

  FILE *fptr;
  fptr = fopen(file_name,"r");

  if(fptr == NULL){
    printf("\nFile: %s could not be opened\n", file_name);
    killChild(1,1);
  }

  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  // Send line by line
  while ((read = getline(&line, &len, fptr)) != -1) {
      // printf("%s", line);
      line[read]='\0';
      sendMessage(line, socket_desc);
      receiveMessage(server_reply, socket_desc);
  }

  fclose(fptr);
  if (line)
      free(line);
  return 1;
}

/*
 * Function: sendMessage
 * --------------------
 * Sends a message to the server
 *
 * char *message: pointer to message string
 * int socket_desc: socket dec
 *
 * Returns: 1 if success, 0 if fail
 */
int sendMessage(char *message, int socket_desc){
  // printf("%s\n", message);
  //Send some data
  if( send(socket_desc , message , strlen(message) , 0) <= 0)
  {
      puts("Send failed");
      return 0;
  }
  // puts("Data Send\n");
  return 1;
}

/*
 * Function: receiveMessage
 * --------------------
 * receives message from server
 *
 * char *server_reply: pointer where server reply should be saved
 * int socket_desc: socket dec
 *
 * Returns: 1 if success, 0 if fail
 */
int receiveMessage(char *server_reply,int socket_desc){
  int bytes;
  //Receive a reply from the server
  while((bytes = recv(socket_desc, server_reply , 2000 , 0)) > 0)
  {
      server_reply[bytes] = '\0';
      return 1;
  }

  if(bytes == 0)
  {
      puts("Client disconnected");
      fflush(stdout);
  }
  else if(bytes == -1)
  {
      perror("recv failed");
      killChild(1, 1);
  }

  return 0;
}

void printHelp(){
    printf("\nCommand: put\n");
    printf("Sends a file to the server and puts it inside a specific program directory\n");
    printf("-----------\n");
    printf("Usage Eg: put test test.txt (-f)\n");
    printf("          ^    ^       ^      ^\n");
    printf("         cmd program source force override\n");
    printf("-----------\n");
    
    printf("\nCommand: get\n");
    printf("Gets a source file from the server and prints to the screen\n");
    printf("-----------\n");
    printf("Usage Eg: get test test.txt\n");
    printf("          ^    ^      ^\n");
    printf("         cmd program source\n");
    printf("-----------\n");
    
    printf("\nCommand: list\n");
    printf("Lists server programs list or the files inside a program.\n");
    printf("-----------\n");
    printf("Usage Eg: list (-l) (progName)\n");
    printf("           ^    ^       ^\n");
    printf("          cmd  long progam name\n");
    printf("-----------\n");
    
    printf("\nCommand: run\n");
    printf("Compiles and runs a program within the server and print results back to screen. Either run all c files inside or specific c files if provided.\n");
    printf("-----------\n");
    printf("Usage Eg: run  test (sourceFile.c)\n");
    printf("           ^    ^       ^\n");
    printf("          cmd program optional source\n");
    printf("-----------\n");
    
    printf("\nCommand: sys\n");
    printf("Prints the system of the server (OS Version and CPU).\n");
    printf("-----------\n");
    printf("Usage Eg: sys\n");
    printf("           ^\n");
    printf("          cmd\n");
    printf("-----------\n");
    
    
}
