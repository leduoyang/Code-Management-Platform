#include <iostream>
#include <fstream>
#include <sstream>
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
#include <unordered_map>

using namespace std;

#define HOST_NAME "127.0.0.1"
#define SERVER_R_UDP_PORT 22910
#define BUFFER_SIZE 1024

string trim(const string &str) {
    size_t first = str.find_first_not_of(' ');
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

string vector_to_string(vector<string> vector) {
    ostringstream oss;
    for (size_t i = 0; i < vector.size(); i++) {
        oss << vector[i] << endl;
    }
    return oss.str();
}

unordered_map<string, vector<string> > read_files(const string &filename) {
    unordered_map<string, vector<string> > user_file_map;
    ifstream file(filename);
    string line;
    getline(file, line);
    while (getline(file, line)) {
        istringstream iss(line);
        string username;
        string file;
        if (iss >> username >> file) {
            user_file_map[username].push_back(file);
        }
    }
    return user_file_map;
}

bool check_if_file_exist(const vector<string> &file_list, string &target) {
    for (const string &filename: file_list) {
        if (filename == target) {
            return true; // File found
        }
    }
    return false;
}

void update_repository(vector<string> &target_list, const string &filename, const string &username,
                       const string &repository, int action) {
    if (action == 1) {
        // push operation
        target_list.push_back(filename);
        ofstream file(repository, ios::app);
        file << username << " " << filename << std::endl;
        file.close();
    } else if (action == 0) {
        // remove operation
        for (auto it = target_list.begin(); it != target_list.end();) {
            if (*it == filename) {
                it = target_list.erase(it);
            } else {
                it += 1;
            }
        }
        ifstream file(repository);
        vector<string> lines;
        string line;
        if (getline(file, line)) {
            lines.push_back(line);
        }
        while (getline(file, line)) {
            istringstream iss(line);
            string curr_username;
            string curr_filename;
            if (iss >> curr_username >> curr_filename) {
                if (!(curr_filename != username) && !(curr_filename != filename)) {
                    lines.push_back(line);
                }
            }
        }
        file.close();
        ofstream output_file(repository, ios::trunc);
        for (const string &output_line: lines) {
            output_file << output_line << endl;
        }
        output_file.close();
    }
}

int main() {
    string FILENAME;
    const string LOOKUP_PREFIX = "lookup";
    const string PUSH_PREFIX = "push";
    const string REMOVE_PREFIX = "remove";
    const string REPOSITORY = "./filenames.txt";
    printf("Server R is up and running using UDP on port %d.", SERVER_R_UDP_PORT);
    unordered_map<string, vector<string> > user_file_map = read_files(REPOSITORY);

    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_UDP_PORT);
    serverR_addr.sin_addr.s_addr = inet_addr(HOST_NAME);;

    if (::bind(udp_sock, (struct sockaddr *) &serverR_addr, sizeof(serverR_addr)) < 0) {
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
        string response;
        if (message.find(LOOKUP_PREFIX) == 0) {
            printf("Server R has received a lookup request from the main server.\n");
            string username = trim(message.substr(LOOKUP_PREFIX.size()));
            if (!username.empty()) {
                vector<string> &target_files = user_file_map[username];
                if (target_files.empty()) {
                    if (user_file_map.find(username) == user_file_map.end()) {
                        response = "-1";
                    } else {
                        response = "0";
                    }
                } else {
                    response = vector_to_string(target_files); // User exists and has files
                }
            }
        } else {
            string prefix;
            string filename;
            string username;
            istringstream iss(message);
            iss >> prefix;
            iss >> filename;
            iss >> username;
            username = trim(username);
            filename = trim(filename);
            if (prefix == PUSH_PREFIX) {
                printf("Server R has received a push request from the main server.\n");
                vector<string> &target_list = user_file_map[username];
                if (target_list.empty() || !check_if_file_exist(target_list, filename)) {
                    update_repository(target_list, filename, username, REPOSITORY, 1);
                    response = "0";
                } else {
                    printf("%s exists in %s's repository; requesting overwrite confirmation.\n", filename.c_str(),
                           username.c_str());
                    FILENAME = filename;
                    response = "1";
                }
            } else if (prefix == REMOVE_PREFIX) {
                printf("Server R has received a remove request from the main server.\n");
                vector<string> &target_list = user_file_map[username];
                if (!target_list.empty() || check_if_file_exist(target_list, filename)) {
                    update_repository(target_list, filename, username, REPOSITORY, 0);
                    response = "0";
                } else {
                    response = "1";
                }
            }
        }

        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);

        if (message.find(LOOKUP_PREFIX) == 0) {
            printf("Server R has finished sending the response to the main server.\n");
        } else if (message.find(PUSH_PREFIX)) {
            if (response == "1") {
                bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr,
                                          &addr_len);
                buffer[bytes_received] = '\0';
                string command(buffer);
                string prefix;
                string decision;
                istringstream iss(command);
                iss >> prefix;
                iss >> decision;
                if (decision == "Y") {
                    printf("User requested overwrite; overwrite successful.\n");
                } else if (decision == "N") {
                    printf("Overwrite denied\n");
                }
            } else {
                printf("%s uploaded successfully.\n", FILENAME.c_str());
            }
        }
    }

    return 0;
}
