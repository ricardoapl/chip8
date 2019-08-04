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
void run(struct chip8_state state);
void terminate(struct chip8_state *state);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Missing parameter program filename.\n");
        return EXIT_FAILURE;
    }

    struct chip8_state state;
    init(&state, argv[1]);

    run(state);

    terminate(&state);

    return EXIT_SUCCESS;
}

void init(struct chip8_state *state, char *filename)
{
    // XXX init ram
    FILE *program_file = fopen(filename, "r");
    if (program_file == NULL) {
        fprintf(stderr, "[ERROR] Unable to open program file %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    uint8_t *ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (ram == NULL) {
        fprintf(stderr, "[ERROR] Unable to allocate Chip-8 RAM!\n");
        exit(EXIT_FAILURE);
    }

    (void)fread(&ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, program_file);
    if (ferror(program_file) || !feof(program_file)) {
        fprintf(stderr, "[ERROR] Unable to load program into Chip-8 RAM!\n");
        free(ram);
        exit(EXIT_FAILURE);
    }

    fclose(program_file);

    // XXX init registers
    struct chip8_registers *registers = calloc(1, sizeof(struct chip8_registers));
    if (registers == NULL) {
        fprintf(stderr, "[ERROR] Unable o allocate Chip-8 registers!\n");
        free(ram);
        exit(EXIT_FAILURE);
    }

    registers->SP = &ram[STACK_START];
    registers->PC = &ram[PROGRAM_START];

    // XXX init display
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "[ERROR] Unable to initialize SDL subsystems: %s\n", SDL_GetError());
        free(registers);
        free(ram);
        exit(EXIT_FAILURE);
    }
    
    SDL_Window *window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_FLAGS);
    if (window == NULL) {
        fprintf(stderr, "[ERROR] Unable to create SDL window: %s\n", SDL_GetError());
        free(registers);
        free(ram);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, SDL_RENDERER_INDEX, SDL_RENDERER_FLAGS);
    if (renderer == NULL) {
        fprintf(stderr, "[ERROR] Unable to create SDL window and renderer: %s\n", SDL_GetError());
        free(registers);
        free(ram);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    struct chip8_display *display = calloc(1, sizeof(struct chip8_display));
    if (display == NULL) {
        fprintf(stderr, "[ERROR] Unable to allocate Chip-8 display!\n");
        free(registers);
        free(ram);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    display->buffer = &ram[BUFFER_START];
    display->window = window;
    display->renderer = renderer;
    
    state->ram = ram;
    state->registers = registers;
    state->display = display;
}

void run(struct chip8_state state)
{
    SDL_Event event;
    int close_request = 0;
    while (!close_request) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_request = 1;
            }
        }

        SDL_Delay(500);
        // TODO Fetch opcode
        // TODO Decode opcode -> instruction
        // TODO Execute instruction
    }
}

void terminate(struct chip8_state *state)
{
    SDL_DestroyRenderer(state->display->renderer);
    SDL_DestroyWindow(state->display->window);
    free(state->display);

    free(state->registers);

    free(state->ram);

    SDL_Quit();
}