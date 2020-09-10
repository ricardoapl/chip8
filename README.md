## Overview

To quote [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM):

> Chip-8 is a simple, interpreted, programming language which was first used on some do-it-yourself computer systems in the late 1970s and early 1980s.
> The COSMAC VIP, DREAM 6800, and ETI 660 computers are a few examples.
> These computers typically were designed to use a television as a display, had between 1 and 4K of RAM, and used a 16-key hexadecimal keypad for input.
> The interpreter took up only 512 bytes of memory, and programs, which were entered into the computer in hexadecimal, were even smaller.

This is an implementation of such an interpreter in the C programming language.

## Installation

Clone this repository into your local machine and invoke your C compiler of choice with the necessary flags to link against SDL2.

Example for MSVC (Windows):

```
cl.exe /Wall .\chip8.c /Fechip8.exe /I D:\include\SDL2 /link /LIBPATH:D:\lib\SDL2\x86 SDL2.lib SDL2main.lib /SUBSYSTEM:CONSOLE
```

Adapt `/I D:\include\SDL2` and `/LIBPATH:D:\lib\SDL2\x86` to your setup.

## Requirements

The only requirement is SDL2.

You should be able to get a copy from your GNU/Linux distribution repositories or the [official website](https://www.libsdl.org/).

## Usage

The CHIP8 interpreter expects the path to a ROM as an argument.

Example: ```.\chip8.exe ..\roms\BLINKY```.

## Support

Please use the [issue tracker](https://github.com/ricardoapl/chip8/issues) to ask for help, request a new feature or report any bugs.

## Roadmap

- [x] Bootstrap
- [ ] Fetch->Decode->Execute
- [ ] Display
- [ ] Keyboard
- [ ] Sound

## Contributing

I am not accepting any pull requests at the moment. However, I welcome any bug reports or suggestions you might have.

## Authors

This software is developed and maintained by Ricardo Lopes ([**@ricardoapl**](https://github.com/ricardoapl)).

## License

CHIP8 is available under the terms of the MIT License.