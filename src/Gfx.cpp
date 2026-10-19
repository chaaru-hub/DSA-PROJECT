// ============================================================================
// Gfx.cpp - anti-aliased shapes and embedded-font text via SDL_RenderGeometry
// ============================================================================
#include "Gfx.h"
#include "Font.h"
#include <algorithm>
#include <cmath>

static const float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Font atlas creation: unpack the embedded hex bitmaps into one texture per size
// ---------------------------------------------------------------------------
bool Gfx::init(SDL_Renderer* r) {
    r_ = r;
    for (int f = 0; f < kNumEmbeddedFonts; ++f) {
        const FontDesc& d = kEmbeddedFonts[f];
        FontAtlas& A = fonts_[f];
        A.pixelSize = d.pixelSize;
        A.ascent = d.ascent;
        A.bold = d.bold;

        // Simple shelf packing into a 512-pixel wide texture.
        const int W = 512;
        int x = 1, y = 1, rowH = 0;
        for (int i = 0; i < 95; ++i) {
            const GlyphInfo& gi = d.info[i];
            if (x + gi.w + 1 > W) { x = 1; y += rowH + 1; rowH = 0; }
            A.glyphs[i].src = SDL_Rect{x, y, gi.w, gi.h};
            A.glyphs[i].xoff = gi.xoff;
            A.glyphs[i].yoff = gi.yoff;
            A.glyphs[i].adv = gi.adv16 / 16.0f;
            x += gi.w + 1;
            rowH = std::max(rowH, (int)gi.h);
        }
        int H = y + rowH + 1;
        std::vector<Uint32> pixels((size_t)W * H, 0x00FFFFFFu);   // white, transparent
        for (int i = 0; i < 95; ++i) {
            const GlyphInfo& gi = d.info[i];
            const char* bits = d.bits[i];
            const SDL_Rect& s = A.glyphs[i].src;
            for (int gy = 0; gy < gi.h; ++gy)
                for (int gx = 0; gx < gi.w; ++gx) {
                    char ch = bits[gy * gi.w + gx];
                    int v = (ch >= 'A') ? ch - 'A' + 10 : ch - '0';
                    Uint32 alpha = (Uint32)(v * 17);
                    pixels[(size_t)(s.y + gy) * W + (s.x + gx)] = (alpha << 24) | 0x00FFFFFFu;
                }
        }
        A.tex = SDL_CreateTexture(r_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H);
        if (!A.tex) return false;
        SDL_UpdateTexture(A.tex, nullptr, pixels.data(), W * 4);
        SDL_SetTextureBlendMode(A.tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(A.tex, SDL_ScaleModeLinear);
    }
    return true;
}

void Gfx::shutdown() {
    for (auto& f : fonts_)
        if (f.tex) { SDL_DestroyTexture(f.tex); f.tex = nullptr; }
}

// ---------------------------------------------------------------------------
// Batching
// ---------------------------------------------------------------------------
void Gfx::flush() {
    if (!idx_.empty()) {
        SDL_RenderGeometry(r_, curTex_, verts_.data(), (int)verts_.size(), idx_.data(), (int)idx_.size());
    }
    verts_.clear();
    idx_.clear();
}

void Gfx::useTexture(SDL_Texture* t) {
    if (t != curTex_) {
        flush();
        curTex_ = t;
    }
}

void Gfx::vtx(float x, float y, Color c, float u, float v) {
    SDL_Vertex V;
    V.position = SDL_FPoint{x, y};
    V.color = SDL_Color{c.r, c.g, c.b, c.a};
    V.tex_coord = SDL_FPoint{u, v};
    verts_.push_back(V);
}

// ---------------------------------------------------------------------------
// Anti-aliased convex polygon.
//   The polygon is shrunk by half a pixel (opaque) and grown by half a pixel
//   (transparent); the strip between gives a smooth 1-pixel edge.
// ---------------------------------------------------------------------------
static void vertexNormals(const std::vector<Pt>& p, std::vector<Pt>& n) {
    size_t N = p.size();
    n.resize(N);
    // Orientation: sign of the polygon area decides which side is "outside".
    float area = 0;
    for (size_t i = 0; i < N; ++i) {
        const Pt& a = p[i];
        const Pt& b = p[(i + 1) % N];
        area += a.x * b.y - b.x * a.y;
    }
    float sgn = area >= 0 ? 1.0f : -1.0f;
    for (size_t i = 0; i < N; ++i) {
        const Pt& prev = p[(i + N - 1) % N];
        const Pt& cur = p[i];
        const Pt& next = p[(i + 1) % N];
        auto edgeN = [sgn](Pt a, Pt b) {
            float dx = b.x - a.x, dy = b.y - a.y;
            float l = std::sqrt(dx * dx + dy * dy);
            if (l < 1e-6f) return Pt{0, 0};
            return Pt{sgn * dy / l, -sgn * dx / l};
        };
        Pt n1 = edgeN(prev, cur), n2 = edgeN(cur, next);
        Pt m{n1.x + n2.x, n1.y + n2.y};
        float l = std::sqrt(m.x * m.x + m.y * m.y);
        if (l < 1e-6f) { n[i] = n2; continue; }
        m.x /= l; m.y /= l;
        // miter: keep the offset distance constant along both edges
        float d = m.x * n2.x + m.y * n2.y;
        float s = d > 0.5f ? 1.0f / d : 2.0f;
        n[i] = Pt{m.x * s, m.y * s};
    }
}

void Gfx::fillConvex(const std::vector<Pt>& p, Color c) {
    if (p.size() < 3 || c.a == 0) return;
    useTexture(nullptr);
    std::vector<Pt> n;
    vertexNormals(p, n);
    float f = 0.5f / pixelScale_;      // half feather width
    Color clear = withAlpha(c, 0.0f);
    int N = (int)p.size();
    int base = (int)verts_.size();
    float cx = 0, cy = 0;
    for (auto& q : p) { cx += q.x; cy += q.y; }
    cx /= N; cy /= N;
    vtx(cx, cy, c);
    for (int i = 0; i < N; ++i) {
        vtx(p[i].x - n[i].x * f, p[i].y - n[i].y * f, c);       // inner (opaque)
        vtx(p[i].x + n[i].x * f, p[i].y + n[i].y * f, clear);   // outer (transparent)
    }
    for (int i = 0; i < N; ++i) {
        int j = (i + 1) % N;
        int in_i = base + 1 + i * 2, out_i = in_i + 1;
        int in_j = base + 1 + j * 2, out_j = in_j + 1;
        idx_.insert(idx_.end(), {base, in_i, in_j});
        idx_.insert(idx_.end(), {in_i, out_i, out_j, in_i, out_j, in_j});
    }
}

void Gfx::strokeClosed(const std::vector<Pt>& p, float thick, Color c) {
    if (p.size() < 3 || c.a == 0) return;
    useTexture(nullptr);
    std::vector<Pt> n;
    vertexNormals(p, n);
    float f = 1.0f / pixelScale_;
    if (thick < f) { c = withAlpha(c, thick / f); thick = f; }
    float h = thick * 0.5f;
    Color clear = withAlpha(c, 0.0f);
    // four offset rings: outer-transparent, outer-opaque, inner-opaque, inner-transparent
    const float off[4] = {h + f * 0.5f, h - f * 0.5f, -h + f * 0.5f, -h - f * 0.5f};
    int N = (int)p.size();
    int base = (int)verts_.size();
    for (int i = 0; i < N; ++i)
        for (int k = 0; k < 4; ++k)
            vtx(p[i].x + n[i].x * off[k], p[i].y + n[i].y * off[k], (k == 0 || k == 3) ? clear : c);
    for (int i = 0; i < N; ++i) {
        int j = (i + 1) % N;
        for (int k = 0; k < 3; ++k) {
            int a = base + i * 4 + k, b = base + i * 4 + k + 1;
            int cc = base + j * 4 + k, d = base + j * 4 + k + 1;
            idx_.insert(idx_.end(), {a, b, d, a, d, cc});
        }
    }
}

void Gfx::circlePath(std::vector<Pt>& out, float cx, float cy, float r) const {
    int segs = std::clamp((int)(r * pixelScale_ * 0.9f) + 12, 16, 128);
    out.clear();
    for (int i = 0; i < segs; ++i) {
        float a = 2 * kPi * i / segs;
        out.push_back(Pt{cx + std::cos(a) * r, cy + std::sin(a) * r});
    }
}

void Gfx::roundRectPath(std::vector<Pt>& out, float x, float y, float w, float h, float rad) const {
    out.clear();
    rad = std::max(0.0f, std::min(rad, std::min(w, h) * 0.5f));
    if (rad < 0.5f) {
        out = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
        return;
    }
    int segs = std::clamp((int)(rad * pixelScale_ * 0.5f) + 3, 3, 16);
    const float cxs[4] = {x + w - rad, x + w - rad, x + rad, x + rad};
    const float cys[4] = {y + rad, y + h - rad, y + h - rad, y + rad};
    const float start[4] = {-kPi / 2, 0, kPi / 2, kPi};
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i <= segs; ++i) {
            float a = start[c] + (kPi / 2) * i / segs;
            out.push_back(Pt{cxs[c] + std::cos(a) * rad, cys[c] + std::sin(a) * rad});
        }
}

