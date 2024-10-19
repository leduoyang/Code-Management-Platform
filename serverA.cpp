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
    vector<pair<string, string> > credentials;
    ifstream file(filename);
    string line;
    getline(file, line);
    while (getline(file, line)) {
        istringstream iss(line);
        string username;
        string password;
        if (iss >> username >> password) {
            credentials.push_back(make_pair(username, password));
        }
    }
    return credentials;
}

bool check_credential(const vector<pair<string, string> > &credentials, const string &buffer) {
    for (const pair<string, string> x: credentials) {
        if (x.first + ":" + x.second == buffer) {
            return true;
        }
    }
    return false;
}

int main() {
    printf("Server A is up and running using UDP on port %d.\n", SERVER_A_UDP_PORT);
    vector<pair<string, string> > credentials = read_credentials("./members.txt");

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

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    while (true) {
        char buffer[BUFFER_SIZE];
        int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
        if (bytes_received < 0) {
            cerr << "Receive error\n";
            continue;
        }
        buffer[bytes_received] = '\0';
        string message(buffer);
        int index = message.find(":");
        string username = message.substr(0, index);
        printf("Server A has received username %s and password ****.", username.c_str());

        string response;
        if (username == "guest" && message == "guest:" + encrypt_password("guest")) {
            response = "0";
        } else if (check_credential(credentials, buffer)) {
            printf("Member %s has been authenticated", username.c_str());
            response = "1";
        } else {
            printf("The username %s or password ****** is incorrect", username.c_str());
            response = "-1";
        }
        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);
    }

    return 0;
}
