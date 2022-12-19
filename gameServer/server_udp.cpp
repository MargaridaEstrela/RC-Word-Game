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

        verb_response = "PLID = ";
        verb_response += (string)arg2 + ": ";
   

        if (!strcmp(arg1, "SNG")) {

            verb_response += "Start new game -> ";

            int status = register_user(arg2);

            switch (status) {
                case STATUS_OK: {
                    char* game_word = get_player_word(arg2);
                    int word_size = strlen(game_word);
                    int errors = max_errors(word_size);
                    response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                    verb_response += "Success; word = \"" + (string)game_word + "\" (";
                    verb_response += std::to_string(word_size) + " letters)\n";
                    break;
                }
                case STATUS_NOK:{
                    int word_size = get_word_size(arg2);
                    int errors = max_errors(word_size);
                    if (check_no_moves(arg2)){
                        response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                        verb_response += "No plays have yet occured in current game; re-sending previous response\n";
                    }

                    else{
                        response = "RSG NOK\n";
                        verb_response += "Fail; a game for this user is still active\n"; 
                    }
                    break;
                }
                case STATUS_ERR:
                    verb_response += "Error; PLID sent may be invalid\n";
                    response = "RSG ERR\n";
                    break;
            }

        } else if (!strcmp(arg1, "PLG")) {

            verb_response += "Play letter -> ";

            status = check_play_status(arg2,arg3,atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
                case STATUS_OK: {
                    verb_response += "Success; \"" + (string)arg3 + "\" is part of the word; word not guessed\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    string pos = get_letter_positions(arg2,arg3);
                    response = "RLG OK " + std::to_string(trial) + " " + pos + "\n";
                    break;
                }
                case STATUS_WIN: {
                    verb_response += "Success; \"" + (string)arg3 + "\" is part of the word; word was guessed (game ended)\n";
                    response = "RLG WIN " + (string)arg4 + "\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    end_current_game(arg2,"WIN");
                    break;
                }
                case STATUS_DUP: {
                    verb_response += "Fail; \"" + (string)arg3 + "\" has been played before; no play is registed\n";
                    response = "RLG DUP\n";
                    break;
                }
                case STATUS_NOK:  {
                    verb_response += "Fail; \"" + (string)arg3 + "\" is not part of the word\n";
                    response = "RLG NOK\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "T %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                    break;
                }
                case STATUS_OVR: {
                    verb_response += "Fail; \"" + (string)arg3 + "\" is not part of the word; max error limit reached (game ended)\n";
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
                        verb_response += "Error; trial number doesn't match; \"" + (string)arg3 + "\" may not be the last played letter \n";
                        response = "RLG INV " + std::to_string(trial) + "\n";
                    }
                    else if (last == STATUS_OK) {
                        verb_response += "Success; lost message re-sent; \"" + (string)arg3 + "\" is part of the word; word not guessed\n";
                        string pos = get_letter_positions(arg2,arg3);
                        response = "RLG OK " + string(arg4) + " " + pos + "\n";
                    }
                    else if (last == STATUS_NOK) {
                        verb_response += "Fail; lost message re-sent; \"" + (string)arg3 + "\" is not part of the word\n";
                        response = "RLG NOK\n";
                    }
                    break;
                }
                case STATUS_ERR: {
                    verb_response += "Error; PLID or letter \"" + (string)arg3 + "\" may be invalid or no game is currently active\n";
                    response = "RLG ERR\n";
                    break;
                }
            }

        } else if (!strcmp(arg1, "PWG")) {

            verb_response += "Guess word -> ";

            status = check_guess_status(arg2,arg3,atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
            case STATUS_WIN:{
                verb_response += "Success; word \"" + (string)arg3 + "\" was the correct guess (game ended)\n";
                response = "RWG WIN " + (string)arg4 + "\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "OK");
                end_current_game(arg2,"WIN");
                break;
            }
            case STATUS_NOK:{
                verb_response += "Fail; word \"" + (string)arg3 + "\" was not the correct guess\n";
                response = "RWG NOK " + (string)arg4 + "\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                break;
            }
            case STATUS_OVR:{
                verb_response += "Fail; word \"" + (string)arg3 + "\" was not the correct guess; max error limit reached (game ended)\n";       
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
                     verb_response += "Error; trial number doesn't match; \"" + (string)arg3 + "\" may not be the last word guessed\n";
                    response = "RLG INV " + std::to_string(trial) + "\n";
                }
                else if (last == 1) {
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    end_current_game(arg2,"WIN");
                }
                else if (last == 2) {
                     verb_response += "Fail; lost message re-sent; \"" + (string)arg3 + "\" is not the hidden word\n";
                    response = "RWG NOK " + (string)arg4 + "\n";
                    char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                }
                break;
            }
            case STATUS_ERR:
                verb_response += "Error; PLID or word \"" + (string)arg3 + "\" may be invalid or no game is currently active\n";
                response = "RWG ERR\n";
                break;
            }

        } else if (!strcmp(arg1, "QUT")) {

            verb_response += "Guess current game -> ";

            char* user_dir = create_user_dir(arg2);

            if (!user_exists(user_dir)) {
                verb_response += "Error; user doesn't yet exist in server database (no game has been played)\n";
                response = "RQT ERR\n";
            }

            char* user_game_dir = create_user_game_dir(arg2);

            if (check_ongoing_game(user_game_dir)) {
                verb_response += "Success; current game has been closed\n";
                end_current_game(arg2,"QUIT");
                response = "RQT OK\n";
            } else {
                verb_response += "Fail; there is no ongoing game currently\n";
                response = "RQT NOK\n";
            }
        } 
        else {
            verb_response += "Error -> Protocol message \"" + (string)arg1 + "\" is not recognized by the server\n";
            response = "ERR\n";
        }
        // SEND RESPONSE
        if (verbose){
            printf("Message received: ");
            int i = ntohs(addr.sin_port);
            char* ip = inet_ntoa(addr.sin_addr);
            printf("Port: %d | IP: %s\n", i, ip);
            std::cout << verb_response;
            std::cout << "\n";
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
    word = new char[sizeof(argv[1])];
    word = argv[1];
    GSPORT = argv[2];
    char* v = argv[3];
    std::cout << word;
    std::cout << GSPORT;
    std::cout << v;

     if (!strcmp(v,"YES")) {
         verbose = true;
     } else {
         verbose = false;
     }

    setup_udp();

    std::cout << "setup" << std::endl;
    process();

    end_UDP_session();

    return 0;
}
