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
        game.close();
        return line.length();
    }
    game.close();
    return -1;
}

char* get_player_word(char* PLID){
    string line;
    char* word = new char[MAX_WORD_SIZE];
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game(game_user_dir);
    std::getline(game,line);
    std::stringstream ss(line);
    ss >> word;
    return word;
}

int get_errors(char* PLID){
    string code,state;
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    game.open(game_user_dir);
    int errors = 0;

    if (game.is_open()) {
        string line;
        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T" || code == "G") {
                stream_line >> code;
                stream_line >> state;
                if (state == "NOK"){
                    errors++;
                }
            }
        }
        return errors;
    }
    return -1;
}

string get_letter_positions(char* PLID,char* letter){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    string line;
    char* word = new char[MAX_WORD_SIZE];
    game.open(game_user_dir);
    if (game.is_open()){
        std::getline(game,line);
        std::stringstream ss(line);
        ss >> word;
        game.close();
        int size = strlen(word);
        int counter = 0;
        line = "";
        for (int i = 0; i < size; i++){
            if (word[i] == letter[0]){
                counter++;
                line += std::to_string(i+1);
                line += " ";
            }
        }
        game.close();
        return std::to_string(counter) + " " + line;
    }
    game.close();
    return "ERR";

}



int check_no_moves(char* PLID){
    int n = get_trials(PLID);
    if (n == 1){
        return 1;
    }
    else {
        return 0;
    }

}

int check_dup(char* PLID,char* play){
    char* game_user_dir = create_user_game_dir(PLID);
    string code, letter;
    std::ifstream game;
    game.open(game_user_dir);

    if (game.is_open()) {
        string line;

        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T") {
                stream_line >> letter;
                if (!strcmp(play,letter.c_str())){
                    game.close();
                    return 1; 
                }
            }
        }
        game.close();
        return 0;
    }
    return -1;
}

int check_completion(char* PLID,char* play){
    string code,state;
    char letter;
    char* game_user_dir = create_user_game_dir(PLID);
    char* play_word = get_player_word(PLID);
    int size = strlen(play_word);
    std::ifstream game;
    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;
        for (int i=0; i < size; i++){
            if (play_word[i] == play[0]){
                count++;
            }
        }

        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T") {
                stream_line >> letter;
                stream_line >> state;
                if (state == "NOK"){
                    continue;
                }
                for (int i = 0; i < size; i++){
                    if (play_word[i] == letter){
                        count++;
                    }
                }
            }
        }
        game.close();
        if (count == size){
            return 1;
        }
        else {
            return 0;
        }
    }
    return -1;
}


int check_letter(char* PLID, char* letter)
{
    int n = 0;
    char* word = get_player_word(PLID);
    int size = strlen(word);
    for (int i = 0; i < size; i++) {
        if (letter[0] == word[i]){
            n++;
        }
    }

    int max_err = max_errors(size);
    int errors = get_errors(PLID);
    if (n == 0 & errors < max_err) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n > 0) {
        if (check_completion(PLID,letter)){
            return STATUS_WIN;
        }
        else {
            return STATUS_OK;
        }
    }

}

int check_word(char* PLID, char* guess){
    int n;
    char* word = get_player_word(PLID);
    int size = strlen(word);
    if (!strcmp(word,guess)){
        n = 1;
    }
    else {
        n = 0;
    }

    int max_err = max_errors(size);
    int errors = get_errors(PLID);
    if (n == 0 & errors < max_err) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n > 0) {
        return STATUS_WIN;
    }
}

int check_last_played(char* PLID,char* guess,char* code){
    char* game_user_dir = create_user_game_dir(PLID);
    string word;
    char* last_w = new char[MAX_WORD_SIZE];
    char check = 'T';
    std::ifstream game;

    game.open(game_user_dir);

    if (game.is_open()) {
        string line;

        while (check == 'T' || check == 'G') {
            std::getline(game,line);
            std::stringstream ss(line);
            ss >> check;
            ss >> last_w;
            ss >> word;
        }
        game.close();
        if (!strcmp(code,"T")){
            if (last_w[0] == guess[0]){
                if (word == "OK"){
                    return STATUS_OK;
                }
                else {
                    return STATUS_NOK;
                }
            }
            else {
                return STATUS_INV;
            }
        }
        else{
            if (!strcmp(guess,last_w)){
                if (word == "OK"){
                    return STATUS_OK;
                }
                else {
                    return STATUS_NOK;
                }
            }
            else {
                return STATUS_INV;
            }
        }
    }
    return -1;
}




