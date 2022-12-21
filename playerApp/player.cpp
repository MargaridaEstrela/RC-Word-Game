#include "../aux_functions.hpp"
#include "../constants.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <cstring>
#include <ctype.h>
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
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

using namespace std;

// GLOBAL VARIABLES
string GSIP = "127.0.0.1"; // Default server IP (local host)
string GSPORT = "58034"; // Default Port

// Session and Game state variables
int n_trials = -1;
string PLID = "";
int n_letters;
char* guessed;



// FUNCTIONS
void flag_decoder();
int TCP_read_to_file();
string TCP_send_receive();
string UDP_send_receive();
void disconnect();
int error_check();




/* 
 * Function responsible for the decoding of the player
 * application execution command (./player [-n GSIP] [-p GSport]).
 */
void flag_decoder(int argc, char* argv[])
{
    // No flags used
    if (argc == 1) {
        return;
    }
    // Flag handling
    else if (argc == 3 || argc == 5) {
        for (int i = 1; i < argc; i += 2) {
            if (string(argv[i]) == "-n") {
                GSIP = argv[i + 1];
            } else if (string(argv[i]) == "-p") {
                GSPORT = argv[i + 1];
            } else {
                cerr << "ERROR: unknown flag: " << argv[i] << ". Usage: ./player [-n GSIP] [-p GSport]\n";
                exit(EXIT_FAILURE);
            }
        }
    } else {
        cerr << "ERROR: invalid application start command. Usage: ./player [-n GSIP] [-p GSport]\n";
        exit(EXIT_FAILURE);
    }
    return;
}


/* 
 * Function responsible for reading the data sent through TCP to a file
 * (commands scoreboard, hint and state)
 * - fd : Socket file descriptor;
 * - filename : name of local file to be written;
 * - byte_size : size of data to be read;
 * - prefix : piece of data read in function TCP_send_receive that is not
 *   part of the status message;
 *   - returns 0 in case of success, -1 in failure  
 */
int TCP_read_to_file(int fd, string filename, int byte_size, string prefix)
{

    char buffer[MAX_TCP_READ];
    ssize_t n;

    ofstream file(filename);
    file << prefix;
    string data = "";

    while (byte_size > 0) {
        n = read(fd, buffer, MAX_TCP_READ);
        if (n == -1) {
            close(fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        byte_size = byte_size - n;

        for (int i = 0; i < n; i++) {
            data += buffer[i];
        }
    }

    data.pop_back(); // Message sent by server contains extra '\n'
    close(fd);
    file << data;
    file.close();
    return 0;
}


/* 
 * Function responsible for sending requests and receiving responses from the server, 
 * through TCP protocol
 * - message : Message to be sent to server;
 * - returns file descriptor for socket and the response
 * (in a unified string) in success, returns ERR_TCP in failure 
 */
string TCP_send_receive(string message)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_COMMAND_LINE];

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        return ERR_TCP;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(GSIP.c_str(), GSPORT.c_str(), &hints, &res);
    if (errcode != 0) {
        close(fd);
        return ERR_TCP;
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(fd);
        return ERR_TCP;
    }

    // Sending request to server
    int n_left = message.size();
    n = write(fd, message.c_str(), message.size());
    while (n_left > 0) {
        n = write(fd, message.c_str(), message.size());
        if (n == -1) {
            close(fd);
            return ERR_TCP;
        }
        n_left = n_left - n;
    }

    // Reading the response from server
    string response = "";
    int n_status_read = MAX_TCP_RESPONSE; // Max status response size (first_word + status + filename + size)
    while (n_status_read > 0) {
        n = read(fd, buffer, n_status_read);
        if (n == -1) {
            close(fd);
            return ERR_TCP;
        }
        if (n == 0) {
            break;
        }
        n_status_read = n_status_read - n;

        for (int i = 0; i < n; i++) {
            response += buffer[i];
        }
    }

    freeaddrinfo(res);

    return to_string(fd) + " " + response;
}


/* 
 * Function responsible for sending requests and
 * receiving responses from the server, through UDP protocol
 * - message : Message to be sent to server;
 * - returns response in success, returns ERR_UDP in failure,
 * returns ERR_LOST in case of timeout (loss of message) 
 */
