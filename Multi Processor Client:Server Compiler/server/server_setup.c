#include "common.h"

// Listen for clients
int listen_clients(int *socket_desc){
  listen(*socket_desc , 10);
  return 1;
}

// Accept clients.
int accept_client(int *new_socket, int *socket_desc, struct sockaddr_in *client){

  int c = sizeof(struct sockaddr_in);
  *new_socket = accept(*socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
  if (*new_socket<0)
  {
      perror("accept failed");
      return -1;
  }
  return 1;
}
