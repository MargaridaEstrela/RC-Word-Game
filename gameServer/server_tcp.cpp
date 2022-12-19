#include "../aux_functions.hpp"
#include "../constants.hpp"
#include "data.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

using string = std::string;

// GLOBAL VARIABLES
char* GSPORT = "58034";
bool verbose;
int status;

int fd, new_fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

// FUNCTIONS
void setup_tcp(void) 
{
    int errcode; 
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP SOCKET
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, GSPORT, &hints, &res);

    if (errcode != 0)
        exit(1);

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(fd, 5) < 0) {
        perror("listen failed");
        exit(1);
    }
}

void process(void) 
{
    std::cout << "start process" << std::endl;

    char request[MAX_TCP_READ];
    string response;
    string verb_response;

    while(1) {
        addrlen = sizeof(addr);
        if ((new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen)) == -1) {
            perror("accept failed");
            exit(1);
        }

        n = read(new_fd, request, MAX_TCP_READ);
        if (n == -1) {
            perror("read failed");
            exit(1);
        } 
    }

}


int main(int argc, char* argv[])
{
    GSPORT = argv[2];
    verbose = argv[3];

    setup_tcp();
    process();

    // end_TCP_session();

    return 0;
} 