#include "../aux_functions.hpp"
#include "../constants.hpp"
#include "data.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
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
#include <vector>

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
    hints.ai_family = AF_INET; // IPv4
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

int check_score_file()
{
    DIR* dp;
    int i = 0;
    struct dirent* ep;
    dp = opendir(SCORES_DIR);

    if (dp != NULL) {
        while (ep = readdir(dp)) {
            i++;
        }

        closedir(dp);
        if (i <= 2) {
            return STATUS_EMPTY;
        } else {
            return STATUS_OK;
        }
    }

    return -1;
}

string read_score_file(string filename)
{
    std::ifstream file(filename);
    string score;
    std::getline(file, score);
    return score;
}

string get_scores()
{
    std::vector<string> result;
    string scoreboard = "";
    struct dirent** filelist;
    int n_entries, i_file;
    char f_name[50];
    n_entries = scandir("SCORES/", &filelist, 0, alphasort);
    int counter = 1;

    while (n_entries--) {
        if (filelist[n_entries]->d_name[0] != '.') {
            if (counter == 11) {
                break;
            }

            sprintf(f_name, "SCORES/%s", filelist[n_entries]->d_name);
            string score_file = read_score_file((string)f_name);
            std::stringstream ss(score_file);
            string word;
            int size;
            if (counter < 10) {
                scoreboard += " ";
            }

            scoreboard += std::to_string(counter);
            counter++;
            scoreboard += " - ";
            ss >> word;
            scoreboard += word;

            for (int k = 0; k < (5 - word.length()); k++) {
                scoreboard += " ";
            }

            ss >> word;
            scoreboard += word + "  ";
            ss >> word;
            size = 39 - word.length();
            scoreboard += word;

            for (int j = 0; j < size; j++) {
                scoreboard += " ";
            }

            ss >> word;
            scoreboard += word + "              ";

            if (word.length() == 1) {
                scoreboard += " ";
            }

            ss >> word;
            scoreboard += word + "\n";
        }
    }
    return scoreboard;
}

string create_scoreboard()
{
    string scoreboard = "";

    for (int i = 0; i < 32; i++) {
        scoreboard += "-";
    }

    scoreboard += " TOP 10 SCORES ";

    for (int i = 0; i < 32; i++) {
        scoreboard += "-";
    }

    scoreboard += "\n";
    scoreboard += "\n";
    scoreboard += "   SCORE  PLAYER   WORD                              GOOD TRIALS   TOTAL TRIALS\n";
    scoreboard += "\n";

    string aux = get_scores();
    scoreboard += aux;
    scoreboard += "\n";
    scoreboard += "\n";
    return scoreboard;
}

int check_image(char* PLID)
{
    char* game_user_dir = create_user_game_dir(PLID);
    if (!check_PLID(PLID) || !check_ongoing_game(game_user_dir)) {
        return STATUS_NOK;
    }

    string line, hint_file, path;

    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> path;
    ss >> hint_file;

    path = "HINTS/" + hint_file;

    std::ifstream file;
    file.open(path);

    if (file.is_open()) {
        file.close();
        return STATUS_OK;
    } else {
        return STATUS_NOK;
    }
    return 1;
}

string get_hint_filename(char* PLID)
{

    string filename, line;
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> line;
    ss >> filename;
    return filename;
}



int get_hint_file(char* PLID)
{

    string line, path;
    string hint_file = "";
    char* game_user_dir = create_user_game_dir(PLID);
    char c;

    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> line;
    ss >> hint_file;

    path = "HINTS/" + hint_file;
    string teste = "teste_" + hint_file;
    hint_file = "";
    FILE* file;
    file = fopen(path.c_str(),"r");
    fseek(file, 0, SEEK_END); 
    int size = ftell(file); 
    fseek(file, 0, SEEK_SET);
    int n = 1;
    string filename = get_hint_filename(PLID);
    string response = "RHL OK " + filename + " " + std::to_string(size) + " ";
    write(new_fd,response.c_str(),response.length());
    int f_size = size;
    while(size >= 0){
        char *buffer = new char[1024];
        if (size == 0){
            buffer[0] = '\n';
            write(new_fd,buffer,1);
            break;
        }
        n = fread(buffer, sizeof(char), 1024, file);
        write(new_fd,buffer,n);
        size = size - n;
        delete[] buffer;
    }

    return f_size;
}



int check_current_state(char* PLID){
    char* user_dir = create_user_dir(PLID);
    if (!check_PLID(PLID) || !user_exists(user_dir)) {
        return STATUS_NOK;
    }

    else {
        char* game_user_dir = create_user_game_dir(PLID);
        if (check_ongoing_game(game_user_dir)){
            return STATUS_ACT;
        }
        else {
            return STATUS_FIN;
        }
    }

}


