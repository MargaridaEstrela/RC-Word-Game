#ifndef DATA
#define DATA

#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <string>
#include <dirent.h>


// DIRECTORIES
#define GAMES_DIR "../GAMES"
#define SCORES_DIR "../SCORES"

// USER PATH
#define USR_DIR "../GAMES/xxxxxx"
#define GAME_USR_DIR "../GAMES/xxxxxx/GAME_xxxxxx.txt"

// FILE EXTENSION
#define HINT_FILE_EXTENSION "_hint.txt"



using string = std::string;


int register_user(char *PLID);
bool user_exists(char *PLID);
char* create_user_dir(char *PLID);

#endif //!DATA
