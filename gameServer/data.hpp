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
#define USER_OG_GAME_DIR "GAMES/GAME_xxxxxx.txt"

// FILE EXTENSION
#define HINT_FILE_EXTENSION "_hint.txt"


using string = std::string;

//
//FUNCTIONS
//

int register_user(char *PLID);

// VERIFY USER
bool user_exists(char *path);
bool check_ongoing_game(char *path);

// CREATE DIRECTORIES & FILES
char* create_user_dir(char *PLID);
char* create_user_game_dir(char *PLID);
void create_game_file(char* PLID);
int create_score_file(char* PLID);

// SCORES
char* get_score_filename(char *PLID, int score);
int check_score_file(void);
string read_score_file(string filename);
string get_scores();

char* get_new_game(char* code);
void end_current_game(char *PLID, char *code);
string find_last_game(char* PLID);

// WORD
char* get_player_word(char *PLID);
int get_word_size(char *PLID);

// PLAY
string get_letter_positions(char *PLID, char *letter);

// TRIALS
int get_trials(char *PLID);
void add_trial(char* PLID, char* line, char* code);


// VERIFICATIONS
int check_completion(char *PLID, char *play);
int check_no_moves(char *PLID);
int check_dup(char *PLID, char *play);
int check_word_dup(char* PLID, char* guess);
int check_letter(char *PLID, char *letter);
int check_word(char* PLID, char* guess);
int check_last_played(char *PLID, char* guess, char *code);
int check_play_status(char* PLID, char* letter, int trials);
int check_guess_status(char* PLID, char* guess, int trials);
int check_image(char *PLID);


int get_errors(char *PLID);

// STATE
int check_current_state(char *PLID);
string get_last_state(char* PLID);
string get_active_state(char* PLID);


#endif //!DATA