string find_last_game(char* PLID){
    struct dirent **filelist;
    int n_entries,found;
    char dirname[20];
    char dir[40];

    sprintf(dirname,"GAMES/%s/",PLID);
    n_entries = scandir(dirname,&filelist,0,alphasort);
    found = 0;

    while (n_entries--){
        if (filelist[n_entries]->d_name[0]!='.'){
            sprintf(dir,"GAMES/%s/%s/",PLID,filelist[n_entries]->d_name);
            found = 1;
        }
        free(filelist[n_entries]);
        if(found){
            break;
        }
    }
    free(filelist);
    return (string)dir;

}

string get_last_state(char* PLID){
    string state,line,word;
    string aux = "";
    int count = 0;
    state = "Last finalized game for player " + (string)PLID;
    state += "\n";
    string last_path = find_last_game(PLID);
    last_path.pop_back();
    std::ifstream game(last_path);
    std::getline(game,line);
    std::stringstream ss(line);
    ss >> word;
    state += "     Word: " + word + "; Hint file: ";
    ss >> word;
    state += word;
    state += "\n";
    while (game) {
        count++;
        std::getline(game, line);
        if (line == ""){
            break;
        }
        std::stringstream stream(line);
        stream >> word;
        if (word == "T"){
            aux += "     Letter trial: ";
            stream >> word;
            aux += word + " - ";
            stream >> word;
            if (word == "OK"){
                aux += "TRUE\n";
            }
            else{
                aux += "FALSE\n";
            }
        }
        else if (word == "G"){
            aux += "     Word guess: ";
            stream >> word;
            aux += word;
            aux += "\n";
        }
        else if (word == "WIN" || word == "FAIL" || word == "QUIT"){
            if (count == 1){
                state += "     Game started - no transactions found\n";
                state += "     Termination: QUIT\n";
                break;
            }
            aux += "     Termination: " + word;
            aux += "\n";
        }
    }
    if (count != 1){
        state += "     --- Transactions found: " + std::to_string(count-2) + " ---\n";
    }

    return state + aux;

}

string get_active_state(char* PLID){
    string state,line,word,positions;
    string aux = "";
    int count = 0;
    int size = get_word_size(PLID);
    int n_pos,pos;
    char* hidden = new char[size];
    char letter;
    for (int i = 0; i < size; i++){
        hidden[i] = '-';
    }
    state = "Active game found for player " + (string)PLID;
    state += "\n";
    char* game_user_dir = create_user_game_dir(PLID);

    std::ifstream game(game_user_dir);
    std::getline(game,line);
    while (game) {
        count++;
        std::getline(game, line);
        if (line == ""){
            break;
        }
        std::stringstream stream(line);
        stream >> word;
        if (word == "T"){
            aux += "     Letter trial: ";
            stream >> letter;
            aux += letter;
            aux += " - ";
            stream >> word;
            if (word == "OK"){
                aux += "TRUE\n";
                positions = get_letter_positions(PLID,&letter);
                std::stringstream pos_aux(positions);
                pos_aux >> n_pos;
                for (int i = 0; i < n_pos; i++){
                    pos_aux >> pos;
                    hidden[pos-1] = letter;
                }
            }
            else{
                aux += "FALSE\n";
            }
        }
        else if (word == "G"){
            aux += "     Word guess: ";
            stream >> word;
            aux += word;
            aux += "\n";
        }
    }
    if (count == 1){
        state += "     Game started - no transactions found\n";
    }
    else{
        state += "     --- Transactions found: " + std::to_string(count-1) + " ---\n";
    }
    state += aux;
    state += "     Solved so far: ";
    for (int i = 0; i < size; i++){
        state += hidden[i];
    }
    delete[] hidden;
    state += "\n";
    return state;
}



