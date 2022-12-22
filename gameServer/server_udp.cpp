#include "../aux_functions.hpp"
#include "../constants.hpp"
#include "data.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
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
char* GSPORT = "58034";


// SESSION AND GAME STATE VARIABLES
int fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

struct sigaction oldact;

char* word;
bool verbose;
int status;


// FUNCTIONS
void setup_udp();
void process();
void end_UDP_session();
void sig_handler(int sig);


/*
 * Function responsible for form a UDP connection with client. 
 */
void setup_udp(void)
{
    int errcode;
    struct sigaction sig_action;

    fd = socket(AF_INET, SOCK_DGRAM, 0); // Create UDP socket

    if (fd < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP SOCKET
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, GSPORT, &hints, &res);

    if (errcode != 0)
        exit(1);

    // Bind the socket to the server address
    n = bind(fd, (const struct sockaddr*)res->ai_addr, res->ai_addrlen);
    if (n < 0) {
        perror("bind failed");
        exit(1);
    }

    // setup SIGINT action
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = &sig_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, &oldact);

    return;
}


/*
 * Function responsible for receive the client request and process it
 */
void process(void)
{
    std::cout << "Start UDP Process" << std::endl;

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

        // Following the start command, the Player requests to start a new game
        if (!strcmp(arg1, "SNG")) {

            verb_response += "Start new game -> ";

            int status = register_user(arg2);

            switch (status) {
            case STATUS_OK: {
                // The game can be started (or no play was yet received)
                char* game_word = get_player_word(arg2);
                int word_size = strlen(game_word);
                int errors = max_errors(word_size);
                response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                verb_response += "Success; word = \"" + (string)game_word + "\" (";
                verb_response += std::to_string(word_size) + " letters)\n";
                break;
            }
            case STATUS_NOK: {
                // The player has any ongoing game (with play moves)
                int word_size = get_word_size(arg2);
                int errors = max_errors(word_size);
                if (check_no_moves(arg2)) { // verify play moves
                    response = "RSG OK " + std::to_string(word_size) + " " + std::to_string(errors) + "\n";
                    verb_response += "No plays have yet occured in current game; re-sending previous response\n";
                }

                else {
                    response = "RSG NOK\n";
                    verb_response += "Fail; a game for this user is still active\n";
                }
                break;
            }
            case STATUS_ERR:
                // The syntax of the SNG was incorrect or the PLID is invalid.
                verb_response += "Error; PLID sent may be invalid\n";
                response = "RSG ERR\n";
                break;
            }

        } else if (!strcmp(arg1, "PLG")) {
            /* Following the play command, the Player sends the GS a request 
             * to check if letter is part of the word to be guessed and the
             * number of trials */
            verb_response += "Play letter -> ";

            status = check_play_status(arg2, arg3, atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
            case STATUS_OK: {
                // The letter guess was successful and the number of trials is correct
                verb_response += "Success; \"" + (string)arg3 + "\" is part of the word; word not guessed\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "T %s ", arg3);
                add_trial(arg2, trial_line, "OK");
                string pos = get_letter_positions(arg2, arg3);
                response = "RLG OK " + std::to_string(trial) + " " + pos + "\n";
                break;
            }
            case STATUS_WIN: {
                // The letter guess completes the word;
                verb_response += "Success; \"" + (string)arg3 + "\" is part of the word; word was guessed (game ended)\n";
                response = "RLG WIN " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "T %s ", arg3);
                add_trial(arg2, trial_line, "OK");
                end_current_game(arg2, "WIN");
                break;
            }
            case STATUS_DUP: {
                // The letter guess completes the word;
                verb_response += "Fail; \"" + (string)arg3 + "\" has been played before; no play is registed\n";
                response = "RLG DUP " + std::to_string(trial) +"\n";
                break;
            }
            case STATUS_NOK: {
                /* The letter is not part of the word to be guessed and the number of 
                 * attempts left is not zero */
                verb_response += "Fail; \"" + (string)arg3 + "\" is not part of the word\n";
                response = "RLG NOK " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "T %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                break;
            }
            case STATUS_OVR: {
                /* The letter is not part of the word to be guessed and there are
                 * more attempts available and there are no more attempts available */
                verb_response += "Fail; \"" + (string)arg3 + "\" is not part of the word; max error limit reached (game ended)\n";
                response = "RLG OVR " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "T %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                end_current_game(arg2, "FAIL");
                break;
            }
            case STATUS_INV: {
                // The trial number is not valid
                int last = check_last_played(arg2, arg3, "T");
                if (last == STATUS_INV) {
                    verb_response += "Error; trial number doesn't match; \"" + (string)arg3 + "\" may not be the last played letter \n";
                    response = "RLG INV " + std::to_string(trial) + "\n";
                } else if (last == STATUS_OK) {
                    verb_response += "Success; lost message re-sent; \"" + (string)arg3 + "\" is part of the word; word not guessed\n";
                    string pos = get_letter_positions(arg2, arg3);
                    response = "RLG OK " + string(arg4) + " " + pos + "\n";
                } else if (last == STATUS_NOK) {
                    verb_response += "Fail; lost message re-sent; \"" + (string)arg3 + "\" is not part of the word\n";
                    response = "RLG NOK\n";
                }
                break;
            }
            case STATUS_ERR: {
                // The syntax of the PLG was incorrect
                verb_response += "Error; PLID or letter \"" + (string)arg3 + "\" may be invalid or no game is currently active\n";
                response = "RLG ERR\n";
                break;
            }
            }

        } else if (!strcmp(arg1, "PWG")) {
            /* Following the guess command, the Player sends the GS a request to 
             * check if the word to guess is word, as well as the number of trials */
            verb_response += "Guess word -> ";

            status = check_guess_status(arg2, arg3, atoi(arg4));
            int trial = get_trials(arg2);

            switch (status) {
            case STATUS_WIN: {
                // The word guess was successful;
                verb_response += "Success; word \"" + (string)arg3 + "\" was the correct guess (game ended)\n";
                response = "RWG WIN " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "OK");
                end_current_game(arg2, "WIN");
                break;
            }
            case STATUS_NOK: {
                // The word guess is not correct and the number of attempts left is not zero
                verb_response += "Fail; word \"" + (string)arg3 + "\" was not the correct guess\n";
                response = "RWG NOK " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                break;
            }
            case STATUS_OVR: {
                // The word guess is not correct and there are no more attempts available
                verb_response += "Fail; word \"" + (string)arg3 + "\" was not the correct guess; max error limit reached (game ended)\n";
                response = "RWG OVR " + (string)arg4 + "\n";
                char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                sprintf(trial_line, "G %s ", arg3);
                add_trial(arg2, trial_line, "NOK");
                end_current_game(arg2, "FAIL");
                break;
            }
            case STATUS_INV: {
                /* The trial number is not the one expected by the GS, or if the
                 * player is repeating the last PWG message received by the GS with a
                 * diferent word */
                int last = check_last_played(arg2, arg3, "G");
                if (last == 0) {
                    verb_response += "Error; trial number doesn't match; \"" + (string)arg3 + "\" may not be the last word played\n";
                    response = "RLG INV " + std::to_string(trial) + "\n";
                } else if (last == 1) {
                    char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "OK");
                    end_current_game(arg2, "WIN");
                } else if (last == 2) {
                    verb_response += "Fail; lost message re-sent; \"" + (string)arg3 + "\" is not the hidden word\n";
                    response = "RWG NOK " + (string)arg4 + "\n";
                    char* trial_line = (char*)calloc(strlen(arg3) + 3, sizeof(char));
                    sprintf(trial_line, "G %s ", arg3);
                    add_trial(arg2, trial_line, "NOK");
                }
                break;
            }
            case STATUS_DUP: {
                // The word was sent in a previous trial
                verb_response += "Fail; word \"" + (string)arg3 +"\" has already been sent before; no play is registed\n";
                response = "RWG DUP " + std::to_string(trial) + "\n";
                break;
            }
            case STATUS_ERR:
                // The syntax of the PWG was incorrect
                verb_response += "Error; PLID or word \"" + (string)arg3 + "\" may be invalid or no game is currently active\n";
                response = "RWG ERR\n";
                break;
            }

        } else if (!strcmp(arg1, "QUT")) {
            /* Following the quit or exit commands, and if there is an ongoing game, 
             * the Player application sends the GS a message with the player PLID requesting 
             * to terminate the game. */

            verb_response += "Quit current game -> ";

            char* user_dir = create_user_dir(arg2);

            if (!user_exists(user_dir)) {
                verb_response += "Error; user doesn't yet exist in server database (no game has been played)\n";
                response = "RQT ERR\n";
            }

            char* user_game_dir = create_user_game_dir(arg2);
            
            // Check if there is an ongoing game
            if (check_ongoing_game(user_game_dir)) {
                verb_response += "Success; current game has been closed\n";
                end_current_game(arg2, "QUIT");
                response = "RQT OK\n";
            } else {
                verb_response += "Fail; there is no ongoing game currently\n";
                response = "RQT NOK\n";
            }
        } else {
            verb_response += "Error -> Protocol message \"" + (string)arg1 + "\" is not recognized by the server\n";
            response = "ERR\n";
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
        n = sendto(fd, response.c_str(), strlen(response.c_str()) * sizeof(char), 0,
            (struct sockaddr*)&addr, addrlen);
        if (n < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }
}

/* 
 * Function responsible for end the UDP session
 */
void end_UDP_session(void)
{
    std::cout << "Closing UDP session..." << std::endl;

    freeaddrinfo(res);
    close(fd);

    return;
}

/*
 * Function responsible for the handling of the interruption signal (ctrl + C).
 * Terminates all the process.
 */
void sig_handler(int sig)
{
    std::cout << "Caught Ctrl-C..." << std::endl;

    end_UDP_session();
    sigaction(SIGINT, &oldact, NULL);
    kill(0, SIGINT);

    return;
}

/*
 * Function responsible for decode the input and call functions to handle with it.
 */
int main(int argc, char* argv[])
{
    word = new char[sizeof(argv[1])];
    word = argv[1];
    GSPORT = new char[sizeof(argv[2])];
    GSPORT = argv[2];
    char* v = argv[3];

    if (!strcmp(v, "YES")) {
        verbose = true;
    } else {
        verbose = false;
    }

    setup_udp();

    process();

    end_UDP_session();

    return 0;
}
