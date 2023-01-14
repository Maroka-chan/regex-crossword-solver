#include "stack.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct State {
        int c;
        struct State *out;
        struct State *out1;
        int last_list;
};

struct Frag {
        struct State *start;
        union Ptrlist *out;
};

union Ptrlist
{
	union Ptrlist *next;
	struct State *state;
};

struct List {
        struct State **states;
        uint32_t size;
};

struct DFAState
{
        struct List lst;
        struct DFAState *next[256];
        struct DFAState *left;
        struct DFAState *right;
};


int precedence(char c)
{
        switch (c) {
                case '+':
                case '-':
                        return 1;
                case '*':
                case '/':
                        return 2;
                case '^':
                        return 3;
        }
        return -1;
}

bool is_literal(char c)
{
        return (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || (c == '[' || c == ']');
}

void infix_to_postfix(char *regex)
{
        Stack *op_stack = st_init(strlen(regex));

        uint32_t sp = 0;

        for (size_t i = 0; regex[i]; i++) {
                char c = regex[i];

                // Match literal
                if (is_literal(c)) {
                        regex[sp++] = c;
                        if (!st_empty(op_stack) && st_peek(op_stack) != '(')
                                regex[sp++] = st_pop(op_stack);
                }

                else if (c == '(')
                        st_push(c, op_stack);

                else if (c == ')') {
                        while (st_peek(op_stack) != '(')
                                regex[sp++] = st_pop(op_stack);
                        st_pop(op_stack);
                }

                else if (c == '[')
                        st_push(c, op_stack);

                else if (c == ']') {
                        while (st_peek(op_stack) != '[')
                                regex[sp++] = st_pop(op_stack);
                        st_pop(op_stack);
                }

                else if (c == '|')
                        regex[sp++] = c;

                else { // Match operator
                        while (!st_empty(op_stack) 
                                        && precedence(c) 
                                        <= precedence(st_peek(op_stack)))
                                regex[sp++] = st_pop(op_stack);
                        st_push(c, op_stack);
                }
        }

        // Pop remaining elements
        while (!st_empty(op_stack))
                regex[sp++] = st_pop(op_stack);

        st_destroy(op_stack);

        regex[sp] = '\0';
}


union Ptrlist *list1(struct State **outp)
{
	union Ptrlist *l;
	
	l = (union Ptrlist*)outp;
	l->next = NULL;
	return l;
}

union Ptrlist *append(union Ptrlist *l1, union Ptrlist *l2)
{
        union Ptrlist *oldl1;
	
	oldl1 = l1;
	while(l1->next)
		l1 = l1->next;
	l1->next = l2;
	return oldl1;
}

void patch(union Ptrlist *lst, struct State *s)
{
	union Ptrlist *next;
	
	for(; lst; lst=next){
		next = lst->next;
		lst->state = s;
	}
}

struct Frag frag(struct State *start, union Ptrlist *out)
{
        struct Frag fr = { start, out };
        return fr;
}

uint32_t state_count;
struct State *state(int c, struct State *out, struct State *out1)
{
        struct State *st;

        state_count++;
        st = malloc(sizeof(*st));
        st->last_list = 0;
        st->c = c;
        st->out = out;
        st->out1 = out1;

        return st;
}

static int Split = 256;
static struct State Match = {.c = 257, NULL, NULL};

struct State *infix_to_nfa(char *postfix)
{
        char *p;
        struct Frag stack[1000], *stackp, e1, e2, e;
        struct State *st;

        uint32_t stack_size = 0;
        bool alternate_index[1000] = {false};

        #define push(s) *stackp++ = s; stack_size++
        #define pop()   *--stackp; stack_size--

        stackp = stack;
        for (p = postfix; *p; p++) {
                switch (*p) {
                        default:
                        {
                                st = state(*p, NULL, NULL);
                                push(frag(st, list1(&st->out)));
                                
                        } break;
                        case ')':
                        {
                                e2 = pop();
                                e1 = pop();
                                e = e2;
                                while (e1.start->c != '(') {
                                        patch(e1.out, e.start);
                                        e = e1;
                                        e1 = pop();
                                }
                                push(frag(e.start, e2.out));
                        } break;
                        case '[':
                        {
                                st = state(*p, NULL, NULL);
                                push(frag(st, list1(&st->out)));

                                if (*(p+1) == '^') {
                                        char group_chars[127] = {'\0'};
                                        p += 2;
                                        for (; *p != ']'; p++)
                                                group_chars[*p] = *p;

                                        for (size_t i = 65; i < 91; i++) {      // Only upper case characters seem to be necessary to solve the puzzles.
                                                if (i == '[' || i == ']' || i == '^' || i == '|' || i == '(' || i == ')' || i == '*' || i == '+' || i == '?' || i == '.')
                                                        continue;
                                                if (group_chars[i] == '\0') {
                                                        st = state(i, NULL, NULL);
                                                        push(frag(st, list1(&st->out)));
                                                }
                                        }
                                        p--;
                                }
                        } break;
                        case ']':
                        {
                                e2 = pop();
                                e1 = pop();
                                struct State *split_state = e2.start;
                                union Ptrlist *outs = list1(&split_state->out);

                                if (e2.start->c == '[') {
                                        fprintf(stderr, "Cannot have empty brackets. Invalid regex.");
                                        exit(EXIT_FAILURE);
                                }

                                while (e1.start->c != '[') {
                                        split_state = state(Split, e1.start, split_state);
                                        outs = append(e1.out, outs);
                                        e1 = pop();
                                }
                                push(frag(split_state, outs));
                        } break;
                        case '?':
                        {
                                e = pop();
                                st = state(Split, e.start, NULL);
                                push(frag(st, append(e.out, list1(&st->out1))));
                        } break;
                        case '*':
                        {
                                e = pop();
                                st = state(Split, e.start, NULL);
                                patch(e.out, st);
                                push(frag(st, list1(&st->out1)));
                        } break;
                        case '+':
                        {
                                e = pop();
                                st = state(Split, e.start, NULL);
                                patch(e.out, st);
                                push(frag(e.start, list1(&st->out1)));
                        } break;
                }
        }

        struct Frag alt_frags[1000], *alt_stackp;
        alt_stackp = alt_frags;
        int alt_size = 0;

        #define pushalt(s) *alt_stackp++ = s; alt_size++
        #define popalt()   *--alt_stackp; alt_size--

        while (stack_size > 1) {        // Concatenate the fragments together.
                e2 = pop();
                e1 = pop();

                if (e1.start->c == '|') {       // If the second fragment is an alternate, push it on the alt stack.
                        pushalt(e2);
                        continue;
                }

                patch(e1.out, e2.start);
                push(frag(e1.start, e2.out));
        }
        
        while (alt_size > 0) {       // Push alternates back to main stack
                e = popalt();
                push(e);
        }
        
        while (stack_size > 1) {       // Split the fragments.
                e2 = pop();
                e1 = pop();
                st = state(Split, e1.start, e2.start);
                push(frag(st, append(e1.out, e2.out)));
        }
        
        e = pop();
        patch(e.out, &Match);
        return e.start;
}

static uint32_t listid = 0;

void add_state(struct List *l, struct State *s)
{
        if (s == NULL || s->last_list == listid)
                return;
        s->last_list = listid;
        if (s->c == Split) {
                add_state(l, s->out);
                add_state(l, s->out1);
                return;
        }
        l->states[l->size++] = s;
}

struct List *start_list(struct State *state, struct List *lst)
{
        lst->size = 0;
        listid++;
        add_state(lst, state);
        return lst;
}

static int listcmp(struct List *l1, struct List *l2)
{
	int i;

	if(l1->size < l2->size)
		return -1;
	if(l1->size > l2->size)
		return 1;
	for(i=0; i < l1->size; i++)
		if(l1->states[i] < l2->states[i])
			return -1;
		else if(l1->states[i] > l2->states[i])
			return 1;
	return 0;
}

static int ptrcmp(const void *ptr1, const void *ptr2)
{
        if (ptr1 < ptr2)
                return -1;
        if (ptr1 > ptr2)
                return 1;
        return 0;
}

struct DFAState *all_dfa_states;
struct DFAState *dfa_state(struct List *l)
{
	int i;
	struct DFAState **dp, *d;

	qsort(l->states, l->size, sizeof l->states[0], ptrcmp);

	/* look in tree for existing DFAState */
	dp = &all_dfa_states;
	while((d = *dp) != NULL){
		i = listcmp(l, &d->lst);
		if(i < 0)
			dp = &d->left;
		else if(i > 0)
			dp = &d->right;
		else
			return d;
	}
	
	/* allocate, initialize new DFAState */
	d = malloc(sizeof *d + l->size * sizeof l->states[0]);
	memset(d, 0, sizeof *d);
	d->lst.states = (struct State**)(d+1);
	memmove(d->lst.states, l->states, l->size * sizeof l->states[0]);
	d->lst.size = l->size;

	/* insert in tree */
	*dp = d;
	return d;
}

void step(struct List *clist, int c, struct List *nlist)
{
        struct State *s;

        listid++;
        nlist->size = 0;
        for (size_t i = 0; i < clist->size; i++) {
                s = clist->states[i];
                if (s->c == c)
                        add_state(nlist, s->out);
        }
}

struct List l1;

struct DFAState *next_state(struct DFAState *d, int c)
{
	step(&d->lst, c, &l1);
	return d->next[c] = dfa_state(&l1);
}

int is_match(struct List *l)
{
        for (size_t i = 0; i < l->size; i++) {
                if (l->states[i] == &Match)
                        return 1;
        }
        return 0;
}

int match(struct DFAState *start, char *s)
{
        int c;
        struct DFAState *d, *next;

        d = start;
        for (; *s; s++) {
                c = *s & 0xFF;
                if ((next = d->next[c]) == NULL)
                        next = next_state(d, c);
                d = next;
        }

        return is_match(&d->lst);
}

int match_at(struct DFAState *start, char c, int pos)
{
        struct List clist = start->lst;
        char accept_char;

        if (pos == 0) {
                struct State *s;
                for (size_t i = 0; i < clist.size; i++) {
                        accept_char = clist.states[i]->c;
                        if (accept_char == c)
                                return 1;
                }
                return 0;
        }
                
        for (size_t i = 0; i < clist.size; i++) {
                accept_char = clist.states[i]->c;

                if (start->next[accept_char] == NULL)
                        start->next[accept_char] = next_state(start, accept_char);

                if (start->next[accept_char] != NULL)
                        return match_at(start->next[accept_char], c, pos - 1);
        }

        return 0;
}

struct DFAState *start_dfa_state(struct State *start)
{
	return dfa_state(start_list(start, &l1));
}

uint32_t start = 64;
uint32_t end = 91;

int solve(uint32_t size, char **rows, char **columns, char **solution)
{
        struct DFAState *row_dfas[size];
        struct DFAState *column_dfas[size];

        memset(row_dfas, 0, sizeof(row_dfas[0]) * size);
        memset(column_dfas, 0, sizeof(column_dfas[0]) * size);

        struct State *row_states[size];
        struct State *column_states[size];
        for (size_t i = 0; i < size; i++) {
                row_states[i] = infix_to_nfa(rows[i]);
                column_states[i] = infix_to_nfa(columns[i]);
        }

        l1.states = malloc(state_count * sizeof l1.states[0]);

        for (size_t i = 0; i < size; i++){
                row_dfas[i] = start_dfa_state(row_states[i]);
                column_dfas[i] = start_dfa_state(column_states[i]);
        }
        

        *solution = malloc(size * size);
        char *result = *solution;
        memset(result, start, size * size);
        
        for (int i = 0; i < size * size;) {       // Go through cells
                int r = i / size;
                int c = i % size;

                bool cell_solved = false;
                for (int z = result[i] + 1; z < end; z++) {
                        char rune = z;

                        int row_match = match_at(row_dfas[r], rune, c);
                        int column_match = match_at(column_dfas[c], rune, r);

                        if (row_match && column_match) {
                                result[i] = rune;
                                cell_solved = true;

                                // Check that the whole regex matches
                                if (c == size - 1) {    // end of row
                                        char row[size + 1];
                                        row[size] = '\0';

                                        memcpy(row, &result[r*size], size);
                                        
                                        if (match(row_dfas[r], row)) {
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

                                        if (match(column_dfas[c], column)) {
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
                if (!cell_solved) {
                        result[i] = start;
                        i--;
                        if (i < 0)
                                return 0;
                } else {
                        i++;
                }
        }

        return 1;
}

// Split in group (e|f) does not work. I think it reads '|' as a regular character
int main (int argc, char *argv[])
{
        int size = 2;
        char *rows[] = {"HE|LL|O+", "[PLEASE]+"};
        char *columns[] = {"[^SPEAK]+", "EP|IP|EF"};

        char *solution;
        solve(size, rows, columns, &solution);

        for (size_t i = 0; i < size * size; i++) {
                printf("%c", solution[i]);
                if ((i+1) % size == 0)
                        printf("\n");
        }

        // struct State *nfa = infix_to_nfa("HE|LL|O+");
        // struct DFAState *dfa = start_dfa_state(nfa);
        // printf("%d\n", match_at(dfa, 'H', 0));
        

        // Cleanup
        free(solution);
        free(l1.states);

        return 0;
}

        // int size;
        // scanf("%d", &size);

        // char *rows[size];
        // char *columns[size];
        // char input[100];

        // printf("size: %d\n", size);
        
        // for (size_t i = 0; i < size; i++) {
        //         scanf("%s", input);
        //         size_t len = strlen(input);
        //         if (len > 0 && input[len-1] == '\n') {
        //                 input[len-1] = '\0';
        //         }
        //         rows[i] = strdup(input);
        // }
                
        // for (size_t i = 0; i < size; i++) {
        //         scanf("%s", input);
        //         size_t len = strlen(input);
        //         if (len > 0 && input[len-1] == '\n') {
        //                 input[len-1] = '\0';
        //         }
        //         columns[i] = strdup(input);
        // }

        // // print rows
        // for (size_t i = 0; i < size; i++)
        //         printf("row: %s\n", rows[i]);

        // for (size_t i = 0; i < size; i++)
        //         printf("column: %s\n", columns[i]);
        


        // l1.lst = malloc(state_count * sizeof l1.lst[0]);

        // char *solution;
        // if (!solve(size, rows, columns, &solution)) {
        //         printf("No Solution.\n");
        //         exit(EXIT_FAILURE);
        // }

        // for (size_t i = 0; i < size * size; i++) {
        //         printf("%c", solution[i]);
        //         if ((i+1) % size == 0)
        //                 printf("\n");
        // }
        

        // // Cleanup
        // free(solution);
        // free(l1.lst);

        // return 0;