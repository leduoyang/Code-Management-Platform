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
#include <vector>

using namespace std;

#define LOCALHOST "127.0.0.1"
#define SERVER_A_UDP_PORT 21910
#define BUFFER_SIZE 1024
#define MEMBER_RESPONSE "1"
#define GUEST_RESPONSE "0"
#define EXCEPTION_RESPONSE "-1"

string encrypt_password(const string &password) {
    string encrypted = password;
    for (char &ch: encrypted) {
        if (isalpha(ch)) {
            if (isupper(ch)) {
                ch = 'A' + (ch - 'A' + 3) % 26;
            } else {
                ch = 'a' + (ch - 'a' + 3) % 26;
            }
        } else if (isdigit(ch)) {
            ch = '0' + (ch - '0' + 3) % 10;
        }
    }
    return encrypted;
}

vector<pair<string, string> > read_credentials(const string &filename) {
    // read information of credentials including the usernames and passwords from the file, and store them into a vector
    vector<pair<string, string> > credentials;
    ifstream file(filename);
    string line;
    getline(file, line);
    // read credential of a user line by line from the file
    while (getline(file, line)) {
        istringstream iss(line);
        string username;
        string password;
        if (iss >> username >> password) {
            credentials.emplace_back(username, password);
        }
    }
    return credentials;
}

bool check_credential(const vector<pair<string, string> > &credentials, const string &message) {
    // check that if a username and password in the credentials equals to the message after concatenation
    for (const pair<string, string> &x: credentials) {
        if (x.first + ":" + x.second == message) {
            return true;
        }
    }
    return false;
}

string process_request(const string &username, const string &message) {
    // check if the username is guest or member, check the credential for the member based on the message, and return the response
    string response;
    vector<pair<string, string> > credentials = read_credentials("./members.txt");
    if (username == "guest" && message == "guest:" + encrypt_password("guest")) {
        response = GUEST_RESPONSE;
    } else if (check_credential(credentials, message)) {
        printf("Member %s has been authenticated", username.c_str());
        response = MEMBER_RESPONSE;
    } else {
        printf("The username %s or password ****** is incorrect", username.c_str());
        response = EXCEPTION_RESPONSE;
    }
    return response;
}

int main() {
    printf("Server A is up and running using UDP on port %d.\n", SERVER_A_UDP_PORT);

    // define and setup UDP server
    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "UDP socket creation error\n";
        return -1;
    }
    struct sockaddr_in serverA_addr;
    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_port = htons(SERVER_A_UDP_PORT);
    serverA_addr.sin_addr.s_addr = inet_addr(LOCALHOST);;
    if (::bind(udp_sock, (struct sockaddr *) &serverA_addr, sizeof(serverA_addr)) < 0) {
        cerr << "Bind error\n";
        return -1;
    }

    // define the client socket variable
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    while (true) {
        // read request from the client
        char buffer[BUFFER_SIZE];
        int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
        if (bytes_received < 0) {
            cerr << "Receive error\n";
            continue;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);
        // get the index of the delimiter
        const int index = message.find(":");
        string username = message.substr(0, index);
        printf("Server A has received username %s and password ****.", username.c_str());
        // process the request and check if the credential is valid
        string response = process_request(username, message);
        // send back the response to the client
        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);
    }
    return 0;
}
