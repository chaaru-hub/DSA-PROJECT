// ============================================================================
// Theme.cpp - dark and light colour themes (F2 switches between them)
// ============================================================================
#include "Theme.h"
#include <algorithm>

Color lerpColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto mix = [t](Uint8 x, Uint8 y) { return (Uint8)(x + (y - x) * t + 0.5f); };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

Color withAlpha(Color c, float alphaMul) {
    alphaMul = std::clamp(alphaMul, 0.0f, 1.0f);
    c.a = (Uint8)(c.a * alphaMul + 0.5f);
    return c;
}

float luminance(Color c) {
    return (0.299f * c.r + 0.587f * c.g + 0.114f * c.b) / 255.0f;
}

static Theme makeDark() {
    Theme t;
    t.bg = rgb(17, 21, 28);        t.topBar = rgb(24, 29, 39);
    t.panel = rgb(26, 32, 43);     t.panelAlt = rgb(34, 42, 55);
    t.border = rgb(46, 55, 70);    t.text = rgb(232, 237, 245);
    t.textDim = rgb(142, 154, 176); t.accent = rgb(90, 169, 255);
    t.canvas = rgb(20, 25, 34);    t.canvasGrid = rgb(28, 35, 47);
    t.nodeFill = rgb(43, 58, 85);  t.nodeRing = rgb(111, 139, 184);
    t.edge = rgb(78, 94, 120);     t.shadow = rgb(0, 0, 0, 90);
    t.current = rgb(255, 210, 63); t.visited = rgb(62, 207, 110);
    t.found = rgb(61, 139, 255);   t.remove = rgb(255, 77, 94);
    t.fresh = rgb(46, 196, 182);   t.paused = rgb(255, 149, 51);
    t.button = rgb(38, 48, 67);    t.buttonHover = rgb(49, 64, 90);
    t.buttonPrimary = rgb(47, 111, 214); t.buttonText = rgb(232, 237, 245);
    t.inputBg = rgb(15, 19, 25);
    return t;
}

static Theme makeLight() {
    Theme t;
    t.bg = rgb(238, 241, 246);     t.topBar = rgb(255, 255, 255);
    t.panel = rgb(255, 255, 255);  t.panelAlt = rgb(243, 245, 249);
    t.border = rgb(213, 219, 229); t.text = rgb(29, 37, 51);
    t.textDim = rgb(93, 106, 128); t.accent = rgb(37, 99, 235);
    t.canvas = rgb(250, 251, 253); t.canvasGrid = rgb(236, 240, 246);
    t.nodeFill = rgb(219, 231, 255); t.nodeRing = rgb(74, 111, 165);
    t.edge = rgb(154, 168, 189);   t.shadow = rgb(0, 0, 0, 40);
    t.current = rgb(255, 204, 0);  t.visited = rgb(34, 179, 90);
    t.found = rgb(47, 124, 246);   t.remove = rgb(236, 59, 76);
    t.fresh = rgb(23, 168, 155);   t.paused = rgb(255, 138, 31);
    t.button = rgb(231, 235, 242); t.buttonHover = rgb(216, 223, 235);
    t.buttonPrimary = rgb(37, 99, 235); t.buttonText = rgb(29, 37, 51);
    t.inputBg = rgb(255, 255, 255);
    return t;
}

static Theme gDark = makeDark();
static Theme gLight = makeLight();
static bool gIsDark = true;

const Theme& theme() { return gIsDark ? gDark : gLight; }
void toggleTheme() { gIsDark = !gIsDark; }

Color hlColor(Hl h) {
    const Theme& t = theme();
    switch (h) {
        case Hl::Current: return t.current;
        case Hl::Visited: return t.visited;
        case Hl::Found:   return t.found;
        case Hl::Remove:  return t.remove;
        case Hl::New:     return t.fresh;
        default:          return t.nodeFill;
    }
}

Color nodeFillColor(Hl h) { return hlColor(h); }

Color nodeRingColor(Hl h) {
    if (h == Hl::None) return theme().nodeRing;
    return lerpColor(hlColor(h), rgb(0, 0, 0), 0.3f);
}
