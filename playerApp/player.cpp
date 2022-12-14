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
string GSIP = "127.0.0.1";
string GSPORT = "58034";

int n_trials = -1;
string PLID = "";
int n_letters;
char* guessed;

// FUNCTIONS
void flag_decoder(int argc, char* argv[])
{

    if (argc == 1) {
        return;
    } else if (argc == 3 || argc == 5) {
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

int TCP_read_to_file(int fd, string filename, int byte_size, string prefix)
{

    char buffer[1024];
    ssize_t n;

    ofstream file(filename);
    file << prefix;

    while (byte_size > 0) {
        n = read(fd, buffer, 1024);
        if (n == -1) {
            close(fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        byte_size = byte_size - n;

        for (int i = 0; i < n; i++) {
            file << buffer[i];
        }
    }

    close(fd);
    file.close();
    return 0;
}

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
        return "-1 TCP";
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(GSIP.c_str(), GSPORT.c_str(), &hints, &res);
    if (errcode != 0) {
        close(fd);
        return "-1 TCP";
    }

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(fd);
        return "-1 TCP";
    }

    int n_left = message.size();
    n = write(fd, message.c_str(), message.size());
    while (n_left > 0) {
        n = write(fd, message.c_str(), message.size());
        if (n == -1) {
            close(fd);
            return "-1 TCP";
        }
        n_left = n_left - n;
    }

    string response = "";
    int n_status_read = 45; // tamanho maximo da resposta (sem a parte dos dados)
    while (n_status_read > 0) {
        n = read(fd, buffer, n_status_read);
        if (n == -1) {
            close(fd);
            return "-1 TCP";
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
        return "UDP";
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(GSIP.c_str(), GSPORT.c_str(), &hints, &res);
    if (errcode != 0) {
        close(fd);
        return "UDP";
    }

    n = sendto(fd, message.c_str(), message.size(), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(fd);
        return "UDP";
    }

    addrlen = sizeof(addr);

    fd_set rfds;
    int counter;
    int maxfd;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    maxfd = fd;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
    if (counter == -1) {
        close(fd);
        return "UDP";
    }

    if (counter == 0) { // TIMEOUT
        cerr << "ERROR: No response was received from the server (timeout = 5 seconds). Please insert the command again\n";
        return "LOST";
    }

    if (FD_ISSET(fd, &rfds)) {
        n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
        if (n == -1) {
            close(fd);
            return "UDP";
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

void disconnect()
{

    cout << "Requesting server for game termination...\n";
    if (n_trials > -1) {
        string word;
        string request = "QUT " + PLID + "\n";
        string response = UDP_send_receive(request);
        stringstream rr(response);
        rr >> word;

        if (word == "UDP") {
            cerr << "ERROR: System call for UDP message or reception has failed while trying to end current game. Terminating app...\n";
            exit(EXIT_FAILURE);
        } else if (word == "LOST") {
            while (word == "LOST") {
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
        rr >> word;
        if (word == "OK") {
            cout << "Ongoing game has been closed\n";
            n_trials = -1;
            delete[] guessed;
        }

        else if (word == "ERR") {
            cout << "No ongoing game has been found\n";
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


int error_check(string code,string protocol){
    if (code == "UDP") {
        cerr << "ERROR: A System call for UDP message or reception has failed. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }
        
    else if (code == "TCP") {
        cerr << "ERROR: A System call for TCP message or reception has failed. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }

    else if (code == "ERR") {
        cerr << "ERROR: Unexpected protocol message received by server. Check command syntax and definition\n";
        return -1;
    }

    else if (code == "LOST") {
        return -1; // UDP Message Lost
    }

    else if (code != protocol) {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }

    else {
        return 0;
    }

}

int translate_status (string state){
    if (state == "OK") {
        return STATUS_OK;
    }
        
    else if (state == "WIN") {
        return STATUS_WIN;
    }

    else if (state == "DUP") {
        return STATUS_DUP;
    }

    else if (state == "NOK") {
        return STATUS_NOK;
    }

    else if (state == "OVR") {
        return STATUS_OVR;
    }

    else if (state == "INV") {
        return STATUS_INV;
    }

    else if (state == "ERR") {
        return STATUS_ERR;
    }

    else if (state == "EMPTY") {
        return STATUS_EMPTY;
    }

    else if (state == "ACT") {
        return STATUS_ACT;
    }

    else if (state == "FIN") {
        return STATUS_FIN;
    }

    else {
        return -1;
    }  

}

void start_command(string ID){

    if (ID == "") {
        cerr << "ERROR: No PLID was given\n";
        return;
    }

    if (PLID != "" && PLID != ID) {
        cout << "Given PLID (" + ID + ") is different from the last PLID used (" + PLID + "). Disconnecting old player...\n";
        disconnect();
    }

    string request = "SNG " + ID + "\n";
    string response = UDP_send_receive(request);
    stringstream rr(response);
    string word;
    rr >> word;

    int err = error_check(word,"RSG");
    if (err == -1){
        return;
    }

    rr >> word;
    string output;
    int status = translate_status(word);

    switch (status) {
        case STATUS_OK: {
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
        default: {
            cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
            disconnect();
            cout << "Closing game app...\n";
            exit(EXIT_FAILURE);
            break;
        }
    }

    cout << output + "\n";

}

void play_command(string letter){

    if (letter == "") {
        cerr << "ERROR: No letter was given\n";
        return;
    } else if ((int)letter.size() > 1 || isalpha(letter[0]) == 0) {
        cerr << "ERROR: Input given is not a letter\n";
        return;
    }

    n_trials++;

    transform(letter.begin(), letter.end(), letter.begin(), ::toupper);

    string request = "PLG " + PLID + " " + letter + " " + to_string(n_trials) + "\n";
    string response = UDP_send_receive(request);

    stringstream rr(response);
    string word;
    rr >> word;

    int i = error_check(word,"RLG");
    if (i == -1){
        return;
    }

    rr >> word;
    string output="";
    int status = translate_status(word);
    switch(status){

        case STATUS_OK: {
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


}

/*void guess_command(string guess){

    if (guess == "") {
        cerr << "ERROR: No guess word was given\n";
        return;
    }

    transform(guess.begin(), guess.end(), guess.begin(), ::toupper);

    n_trials++;
    string request = "PWG " + PLID + " " + guess + " " + to_string(n_trials) + "\n";
    string response = UDP_send_receive(request);
    stringstream rr(response);

    rr >> word;

    if (word == "UDP") {
        cerr << "ERROR: A System call for UDP message or reception has failed. Terminating connection...\n";
        n_trials--;
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }

    if (word == "LOST") {
        n_trials--;
        continue; // UDP Message Lost
    }

    else if (word != "RWG") {
        n_trials--;
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }
    rr >> word;

    if (word == "WIN") {
        cout << "WELL DONE! You guessed: " + fields[1] + "\n";
        n_trials = -1;
        delete[] guessed;
    } else if (word == "NOK") {
        cout << "The guess \"" + fields[1] + "\" is not the hidden word. Try again\n";
    } else if (word == "OVR") {
        cout << "GAME OVER! You have reached the max error limit for this word. Play another round?\n";
        n_trials = -1;
        delete[] guessed;
        continue;
    } else if (word == "INV") {
        cerr << "ERROR: The number of trials is not coherent with the server. If a UDP timeout occured, please repeat the exact command\n";
    } else if (word == "ERR") {
        n_trials--;
        cerr << "ERROR: The 'guess' command was rejected by the server. No game must be active or word isn't valid (3-30 letters long)\n";
        continue;
    }

    else {
        cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
        n_trials--;
        disconnect();
        cout << "Closing game app...\n";
        exit(EXIT_FAILURE);
    }


}*/






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

        if (command == "start" || command == "sg") {
            start_command(fields[1]);
        } 
        
        else if (command == "play" || command == "pl") {
            play_command(fields[1]);
        } 
        
        else if (command == "guess" || command == "gw") {



        } else if (command == "scoreboard" || command == "sb") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            string response = TCP_send_receive("GSB\n");
            stringstream rr(response);
            int fd;
            string code;
            rr >> fd;
            rr >> code;
            if (code == "TCP") {
                cerr << "ERROR: A System call for TCP message or reception has failed. Terminating connection...\n";
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code == "ERR") {
                cerr << "ERROR: Unexpected protocol message received by server. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code != "RSB") {
                cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            string status;
            rr >> status;
            if (status == "EMPTY") {
                cout << "No games have yet been won on this server\n";
                close(fd);

            } else if (status == "OK") {
                string filename;
                int size;
                string prefix;
                rr >> filename;
                rr >> size;
                rr >> prefix;
                int i = TCP_read_to_file(fd, filename, size - 45, prefix);
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
            } else {
                cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

        } else if (command == "hint" || command == "h") {
            if (PLID == "") {
                cerr << "ERROR: PLID is currently NULL (no 'start' command has been used)\n";
                continue;
            }
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            string response = TCP_send_receive("GHL " + PLID + "\n");
            stringstream rr(response);
            int fd;
            string code;
            rr >> fd;
            rr >> code;

            if (code == "TCP") {
                cerr << "ERROR: System call for TCP message or reception has failed. Terminating connection...\n";
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code == "ERR") {
                cerr << "ERROR: Unexpected protocol message received by server. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code != "RHL") {
                cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }
            string status;
            rr >> status;
            if (status == "NOK") {
                cout << "The server could not respond to the request. Try again later\n";
                close(fd);
            } else if (status == "OK") {
                string filename;
                int size;
                string prefix;
                rr >> filename;
                rr >> size;
                rr >> prefix;
                int i = TCP_read_to_file(fd, filename, size - 45, prefix);
                if (i == -1) {
                    cerr << "ERROR: System call for TCP message or reception during file transfer has failed. Terminating connection...\n";
                    disconnect();
                    cout << "Closing game app...\n";
                    exit(EXIT_FAILURE);
                }
                cout << "Received hint file: " + filename + " (";
                cout << size;
                cout << " bytes)\n";
            } else {
                cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

        } else if (command == "state" || command == "st") {

            if (PLID == "") {
                cerr << "ERROR: PLID is currently NULL (no 'start' command has been used)\n";
                continue;
            }
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            string response = TCP_send_receive("STA " + PLID + "\n");

            stringstream rr(response);
            int fd;
            string code;
            rr >> fd;
            rr >> code;
            if (code == "TCP") {
                cerr << "ERROR: System call for TCP message or reception has failed. Terminating connection...\n";
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code == "ERR") {
                cerr << "ERROR: Unexpected protocol message received by server. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }

            else if (code != "RST") {
                cerr << "ERROR: Wrong Protocol Message Received. Terminating connection...\n";
                close(fd);
                disconnect();
                cout << "Closing game app...\n";
                exit(EXIT_FAILURE);
            }
            string status;
            rr >> status;
            if (status == "NOK") {
                cout << "No games have been played by this player (PLID = " + PLID + ")\n";
                close(fd);
            } else if (status == "ACT" || status == "FIN") {
                string filename;
                int size;
                string prefix = "";
                string word;
                rr >> filename;
                rr >> size;
                while (rr >> word) {
                    prefix = prefix + word + " ";
                }
                prefix.pop_back();
                int i = TCP_read_to_file(fd, filename, size - 45, prefix);
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
            }

        } else if (command == "quit") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            disconnect();

        } else if (command == "exit") {
            if (fields[1] != "") {
                cerr << "ERROR: Argument not required in this command\n";
                continue;
            }
            disconnect();
            cout << "Exiting player application. Until next time!\n";
            exit(EXIT_SUCCESS);
        } else {
            cerr << "ERROR: Command name not known\n";
        }
    }
}
