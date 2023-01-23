#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "regex_crossword.h"

struct RegexPuzzle {
        uint32_t width, height;
        DFAState *row_dfas, *column_dfas;
};

// Range of characters to try when solving a cell
static uint32_t start_char = 64;
static uint32_t end_char = 91;


static int solve_cell(struct RegexPuzzle *puzzle, int row, int column, char **solution);


struct RegexPuzzle *create_puzzle(uint32_t width, uint32_t height, char **rows, char **columns)
{
        struct RegexPuzzle *puzzle = malloc(sizeof(struct RegexPuzzle));
        puzzle->width = width;
        puzzle->height = height;

        puzzle->row_dfas = malloc(sizeof(DFAState) * height);
        puzzle->column_dfas = malloc(sizeof(DFAState) * width);

        for (size_t i = 0; i < height; i++) {
                printf("row: %s\n", rows[i]);
                puzzle->row_dfas[i] = *compile_regexp(rows[i]);
        }
                

        for (size_t i = 0; i < width; i++) {
                printf("column: %s\n", columns[i]);
                puzzle->column_dfas[i] = *compile_regexp(columns[i]);
        }
                

        return puzzle;
}

void destroy_puzzle(struct RegexPuzzle *puzzle)
{
        free(puzzle->row_dfas);
        free(puzzle->column_dfas);
        free(puzzle);
}

int solve(struct RegexPuzzle *puzzle, char **solution)
{
        uint32_t width = puzzle->width;
        uint32_t height = puzzle->height;

        uint32_t cell_count = width * height;

        *solution = malloc(cell_count);
        char *result = *solution;
        memset(result, start_char, cell_count);
        
        for (int i = 0; i < cell_count;) {       // Go through cells
                int row = i / width;
                int column = i % width;
                
                if (!solve_cell(puzzle, row, column, solution)) {     // Backtrack
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

static int solve_cell(struct RegexPuzzle *puzzle, int row, int column, char **solution)
{
        uint32_t width = puzzle->width;
        uint32_t height = puzzle->height;

        DFAState *row_dfas = puzzle->row_dfas;
        DFAState *column_dfas = puzzle->column_dfas;

        char *result = *solution;

        int i = row * width + column;
        bool cell_solved = false;
        
        for (int z = result[i] + 1; z < end_char; z++) {
                char rune = z;

                int row_match = match_at(&row_dfas[row], rune, column);
                int column_match = match_at(&column_dfas[column], rune, row);

                if (row_match && column_match) {
                        result[i] = rune;
                        cell_solved = true;

                        // Check that the whole regex matches
                        if (column == width - 1) {    // end of row
                                char row_res[width + 1];
                                row_res[width] = '\0';

                                memcpy(row_res, &result[row*width], width);
                                
                                if (match(&row_dfas[row], row_res)) {
                                        cell_solved = true;
                                } else {
                                        cell_solved = false;
                                        continue;
                                }
                        }

                        if (row == height - 1) {    // end of column
                                char column_res[height + 1];
                                column_res[height] = '\0';

                                for (size_t j = 0; j < height; j++)
                                        column_res[j] = result[j*width+column];

                                if (match(&column_dfas[column], column_res)) {
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

        return cell_solved;
}