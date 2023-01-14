#ifndef REGEX_CROSSWORD_H
#define REGEX_CROSSWORD_H

#include "regex.h"

int solve(int size, DFAState *row_dfas, DFAState *column_dfas, char **solution);

#endif