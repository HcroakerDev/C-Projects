void handleMessages(int *socket_desc);
int receiveMessage(int socket_desc,char *message);
int sendFile(char *file_name, int socket_desc);
int receiveFile(char *file_name, int socket_desc);
void sendPrograms(int socket_desc);
void sendSourceFiles(char *filePath, int longList, int socket_desc);
int compileAndSend(int socket_desc, char *progname);
int executeAndSend(char *executeCmd, int socket_desc);
int checkDir(int socket_desc, int size, char strings[size][50]);
int handleListCommand(int socket_desc, int size, char strings[size][50]);
int sendSystemInfo(int socket_desc);
