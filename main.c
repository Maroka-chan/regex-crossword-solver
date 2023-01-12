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
        struct Ptrlist *out;
};

struct Ptrlist {
        struct State ***lst;
        uint32_t size;
};

struct List {
        struct State **lst;
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


struct Ptrlist *list1(struct State **outp)
{
        struct Ptrlist *ptr_lst = malloc(sizeof(struct Ptrlist));
        ptr_lst->lst = malloc(sizeof(struct State**));
        ptr_lst->lst[0] = outp;
        ptr_lst->size = 1;
        return ptr_lst;
}

struct Ptrlist *append(struct Ptrlist *l1, struct Ptrlist *l2)
{
        uint32_t size = l1->size + l2->size;

        struct Ptrlist *ptr_lst = malloc(sizeof(struct Ptrlist));
        ptr_lst->lst = malloc(size * sizeof(struct State**));
        ptr_lst->size = size;

        for (size_t i = 0; i < l1->size; i++)
                ptr_lst->lst[i] = l1->lst[i];
        for (size_t i = 0; i < l2->size; i++)
                ptr_lst->lst[l1->size + i] = l2->lst[i];

        return ptr_lst;
}

void patch(struct Ptrlist *lst, struct State *s)
{
        for (size_t i = 0; i < lst->size; i++) {
                if (*lst->lst[i] == NULL)
                        *lst->lst[i] = s;
        }
}

struct Frag *frag(struct State *st, struct Ptrlist *lst)
{
        struct Frag *fr = malloc(sizeof(struct Frag));
        fr->start = st;
        fr->out = lst;

        return fr;
}

struct State *state(int c, struct State *out, struct State *out1)
{
        struct State *st = malloc(sizeof(struct State));
        st->c = c;
        st->out = out;
        st->out1 = out1;
        st->last_list = 0;

        return st;
}

static int Split = 256;
static struct State Match = {.c = 257, NULL, NULL};

struct State *infix_to_nfa(char *postfix)
{
        char *p;
        struct Frag stack[1000], *stackp, e1, e2, e;
        uint32_t stack_size = 0;
        bool alternate_index[1000] = {false};

        #define push(s) *stackp++ = s; stack_size++
        #define pop()   *--stackp; stack_size--

        stackp = stack;
        for (p = postfix; *p; p++) {
                switch (*p) {
                        default:
                        {
                                struct State *st = state(*p, NULL, NULL);
                                struct Ptrlist *lst = list1(&st->out);
                                struct Frag *fr = frag(st, lst);
                                push(*fr);
                                
                        } break;
                        case ')':
                        {
                                struct Frag end = pop();
                                struct Frag start = pop();
                                struct Frag prev = end;
                                while (start.start->c != '(') {
                                        patch(start.out, prev.start);
                                        prev = start;
                                        start = pop();
                                }
                                push(*frag(prev.start, end.out));
                        } break;
                        case '[':
                        {
                                struct State *st = state(*p, NULL, NULL);
                                struct Ptrlist *lst = list1(&st->out);
                                struct Frag *fr = frag(st, lst);
                                push(*fr);

                                if (*(p+1) == '^') {
                                        char group_chars[127] = {'\0'};
                                        p += 2;
                                        for (; *p != ']'; p++)
                                                group_chars[*p] = *p;

                                        for (size_t i = 33; i < 127; i++) {
                                                if (i == '[' || i == ']' || i == '^' || i == '|' || i == '(' || i == ')' || i == '*' || i == '+' || i == '?' || i == '.')
                                                        continue;
                                                if (group_chars[i] == '\0') {
                                                        struct State *st = state(i, NULL, NULL);
                                                        struct Ptrlist *lst = list1(&st->out);
                                                        struct Frag *fr = frag(st, lst);
                                                        push(*fr);
                                                }
                                        }
                                }
                                p--;
                        } break;
                        case ']':
                        {
                                e2 = pop();
                                e1 = pop();
                                struct State *st = e2.start;
                                struct Ptrlist *outs = list1(&st->out);

                                if (e2.start->c == '[') {
                                        fprintf(stderr, "Cannot have empty brackets. Invalid regex.");
                                        exit(EXIT_FAILURE);
                                }

                                struct Ptrlist *lst;
                                while (e1.start->c != '[') {
                                        st = state(Split, e1.start, st);
                                        lst = outs;
                                        outs = append(e1.out, outs);
                                        free(lst->lst);
                                        free(lst);
                                        free(e1.out->lst);
                                        free(e1.out);
                                        e1 = pop();
                                }
                                push(*frag(st, outs));
                        } break;
                        case '|':
                        {
                                alternate_index[stack_size+1] = true;
                        } break;
                        case '?':
                        {
                                e = pop();
                                struct State *st = state(Split, e.start, NULL);
                                struct Ptrlist *lst = list1(&st->out1);
                                push(*frag(st, append(e.out, lst)));
                        } break;
                        case '*':
                        {
                                e = pop();
                                struct State *st = state(Split, e.start, NULL);
                                patch(e.out, st);
                                struct Ptrlist *lst = list1(&st->out1);
                                push(*frag(st, lst));
                        } break;
                        case '+':
                        {
                                e = pop();
                                struct State *split_state = state(Split, e.start, NULL);
                                patch(e.out, split_state);
                                struct Ptrlist *lst = list1(&split_state->out1);
                                push(*frag(e.start, lst));
                        } break;
                }
        }

