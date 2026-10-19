// ============================================================================
// Theme.h - colours and highlight kinds shared by the whole program
// ============================================================================
#pragma once
#include <SDL.h>
#include <cstdint>

struct Color {
    Uint8 r = 0, g = 0, b = 0, a = 255;
};

inline Color rgb(int r, int g, int b, int a = 255) {
    return Color{(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a};
}
Color lerpColor(Color a, Color b, float t);
Color withAlpha(Color c, float alphaMul);   // multiplies the alpha channel
float luminance(Color c);                   // 0..1 perceived brightness

// What role a node plays in the current animation step.
// The colour for each role comes from the theme (yellow = current, ...).
enum class Hl : std::uint8_t {
    None,      // normal node
    Current,   // node being compared right now      (yellow)
    Visited,   // already on the search path          (green)
    Found,     // search hit / answer                 (blue)
    Remove,    // node about to be deleted            (red)
    New        // freshly inserted / copied key       (teal)
};

struct Theme {
    Color bg, topBar, panel, panelAlt, border, text, textDim, accent;
    Color canvas, canvasGrid;
    Color nodeFill, nodeRing, edge, shadow;
    Color current, visited, found, remove, fresh, paused;
    Color button, buttonHover, buttonPrimary, buttonText, inputBg;
};

const Theme& theme();
void toggleTheme();

Color hlColor(Hl h);        // colour of a highlight kind
Color nodeFillColor(Hl h);  // fill of a node with that highlight
Color nodeRingColor(Hl h);  // outline of a node with that highlight
