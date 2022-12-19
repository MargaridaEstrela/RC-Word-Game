#include "data.hpp"
#include "../aux_functions.hpp"
#include "../constants.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

void create_game_file(char* PLID)
{

    srand(time(0));
    int index = rand() % WORD_COUNT;
    string line;
    char* game_user_dir = create_user_game_dir(PLID);
    std::ofstream new_game(game_user_dir);
    std::ifstream word_file(WORD_FILE);
    for (int i = 0; i < index; i++) {
        std::getline(word_file, line);
    }
    word_file.close();
    new_game << line;
    new_game << "\n";
    new_game.close();
}

int register_user(char* PLID)
{
    if (!check_PLID(PLID)) {
        return STATUS_ERR;
    }

    std::ifstream game;
    char *user_dir, *game_user_dir;
    int check;

    user_dir = create_user_dir(PLID);

    if (!user_exists(user_dir)) {
        check = mkdir(user_dir, 0777);

        if (check == -1) {
            std::cerr << "Unable to create directory" << std::endl;
            exit(EXIT_FAILURE);
        }

        create_game_file(PLID);
        return STATUS_OK;
    }

    game_user_dir = create_user_game_dir(PLID);

    if (check_ongoing_game(game_user_dir)) {
        free(game_user_dir);
        return STATUS_NOK;
    }

    create_game_file(PLID);
    return STATUS_OK;
}

bool user_exists(char* path)
{
    struct stat user_dir;

    int stat_user_dir = stat(path, &user_dir);

    if (stat_user_dir != 0) {
        return false;
    }
    return (user_dir.st_mode & S_IFDIR);
}

bool check_ongoing_game(char* path)
{
    FILE* game;
    game = fopen(path, "r");
    if (game == NULL) {
        return false;
    } else {
        fclose(game);
        return true;
    }
}

char* create_user_dir(char* PLID)
{
    char* user_dir = (char*)calloc(strlen(USER_DIR), sizeof(char));

    if (check_PLID(PLID)) {
        sprintf(user_dir, GAMES_DIR "/%s", PLID);
    }

    return user_dir;
}

char* create_user_game_dir(char* PLID)
{
    char* user_game_dir = (char*)calloc(/*strlen(USER_OG_GAME_DIR)+*/ 30, sizeof(char));

    sprintf(user_game_dir, "%s/GAME_%s.txt", GAMES_DIR, PLID);

    return user_game_dir;
}

int create_score_file(char* PLID){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    game.open(game_user_dir);
    if (game.is_open()){
    char* filename;
    char* word; 
    string line,code,state;
    int hits = 0;
    int total = 0;
    float score;

    while (std::getline(game,line)) {
        std::stringstream stream_line(line);
        stream_line >> code;
        if (code == "T" || code == "G") {
            total++;
            stream_line >> code;
            stream_line >> state;
            if (state == "OK"){
                hits++;
            }
        }
    }

    game.close();
    score = (hits*100/total);
    filename = get_score_filename(PLID,(int)score);
    word = get_player_word(PLID);
    std::ofstream score_file (filename);
    score_file << (int)score;
    score_file << " ";
    score_file << PLID;
    score_file << " ";
    score_file << word;
    score_file << " ";
    score_file << hits;
    score_file << " ";
    score_file << total;
    score_file << "\n";
    score_file.close();
    return 0;
    }
    return -1;
}

char* get_new_name(char* code){
    time_t rawtime;
    tm* timeinfo;
    char* buffer = new char[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"/%Y%m%d_%H%M%S",timeinfo);
    strcat(buffer,"_");
    strcat(buffer,code);
    strcat(buffer,".txt");
    
    return buffer;
}

