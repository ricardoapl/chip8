#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <SDL.h>

#define RAM_SIZE 4096
#define PROGRAM_START 0x200
#define PROGRAM_END 0xEDE
#define STACK_START 0xEFE
#define BUFFER_START 0xF00
#define NUM_DATA_REGISTERS 16
// XXX SDL specific
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define SDL_WINDOW_FLAGS 0
#define SDL_RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define SDL_RENDERER_INDEX -1

struct chip8_registers {
    uint8_t V[NUM_DATA_REGISTERS];
    uint8_t DT;
    uint8_t ST;
    uint8_t *SP;
    uint8_t *PC;
    uint8_t *I;
};

struct chip8_display {
    uint8_t *buffer;
    SDL_Window *window;
    SDL_Renderer *renderer;
};

struct chip8_state {
    uint8_t *ram;
    struct chip8_registers *registers;
    struct chip8_display *display;
};

void init(struct chip8_state *state, char *filename);
int init_ram(struct chip8_state *state, char *filename);
int init_registers(struct chip8_state *state);
int init_display(struct chip8_state *state);
void run(struct chip8_state state);
void terminate(struct chip8_state *state);

int main(int argc, char *argv[])
{
    struct chip8_state state;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    init(&state, argv[1]);
    run(state);
    terminate(&state);

    return EXIT_SUCCESS;
}

// XXX exit/return errno-like constants?
// XXX change return type to 'int' and let main handle exit()/return?
void init(struct chip8_state *state, char *filename)
{
    if (init_ram(state, filename) < 0) {
        exit(EXIT_FAILURE);
    }

    if (init_registers(state) < 0) {
        free(state->ram);
        exit(EXIT_FAILURE);
    }

    if (init_display(state) < 0) {
        free(state->registers);
        free(state->ram);
        exit(EXIT_FAILURE);
    }
}

int init_ram(struct chip8_state *state, char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to open '%s': %s\n", filename, strerror(errno));
        return -EIO;
    }

    state->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (state->ram == NULL) {
        fprintf(stderr, "Unable to allocate Chip-8 RAM!\n");
        return -ENOMEM;
    }

    (void)fread(&state->ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, file);
    if (ferror(file) || !feof(file)) {
        fprintf(stderr, "Unable to load file into Chip-8 RAM!\n");
        free(state->ram);
        return -EIO;
    }

    fclose(file);

    return 0;
}

int init_registers(struct chip8_state *state)
{
    state->registers = calloc(1, sizeof(struct chip8_registers));
    if (state->registers == NULL) {
        fprintf(stderr, "Unable to allocate Chip-8 registers!\n");
        return -ENOMEM;
    }

    state->registers->SP = &state->ram[STACK_START];
    state->registers->PC = &state->ram[PROGRAM_START];

    return 0;
}

// XXX return errno-like constants?
// XXX declare window and renderer in the beginning
int init_display(struct chip8_state *state)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to initialize SDL subsystems: %s\n", SDL_GetError());
        return -1;
    }

    // TODO make window title a constant
    SDL_Window *window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FLAGS);
    if (window == NULL) {
        fprintf(stderr, "Unable to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, SDL_RENDERER_INDEX, SDL_RENDERER_FLAGS);
    if (renderer == NULL) {
        fprintf(stderr, "Unable to create SDL renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    state->display = calloc(1, sizeof(struct chip8_display));
    if (state->display == NULL) {
        fprintf(stderr, "Unable to allocate Chip-8 display!\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -ENOMEM;
    }

    state->display->window = window;
    state->display->renderer = renderer;
    state->display->buffer = &state->ram[BUFFER_START];

    return 0;
}

// XXX shouldn't 'state' be '*state' to update register->SP etc.?
void run(struct chip8_state state)
{
    int close_request = 0;

    while (!close_request) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_request = 1;
            }
        }

        SDL_Delay(500);

        // TODO fetch opcode
        // TODO decode opcode into instruction
        // TODO execute instruction
    }
}

void terminate(struct chip8_state *state)
{
    SDL_DestroyRenderer(state->display->renderer);
    SDL_DestroyWindow(state->display->window);
    SDL_Quit();

    free(state->display);
    free(state->registers);
    free(state->ram);
}