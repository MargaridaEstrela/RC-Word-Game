#include "server.cpp"
#include "../constants.hpp"

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

using string = std::string;

// GLOBAL VARIABLES
int fd;
ssize_t n;

struct addrinfo hints, *res;
struct sockaddr_in addr;
socklen_t addrlen;

string PLID;
int status = 0; // no to be here
int trials = 0; // not to be here
char *guess;


void setup_udp(void){

  int errcode;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd < 0) {
    perror("socket failed");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;      // IPv4
  hints.ai_socktype = SOCK_DGRAM; // UDP SOCKET
  hints.ai_flags = AI_PASSIVE;

  errcode = getaddrinfo(NULL, GSPORT.c_str(), &hints, &res);

  if (errcode != 0)
    exit(1);

  n = bind(fd, (const struct sockaddr*) res->ai_addr, res->ai_addrlen);
  if (n < 0) {
    perror("bind failed");
    exit(1);
  }

  return;
}

int max_errors(int word_size) {
  if (word_size <= 6) {
    return 7;
  } else if (word_size > 6 & word_size <= 10) {
    return 8;
  } else {
    return 9;
  }
}


void process(){

  char request[MAX_COMMAND_LINE];
  string response;

  while (1) {

    addrlen = sizeof(addr);
    n = recvfrom(fd, request, MAX_COMMAND_LINE, 0, (struct sockaddr *)&addr, &addrlen);

    printf("message received\n");

    if (n < 0) {
      perror("recvfrom failed");
      exit(EXIT_FAILURE);
    }

    request[n] = '\0';
    if (verbose) {
      // TO DO
    }

    char *arg1 = new char[MAX_COMMAND_LINE];
    char *arg2 = new char[MAX_COMMAND_LINE];
    char *arg3 = new char[MAX_COMMAND_LINE];
    char *arg4 = new char[MAX_COMMAND_LINE];

    sscanf(request, "%s %s %s %s", arg1, arg2, arg3, arg4);

    if (!strcmp(arg1, "SNG")) {
      if (strlen(arg2) != 6) {
        cerr << "PLID: bad format. PLID is always sent using 6 digits.\n";
        exit(EXIT_FAILURE);
      }

      PLID = arg2;
      
      // status will be the output from register_user
      // we'll have a file just for register, unregister, etc.


      // check if PLID has any ongoing game (whit play moves)

      switch(status) {
        case STATUS_OK:
          response = "RSG OK " + std::to_string(max_errors(word.length()));
          break;
        case STATUS_NOK:
          response = "RSG NOK"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
      }
    } else if (!strcmp(arg1, "PLG")) {

      int n = 0;
      char letter = arg3[0];
      
      for (int i = 0; i < word.length(); i++) {
        if (letter == word[i]) { // não gosto disto assim mas por agora deve funcionar
          n ++;
        }
      }

      switch(status) {
        case STATUS_OK:
          response = "RLG OK " + to_string(letter) + " " + to_string(n) + "\0";
          break;
        case STATUS_WIN:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
        case STATUS_DUP:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
        case STATUS_NOK:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
        case STATUS_OVR:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
        case STATUS_INV:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
        case STATUS_ERR:
          response = "RLG NOK\0"; 
          // checks if player PLID has any ongoing game (with play moves)
          break;
      }


      

    } else if (!strcmp(arg1, "PWG")) {

    } else if (!strcmp(arg1, "QUT")) {

    } else if (!strcmp(arg1, "REV")) {

    } else {
      cerr << "ERROR_UDP: invalid command." << std::endl;
      exit(EXIT_FAILURE);
    }
    

    // SEND RESPONSE
    n = sendto(fd, response.c_str(), strlen(response.c_str())*sizeof(char), 0, (struct sockaddr *)&addr, addrlen);
    if (n < 0) {
      perror("sendto failed");
      exit(EXIT_FAILURE);
    }
  }


}

void end_UDP_session() {

  freeaddrinfo(res);
  close(fd);

  return;
}

int main(int argc, char *argv[]) {

  word = argv[1];
  guess = new char[word.length()];
  GSPORT = argv[2];
  verbose = argv[3];

  setup_udp();
  process();

  end_UDP_session();

  return 0;

}
