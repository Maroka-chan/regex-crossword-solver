#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "regex.h"

enum { Split = 256, Match = 257 };
struct State match_state = { .c = Match };

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
        struct State **lst;
        uint32_t size;
};


// Global variables
static uint32_t state_count;
static uint32_t listid = 0;
static struct DFAState *all_dfa_states;
static struct List l1;
// Only upper case characters seem to be necessary to solve the puzzles.
static uint32_t start_char = 65;
static uint32_t end_char = 91;


// Function prototypes
// global
struct DFAState *compile_regexp(char *regexp);
int match(struct DFAState *start, char *s);
int match_at(struct DFAState *start, char c, int pos);

// local
static struct State *infix_to_nfa(char *postfix);
static struct List *create_list(uint32_t length);
static union Ptrlist *create_ptrlist(struct State **outp);
static union Ptrlist *append(union Ptrlist *l1, union Ptrlist *l2);
static void patch(union Ptrlist *lst, struct State *s);
static struct Frag frag(struct State *start, union Ptrlist *out);
static struct State *state(int c, struct State *out, struct State *out1);
static void add_state(struct List *l, struct State *s);
static struct List *start_list(struct State *state, struct List *lst);
static int listcmp(struct List *l1, struct List *l2);
static int ptrcmp(const void *ptr1, const void *ptr2);
static struct DFAState *dfa_state(struct List *l);
static void step(struct List *clist, int c, struct List *nlist);
static struct DFAState *next_state(struct DFAState *d, int c);
static int is_match(struct List *l);
static struct DFAState *start_dfa_state(struct State *start);



struct DFAState *compile_regexp(char *regexp)
{
        struct State *start = infix_to_nfa(regexp);
        
        l1.lst = malloc(state_count * sizeof l1.lst[0]);

        return start_dfa_state(start);
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

        return is_match(d->states);
}

int match_at(struct DFAState *start, char c, int pos)
{
        struct List *states = start->states;
        char accept_char;

        if (pos == 0) {
                struct State *s;
                for (size_t i = 0; i < states->size; i++) {
                        accept_char = states->lst[i]->c;
                        if (accept_char == c)
                                return 1;
                }
                return 0;
        }
                
        for (size_t i = 0; i < states->size; i++) {
                accept_char = states->lst[i]->c;

                if (start->next[accept_char] == NULL)
                        start->next[accept_char] = next_state(start, accept_char);

                if (start->next[accept_char] != NULL) {
                        if (match_at(start->next[accept_char], c, pos - 1))
                                return 1;
                }
        }

        return 0;
}

static struct State *infix_to_nfa(char *postfix)
{
        char *p;
        struct Frag stack[1000], *stackp, e1, e2, e;
        struct State *st;
        uint32_t stack_size = 0;
        stackp = stack;

        struct Frag final_stack[1000], *final_stackp;
        uint32_t final_stack_size = 0;
        final_stackp = final_stack;

        #define push(s) *stackp++ = s; stack_size++
        #define pop()   *--stackp; stack_size--

        #define push_final(s) *final_stackp++ = s; final_stack_size++
        #define pop_final()   *--final_stackp; final_stack_size--

