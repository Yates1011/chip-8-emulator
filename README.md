# chip-8-emulator

## Build 
g++ cpu.cpp -o chip8 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -std=c++23

## Profile

# 1. Install valgrind
sudo apt-get install valgrind kcachegrind

# 2. Compile normally  
As shown in build instructions

# 3. Run with callgrind
valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./chip8

# Let it run, then Ctrl+C

# 4. Visualise with kcachegrind (GUI)
kcachegrind callgrind.out