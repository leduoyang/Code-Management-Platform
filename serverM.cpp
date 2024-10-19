#include <iostream>
#include <cstdlib>      // for exit, atoi
#include <cstring>      // for memset, strerror
#include <unistd.h>     // for close, read, write
#include <errno.h>      // for errno
#include <netdb.h>      // for hostent, gethostbyname
#include <sys/types.h>  // for socket types
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket, connect, etc.
#include <arpa/inet.h>  // for inet_ntoa, htons
#include <sys/wait.h>   // for waitpid
#include <pthread.h>

using namespace std;

#define HOST_NAME "127.0.0.1"
#define SERVER_A_UDP_PORT 21910
#define SERVER_R_UDP_PORT 22910
#define SERVER_D_UDP_PORT 23910
#define MAIN_SERVER_UDP_PORT 24910
#define MAIN_SERVER_TCP_PORT 25910
#define BUFFER_SIZE 1024

struct ThreadData {
    int client_sock;
    sockaddr_in serverA_addr;
    sockaddr_in serverD_addr;
    sockaddr_in serverR_addr;
    int udp_sock_a;
    int udp_sock_d;
    int udp_sock_r;
};

int init_tcp_server(sockaddr *server_addr) {
    int tcp_sock;
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    if (::bind(tcp_sock, server_addr, sizeof(struct sockaddr_in)) < 0) {
        return -1;
    }
    return tcp_sock;
}

int init_udp_socket() {
    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    return udp_sock;
}

string udp_send_request(int udp_sock, sockaddr_in *server_addr, string message) {
    sendto(udp_sock, message.c_str(), message.size(), 0, (struct sockaddr *) server_addr, sizeof(*server_addr));
    char buffer[BUFFER_SIZE];
    int bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    }
    return buffer;
}

void *handle_client(void *arg) {
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
    cout << "Received from Client: " << message << endl;
    string response;
    if (message.find("lookup ") == 0 || message.find("push ") == 0 || message.find("decision ") == 0) {
        cout << message << endl;
        response = udp_send_request(data->udp_sock_r, &data->serverR_addr, message);
    } else if (message == "deploy") {
        response = udp_send_request(data->udp_sock_d, &data->serverD_addr, message);
    } else {
        int index = message.find(":");
        printf("Server M has received username %s and password ****.\n", message.substr(0, index).c_str());
        printf("Server M has sent authentication request to Server A\n");
        response = udp_send_request(data->udp_sock_a, &data->serverA_addr, message);
        printf("The main server has received the response from server A using UDP over %d\n", MAIN_SERVER_UDP_PORT);
        printf("The main server has sent the response from server A to client using TCP over port %d.\n", MAIN_SERVER_TCP_PORT);
    }
    send(client_sock, response.c_str(), response.size(), 0);
    close(client_sock);
    delete data;
    return nullptr;
}


int main() {
    printf("Server M is up and running using UDP on port %d.\n", MAIN_SERVER_UDP_PORT);

    // initiate UDP parameters
    struct sockaddr_in serverA_addr;
    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_port = htons(SERVER_A_UDP_PORT);
    serverA_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    struct sockaddr_in serverD_addr;
    serverD_addr.sin_family = AF_INET;
    serverD_addr.sin_port = htons(SERVER_D_UDP_PORT);
    serverD_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_UDP_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    const int udp_sock_a = init_udp_socket();
    const int udp_sock_d = init_udp_socket();
    const int udp_sock_r = init_udp_socket();


    // setting up socket for the tcp server
    struct sockaddr_in serverM_addr;
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_port = htons(MAIN_SERVER_TCP_PORT);
    serverM_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    int tcp_sock = init_tcp_server((sockaddr *) &serverM_addr);
    if (listen(tcp_sock, 5) < 0) {
        cerr << "Listen error\n";
        return -1;
    }
    while (true) {
        struct sockaddr_in tcp_client_addr;
        socklen_t addr_len = sizeof(tcp_client_addr);
        int client_socket = accept(tcp_sock, (struct sockaddr *) &tcp_client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "client accept error";
            continue;
        }

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