        while (stack_size > 1) {
                if (alternate_index[stack_size]) {
                        e2 = pop();
                        e1 = pop();
                        struct State *st = state(Split, e1.start, e2.start);
                        push(*frag(st, append(e1.out, e2.out)));
                        continue;
                }
                e2 = pop();
                e1 = pop();
                patch(e1.out, e2.start);
                push(*frag(e1.start, e2.out));
        }

        e = pop();
        patch(e.out, &Match);
        return e.start;
}

int is_match(struct List *l)
{
        for (size_t i = 0; i < l->size; i++) {
                if (l->lst[i] == &Match)
                        return 1;
        }
        return 0;
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
        l->lst[l->size++] = s;
}

struct List *start_list(struct State *state, struct List *lst)
{
        listid++;
        lst->size = 0;
        add_state(lst, state);
        return lst;
}

void step(struct List *clist, int c, struct List *nlist)
{
        struct State *s;

        listid++;
        nlist->size = 0;
        for (size_t i = 0; i < clist->size; i++) {
                s = clist->lst[i];
                if (s->c == c)
                        add_state(nlist, s->out);
        }
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

struct DFAState *all_dfa_states;
struct DFAState *dfa_state(struct List *l)
{
	int i;
	struct DFAState **dp, *d;

	qsort(l->lst, l->size, sizeof l->lst[0], ptrcmp);

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
	d = malloc(sizeof *d + l->size * sizeof l->lst[0]);
	memset(d, 0, sizeof *d);
	d->lst.lst = (struct State**)(d+1);
	memmove(d->lst.lst, l->lst, l->size * sizeof l->lst[0]);
	d->lst.size = l->size;

	/* insert in tree */
	*dp = d;
	return d;
}

static struct List l1;

struct DFAState *next_state(struct DFAState *d, int c)
{
	step(&d->lst, c, &l1);
	return d->next[c] = dfa_state(&l1);
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

int match_at(struct State *start, char *s, uint32_t pos)
{
        struct State *state = start;

        for (size_t j = 0; j < pos; j++) {
                if (start->c == Split) {
                        if (match_at(start->out, s, pos))
                                return 1;
                        if (match_at(start->out1, s, pos))
                                return 1;
                        return 0;
                }
                state = state->out;
        }
                

        if (state->c == *s)
                return 1;
        
        return 0;
}

//int match_at(struct DFAState *start, char *s, uint32_t pos)
//{
//        for (size_t i = 0; i < start->lst.size; i++) {
//                struct State *state = start->lst.lst[i];
//
//                for (size_t j = 0; j < pos; j++)
//                        state = state->out;
//
//                if (state->c == *s)
//                        return 1;
//        }
//        
//        return 0;
//}

struct DFAState *start_dfa_state(struct State *start)
{
	return dfa_state(start_list(start, &l1));
}

// Split in group (e|f) does not work. I think it reads '|' as a regular character
int main (int argc, char *argv[])
{
        // int size;
        // scanf("%d", &size);

        // char *rows[size];
        // for (int i = 0; i < size; i++)
        //         scanf("%s", rows[i]);

        // char *columns[size];
        // for (int i = 0; i < size; i++)
        //         scanf("%s", columns[i]);


        l1.lst = malloc(64 * sizeof(struct State*));

        struct State *nfa = infix_to_nfa("[^PLEASE]+");
        struct DFAState *dfa = start_dfa_state(nfa);

        printf("%d\n", match(dfa, "VKJHKJHIUY"));

        //printf("%d\n", match_at(nfa, "f", 1));

        free(l1.lst);

        return 0;
}
