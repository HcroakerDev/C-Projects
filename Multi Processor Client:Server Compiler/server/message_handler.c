#include "common.h"
#include "duplicate.h"
#include "file_handler.h"
#include "message_handler.h"

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
 * Function: handleMessages
 * --------------------
 * Handles messages from client
 *
 * int *socket_desc: ptr to socket_desc
 *
 * Returns: NIL
 */
void handleMessages(int *socket_desc){


    char client_message[BUFFSIZE];

    printf("Waiting for messages\n");
    //Receive a message from client

    if(receiveMessage(*socket_desc, client_message)!=1){
      exit(1);
    }


    int size = count_words(client_message);
    char strings[size][50];
    split(client_message,size,strings);

    // Handle put command but receiving line by line
    if(cmp(strings[0], "put")){

      int force=cmp(strings[size-1],"-f");

      // Make a dir, force if -f is present
      if(makeDir(strings[1],force)){
        write(*socket_desc,"Ready", strlen("Ready")); // send ready

        // For each file receive it
        for(int i=2;i<size;i++){
          if(cmp(strings[i], "-f")!=1){
            char file_path[128];
            sprintf(file_path, "./%s/%s", strings[1], strings[i]); // Gen file path
            fclose(fopen(file_path, "w"));// clear contents
            receiveFile(file_path, *socket_desc);
          }
        }
        receiveMessage(*socket_desc, client_message);
        // If finished send back success
        if (cmp(client_message, "Finished")) {
          write(*socket_desc , "Success", strlen("Success"));
        }else{
          // Handle error
          printf("\nError: client does not seem to be finished %s\n",client_message);
        }

      }
      else{

        // Handle error of not forceing and prog already exists
        printf("Dir Already exists\n");
        char message[128];
        sprintf(message, "Error: Program name '%s' already exists", strings[1]);
        write(*socket_desc , message, strlen(message));
      }

    }else if(cmp(strings[0],"get")){

      // Handle get command
      char file_path[128];
      sprintf(file_path, "./%s/%s", strings[1], strings[2]);

      // Check file exists
      if(fileExists(file_path)){
        // send file
        sendFile(file_path,*socket_desc);
      }
      else{

        // Handle error of not existing
        char message[128];
        sprintf(message, "Error: Source file '%s' within '%s' does not exist", strings[2],strings[1]);
        write(*socket_desc , message, strlen(message));
      }


    }else if (cmp(strings[0],"run")){

      // hanlde run command.
      // Check the directory
      checkDir(*socket_desc, size, strings);
      char execute[256];
      sprintf(execute, "./%s/%s.out", strings[1],strings[1]);
      if(size>2){
        for(int i=2; i<size ; i++){
          if(cmp(strings[i], "-f") || cmp(strings[i], "-w")){
            break;
          }
          strcat(execute, " ");
          strcat(execute, strings[i]);
        }
      }
      strcat(execute, " 2>&1");

      // Compile and send compile errrors if any
      if(compileAndSend(*socket_desc, strings[1])){

        // If compile successfully then execute and send output.
        executeAndSend(execute, *socket_desc);
      }

    }else if(cmp(strings[0],"list")){
      // Handle list
      handleListCommand(*socket_desc, size, strings);
    }else if(cmp(strings[0],"sys")){
      // Send sys info
      sendSystemInfo(*socket_desc);
    }
    else{
      // Not valid error
      write(*socket_desc , "Error: Not a valid command", strlen("Error: Not a valid command"));
    }
    exit(1); // Kill child process

}

// Check directory exists and send back to client it exists
int checkDir(int socket_desc, int size, char strings[size][50]){

  char client_message[BUFFSIZE];
  char file_path[128];
  sprintf(file_path, "./%s/%s.out", strings[1],strings[1]);
  DIR* dir = opendir(strings[1]);
  int errno;
  if (dir)
  {
      /* Directory exists. */
      closedir(dir);
      printf("Directory exists\n");
      write(socket_desc , "Exists", strlen("Exists"));
      receiveMessage(socket_desc, client_message);
      if(cmp(client_message, "Ready")!=1){
        printf("Client not ready...\n");
        exit(1);
      }
  }
  else if (ENOENT == errno)
  {
      /* Directory does not exist. */
      printf("Directory does not exist\n");
      write(socket_desc , "Error: Program does not exist!", strlen("Error: Program does not exist!"));
      exit(1);
  }
  else
  {
      /* opendir() failed for some other reason. */
      char return_message[BUFFSIZE];
      sprintf(return_message, "Error: %s ", strerror(errno));
      write(socket_desc , return_message, strlen(return_message));
      exit(1);
  }
  return 1;
}

