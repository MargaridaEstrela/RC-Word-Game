#include "data.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using string = std::string;

// GLOBAL VARIABLES
string GSPORT = "58034";
string verbose = "NO";
char* word;

int fd, new_fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

struct sigaction oldact;

pid_t udp_pid;
pid_t tcp_pid;

// FUNCTIONS
void decoder(int argc, char* argv[]);
void ctrl_c_handler(int sig);

void decoder(int argc, char* argv[])
{
    if (argc == 2) {
        word = new char[sizeof(argv[1])];
        word = argv[1];
        return;
    }

    if (argc < 2 || argc > 5) {
        std::cerr << "ERROR: invalid application start command. Usage: ./GS word_file [-p GSport] [-v]\n";
        exit(EXIT_FAILURE);
    }

    word = new char[sizeof(argv[1])];
    word = argv[1];

    if (argc >= 3 && argc <= 5) {
        for (int i = 2; i < argc; i += 2) {
            if (string(argv[i]) == "-p") {
                GSPORT = argv[i + 1];
            } else if (string(argv[i]) == "-v") {
                verbose = "YES";
            } else {
                std::cerr << "ERROR: unknown flag: " << argv[i] << ". Usage: ./GS word_file [-p GSport] [-v]\n";
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
}

void sig_handler(int sig)
{
    std::cout << "Caught Ctrl-C..." << std::endl;

    kill(udp_pid, SIGINT);
    kill(tcp_pid, SIGINT);

    sigaction(SIGINT, &oldact, NULL);
    kill(0, SIGINT);
}

int main(int argc, char* argv[])
{
    struct sigaction sig_action;

    decoder(argc, argv);

    mkdir(GAMES_DIR, 0777);
    mkdir(SCORES_DIR, 0777);

    udp_pid = fork();
    tcp_pid = fork();

    if (udp_pid == 0) {
        execl("./server_udp", "server_udp", word, GSPORT.c_str(), verbose.c_str(), NULL);
        std::cerr << "ERROR: cannot execute UDP server\n";
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_FAILURE);
    }

    if (tcp_pid == 0) {
        execl("./server_tcp", "server_tcp", word, GSPORT.c_str(), verbose.c_str(), NULL);
        std::cerr << "ERROR: cannot execute TCP server\n";
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_FAILURE);
    }

    // setup SIGINT action
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = &sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    return 0;
}
