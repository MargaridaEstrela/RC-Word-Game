#include "../constants.hpp"

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

using string = std::string;

// GLOBAL VARIABLES
int fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

string word;
int word_size;
int errors;
string PLID;
string GSPORT = "58034";
bool verbose;
int trials;
char last_guess_letter;
string last_guess_word;

// FUNCTIONS
void setup(void);
int max_errors(int word_size);
void process(void);
void end_UDP_session(void);

void setup_udp(void)
{

    int errcode;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP SOCKET
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, GSPORT.c_str(), &hints, &res);

    if (errcode != 0)
        exit(1);

    n = bind(fd, (const struct sockaddr*)res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }

    return;
}

int max_errors(int word_size)
{
    if (word_size <= 6) {
        return 7;
    } else if (word_size > 6 & word_size <= 10) {
        return 8;
    } else {
        return 9;
    }
}

int check_letter(char letter)
{
    int n = 0;

    if (letter == last_guess_letter) {
        return STATUS_DUP;
    }

    trials++;

    for (int i = 0; i < word_size; i++) {
        if (letter == word[i])
            n++;
    }

    if (n == 0 && trials < errors) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n == 1) {
        return STATUS_WIN;
    }

    return STATUS_OK;
}

void process(void)
{

    char request[MAX_COMMAND_LINE];
    string response;

    while (1) {

        addrlen = sizeof(addr);
        n = recvfrom(fd, request, MAX_COMMAND_LINE, 0, (struct sockaddr*)&addr, &addrlen);

        printf("message received\n");

        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        request[n] = '\0';
        if (verbose) {
            // TO DO
        }

        char* arg1 = new char[MAX_COMMAND_LINE];
        char* arg2 = new char[MAX_COMMAND_LINE];
        char* arg3 = new char[MAX_COMMAND_LINE];
        char* arg4 = new char[MAX_COMMAND_LINE];

        sscanf(request, "%s %s %s %s", arg1, arg2, arg3, arg4);

        if (!strcmp(arg1, "SNG")) {

            if (strlen(arg2) != 6) {
                std::cerr << "PLID: bad format. PLID is always sent using 6 digits." << std::endl;
                exit(EXIT_FAILURE);
            }

            PLID = arg2;

            int status = 0; // to be changed

            // status will be the output from register_user
            // we'll have a file just for register, unregister, etc.

            // check if PLID has any ongoing game (whit play moves)

            switch (status) {
            case STATUS_OK:
                response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                break;
            case STATUS_NOK:
                response = "RSG NOK\n";
                // checks if player PLID has any ongoing game (with play moves)
                break;
            }
        } else if (!strcmp(arg1, "PLG")) {

            if (PLID.length() == 0) {
                response = "RLG ERR\n";
            }

            if (trials != atoi(arg4)) {
                response = "RLG INV " + (string)arg4 + "\n";
            }

            last_guess_letter = arg3[0];
            int status = check_letter(last_guess_letter);

            switch (status) {
            case STATUS_OK:
                response = "RLG OK " + std::to_string(last_guess_letter) + " " + std::to_string(n) + "\n";
                break;
            case STATUS_WIN:
                response = "RLG WIN " + (string)arg4 + "\n";

                break;
            case STATUS_DUP:
                response = "RLG DUP\n";
                // checks if player PLID has any ongoing game (with play moves)
                break;
            case STATUS_NOK:
                response = "RLG NOK\n";
                // checks if player PLID has any ongoing game (with play moves)
                break;
            case STATUS_OVR:
                response = "RLG OVR\n";
                // checks if player PLID has any ongoing game (with play moves)
                break;
            }

        } else if (!strcmp(arg1, "PWG")) {
            // no idea what to do with PLID
            // i will check this only just for now

            if (arg2 != PLID) {
                // need to check if there is any ongoing game for this PLID
                response = "RWG ERR\n";
            }

            if (atoi(arg4) != trials || !strcmp(arg3, last_guess_word.c_str())) {
                response = "RWG INV " + std::to_string(trials) + "\n";
            }

            if (!strcmp(arg2, word.c_str())) {
                response = "RWG WIN\n";
            } else {
                if (trials < errors) {
                    response = "RWG NOK\n";
                } else {
                    response = "RWG OVR\n";
                }
            }

        } else if (!strcmp(arg1, "QUT")) {

        } else if (!strcmp(arg1, "REV")) {

        } else {
            std::cerr << "ERROR_UDP: invalid command." << std::endl;
            exit(EXIT_FAILURE);
        }

        // SEND RESPONSE
        n = sendto(fd, response.c_str(), strlen(response.c_str()) * sizeof(char), 0, (struct sockaddr*)&addr, addrlen);
        if (n < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }
}

void end_UDP_session()
{

    freeaddrinfo(res);
    close(fd);

    return;
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "ERROR_UDP: bad input" << std::endl;
    }

    word = argv[1];
    word_size = word.length();
    errors = max_errors(word_size);
    GSPORT = argv[2];
    verbose = argv[3];

    setup_udp();
    process();

    end_UDP_session();

    return 0;
}
