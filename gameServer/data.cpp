#include "data.hpp"
#include "../aux_functions.hpp"
#include "../constants.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

void create_game_file(char* PLID){

    srand(time(0));
    int index = rand() % WORD_COUNT;
    string line;
    char *game_user_dir = create_user_game_dir(PLID);
    std::ofstream new_game (game_user_dir);
    std::ifstream word_file(WORD_FILE);
    for (int i = 0; i < index ; i++){
        std::getline(word_file,line);
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
        check = mkdir(user_dir,0777);

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
    game = fopen(path,"r");
    if (game == NULL){
        return false;
    }
    else{
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
    char* user_game_dir = (char*)calloc(/*strlen(USER_OG_GAME_DIR)+*/30, sizeof(char));

    sprintf(user_game_dir, "%s/GAME_%s.txt",GAMES_DIR, PLID);

    return user_game_dir;
}

char* get_last_guess_letter(char* PLID)
{
    std::ifstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(PLID);
    //char* code = nullptr;
    //char* letter = nullptr;
    char* code;
    char* letter;

    game.open(game_user_dir);

    if (game.is_open()) {
        string line;
        int count = 0;

        while (std::getline(game,line)) {

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
