#include "aux_functions.hpp"

bool check_PLID(char* PLID)
{

    if (sizeof(PLID) != 6) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        if (!isdigit(PLID[i])) {
            return false;
        }
    }

    return true;
}

int max_errors(int word_size)
{
    if (word_size <= 6) {
        return 7;
    } else if (word_size > 6 & word_size <= 10) {
        return 8;
    } else {
        return 9;
    }
}
