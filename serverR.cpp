#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <unordered_map>

using namespace std;

#define BUFFER_SIZE 1024
#define CREDENTIALS "./members.txt"
#define DEPLOY_PREFIX "deploy"
#define LOCALHOST "127.0.0.1"
#define LOOKUP_PREFIX "lookup"
#define PUSH_PREFIX "push"
#define REMOVE_PREFIX "remove"
#define REPOSITORY "./filenames.txt"
#define SERVER_R_UDP_PORT 22910

set<string> load_username() {
    // load usernames from the members.txt and save them in a string set
    set<string> username_set;
    ifstream file(CREDENTIALS);
    string line;
    getline(file, line);
    while (getline(file, line)) {
        istringstream iss(line);
        string username;
        string password;
        if (iss >> username >> password) {
            username_set.insert(username);
        }
    }
    return username_set;
}

string trim(const string &str) {
    // remove any leading and trailing spaces
    size_t first = str.find_first_not_of(' ');
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

string vector_to_string(vector<string> vector) {
    // concatenate all elements in a vector to a string
    ostringstream oss;
    for (size_t i = 0; i < vector.size(); i++) {
        oss << vector[i] << endl;
    }
    return oss.str();
}

unordered_map<string, vector<string> > read_files() {
    // read the usernames and filenames from filenames.txt and save the information in a map
    unordered_map<string, vector<string> > user_file_map;
    ifstream file(REPOSITORY);
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
    // check that if a file exists
    for (const string &filename: file_list) {
        if (filename == target) {
            return true; // File found
        }
    }
    return false;
}

void update_repository(vector<string> &target_list, const string &filename, const string &username, int action) {
    if (action == 1) {
        // push operation where filename will be push back to the target_list for the user and the repository will be updated
        target_list.push_back(filename);
        ofstream file(REPOSITORY, ios::app);
        file << username << " " << filename << std::endl;
        file.close();
    } else if (action == 0) {
        // remove operation where the filename in the target_list will be erased
        for (auto it = target_list.begin(); it != target_list.end();) {
            if (*it == filename) {
                it = target_list.erase(it);
            } else {
                it += 1;
            }
        }
        // read the repository again to store the information into vector lines
        ifstream file(REPOSITORY);
        vector<string> lines;
        string line;
        if (getline(file, line)) {
            lines.push_back(line);
        }
        // adding back the data except for one that is being removed
        while (getline(file, line)) {
            istringstream iss(line);
            string curr_username;
            string curr_filename;
            if (iss >> curr_username >> curr_filename) {
                if (curr_username != username && curr_filename != filename) {
                    lines.push_back(line);
                }
            }
        }
        file.close();
        // update the repository based on vector lines, which has removed the target username and filename
        ofstream output_file(REPOSITORY, ios::trunc);
        for (const string &output_line: lines) {
            output_file << output_line << endl;
        }
        output_file.close();
    }
}

int main() {
    string FILENAME;
    printf("Server R is up and running using UDP on port %d.\n", SERVER_R_UDP_PORT);
    unordered_map<string, vector<string> > user_file_map = read_files();

    // define and setup UDP server
    int udp_sock;
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    struct sockaddr_in serverR_addr;
    serverR_addr.sin_family = AF_INET;
    serverR_addr.sin_port = htons(SERVER_R_UDP_PORT);
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
        istringstream iss(message);
        iss >> member_name;
        iss >> permission;
        iss >> prefix;
        if (prefix == LOOKUP_PREFIX || prefix == DEPLOY_PREFIX) {
            string username;
            iss >> username;
            if(prefix == LOOKUP_PREFIX) {
                printf("Server R has received a lookup request from the main server.\n");
            } else {
                printf("Server R has received a deploy request from the main server.\n");
            }
            if (!username.empty()) {
                set<string> username_set = load_username();
                if (username_set.find(username) == username_set.end()) {
                    response = "-1";
                } else {
                    vector<string> &target_files = user_file_map[username];
                    if (target_files.empty()) {
                        response = "0";
                    } else {
                        response = vector_to_string(target_files); // User exists and has files
                    }
                }
            }
        } else {
            string filename;
            string username;
            iss >> filename;
            iss >> username;
            username = trim(username);
            filename = trim(filename);
            if (prefix == PUSH_PREFIX) {
                printf("Server R has received a push request from the main server.\n");
                vector<string> &target_list = user_file_map[username];
                if (target_list.empty() || !check_if_file_exist(target_list, filename)) {
                    update_repository(target_list, filename, username, 1);
                    response = "0";
                } else {
                    printf("%s exists in %s's repository; requesting overwrite confirmation.\n", filename.c_str(),
                           username.c_str());
                    response = "1";
                }
                FILENAME = filename;
            } else if (prefix == REMOVE_PREFIX) {
                printf("Server R has received a remove request from the main server.\n");
                vector<string> &target_list = user_file_map[username];
                if (!target_list.empty() || check_if_file_exist(target_list, filename)) {
                    update_repository(target_list, filename, username, 0);
                    response = "0";
                } else {
                    response = "1";
                }
            }
        }

        // send back the response to the client
        sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);

        // print the logs based on the action and response
        if (prefix == LOOKUP_PREFIX) {
            printf("Server R has finished sending the response to the main server.\n");
        }
        else if (prefix == DEPLOY_PREFIX) {
            printf("Server R has finished sending the response to the main server.\n");
        }else if (prefix == PUSH_PREFIX) {
            if (response == "1") {
                // handle the decision made by the user
                bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr,
                                          &addr_len);
                buffer[bytes_received] = '\0';
                string command(buffer);
                string decision;
                istringstream decision_iss(command);
                decision_iss >> member_name;
                decision_iss >> permission;
                decision_iss >> prefix;
                decision_iss >> decision;
                if (decision == "Y") {
                    printf("User requested overwrite; overwrite successful.\n");
                } else if (decision == "N") {
                    printf("Overwrite denied\n");
                }
                response = "0";
                sendto(udp_sock, response.c_str(), response.size(), 0, (struct sockaddr *) &client_addr, addr_len);
            } else {
                printf("%s uploaded successfully.\n", FILENAME.c_str());
            }
        }
    }

    return 0;
}
