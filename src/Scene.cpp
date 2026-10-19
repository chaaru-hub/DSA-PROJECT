// ============================================================================
// Scene.cpp - renders the blend of two frames: gliding nodes, fading edges,
//             auto-fit camera and the caption line.
// ============================================================================
#include "Scene.h"
#include <algorithm>
#include <cmath>

namespace {

// Bounding box of a frame in layout units.
struct Bounds { float minX = 0, maxX = 0, minY = 0, maxY = 0; bool valid = false; };

Bounds boundsOf(const Frame& f) {
    Bounds b;
    for (const VNode& n : f.nodes) {
        if (!b.valid) { b.minX = b.maxX = n.x; b.minY = b.maxY = n.y; b.valid = true; }
        b.minX = std::min(b.minX, n.x); b.maxX = std::max(b.maxX, n.x);
        b.minY = std::min(b.minY, n.y); b.maxY = std::max(b.maxY, n.y);
    }
    b.minX -= 0.5f;   // half a column of margin on each side
    b.maxX += 0.5f;
    return b;
}

float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Maps layout units to screen pixels.
struct Camera {
    float sx = 50, sy = 80, midX = 0, minY = 0, cx = 0, top = 0, r = 20;
    float X(float x) const { return cx + (x - midX) * sx; }
    float Y(float y) const { return top + (y - minY + 0.5f) * sy; }
};

// A node after blending frame A and frame B.
struct BNode {
    const VNode* a = nullptr;      // the node in the old frame (if it existed)
    const VNode* b = nullptr;      // the node in the new frame (if it exists)
    float x = 0, y = 0, alpha = 1, scale = 1;
    Color fill, ring;
    Hl hl = Hl::None;
    int id() const { return b ? b->id : a->id; }
};

const VNode* findNode(const std::vector<VNode>& v, int id) {
    for (const VNode& x : v)
        if (x.id == id) return &x;
    return nullptr;
}

bool hasEdge(const std::vector<VEdge>& v, const VEdge& e) {
    for (const VEdge& x : v)
        if (x.from == e.from && x.to == e.to) return true;
    return false;
}

Color textOn(Color fill) {
    return luminance(fill) > 0.55f ? rgb(20, 24, 32) : rgb(255, 255, 255);
}

// Draws text that cross-fades when it changes between two frames
// (e.g. a key replaced by its inorder successor during delete).
void crossText(Gfx& g, const std::string& sa, const std::string& sb, float e, float cx, float cy,
               float size, Color c, float maxW) {
    auto one = [&](const std::string& s, float alpha) {
        if (s.empty() || alpha <= 0.01f) return;
        float sz = size;
        float w = g.textWidth(s, sz, true);
        if (w > maxW) sz = std::max(8.0f, sz * maxW / w);
        g.textCentered(s, cx, cy, sz, withAlpha(c, alpha), true);
    };
    if (sa == sb) one(sb, 1);
    else { one(sa, 1 - e); one(sb, e); }
}

} // namespace

