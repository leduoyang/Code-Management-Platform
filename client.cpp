#include <iostream>
#include <cstdlib>      // for exit, atoi
#include <cstring>      // for memset, strerror
#include <unistd.h>     // for close, read, write
#include <errno.h>      // for errno
#include <netdb.h>      // for hostent, gethostbyname
#include <sstream>
#include <sys/types.h>  // for socket types
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket, connect, etc.
#include <arpa/inet.h>  // for inet_ntoa, htons
#include <sys/wait.h>   // for waitpid

using namespace std;

#define HOST_NAME "127.0.0.1"
#define MAIN_SERVER_TCP_PORT 25910
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

int connect_to_main_server() {
    int sock;
    struct sockaddr_in server_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error\n";
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection to server failed\n";
        return -1;
    }
    return sock;
}

string send_message(int sock, const string &message) {
    send(sock, message.c_str(), message.size(), 0);
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    // 0 for guest, 1 for member
    int permission = 0;

    // initiate credentials
    const string username = argv[1];
    const string password = argv[2];
    const string encrypted_password = encrypt_password(password);
    const string credentials = username + ":" + encrypted_password;

    int client = connect_to_main_server();
    cout << "The client is up and running." << endl;
    string res = send_message(client, credentials);
    if (res == "0") {
        cout << "You have been granted guest access." << endl;
    } else if (res == "1") {
        cout << "You have been granted member access" << endl;
        permission = 1;
    } else {
        cout << "The credentials are incorrect. Please try again." << endl;
        return 0;
    }
    // check if login success

    string command;
    while (true) {
        getline(cin, command);

        string action;
        string param;
        istringstream iss(command);
        iss >> action;
        iss >> param;
        if (action == "lookup" && param.empty()) {
            if (permission == 1) {
                printf("Username is not specified. Will lookup %s.\n", username.c_str());
                command = action;
                command.append(" ").append(username);
                printf("%s sent a lookup request to the main server.\n", username.c_str());
            } else {
                cout << "username is not specified." << endl;
                cout <<
                        "Please enter the command: <lookup <username>> , <push <filename> > , <remove <filename> > , <deploy> , <log>.\n";
                continue;
            }
        }
        if ((action == "push" || action == "remove")) {
            if (param.empty()) {
                cout << "Error: Filename is required. Please specify a filename to push.\n" << endl;
                cout <<
                        "Please enter the command: <lookup <username>> , <push <filename> > , <remove <filename> > , <deploy> , <log>.\n";
                continue;
            }
            if(action == "remove") {
                printf("%s sent a remove request to the main server.", username.c_str());
            }
            command.append(" ").append(username);
        }

        client = connect_to_main_server();
        string response = send_message(client, command);

        if (action == "lookup") {
            printf("The client received the response from the main server using TCP over port %d\n",
                   MAIN_SERVER_TCP_PORT);
            if (response.empty() || response == "-1") {
                printf("%s does not exist. Please try again.\n", username.c_str());
            } else if (response == "0") {
                printf("Empty repository..\n");
            } else {
                cout << response << endl;
            }
        } else if (action == "push") {
            if (response == "1") {
                printf("%s exists in %s’s repository, do you want to overwrite (Y/N)?\n", param.c_str(),
                       username.c_str());
                string decision;
                getline(cin, decision);
                command = "decision " + decision;
                send_message(client, command);
            } else {
                printf("%s pushed successfully\n", param.c_str());
            }
        } else if (action == "remove") {
            if (response == "1") {
                // todo
            } else {
                printf("The remove request was successful.\n");
            }
        }
    }

    close(client);
    return 0;
}
