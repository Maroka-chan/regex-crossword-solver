#ifndef REGEX_CROSSWORD_H
#define REGEX_CROSSWORD_H

#include "regex.h"

typedef struct RegexPuzzle RegexPuzzle;

struct RegexPuzzle *create_puzzle(uint32_t width, uint32_t height, char **rows, char **columns);
void destroy_puzzle(RegexPuzzle *puzzle);
int solve(struct RegexPuzzle *puzzle, char **solution);

#endif