string UDP_send_receive(string message)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_COMMAND_LINE];

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1) {
        return ERR_UDP;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(GSIP.c_str(), GSPORT.c_str(), &hints, &res);
    if (errcode != 0) {
        close(fd);
        return ERR_UDP;
    }

    // Request sent to server
    n = sendto(fd, message.c_str(), message.size(), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(fd);
        return ERR_UDP;
    }

    addrlen = sizeof(addr);

    // Message reception, designed to declare a possible message loss (timeout)
    fd_set rfds;
    int counter;
    int maxfd;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    maxfd = fd;
    struct timeval tv;
    tv.tv_sec = TIME_LIMIT; // Time limit to declare message loss
    tv.tv_usec = 0;
    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
    if (counter == -1) {
        close(fd);
        return ERR_UDP;
    }

    if (counter == 0) { // TIMEOUT
        cerr << "ERROR: No response was received from the server (timeout = 5 seconds). Please insert the command again\n";
        return ERR_LOST;
    }

    if (FD_ISSET(fd, &rfds)) {
        n = recvfrom(fd, buffer, MAX_COMMAND_LINE, 0, (struct sockaddr*)&addr, &addrlen);
        if (n == -1) {
            close(fd);
            return ERR_UDP;
        }
    }

    freeaddrinfo(res);
    close(fd);

    int i = 0;
    string response = "";

    for (i = 0; i < n; i++) {
        response += buffer[i];
    }

    return response;
}


/* 
 * Function responsible for requesting the server for
 * game termination, as well as handling the server
 * response. 
 */
void disconnect()
{
    cout << "Requesting server for game termination...\n"; // Only actually requests server if a game is currently active
    if (n_trials > -1) {
        string word;
        string request = "QUT " + PLID + "\n";
        string response = UDP_send_receive(request);
        stringstream rr(response);
        rr >> word;

        // First word error handling
        if (word == ERR_UDP) {
            cerr << "ERROR: System call for UDP message or reception has failed while trying to end current game. Terminating app...\n";
            exit(EXIT_FAILURE);
        }

        else if (word == ERR_LOST) {
            while (word == ERR_LOST) {
                cout << "Because message was lost during disconnect, another request will be automatically sent again\n";
                response = UDP_send_receive(request);
                stringstream rr(response);
                rr >> word;
            }
        }

        else if (word != "RQT") {
            cerr << "ERROR: Wrong Protocol Message received while trying to end current game. Terminating app...\n";
            exit(EXIT_FAILURE);
        }

        // Status case handling
        rr >> word;
        if (word == "OK") {
            cout << "Ongoing game has been closed\n";
            n_trials = -1;
            delete[] guessed;
        }

        else if (word == "NOK") {
            cout << "No ongoing game has been found\n";
        }

        else if (word == "ERR") {
            cerr << "ERROR: Server responded with error while trying to end current game. Terminating app...\n";
            exit(EXIT_FAILURE);
        }

        else {
            cerr << "ERROR: Wrong Protocol Message received while trying to end current game. Terminating app...\n";
            exit(EXIT_FAILURE);
        }
    }

    else {
        cout << "No ongoing game at the moment\n";
    }
}


/* 
 * Function responsible for checking for errors in the first word of the response, received
 * from the server and interpreted by the functions above
 * - code : first word of the message received;
 * - protocol : the correct word that should be received for the specific command
 * - returns 0 with success, -1 if an error is detected 
 *   */
