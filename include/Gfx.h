// ============================================================================
// Gfx.h - tiny 2D drawing layer on top of SDL_Renderer.
//
// Every shape is turned into triangles and sent through SDL_RenderGeometry.
// Anti-aliasing trick: each shape gets an extra 1-pixel "feather" strip whose
// outer vertices are fully transparent, so the GPU blends the edge smoothly.
// Triangles are batched and flushed only when the texture changes (shapes use
// no texture, text uses the font atlas), which keeps the frame rate high.
// ============================================================================
#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include "Theme.h"

enum class Align { Left, Center, Right };

struct Pt { float x, y; };

class Gfx {
public:
    bool init(SDL_Renderer* r);
    void shutdown();
    SDL_Renderer* sdl() const { return r_; }

    // pixelScale = physical pixels per logical pixel (2 on Retina screens).
    void setPixelScale(float s) { pixelScale_ = s; }

    void flush();   // send pending triangles to the GPU

    // ---- shapes (all anti-aliased) ----
    void fillRect(float x, float y, float w, float h, Color c);
    void fillRoundRect(float x, float y, float w, float h, float rad, Color c);
    void strokeRoundRect(float x, float y, float w, float h, float rad, float thick, Color c);
    void fillCircle(float cx, float cy, float r, Color c);
    void ring(float cx, float cy, float r, float thick, Color c);
    void line(float x1, float y1, float x2, float y2, float thick, Color c);

    // ---- text ----
    // y is the TOP of the text line.
    void text(const std::string& s, float x, float y, float size, Color c,
              Align a = Align::Left, bool bold = false);
    // Text centred on (cx, cy) both horizontally and vertically.
    void textCentered(const std::string& s, float cx, float cy, float size, Color c, bool bold = true);
    float textWidth(const std::string& s, float size, bool bold = false) const;
    float lineHeight(float size) const { return size * 1.25f; }
    // Word-wrap a paragraph to a maximum width.
    std::vector<std::string> wrap(const std::string& s, float size, float maxW, bool bold = false) const;

    // ---- clipping (nested) ----
    void pushClip(float x, float y, float w, float h);
    void popClip();

private:
    struct Glyph { SDL_Rect src; float xoff, yoff, adv; };
    struct FontAtlas {
        SDL_Texture* tex = nullptr;
        int pixelSize = 0, ascent = 0;
        bool bold = false;
        Glyph glyphs[95];
    };
    const FontAtlas& pickFont(float size, bool bold) const;
    void useTexture(SDL_Texture* t);
    void vtx(float x, float y, Color c, float u = 0, float v = 0);
    // Convex polygon fill / closed outline with soft (anti-aliased) edges.
    void fillConvex(const std::vector<Pt>& pts, Color c);
    void strokeClosed(const std::vector<Pt>& pts, float thick, Color c);
    void roundRectPath(std::vector<Pt>& out, float x, float y, float w, float h, float rad) const;
    void circlePath(std::vector<Pt>& out, float cx, float cy, float r) const;

    SDL_Renderer* r_ = nullptr;
    float pixelScale_ = 1.0f;
    std::vector<SDL_Vertex> verts_;
    std::vector<int> idx_;
    SDL_Texture* curTex_ = nullptr;
    FontAtlas fonts_[4];
    std::vector<SDL_Rect> clipStack_;
    std::vector<Pt> scratch_;
};
