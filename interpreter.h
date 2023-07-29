typedef struct {
    enum {
        One,
        Height,
        Pop,
        Toggle,
        Open,
        Push,
        Negate,
        Drop,
        LoopOpen,
        LoopEnd,
    } type;
    size_t target;
} Token;

#define token(x) ((Token) {.type = (x)})

typedef struct {
    Token tokens[PROGRAM_SIZE];
    size_t tokens_size;
} Tokens;

Tokens lex(char *program) {
    Tokens p;
    p.tokens_size = 0;
    #define push_token(x) do { p.tokens[p.tokens_size++] = (x); } while (0)

    size_t open_brackets[PROGRAM_SIZE];
    size_t open_brackets_size = 0;

    for (size_t i = 0; program[i]; i++) {
        switch (program[i]) {
            case '(': case '[': case '<':
                push_token(token(Open));
                break;
            #define match_last(x, a, b) if (p.tokens_size && p.tokens[p.tokens_size-1].type == Open) p.tokens[p.tokens_size-1] = token(a); else push_token(token(b));
            case ')': match_last('(', One, Push); break;
            case ']': match_last('[', Height, Negate); break;
            case '>': match_last('<', Toggle, Drop); break;
            case '{':
                open_brackets[open_brackets_size++] = p.tokens_size;
                push_token(token(LoopOpen));
                break;
            case '}':
                open_brackets_size--;
                if (p.tokens_size && p.tokens[p.tokens_size-1].type == LoopOpen) {
                    p.tokens[p.tokens_size-1] = token(Pop);
                } else {
                    size_t idx = open_brackets[open_brackets_size];
                    p.tokens[idx].target = p.tokens_size;
                    push_token(((Token) {.type = LoopEnd, .target = idx}));
                }
                break;
        }
    }

    return p;
}

Stack execute(Stack active, char *program) {
    Tokens p = lex(program);
    Stack inactive = {0};
    Stack third = {0};
    stack_push(&third, 0);

    for (size_t i = 0; i < p.tokens_size; i++) {
        Token t = p.tokens[i];
        switch (t.type) {
            case One:
                (*stack_peek(&third))++;
                break;
            case Height:
                *stack_peek(&third) += active.size;
                break;
            case Pop:
                *stack_peek(&third) += active.size ? stack_pop(&active) : 0;
                break;
            case Toggle:
                Stack tmp = active;
                active = inactive;
                inactive = tmp;
                break;
            case Open:
                stack_push(&third, 0);
                break;
            case Push: {
                int64_t x = stack_pop(&third);
                *stack_peek(&third) += x;
                stack_push(&active, x);
                break;
            }
            case Negate: {
                int64_t x = stack_pop(&third);
                *stack_peek(&third) -= x;
                break;
            }
            case Drop:
                stack_pop(&third);
                break;
            case LoopOpen:
                if (!active.size || !*stack_peek(&active)) i = t.target;
                break;
            case LoopEnd:
                if (active.size && *stack_peek(&active)) i = t.target;
                break;
        }
    }

    stack_destroy(&inactive);
    stack_destroy(&third);
    return active;
}
