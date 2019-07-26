#include <stdint.h>

#define RAM_SIZE   4096
#define STACK_SIZE 16

uint8_t  RAM[RAM_SIZE];
uint16_t STACK[STACK_SIZE];

struct cpu_registers {
    uint8_t  V0;
    uint8_t  V1;
    uint8_t  V2;
    uint8_t  V3;
    uint8_t  V4;
    uint8_t  V5;
    uint8_t  V6;
    uint8_t  V7;
    uint8_t  V8;
    uint8_t  V9;
    uint8_t  VA;
    uint8_t  VB;
    uint8_t  VC;
    uint8_t  VD;
    uint8_t  VE;
    uint8_t  VF;
    uint8_t  DT;
    uint8_t  ST;
    uint8_t  SP;
    uint16_t I;
    uint16_t PC;
};

int main(int argc, char *argv[])
{
    return 0;
}