// Handle list command
int handleListCommand(int socket_desc, int size, char strings[size][50]){

  char client_message[BUFFSIZE];

  if(size==1){
    // sedn prog names
    sendPrograms(socket_desc);
  }else if(size==2){
    if(cmp(strings[size-1], "-l")){
      // Just list prog names
      printf("\nHere\n");
      sendPrograms(socket_desc);
    }else{
      // Just list files in progname
      printf("\nOver Here\n");
      sendSourceFiles(strings[1],0,socket_desc);
    }
  }else if(size==3){
    if(cmp(strings[size-2], "-l")){
      // Long list of files in progname
      char file_path[128];
      sprintf(file_path, "./%s",strings[2]);
      sendSourceFiles(file_path,1,socket_desc);
    }
    else{
      // Invalid
      write(socket_desc , "Error: Invalid Command sequence; list [-l] [Program name]", strlen("Error: Invalid Command sequence; list [-l] [Program name]"));
      receiveMessage(socket_desc, client_message);
      write(socket_desc , "Done", strlen("Done"));
      receiveMessage(socket_desc, client_message);

      if(cmp(client_message, "Success")!=1){
        perror("Error with client");
      }
      exit(1);
    }
  }
  return 1;
}

// Send system info, returns success
int sendSystemInfo(int socket_desc){

  struct utsname buf;
  char return_message[BUFFSIZE];

  uname(&buf);
  // printf("%s %s\n",buf.sysname, buf.release);

  FILE *fp;
  char output[256];

  fp = popen(cpu(), "r");
  if (fp == NULL){
    // Unkown error
    perror("Cpu info");
    exit(1);
  }

  // Get output
  fgets(output, BUFFSIZE, fp);
  sprintf(return_message, "OS: %s\nVersion: %s\nCPU: %s", buf.sysname, buf.release, output);
  write(socket_desc , return_message, strlen(return_message));
  pclose(fp);
  return 1;

}

// Compile .c files and send errors if any, returns success (1,0 respectively)
int compileAndSend(int socket_desc, char *progname){

  FILE *fp;
  int status;
  char output[BUFFSIZE];
  char compile_command[128];
  char client_message[BUFFSIZE];

  // Compoile all c files
  sprintf(compile_command, "gcc ./%s/*.c -o ./%s/%s.out 2>&1", progname,progname,progname); // Pipe to stdin
  fp = popen(compile_command, "r");
  if (fp == NULL){
    // Unkown error
    perror("Compile");
    write(socket_desc , "Error: Compile could not be completed", strlen("Error: Compile could not be completed"));
    return 0;
  }

  int compile_err = 0;
  // Will be output if compile err;
  while (fgets(output, BUFFSIZE, fp) != NULL){
    // printf("%s", output);
    write(socket_desc , output, strlen(output));
    receiveMessage(socket_desc, client_message);
    compile_err = 1;
  }

  status = pclose(fp);
  if(compile_err){
    write(socket_desc , "Done", strlen("Done"));
    return 0;
  }
  return 1;

}

// Exiecute and send output
int executeAndSend(char *executeCmd, int socket_desc){
  FILE *fp;
  char output[BUFFSIZE];
  char client_message[BUFFSIZE];
  fp = popen(executeCmd, "r");
  if (fp == NULL){
    // Unkown error
    perror("Run");
    write(socket_desc , "Error: Run could not be completed", strlen("Error: Run could not be completed"));
    return 0;
  }

  int count = 0;
  // Will be output the results of the program;
  while (fgets(output, BUFFSIZE, fp) != NULL){
    // printf("%s", output);
    write(socket_desc , output, strlen(output));
    receiveMessage(socket_desc, client_message);
    if(count++ > 1000){
      break;
    }
  }

  pclose(fp);
  write(socket_desc , "Done", strlen("Done"));
  return 1;
}

