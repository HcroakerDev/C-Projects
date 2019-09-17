#include <time.h>

void handle_commands(char *address);
int sendMessage(char *message,int socket_desc);
int receiveMessage(char *server_reply, int socket_desc);
int send_file(char *file_name,int socket_desc);
void handlePutCommand(int socket_desc, char *command, int size, char strings[size][50], clock_t start);
void handleGetCommand(int socket_desc, char *command, int size, clock_t start);
void handleRunCommand(int socket_desc, char *command, clock_t start, int size, char strings[size][50]);
void handleListCommand(int socket_desc, char *command, clock_t start);
void generalCommand(int socket_desc, char *command,clock_t start);
void printResponseTime(clock_t start);
void killChild(int exitStatus, int printEnter);
void printHelp();
