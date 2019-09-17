#include "common.h"
#include "message_handler.h"
#include "duplicate.h"
#include "server_setup.h"

void ZombieKill(int sig){
  if(waitpid(-1, &sig, WNOHANG)<0)
  printf("Killed me!\n");
}

int main(int argc , char *argv[])
{
    signal(SIGCHLD, ZombieKill);

    int socket_desc , new_socket;
    struct sockaddr_in server , client;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
        return 1;
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("Bind failed\n");
        return 1;
    }
    printf("Bind done\n");
    while(1){

      //Accept and incoming connection
      printf("Waiting for incoming connections...\n");

      listen_clients(&socket_desc);
      int accepted = accept_client(&new_socket, &socket_desc, &client);

      if(accepted==-1){
        printf("Connection failed\n");
      }
      printf("Connection accepted\n");

      int pid = duplicate();
      if (pid == -1)
      {
          printf("Failed\n");
          continue;
      }
      else if(pid > 0)
      {
          //close(new);
          continue;
      }
      else if(pid == 0)
      {
          printf("%d\n",client.sin_addr.s_addr);
          handleMessages(&new_socket);
          exit(1);
      }


    }

    return 0;
}
