#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <SDL.h>

#define RAM_SIZE 4096
#define PROGRAM_START 0x200
#define PROGRAM_END 0xFFF
#define STACK_SIZE 16
#define STACK_START 15
#define SCREEN_BUFFER_WIDTH 64
#define SCREEN_BUFFER_HEIGHT 32
#define NUM_V_REGISTERS 16

#define OPCODE_SIZE 2
#define MS_1BITS(byte) ((byte >> 7) & 0x01)
#define LS_1BITS(byte) (byte & 0x1)
#define MS_4BITS(byte) ((byte >> 4) & 0x0F)
#define LS_4BITS(byte) (byte & 0x0F)
#define MS_8BITS(bytes) (bytes[0])
#define LS_8BITS(bytes) (bytes[1])
#define LS_12BITS(bytes) ((LS_4BITS(MS_8BITS(bytes)) << 8) | LS_8BITS(bytes))

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
    uint8_t V[NUM_V_REGISTERS];
    uint8_t DT;
    uint8_t ST;
    uint8_t **SP;
    uint8_t *PC;
    uint8_t *I;
};

struct chip8_screen {
    uint8_t buffer[SCREEN_BUFFER_WIDTH * SCREEN_BUFFER_HEIGHT];
    SDL_Window *window;
    SDL_Renderer *renderer;
};

struct chip8_state {
    uint8_t *ram;
    uint8_t **stack;
    struct chip8_registers *registers;
    struct chip8_screen *screen;
};

int init(struct chip8_state *state, char *filename);
int init_memory(struct chip8_state *state, char *filename);
int init_registers(struct chip8_state *state);
int init_screen(struct chip8_state *state);
void deinit(struct chip8_state *state);
void deinit_screen(struct chip8_state *state);
void deinit_registers(struct chip8_state *state);
void deinit_memory(struct chip8_state *state);
int run(struct chip8_state *state);
int fetch_decode_execute(struct chip8_state *state);
int screen_clear(struct chip8_screen *screen);
int screen_draw(struct chip8_screen *screen, uint8_t x, uint8_t y, size_t sz, uint8_t *start);

// TODO Fix code style (duplicate deinit() call)
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

    err = run(&state);
    if (err) {
        deinit(&state);
        return EXIT_RUN_FAILURE;
    }

    deinit(&state);

    return EXIT_SUCCESS;
}

// TODO Call srand() to seed RNG used by rand()
int init(struct chip8_state *state, char *filename)
{
    int err;

    err = init_memory(state, filename);
    if (err) {
        goto error_init_memory;
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
    deinit_memory(state);
error_init_memory:
    return err;
}

int init_memory(struct chip8_state *state, char *filename)
{
    int err;
    FILE *file;

    state->stack = calloc(STACK_SIZE, sizeof(uint8_t *));
    if (state->stack == NULL) {
        fprintf(stderr, "calloc() for Chip-8 STACK failed\n");
        err = ENOMEM;
        goto error_alloc_stack;
    }

    state->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (state->ram == NULL) {
        fprintf(stderr, "calloc() for Chip-8 RAM failed\n");
        err = ENOMEM;
        goto error_alloc_ram;
    }

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "fopen() failed: %s\n", strerror(errno));
        err = EIO;
        goto error_open_file;
    }

    fread(&state->ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START + 1, file);
    if (ferror(file) || !feof(file)) {
        fprintf(stderr, "fread() into Chip-8 RAM failed\n");
        err = EIO;
        goto error_read_file;
    }

    fclose(file);

    return 0;

error_read_file:
    fclose(file);
error_open_file:
    free(state->ram);
error_alloc_ram:
    free(state->stack);
error_alloc_stack:
    return err;
}

int init_registers(struct chip8_state *state)
{
    state->registers = calloc(1, sizeof(struct chip8_registers));
    if (state->registers == NULL) {
        fprintf(stderr, "calloc() for Chip-8 registers failed\n");
        return ENOMEM;
    }

    state->registers->SP = &state->stack[STACK_START];
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
    deinit_memory(state);
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

void deinit_memory(struct chip8_state *state)
{
    free(state->stack);
    state->stack = NULL;    

    free(state->ram);
    state->ram = NULL;
}

// TODO Check return value of fetch_decode_execute(), and return error if necessary
int run(struct chip8_state *state)
{
    int close_request = 0;
    SDL_Event event;
    
    while (!close_request) {

        fetch_decode_execute(state);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                close_request = 1;
                break;
            }
        }
    }

    return 0;
}


