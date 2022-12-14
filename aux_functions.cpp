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
