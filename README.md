# Code Management Platform
This is the socket programming project for EE450 Computer Networks

## Introduction
This project implements five main components: `client.cpp`, `serverM.cpp`, `serverA.cpp`, `serverR.cpp`, and `serverD.cpp`. Communication between the client and `serverM` is established using TCP, while communication between the servers is handled via UDP. The implementation leverages the `<sys/socket.h>` library and related libraries for socket programming. Additionally, `serverM` employs `pthreads` to manage multiple simultaneous TCP requests from different users. An extra credit log operation is also implemented.

Clients are categorized as either members or guests based on the credentials they provide upon executing `./client` with their username and password as parameters. Members are granted permission "1" and can perform the following actions:
- **lookup**
- **push**
- **remove**
- **deploy**

Guests, on the other hand, have permission "0" and are limited to executing only the **lookup** command.

## Code Files
- **`Makefile`**: Builds the executables for `.cpp` files using the `g++` compiler with the C++17 standard.
- **`Dockerfile`**: Creates a testing environment based on Ubuntu 20.04.
- **`client.cpp`**: Uses a dynamic TCP port to send requests to and receive responses from the main server (`serverM`).
- **`serverM.cpp`**: Interacts with the client, forwards requests to other backend servers via UDP, and returns responses to the client through TCP.
- **`serverA.cpp`**: Handles user authentication and returns the corresponding permission to the client. Permission "1" is for members, and "0" is for guests.
- **`serverR.cpp`**: Processes the **lookup**, **deploy**, **push**, and **remove** actions, and sends responses back to `serverM`.
- **`serverD.cpp`**: Generates the `deployed.txt` file and appends deployment information for users requesting the **deploy** action.

## Message Format
### Authentication Messages
`<username>:<password>`

### Action Messages (space-separated)
`<current username> <permission> <action> {<username>, <filename>}`

#### Examples:
- **lookup**: `CharlieMartinez274 1 lookup GraceSmith845`
- **push**: `CharlieMartinez274 1 push EE450ProjectFile_618`
- **remove**: `CharlieMartinez274 1 remove EE450ProjectFile_618`
- **deploy**: `CharlieMartinez274 1 deploy`