// TODO Implement missing cases 0x8, 0xE and 0xF
// TODO Check return value for cases 0x00E0 and 0xD, and return error if necessary
int fetch_decode_execute(struct chip8_state *state)
{
    uint8_t opcode[OPCODE_SIZE];
    uint8_t x, y, n, nn, random, offset;
    uint16_t nnn;
    
    memcpy(opcode, state->registers->PC, OPCODE_SIZE);
    state->registers->PC += OPCODE_SIZE;

    x = LS_4BITS(MS_8BITS(opcode));
    y = MS_4BITS(LS_8BITS(opcode));
    n = LS_4BITS(LS_8BITS(opcode));
    nn = LS_8BITS(opcode);
    nnn = LS_12BITS(opcode);

    switch (MS_4BITS(MS_8BITS(opcode))) {
    case 0x0:
        switch (LS_8BITS(opcode)) {
        case 0xE0: // 0x00E0
            screen_clear(state->screen);
            break;
        case 0xEE: // 0x00EE
            state->registers->SP++;
            state->registers->PC = *state->registers->SP;
            break;
        default:
            break;
        }
        break;

    case 0x1: // 1nnn
        state->registers->PC = &state->ram[nnn];
        break;

    case 0x2: // 2nnn
        *state->registers->SP = state->registers->PC;
        state->registers->SP--;
        state->registers->PC = &state->ram[nnn];
        break;

    case 0x3: // 3xnn
        if (state->registers->V[x] == nn) {
            state->registers->PC += OPCODE_SIZE;
        }
        break;

    case 0x4: // 4xnn
        if (state->registers->V[x] != nn) {
            state->registers->PC += OPCODE_SIZE;
        }
        break;

    case 0x5: // 5xy0
        if (state->registers->V[x] == state->registers->V[y]) {
            state->registers->PC += OPCODE_SIZE;
        }
        break;

    case 0x6: // 6xnn
        state->registers->V[x] = nn;
        break;

    case 0x7: // 7xnn
        state->registers->V[x] += nn;
        break;

    case 0x8:
        switch (LS_4BITS(LS_8BITS(opcode))) {
        case 0x0: // 8xy0
            state->registers->V[x] = state->registers->V[y];
            break;
        case 0x1: // 8xy1
            state->registers->V[x] |= state->registers->V[y];
            break;
        case 0x2: // 8xy2
            state->registers->V[x] &= state->registers->V[y];
            break;
        case 0x3: // 8xy3
            state->registers->V[x] ^= state->registers->V[y];
            break;
        case 0x4: // 8xy4
            break;
        case 0x5: // 8xy5
            break;
        case 0x6: // 8xy6
            state->registers->V[15] = LS_1BITS(state->registers->V[y]);
            state->registers->V[y] >>= 1;
            state->registers->V[x] = state->registers->V[y];
            break;
        case 0x7: // 8xy7
            break;
        case 0xE: // 8xyE
            state->registers->V[15] = MS_1BITS(state->registers->V[y]);
            state->registers->V[y] <<= 1;
            state->registers->V[x] = state->registers->V[y];
            break;
        default:
            break;
        }
        break;

    case 0x9: // 9xy0
        if (state->registers->V[x] != state->registers->V[y]) {
            state->registers->PC += OPCODE_SIZE;
        }
        break;

    case 0xA: // Annn
        state->registers->I = &state->ram[nnn];
        break;

    case 0xB: // Bnnn
        offset = state->registers->V[0];
        state->registers->PC = &state->ram[nnn + offset];
        break;

    case 0xC: // Cxnn
        random = rand() % (UINT8_MAX + 1);
        state->registers->V[x] = random & nn;
        break;

    case 0xD: // Dxyn
        screen_draw(state->screen, x, y, n, state->registers->I);
        state->registers->V[15] = 0;
        break;

    case 0xE:
        switch (LS_8BITS(opcode)) {
        case 0x9E: // Ex9E
            break;
        case 0xA1: // ExA1
            break;
        default:
            break;
        }
        break;

    case 0xF:
        switch (LS_8BITS(opcode)) {
        case 0x07: // Fx07
            state->registers->V[x] = state->registers->DT;
            break;
        case 0x0A: // Fx0A
            break;
        case 0x15: // Fx15
            state->registers->DT = state->registers->V[x];
            break;
        case 0x18: // Fx18
            state->registers->ST = state->registers->V[x];
            break;
        case 0x1E: // Fx1E
            state->registers->I += state->registers->V[x];
            break;
        case 0x29: // Fx29
            break;
        case 0x33: // Fx33
            break;
        case 0x55: // Fx55
            for (int i = 0; i <= x; i++) {
                *state->registers->I = state->registers->V[i];
                state->registers->I++;
            }
            break;
        case 0x65: // Fx65
            for (int i = 0; i <= x; i++) {
                state->registers->V[i] = *state->registers->I;
                state->registers->I++;
            }
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

int screen_clear(struct chip8_screen *screen)
{
    return 0;
}

int screen_draw(struct chip8_screen *screen, uint8_t x, uint8_t y, size_t sz, uint8_t *start)
{
    return 0;
}