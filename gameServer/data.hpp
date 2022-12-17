#ifndef DATA
#define DATA

#include <sys/types.h>
#include <filesystem>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <sstream>
#include <stdio.h>
#include <string>


// DIRECTORIES AND FILES
#define GAMES_DIR "GAMES"
#define SCORES_DIR "SCORES"
#define WORD_FILE "word_eng.txt"

// USER PATH
#define USER_DIR "GAMES/xxxxxx"
#define USER_OG_GAME_DIR "GAMES/GAME_xxxxxx.txt" //on_going games

// FILE EXTENSION
#define HINT_FILE_EXTENSION "_hint.txt"


using string = std::string;


int register_user(char *PLID);

bool user_exists(char *path);
bool check_ongoing_game(char *path);

char* create_user_dir(char *PLID);
char* create_user_game_dir(char *PLID);

char* get_last_guess_letter(char *PLID);
char* get_last_guess_word(char *PLID);
int get_trials(char *PLID);

void add_trial(char* PLID, char* line, char* code);

#endif //!DATA
