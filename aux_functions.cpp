#include "aux_functions.hpp"
#include "constants.hpp"

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

int translate_status(std::string state)
{
    if (state == "OK") {
        return STATUS_OK;
    }

    else if (state == "WIN") {
        return STATUS_WIN;
    }

    else if (state == "DUP") {
        return STATUS_DUP;
    }

    else if (state == "NOK") {
        return STATUS_NOK;
    }

    else if (state == "OVR") {
        return STATUS_OVR;
    }

    else if (state == "INV") {
        return STATUS_INV;
    }

    else if (state == "ERR") {
        return STATUS_ERR;
    }

    else if (state == "EMPTY") {
        return STATUS_EMPTY;
    }

    else if (state == "ACT") {
        return STATUS_ACT;
    }

    else if (state == "FIN") {
        return STATUS_FIN;
    }

    else {
        return -1;
    }
}
