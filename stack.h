#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Stack Stack;

Stack *st_init(uint32_t length);
void st_destroy(Stack *stack);

void st_push(char c, Stack *stack);
char st_pop(Stack *stack);
char st_peek(Stack *stack);
bool st_empty(Stack *stack);

#endif
