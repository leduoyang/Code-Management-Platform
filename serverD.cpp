#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
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
#include <vector>
#include <filesystem>

using namespace std;

#define HOST_NAME "127.0.0.1"
#define SERVER_D_UDP_PORT 23910
#define BUFFER_SIZE 1024

void build_deploy_if_absent(const string &path) {
    if (!std::filesystem::exists(path)) {
        ofstream file(path);
        file.close();
    }
}

void update_deploy(const string &line, const string &deploy) {
    ofstream file(deploy, ios::app);
    file << line << std::endl;
    file.close();
}

int main() {
    string FILENAME;
    const string DEPLOY_PREFIX = "deploy";
    const string DEPLOYED = "./deployed.txt";
    printf("Server D is up and running using UDP on port %d.\n", SERVER_D_UDP_PORT);

    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_D_UDP_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr(HOST_NAME);;
    if (::bind(udp_sock, (struct sockaddr *) &serverR_addr, sizeof(serverR_addr)) < 0) {
        return -1;
    }

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    while (true) {
        char buffer[BUFFER_SIZE];
        int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
        printf("Server D has received a deploy request from the main server.\n");
        if (bytes_received < 0) {
            cerr << "Receive error\n";
            continue;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);
        string response;
        string member_name;
        string permission;
        string prefix;
        string filename;
        istringstream iss(message);
        iss >> member_name;
        iss >> permission;
        iss >> prefix;
        string deploy_line = member_name.append(" ");
        if (prefix == DEPLOY_PREFIX) {
            build_deploy_if_absent(DEPLOYED);
            while (getline(iss, filename)) {
                deploy_line.append(filename).append(" ");
            }
            update_deploy(deploy_line, DEPLOYED);
        }
        printf("Server D has deployed the user %s’s repository.\n", member_name.c_str());
        response = '0';
        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);
    }

    return 0;
}
