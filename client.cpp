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
    string permission = "0";

    // initiate credentials
    const string username = argv[1];
    const string password = argv[2];
    const string encrypted_password = encrypt_password(password);
    const string credentials = username + ":" + encrypted_password;

    int client = connect_to_main_server();
    printf("The client is up and running.");
    string res = send_message(client, credentials);
    if (res == "0") {
        cout << "You have been granted guest access." << endl;
    } else if (res == "1") {
        cout << "You have been granted member access" << endl;
        permission = "1";
    } else {
        cout << "The credentials are incorrect. Please try again." << endl;
        return 0;
    }
    string meta_data = username + " " + permission;

    string command;
    while (true) {
        if (permission == "1") {
            printf(
                "Please enter the command:\n<lookup <username>>\n<push <filename>>\n<remove <filename>>\n<deploy><log>\n");
        } else {
            printf("Please enter the command: <lookup <username>>\n");
        }
        getline(cin, command);

        string action;
        string param;
        istringstream iss(command);
        iss >> action;
        iss >> param;
        if (permission == "1") {
            if (action == "lookup") {
                if(param.empty()) {
                    param = username;
                    printf("Username is not specified. Will lookup %s.\n", param.c_str());
                    command = action;
                    command.append(" ").append(param);
                    printf("%s sent a lookup request to the main server.\n", username.c_str());
                }
            }
            if ((action == "push" || action == "remove")) {
                if (param.empty()) {
                    printf("Error: Filename is required. Please specify a filename to push.\n");
                    continue;
                }
                if (action == "remove") {
                    printf("%s sent a remove request to the main server.", username.c_str());
                }
                command.append(" ").append(username);
            }
        } else {
            if (action == "lookup") {
                if (param.empty()) {
                    printf("Error: Username is required. Please specify a username to lookup.\n");
                    continue;
                }
            } else {
                printf("Guests can only use the lookup command\n");
                continue;
            }
            printf("Guest sent a lookup request to the main server.");
        }

        client = connect_to_main_server();
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getsockname(client, (struct sockaddr *) &addr, &addr_len) == -1) {
            return -1;
        }
        const int CLIENT_TCP_PORT = ntohs(addr.sin_port);
        command = meta_data + " " + command;
        string response = send_message(client, command);

        if (permission == "1") {
            if (action == "lookup") {
                printf("The client received the response from the main server using TCP over port %d\n",
                       CLIENT_TCP_PORT);
                if (response.empty() || response == "-1") {
                    printf("%s does not exist. Please try again.\n", param.c_str());
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
                    command = meta_data + " " + command;
                    client = connect_to_main_server();
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
        } else {
            if (response.empty() || response == "-1") {
                printf(
                    "The client received the response from the main server using TCP over port %d. %s does not exist. Please try again.\n",
                    CLIENT_TCP_PORT, username.c_str());
            } else if (response == "0") {
                printf(
                    "The client received the response from the main server using TCP over port %d. Empty repository. \n",
                    CLIENT_TCP_PORT);
            } else {
                printf("The client received the response from the main server using TCP over port %d\n",
                       CLIENT_TCP_PORT);
                cout << response << endl;
            }
        }
    }

    close(client);
    return 0;
}
