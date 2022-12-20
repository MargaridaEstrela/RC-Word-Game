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
        if (i == 0) {
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
    if (!check_PLID(PLID) || !check_ongoing_game(PLID)) {
        return STATUS_NOK;
    }

    string line, hint_file, path;
    char* game_user_dir = create_user_game_dir(PLID);

    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> path;
    ss >> hint_file;

    path = "HINTS/" + hint_file;
    std::cout << path;

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

string get_hint_file(char* PLID)
{

    string line, hint_file, path;
    char* game_user_dir = create_user_game_dir(PLID);

    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> line;
    ss >> hint_file;

    path = "HINTS/" + hint_file;

    hint_file = "";
    std::ifstream file(path);

    while (std::getline(file, line)) {
        hint_file += line;
    }
    return hint_file;
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

void process(void)
{
    std::cout << "start process" << std::endl;

    char request[MAX_TCP_READ];
    string response;
    string verb_response;

    while (1) {
        addrlen = sizeof(addr);
        if ((new_fd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
            perror("accept failed");
            exit(1);
        }

        n = read(new_fd, request, MAX_TCP_READ);
        if (n == -1) {
            perror("read failed");
            exit(1);
        }

        request[n] = '\0';

        char* arg1 = new char[MAX_COMMAND_LINE];
        char* arg2 = new char[MAX_COMMAND_LINE];
        char* arg3 = new char[MAX_COMMAND_LINE];
        char* arg4 = new char[MAX_COMMAND_LINE];

        sscanf(request, "%s %s %s %s", arg1, arg2, arg3, arg4);
        std::cout << (string)arg1;

        if (!strcmp(arg1, "GSB")) {
            int status = check_score_file();
            switch (status) {
            case STATUS_EMPTY:
                std::cout << "EMPTY";
                response = "RSB EMPTY\n";
                break;

            case STATUS_OK:
                string score_file = create_scoreboard();
                pid_t pid = getpid();
                string filename = "TOPSCORES_" + std::to_string(pid) + ".txt";
                string size = std::to_string(score_file.length());
                response = "RSB OK " + filename + " " + size + " " + score_file + "\n";
                std::cout << score_file;
                break;
            }

        } else if (!strcmp(arg1, "GHL")) {
            std::cout << "AQUI";
            int status = check_image(arg2);
            switch (status) {
            case STATUS_OK:
                string hint_file = get_hint_file(arg2);
                string filename = get_hint_filename(arg2);
                string size = std::to_string(hint_file.length());
                std::cout << filename;
                std::cout << size;
                response = "RHL OK " + filename + " " + size + " " + hint_file + "\n";
                break;
            }

        } else if (!strcmp(arg1, "STA")) {

        } else {
            std::cout << "OLA";
        }

        // SEND RESPONSE
        
        // AFTER SEND RESPONSE CLOSE NEW_FD
        close(new_fd);
    }
}

void end_TCP_session(void)
{
    std::cout << "Clossing TCP session..." << std::endl;    

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
    std::cout << "setup TCP" << std::endl;
    process();

    end_TCP_session();

    return 0;
}
