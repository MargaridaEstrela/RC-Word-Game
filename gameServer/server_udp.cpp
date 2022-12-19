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
#include <signal.h>

using string = std::string;

// GLOBAL VARIABLES
int fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

struct sigaction oldact;

char* GSPORT = "58034";
char* word;
bool verbose;
int status;

// FUNCTIONS
void setup_udp(void); 
int max_errors(int word_size);
void process(void);
void end_UDP_session(void);
void ctrl_c_handler(int sig);

void setup_udp(void)
{
    int errcode;
    struct sigaction sigCtrlC_action;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP SOCKET
    hints.ai_flags = AI_PASSIVE;

    // addrlen = sizeof(addr);

    errcode = getaddrinfo(NULL, GSPORT, &hints, &res);

    if (errcode != 0)
        exit(1);

    n = bind(fd, (const struct sockaddr*)res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }

    // setup CTRL-C action
    memset(&sigCtrlC_action, 0, sizeof(sigCtrlC_action));
    sigCtrlC_action.sa_handler = &ctrl_c_handler;
    sigemptyset(&sigCtrlC_action.sa_mask);
    sigCtrlC_action.sa_flags = 0;

    sigaction(SIGINT, &sigCtrlC_action, &oldact);

    return;
}

void process(void)
{
    std::cout << "start process" << std::endl;

    char request[MAX_COMMAND_LINE];
    string response;
    string verb_response;

    while (1) {

        addrlen = sizeof(addr);
        n = recvfrom(fd, request, MAX_COMMAND_LINE, 0, (struct sockaddr*)&addr,
            &addrlen);

        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        request[n] = '\0';

        char* arg1 = new char[MAX_COMMAND_LINE];
        char* arg2 = new char[MAX_COMMAND_LINE];
        char* arg3 = new char[MAX_COMMAND_LINE];
        char* arg4 = new char[MAX_COMMAND_LINE];

        sscanf(request, "%s %s %s %s", arg1, arg2, arg3, arg4);
   

        if (!strcmp(arg1, "SNG")) {

            verb_response = "PLID = ";
            verb_response += (string)arg2 + ": ";
            verb_response = "Start new game: ";

            int status = register_user(arg2);
            int word_size = get_word_size(arg2);
            int errors = max_errors(word_size);

            switch (status) {
                case STATUS_OK: 
                    response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                    break;
                case STATUS_NOK:
                    if (check_no_moves(arg2)){
                        response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                    }

                    else{
                        response = "RSG NOK\n";
                    }
                    break;
                case STATUS_ERR:
                    verb_response += "Error (PLID may be invalid)\n";
                    response = "RSG ERR\n";
                    break;
            }

        } else if (!strcmp(arg1, "PLG")) {

            status = check_play_status(arg2,arg3,atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
                case STATUS_OK: {
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    string pos = get_letter_positions(arg2,arg3);
                    response = "RLG OK " + std::to_string(trial) + " " + pos + "\n";
                    break;
                }
                case STATUS_WIN: {
                    response = "RLG WIN " + (string)arg4 + "\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    end_current_game(arg2,"WIN");
                    break;
                }
                case STATUS_DUP: {
                    response = "RLG DUP\n";
                    break;
                }
                case STATUS_NOK:  {
                    response = "RLG NOK\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                    break;
                }
                case STATUS_OVR: {
                    response = "RLG OVR\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                    end_current_game(arg2,"FAIL");
                    break;
                }
                case STATUS_INV: {
                    int last = check_last_played(arg2,arg3,"T");
                    if (last == STATUS_INV){
                        response = "RLG INV " + std::to_string(trial) + "\n";
                    }
                    else if (last == STATUS_OK) {
                        string pos = get_letter_positions(arg2,arg3);
                        response = "RLG OK " + string(arg4) + " " + pos + "\n";
                    }
                    else if (last == STATUS_NOK) {
                        response = "RLG NOK\n";
                    }
                    break;
                }
                case STATUS_ERR: {
                    response = "RLG ERR\n";
                    break;
                }
            }

        } else if (!strcmp(arg1, "PWG")) {

            status = check_play_status(arg2,arg3,atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
            case STATUS_WIN:{
                response = "RWG WIN " + (string)arg4 + "\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "OK");
                end_current_game(arg2,"WIN");
                break;
            }
            case STATUS_NOK:{
                response = "RWG NOK " + (string)arg4 + "\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                break;
            }
            case STATUS_OVR:{
                response = "RWG OVR " + (string)arg4 + "\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                end_current_game(arg2,"FAIL");
                break;
            }
            case STATUS_INV:{
                int last = check_last_played(arg2,arg3,"G");
                if (last == 0){
                    response = "RLG INV " + std::to_string(trial) + "\n";
                }
                else if (last == 1) {
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    end_current_game(arg2,"WIN");
                }
                else if (last == 2) {
                    response = "RWG NOK " + (string)arg4 + "\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                }
                break;
            }
            case STATUS_ERR:
                response = "RWG ERR\n";
                break;
            }

        } else if (!strcmp(arg1, "QUT")) {

            char* user_dir = create_user_dir(arg2);

            if (!user_exists(user_dir)) {
                response = "RQT ERR\n";
            }

            char* user_game_dir = create_user_game_dir(arg2);

            if (check_ongoing_game(user_game_dir)) {
                end_current_game(arg2,"QUIT");
                response = "RQT OK\n";
            } else {
                response = "RQT NOK\n";
            }
        } 
        else {
            response = "ERR\n";
        }
        // SEND RESPONSE
        if (verbose){
            printf("Message received: ");
            int i = ntohs(addr.sin_port);
            char* ip = inet_ntoa(addr.sin_addr);
            printf("Port: %d | IP: %s\n", i, ip);
            std::cout << verb_response;
        }
        n = sendto(fd, response.c_str(), strlen(response.c_str()) * sizeof(char), 0,
            (struct sockaddr*)&addr, addrlen);
        if (n < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }
}

void end_UDP_session(void)
{
    std::cout << "Closing UDP session..." << std::endl;

    freeaddrinfo(res);
    close(fd);

    return;
}

void ctrl_c_handler(int sig)
{
    std::cout << "Caught Ctrl-C..." << std::endl;

    end_UDP_session();
    sigaction(SIGINT, &oldact, NULL);
    kill(0, SIGINT);

    return;
}

int main(int argc, char* argv[])
{
    std::cout << "here" << std::endl;
    word = new char[sizeof(argv[1])];
    word = argv[1];
    GSPORT = argv[2];

    // if (*argv[3] == 1) {
    //     verbose = true;
    // } else {
    //     verbose = false;
    // }


    std::cout << verbose << std::endl;

    setup_udp();

    std::cout << "setup" << std::endl;
    process();

    end_UDP_session();

    return 0;
}
