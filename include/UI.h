// ============================================================================
// UI.h - minimal widgets: Button, InputBox, Slider (drawn with Gfx)
// ============================================================================
#pragma once
#include <SDL.h>
#include <string>
#include "Gfx.h"

inline bool pointIn(const SDL_FRect& r, float x, float y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

struct Button {
    int id = 0;
    std::string label;
    std::string tip;        // tooltip shown on hover
    SDL_FRect rect{0, 0, 0, 0};
    bool primary = false;
    bool active = false;    // toggled look (e.g. Pause)
    bool selected = false;  // the operation the user ran last (highlighted)
    float hover = 0;        // 0..1 animated hover amount
    float press = 0;        // 0..1 click flash

    void update(float dt, float mx, float my);
    void draw(Gfx& g) const;
    bool hit(float x, float y) const { return pointIn(rect, x, y); }
};

struct InputBox {
    std::string text;
    std::string placeholder;
    SDL_FRect rect{0, 0, 0, 0};
    bool focused = false;
    float blink = 0;
    size_t maxLen = 48;

    void update(float dt) { blink += dt; }
    void draw(Gfx& g) const;
    // Returns true when the event was consumed by the box.
    bool handleText(const char* utf8);
    bool handleBackspace();
};

struct Slider {
    SDL_FRect rect{0, 0, 0, 0};
    float value = 0.5f;     // 0..1
    bool dragging = false;
    void draw(Gfx& g, const std::string& label) const;
    // returns true if the value changed
    bool handleEvent(const SDL_Event& e);
};
