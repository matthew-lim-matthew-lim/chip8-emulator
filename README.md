# chip8-emulator

## How to run the Chip8 Emulator
- Create a `build` folder in the root directory: `mkdir build`.
- `cd` into the `build` folder: `cd build`.
- Use `cmake ..` to generate the build files.
- Use `make` to build the program.
- Run your chip8 roms using `./chip8 10 4 ../chip8-roms/test_opcode.ch8` (change the name of the rom to match the rom you want to run).

## Credits
- Cowgod's Chip8 Technical Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM - A really concise reference that can be used as a schema for what instructions you need to write.
- Austin Morlan's Chip8 Blog: https://austinmorlan.com/posts/chip8_emulator/ - A detailed guide on his implementation of a Chip8 emulator. However, he opted to use `glad`, `sdl`, and `imgui`, which was very buggy on my machine. I instead only used `sdl`, meaning that my `platform.cpp` and `platform.h` code is quite different.

Have fun!