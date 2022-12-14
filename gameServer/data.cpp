#include "data.hpp"
#include "../aux_functions.hpp"
#include "../constants.hpp"

int register_user(char* PLID)
{

    if (!check_PLID(PLID)) {
        return STATUS_NOK;
    }

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

    // verify player moves

    return STATUS_NOK;
}

bool user_exists(char* path)
{
    DIR *user_dir;
    user_dir = opendir(path);

    if (!user_dir) {
        free(user_dir);
        return false;
    }

    return true;
    
}

char* create_user_dir(char* PLID)
{

    char* usr_dir = (char*)calloc(strlen(USR_DIR) + strlen(PLID) + 1, sizeof(char));

    if (check_PLID(PLID)) {
        sprintf(usr_dir, GAMES_DIR "/%s", PLID);
    }

    return usr_dir;
}