char* get_score_filename(char* PLID, int score){
    time_t rawtime;
    tm* timeinfo;
    char* buffer = new char[80];
    char* filename = new char[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%d%m%Y_%H%M%S",timeinfo);
    sprintf(filename,"SCORES/%d_%s_",score,PLID);
    strcat(filename,buffer);
    strcat(filename,".txt");
    
    return filename;
}

void end_current_game(char* PLID,char* code){

    if (!strcmp(code,"WIN")){
        int i = create_score_file(PLID);
    }

    char* game_user_dir = create_user_game_dir(PLID);
    char* user_dir = create_user_dir(PLID);
    char* command = new char[100];

    std::ofstream game(game_user_dir,std::ios::app);
    game << (string)code;
    game << "\n";
    game.close();

    strcpy(command,"mv ");
    strcat(command,game_user_dir);
    strcat(command," ");
    strcat(command,user_dir);

    char* new_file = get_new_name(code);
    strcat(command,new_file);
    system(command);

    return;
}




char* get_player_word(char* PLID){
    string line;
    char* word = new char[MAX_WORD_SIZE];
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game(game_user_dir);
    std::getline(game,line);
    std::stringstream ss(line);
    ss >> word;
    return word;
}

int get_word_size(char* PLID){

    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    string line;
    game.open(game_user_dir);
    if (game.is_open()){
        std::getline(game,line);
        std::stringstream ss(line);
        ss >> line;
        game.close();
        return line.length();
    }
    game.close();
    return -1;
}

char* get_last_guess_letter(char* PLID)
{
    std::ifstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(PLID);
    char* code = nullptr;
    char* letter = nullptr;

    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;

        while (std::getline(game, line)) {

            if (count == 0) {
                continue;
            }

            std::stringstream stream_line(line);
            stream_line >> code;
            if (!strcmp(code, "T")) {
                stream_line >> letter;
            }
        }
    }

    return letter;
}

char* get_last_guess_word(char* PLID)
{
    std::ifstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(PLID);
    char* code = nullptr;
    char* word = nullptr;

    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;

        while (game) {
            std::getline(game, line);

            if (count == 0) {
                continue;
            }

            std::stringstream stream_line(line);
            stream_line >> code;

            if (!strcmp(code, "G")) {
                stream_line >> word;
            }
        }
    }
    return word;
}


string get_letter_positions(char* PLID,char* letter){
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    string line;
    char* word = new char[MAX_WORD_SIZE];
    game.open(game_user_dir);
    if (game.is_open()){
        std::getline(game,line);
        std::stringstream ss(line);
        ss >> word;
        game.close();
        int size = strlen(word);
        int counter = 0;
        line = "";
        for (int i = 0; i < size; i++){
            if (word[i] == letter[0]){
                counter++;
                line += std::to_string(i+1);
                line += " ";
            }
        }
        game.close();
        return std::to_string(counter) + " " + line;
    }
    game.close();
    return "ERR";
}


int get_trials(char* PLID)
{
    std::ifstream game;
    char* game_user_dir;
    game_user_dir = create_user_game_dir(PLID);

    game.open(game_user_dir);
    int count = 0;

    if (game.is_open()) {
        string line;

        while (game) {
            std::getline(game, line);
            count++;
        }
    }
    return count - 1;
}

void add_trial(char* PLID, char* trial, char* code)
{
    std::ofstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(PLID);

    game.open(game_user_dir, std::ios::app);

    if (game.is_open()) {
        game << trial;
        game << code;
        game << "\n";
        game.close();
    }
}

int check_completion(char* PLID,char* play){
    string code,state;
    char letter;
    char* game_user_dir = create_user_game_dir(PLID);
    char* play_word = get_player_word(PLID);
    int size = strlen(play_word);
    std::ifstream game;
    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;
        for (int i=0; i < size; i++){
            if (play_word[i] == play[0]){
                count++;
            }
        }

        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T") {
                stream_line >> letter;
                stream_line >> state;
                if (state == "NOK"){
                    continue;
                }
                for (int i = 0; i < size; i++){
                    if (play_word[i] == letter){
                        count++;
                    }
                }
            }
        }
        game.close();
        if (count == size){
            return 1;
        }
        else {
            return 0;
        }
    } else {
        return -1;
    }
}


