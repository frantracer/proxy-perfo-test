/* The port number is passed as an argument */
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

using namespace std;

void perr (string msg) {
  cout << "[ERROR] " << msg << endl;
  exit(1);
}

void pdebug (string msg) {
  cerr << "[DEBUG] " << msg << endl;
}

int main(int argc, char *argv[])
{

  char msg_buffer[1024];

  // Read arguments
  if (argc < 2) {
    perr("Usage:\n" + string(argv[0]) + " <port> [<buffer_size>] [<echo_enabled>] [<nodelay_enabled>]");
  }

  int port = atoi(argv[1]);
  int buffer_size = 256;
  int echo_enabled = 1;
  int nodelay_enabled = 1;
  if (argc > 2)
    buffer_size = atoi(argv[2]);
  if (argc > 3)
    echo_enabled = atoi(argv[3]);
  if (argc > 4)
    nodelay_enabled = atoi(argv[4]);

  // Open socket
  int server_sockfd =  socket(AF_INET, SOCK_STREAM, 0);
  if (server_sockfd < 0) {
    perr("Socket could not be opened");
  }

  // Set configuration for the socket
  struct sockaddr_in server_addr;
  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;  
  server_addr.sin_addr.s_addr = INADDR_ANY;  
  server_addr.sin_port = htons(port);

  // Bind configuration to the server socket
  if ( bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 ) { 
    perr("Binding failed");
  }

  // Set maximum number of connection to be quequed
  listen(server_sockfd, 1);

  // Wait for a client to connect
  struct sockaddr_in client_addr;
  socklen_t size_client_addr = sizeof(client_addr);
  int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &size_client_addr);
  if (client_sockfd < 0) {
    perr("Accept error");
  }
  sprintf(msg_buffer, "New connection from %s port %d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
  pdebug(msg_buffer);

  // Enable TCP NODELAY whether required
  int result = setsockopt(client_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay_enabled, sizeof(nodelay_enabled));

  // Recieve message from the client
  char* buffer = new char[buffer_size];
  int n;
  while( (n = read(client_sockfd, buffer, buffer_size)) > 0) {

    pdebug("ECHO: " + string(buffer));

    if(echo_enabled) {
      // Send message to the client
      write(client_sockfd, buffer, n);
    }

    // Clean buffer
    bzero(buffer, buffer_size);

  }

  if (n < 0) {
    perr("Cannot read from socket");
  }

  delete buffer;
 
  // Close connection
  close(client_sockfd);
  close(server_sockfd);

  return 0;
}
