# CHIP8

## About

To quote [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM):

> Chip-8 is a simple, interpreted, programming language which was first used on some do-it-yourself computer systems in the late 1970s and early 1980s.
> The COSMAC VIP, DREAM 6800, and ETI 660 computers are a few examples.
> These computers typically were designed to use a television as a display, had between 1 and 4K of RAM, and used a 16-key hexadecimal keypad for input.
> The interpreter took up only 512 bytes of memory, and programs, which were entered into the computer in hexadecimal, were even smaller.

This is a simple implementation of such an interpreter, in the C programming language.

## Building from source

The only requirement is **SDL2**.

You should be able to get a copy from your GNU/Linux distribution repositories or the [official website](https://www.libsdl.org/). Once that's dealt with, all you need to do is call your C compiler with the necessary flags to link against SDL2.

Example for MSVC (Windows):

```
cl.exe /Wall .\chip8.c /Fechip8.exe /I D:\include\SDL2 /link /LIBPATH:D:\lib\c\SDL2\x86 SDL2.lib SDL2main.lib /SUBSYSTEM:CONSOLE
```

Adapt `/I D:\include\SDL2` and `/LIBPATH:D:\lib\SDL2\x86` to your setup.

## TODO

* ~~Initialization~~
* CPU loop
* Opcode parsing
* Instructions
* Display
* Keyboard
* Sound
* Debugger
* Assembler
* Disassembler

## References

https://en.wikipedia.org/wiki/CHIP-8

http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

http://mattmik.com/files/chip8/mastering/chip8.html