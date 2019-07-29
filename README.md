# CHIP8

## About

To quote [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM):

> Chip-8 is a simple, interpreted, programming language which was first used on some do-it-yourself computer systems in the late 1970s and early 1980s.
> The COSMAC VIP, DREAM 6800, and ETI 660 computers are a few examples.
> These computers typically were designed to use a television as a display, had between 1 and 4K of RAM, and used a 16-key hexadecimal keypad for input.
> The interpreter took up only 512 bytes of memory, and programs, which were entered into the computer in hexadecimal, were even smaller.

This is a simple implementation of such an interpreter, in the C programming language.

## Building from source

For now I'm using **CMake**.

**CMake** is often refered to as a build file generator - it is responsible for the automated creation of Makefiles, Visual Studio Solutions, etc.
You can get **CMake** from the [official website](https://cmake.org/download/), your GNU/Linux distribution repositories or Visual Studio installer.
Once that's dealt with, all you need to do is:

```
cd chip8
mkdir build
cd build
cmake ..
cmake --build . --target Release
```

## TODO

* ~~Initialization~~
* CPU loop
* Display
* Keyboard
* Sound
* Assembler
* Disassembler
* Debugger

## References

https://en.wikipedia.org/wiki/CHIP-8

http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

http://mattmik.com/files/chip8/mastering/chip8.html

https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Technical-Reference