void drawScene(Gfx& g, const Frame& A, const Frame& B, float t, const SDL_FRect& area, float time) {
    const Theme& th = theme();
    float e = easeInOutCubic(t);

    // ---- region for the tree (below the caption line) ----
    bool hasCaption = !B.caption.empty() || !A.caption.empty();
    float capH = hasCaption ? 40.0f : 8.0f;
    SDL_FRect tree{area.x + 30, area.y + capH + 14, area.w - 60, area.h - capH - 40};

    // ---- camera: fit the tree, interpolated so zoom changes are smooth ----
    Bounds ba = boundsOf(A), bb = boundsOf(B);
    if (!ba.valid) ba = bb;
    if (!bb.valid) bb = ba;
    Camera cam;
    if (bb.valid) {
        float minX = lerp(ba.minX, bb.minX, e), maxX = lerp(ba.maxX, bb.maxX, e);
        float minY = lerp(ba.minY, bb.minY, e), maxY = lerp(ba.maxY, bb.maxY, e);
        float bw = std::max(maxX - minX, 1.0f);
        float bh = (maxY - minY) + 1.0f;
        cam.sx = std::min(72.0f, tree.w / bw);
        cam.sy = std::min(100.0f, tree.h / bh);
        cam.midX = (minX + maxX) * 0.5f;
        cam.minY = minY;
        cam.cx = tree.x + tree.w * 0.5f;
        cam.top = tree.y + std::max(0.0f, (tree.h - bh * cam.sy) * 0.25f);
        cam.r = std::clamp(std::min(cam.sx * 0.42f, cam.sy * 0.30f), 9.0f, 26.0f);
    }

    // ---- blend nodes (matched by id between the two frames) ----
    std::vector<BNode> nodes;
    for (const VNode& nb : B.nodes) {
        BNode n;
        n.b = &nb;
        n.a = findNode(A.nodes, nb.id);
        n.hl = nb.hl;
        if (n.a) {
            // existing node: glides from its old position to the new one
            n.x = lerp(cam.X(n.a->x), cam.X(nb.x), e);
            n.y = lerp(cam.Y(n.a->y), cam.Y(nb.y), e);
            n.fill = lerpColor(nodeFillColor(n.a->hl), nodeFillColor(nb.hl), e);
            n.ring = lerpColor(nodeRingColor(n.a->hl), nodeRingColor(nb.hl), e);
        } else {
            // new node: grows (with a small overshoot) and fades in
            n.x = cam.X(nb.x);
            n.y = cam.Y(nb.y);
            n.alpha = clamp01(t * 1.6f);
            n.scale = easeOutBack(t);
            n.fill = nodeFillColor(nb.hl);
            n.ring = nodeRingColor(nb.hl);
        }
        nodes.push_back(n);
    }
    for (const VNode& na : A.nodes) {
        if (findNode(B.nodes, na.id)) continue;
        // removed node: shrinks and fades out
        BNode n;
        n.a = &na;
        n.x = cam.X(na.x);
        n.y = cam.Y(na.y);
        n.alpha = 1 - e;
        n.scale = 1 - 0.5f * e;
        n.hl = na.hl;
        n.fill = nodeFillColor(na.hl);
        n.ring = nodeRingColor(na.hl);
        nodes.push_back(n);
    }
    auto findBlended = [&](int id) -> const BNode* {
        for (const BNode& n : nodes)
            if (n.id() == id) return &n;
        return nullptr;
    };

    // ---- edges: drawn between the CURRENT animated positions, so they
    //      follow the nodes; new edges fade in, removed edges fade out ----
    auto drawEdge = [&](const VEdge& ed, float alpha) {
        const BNode* p = findBlended(ed.from);
        const BNode* c = findBlended(ed.to);
        if (!p || !c) return;
        alpha *= std::min(p->alpha, c->alpha);
        g.line(p->x, p->y, c->x, c->y, 2.0f, withAlpha(th.edge, alpha));
    };
    for (const VEdge& ed : B.edges) drawEdge(ed, hasEdge(A.edges, ed) ? 1.0f : e);
    for (const VEdge& ed : A.edges)
        if (!hasEdge(B.edges, ed)) drawEdge(ed, 1 - e);

    // ---- nodes (on top of the edges) ----
    for (const BNode& n : nodes) {
        float a = n.alpha;
        float r = cam.r * n.scale;
        if (r < 0.5f || a <= 0.01f) continue;
        bool highlighted = n.hl != Hl::None;
        if (n.hl == Hl::Current) {
            // pulsing glow around the node being compared
            float pulse = 0.5f + 0.5f * std::sin(time * 6.0f);
            g.fillCircle(n.x, n.y, r + 5 + 3 * pulse, withAlpha(hlColor(n.hl), a * (0.18f + 0.12f * pulse)));
        } else if (highlighted) {
            g.fillCircle(n.x, n.y, r + 5, withAlpha(hlColor(n.hl), a * 0.18f));
        }
        g.fillCircle(n.x + 1.5f, n.y + 3, r, withAlpha(th.shadow, a));      // drop shadow
        g.fillCircle(n.x, n.y, r, withAlpha(n.fill, a));
        g.ring(n.x, n.y, r, highlighted ? 3.0f : 2.0f, withAlpha(n.ring, a));
        std::string la = n.a ? n.a->label : n.b->label;
        std::string lb = n.b ? n.b->label : n.a->label;
        crossText(g, la, lb, e, n.x, n.y, r * 0.85f, withAlpha(textOn(n.fill), a), r * 1.75f);
    }

    // ---- caption (traversal output) ----
    if (hasCaption) {
        if (B.caption != A.caption && !A.caption.empty())
            g.text(A.caption, area.x + 18, area.y + 14, 18, withAlpha(th.text, 1 - e), Align::Left, true);
        g.text(B.caption, area.x + 18, area.y + 14, 18,
               withAlpha(th.text, B.caption != A.caption ? e : 1.0f), Align::Left, true);
    }
}
