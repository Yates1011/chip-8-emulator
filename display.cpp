#include "cpu.hpp"

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>


void initialise(Chip8 &chip8);
void emulateCycle(Chip8 &c);

bool loadROM(std::string_view filename, Chip8 &chip8);

static int mapSDLKey(SDL_Scancode sc) {
    switch (sc) {
        case SDL_SCANCODE_1: return 0x1;
        case SDL_SCANCODE_2: return 0x2;
        case SDL_SCANCODE_3: return 0x3;
        case SDL_SCANCODE_4: return 0xC;
        case SDL_SCANCODE_Q: return 0x4;
        case SDL_SCANCODE_W: return 0x5;
        case SDL_SCANCODE_E: return 0x6;
        case SDL_SCANCODE_R: return 0xD;
        case SDL_SCANCODE_A: return 0x7;
        case SDL_SCANCODE_S: return 0x8;
        case SDL_SCANCODE_D: return 0x9;
        case SDL_SCANCODE_F: return 0xE;
        case SDL_SCANCODE_Z: return 0xA;
        case SDL_SCANCODE_X: return 0x0;
        case SDL_SCANCODE_C: return 0xB;
        case SDL_SCANCODE_V: return 0xF;
        default:             return -1;
    }
}


//  Upload chip8.gfx into an existing OpenGL texture
static void uploadDisplay(Chip8 &chip8, GLuint tex) {
    // Build an RGBA pixel buffer 
    static uint32_t pixels[SCREEN_HEIGHT * SCREEN_WIDTH];

    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            pixels[y * SCREEN_WIDTH + x] = chip8.gfx[y][x] ? 0xFFFFFFFF : 0xFF000000;

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 SCREEN_WIDTH, SCREEN_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}


static void renderDisplayWindow(GLuint tex) {
    constexpr float SCALE = 10.0f;
    constexpr float W = SCREEN_WIDTH  * SCALE;
    constexpr float H = SCREEN_HEIGHT * SCALE;

    ImGui::SetNextWindowSize(ImVec2(W + W_BUFFER_SIZE, H + H_BUFFER_SIZE), ImGuiCond_Always);

    ImGui::Begin("Display", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImGui::Image((ImTextureID)(uintptr_t)tex, ImVec2(W, H));

    ImGui::End();
}


static void renderDebugWindow(const Chip8 &c) {
    ImGui::SetNextWindowSize(ImVec2(X_MAIN_WINDOW_SIZE, Y_MAIN_WINDOW_SIZE), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debugger");

    if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(4, "regs", true);
        for (int i = 0; i < NUM_REGISTERS; ++i) {
            ImGui::Text("V%X: %02X", i, c.V[i]);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::Text("PC : %04X", c.pc);
        ImGui::SameLine(120);
        ImGui::Text("I  : %04X", c.I);
        ImGui::Text("SP : %02X",  c.sp);
        ImGui::SameLine(120);
        ImGui::Text("OP : %04X", c.opcode);
    }

    if (ImGui::CollapsingHeader("Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Delay : %d", c.delayTimer);
        ImGui::SameLine(120);
        ImGui::Text("Sound : %d", c.soundTimer);
    }

    if (ImGui::CollapsingHeader("Stack", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < STACK_SIZE; ++i) {
            // Highlight the current stack pointer entry
            if (i == c.sp - 1)
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f),
                                   "[%02d] %04X  <-- SP", i, c.stack[i]);
            else
                ImGui::Text("[%02d] %04X", i, c.stack[i]);
        }
    }

    if (ImGui::CollapsingHeader("Keypad")) {
        const char* labels[NUM_KEYS] = {
            "0","1","2","3","4","5","6","7",
            "8","9","A","B","C","D","E","F"
        };
        ImGui::Columns(4, "keys", false);
        for (int i = 0; i < NUM_KEYS; ++i) {
            if (c.keys[i])
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[%s]", labels[i]);
            else
                ImGui::Text("[%s]", labels[i]);
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }

    ImGui::End();
}


int main() {
    Chip8 chip8;
    initialise(chip8);

    std::string romPath;
    std::cout << "Enter path to ROM: ";
    std::getline(std::cin, romPath);

    if (!loadROM(romPath, chip8))
        return 1;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* win = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1100, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glCtx = SDL_GL_CreateContext(win);
    SDL_GL_SetSwapInterval(1); // vsync

    GLuint displayTex = 0;
    glGenTextures(1, &displayTex);
    glBindTexture(GL_TEXTURE_2D, displayTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    uploadDisplay(chip8, displayTex); // blank frame to start

 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(win, glCtx);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    auto lastTimer = Clock::now();

    bool running = true;
    while (running) {

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);

            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_KEYDOWN) {
                int k = mapSDLKey(e.key.keysym.scancode);
                if (k >= 0) chip8.keys[k] = true;
            }
            if (e.type == SDL_KEYUP) {
                int k = mapSDLKey(e.key.keysym.scancode);
                if (k >= 0) chip8.keys[k] = false;
            }
        }

        // Emulate several cycles per frame (~500 Hz at 60fps)
        for (int i = 0; i < 8; ++i)
            emulateCycle(chip8);

        auto now = Clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastTimer).count() >= 16) {
            if (chip8.delayTimer > 0) --chip8.delayTimer;
            if (chip8.soundTimer > 0) --chip8.soundTimer;
            lastTimer = now;
        }

        if (chip8.draw_flag) {
            uploadDisplay(chip8, displayTex);
            chip8.draw_flag = false;
        }


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        renderDisplayWindow(displayTex);
        renderDebugWindow(chip8);

        ImGui::Render();
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &displayTex);
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}