// Send long list
void sendSourceFiles(char *filePath, int longList, int socket_desc){
  FILE *fp;
  int status;
  char command[128];
  char output[BUFFSIZE];
  char client_message[BUFFSIZE];

  if(longList){
    sprintf(command, "ls -l %s 2>&1", filePath);
  }
  else{

    DIR* dir = opendir(filePath);
    int errno;
    if (dir)
    {
        sprintf(command, "ls %s 2>&1", filePath);
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        printf("Directory does not exist\n");
        write(socket_desc , "Error: Program does not exist!", strlen("Error: Program does not exist!"));
        exit(1);
    }


  }

  fp = popen(command, "r");
  if (fp == NULL){
    // Unkown error
    printf("\nError: Command could not be completed\n");
    exit(1);
  }

  // Get output
  while (fgets(output, BUFFSIZE, fp) != NULL){
    // printf("%s", output);
    write(socket_desc , output, strlen(output));
    receiveMessage(socket_desc, client_message);
  }
  status = pclose(fp);
  write(socket_desc , "Done", strlen("Done"));
  receiveMessage(socket_desc, client_message);

  if(cmp(client_message, "Success")!=1){
    perror("Error with client");
  }
  exit(1);
}

// Send programs (All dir)
void sendPrograms(int socket_desc){

  FILE *fp;
  int status;
  char output[BUFFSIZE];
  char client_message[BUFFSIZE];

  fp = popen("find * -maxdepth 0 -type d 2>&1", "r");
  if (fp == NULL){
    // Unkown error
    printf("\nError: Compile could not be completed\n");
    exit(1);
  }

  // Will be output if compile err;
  while (fgets(output, BUFFSIZE, fp) != NULL){
    // printf("%s", output);
    write(socket_desc , output, strlen(output));
    receiveMessage(socket_desc, client_message);
  }
  status = pclose(fp);
  write(socket_desc , "Done", strlen("Done"));
  receiveMessage(socket_desc, client_message);

  if(cmp(client_message, "Success")!=1){
    perror("Error with client");
  }
  exit(1);
}

// Receive a file line by line.
int receiveFile(char *file_name, int socket_desc){

  FILE *fptr;
  fptr = fopen(file_name,"a");
  if(fptr == NULL){
    printf("File: %s could not be opened\n", file_name);
    return 0;
  }

  while(1){
    char line[LINESIZE];
    receiveMessage(socket_desc, line);
    // printf("Received line %s", line);
    if (cmp(line, "Done")) {
      write(socket_desc , "Ok", strlen("Ok"));
      break;
    }
    if(appendFile(fptr,line)){
      // printf("Line written\n");
      write(socket_desc , "Ok", strlen("Ok"));
    }else{
      // Error writing line but don't worry to much, only will happen on big big files...
      perror("Line");
      write(socket_desc , "Ok", strlen("Ok"));
    };
  };
  fclose(fptr);
  return 1;

}

// Send a file line by line.
int sendFile(char *file_name,int socket_desc){

  char client_reply[2048];
  printf("Sending File: %s\n",file_name);

  FILE *fptr;
  fptr = fopen(file_name,"r");

  if(fptr == NULL){
    printf("File: %s could not be opened\n", file_name);
    exit(1);
  }

  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, fptr)) != -1) {
      // printf("Retrieved line of length %zu :\n", read);
      // printf("%s", line);
      write(socket_desc , line, strlen(line));
      receiveMessage(socket_desc,client_reply);
      if(cmp(client_reply, "Ok")!=1){
        printf("Error sending line %s\n", line);
        return 0;
      }
  }

  fclose(fptr);
  if (line)
      free(line);

  char *message = "Done";
  write(socket_desc,message, strlen(message));
  receiveMessage(socket_desc,client_reply);
  if(cmp(client_reply, "Success")!=1){
    printf("\nError sending file\n");
    return 0;
  }

  return 1;
}


// Receive a message.
int receiveMessage(int socket_desc,char *message){
  int read_size;
  while( (read_size = recv(socket_desc , message , BUFFSIZE , 0)) > 0 )
  {
    message[read_size] = '\0';
    return 1;
  }

  if(read_size == 0)
  {
      printf("\nClient disconnected\n");
      return 0;
      fflush(stdout);
  }
  else if(read_size == -1)
  {
      perror("recv failed");
      return 0;
  }
  return 0;
}
