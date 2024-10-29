#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sstream>

using namespace std;

#define BUFFER_SIZE 1024
#define DECISION_PREFIX "decision"
#define DEPLOY_PREFIX "deploy"
#define LOCALHOST "127.0.0.1"
#define LOOKUP_PREFIX "lookup"
#define MAIN_SERVER_UDP_PORT 24910
#define MAIN_SERVER_TCP_PORT 25910
#define PUSH_PREFIX "push"
#define REMOVE_PREFIX "remove"
#define SERVER_A_UDP_PORT 21910
#define SERVER_R_UDP_PORT 22910
#define SERVER_D_UDP_PORT 23910

// the data structure for each client request, which will be handled by threads
struct ThreadData {
    int client_sock;
    sockaddr_in serverA_addr;
    sockaddr_in serverD_addr;
    sockaddr_in serverR_addr;
    int udp_sock_a;
    int udp_sock_d;
    int udp_sock_r;
};

/* initiate and handle tcp and udp request */
int init_udp_socket() {
    // create a UDP socket
    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    return udp_sock;
}

int init_tcp_server(sockaddr *server_addr) {
    // create a tcp server based on the info indicated in server_addr
    int tcp_sock;
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    if (::bind(tcp_sock, server_addr, sizeof(struct sockaddr_in)) < 0) {
        return -1;
    }
    return tcp_sock;
}

string udp_send_request(int udp_sock, sockaddr_in *server_addr, string message) {
    // send request to a UDP server with the parameter of message
    sendto(udp_sock, message.c_str(), message.size(), 0, (struct sockaddr *) server_addr, sizeof(*server_addr));
    char buffer[BUFFER_SIZE];
    // receive the response from the udp server
    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    return buffer;
}

/* handle request based on the action */
string authenticate(string &message) {
    int index = message.find(":");
    printf("Server M has received username %s and password ****.\n", message.substr(0, index).c_str());
    printf("Server M has sent authentication request to Server A\n");
    string response = udp_send_request(data->udp_sock_a, &data->serverA_addr, message);
    printf("The main server has received the response from server A using UDP over %d\n", MAIN_SERVER_UDP_PORT);
    printf("The main server has sent the response from server A to client using TCP over port %d.\n",
           MAIN_SERVER_TCP_PORT);
    return response;
}

string decision(string &member_name, string &message) {
    printf("The main server has received the overwrite confirmation response from %s using TCP over port %d\n",
           member_name.c_str(),
           MAIN_SERVER_TCP_PORT);
    string response = udp_send_request(data->udp_sock_r, &data->serverR_addr, message);
    printf("The main server has sent the overwrite confirmation response to server R.");
    return response;
}

string deploy(string &member_name, string &username, string &permission, string &message) {
    printf("The main server has received a remove request from member %s TCP over port %d.\n", username.c_str(),
           MAIN_SERVER_TCP_PORT);
    printf("The main server has sent the lookup request to server R.\n");
    string deploy_message = member_name.append(" ").append(permission).append(" ").append("deploy").append(" ").
            append(member_name);
    string response = udp_send_request(data->udp_sock_r, &data->serverR_addr, deploy_message);
    printf("The main server received the lookup response from server R.");
    if (response.empty() || response == "0") {
        response = "1";
    } else {
        printf("The main server has sent the deploy request to server D.\n");
        udp_send_request(data->udp_sock_d, &data->serverD_addr, message + " " + response);
        printf("The user %s’s repository has been deployed at server D.\n", member_name.c_str());
    }
    return response;
}

string lookup(string &member_name, string &username, string &permission, string &message) {
    if (permission == "1") {
        printf(
            "The main server has received a lookup request from %s to lookup %s’s repository using TCP over port %d\n",
            member_name.c_str(),
            username.c_str(),
            MAIN_SERVER_TCP_PORT);
    } else {
        printf(
            "The main server has received a lookup request from Guest to lookup %s’s repository using TCP over port %d.\n",
            username.c_str(),
            MAIN_SERVER_TCP_PORT);
    }
    printf("The main server has sent the lookup request to server R.\n");
    string response = udp_send_request(data->udp_sock_r, &data->serverR_addr, message);
    printf("The main server has received the response from server R using UDP over %d\n", MAIN_SERVER_UDP_PORT);
    printf("The main server has sent the response to the client.\n");
    return response;
}

