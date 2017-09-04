#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <chrono>

using namespace std;

void perr(string msg) {
  cout << "[ERROR] " << msg << endl;
  exit(1);
}

void pdebug(string msg) {
  cerr << "[DEBUG] " << msg << endl;
}

int main(int argc, char** argv) {

  char msg_buffer[1024];

  // Read arguments
  if (argc < 5) {
    perr("Usage:\n" + string(argv[0]) + " <hostname> <port> <file> <repetitions> [<buffer_size>] [<nodelay_enabled>]");
  }

  char* hostname = argv[1];
  int port = atoi(argv[2]);
  char* filepath = argv[3];
  int n_repetitions = atoi(argv[4]);
  int buffer_size = 256;
  int nodelay_enabled = 0;
  if (argc > 5)
    buffer_size = atoi(argv[5]);
  if (argc > 6)
    nodelay_enabled = atoi(argv[6]);

  // Initialise time measurement variables
  double* time_measurements = new double[n_repetitions];
  double total_elapsed_time = 0;
  double max_elapsed_time = -DBL_MAX;
  double min_elapsed_time = DBL_MAX;

  // Create scoket
  int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sockfd < 0) {
    perr("Could not open socket");
  }

  // Set socket configuration
  struct sockaddr_in server_addr;
  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, hostname, &server_addr.sin_addr);

  // Connect to server
  if (connect(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) { 
    perr("Problem while connecting");
  }

  // Enable TCP NODELAY whether required
  int result = setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay_enabled, sizeof(nodelay_enabled));

  char* file_buffer = new char[buffer_size]();
  char* socket_buffer= new char[buffer_size]();
  int n;
 
  // Start loop
  for (int i = 1; i <= n_repetitions; ++i) {

    sprintf(msg_buffer, "Iteration %d", i);
    pdebug(msg_buffer);

    // Open file description
    ifstream file_stream(filepath);

    if(file_stream) {

      chrono::system_clock::time_point begin = chrono::system_clock::now();

      while(!file_stream.eof()) {

        // Read file chunk
        file_stream.read(file_buffer, buffer_size);
        n = file_stream.gcount();

        // Send to the server
        n = write(server_sockfd, file_buffer, n);
        if (n < 0) {
          perr("Problem while writing to socket");
        }

        // Receive from the server
        n = read(server_sockfd, socket_buffer, buffer_size);
        if (n < 0) {
          perr("Problem while reading from socket");
        }

        // Clean buffers 
        bzero(file_buffer, buffer_size);
        bzero(socket_buffer, buffer_size);

      }

      // Measure time elapsed
      chrono::system_clock::time_point end = chrono::system_clock::now();
      chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(end - begin);
      double elapsed_time = time_span.count();
      total_elapsed_time += elapsed_time;
      if(elapsed_time < min_elapsed_time)
        min_elapsed_time = elapsed_time;
      if(elapsed_time > max_elapsed_time)
        max_elapsed_time = elapsed_time;
      time_measurements[i-1] = elapsed_time;

      sprintf(msg_buffer, "Iteration %d elapsed time is %f secs", i, elapsed_time);
      pdebug(msg_buffer);

      // Close file
      file_stream.close();

    } else {
      perr("Cannot open file");
    }

  }


  delete file_buffer;
  delete socket_buffer;

  // Close server socket
  close(server_sockfd);

  cout << "Min\t\tMax\t\tAverage\t\tTotal" << endl;
  cout << min_elapsed_time << "\t" << max_elapsed_time << "\t" << 
    total_elapsed_time / n_repetitions << "\t" << total_elapsed_time << endl;

  delete time_measurements;

  return 0;

}
