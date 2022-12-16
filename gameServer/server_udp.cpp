#include "../aux_functions.hpp"
#include "../constants.hpp"
#include "data.hpp"

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
int fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

char* word;
int word_size;
int errors;
char* PLID;
char* GSPORT = "58034";
bool verbose;
int trials = 0;
int status;
char* last_guess_letter;
char* last_guess_word;

// FUNCTIONS
void setup(void); // Aqui não é setup_udp?
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

    errcode = getaddrinfo(NULL, GSPORT, &hints, &res);

    if (errcode != 0)
        exit(1);

    n = bind(fd, (const struct sockaddr*)res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }
    return;
}

int check_letter(char* letter)
{
    int n = 0;
    char* last_guess_letter = get_last_guess_letter(PLID);

    if (!strcmp(letter, last_guess_letter)) {
        return STATUS_DUP;
    }

    for (int i = 0; i < word_size; i++) {
        if (*letter == word[i])
            n++;
    }

    if (n == 0 & trials < errors) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n == 1) {
        return STATUS_WIN;
    }
    return STATUS_OK;
}

int check_word(char* guess_word)
{
    char* last_guess_word = get_last_guess_word(PLID);

    if (!strcmp(guess_word, last_guess_word)) {
        return STATUS_WIN;
    } else if (get_trials(PLID) < errors) {
        return STATUS_NOK;
    } else {
        return STATUS_OVR;
    }
}

void process(void)
{
    std::cout << "start process" << std::endl;

    char request[MAX_COMMAND_LINE];
    string response;

    while (1) {

        addrlen = sizeof(addr);
        n = recvfrom(fd, request, MAX_COMMAND_LINE, 0, (struct sockaddr*)&addr,
            &addrlen);

        printf("message received\n");
        int i = ntohs(addr.sin_port);
        char* ip = inet_ntoa(addr.sin_addr);
        printf("Port: %d | IP: %s\n", i, ip);

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

            PLID = arg2;

            std::cout << "start command" << std::endl;

            int status = register_user(arg2);

            switch (status) {
            case STATUS_OK:
                response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                break;
            case STATUS_NOK:
                trials = get_trials(PLID);
                response = "RSG NOK\n";
                break;
            }
        } else if (!strcmp(arg1, "PLG")) {

            if (!check_PLID(PLID)) {
                response = "RLG ERR\n";
            }

            if (trials != atoi(arg4) || trials != get_trials(PLID)) {
                response = "RLG INV " + (string)arg4 + "\n";
            }

            status = check_letter(arg3);

            switch (status) {
            case STATUS_OK:
                response = "RLG OK " + (string)arg3 + " " + std::to_string(n) + "\n";
                break;
            case STATUS_WIN:
                response = "RLG WIN " + (string)arg4 + "\n";

                break;
            case STATUS_DUP:
                response = "RLG DUP\n";
                break;
            case STATUS_NOK:
                response = "RLG NOK\n";
                break;
            case STATUS_OVR:
                response = "RLG OVR\n";
                break;
            }

            char *trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
            sprintf(trial_line, "T %s\n", arg3);
            add_trial(PLID, trial_line);

        } else if (!strcmp(arg1, "PWG")) {

            PLID = arg2;

            char* user_dir = create_user_dir(PLID);
            char* user_game_dir = create_user_game_dir(user_dir, PLID);

            if (arg2 != PLID || check_ongoing_game(user_game_dir)) {
                response = "RWG ERR\n";
            }

            trials = get_trials(PLID);
            last_guess_word = get_last_guess_word(PLID);

            if (atoi(arg4) != trials || !strcmp(arg3, last_guess_word)) {
                response = "RWG INV " + std::to_string(trials) + "\n";
            }

            status = check_word(arg3);

            switch (status) {
            case STATUS_WIN:
                response = "RWG WIN " + std::to_string(trials) + "\n";
                break;
            case STATUS_NOK:
                response = "RWG WIN " + std::to_string(trials) + "\n";
                break;
            case STATUS_OVR:
                response = "RWG OVR " + std::to_string(trials) + "\n";
                break;
            }

            char *trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
            sprintf(trial_line, "T %s\n", arg3);
            add_trial(PLID, trial_line);

        } else if (!strcmp(arg1, "QUT")) {

            PLID = arg2;
            char* user_dir = create_user_dir(PLID);

            if (!user_exists(user_dir)) {
                response = "RQT ERR\n";
            }

            char* user_game_dir = create_user_game_dir(user_dir, PLID);

            if (check_ongoing_game(user_game_dir)) {
                response = "RQT OK\n";
            } else {
                response = "RQT NOK\n";
            }
        } else if (!strcmp(arg1, "REV")) {

            PLID = arg2;

            char* user_dir = create_user_dir(PLID);
            char* user_game_dir = create_user_game_dir(user_dir, PLID);

            if (check_ongoing_game(user_game_dir)) {
                response = "RRV " + (string)word + "\n";
            }
        } else {
            std::cerr << "ERROR_UDP: invalid command." << std::endl;
            exit(EXIT_FAILURE);
        }

        // SEND RESPONSE
        std::cout << response << std::endl;
        n = sendto(fd, response.c_str(), strlen(response.c_str()) * sizeof(char), 0,
            (struct sockaddr*)&addr, addrlen);
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

    word = new char[sizeof(argv[1])];
    PLID = new char[PLID_SIZE];

    word = argv[1];
    word_size = sizeof(argv[1]);

    errors = max_errors(word_size);
    GSPORT = argv[2];
    verbose = argv[3];

    setup_udp();
    process();

    end_UDP_session();

    return 0;
}