string push(string &username, string &message) {
    printf("The main server has received a push request from %s TCP over port %d\n.", username.c_str(),
           MAIN_SERVER_TCP_PORT);
    printf("The main server has sent the push request to server R.");
    string response = udp_send_request(data->udp_sock_r, &data->serverR_addr, message);
    printf("The main server has received the response from server R using UDP over %d\n", MAIN_SERVER_UDP_PORT);
    if (response == "1") {
        printf(
            "The main server has received the response from server R using UDP over %d, asking for overwrite confirmation\n",
            MAIN_SERVER_UDP_PORT);
        printf("The main server has sent the overwrite confirmation request to the client.\n");
    } else {
        printf("The main server has sent the response to the client.");
    }
    return response;
}

string remove(string &username, string &message) {
    printf("The main server has received a remove request from member %s TCP over port %d.\n", username.c_str(),
           MAIN_SERVER_TCP_PORT);
    string response = udp_send_request(data->udp_sock_r, &data->serverR_addr, message);
    printf("The main server has received confirmation of the remove request done by the server R.\n");
    return response;
}

/* TCP child thread handling client requests */
void *handle_client(void *arg) {
    // handle the request from tcp clients
    ThreadData *data = (ThreadData *) arg;
    int client_sock = data->client_sock;
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        close(client_sock);
        delete data;
        return nullptr;
    }
    buffer[bytes_received] = '\0';
    string message = buffer;
    // parse the message from the client, store the information to corresponding variables using istringstream
    string response;
    string member_name;
    string permission;
    string prefix;
    string filename;
    string username;
    istringstream iss(message);
    iss >> member_name;
    iss >> permission;
    iss >> prefix;
    // forward request to other UDP servers based on the action provided in the message
    if (prefix == LOOKUP_PREFIX) {
        iss >> username;
        response = lookup(member_name, username, permission, message);
    } else if (prefix == DECISION_PREFIX) {
        response = decision(member_name, message);
    } else if (prefix == PUSH_PREFIX) {
        iss >> filename;
        iss >> username;
        response = push(username, message);
    } else if (prefix == REMOVE_PREFIX) {
        iss >> filename;
        iss >> username;
        response = remove(username, message);
    } else if (prefix == DEPLOY_PREFIX) {
        response = deploy(member_name, username, permission, message);
    } else {
        response = authenticate(message);
    }// sending back the response to the client
    send(client_sock, response.c_str(), response.size(), 0);
    // close client socket after the request has been fulfilled
    close(client_sock);
    delete data;
    return nullptr;
}


int main() {
    printf("Server M is up and running using UDP on port %d.\n", MAIN_SERVER_UDP_PORT);

    // initiate UDP parameters and setup UDP client
    struct sockaddr_in serverA_addr;
    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_port = htons(SERVER_A_UDP_PORT);
    serverA_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    struct sockaddr_in serverD_addr;
    serverD_addr.sin_family = AF_INET;
    serverD_addr.sin_port = htons(SERVER_D_UDP_PORT);
    serverD_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_UDP_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    const int udp_sock_a = init_udp_socket();
    const int udp_sock_d = init_udp_socket();
    const int udp_sock_r = init_udp_socket();

    // setting up socket for the tcp server
    struct sockaddr_in serverM_addr;
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    serverM_addr.sin_addr.s_addr = inet_addr(LOCALHOST);
    int tcp_sock = init_tcp_server((sockaddr *) &serverM_addr);
    if (listen(tcp_sock, 5) < 0) {
        cerr << "Listen error\n";
        return -1;
    }
    while (true) {
        // define the client socket variable
        struct sockaddr_in tcp_client_addr;
        socklen_t addr_len = sizeof(tcp_client_addr);
        int client_socket = accept(tcp_sock, (struct sockaddr *) &tcp_client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "client accept error";
            continue;
        }
        // create a thread data object and will be used by a child thread running the handle_client function
        ThreadData *data = new ThreadData();
        data->client_sock = client_socket;
        data->serverA_addr = serverA_addr;
        data->serverD_addr = serverD_addr;
        data->serverR_addr = serverR_addr;
        data->udp_sock_a = udp_sock_a;
        data->udp_sock_d = udp_sock_d;
        data->udp_sock_r = udp_sock_r;
        pthread_t client_thread;
        pthread_create(&client_thread, nullptr, handle_client, data);
        pthread_detach(client_thread);
    }

    close(tcp_sock);
    close(udp_sock_a);
    close(udp_sock_d);
    close(udp_sock_r);
    return 0;
}
