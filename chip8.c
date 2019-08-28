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

#define SDL_WINDOW_NAME "CHIP-8"
#define SDL_WINDOW_WIDTH 640
#define SDL_WINDOW_HEIGHT 480
#define SDL_WINDOW_FLAGS 0
#define SDL_RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define SDL_RENDERER_INDEX -1

#define ESDLINIT 250
#define ESDLWIN 251
#define ESDLRNDR 252

#define EXIT_INVALID_ARGS 1
#define EXIT_INIT_FAILURE 2
#define EXIT_RUN_FAILURE 3

struct chip8_registers {
    uint8_t V[NUM_DATA_REGISTERS];
    uint8_t DT;
    uint8_t ST;
    uint8_t *SP;
    uint8_t *PC;
    uint8_t *I;
};

struct chip8_screen {
    uint8_t *buffer;
    SDL_Window *window;
    SDL_Renderer *renderer;
};

struct chip8_state {
    uint8_t *ram;
    struct chip8_registers *registers;
    struct chip8_screen *screen;
};

int init(struct chip8_state *state, char *filename);
int init_ram(struct chip8_state *state, char *filename);
int init_registers(struct chip8_state *state);
int init_screen(struct chip8_state *state);
void deinit(struct chip8_state *state);
void deinit_screen(struct chip8_state *state);
void deinit_registers(struct chip8_state *state);
void deinit_ram(struct chip8_state *state);
int run(struct chip8_state state);

int main(int argc, char *argv[])
{
    int err;
    struct chip8_state state;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_INVALID_ARGS;
    }

    err = init(&state, argv[1]);
    if (err) {
        return EXIT_INIT_FAILURE;
    }

    err = run(state);
    if (err) {
        deinit(&state);
        return EXIT_RUN_FAILURE;
    }

    deinit(&state);

    return EXIT_SUCCESS;
}

int init(struct chip8_state *state, char *filename)
{
    int err;

    err = init_ram(state, filename);
    if (err) {
        goto error_init_ram;
    }

    err = init_registers(state);
    if (err) {
        goto error_init_registers;
    }

    err = init_screen(state);
    if (err) {
        goto error_init_screen;
    }

    return 0;

error_init_screen:
    deinit_registers(state);
error_init_registers:
    deinit_ram(state);
error_init_ram:
    return err;
}

int init_ram(struct chip8_state *state, char *filename)
{
    int err;
    FILE *file;

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "fopen() failed: %s\n", strerror(errno));
        err = EIO;
        goto error_open_file;
    }

    state->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (state->ram == NULL) {
        fprintf(stderr, "calloc() for Chip-8 RAM failed\n");
        err = ENOMEM;
        goto error_alloc_ram;
    }

    (void)fread(&state->ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, file);
    if (ferror(file) || !feof(file)) {
        fprintf(stderr, "fread() into Chip-8 RAM failed\n");
        err = EIO;
        goto error_read_file;
    }

    fclose(file);

    return 0;

error_read_file:
    free(state->ram);
error_alloc_ram:
    fclose(file);
error_open_file:
    return err;
}

int init_registers(struct chip8_state *state)
{
    state->registers = calloc(1, sizeof(struct chip8_registers));
    if (state->registers == NULL) {
        fprintf(stderr, "calloc() for Chip-8 registers failed\n");
        return ENOMEM;
    }

    state->registers->SP = &state->ram[STACK_START];
    state->registers->PC = &state->ram[PROGRAM_START];

    return 0;
}

int init_screen(struct chip8_state *state)
{
    int err;
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
        err = ESDLINIT;
        goto error_init_sdl;
    }

    window = SDL_CreateWindow(SDL_WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_WINDOW_FLAGS);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        err = ESDLWIN;
        goto error_create_window;
    }

    renderer = SDL_CreateRenderer(window, SDL_RENDERER_INDEX, SDL_RENDERER_FLAGS);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        err = ESDLRNDR;
        goto error_create_renderer;
    }

    state->screen = calloc(1, sizeof(struct chip8_screen));
    if (state->screen == NULL) {
        fprintf(stderr, "calloc() for Chip-8 screen failed\n");
        err = ENOMEM;
        goto error_alloc_screen;
    }

    state->screen->window = window;
    state->screen->renderer = renderer;
    state->screen->buffer = &state->ram[BUFFER_START];

    return 0;

error_alloc_screen:
    SDL_DestroyRenderer(renderer);
error_create_renderer:
    SDL_DestroyWindow(window);
error_create_window:
    SDL_Quit();
error_init_sdl:
    return err;
}

void deinit(struct chip8_state *state)
{
    deinit_screen(state);
    deinit_registers(state);
    deinit_ram(state);
}

void deinit_screen(struct chip8_state *state)
{
    SDL_DestroyRenderer(state->screen->renderer);
    SDL_DestroyWindow(state->screen->window);
    SDL_Quit();
    
    free(state->screen);
    state->screen = NULL;
}

void deinit_registers(struct chip8_state *state)
{
    free(state->registers);
    state->registers = NULL;
}

void deinit_ram(struct chip8_state *state)
{
    free(state->ram);
    state->ram = NULL;
}

int run(struct chip8_state state)
{
    int close_request = 0;
    SDL_Event event;
    
    while (!close_request) {

        // TODO Fetch

        // TODO Decode

        // TODO Execute

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_request = 1;
                break;
            }
        }
    }

    return 0;
}