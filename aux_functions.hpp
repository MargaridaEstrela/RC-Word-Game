#ifndef AUX_FUNCTIONS
#define AUX_FUNCTIONS

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

using string = std::string;

bool check_PLID(char* PLID);
int max_errors(int word_size);
int translate_status(string state);

#endif
