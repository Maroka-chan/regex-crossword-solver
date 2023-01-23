#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "regex.h"
#include "regex_crossword.h"


// Split in group (e|f) does not work. I think it reads '|' as a regular character
int main (int argc, char *argv[])
{
        // Parse input
        int size;
        scanf("%d", &size);

        uint32_t height, width = size;

        char *rows[height];
        char *columns[width];

        char input[100] = {'\0'};
        for (size_t i = 0; i < height; i++) {
                scanf("%s", input);
                rows[i] = strdup(input);
                memset(input, '\0', 100);
        }

        for (size_t i = 0; i < width; i++) {
                scanf("%s", input);
                columns[i] = strdup(input);
                memset(input, '\0', 100);
        }

        // Create puzzle
        RegexPuzzle *puzzle = create_puzzle(width, height, rows, columns);

        // Solve
        char *solution;
        if (!solve(puzzle, &solution)) {
                printf("No solution\n");
                return 0;
        }

        // Print solution
        for (size_t i = 0; i < width * height; i++) {
                printf("%c", solution[i]);
                if ((i+1) % width == 0)
                        printf("\n");
        }
        
        // Cleanup
        free(solution);
        destroy_puzzle(puzzle);

        return 0;
}