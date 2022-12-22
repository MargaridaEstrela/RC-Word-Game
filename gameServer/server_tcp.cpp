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
#include <signal.h>

using string = std::string;

// GLOBAL VARIABLES
char* GSPORT = "58034";


// SESSION AND GAME STATE VARIABLES
bool verbose;
int status;

int fd, new_fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

struct sigaction oldact;


// FUNCTIONS
void setup_tcp();
string create_scoreboard();
string get_hint_filename();
int get_hint_fileI();
void process();
void end_TCP_session();
void sig_handler(int sig);



/*
 * Function responsible for form a TCP connection with client. 
 */
void setup_tcp(void)
{
    int errcode;
    struct sigaction sig_action;
    fd = socket(AF_INET, SOCK_STREAM, 0); // Create socket

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

    // Bind the socket to the server address
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }

    // Put the server socket in a passive mode
    if (listen(fd, 5) < 0) {
        perror("listen failed");
        exit(1);
    }

    // setup SIGINT action
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = &sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, &oldact);
}


/*
 * Function responsible for create the scoreboard
 */
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


/*
 * Function responsible for get the hint filename associated with the player
 */
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


/*
 * Function responsible for eads current game file 
 * to discover hint file name. Reads it and sends 
 * its contents through TCP to the user 
 */
int get_hint_file(char* PLID)
{

    string line, path;
    string hint_file = "";
    char* game_user_dir = create_user_game_dir(PLID);

    std::ifstream game(game_user_dir);
    std::getline(game, line);
    std::stringstream ss(line);
    ss >> line;
    ss >> hint_file;

    path = "HINTS/" + hint_file;
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


/*
 * Function responsible for receive the client request and process it
 */
void process(void)
{
    std::cout << "Start TCP Process" << std::endl;

    char request[MAX_TCP_READ];
    string response;
    string verb_response;

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

            sscanf(request, "%s %s", arg1, arg2);


            if (!strcmp(arg1, "GSB")) {
                // Th Player asks to receive the top-10 scoreboard.
                verb_response = "PLID not given: ";
                verb_response += "Get scoreboard -> ";
                int status = check_score_file();
                switch (status) {
                case STATUS_EMPTY:{
                    // The scoreboard is still empty (no game was yet won by any player)
                    response = "RSB EMPTY\n";
                    verb_response += "Fail; scoreboard is currently empty (no games have been won yet)\n";
                    break;
                }
                case STATUS_OK: {
                    // Sends a text file containing the top-10 scores of the game.
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
                // The Player asks to receive an image illustrating the class to which the word belongs.
                verb_response = "PLID = ";
                verb_response += (string)arg2 + ": ";
                verb_response += "Get hint file -> ";

                int status = check_image(arg2);
                switch (status) {
                    case STATUS_OK: {
                        // Sends a file containing the image illustrative of the word class.
                        int size = get_hint_file(arg2);
                        string filename = get_hint_filename(arg2);
                        verb_response += "Success; hint file will be sent under filename " + filename;
                        verb_response += " (" + std::to_string(size) + " bytes)\n";
                        break;
                    }
                    case STATUS_NOK:
                        // There is no file to be sent, or some other problem,
                        response = "RHL NOK\n";
                        verb_response += "Error; PLID may be invalid or no game must be currently ongoing\n";
                        break;
                    
                }

            } else if (!strcmp(arg1, "STA")) {
                // The Player asks about the state of the ongoing game at the Player.
                verb_response = "PLID = ";
                verb_response += (string)arg2 + ": ";
                verb_response += "Get game state file -> ";

                int status = check_current_state(arg2);
                switch(status) {
                    case STATUS_NOK: {
                        // The GS server finds no games (active or finished) for this player
                        response = "RST NOK\n";
                        verb_response += "Error; PLID may be invalid or no games are registered to it yet\n"; 
                        break;
                    }
                    case STATUS_ACT: {
                        // There is an ongoing game
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
                        // There is no ongoing game
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
            exit(EXIT_SUCCESS);
        }
        else {
            close(new_fd);
            continue;
        }
    }
}


/* 
 * Function responsible for end the UDP session
 */
void end_TCP_session(void)
{
    std::cout << "Closing TCP session..." << std::endl;    

    freeaddrinfo(res);
    close(fd);
}


/*
 * Function responsible for the handling of the interruption signal (ctrl + C).
 * Terminates all the process.
 */
void sig_handler(int sig)
{
    std::cout << "Caught Ctrl-C..." << std::endl;

    end_TCP_session();
    sigaction(SIGINT, &oldact, NULL);
    kill(0, SIGINT);

    return;
}


/*
 * Function responsible for decode the input and call functions to handle with it.
 */
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
