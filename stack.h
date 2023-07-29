#include <stdint.h>
#include <malloc.h>
#include <string.h>

typedef struct {
    int64_t *values;
    size_t size;
    size_t capacity;
} Stack;

int64_t *stack_push(Stack *s, int64_t val) {
    if (s->size == s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 64;
        s->values = realloc(s->values, s->capacity * sizeof (int64_t));
    }
    int64_t *p = &s->values[s->size++];
    *p = val;
    return p;
}

int64_t *stack_peek(Stack *s) {
    return &s->values[s->size-1];
}

int64_t stack_pop(Stack *s) {
    return s->values[--s->size];
}

Stack stack_clone(Stack *s) {
    Stack c = *s;
    c.values = malloc(c.capacity * sizeof (int64_t));
    memcpy(c.values, s->values, c.size * sizeof (int64_t));
    return c;
}

void stack_destroy(Stack *s) {
    free(s->values);
}
