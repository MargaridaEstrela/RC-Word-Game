#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
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
#include <unistd.h>

using namespace std;

// GLOBAL VARIABLES
string GSIP = "127.0.0.1";
string GSPORT = "59034";

void flag_decoder(int argc, char *argv[]) {

  cout << argv[1] << endl;
  cout << argv[2] << endl;
  // cout << argv[3] << endl;
  // cout << argv[4] << endl;

  if (argc == 1) {
    return;
  } else if (argc == 3 || argc == 5) {
    for (int i = 1; i < argc; i++) {
      if (string(argv[i]) == "-n") {
        GSIP = argv[i + 1];
      } else if (string(argv[i]) == "-p") {
        GSPORT = argv[i + 1];
      } else {
        perror("bad input");
        exit(EXIT_FAILURE);
      }
      i++;
    }
  } else {
    exit(EXIT_FAILURE);
  }

  return;
}

int main(int argc, char *argv[]) {

  flag_decoder(argc, argv);

  cout << "GSIP: " << GSIP << " GSPORT: " << GSPORT << endl;

  string command;

  while (1) {
    std::cin >> command;

    if (cin.fail()) {
      exit(EXIT_FAILURE);
    }

    if (command == "start" || command == "sg") {
      cout << "here yesss" << endl;
    } else if (command == "play" || command == "pl") {
    } else if (command == "guess" || command == "gw") {
    } else if (command == "scoreboard" || command == "sb") {
    } else if (command == "hint" || command == "h") {
    } else if (command == "state" || command == "st") {
    } else if (command == "quit") {
    } else if (command == "exit") {
      exit(EXIT_SUCCESS);
    } else {
      exit(EXIT_FAILURE);
    }
  }
}