int error_check(string code, string protocol)
{
    if (code == ERR_UDP) {
        cerr << "ERROR: A System call for UDP message or reception has failed. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }

    else if (code == ERR_TCP) {
        cerr << "ERROR: A System call for TCP message or reception has failed. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }

    else if (code == ERR_ERR) {
        cerr << "ERROR: Unexpected protocol message received by server. Check command syntax and definition\n";
        return -1; // Server returns ERR
    }

    else if (code == ERR_LOST) {
        return -1; // UDP Message Loss
    }

    else if (code != protocol) {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE); // Unknown response
    }

    else {
        return 0; // No errors
    }
}


// COMMANDS

/* 
 * Requests server for game start and interprets its response
 * - ID : PLID sent by the user in the command 
 */
void start_command(string ID)
{
    if (ID == "") {
        cerr << "ERROR: No PLID was given\n";
        return;
    }

    // If PLID is different from last, automatically disconnects the previous one
    if (PLID != "" && PLID != ID) {
        cout << "Given PLID (" + ID + ") is different from the last PLID used (" + PLID + "). Disconnecting old player...\n";
        disconnect();
        PLID = "";
    }

    // Sending request and reading response
    string request = "SNG " + ID + "\n";
    string response = UDP_send_receive(request);
    stringstream rr(response);
    string word;
    rr >> word;

    // First word error check
    int err = error_check(word, "RSG");
    if (err == -1) {
        return;
    }

    rr >> word;
    string output;
    int status = translate_status(word);

    // Status switch case
    switch (status) {
    case STATUS_OK: {
        // New game initialization
        PLID = ID;
        string n_misses;
        rr >> n_letters;
        rr >> n_misses;
        n_trials = 0;
        output = "New game started (max " + n_misses + " errors): ";
        guessed = new char[n_letters];

        for (int c = 0; c < n_letters; c++) {
            output += "_ ";
            guessed[c] = '_';
        }
        break;
    }
    case STATUS_NOK: {
        output = "Game still in progress. Use command 'quit' to end current game";
        break;
    }
    case STATUS_ERR: {
        output = "ERROR: The 'start' command was rejected by the server. Syntax or PLID may be invalid";
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }

    cout << output + "\n";
    return;
}


/* 
 * Requests server to accept a letter for the game word and interprets its response
 * - letter : letter guessed by the player 
 *   */
void play_command(string letter)
{
    if (letter == "") {
        cerr << "ERROR: No letter was given\n";
        return;
    } else if ((int)letter.size() > 1 || isalpha(letter[0]) == 0) {
        cerr << "ERROR: Input given is not a letter\n";
        return;
    }

    n_trials++;

    transform(letter.begin(), letter.end(), letter.begin(), ::toupper); // Setting to uppercase

    // Sending request and reading response
    string request = "PLG " + PLID + " " + letter + " " + to_string(n_trials) + "\n";
    string response = UDP_send_receive(request);

    stringstream rr(response);
    string word;
    rr >> word;

    // First word error check
    int i = error_check(word, "RLG");
    if (i == -1) {
        n_trials--;
        return;
    }

    rr >> word;
    string output = "";
    int status = translate_status(word);
    switch (status) {

    // Status switch case
    case STATUS_OK: {
        // Update stored word with correct letter guess
        int ocorr;
        rr >> word;
        rr >> ocorr;
        int pos[ocorr];

        for (int j = 0; j < ocorr; j++) {
            rr >> pos[j];
        }
        int k = 0;
        for (int j = 0; j < n_letters; j++) {
            if (j == (pos[k] - 1)) {
                k++;
                guessed[j] = letter.at(0);
            }
            output = output + guessed[j] + " ";
        }
        cout << "Yes, \"" + letter + "\" is part of the word: " + output + "\n";
        break;
    }
    case STATUS_WIN: {
        // Complete stored word and end current game
        for (int j = 0; j < n_letters; j++) {
            if (guessed[j] == '_') {
                guessed[j] = letter.at(0);
            }
            output = output + guessed[j];
        }
        cout << "WELL DONE! The word was: " + output + "\n";
        n_trials = -1;
        delete[] guessed;
        break;
    }
    case STATUS_DUP: {
        cout << "The letter \"" + letter + "\" has already been played. Try a new one\n";
        n_trials--;
        break;
    }
    case STATUS_NOK: {
        cout << "The letter \"" + letter + "\" is not part of the word. Try again\n";
        break;
    }
    case STATUS_OVR: {
        // End current game
        cout << "GAME OVER! You have reached the max error limit for this word. Play another round?\n";
        n_trials = -1;
        delete[] guessed;
        break;
    }
    case STATUS_INV: {
        cerr << "ERROR: The number of trials is not coherent with the server. If a UDP timeout occured, please repeat the exact command\n";
        break;
    }
    case STATUS_ERR: {
        n_trials--;
        cerr << "ERROR: The 'play' command was rejected by the server. Check if there is an ongoing game with the 'state' command\n";
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        n_trials--;
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }
    return;
}


/* 
 * Requests server to accept a word to be guessed and interprets its response
 * - guess : word guessed by the player 
 */
void guess_command(string guess)
{
    if (guess == "") {
        cerr << "ERROR: No guess word was given\n";
        return;
    }

    transform(guess.begin(), guess.end(), guess.begin(), ::toupper); // Setting word to uppercase

    // Sending request and reading response
    n_trials++;
    string request = "PWG " + PLID + " " + guess + " " + to_string(n_trials) + "\n";
    string response = UDP_send_receive(request);
    stringstream rr(response);
    string word;
    rr >> word;

    // First word error check
    int err = error_check(word, "RWG");
    if (err == -1) {
        n_trials--;
        return;
    }

    rr >> word;
    int status = translate_status(word);

    // Status switch case
    switch (status) {
    case STATUS_WIN: {
        // End current game
        cout << "WELL DONE! You guessed: " + guess + "\n";
        n_trials = -1;
        delete[] guessed;
        break;
    }
    case STATUS_NOK: {
        cout << "The guess \"" + guess + "\" is not the hidden word. Try again\n";
        break;
    }
    case STATUS_OVR: {
        // End current game
        cout << "GAME OVER! You have reached the max error limit for this word. Play another round?\n";
        n_trials = -1;
        delete[] guessed;
        break;
    }
    case STATUS_INV: {
        cerr << "ERROR: The number of trials is not coherent with the server. If a UDP timeout occured, please repeat the exact command\n";
        break;
    }
    case STATUS_ERR: {
        n_trials--;
        cerr << "ERROR: The 'guess' command was rejected by the server. No game must be active or word isn't valid (3-30 letters long)\n";
        break;
    }
    case STATUS_DUP: {
        n_trials--;
        cerr << "ERROR: The guess \"" + guess + "\" has already been sent before. Try another word\n";
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        n_trials--;
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }
    return;
}


/* 
 * Requests server for the top scores scoreboard and interprets its response 
 */
void scoreboard_command()
{
    // Sending request and reading response
    string response = TCP_send_receive("GSB\n");
    stringstream rr(response);
    int fd;
    string code;
    rr >> fd;
    rr >> code;

    // First word error check
    int i = error_check(code, "RSB");
    if (i == -1) {
        return;
    }

    string word;
    rr >> word;
    int status = translate_status(word);

    // Status switch case
    switch (status) {
    case STATUS_OK: {
        // Read the scoreboard and store it in a local file
        string filename;
        int size;
        string prefix;
        rr >> filename;
        rr >> size;
        rr >> prefix;
        int i = TCP_read_to_file(fd, filename, size - MAX_TCP_RESPONSE, prefix);
        if (i == -1) {
            cerr << "ERROR: A System call for TCP message or reception during file transfer has failed. Terminating connection...\n";
            disconnect();
            cout << "Closing game app...\n";
            exit(EXIT_FAILURE);
        }

        ifstream file(filename);
        if (file.is_open()) {
            cout << file.rdbuf();
        }

        file.close();
        cout << "Local copy of the scoreboard saved in file: " + filename + "\n";
        break;
    }
    case STATUS_EMPTY: {
        cout << "No games have yet been won on this server\n";
        close(fd);
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        close(fd);
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }
    return;
}


/* 
 * Requests server for a hint image of the current game and interprets its response 
 */
void hint_command()
{
    // Sending request and reading response
    string response = TCP_send_receive("GHL " + PLID + "\n");
    stringstream rr(response);
    int fd;
    string code;
    rr >> fd;
    rr >> code;

    // First word error check
    int err = error_check(code, "RHL");
    if (err == -1) {
        return;
    }

    string word;
    rr >> word;
    int status = translate_status(word);

    // Status switch case
    switch (status) {
    case STATUS_OK: {
        // Read the image data and store it in a local file
        string filename;
        int size;
        string prefix;
        rr >> filename;
        rr >> size;
        rr >> prefix;
        int i = TCP_read_to_file(fd, filename, size - MAX_TCP_RESPONSE, prefix);
        if (i == -1) {
            cerr << "ERROR: System call for TCP message or reception during file transfer has failed. Terminating connection...\n";
            disconnect();
            cout << "Closing game app...\n";
            exit(EXIT_FAILURE);
        }
        cout << "Received hint file: " + filename + " (";
        cout << size;
        cout << " bytes)\n";
        break;
    }
    case STATUS_NOK: {
        cout << "The server could not respond to the request. There may be a syntax error or no image available for transfer\n";
        close(fd);
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        close(fd);
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }
    return;
}


/* 
 * Requests server for the state of the current or last finished game and interprets its response 
 */
void state_command()
{
    // Sending request and reading response
    string response = TCP_send_receive("STA " + PLID + "\n");
    stringstream rr(response);
    int fd;
    string code;
    rr >> fd;
    rr >> code;

    // First word error check
    int err = error_check(code, "RST");
    if (err == -1) {
        return;
    }

    string word;
    rr >> word;
    int status = translate_status(word);

    // Status switch case
    switch (status) {
    case STATUS_ACT:
    case STATUS_FIN: {
        // Read either current or last finished game state and store it in a local file
        string filename;
        int size;
        string prefix = "";
        string word;
        rr >> filename;
        rr >> size;
        while (rr >> word) {
            prefix = prefix + word + " ";
        }
        prefix.pop_back(); // extra space
        int i = TCP_read_to_file(fd, filename, size - MAX_TCP_RESPONSE, prefix);
        if (i == -1) {
            cerr << "ERROR: System call for TCP message or reception during file transfer has failed. Terminating connection...\n";
            disconnect();
            cout << "Closing game app...\n";
            exit(EXIT_FAILURE);
        }
        ifstream file(filename);
        if (file.is_open()) {
            cout << file.rdbuf();
        }

        file.close();
        cout << "Local copy of the state file saved in file: " + filename + " (";
        cout << size;
        cout << " bytes)\n";
        break;
    }
    case STATUS_NOK: {
        cout << "No games have been played by this player (PLID = " + PLID + ") or command syntax may be wrong\n";
        close(fd);
        break;
    }
    default: {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        close(fd);
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
        break;
    }
    }

    return;
}


/* 
 * Infinite loop that reads commands and redirects them to the right function to be handled 
 */
int main(int argc, char* argv[])
{

    flag_decoder(argc, argv);

    string command_line;
    string command;
    string fields[2];

    while (1) {
        getline(cin, command_line, '\n');
        fields[0] = "";
        fields[1] = "";
        int f_counter = 0;
        string word;
        stringstream ss(command_line);

        // counter of words in command (max: 2)
        while (ss >> word) {
            if (f_counter > 1) {
                cerr << "ERROR: Too many command fields. Check the proper command format\n";
                f_counter = 3;
                break;
            }
            fields[f_counter] = word;
            f_counter++;
        }

        if (f_counter == 3) {
            continue;
        }

        command = fields[0];

        // Commands
        if (command == "start" || command == "sg") {
            start_command(fields[1]);
        }

        else if (command == "play" || command == "pl") {
            play_command(fields[1]);
        }

        else if (command == "guess" || command == "gw") {
            guess_command(fields[1]);

        }

        else if (command == "scoreboard" || command == "sb") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            scoreboard_command();

        }

        else if (command == "hint" || command == "h") {
            if (PLID == "") {
                cerr << "ERROR: PLID is currently NULL (no 'start' command has been used)\n";
                continue;
            }
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            hint_command();

        }

        else if (command == "state" || command == "st") {

            if (PLID == "") {
                cerr << "ERROR: PLID is currently NULL (no 'start' command has been used)\n";
                continue;
            }
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            state_command();

        }

        else if (command == "quit") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            disconnect();

        }

        else if (command == "exit") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            disconnect();
            cout << "Exiting player application. Until next time!\n";
            exit(EXIT_SUCCESS);
        }

        else {
            cerr << "ERROR: Command name not known\n";
        }
    }
}
