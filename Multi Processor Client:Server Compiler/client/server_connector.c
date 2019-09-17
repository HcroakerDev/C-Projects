#include "common.h"

/*
 * Function: connect_server
 * --------------------
 * Connect to server
 *
 * int socket_desc: socket dec
 *
 * Returns: 1 if success, 0 if fail
 */
int connect_server(int *socket_desc, char *address){
    struct sockaddr_in server;
    //Create socket
    *socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (*socket_desc == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(address);
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    //Connect to remote server
    if (connect(*socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        // printf("Connect error\n");
        return -1;
    }

    //puts("Connected\n");

    // Set timeout for windows and unix to 10s
    #ifdef _WIN32
      DWORD timeout = 10 * 1000;
      setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
    #else
      struct timespec tv;
      tv.tv_sec = 10;
      setsockopt(*socket_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    #endif

    return 1;
}
