int setup_server(int *socket_desc,struct sockaddr_in *server);
int listen_clients(int *socket_desc);
int accept_client(int *new_socket, int *socket_desc,struct sockaddr_in *client);
