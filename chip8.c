#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <SDL.h>

#define STACK_SIZE 16
#define STACK_START 0x00
#define RAM_SIZE 4096
#define PROGRAM_START 0x200
#define PROGRAM_END 0xFFF
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
#define SDL_WINDOWPOS_X SDL_WINDOWPOS_CENTERED
#define SDL_WINDOWPOS_Y SDL_WINDOWPOS_CENTERED
#define SDL_WINDOW_WIDTH 640
#define SDL_WINDOW_HEIGHT 480
#define SDL_WINDOW_FLAGS 0

#define SDL_RENDERER_FLAGS SDL_RENDERER_ACCELERATED
#define SDL_RENDERER_INDEX -1

#define SDL_TEXTURE_FORMAT SDL_PIXELFORMAT_RGBA8888
#define SDL_TEXTURE_ACESS SDL_TEXTUREACCESS_STREAMING
#define SDL_TEXTURE_WIDTH PIXELS_WIDTH
#define SDL_TEXTURE_HEIGHT PIXELS_HEIGHT

#define PIXELS_WIDTH 64
#define PIXELS_HEIGHT 32
#define BYTES_PER_PIXEL 4
#define PIXEL_DEFAULT_MASK 0x80
#define PIXEL_STATE_UNSET 0x00000000
#define PIXEL_STATE_SET 0xFFFFFF00

#define SPRITE_WIDTH 8

#define CPU_CLOCK_HZ 500
#define TIMER_CLOCK_HZ 60
#define TIMER_UPDATE_CYCLES (CPU_CLOCK_HZ / TIMER_CLOCK_HZ)

#define EXIT_INVALID_ARGS 1
#define EXIT_INIT_FAILURE 2

#define ESDL 250

struct chip8_registers {
    uint8_t V[NUM_V_REGISTERS];
    uint8_t DT;
    uint8_t ST;
    uint8_t **SP;
    uint8_t *PC;
    uint8_t *I;
};

struct chip8_display {
    uint32_t pixels[PIXELS_WIDTH * PIXELS_HEIGHT];
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};

struct chip8_state {
    uint8_t *ram;
    uint8_t **stack;
    struct chip8_registers *registers;
    struct chip8_display *display;
};

int init(struct chip8_state *state, char *filename);
int init_memory(struct chip8_state *state, char *filename);
int init_registers(struct chip8_state *state);
int init_display(struct chip8_state *state);
void deinit(struct chip8_state *state);
void deinit_display(struct chip8_state *state);
void deinit_registers(struct chip8_state *state);
void deinit_memory(struct chip8_state *state);
int run(struct chip8_state *state);
void clear_display(struct chip8_display *display);
int draw_display(struct chip8_display *display, uint8_t x, uint8_t y, uint8_t sprite_height, uint8_t *sprite);
int refresh_display(struct chip8_display *display);

int main(int argc, char *argv[])
{
    int err = 0;
    int quit = 0;
    int cycles = 0;
    uint32_t delay, delta_ticks, start_tick, end_tick;
    struct chip8_state state;
    SDL_Event event;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_INVALID_ARGS;
    }

    err = init(&state, argv[1]);
    if (err) {
        return EXIT_INIT_FAILURE;
    }

    while (!quit) {
        start_tick = SDL_GetTicks();

        err = run(&state);  // Fetch, decode and execute next CHIP-8 instruction
        if (err) {
            break;
        }

        cycles++;
        if (cycles == TIMER_UPDATE_CYCLES) {
            err = refresh_display(state.display);
            if (err) {
                break;
            }
            cycles = 0;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
                break;
            }
        }

        end_tick = SDL_GetTicks();
        delta_ticks = end_tick - start_tick;
        if (delta_ticks < (1000 / CPU_CLOCK_HZ)) {
            delay = (1000 / CPU_CLOCK_HZ) - delta_ticks;
            SDL_Delay(delay);
        }
    }

    deinit(&state);
    return err;
}

