#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sstream>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

#define BUFFER_SIZE 1024
#define DECISION_PREFIX "decision"
#define DEPLOY_PREFIX "deploy"
#define LOCALHOST "127.0.0.1"
#define LOOKUP_PREFIX "lookup"
#define MAIN_SERVER_TCP_PORT 25910
#define PUSH_PREFIX "push"
#define REMOVE_PREFIX "remove"

string encrypt_password(const string &password) {
    // encrypt the password based on the criteria in the project description
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

/* connect to the TCP server and send TCP request */
int connect_to_main_server() {
    int sock;
    struct sockaddr_in server_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error\n";
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection to server failed\n";
        return -1;
    }
    return sock;
}

string send_message(int sock, const string &message) {
    // sned message to the TCP server
    send(sock, message.c_str(), message.size(), 0);
    char buffer[BUFFER_SIZE];
    // receive message from the server and return
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    return buffer;
}

/* process the request based on the specified action */
string authenticate(string &credentials) {
    string permission = "0";
    int client = connect_to_main_server();
    printf("The client is up and running.");
    string res = send_message(client, credentials);
    // check the response to see if the authentication succeeded
    if (res == "0") {
        cout << "You have been granted guest access." << endl;
    } else if (res == "1") {
        cout << "You have been granted member access" << endl;
        permission = "1";
    } else {
        cout << "The credentials are incorrect. Please try again." << endl;
        return "-1";
    }
    close(client);
    return permission;
}

int main(int argc, char *argv[]) {
    // initiate credentials
    string username = argv[1];
    string password = argv[2];
    string encrypted_password = encrypt_password(password);
    string credentials = username + ":" + encrypted_password;
    // initiate a TCP client and send the credentials for authentication
    string permission = authenticate(credentials);
    if(permission == "-1") {
        return 0;
    }
    // set up the meta_data, which will be combined to the requests
    string meta_data = username + " " + permission;

    string command;
    while (true) {
        // show different prompt based on the permission of the current user (member or guest)
        if (permission == "1") {
            printf(
                "Please enter the command:\n<lookup <username>>\n<push <filename>>\n<remove <filename>>\n<deploy>\n<log>\n");
        } else {
            printf("Please enter the command: <lookup <username>>\n");
        }
        getline(cin, command);

        string action;
        string param;
        istringstream iss(command);
        iss >> action;
        iss >> param;
        // check the permission first, and send the request based on the action indicated
        if (permission == "1") {
            if (action == LOOKUP_PREFIX) {
                if (param.empty()) {
                    param = username;
                    printf("Username is not specified. Will lookup %s.\n", param.c_str());
                    command = action;
                    command.append(" ").append(param);
                }
                printf("%s sent a lookup request to the main server.\n", username.c_str());
            }
            if (action == DEPLOY_PREFIX) {
                printf("%s sent a lookup request to the main server.\n", username.c_str());
            }
            if ((action == PUSH_PREFIX || action == REMOVE_PREFIX)) {
                if (param.empty()) {
                    printf("Error: Filename is required. Please specify a filename to push.\n");
                    continue;
                }
                if (action == REMOVE_PREFIX) {
                    printf("%s sent a remove request to the main server.", username.c_str());
                }
                command.append(" ").append(username);
            }
        } else {
            if (action == LOOKUP_PREFIX) {
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

        // initiate a TCP client and send the command to the server
        int client = connect_to_main_server();
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getsockname(client, (struct sockaddr *) &addr, &addr_len) == -1) {
            return -1;
        }
        const int CLIENT_TCP_PORT = ntohs(addr.sin_port);
        // combine the meta_data and command, send the request to the main server, and retrieve the response
        command = meta_data + " " + command;
        string response = send_message(client, command);

        // print the message based on the action indicated and the response retrieved
        // process the response for members
        if (permission == "1") {
            if (action == LOOKUP_PREFIX) {
                printf("The client received the response from the main server using TCP over port %d\n",
                       CLIENT_TCP_PORT);
                if (response.empty() || response == "-1") {
                    printf("%s does not exist. Please try again.\n", param.c_str());
                } else if (response == "0") {
                    printf("Empty repository..\n");
                } else {
                    cout << response << endl;
                }
            } else if (action == PUSH_PREFIX) {
                if (response == "1") {
                    printf("%s exists in %s’s repository, do you want to overwrite (Y/N)?\n", param.c_str(),
                           username.c_str());
                    string decision;
                    getline(cin, decision);
                    command = "decision " + decision;
                    command = meta_data + " " + command;
                    client = connect_to_main_server();
                    send_message(client, command);
                } else if (response == "0") {
                    printf("%s pushed successfully\n", param.c_str());
                }
                else {
                    printf("%s was not pushed successfully.\n", param.c_str());
                }
            } else if (action == REMOVE_PREFIX) {
                if (response == "1") {
                    // todo
                } else {
                    printf("The remove request was successful.\n");
                }
            } else if (action == DEPLOY_PREFIX) {
                if (response == "1") {
                    // todo
                } else {
                    printf(
                        "The client received the response from the main server using TCP over port %d. The following files in his/her repository have been deployed.\n",
                        CLIENT_TCP_PORT);
                    cout << response << endl;
                }
            }
        }
        // process the response for guests
        else {
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
    return 0;
}
