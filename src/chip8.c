#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define RAM_SIZE    4096
#define PROG_START  0x200
#define PROG_END    0xEA0
#define STACK_START 0xEC8
#define REG_START   0xEE8

void run(struct chip8_state state);

struct chip8_registers {
    uint8_t  *V; // first of the contiguous registers V[0]...V[F]
    uint8_t  *DT;
    uint8_t  *ST;
    uint16_t *SP;
    uint16_t *PC;
    uint16_t *I;
};

struct chip8_state {
    struct chip8_registers *reg;
    uint8_t *ram;
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "[ERROR] Missing program filename as a parameter.\n");
        return EXIT_FAILURE;
    }

    FILE *progfile = fopen(argv[1], "r");
    if (progfile == NULL) {
        fprintf(stderr, "[ERROR] Unable to open program file %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    uint8_t ram[RAM_SIZE] = {0};
    (void)fread(&ram[PROG_START], 1, PROG_END - PROG_START, progfile);
    if (ferror(progfile) || !feof(progfile)) {
        fprintf(stderr, "[ERROR] Unable to load program into RAM! Exiting...\n");
        return EXIT_FAILURE;
    }

    (void)fclose(progfile);

    struct chip8_registers reg = {
        .V  = &ram[REG_START],
        .SP = (uint16_t *)&ram[STACK_START],
        .PC = (uint16_t *)&ram[PROG_START],
    };

    struct chip8_state state = {
        .reg = &reg,
        .ram = ram,
    };

    run(state);

    return EXIT_SUCCESS;
}

void run(struct chip8_state state)
{
    // printf("state.reg->V points to %p\n", state.reg->V);
    // printf("&state.ram[REG_START] is %p\n", &(state.ram[REG_START]));
}