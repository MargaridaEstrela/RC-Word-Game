#include "data.hpp"
#include "../aux_functions.hpp"
#include "../constants.hpp"

#include <cstring>
#include <fstream>
#include <sstream>

int register_user(char* PLID)
{
    if (!check_PLID(PLID)) {
        return STATUS_NOK; // Aqui não pode ser STATUS_NOK, isso vai entrar em conflito com o STATUS_NOK do comando start
                           // Para além disso esta função faz o mesmo do que o que está no inicio do comando start, por isso
                           // podemos tirar essa parte de lá e ficamos só com esta (e esta retorna STATUS_ERR)
    }

    std::ifstream game;
    char *user_dir, *game_user_dir;
    int check;

    user_dir = create_user_dir(PLID);

    if (!user_exists(user_dir)) {
        check = mkdir(user_dir, 0777);

        if (check) {
            std::cerr << "Unable to create directory" << std::endl;
            exit(EXIT_FAILURE);
        } 

        return STATUS_OK;
    }

    game_user_dir = create_user_game_dir(user_dir, PLID);

    if (check_ongoing_game(game_user_dir)) { // Estás a ter o pensamento errado aqui. Tu queres dar OK se nenhum jogo estiver ativo
        game.open(game_user_dir);            // e NOK se estiver (ou seja, está ao contrário)
                                             // No entanto, um jogo sem jogadas nenhumas também conta como um mensagem OK, por isso
        if (game.is_open()) {                // temos de verificar isso
            int count = 0;
            string line;
            string word;
            int max_trials = 0;

            while (game) {
                std::getline(game, line);

                if (count == 0) {
                    std::stringstream stream_line(line);
                    stream_line >> word;
                    max_trials = max_errors(word.size());
                }
                count++;
            }

            if (count - 1 < max_trials) {
                return STATUS_OK;
            }
        }
    }

    return STATUS_NOK; 
}

bool user_exists(char* path) // Penso que esta função não funciona, acho que o opendir não deteta diretorias existentes
{                            // Estou a dizer isto porque esta função dá verdadeiro mesmo usando PLID's novos
    DIR* user_dir;
    user_dir = opendir(path);

    if (!user_dir) {
        free(user_dir);
        return false;
    }

    return true;
}

bool check_ongoing_game(char* path)
{
    DIR* game_dir;
    game_dir = opendir(path);

    if (!game_dir) {
        free(game_dir);
        return false;
    }

    return true;
}

char* create_user_dir(char* PLID)
{
    char* user_dir = (char*)calloc(strlen(USER_DIR), sizeof(char));

    if (check_PLID(PLID)) {
        sprintf(user_dir, GAMES_DIR "/%s", PLID);
    }

    return user_dir;
}

char* create_user_game_dir(char* user_dir, char* PLID)
{
    char* user_game_dir = (char*)calloc(strlen(USER_OG_GAME_DIR), sizeof(char));

    sprintf(user_game_dir, "%s/GAME_%s.txt", user_dir, PLID);

    return user_game_dir;
}

char* get_last_guess_letter(char* PLID)
{
    std::ifstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(user_dir, PLID);
    char* code = nullptr;
    char* letter = nullptr;

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
    game_user_dir = create_user_game_dir(user_dir, PLID);
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
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(user_dir, PLID);

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

void add_trial(char* PLID, char* trial)
{
    std::ofstream game;
    char *user_dir, *game_user_dir;
    user_dir = create_user_dir(PLID);
    game_user_dir = create_user_game_dir(user_dir, PLID);

    game.open(game_user_dir, std::ios::app);

    if (game.is_open()) {
        game << trial;
        game.close();
    }
}
