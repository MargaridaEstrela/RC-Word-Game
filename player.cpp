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


string UDP_send_receive(string message){
	int fd,errcode;
	ssize_t n;
	socklen_t addrlen;
	struct addrinfo hints,*res;
	struct sockaddr_in addr;
	char buffer[128];

	fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd==-1){
		exit(1);
	}
	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	errcode = getaddrinfo(GSIP.c_str(),GSPORT.c_str(),&hints,&res);
	if (errcode!=0){
		exit(1);
	}

	n = sendto(fd,message.c_str(),message.size(),0,res->ai_addr,res->ai_addrlen);
	if (n==-1){
		exit(1);
	}

	addrlen= sizeof(addr);
	n = recvfrom(fd,buffer,128,0,(struct sockaddr*) &addr,&addrlen);
	if (n==-1){
		exit(1);
	}

	write(1,"echo: ",6);
	write(1,buffer,n);

	freeaddrinfo(res);
	close(fd);
	int i=0;
	string response = "";
	for(i=0;i<n;i++){
		response += buffer[i];
	}
	return response;

}






int main(int argc, char *argv[]) {

  flag_decoder(argc, argv);

  string command_line;
  string command;

  int n_trials = -1;
  string PLID="";
  int n_letters;
  char* guessed;

  while (1) {
    getline(cin,command_line,'\n');
    string* fields = new string[2];
    int f_counter = 0;
    string word;
    stringstream ss(command_line);
    while (ss >> word){
	    if (f_counter > 1){
		    cerr << "ERROR: Too many command fields. Check the proper command format\n";
		    f_counter = 3;
		    break;
	    }
	    fields[f_counter] = word;
	    f_counter++;
    }

    if (f_counter == 3){continue;}
    command = fields[0];

    if (command == "start" || command == "sg") {
	    if (fields[1]==""){
		    cerr << "ERROR: No PLID was given\n";
		    continue;
	    }
	    string request = "SNG " + fields[1] + "\n";
	    string response = UDP_send_receive(request);
	    stringstream rr(response);
	    rr >> word;
	    if (word != "RSG"){
		    cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
		    exit(EXIT_FAILURE);
	    }
	    rr >> word;
	    string output;
	    if (word == "OK"){
		    PLID = fields[1];
		    string n_misses;
		    rr >> n_letters;
		    rr >> n_misses;
		    n_trials = 0;
		    output = "New game started (max " + n_misses + " errors): ";
		    guessed = new char[n_letters];  
		    for(int c = 0; c < n_letters; c++){
			    output += "_ ";
			    guessed[c] = '_';
		    }
	    }
	    else if (word == "NOK"){
		    output = "Game still in progress. Use command 'quit' to end current game";
	    }
	    else {
		    cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
		    exit(EXIT_FAILURE);
	    }

	    cout << output + "\n";

    } else if (command == "play" || command == "pl") {

	    //FALTA COLOCAR TODAS AS LETRAS RECEBIDAS EM MAISCULAS SEMPRE, PARA NAO HAVER 
	    //PROBLEMAS DE COERENCIA NO JOGO
	    if (PLID == ""){
		    cerr << "ERROR: No player has yet started any games (PLID = NULL)\n";
		    continue;
           }
	    if (fields[1]==""){
                    cerr << "ERROR: No letter was given\n";
                    continue;
            }
            if ((int) fields[1].size() > 1 || isalpha(fields[1][0])==0){
		    cerr << "ERROR: Input given is not a letter\n";
		    continue;
	    }

	    n_trials++;
	    string request = "PLG " + PLID + " " + fields[1] + " " + to_string(n_trials) + "\n";
	    cout << request;
	    string response = UDP_send_receive(request);
	    cout << response;
	    stringstream rr(response);
	    rr >> word;
	    if (word != "RLG"){
		    cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
		    exit(EXIT_FAILURE);
	    }
	    rr >> word;
	    if (word == "OK"){
		    int ocorr;
		    rr >> word;
		    rr >> ocorr;
		    int pos[ocorr];
		    string output = "";
		    for (int j=0; j<ocorr; j++){
			rr >> pos[j];
		    }
		    int k=0;
		    for (int j=0; j<n_letters;j++){
			if (j==(pos[k]-1)){
				k++;
				guessed[j] = fields[1].at(0);
			}
			output = output + guessed[j] + " ";
        	    }
		    cout << "Yes, \"" + fields[1] +"\" is part of the word: " + output + "\n";

	    }

	    if (word == "WIN"){
		    string output = "";
		    for (int j=0; j<n_letters; j++){
			if (guessed[j]=='_'){
				guessed[j] = fields[1].at(0);
			}
			output = output + guessed[j];
	    	    }		
		    cout << "WELL DONE! The word was: " + output + "\n";
		    n_trials = -1;
		    delete[] guessed;
	    }
	    if (word == "DUP"){
		    cout << "The letter \"" + fields[1] + "\" has already been played. Try a new one\n";
		    n_trials--;
	    }
	    if (word == "NOK"){
		    cout << "The letter \"" + fields[1] + "\" is not part of the word. Try again\n";
            }
	    if (word == "OVR"){
		    cout << "GAME OVER! You have reached the max error limit for this word. Play another round?\n";
		    n_trials = -1;
		    delete[] guessed;
	    }
	    if (word == "INV"){//Ainda nao sei o que meter aqui, se dou so print da mensagem
		               //de erro, ou se termino logo o jogo, ou se ha alguma maneira
			       //de meter o numero de trials como deve de ser, depois exploro melhor
	    }
	    if (word == "ERR"){
		    cerr << "ERROR: The 'play' command was rejected by the server. Check if there is an ongoing game with the 'state' command\n";
		    continue;
	    } 

    } else if (command == "guess" || command == "gw") {
	   //Falta impedir que palavras tenham menos de 3 letras e mais de 30 (servidor nao aceita)
	    if (PLID == ""){
		    cerr << "ERROR: No player has yet started any games (PLID = NULL)\n";
		    continue;
           }
	    if (fields[1]==""){
		    cerr << "ERROR: No guess word was given\n";
		    continue;
	    }

	    n_trials++;
	    string request = "PWG " + PLID + " " + fields[1] + " " + to_string(n_trials) + "\n";
	    cout << request;
	    string response = UDP_send_receive(request);
	    cout << response;
	    stringstream rr(response);

	    rr >> word;
	    if (word != "RWG"){
		    cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
		    exit(EXIT_FAILURE);
	    }
	    rr >> word;

	    if (word == "WIN"){	
		    cout << "WELL DONE! You guessed: " + fields[1] + "\n";
		    n_trials = -1;
		    delete[] guessed;
	    }
	    if (word == "NOK"){
		    cout << "The guess \"" + fields[1] + "\" is not the hidden word. Try again\n";
            }
	    if (word == "OVR"){
		    cout << "GAME OVER! You have reached the max error limit for this word. Play another round?\n";
		    n_trials = -1;
		    delete[] guessed;
		    continue;
	    }
	    if (word == "INV"){//Ainda nao sei o que meter aqui, se dou so print da mensagem
		               //de erro, ou se termino logo o jogo, ou se ha alguma maneira
			       //de meter o numero de trials como deve de ser, depois exploro melhor
	    }
	    if (word == "ERR"){
		    cerr << "ERROR: The 'guess' command was rejected by the server. Check if there is an ongoing game with the 'state' command\n";
		    continue;
	    } 
    } else if (command == "scoreboard" || command == "sb") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    continue;
	    }
    } else if (command == "hint" || command == "h") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    continue;
	    }
    } else if (command == "state" || command == "st") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    continue;
	    }
    } else if (command == "quit") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    continue;
	    }
	    if (n_trials != -1){
	    	string request = "QUT " + PLID + "\n";
	    	string response = UDP_send_receive(request);
	    	stringstream rr(response);
	    	rr >> word;
            	if (word != "RQT"){
            		cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";    
	    		exit(EXIT_FAILURE);
           	}
	    	rr >> word;
	    	if (word == "OK"){
	    		cout << "Ongoing game has been closed\n";
			n_trials = -1;
			delete[] guessed;
	    	}
	    	else if (word == "ERR"){
			cout << "No ongoing game has been found\n";
	    	}
	    	else{
			cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
			exit(EXIT_FAILURE);
	    	}   
	    }
	    else {
		cerr << "No ongoing game at the moment\n";
	    }
            
    }else if (command == "exit") {
	    if (fields[1]!=""){
		    cerr << "ERROR: Argument not required in this command\n";
		    continue;
	    }
	    if (n_trials != -1){
	    	string request = "QUT " + PLID + "\n";
	    	string response = UDP_send_receive(request);
	    	stringstream rr(response);
	    	rr >> word;
            	if (word != "RQT"){
            		cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";    
	    		exit(EXIT_FAILURE);
           	}
	    	rr >> word;
	    	if (word == "OK"){
	    		cout << "Ongoing game has been closed\n";
			n_trials = -1;
			delete[] guessed;
	    	}
	    	else if (word == "ERR"){
			cout << "No ongoing game has been found\n";
	    	}
	    	else{
			cerr << "ERROR: Wrong Protocol Message Received. Terminating connection\n";
			exit(EXIT_FAILURE);
	    	}   
	    }
	    cout << "Exiting player application. Until next time!\n";
            exit(EXIT_SUCCESS);
    } else {
      cerr << "ERROR: Command name not known\n";
    }
    delete[] fields;
  }
}
