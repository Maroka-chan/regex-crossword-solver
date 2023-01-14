#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "regex_crossword.h"

static uint32_t start_char = 64;
static uint32_t end_char = 91;

int solve(int size, DFAState *row_dfas, DFAState *column_dfas, char **solution)
{
        *solution = malloc(size * size);
        char *result = *solution;
        memset(result, start_char, size * size);
        
        for (int i = 0; i < size * size;) {       // Go through cells
                int r = i / size;
                int c = i % size;

                bool cell_solved = false;
                for (int z = result[i] + 1; z < end_char; z++) {
                        char rune = z;

                        int row_match = match_at(&row_dfas[r], rune, c);
                        int column_match = match_at(&column_dfas[c], rune, r);

                        if (row_match && column_match) {
                                result[i] = rune;
                                cell_solved = true;

                                // Check that the whole regex matches
                                if (c == size - 1) {    // end of row
                                        char row[size + 1];
                                        row[size] = '\0';

                                        memcpy(row, &result[r*size], size);
                                        
                                        if (match(&row_dfas[r], row)) {
                                                cell_solved = true;
                                        } else {
                                                cell_solved = false;
                                                continue;
                                        }
                                }

                                if (r == size - 1) {    // end of column
                                        char column[size];
                                        column[size] = '\0';

                                        for (size_t j = 0; j < size; j++)
                                                column[j] = result[j*size+c];

                                        if (match(&column_dfas[c], column)) {
                                                cell_solved = true;
                                        } else {
                                                cell_solved = false;
                                                continue;
                                        }
                                }
                        }
                        if (cell_solved)
                                break;
                }
                if (!cell_solved) {     // Backtrack
                        result[i] = start_char;
                        i--;
                        if (i < 0)
                                return 0;
                } else {
                        i++;
                }
        }

        return 1;
}