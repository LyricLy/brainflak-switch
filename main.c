#define PROGRAM_SIZE 500
#define INPUT_SIZE 10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <switch.h>
#include "stack.h"
#include "interpreter.h"

#define error(s, kind) do { strncpy(string, (s), string_size); return SwkbdTextCheckResult_##kind; } while (0)

SwkbdTextCheckResult is_balanced(char* string, size_t string_size) {
    char stack[500];
    size_t height = 0;
    bool non_bracket = false;

    for (size_t i = 0; string[i]; i++) {
        char c = string[i];
        switch (string[i]) {
            case '<': case '{': case '[': case '(':
                stack[height++] = c;
                break;

            #define ensure(x) if (!height) error("Unexpected close bracket.", Bad); else if (stack[--height] != (x)) error("Brackets are mismatched.", Bad); else {}
            case '>': ensure('<'); break;
            case ']': ensure('['); break;
            case ')': ensure('('); break;
            case '}': ensure('{'); break;

            case ' ': case '\n': break;
            default: non_bracket = true;
        } 
    }

    if (height) error("Unclosed bracket.", Bad);
    if (non_bracket) error("Some characters are not brackets and will be ignored.", Prompt);

    return SwkbdTextCheckResult_OK;
}

void query_program(char *program) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (!R_SUCCEEDED(rc)) return;
    swkbdConfigMakePresetDefault(&kbd);

    swkbdConfigSetInitialText(&kbd, program);
    swkbdConfigSetGuideText(&kbd, "Program (sorry, it only lets me do 500 characters...)");
    swkbdConfigSetTextCheckCallback(&kbd, is_balanced);

    char out[PROGRAM_SIZE+1] = {0};
    rc = swkbdShow(&kbd, out, sizeof out);

    if (R_SUCCEEDED(rc)) {
        strcpy(program, out);
    }

    swkbdClose(&kbd);
}

void query_input(int64_t *n) {
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (!R_SUCCEEDED(rc)) return;
    swkbdConfigMakePresetDefault(&kbd);

    char initial[INPUT_SIZE+1];
    snprintf(initial, sizeof initial, "%" PRId64, *n);
    swkbdConfigSetInitialText(&kbd, initial);

    swkbdConfigSetType(&kbd, SwkbdType_NumPad);
    swkbdConfigSetHeaderText(&kbd, "Enter an input value.");
    swkbdConfigSetStringLenMax(&kbd, INPUT_SIZE);
    swkbdConfigSetTextDrawType(&kbd, SwkbdTextDrawType_Line);

    char out[INPUT_SIZE+1];
    rc = swkbdShow(&kbd, out, sizeof out);

    if (R_SUCCEEDED(rc) && *out) {
        *n = atoll(out);
    }

    swkbdClose(&kbd);
}

typedef enum {
    Input,
    Program,
    Execute,
} Selected;

typedef struct {
    Stack input;
    Stack output;
    size_t selected_input;
    Selected selected;
    char program[PROGRAM_SIZE+1];
} InputState;

void clear(void) {
    printf("\033[2J");
}

void cyan(void) {
    printf("\033[36;1m");
}

void green(void) {
    printf("\033[32;1m");
}

void reset(void) {
    printf("\033[0m");
}

void print_state(InputState *state) {
    clear();
    printf("Move to select\nA to edit value\nY to push input\nX to pop input\n+ to exit\n\nInputs: ");

    for (size_t i = 0; i < state->input.size; i++) {
        if (state->selected == Input && i == state->selected_input) cyan();
        printf("%" PRId64 " ", state->input.values[i]);
        reset();
    }

    if (state->selected == Program) cyan();
    printf("\nProgram:\n%s\n\n", state->program);
    reset();

    if (state->selected == Execute) cyan();
    printf("Execute\n");
    reset();

    green();
    for (size_t i = 0; i < state->output.size; i++) {
        printf("%" PRId64 " ", state->output.values[i]);
    }
    reset();

    consoleUpdate(NULL);
}

void correct_selected(InputState *state) {
    consoleUpdate(NULL);
    Selected min = state->input.size ? Input : Program;
    Selected max = Execute;
    if (state->selected < min) state->selected = min;
    if (state->selected > max) state->selected = max;
}

int main(void) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    InputState state = {.selected = Program};
    print_state(&state);

    while (appletMainLoop()) {
        padUpdate(&pad);

        u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus) break;
        if (down & HidNpadButton_A) {
            switch (state.selected) {
                case Input:
                    query_input(&state.input.values[state.selected_input]);
                    break;
                case Program:
                    query_program(state.program);
                    break;
                case Execute:
                    stack_destroy(&state.output);
                    state.output = execute(stack_clone(&state.input), state.program);
                    break;
            }
        }
        if (down & HidNpadButton_Y) stack_push(&state.input, 0);
        if (down & HidNpadButton_X && state.input.size) stack_pop(&state.input);
        if (down & HidNpadButton_AnyUp) {
            state.selected--;
            correct_selected(&state);
        }
        if (down & HidNpadButton_AnyDown) {
            state.selected++;
            correct_selected(&state);
        }
        if (state.selected == Input) {
            if (down & HidNpadButton_AnyLeft && state.selected_input) state.selected_input--;
            if (down & HidNpadButton_AnyRight) state.selected_input++;
            if (state.selected_input >= state.input.size) state.selected_input = state.input.size-1;
        }
        if (down) print_state(&state);
    }

    stack_destroy(&state.input);
    stack_destroy(&state.output);
    consoleExit(NULL);
}