// ---------------------------------------------------------------------------
// Public shapes
// ---------------------------------------------------------------------------
void Gfx::fillRect(float x, float y, float w, float h, Color c) {
    if (c.a == 0 || w <= 0 || h <= 0) return;
    useTexture(nullptr);
    int b = (int)verts_.size();
    vtx(x, y, c); vtx(x + w, y, c); vtx(x + w, y + h, c); vtx(x, y + h, c);
    idx_.insert(idx_.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
}

void Gfx::fillRoundRect(float x, float y, float w, float h, float rad, Color c) {
    if (w <= 0 || h <= 0) return;
    roundRectPath(scratch_, x, y, w, h, rad);
    fillConvex(scratch_, c);
}

void Gfx::strokeRoundRect(float x, float y, float w, float h, float rad, float thick, Color c) {
    if (w <= 0 || h <= 0) return;
    roundRectPath(scratch_, x, y, w, h, rad);
    strokeClosed(scratch_, thick, c);
}

void Gfx::fillCircle(float cx, float cy, float r, Color c) {
    if (r <= 0.1f) return;
    circlePath(scratch_, cx, cy, r);
    fillConvex(scratch_, c);
}

void Gfx::ring(float cx, float cy, float r, float thick, Color c) {
    if (r <= 0.1f) return;
    circlePath(scratch_, cx, cy, r);
    strokeClosed(scratch_, thick, c);
}

void Gfx::line(float x1, float y1, float x2, float y2, float thick, Color c) {
    if (c.a == 0) return;
    float dx = x2 - x1, dy = y2 - y1;
    float l = std::sqrt(dx * dx + dy * dy);
    if (l < 1e-4f) return;
    useTexture(nullptr);
    float f = 1.0f / pixelScale_;
    if (thick < f) { c = withAlpha(c, thick / f); thick = f; }
    float nx = -dy / l, ny = dx / l;
    float h = thick * 0.5f;
    Color clear = withAlpha(c, 0.0f);
    const float off[4] = {h + f * 0.5f, h - f * 0.5f, -h + f * 0.5f, -h - f * 0.5f};
    int b = (int)verts_.size();
    for (int k = 0; k < 4; ++k) {
        Color col = (k == 0 || k == 3) ? clear : c;
        vtx(x1 + nx * off[k], y1 + ny * off[k], col);
        vtx(x2 + nx * off[k], y2 + ny * off[k], col);
    }
    for (int k = 0; k < 3; ++k) {
        int a0 = b + k * 2, a1 = a0 + 1, b0 = a0 + 2, b1 = a0 + 3;
        idx_.insert(idx_.end(), {a0, a1, b1, a0, b1, b0});
    }
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
const Gfx::FontAtlas& Gfx::pickFont(float size, bool bold) const {
    float want = size * pixelScale_ * 0.95f;
    const FontAtlas* best = nullptr;
    const FontAtlas* largest = nullptr;
    for (const auto& f : fonts_) {
        if (f.bold != bold) continue;
        if (!largest || f.pixelSize > largest->pixelSize) largest = &f;
        if (f.pixelSize >= want && (!best || f.pixelSize < best->pixelSize)) best = &f;
    }
    return best ? *best : *largest;
}

float Gfx::textWidth(const std::string& s, float size, bool bold) const {
    const FontAtlas& F = pickFont(size, bold);
    float sc = size / F.pixelSize, w = 0;
    for (unsigned char ch : s) {
        if (ch < 32 || ch > 126) ch = '?';
        w += F.glyphs[ch - 32].adv * sc;
    }
    return w;
}

void Gfx::text(const std::string& s, float x, float y, float size, Color c, Align a, bool bold) {
    if (s.empty() || c.a == 0) return;
    const FontAtlas& F = pickFont(size, bold);
    float sc = size / F.pixelSize;
    if (a != Align::Left) {
        float w = textWidth(s, size, bold);
        x -= (a == Align::Center) ? w * 0.5f : w;
    }
    // snap to the pixel grid for crisp text
    x = std::round(x * pixelScale_) / pixelScale_;
    y = std::round(y * pixelScale_) / pixelScale_;
    useTexture(F.tex);
    int tw = 0, th = 0;
    SDL_QueryTexture(F.tex, nullptr, nullptr, &tw, &th);
    float pen = x;
    for (unsigned char ch : s) {
        if (ch < 32 || ch > 126) ch = '?';
        const Glyph& g = F.glyphs[ch - 32];
        if (g.src.w > 0) {
            float gx = pen + g.xoff * sc, gy = y + g.yoff * sc;
            float gw = g.src.w * sc, gh = g.src.h * sc;
            float u0 = (float)g.src.x / tw, v0 = (float)g.src.y / th;
            float u1 = (float)(g.src.x + g.src.w) / tw, v1 = (float)(g.src.y + g.src.h) / th;
            int b = (int)verts_.size();
            vtx(gx, gy, c, u0, v0);
            vtx(gx + gw, gy, c, u1, v0);
            vtx(gx + gw, gy + gh, c, u1, v1);
            vtx(gx, gy + gh, c, u0, v1);
            idx_.insert(idx_.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
        }
        pen += g.adv * sc;
    }
}

void Gfx::textCentered(const std::string& s, float cx, float cy, float size, Color c, bool bold) {
    const FontAtlas& F = pickFont(size, bold);
    const Glyph& H = F.glyphs['H' - 32];
    float capCenter = (H.yoff + H.src.h * 0.5f) * (size / F.pixelSize);
    text(s, cx, cy - capCenter, size, c, Align::Center, bold);
}

std::vector<std::string> Gfx::wrap(const std::string& s, float size, float maxW, bool bold) const {
    std::vector<std::string> lines;
    std::string cur, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && textWidth(trial, size, bold) > maxW) {
            lines.push_back(cur);
            cur = word;
        } else {
            cur = trial;
        }
        word.clear();
    };
    for (char ch : s) {
        if (ch == ' ') flushWord();
        else if (ch == '\n') { flushWord(); lines.push_back(cur); cur.clear(); }
        else word += ch;
    }
    flushWord();
    if (!cur.empty() || lines.empty()) lines.push_back(cur);
    return lines;
}

// ---------------------------------------------------------------------------
// Clipping
// ---------------------------------------------------------------------------
void Gfx::pushClip(float x, float y, float w, float h) {
    flush();
    SDL_Rect r{(int)std::floor(x), (int)std::floor(y), (int)std::ceil(w), (int)std::ceil(h)};
    if (!clipStack_.empty()) {
        SDL_Rect out;
        if (!SDL_IntersectRect(&r, &clipStack_.back(), &out)) out = SDL_Rect{0, 0, 0, 0};
        r = out;
    }
    clipStack_.push_back(r);
    SDL_RenderSetClipRect(r_, &r);
}

void Gfx::popClip() {
    flush();
    if (!clipStack_.empty()) clipStack_.pop_back();
    if (clipStack_.empty()) SDL_RenderSetClipRect(r_, nullptr);
    else SDL_RenderSetClipRect(r_, &clipStack_.back());
}