int check_play_status(char* PLID, char* letter, int trials){
    char* game_user_dir = create_user_game_dir(PLID);

    int size = strlen(letter);
    if (size > 1){
        return STATUS_ERR;
    }
    for (int i = 0; i < size; i++){
        if (isalpha(letter[i]) == 0){
            return STATUS_ERR;
        }
        letter[i] = tolower(letter[i]);
    }

    if (!check_PLID(PLID) || !check_ongoing_game(game_user_dir)) {
        return STATUS_ERR;
    }
    else if(trials == get_trials(PLID)){
        if (check_dup(PLID,letter)){
            return STATUS_DUP;
        }
        else {
            return check_letter(PLID,letter);
        }
    }
    else{
        return STATUS_INV;
    }

}

int check_guess_status(char* PLID,char* guess, int trials){
    char* user_game_dir = create_user_game_dir(PLID);
    int size = strlen(guess);
    if (size < 3 || size > 30){
        return STATUS_ERR;
    }
    for (int i = 0; i < size; i++){
        if (isalpha(guess[i]) == 0){
            return STATUS_ERR;
        }
        guess[i] = tolower(guess[i]);
    }

    if (!check_PLID(PLID) || !check_ongoing_game(user_game_dir)) {
        return STATUS_ERR;
    }
    else if(trials == get_trials(PLID)){
        return check_word(PLID,guess);
    }
    else{
        return STATUS_INV;
    }
}

char* get_score_filename(char* PLID, int score){
    time_t rawtime;
    tm* timeinfo;
    char* buffer = new char[80];
    char* filename = new char[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%d%m%Y_%H%M%S",timeinfo);
    sprintf(filename,"SCORES/%d_%s_",score,PLID);
    strcat(filename,buffer);
    strcat(filename,".txt");
    
    return filename;

}




int create_score_file(char* PLID){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    game.open(game_user_dir);
    if (game.is_open()){
    char* filename;
    char* word; 
    string line,code,state;
    int hits = 0;
    int total = 0;
    float score;

    while (std::getline(game,line)) {
        std::stringstream stream_line(line);
        stream_line >> code;
        if (code == "T" || code == "G") {
            total++;
            stream_line >> code;
            stream_line >> state;
            if (state == "OK"){
                hits++;
            }
        }
    }

    game.close();
    score = (hits*100/total);
    filename = get_score_filename(PLID,(int)score);
    word = get_player_word(PLID);
    std::ofstream score_file (filename);
    score_file << (int)score;
    score_file << " ";
    score_file << PLID;
    score_file << " ";
    score_file << word;
    score_file << " ";
    score_file << hits;
    score_file << " ";
    score_file << total;
    score_file << "\n";
    score_file.close();
    return 0;
    }
    return -1;
}


char* get_new_name(char* code){
    time_t rawtime;
    tm* timeinfo;
    char* buffer = new char[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"/%Y%m%d_%H%M%S",timeinfo);
    strcat(buffer,"_");
    strcat(buffer,code);
    strcat(buffer,".txt");
    
    return buffer;
}



void end_current_game(char* PLID,char* code){

    if (!strcmp(code,"WIN")){
        int i = create_score_file(PLID);
    }

    char* game_user_dir = create_user_game_dir(PLID);
    char* user_dir = create_user_dir(PLID);
    char* command = new char[100];

    std::ofstream game(game_user_dir,std::ios::app);
    game << (string)code;
    game << "\n";
    game.close();

    strcpy(command,"mv ");
    strcat(command,game_user_dir);
    strcat(command," ");
    strcat(command,user_dir);

    char* new_file = get_new_name(code);
    strcat(command,new_file);
    system(command);

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
            verb_response += (string)arg2; + ": ";
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
            case STATUS_DUP:
                response = "RLG DUP\n";
                break;
            case STATUS_NOK: {
                response = "RLG NOK\n";
                char *trial_line = (char*)calloc(strlen(arg3)+3, sizeof(char));
                sprintf(trial_line, "T %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                break;
            }
            case STATUS_OVR:{
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
            case STATUS_ERR:
                response = "RLG ERR\n";
            }

        } else if (!strcmp(arg1, "PWG")) {

            status = check_guess_status(arg2,arg3,atoi(arg4));
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

void end_UDP_session()
{

    freeaddrinfo(res);
    close(fd);

    return;
}

int main(int argc, char* argv[])
{
    GSPORT = argv[2];

    // if (*argv[3] == 1) {
    //     verbose = true;
    // } else {
    //     verbose = false;
    // }

    std::cout << "here" << std::endl;

    std::cout << verbose << std::endl;

    setup_udp();

    std::cout << "setup" << std::endl;
    process();

    end_UDP_session();

    return 0;
}
