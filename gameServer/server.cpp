#include "data.hpp"

#include <arpa/inet.h>
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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// GLOBAL VARIABLES
string GSPORT = "58034";
bool verbose = false;
char* word;

pid_t udp_pid;
pid_t tcp_pid;

// FUNCTIONS
void decoder(int argc, char* argv[]);

void decoder(int argc, char* argv[])
{

    if (argc == 2) {
        word = new char[sizeof(argv[1])];
        word = argv[1];
        return;
    } else if (argc >= 3 & argc <= 5) {
        for (int i = 2; i < argc; i += 2) {
            if (string(argv[i]) == "-p") {
                GSPORT = argv[i + 1];
            } else if (string(argv[i]) == "-v") {
                verbose = true;
            } else {
                cerr << "ERROR: unknown flag: " << argv[i] << ". Usage: ./GS word_file [-p GSport] [-v]\n";
                exit(EXIT_FAILURE);
            }
        }
    } else {
        cerr << "ERROR: invalid application start command. Usage: ./GS word_file [-p GSport] [-v]\n";
        exit(EXIT_FAILURE);
    }
    return;
}

int main(int argc, char* argv[])
{

    decoder(argc, argv);
    mkdir(GAMES_DIR,0777);
    mkdir(SCORES_DIR,0777);

    udp_pid = fork();

    if (udp_pid == 0) {
        execl("./server_udp", "./server_udp", word, GSPORT.c_str(), verbose, NULL);
        cerr << "ERROR: cannot execute UDP server\n";
        exit(EXIT_FAILURE);
    } else if (udp_pid == -1) {
        exit(EXIT_FAILURE);
    }

    // samething for TCP

    return 0;
}
