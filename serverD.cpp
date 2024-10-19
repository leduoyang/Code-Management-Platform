#include <iostream>
#include <cstdlib>      // for std::exit, std::atoi
#include <cstring>      // for std::memset, std::strerror
#include <unistd.h>     // for close, read, write
#include <errno.h>      // for errno
#include <netdb.h>      // for hostent, gethostbyname
#include <sys/types.h>  // for socket types
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket, connect, etc.
#include <arpa/inet.h>  // for inet_ntoa, htons
#include <sys/wait.h>   // for waitpid

using namespace std;

#define MAIN_SERVER_UDP_PORT 24910
#define BUFFER_SIZE 1024

int main() {
    int udp_sock;
    struct sockaddr_in server_addr, server1_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server1_addr);

    // Create UDP socket for Server 2
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "UDP socket creation error\n";
        return -1;
    }

    // Set up Server 2 address for UDP
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER2_UDP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind UDP socket
    if (bind(udp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "UDP socket bind error\n";
        return -1;
    }

    std::cout << "Server 2 listening on UDP port " << SERVER2_UDP_PORT << std::endl;

    // Receive message from Server 1
    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server1_addr, &addr_len);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << "Received from Server 1: " << buffer << std::endl;

        // Process the message (for example, append extra information)
        std::string response = std::string(buffer) + " (processed by Server 2)";

        // Send the response back to Server 1 over UDP
        sendto(udp_sock, response.c_str(), response.length(), 0, (struct sockaddr*)&server1_addr, addr_len);
        std::cout << "Response sent back to Server 1\n";
    }

    // Close the UDP socket
    close(udp_sock);
    return 0;
}