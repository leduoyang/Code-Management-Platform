#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <filesystem>

using namespace std;

#define BUFFER_SIZE 1024
#define DEPLOY_PREFIX "deploy"
#define DEPLOY_SUCCESS_RESPONSE "0"
#define LOCALHOST "127.0.0.1"
#define SERVER_D_UDP_PORT 23910

void update_deploy(const string &line) {
    const string DEPLOYED = "./deployed.txt";
    // build_deploy_if_absent
    if (!std::filesystem::exists(DEPLOYED)) {
        ofstream file(DEPLOYED);
        file.close();
    }
    // update the deploy information to the target file "./deployed.txt"
    ofstream file(DEPLOYED, ios::app);
    file << line << std::endl;
    file.close();
}

string process_request(string &message) {
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
        // read filenames from the request message, acquired from serverM
        while (getline(iss, filename)) {
            // combine the filenames with spaces
            deploy_line.append(filename).append(" ");
        }
        // update ./deployed.txt with the information in the message
        update_deploy(deploy_line);
        printf("Server D has deployed the user %s’s repository.\n", member_name.c_str());
    }
    return DEPLOY_SUCCESS_RESPONSE;
}

int main() {
    // define and setup UDP server
    printf("Server D is up and running using UDP on port %d.\n", SERVER_D_UDP_PORT);
    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_D_UDP_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr(LOCALHOST);;
    if (::bind(udp_sock, (struct sockaddr *) &serverR_addr, sizeof(serverR_addr)) < 0) {
        return -1;
    }

    // define the client socket variable
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    while (true) {
        // read request from the client
        char buffer[BUFFER_SIZE];
        int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
        printf("Server D has received a deploy request from the main server.\n");
        if (bytes_received < 0) {
            continue;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);
        // process the request and deploy the files based on the username provided
        string response = process_request(message);
        // send back the response to the client
        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);
    }
    return 0;
}
