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
        // Read input
        int size;
        scanf("%d", &size);

        char *rows[size];
        char *columns[size];

        char input[100] = {'\0'};
        for (size_t i = 0; i < size; i++) {
                scanf("%s", input);
                rows[i] = strdup(input);
                memset(input, '\0', 100);
        }

        for (size_t i = 0; i < size; i++) {
                scanf("%s", input);
                columns[i] = strdup(input);
                memset(input, '\0', 100);
        }

        // Compile regexes
        DFAState row_dfas[size];
        DFAState column_dfas[size];

        memset(row_dfas, 0, sizeof(row_dfas[0]) * size);
        memset(column_dfas, 0, sizeof(column_dfas[0]) * size);

        for (size_t i = 0; i < size; i++){
                row_dfas[i] = *compile_regexp(rows[i]);
                column_dfas[i] = *compile_regexp(columns[i]);
        }

        // Solve
        char *solution;
        if (!solve(size, row_dfas, column_dfas, &solution)) {
                printf("No solution\n");
                return 0;
        }

        // Print solution
        for (size_t i = 0; i < size * size; i++) {
                printf("%c", solution[i]);
                if ((i+1) % size == 0)
                        printf("\n");
        }
        
        // Cleanup
        free(solution);

        return 0;
}