int init(struct chip8_state *state, char *filename)
{
    int err;
    unsigned int seed;

    err = init_memory(state, filename);
    if (err) {
        goto error_init_memory;
    }

    err = init_registers(state);
    if (err) {
        goto error_init_registers;
    }

    err = init_display(state);
    if (err) {
        goto error_init_display;
    }

    seed = time(NULL);
    srand(seed);

    return 0;

error_init_display:
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
        fprintf(stderr, "calloc() for CHIP-8 STACK failed\n");
        err = ENOMEM;
        goto error_alloc_stack;
    }

    state->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    if (state->ram == NULL) {
        fprintf(stderr, "calloc() for CHIP-8 RAM failed\n");
        err = ENOMEM;
        goto error_alloc_ram;
    }

    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "fopen() failed: %s\n", strerror(errno));
        err = EIO;
        goto error_open_file;
    }

    fread(&state->ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START + 1, file);
    if (ferror(file) || !feof(file)) {
        fprintf(stderr, "fread() into CHIP-8 RAM failed\n");
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
        fprintf(stderr, "calloc() for CHIP-8 REGISTERS failed\n");
        return ENOMEM;
    }

    state->registers->SP = &state->stack[STACK_START];
    state->registers->PC = &state->ram[PROGRAM_START];

    return 0;
}

int init_display(struct chip8_state *state)
{
    int err;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
        err = ESDL;
        goto error_init_sdl;
    }

    window = SDL_CreateWindow(SDL_WINDOW_NAME, SDL_WINDOWPOS_X, SDL_WINDOWPOS_Y, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_WINDOW_FLAGS);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow() failed: %s\n", SDL_GetError());
        err = ESDL;
        goto error_create_window;
    }

    renderer = SDL_CreateRenderer(window, SDL_RENDERER_INDEX, SDL_RENDERER_FLAGS);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer() failed: %s\n", SDL_GetError());
        err = ESDL;
        goto error_create_renderer;
    }

    texture = SDL_CreateTexture(renderer, SDL_TEXTURE_FORMAT, SDL_TEXTURE_ACESS, SDL_TEXTURE_WIDTH, SDL_TEXTURE_HEIGHT);
    if (texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture() failed: %s\n", SDL_GetError());
        err = ESDL;
        goto error_create_texture;
    }

    state->display = calloc(1, sizeof(struct chip8_display));
    if (state->display == NULL) {
        fprintf(stderr, "calloc() for CHIP-8 DISPLAY failed\n");
        err = ENOMEM;
        goto error_alloc_display;
    }

    state->display->window = window;
    state->display->renderer = renderer;
    state->display->texture = texture;

    return 0;

error_alloc_display:
    SDL_DestroyTexture(texture);
error_create_texture:
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
    deinit_display(state);
    deinit_registers(state);
    deinit_memory(state);
}

