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
    perr("Usage:\n" + string(argv[0]) + " <hostname> <port> <file> <repetitions>");
  }

  int port = atoi(argv[2]);
  int n_repetitions = atoi(argv[4]);
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
  inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

  // Connect to server
  if (connect(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) { 
    perr("Problem while connecting");
  }

  char file_buffer[256];
  char socket_buffer[256];
  int n;
 
  // Start loop
  for (int i = 1; i <= n_repetitions; ++i) {

    sprintf(msg_buffer, "Iteration %d", i);
    pdebug(msg_buffer);

    // Open file description
    ifstream file_stream(argv[3]);

    if(file_stream) {

      chrono::system_clock::time_point begin = chrono::system_clock::now();

      while(!file_stream.eof()) {

        // Read file chunk
        file_stream.read(file_buffer, sizeof(file_buffer));
        n = file_stream.gcount();

        // Send to the server
        n = write(server_sockfd, file_buffer, n);
        if (n < 0) {
          perr("Problem while writing to socket");
        }

        // Receive from the server
        n = read(server_sockfd, socket_buffer, sizeof(socket_buffer));
        if (n < 0) {
          perr("Problem while reading from socket");
        }

        // Clean buffers 
        bzero(file_buffer, sizeof(file_buffer));
        bzero(socket_buffer, sizeof(socket_buffer));

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

  // Close server socket
  close(server_sockfd);

  cout << "Min\t\tMax\t\tAverage\t\tTotal" << endl;
  cout << min_elapsed_time << "\t" << max_elapsed_time << "\t" << 
    total_elapsed_time / n_repetitions << "\t" << total_elapsed_time << endl;

  delete time_measurements;

  return 0;

}
