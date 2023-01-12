#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct Stack
{
        char *sp;
        uint32_t size;
};

struct Stack *st_init(uint32_t length)
{
        struct Stack *stack = malloc(sizeof(struct Stack));

        if (!stack)
                return NULL;

        stack->sp = malloc(length);
        stack->size = 0;

        return stack;
}

void st_destroy(struct Stack *stack)
{
        free(stack->sp);
        free(stack);
}

void st_push(char c, struct Stack *stack)
{
        stack->sp[stack->size] = c;
        stack->size++;
}

char st_pop(struct Stack *stack)
{
        stack->size--;
        return stack->sp[stack->size];
}

char st_peek(struct Stack *stack)
{
        return stack->sp[stack->size - 1];
}

bool st_empty(struct Stack *stack)
{
        return stack->size == 0;
}