void deinit_display(struct chip8_state *state)
{
    SDL_DestroyTexture(state->display->texture);
    SDL_DestroyRenderer(state->display->renderer);
    SDL_DestroyWindow(state->display->window);
    SDL_Quit();
    
    free(state->display);
    state->display = NULL;
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

int run(struct chip8_state *state)
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
            clear_display(state->display);
            break;
        case 0xEE: // 0x00EE
            state->registers->PC = *state->registers->SP;
            state->registers->SP--;
            break;
        default:
            break;
        }
        break;

    case 0x1: // 1nnn
        state->registers->PC = &state->ram[nnn];
        break;

    case 0x2: // 2nnn
        state->registers->SP++;
        *state->registers->SP = state->registers->PC;
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
            if (state->registers->V[x] + state->registers->V[y] > UINT8_MAX) {
                state->registers->V[15] = 1; // A carry will occur
            } else {
                state->registers->V[15] = 0;
            }
            state->registers->V[x] += state->registers->V[y];
            break;
        case 0x5: // 8xy5
            if (state->registers->V[y] > state->registers->V[x]) {
                state->registers->V[15] = 0; // A borrow will occur
            } else {
                state->registers->V[15] = 1;
            }
            state->registers->V[x] -= state->registers->V[y];
            break;
        case 0x6: // 8xy6
            state->registers->V[15] = LS_1BITS(state->registers->V[y]);
            state->registers->V[y] >>= 1;
            state->registers->V[x] = state->registers->V[y];
            break;
        case 0x7: // 8xy7
            if (state->registers->V[x] > state->registers->V[y]) {
                state->registers->V[15] = 0; // A borrow will occur
            } else {
                state->registers->V[15] = 1;
            }
            state->registers->V[x] = state->registers->V[y] - state->registers->V[x];
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
        state->registers->V[15] = draw_display(state->display, state->registers->V[x], state->registers->V[y], n, state->registers->I);
        break;

    case 0xE:
        switch (LS_8BITS(opcode)) {
        case 0x9E: // Ex9E
            // TODO (ricardoapl) Skip next instruction if key with the value of V[x] is pressed
            break;
        case 0xA1: // ExA1
            // TODO (ricardoapl) Skip next instruction if key with the value of V[x] is not pressed
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
            // TODO (ricardoapl) Wait for a key press, store the value of the key in V[x]
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
            // TODO (ricardoapl) Set I = location of sprite for digit V[x]
            break;
        case 0x33: // Fx33
            // TODO (ricardoapl) Store BCD representation of V[x] in memory locations I, I+1, and I+2
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

void clear_display(struct chip8_display *display)
{
    uint16_t idx;

    for (uint8_t row = 0; row < PIXELS_HEIGHT; row++) {
        for (uint8_t col = 0; col < PIXELS_WIDTH; col++) {
            idx = (row * PIXELS_WIDTH) + col;
            display->pixels[idx] = PIXEL_STATE_UNSET;
        }
    }
}

int draw_display(struct chip8_display *display, uint8_t x, uint8_t y, uint8_t sprite_height, uint8_t *sprite)
{
    uint8_t display_row, display_col, sprite_row, sprite_col, mask, ret = 0;
    uint16_t idx;
    uint32_t old_state, new_state;

    for (uint8_t row = 0; row < sprite_height; row++) {
        display_row = y + row;
        sprite_row = sprite[row];
        mask = PIXEL_DEFAULT_MASK;
        for (uint8_t col = 0; col < SPRITE_WIDTH; col++) {
            display_col = x + col;
            sprite_col = (sprite_row & mask) >> (SPRITE_WIDTH - 1 - col);
            mask >>= 1;
            
            idx = (display_row * PIXELS_WIDTH) + display_col;
            old_state = display->pixels[idx];
            display->pixels[idx] ^= (sprite_col * PIXEL_STATE_SET);
            new_state = display->pixels[idx];
            if (old_state == PIXEL_STATE_SET && new_state == PIXEL_STATE_UNSET) {
                ret = 1;
            }
        }
    }

    return ret;
}

int refresh_display(struct chip8_display *display)
{
    int err;
    int pitch = PIXELS_WIDTH * BYTES_PER_PIXEL;

    err = SDL_RenderClear(display->renderer);
    if (err) {
        fprintf(stderr, "SDL_RenderClear() failed: %s\n", SDL_GetError());
        return err;
    }

    err = SDL_UpdateTexture(display->texture, NULL, display->pixels, pitch);
    if (err) {
        fprintf(stderr, "SDL_UpdateTexture() failed: %s\n", SDL_GetError());
        return err;
    }

    err = SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
    if (err) {
        fprintf(stderr, "SDL_RenderCopy() failed: %s\n", SDL_GetError());
        return err;
    }

    SDL_RenderPresent(display->renderer);

    return 0;
}