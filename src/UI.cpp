// ============================================================================
// UI.cpp - drawing and input handling for the small widget set
// ============================================================================
#include "UI.h"
#include <algorithm>
#include <cmath>

void Button::update(float dt, float mx, float my) {
    float target = hit(mx, my) ? 1.0f : 0.0f;
    hover += (target - hover) * std::min(1.0f, dt * 14.0f);   // smooth hover fade
    press = std::max(0.0f, press - dt * 3.0f);
}

void Button::draw(Gfx& g) const {
    const Theme& t = theme();
    Color base = primary ? t.buttonPrimary : t.button;
    Color hov = primary ? lerpColor(t.buttonPrimary, rgb(255, 255, 255), 0.18f) : t.buttonHover;
    if (active) base = hov = t.accent;
    if (selected) base = hov = t.current;   // last used operation: yellow
    Color c = lerpColor(base, hov, hover);
    c = lerpColor(c, rgb(255, 255, 255), press * 0.35f);
    float lift = hover * 1.5f;
    g.fillRoundRect(rect.x, rect.y + 2, rect.w, rect.h, 8, withAlpha(t.shadow, 0.6f));
    g.fillRoundRect(rect.x, rect.y - lift, rect.w, rect.h, 8, c);
    if (!primary && !active && !selected) g.strokeRoundRect(rect.x, rect.y - lift, rect.w, rect.h, 8, 1.0f, t.border);
    if (selected) {
        // glow ring so the last used button stands out
        g.strokeRoundRect(rect.x - 3, rect.y - lift - 3, rect.w + 6, rect.h + 6, 10, 2.0f, withAlpha(t.current, 0.7f));
    }
    Color tc = selected ? rgb(30, 24, 8) : (primary || active) ? rgb(255, 255, 255) : t.buttonText;
    g.textCentered(label, rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f - lift, 14, tc, true);
}

void InputBox::draw(Gfx& g) const {
    const Theme& t = theme();
    g.fillRoundRect(rect.x, rect.y, rect.w, rect.h, 8, t.inputBg);
    g.strokeRoundRect(rect.x, rect.y, rect.w, rect.h, 8, focused ? 2.0f : 1.2f, focused ? t.accent : t.border);
    float tx = rect.x + 12, cy = rect.y + rect.h * 0.5f;
    g.pushClip(rect.x + 4, rect.y, rect.w - 8, rect.h);
    if (text.empty()) {
        g.text(placeholder, tx, cy - 9, 14, t.textDim);
    } else {
        // keep the end of long text visible
        float w = g.textWidth(text, 16);
        if (w > rect.w - 28) tx -= w - (rect.w - 28);
        g.text(text, tx, cy - 10, 16, t.text);
    }
    if (focused && std::fmod(blink, 1.0f) < 0.6f) {
        float cx = tx + (text.empty() ? 0 : g.textWidth(text, 16)) + 2;
        g.fillRect(cx, cy - 10, 2, 20, t.accent);
    }
    g.popClip();
}

bool InputBox::handleText(const char* utf8) {
    for (const char* p = utf8; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (ch >= 32 && ch < 127 && text.size() < maxLen) text += (char)ch;
    }
    blink = 0;
    return true;
}

bool InputBox::handleBackspace() {
    if (!text.empty()) text.pop_back();
    blink = 0;
    return true;
}

void Slider::draw(Gfx& g, const std::string& label) const {
    const Theme& t = theme();
    float cy = rect.y + rect.h * 0.5f;
    g.fillRoundRect(rect.x, cy - 3, rect.w, 6, 3, t.panelAlt);
    g.fillRoundRect(rect.x, cy - 3, rect.w * value, 6, 3, t.accent);
    float kx = rect.x + rect.w * value;
    g.fillCircle(kx, cy, dragging ? 9.0f : 8.0f, t.accent);
    g.fillCircle(kx, cy, 4, rgb(255, 255, 255));
    g.text(label, rect.x + rect.w + 10, cy - 9, 14, t.text, Align::Left, true);
}

bool Slider::handleEvent(const SDL_Event& e) {
    auto setFromX = [this](float x) {
        float v = std::clamp((x - rect.x) / rect.w, 0.0f, 1.0f);
        bool changed = v != value;
        value = v;
        return changed;
    };
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        SDL_FRect grab{rect.x - 10, rect.y, rect.w + 20, rect.h};
        if (pointIn(grab, (float)e.button.x, (float)e.button.y)) {
            dragging = true;
            setFromX((float)e.button.x);
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION && dragging) {
        return setFromX((float)e.motion.x);
    } else if (e.type == SDL_MOUSEBUTTONUP && dragging) {
        dragging = false;
        return true;
    }
    return false;
}
