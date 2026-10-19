// ============================================================================
// App.h - window, renderer and the 60 FPS main loop around the BST screen
// ============================================================================
#pragma once
#include <SDL.h>
#include <string>
#include "BSTVis.h"
#include "Gfx.h"

class App {
public:
    bool init();
    void run(bool autotest = false);
    void frame();                    // one main-loop iteration (public for the web build)
    void shutdown();
    // Test mode: run a script of button presses and save a screenshot (BMP).
    bool screenshot(const std::string& file, const std::string& script, int frame, float t);

private:
    void render();
    void updatePixelScale();
    void injectAutotestEvents(float elapsed);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    Gfx gfx_;
    BSTVis bst_;                     // the Binary Search Tree screen
    bool running_ = true;
    bool vsync_ = true;
    int w_ = 1280, h_ = 800;
    bool showFps_ = false;           // F3
    float fps_ = 0, fpsTimer_ = 0;
    int fpsFrames_ = 0;
    int autoStep_ = -1;
    long totalFrames_ = 0;
    bool autotest_ = false, fullscreen_ = false;
    Uint64 freq_ = 1, last_ = 0, start_ = 0;
};
