#include <arpa/inet.h>
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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// GLOBAL VARIABLES
string GSIP = "127.0.0.1";
 
string GSPORT = "58034";
 


void flag_decoder(int argc, char *argv[]) {

  if (argc == 1) {
    return;
  } else if (argc == 3 || argc == 5) {
    for (int i = 1; i < argc; i++) {
      if (string(argv[i]) == "-n") {
        GSIP = argv[i + 1];
      } else if (string(argv[i]) == "-p") {
        GSPORT = argv[i + 1];
      } else {
	cerr << "ERROR: unknown flag: " << argv[i] << "\n";
        exit(EXIT_FAILURE);
      }
      i++;
    }
  } else {
    cerr << "ERROR: invalid application start command\n";
    exit(EXIT_FAILURE);
  }

  return;
}


int main(int argc, char *argv[]) {

  flag_decoder(argc, argv);

  string command_line;
  string command;

  while (1) {
    getline(cin,command_line,'\n');

    string* fields = new string[2];
    int f_counter = 0;
    string word;
    stringstream ss(command_line);
    while (ss >> word){
	    if (f_counter > 1){
		    cerr << "ERROR: Too many command fields. Check the proper command format\n";
		    exit(EXIT_FAILURE);
	    }
	    fields[f_counter] = word;
	    f_counter++;
    }


    command = fields[0];

    if (command == "start" || command == "sg") {
	    if (fields[1]==""){
		    cerr << "ERROR: No PLID was given\n";
		    exit(EXIT_FAILURE);
	    }
	    string PLID = fields[1];
    } else if (command == "play" || command == "pl") {
	    if (fields[1]==""){
                    cerr << "ERROR: No letter was given\n";
                    exit(EXIT_FAILURE);
            }
            if ((int) fields[1].size() > 1 || isalpha(fields[1][0])==0){
		    cerr << "ERROR: Input given is not a letter\n";
		    exit(EXIT_FAILURE);
	    }
            char letter = fields[1][0];
    } else if (command == "guess" || command == "gw") {
	    if (fields[1]==""){
		    cerr << "ERROR: No guess word was given\n";
		    exit(EXIT_FAILURE);
	    }
	    string guess = fields[1];
    } else if (command == "scoreboard" || command == "sb") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    exit(EXIT_FAILURE);
	    }
    } else if (command == "hint" || command == "h") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    exit(EXIT_FAILURE);
	    }
    } else if (command == "state" || command == "st") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    exit(EXIT_FAILURE);
	    }
    } else if (command == "quit") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    exit(EXIT_FAILURE);
	    }
    } else if (command == "exit") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    exit(EXIT_FAILURE);
	    }
      exit(EXIT_SUCCESS);
    } else {
      cerr << "ERROR: Command name not known\n";
      exit(EXIT_FAILURE);
    }
  }
}