void process(void)
{
    std::cout << "Start TCP Process" << std::endl;

    char request[MAX_TCP_READ];
    string response;
    string verb_response;
    pid_t child_pid;

    while (1) {
        addrlen = sizeof(addr);
        if ((new_fd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
            perror("accept failed");
            exit(1);
        }

        pid_t tcp_pid = fork();
        if (tcp_pid == 0){
            n = recv(new_fd, request, MAX_TCP_READ,0);
            if (n == -1) {
                perror("read failed");
                exit(1);
            }

            request[n] = '\0';

            char* arg1 = new char[MAX_COMMAND_LINE];
            char* arg2 = new char[MAX_COMMAND_LINE];
            char* arg3 = new char[MAX_COMMAND_LINE];
            char* arg4 = new char[MAX_COMMAND_LINE];

            sscanf(request, "%s %s", arg1, arg2);


            if (!strcmp(arg1, "GSB")) {
                verb_response = "PLID not given: ";
                verb_response += "Get scoreboard -> ";
                int status = check_score_file();
                switch (status) {
                case STATUS_EMPTY:
                    response = "RSB EMPTY\n";
                    verb_response += "Fail; scoreboard is currently empty (no games have been won yet)\n";
                    break;

                case STATUS_OK: {
                    string score_file = create_scoreboard();
                    pid_t pid = getpid();
                    string filename = "TOPSCORES_" + std::to_string(pid) + ".txt";
                    string size = std::to_string(score_file.length());
                    response = "RSB OK " + filename + " " + size + " " + score_file + "\n";
                    verb_response += "Success; scoreboard will be sent under filename " + filename;
                    verb_response += " (" + size + " bytes)\n";
                    break;
                }
                }

            } else if (!strcmp(arg1,"GHL")) {

                verb_response = "PLID = ";
                verb_response += (string)arg2 + ": ";
                verb_response += "Get hint file -> ";

                int status = check_image(arg2);
                switch (status) {
                    case STATUS_OK: {
                        int size = get_hint_file(arg2);
                        string filename = get_hint_filename(arg2);
                        verb_response += "Success; hint file will be sent under filename " + filename;
                        verb_response += " (" + std::to_string(size) + " bytes)\n";
                        break;
                    }
                    case STATUS_NOK:
                        response = "RHL NOK\n";
                        verb_response += "Error; PLID may be invalid or no game must be currently ongoing\n";
                        break;
                    
                }

            } else if (!strcmp(arg1, "STA")) {

                verb_response = "PLID = ";
                verb_response += (string)arg2 + ": ";
                verb_response += "Get game state file -> ";

                int status = check_current_state(arg2);
                switch(status) {
                    case STATUS_NOK:
                        response = "RST NOK\n";
                        verb_response += "Error; PLID may be invalid or no games are registered to it yet\n"; 
                        break;
                    case STATUS_ACT: {
                        string active_game = get_active_state(arg2);
                        string fname = "STATE_" + (string)arg2;
                        string f_size = std::to_string(active_game.length());
                        fname += ".txt";
                        response = "RST ACT " + fname + " " + f_size + " " + active_game + "\n";
                        verb_response += "Success; current game state file will be sent under filename " + fname;
                        verb_response += " (" + f_size + " bytes)\n";
                        break;
                    }
                    case STATUS_FIN: {
                        string last_game = get_last_state(arg2);
                        string fname = "STATE_" + (string)arg2;
                        string f_size = std::to_string(last_game.length());
                        fname += ".txt";
                        response = "RST FIN " + fname + " " + f_size + " " + last_game + "\n";
                        verb_response += "Success; last finished game state file will be sent under filename " + fname;
                        verb_response += " (" + f_size + " bytes)\n";
                        break;
                    }
                }
            } else {
                response = "ERR\n";
                verb_response += "Error -> Protocol message \"" + (string)arg1 + "\" is not recognized by the server\n";
            }

            // SEND RESPONSE
            
            if (verbose) {
                printf("Message received: ");
                int i = ntohs(addr.sin_port);
                char* ip = inet_ntoa(addr.sin_addr);
                printf("Port: %d | IP: %s\n", i, ip);
                std::cout << verb_response;
                std::cout << "\n";
            }
            if (strcmp(arg1,"GHL")){
                int k = 0;
                int message_size = response.length();
                while (message_size>0){
                    string r = response.substr(k,128);
                    char* message = new char[r.length()];
                    strcpy(message,r.c_str());
                    n = write(new_fd,message,r.length());
                    k += n;
                    if (n == -1) {
                        close(new_fd);
                    }
                    message_size = message_size - n;
                    delete[] message;
                }
            }

            close(new_fd);
        }
        else {
            close(new_fd);
            continue;
        }
    }
}

void end_TCP_session(void)
{
    std::cout << "Closing TCP session..." << std::endl;    

    freeaddrinfo(res);
    close(fd);
}

int main(int argc, char* argv[])
{
    GSPORT = argv[2];
    char* v = argv[3];

    if (!strcmp(v, "YES")) {
        verbose = true;
    } else {
        verbose = false;
    }

    setup_tcp();
    process();

    end_TCP_session();

    return 0;
}
