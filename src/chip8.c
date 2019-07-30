#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define RAM_SIZE           4096  // bytes
#define PROGRAM_START      0x200
#define PROGRAM_END        0xEDE
#define STACK_START        0xEFE
#define BUFFER_START       0xF00
#define NUM_DATA_REGISTERS 16

void run(struct chip8_state state);

struct chip8_registers {
    uint8_t  V[NUM_DATA_REGISTERS];
    uint8_t  DT;
    uint8_t  ST;
    uint16_t *SP;
    uint16_t *PC;
    uint16_t *I;
};

struct chip8_state {
    struct chip8_registers *registers;
    uint8_t *ram;
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Missing program filename as a parameter.\n");
        return EXIT_FAILURE;
    }

    FILE *program_file = fopen(argv[1], "r");
    if (program_file == NULL) {
        fprintf(stderr, "[ERROR] Unable to open program file %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    uint8_t ram[RAM_SIZE] = {0};
    // XXX Instructions are stored in memory in a big-endian fashion
    (void)fread(&ram[PROGRAM_START], 1, PROGRAM_END - PROGRAM_START, program_file);
    if (ferror(program_file) || !feof(program_file)) {
        fprintf(stderr, "[ERROR] Unable to load program into RAM! Exiting...\n");
        return EXIT_FAILURE;
    }

    (void)fclose(program_file);

    struct chip8_registers registers = {
        .SP = (uint16_t *)&ram[STACK_START],
        .PC = (uint16_t *)&ram[PROGRAM_START],
    };

    struct chip8_state state = {
        .registers = &registers,
        .ram = ram,
    };

    run(state);

    return EXIT_SUCCESS;
}

void run(struct chip8_state state)
{
#if 0
    printf("state.registers->SP points to %p\n", state.registers->SP);
    printf("*(state.registers->SP) contains %02x\n", *(state.registers->SP));

    state.registers->SP--;

    printf("state.registers->SP points to %p\n", state.registers->SP);
    printf("*(state.registers->SP) contains %02x\n", *(state.registers->SP));
#endif
}