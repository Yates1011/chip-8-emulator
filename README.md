# chip-8-emulator



## Build with imgui

# 1.  
sudo apt install libsdl2-dev

# 2. g++ cpu.cpp display.cpp \
    imgui/imgui.cpp imgui/imgui_draw.cpp \
    imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_sdl2.cpp \
    imgui/backends/imgui_impl_opengl3.cpp \
    -I. -Iimgui \
    -o chip8 -std=c++23 -O2 \
    $(sdl2-config --cflags --libs) -lGL

## Optional
-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror

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