        for (p = postfix; *p; p++) {
                switch (*p) {
                        default:
                        {
                                st = state(*p, NULL, NULL);
                                push(frag(st, create_ptrlist(&st->out)));
                                
                        } break;
                        case '.':
                        {
                                struct State *split_state = state(start_char, NULL, NULL);
                                union Ptrlist *outs = create_ptrlist(&split_state->out);

                                for (size_t i = start_char + 1; i < end_char; i++) {
                                        st = state(i, NULL, NULL);
                                        split_state = state(Split, st, split_state);
                                        outs = append(create_ptrlist(&st->out), outs);
                                }

                                push(frag(split_state, outs));
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
                                push(frag(st, create_ptrlist(&st->out)));

                                if (*(p+1) == '^') {
                                        char group_chars[127] = {'\0'};
                                        p += 2;
                                        for (; *p != ']'; p++)
                                                group_chars[*p] = *p;

                                        for (size_t i = start_char; i < end_char; i++) {
                                                if (group_chars[i] == '\0') {
                                                        st = state(i, NULL, NULL);
                                                        push(frag(st, create_ptrlist(&st->out)));
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
                                union Ptrlist *outs = create_ptrlist(&split_state->out);

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
                                push(frag(st, append(e.out, create_ptrlist(&st->out1))));
                        } break;
                        case '*':
                        {
                                e = pop();
                                st = state(Split, e.start, NULL);
                                patch(e.out, st);
                                push(frag(st, create_ptrlist(&st->out1)));
                        } break;
                        case '+':
                        {
                                e = pop();
                                st = state(Split, e.start, NULL);
                                patch(e.out, st);
                                push(frag(e.start, create_ptrlist(&st->out1)));
                        } break;
                        case '|':
                        {
                                while (stack_size > 1) {        // Concatenate the fragments together.
                                        e2 = pop();
                                        e1 = pop();

                                        patch(e1.out, e2.start);
                                        push(frag(e1.start, e2.out));
                                }

                                e = pop();
                                push_final(e);
                        } break;
                }
        }
        
        while (stack_size > 1) {        // Concatenate the fragments together.
                e2 = pop();
                e1 = pop();

                patch(e1.out, e2.start);
                push(frag(e1.start, e2.out));
        }

        e = pop();
        push_final(e);
        
        while (final_stack_size > 1) {       // Add fragments together with splits.
                e2 = pop_final();
                e1 = pop_final();
                st = state(Split, e1.start, e2.start);
                
                push_final(frag(st, append(e1.out, e2.out)));
        }
        
        e = pop_final();
        patch(e.out, &match_state);
        return e.start;
}

static struct List *create_list(uint32_t length)
{
        struct List *lst = malloc(sizeof(struct List));
        lst->lst = malloc(sizeof(struct State*) * length);
        lst->size = 0;

        return lst;
}

static union Ptrlist *create_ptrlist(struct State **outp)
{
	union Ptrlist *l;
	
	l = (union Ptrlist*)outp;
	l->next = NULL;
	return l;
}

static union Ptrlist *append(union Ptrlist *l1, union Ptrlist *l2)
{
        union Ptrlist *oldl1;
	
	oldl1 = l1;
	while(l1->next)
		l1 = l1->next;
	l1->next = l2;
	return oldl1;
}

static void patch(union Ptrlist *lst, struct State *s)
{
	union Ptrlist *next;
	
	for(; lst; lst=next){
		next = lst->next;
		lst->state = s;
	}
}

static struct Frag frag(struct State *start, union Ptrlist *out)
{
        struct Frag fr = { start, out };
        return fr;
}

static struct State *state(int c, struct State *out, struct State *out1)
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

static void add_state(struct List *state_lst, struct State *s)
{
        if (s == NULL || s->last_list == listid)
                return;
        s->last_list = listid;
        if (s->c == Split) {
                add_state(state_lst, s->out);
                add_state(state_lst, s->out1);
                return;
        }
        state_lst->lst[state_lst->size++] = s;
}

static struct List *start_list(struct State *state, struct List *state_lst)
{
        state_lst->size = 0;
        listid++;
        add_state(state_lst, state);
        return state_lst;
}

static int listcmp(struct List *l1, struct List *l2)
{
	int i;

	if(l1->size < l2->size)
		return -1;
	if(l1->size > l2->size)
		return 1;
	for(i=0; i < l1->size; i++)
		if(l1->lst[i] < l2->lst[i])
			return -1;
		else if(l1->lst[i] > l2->lst[i])
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

static struct DFAState *dfa_state(struct List *states)
{
	int i;
	struct DFAState **dp, *dfa;

	qsort(states->lst, states->size, sizeof states->lst[0], ptrcmp);

	/* look in tree for existing DFAState */
	dp = &all_dfa_states;
	while((dfa = *dp) != NULL){
		i = listcmp(states, dfa->states);
		if(i < 0)
			dp = &dfa->left;
		else if(i > 0)
			dp = &dfa->right;
		else
			return dfa;
	}
	
	/* allocate, initialize new DFAState */
	dfa = malloc(sizeof *dfa);
        memset(dfa, 0, sizeof *dfa);
        dfa->states = create_list(states->size);
        memmove(dfa->states->lst, states->lst, states->size * sizeof states->lst[0]);
        dfa->states->size = states->size;

	/* insert in tree */
	*dp = dfa;
	return dfa;
}

static void step(struct List *current_states, int c, struct List *next_states)
{
        struct State *s;

        listid++;
        next_states->size = 0;
        for (size_t i = 0; i < current_states->size; i++) {
                s = current_states->lst[i];
                if (s->c == c)
                        add_state(next_states, s->out);
        }
}

static struct DFAState *next_state(struct DFAState *dfa, int c)
{
	step(dfa->states, c, &l1);
	return dfa->next[c] = dfa_state(&l1);
}

static int is_match(struct List *states)
{
        // Check that states contains match_state
        for (size_t i = 0; i < states->size; i++) {
                if (states->lst[i] == &match_state)
                        return 1;
        }
        return 0;
}

static struct DFAState *start_dfa_state(struct State *start)
{
	return dfa_state(start_list(start, &l1));
}