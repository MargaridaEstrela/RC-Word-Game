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

char* GSPORT = "58034";
bool verbose;
int status;

// FUNCTIONS
void setup_udp(void); 
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

int get_word_size(char* PLID){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    string line;
    game.open(game_user_dir);
    if (game.is_open()){
        std::getline(game,line);
        std::stringstream ss(line);
        ss >> line;
        return line.length();
    }
    return -1;

}

int check_no_moves(char* PLID){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    string line;
    int n = 0;
    game.open(game_user_dir);
    if (game.is_open()){
        while(std::getline(game,line)){
            n++;
        }
        if (n == 1){
            return 1;
        }
        else {
            return 0;
        }
    }
    return -1;  

}

/*int check_dup(char* PLID,char* play){
    char* game_user_dir = create_user_game_dir(PLID);
    string code, letter;
    std::ifstream game;
    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;

        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T") {
                stream_line >> letter;
                if (play == letter){
                    return 1; 
                }
            }
        }
        return 0;
    }
    return -1;
}

int check_letter(char* PLID, char* letter)
{
    int n = 0;
    int size = get_word_size(PLID);
    char* word = get_player_word(PLID);
    for (int i = 0; i < size; i++) {
        if (letter[0] == word[i])
            n++;
    }

    int trials = get_trials(PLID);
    int errors = max_errors(size);
    if (n == 0 & trials < errors) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n > 0) {
        if (check_completion(PLID)){
            return STATUS_WIN;
        }
        else {
            return STATUS_OK;
        }
    }

}


int check_status(char* PLID, char* letter, int trials){
    if(arg4 == get_trials(PLID)){
        if (check_dup(PLID,letter)){
            return STATUS_DUP;
        }
        else {
            return check_letter(PLID,letter);
        }

    }


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
}*/

void process(void)
{
    std::cout << "start process" << std::endl;

    char request[MAX_COMMAND_LINE];
    string response;

    while (1) {

        addrlen = sizeof(addr);
        n = recvfrom(fd, request, MAX_COMMAND_LINE, 0, (struct sockaddr*)&addr,
            &addrlen);

        printf("Message received: ");
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

            std::cout << "start command" << std::endl;

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
                response = "RSG ERR\n";
                break;
            }
        } /*else if (!strcmp(arg1, "PLG")) {
            
            char* game_user_dir = create_user_game_dir(arg2);
            if (!check_PLID(arg2) || !check_ongoing_game(game_user_dir)) {
                response = "RLG ERR\n";
            }

            /*if (check_inv(PLID,arg3,arg4)) {
                response = "RLG INV " + (string)arg4 + "\n";
            }*/

           /* status = check_status(arg2,arg3,atoi(arg4));

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
            response = "RLG NOK\n";
            char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
            sprintf(trial_line, "T %s\n", arg3);
            add_trial(PLID, trial_line);
            std::cout << "PLAY COmPLETADO\n";

        } /*else if (!strcmp(arg1, "PWG")) {

            char* user_game_dir = create_user_game_dir(arg2);
            char* PLID;
            if (arg2 != PLID || check_ongoing_game(user_game_dir)) {
                response = "RWG ERR\n";
            }

            int trials = get_trials(PLID);
            char* last_guess_word = get_last_guess_word(PLID);

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

            char* PLID = arg2;
            char* user_dir = create_user_dir(PLID);

            if (!user_exists(user_dir)) {
                response = "RQT ERR\n";
            }

            char* user_game_dir = create_user_game_dir(PLID);

            if (check_ongoing_game(user_game_dir)) {
                response = "RQT OK\n";
            } else {
                response = "RQT NOK\n";
            }
        } else if (!strcmp(arg1, "REV")) {

            char* PLID = arg2;

            char* user_dir = create_user_dir(PLID);
            char* user_game_dir = create_user_game_dir(PLID);

            if (check_ongoing_game(user_game_dir)) {
                response = "RRV " + (string)word + "\n";
            }
        } else {
            std::cerr << "ERROR_UDP: invalid command." << std::endl;
            exit(EXIT_FAILURE);
        }*/

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
        
    GSPORT = argv[2];
    verbose = argv[3];

    setup_udp();
    process();

    end_UDP_session();

    return 0;
}