int check_no_moves(char* PLID){
    int n = get_trials(PLID);
    if (n == 1){
        return 1;
    }
    else {
        return 0;
    }
}

int check_dup(char* PLID,char* play){
    char* game_user_dir = create_user_game_dir(PLID);
    string code, letter;
    std::ifstream game;
    game.open(game_user_dir);

    if (game.is_open()) {
        string line;

        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T") {
                stream_line >> letter;
                if (!strcmp(play,letter.c_str())){
                    game.close();
                    return 1; 
                }
            }
        }
        game.close();
        return 0;
    }
    return -1;
}

int check_letter(char* PLID, char* letter)
{
    int n = 0;
    char* word = get_player_word(PLID);
    int size = strlen(word);
    for (int i = 0; i < size; i++) {
        if (letter[0] == word[i]){
            n++;
        }
    }

    int max_err = max_errors(size);
    int errors = get_errors(PLID);
    if (n == 0 & errors < max_err) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n > 0) {
        if (check_completion(PLID,letter)){
            return STATUS_WIN;
        }
        else {
            return STATUS_OK;
        }
    }
}

int check_word(char* PLID, char* guess){
    int n;
    char* word = get_player_word(PLID);
    int size = strlen(word);
    if (!strcmp(word,guess)){
        n = 1;
    }
    else {
        n = 0;
    }

    int max_err = max_errors(size);
    int errors = get_errors(PLID);
    if (n == 0 & errors < max_err) {
        return STATUS_NOK;
    } else if (n == 0) {
        return STATUS_OVR;
    } else if (n > 0) {
        return STATUS_WIN;
    }
}

int check_last_played(char* PLID,char* guess,char* code){
    char* game_user_dir = create_user_game_dir(PLID);
    string word;
    char* last_w = new char[MAX_WORD_SIZE];
    char check = 'T';
    std::ifstream game;

    game.open(game_user_dir);

    if (game.is_open()) {
        string line;

        while (check == 'T' || check == 'G') {
            std::getline(game,line);
            std::stringstream ss(line);
            ss >> check;
            ss >> last_w;
            ss >> word;
        }
        game.close();
        if (!strcmp(code,"T")){
            if (last_w[0] == guess[0]){
                if (word == "OK"){
                    return STATUS_OK;
                }
                else {
                    return STATUS_NOK;
                }
            }
            else {
                return STATUS_INV;
            }
        }
        else{
            if (!strcmp(guess,last_w)){
                if (word == "OK"){
                    return STATUS_OK;
                }
                else {
                    return STATUS_NOK;
                }
            }
            else {
                return STATUS_INV;
            }
        }
    }
    return -1;
}

int check_play_status(char* PLID, char* letter, int trials){
    char* game_user_dir = create_user_game_dir(PLID);

    int size = strlen(letter);
    if (size > 1){
        return STATUS_ERR;
    }
    for (int i = 0; i < size; i++){
        if (isalpha(letter[i]) == 0){
            return STATUS_ERR;
        }
        letter[i] = tolower(letter[i]);
    }

    if (!check_PLID(PLID) || !check_ongoing_game(game_user_dir)) {
        return STATUS_ERR;
    }
    else if(trials == get_trials(PLID)){
        if (check_dup(PLID,letter)){
            return STATUS_DUP;
        }
        else {
            return check_letter(PLID,letter);
        }
    }
    else{
        return STATUS_INV;
    }

}


int get_errors(char* PLID){
    string code,state;
    char* game_user_dir = create_user_game_dir(PLID);
    std::ifstream game;
    game.open(game_user_dir);
    int errors = 0;

    if (game.is_open()) {
        string line;
        while (std::getline(game,line)) {
            std::stringstream stream_line(line);
            stream_line >> code;
            if (code == "T" || code == "G") {
                stream_line >> code;
                stream_line >> state;
                if (state == "NOK"){
                    errors++;
                }
            }
        }
        return errors;
    }
    return -1;
}
