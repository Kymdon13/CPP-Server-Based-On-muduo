#include <iostream>

#include "util.h"

void errif(bool condition, const char *msg) {
    if (condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}