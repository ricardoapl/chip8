#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define RAM_SIZE 4096
#define PROGRAM_START 0x200
#define PROGRAM_END 0xEDE
#define STACK_START 0xEFE
#define BUFFER_START 0xF00
#define NUM_DATA_REGISTERS 16

struct chip8_registers {
    uint8_t V[NUM_DATA_REGISTERS];
    uint8_t DT;
    uint8_t ST;
    uint8_t *SP;
    uint8_t *PC;
    uint8_t *I;
};

struct chip8_state {
    struct chip8_registers *registers;
    uint8_t *ram;
};

void init(struct chip8_state *state, char *filename);
void run(struct chip8_state state);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Missing parameter program filename.\n");
        return EXIT_FAILURE;
    }

    struct chip8_state state;
    init(&state, argv[1]);
    run(state);
}

void init(struct chip8_state *state, char *filename)
{
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

    struct chip8_registers *registers = calloc(1, sizeof(struct chip8_registers));
    if (registers == NULL) {
        fprintf(stderr, "[ERROR] Unable o allocate Chip-8 registers!\n");
        free(ram);
        exit(EXIT_FAILURE);
    }

    registers->SP = &ram[STACK_START];
    registers->PC = &ram[PROGRAM_START];
    
    state->registers = registers;
    state->ram = ram;
}

void run(struct chip8_state state)
{
    while (1) {
        // Fetch opcode
        // Decode opcode -> instruction
        // Execute instruction
    }
}