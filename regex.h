#ifndef REGEX_H
#define REGEX_H

struct List;

typedef struct State {
        int c;
        struct State *out;
        struct State *out1;
        int last_list;
} State;

typedef struct DFAState{
        struct List *states;
        struct DFAState *next[256];
        struct DFAState *left;
        struct DFAState *right;
} DFAState;

DFAState *compile_regexp(char *regexp);
int match(DFAState *start, char *s);
int match_at(DFAState *start, char c, int pos);

#endif