#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <chrono>

using namespace std;

/* Output messages functions */
void perr(string msg) {
  cout << "[ERROR] " << msg << endl;
  exit(1);
}

void pdebug(string msg) {
  cerr << "[DEBUG] " << msg << endl;
}

/* Statistic generation */
template <class T>
class Statistics {
  public:
  T minimum, maximum, average, std_deviation, total;
  Statistics(list<T>* values) {
    this->minimum = 0;
    this->maximum = 0;
    this->average = 0;
    this->std_deviation = 0;
    this->total = 0;
    if (values->size() > 0) {
      typename list<T>::iterator it = values->begin();
      this->minimum = *it;
      this->maximum = *it;
      while(it != values->end()){
        this->total += *it;
        if(*it < this->minimum)
          this->minimum = *it;
        if(*it > this->maximum)
          this->maximum = *it;
        ++it;
      }
      this->average = this->total / values->size();
      T tmp = 0;
      for (it = values->begin(); it != values->end(); ++it) {
        tmp = *it - this->average;
        this->std_deviation += (tmp * tmp);
      }
      this->std_deviation = sqrt(this->std_deviation / values->size());
    }
  }
};

/***** MAIN *****/
int main(int argc, char** argv) {

  char msg_buffer[1024];

  // Read arguments
  if (argc < 5) {
    perr("Usage:\n" + string(argv[0]) + " <hostname> <port> <file> <repetitions> [<buffer_size>] [<echo_enabled>] [<nodelay_enabled>]");
  }

  char* hostname = argv[1];
  int port = atoi(argv[2]);
  char* filepath = argv[3];
  int n_repetitions = atoi(argv[4]);
  int buffer_size = 256;
  int echo_enabled = 1;
  int nodelay_enabled = 1;
  if (argc > 5)
    buffer_size = atoi(argv[5]);
  if (argc > 6)
    echo_enabled = atoi(argv[6]);
  if (argc > 7)
    nodelay_enabled = atoi(argv[7]);

  // Initialise time measurement variables
  list<double> time_measurements(0);
  double elapsed_time = 0;

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
  int n_bytes_read, n_bytes_sent, n_bytes_received;
 
  // Start loop
  for (int i = 1; i <= n_repetitions; ++i) {

    sprintf(msg_buffer, "Iteration %d", i);
    pdebug(msg_buffer);

    // Open file description
    ifstream file_stream(filepath);

    if(file_stream) {

      while(!file_stream.eof()) {

        chrono::system_clock::time_point begin = chrono::system_clock::now();

        // Read file chunk
        file_stream.read(file_buffer, buffer_size);
        n_bytes_read = file_stream.gcount();

        // Send to the server
        n_bytes_sent = write(server_sockfd, file_buffer, n_bytes_read);
        if (n_bytes_sent < 0) {
          perr("Problem while writing to socket");
        }

        if(echo_enabled) {
          // Receive from the server
          n_bytes_received = read(server_sockfd, socket_buffer, buffer_size);
          if (n_bytes_received < 0) {
            perr("Problem while reading from socket");
          }

          // Check message recieved
          if (n_bytes_sent != n_bytes_received) {
            sprintf(msg_buffer, "Bytes sent (%d) are not the same as received (%d)", n_bytes_sent, n_bytes_received);
            perr(msg_buffer);
          }
        }

        // Clean buffers 
        bzero(file_buffer, buffer_size);
        bzero(socket_buffer, buffer_size);

        // Measure time elapsed
        chrono::system_clock::time_point end = chrono::system_clock::now();
        chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(end - begin);
        elapsed_time = time_span.count();

        time_measurements.push_back(elapsed_time);

      }

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

  Statistics<double>* stats = new Statistics<double>(&time_measurements);
  printf("Min\t\tMax\t\tAverage\t\tStd.Dev\t\tTotal\t\tCount\n");
  printf("%.5e\t%.5e\t%.5e\t%.5e\t%.9f\t%lu\n", stats->minimum, stats->maximum,
    stats->average, stats->std_deviation, stats->total, time_measurements.size());
  delete stats;

  return 0;

}

