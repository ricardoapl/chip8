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
#define NUM_THREADS 2

#define SDL_WINDOW_WIDTH 640
#define SDL_WINDOW_HEIGHT 480
#define SDL_WINDOW_FLAGS 0
#define SDL_RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define SDL_RENDERER_INDEX -1

#define ESDL 255
#define EMUTEX 254
#define ETHREAD 253

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

struct chip8_thread_params {
    struct chip8_state state;
    SDL_mutex *mutex;
    int cond;
};

int init(struct chip8_state *state, char *filename);
int init_ram(struct chip8_state *state, char *filename);
int init_registers(struct chip8_state *state);
int init_screen(struct chip8_state *state);
void deinit(struct chip8_state state);
int run(struct chip8_state state);
int run_cpu(void *data);
int run_io(void *data);

int main(int argc, char *argv[])
{
    int err;
    struct chip8_state state;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    err = init(&state, argv[1]);
    if (err) {
        return EXIT_FAILURE;
    }

    err = run(state);
    if (err) {
        deinit(state);
        return EXIT_FAILURE;
    }
    
    deinit(state);

    return EXIT_SUCCESS;
}

int init(struct chip8_state *state, char *filename)
{
    int err;

    err = init_ram(state, filename);
    if (err) {
        return err;
    }

    err = init_registers(state);
    if (err) {
        free(state->ram);
        return err;
    }

    err = init_screen(state);
    if (err) {
        free(state->registers);
        free(state->ram);
        return err;
    }

    return 0;
}

// XXX call fclose() on failure
int init_ram(struct chip8_state *state, char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "fopen() failed: %s\n", filename, strerror(errno));
        return EIO;
    }

    state->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (state->ram == NULL) {
        fprintf(stderr, "calloc() for Chip-8 RAM failed\n");
        return ENOMEM;
    }

    (void)fread(&state->ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, file);
    if (ferror(file) || !feof(file)) {
        fprintf(stderr, "fread() into Chip-8 RAM failed\n");
        free(state->ram);
        return EIO;
    }

    fclose(file);

    return 0;
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
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
        return ESDL;
    }

    window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_WINDOW_FLAGS);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        SDL_Quit();
        return ESDL;
    }

    renderer = SDL_CreateRenderer(window, SDL_RENDERER_INDEX, SDL_RENDERER_FLAGS);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return ESDL;
    }

    state->screen = calloc(1, sizeof(struct chip8_screen));
    if (state->screen == NULL) {
        fprintf(stderr, "calloc() for Chip-8 screen failed\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return ENOMEM;
    }

    state->screen->window = window;
    state->screen->renderer = renderer;
    state->screen->buffer = &state->ram[BUFFER_START];

    return 0;
}

void deinit(struct chip8_state state)
{
    SDL_DestroyRenderer(state.screen->renderer);
    SDL_DestroyWindow(state.screen->window);
    SDL_Quit();

    free(state.screen);

    free(state.registers);

    free(state.ram);
}

int run(struct chip8_state state)
{
    SDL_Thread *threads[NUM_THREADS];
    struct chip8_thread_params params;

    params.cond = 0;
    params.state = state;
    params.mutex = SDL_CreateMutex();
    if (params.mutex == NULL) {
        fprintf(stderr, "SDL_CreateMutex() failed: %s\n", SDL_GetError());
        return EMUTEX;
    }
    
    threads[0] = SDL_CreateThread(run_cpu, "CHIP-8 CPU", (void *)&params);
    if (threads[0] == NULL) {
        fprintf(stderr, "SDL_CreateThread() failed: %s\n", SDL_GetError());
        return ETHREAD;
    }

    threads[1] = SDL_CreateThread(run_io, "CHIP-8 IO", (void *)&params);
    if (threads[1] == NULL) {
        fprintf(stderr, "SDL_CreateThread() failed: %s\n", SDL_GetError());
        return ETHREAD;
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        SDL_WaitThread(threads[i], NULL);
    }

    SDL_DestroyMutex(params.mutex);
}

int run_cpu(void *data)
{
    struct chip8_thread_params *params = data;

    // TODO fetch
    // TODO decode
    // TODO execute
    
    return 0;
}

// TODO check if deinit() is worth calling
int run_io(void *data)
{
    struct chip8_thread_params *params = data;
    SDL_Event event;

    while (1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                if (SDL_LockMutex(params->mutex) < 0) {
                    fprintf(stderr, "SDL_LockMutex() failed: %s\n", SDL_GetError());
                    exit(ENOLCK);
                }
                params->cond = 1;
                if (SDL_UnlockMutex(params->mutex) < 0) {
                    fprintf(stderr, "SDL_UnlockMutex() failed: %s\n", SDL_GetError());
                    exit(ENOLCK);
                }
                return 0;
            }
        }
    }
}