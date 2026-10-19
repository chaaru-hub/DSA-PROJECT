// ============================================================================
// TreeVisualizer.cpp - layout, controls, step recording, log and info panel
// ============================================================================
#include "TreeVisualizer.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include "Scene.h"

namespace {
enum InternalButton { BtnBack = -2, BtnPlay = -3, BtnStep = -4 };
const float kTopBarH = 60.0f;
}

TreeVisualizer::TreeVisualizer(std::string title, std::string subtitle)
    : title_(std::move(title)), subtitle_(std::move(subtitle)) {
    btnBack_.id = BtnBack;  btnBack_.label = "< Step";
    btnPlay_.id = BtnPlay;  btnPlay_.label = "Pause";
    btnStep_.id = BtnStep;  btnStep_.label = "Step >";
    input_.placeholder = "Type a number...";
    legend_ = {{Hl::Current, "current"}, {Hl::Visited, "path"}, {Hl::Found, "found"},
               {Hl::New, "new / copied"}, {Hl::Remove, "delete"}};
    setSpeed(AnimSettings::speed);
}

void TreeVisualizer::init() {
    setup();
    if (player_.count() <= 1) showNow();
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------
void TreeVisualizer::addButton(int id, const std::string& label, const std::string& tip, bool primary) {
    Button b;
    b.id = id;
    b.label = label;
    b.tip = tip;
    b.primary = primary;
    buttons_.push_back(b);
}

bool TreeVisualizer::hasButton(int id) const {
    for (const Button& b : buttons_)
        if (b.id == id) return true;
    return false;
}

void TreeVisualizer::setSpeed(float s) {
    AnimSettings::speed = std::clamp(s, AnimSettings::kMinSpeed, AnimSettings::kMaxSpeed);
    // the slider is logarithmic: 0.25x at the left, 4x at the right
    speed_.value = std::log(AnimSettings::speed / AnimSettings::kMinSpeed) /
                   std::log(AnimSettings::kMaxSpeed / AnimSettings::kMinSpeed);
}

void TreeVisualizer::triggerAction(int id) {
    // remember which button was used, so it stays highlighted
    for (Button& b : buttons_) {
        if (b.id == id) b.press = 1.0f;
        b.selected = (b.id == id);
    }
    std::string text = input_.text;
    input_.text.clear();
    input_.focused = false;
    runAction(id, text);
}

void TreeVisualizer::runAction(int id, const std::string& text) {
    // A list such as "50 30 70" runs the operation once per value; the
    // animations are queued and play one after another.
    if (id == ActInsert || id == ActDelete) {
        std::vector<std::string> items = splitList(text);
        if (items.size() > 1) {
            for (const std::string& it : items) onAction(id, it);
            return;
        }
    }
    onAction(id, text);
}

std::vector<std::string> TreeVisualizer::splitList(const std::string& input) const {
    std::vector<std::string> out;
    if (numberLists_) {
        for (int v : parseInts(input)) out.push_back(std::to_string(v));
    } else if (!input.empty()) {
        out.push_back(input);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Recording animation steps
// ---------------------------------------------------------------------------
void TreeVisualizer::setHl(int id, Hl h) {
    if (id < 0) return;
    if ((int)hls_.size() <= id) hls_.resize(id + 64, Hl::None);
    hls_[id] = h;
}

Hl TreeVisualizer::hl(int id) const {
    return (id >= 0 && id < (int)hls_.size()) ? hls_[id] : Hl::None;
}

void TreeVisualizer::clearHl() { std::fill(hls_.begin(), hls_.end(), Hl::None); }

void TreeVisualizer::beginOp(const std::string& header) {
    clearHl();
    if (!recording_) return;      // silent operations (samples) have no header
    caption_.clear();
    pendingHeader_ = header;
}

void TreeVisualizer::record(const std::string& msg, int kind) {
    if (!recording_) return;
    Frame f;
    buildScene(f);
    f.header = pendingHeader_;
    pendingHeader_.clear();
    f.log = msg;
    f.logKind = kind;
    f.caption = caption_;
    player_.push(f);
}

void TreeVisualizer::step(const std::string& msg) { record(msg, LogStep); }

void TreeVisualizer::done(const std::string& msg) {
    if (!recording_) { clearHl(); return; }
    record(msg, LogResult);
    clearHl();
    record("", LogStep);     // colours fade back to normal
}

void TreeVisualizer::showNow() {
    clearHl();
    Frame f;
    buildScene(f);
    f.caption = caption_;
    player_.reset(f);
}

void TreeVisualizer::toast(const std::string& msg) {
    toast_ = msg;
    toastTime_ = 2.8f;
}

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------
bool TreeVisualizer::parseInt(const std::string& s, int& out) {
    std::string t;
    for (char c : s)
        if (!std::isspace((unsigned char)c)) t += c;
    if (t.empty() || t.size() > 6) return false;
    size_t i = (t[0] == '-') ? 1 : 0;
    if (i == t.size()) return false;
    for (size_t k = i; k < t.size(); ++k)
        if (!std::isdigit((unsigned char)t[k])) return false;
    out = std::stoi(t);
    return true;
}

std::vector<int> TreeVisualizer::parseInts(const std::string& s) {
    std::vector<int> v;
    std::string cur;
    auto flush = [&]() {
        int x;
        if (parseInt(cur, x)) v.push_back(x);
        cur.clear();
    };
    for (char c : s) {
        if (std::isdigit((unsigned char)c) || (c == '-' && cur.empty())) cur += c;
        else flush();
    }
    flush();
    return v;
}

// ---------------------------------------------------------------------------
// Scripted control (tests and screenshots)
// ---------------------------------------------------------------------------
bool TreeVisualizer::runScript(const std::string& script) {
    std::stringstream ss(script);
    std::string tok;
    while (std::getline(ss, tok, '|')) {
        std::string label = tok, arg;
        size_t c = tok.find(':');
        if (c != std::string::npos) { label = tok.substr(0, c); arg = tok.substr(c + 1); }
        auto lower = [](std::string s) { for (char& ch : s) ch = (char)std::tolower((unsigned char)ch); return s; };
        bool ok = false;
        for (const Button& b : buttons_)
            if (lower(b.label) == lower(label)) { runAction(b.id, arg); ok = true; break; }
        if (!ok) return false;
    }
    return true;
}

std::string TreeVisualizer::selfTest(std::mt19937& rng, int ops) {
    for (int i = 0; i < ops; ++i) {
        int r = (int)(rng() % 10);
        int v = (int)(rng() % 60);
        std::string arg = std::to_string(v);
        if (r < 5 && hasButton(ActInsert)) onAction(ActInsert, arg);
        else if (r < 8 && hasButton(ActDelete)) onAction(ActDelete, arg);
        else if (hasButton(ActSearch)) onAction(ActSearch, arg);
        if (i % 97 == 96 && hasButton(ActClear)) onAction(ActClear, "");
        std::string err = validate();
        if (!err.empty()) return "op " + std::to_string(i) + ": " + err;
        if (player_.count() > 500) showNow();
    }
    return "";
}

// ---------------------------------------------------------------------------
// Update / events
// ---------------------------------------------------------------------------
void TreeVisualizer::update(float dt) {
    time_ += dt;
    player_.update(dt);
    input_.update(dt);
    for (Button& b : buttons_) b.update(dt, mouseX_, mouseY_);
    for (Button* b : {&btnBack_, &btnPlay_, &btnStep_}) b->update(dt, mouseX_, mouseY_);
    btnPlay_.label = player_.paused() ? "Play" : "Pause";
    btnPlay_.active = player_.paused();
    toastTime_ = std::max(0.0f, toastTime_ - dt);
}

void TreeVisualizer::handleEvent(const SDL_Event& e) {
    if (speed_.handleEvent(e)) {
        float s = AnimSettings::kMinSpeed *
                  std::pow(AnimSettings::kMaxSpeed / AnimSettings::kMinSpeed, speed_.value);
        AnimSettings::speed = s;
        return;
    }
    switch (e.type) {
    case SDL_MOUSEMOTION:
        mouseX_ = (float)e.motion.x;
        mouseY_ = (float)e.motion.y;
        break;
    case SDL_MOUSEBUTTONDOWN: {
        if (e.button.button != SDL_BUTTON_LEFT) break;
        float x = (float)e.button.x, y = (float)e.button.y;
        input_.focused = pointIn(input_.rect, x, y);
        if (btnBack_.hit(x, y)) { btnBack_.press = 1; player_.stepBack(); break; }
        if (btnPlay_.hit(x, y)) { btnPlay_.press = 1; player_.togglePause(); break; }
        if (btnStep_.hit(x, y)) { btnStep_.press = 1; player_.stepForward(); break; }
        for (const Button& b : buttons_)
            if (b.hit(x, y)) { triggerAction(b.id); break; }
        break;
    }
    case SDL_MOUSEWHEEL: {
        if (pointIn(infoRect_, mouseX_, mouseY_))
            infoScroll_ = std::clamp(infoScroll_ - e.wheel.y * 40.0f, 0.0f, infoMaxScroll_);
        else if (pointIn(logRect_, mouseX_, mouseY_))
            logScroll_ = std::max(0, logScroll_ + e.wheel.y * 2);
        break;
    }
    case SDL_TEXTINPUT: {
        // Typing anything (except a lone space) starts editing the input box,
        // so "type 17, press Enter" works without clicking first.
        std::string t = e.text.text;
        if (input_.focused) input_.handleText(e.text.text);
        else if (t != " ") { input_.focused = true; input_.handleText(e.text.text); }
        break;
    }
    case SDL_KEYDOWN: {
        SDL_Keycode k = e.key.keysym.sym;
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            if (!input_.text.empty()) triggerAction(enterAction_);
            input_.focused = false;
        } else if (k == SDLK_BACKSPACE) {
            if (input_.focused) input_.handleBackspace();
        } else if (k == SDLK_SPACE) {
            if (!input_.focused) player_.togglePause();
        } else if (k == SDLK_RIGHT) {
            player_.stepForward();
        } else if (k == SDLK_LEFT) {
            player_.stepBack();
        } else if (k == SDLK_UP) {
            setSpeed(AnimSettings::speed * 1.25f);
        } else if (k == SDLK_DOWN) {
            setSpeed(AnimSettings::speed / 1.25f);
        } else if (k == SDLK_END) {
            player_.skipToEnd();
        } else if (k == SDLK_DELETE) {
            if (hasButton(ActDelete) && !input_.text.empty()) triggerAction(ActDelete);
        } else if (k == SDLK_TAB) {
            input_.focused = !input_.focused;
        } else if (k == SDLK_F1) {
            showPanel_ = !showPanel_;
        }
        break;
    }
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void TreeVisualizer::layout(Gfx& g, int w, int h) {
    winW_ = w;
    winH_ = h;
    float panelW = showPanel_ ? std::clamp(w * 0.28f, 300.0f, 440.0f) : 0.0f;
    float leftW = w - panelW;
    topBar_ = SDL_FRect{0, 0, (float)w, kTopBarH};
    panel_ = SDL_FRect{leftW, kTopBarH, panelW, h - kTopBarH};
    float logH = std::max(220.0f, panel_.h * 0.38f);
    infoRect_ = SDL_FRect{panel_.x + 16, panel_.y + 40, panel_.w - 30, panel_.h - logH - 64};
    logRect_ = SDL_FRect{panel_.x + 16, panel_.y + panel_.h - logH + 10, panel_.w - 30, logH - 22};

    // top bar buttons (right aligned playback cluster)
    float x = w - 14 - 150 - 56;         // slider + value label (right edge of the window)
    speed_.rect = SDL_FRect{x, 18, 150, 24};
    x -= 64;                             // "Speed" caption
    for (Button* b : {&btnStep_, &btnPlay_, &btnBack_}) {
        b->rect = SDL_FRect{x - 76, 13, 76, 34};
        x -= 84;
    }

    // control bar: input box followed by buttons, wrapped into rows
    const float pad = 12, rowH = 36, gap = 8;
    float cx = pad, cy = 0;
    int rows = 1;
    std::vector<SDL_FRect> rects;
    float inputW = 210;
    rects.push_back(SDL_FRect{cx, cy, inputW, rowH});
    cx += inputW + gap;
    for (Button& b : buttons_) {
        float bw = std::max(64.0f, g.textWidth(b.label, 14, true) + 30);
        if (cx + bw > leftW - pad) { cx = pad; cy += rowH + gap; ++rows; }
        rects.push_back(SDL_FRect{cx, cy, bw, rowH});
        cx += bw + gap;
    }
    float ctrlH = rows * rowH + (rows - 1) * gap + 2 * pad + 4;
    controls_ = SDL_FRect{0, h - ctrlH, leftW, ctrlH};
    input_.rect = rects[0];
    input_.rect.y += controls_.y + pad;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        buttons_[i].rect = rects[i + 1];
        buttons_[i].rect.y += controls_.y + pad;
    }
    canvas_ = SDL_FRect{12, kTopBarH + 12, leftW - 24, controls_.y - kTopBarH - 18};
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
void TreeVisualizer::draw(Gfx& g, int winW, int winH) {
    const Theme& th = theme();
    layout(g, winW, winH);

    g.fillRect(0, 0, (float)winW, (float)winH, th.bg);

    // ---- canvas with a faint grid ----
    g.fillRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, 12, th.canvas);
    g.pushClip(canvas_.x, canvas_.y, canvas_.w, canvas_.h);
    for (float gx = canvas_.x + 20; gx < canvas_.x + canvas_.w; gx += 40)
        g.fillRect(gx, canvas_.y, 1, canvas_.h, th.canvasGrid);
    for (float gy = canvas_.y + 20; gy < canvas_.y + canvas_.h; gy += 40)
        g.fillRect(canvas_.x, gy, canvas_.w, 1, th.canvasGrid);

    const float bannerH = 48, legendH = 28;
    SDL_FRect sceneArea{canvas_.x, canvas_.y, canvas_.w, canvas_.h - bannerH - legendH};
    drawScene(g, player_.from(), player_.to(), player_.progress(), sceneArea, time_);
    if (player_.to().nodes.empty() && player_.from().nodes.empty())
        g.textCentered("The tree is empty - type a value and press Enter, or click Load sample",
                       canvas_.x + canvas_.w * 0.5f, canvas_.y + canvas_.h * 0.45f, 16, th.textDim, false);

    // ---- step banner: the current explanation in big letters ----
    {
        const Frame& f = player_.to();
        std::string msg = !f.log.empty() ? f.log : f.header;
        if (msg.empty() && player_.current() > 0) {
            // neutral "colours back to normal" frame: keep the previous message
            for (int i = player_.current() - 1; i >= 0 && msg.empty(); --i) {
                const Frame& p = player_.frames()[i];
                msg = !p.log.empty() ? p.log : p.header;
            }
        }
        float by = canvas_.y + canvas_.h - bannerH;
        g.fillRect(canvas_.x, by, canvas_.w, bannerH, withAlpha(th.panelAlt, 0.95f));
        g.fillRect(canvas_.x, by, canvas_.w, 1, th.border);
        Color mc = (f.logKind == LogResult) ? th.visited : th.text;
        if (msg.empty()) { msg = "Ready."; mc = th.textDim; }
        std::string stepTxt = "step " + std::to_string(player_.current()) + " / " +
                              std::to_string(player_.count() - 1);
        float stepW = g.textWidth(stepTxt, 13);
        auto lines = g.wrap(msg, 17, canvas_.w - stepW - 60, true);
        if (lines.size() > 2) lines.resize(2);
        float ly = by + bannerH * 0.5f - lines.size() * 10.0f;
        for (auto& l : lines) { g.text(l, canvas_.x + 18, ly, 17, mc, Align::Left, true); ly += 20; }
        g.text(stepTxt, canvas_.x + canvas_.w - 16, by + bannerH * 0.5f - 8, 13, th.textDim, Align::Right);
        // progress through the queued steps
        float prog = player_.count() > 1 ? (player_.current() - 1 + player_.progress()) / (player_.count() - 1) : 1;
        g.fillRect(canvas_.x, by + bannerH - 3, canvas_.w * std::clamp(prog, 0.0f, 1.0f), 3, th.accent);
    }
    drawLegend(g);
    g.popClip();
    g.strokeRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, 12, 1.2f, th.border);

    // ---- paused indicator (right side of the legend band) ----
    if (player_.paused()) {
        std::string s = "PAUSED  -  Space: resume   Right: next step   Left: previous";
        float w = g.textWidth(s, 13, true) + 24;
        if (canvas_.w - w - 20 < legendWidth_) {
            s = "PAUSED (Space / Right)";
            w = g.textWidth(s, 13, true) + 24;
        }
        float x = canvas_.x + canvas_.w - w - 10, y = canvas_.y + canvas_.h - bannerH - legendH + 1;
        g.fillRoundRect(x, y, w, 26, 13, th.paused);
        g.textCentered(s, x + w * 0.5f, y + 13, 13, rgb(30, 20, 10), true);
    }
    // ---- toast ----
    if (toastTime_ > 0) {
        float a = std::min(1.0f, toastTime_ * 2);
        float w = g.textWidth(toast_, 15, true) + 32;
        float x = canvas_.x + (canvas_.w - w) * 0.5f, y = canvas_.y + 54;
        g.fillRoundRect(x, y, w, 34, 17, withAlpha(th.remove, a));
        g.textCentered(toast_, x + w * 0.5f, y + 17, 15, withAlpha(rgb(255, 255, 255), a), true);
    }

    drawControls(g);
    drawTopBar(g);
    if (showPanel_) drawSidePanel(g);

    // ---- tooltip for hovered button ----
    for (const Button& b : buttons_)
        if (b.hover > 0.9f && !b.tip.empty()) {
            float w = g.textWidth(b.tip, 13) + 20;
            float x = std::clamp(b.rect.x, 4.0f, winW - w - 4), y = b.rect.y - 34;
            g.fillRoundRect(x, y, w, 26, 6, th.panelAlt);
            g.strokeRoundRect(x, y, w, 26, 6, 1, th.border);
            g.text(b.tip, x + 10, y + 5, 13, th.text);
        }
    g.flush();
}

void TreeVisualizer::drawLegend(Gfx& g) {
    const Theme& th = theme();
    float x = canvas_.x + 16, y = canvas_.y + canvas_.h - 48 - 21;
    for (const LegendItem& l : legend_) {
        g.fillCircle(x + 6, y + 7, 6, nodeFillColor(l.hl));
        g.ring(x + 6, y + 7, 6, 1.2f, nodeRingColor(l.hl));
        g.text(l.text, x + 16, y - 1, 12, th.textDim);
        x += 26 + g.textWidth(l.text, 12);
    }
    legendWidth_ = x - canvas_.x;
}

void TreeVisualizer::drawTopBar(Gfx& g) {
    const Theme& th = theme();
    g.fillRect(topBar_.x, topBar_.y, topBar_.w, topBar_.h, th.topBar);
    g.fillRect(topBar_.x, topBar_.h - 1, topBar_.w, 1, th.border);
    g.pushClip(0, 0, btnBack_.rect.x - 12, topBar_.h);
    g.text(title_, 18, 9, 22, th.text, Align::Left, true);
    g.text(subtitle_, 19, 36, 13, th.textDim);
    g.popClip();
    btnBack_.draw(g);
    btnPlay_.draw(g);
    btnStep_.draw(g);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.2fx", AnimSettings::speed);
    speed_.draw(g, buf);
    g.text("Speed", speed_.rect.x - 14, 21, 13, th.textDim, Align::Right);

}

void TreeVisualizer::drawControls(Gfx& g) {
    const Theme& th = theme();
    g.fillRect(controls_.x, controls_.y, controls_.w, controls_.h, th.topBar);
    g.fillRect(controls_.x, controls_.y, controls_.w, 1, th.border);
    input_.draw(g);
    for (const Button& b : buttons_) b.draw(g);
}

void TreeVisualizer::drawSidePanel(Gfx& g) {
    const Theme& th = theme();
    g.fillRect(panel_.x, panel_.y, panel_.w, panel_.h, th.panel);
    g.fillRect(panel_.x, panel_.y, 1, panel_.h, th.border);
    g.text("ABOUT THIS TREE", panel_.x + 16, panel_.y + 14, 13, th.accent, Align::Left, true);
    const char* hint = "F1 hide  F2 theme  F3 FPS";
    if (g.textWidth("ABOUT THIS TREE", 13, true) + g.textWidth(hint, 12) + 48 < panel_.w)
        g.text(hint, panel_.x + panel_.w - 16, panel_.y + 14, 12, th.textDim, Align::Right);
    drawInfo(g, infoRect_);
    float ly = logRect_.y - 30;
    g.fillRect(panel_.x, ly - 8, panel_.w, 1, th.border);
    g.text("STEP LOG", panel_.x + 16, ly, 13, th.accent, Align::Left, true);
    g.text("wheel to scroll", panel_.x + panel_.w - 16, ly, 12, th.textDim, Align::Right);
    drawLog(g, logRect_);
}

void TreeVisualizer::drawInfo(Gfx& g, const SDL_FRect& r) {
    const Theme& th = theme();
    g.pushClip(r.x - 4, r.y - 4, r.w + 8, r.h + 4);
    float y = r.y - infoScroll_;
    const float body = 14, lh = 19;
    auto heading = [&](const std::string& s) {
        y += 6;
        g.text(s, r.x, y, 15, th.text, Align::Left, true);
        y += 22;
    };
    auto para = [&](const std::string& s, float indent) {
        for (auto& l : g.wrap(s, body, r.w - indent)) {
            g.text(l, r.x + indent, y, body, th.textDim);
            y += lh;
        }
    };
    auto bullets = [&](const std::vector<std::string>& v) {
        for (auto& s : v) {
            g.fillCircle(r.x + 5, y + 9, 2.6f, th.accent);
            para(s, 16);
            y += 3;
        }
    };
    heading("Definition");
    para(info_.definition, 0);
    if (!info_.properties.empty()) { heading("Key properties"); bullets(info_.properties); }
    if (!info_.complexity.empty()) {
        heading("Time complexity");
        float c0 = r.w * 0.34f, cw = (r.w - c0) / 3;
        auto row = [&](const ComplexityRow& c, bool head, int i) {
            if (!head && i % 2 == 0) g.fillRect(r.x, y - 2, r.w, 22, th.panelAlt);
            Color col = head ? th.text : th.textDim;
            g.text(c.op, r.x + 6, y, 13, th.text, Align::Left, head);
            g.text(c.best, r.x + c0 + cw * 0.5f, y, 13, col, Align::Center, head);
            g.text(c.avg, r.x + c0 + cw * 1.5f, y, 13, col, Align::Center, head);
            g.text(c.worst, r.x + c0 + cw * 2.5f, y, 13, col, Align::Center, head);
            y += 22;
        };
        row({"Operation", "Best", "Average", "Worst"}, true, 0);
        g.fillRect(r.x, y - 3, r.w, 1, th.border);
        int i = 0;
        for (auto& c : info_.complexity) row(c, false, i++);
        if (!info_.space.empty()) { y += 4; para("Space: " + info_.space, 0); }
    }
    if (!info_.uses.empty()) { heading("Real-world uses"); bullets(info_.uses); }
    if (!info_.tips.empty()) { heading("Using this screen"); bullets(info_.tips); }
    float total = y + infoScroll_ - r.y;
    infoMaxScroll_ = std::max(0.0f, total - r.h + 10);
    infoScroll_ = std::min(infoScroll_, infoMaxScroll_);
    g.popClip();
    if (infoMaxScroll_ > 0) {
        // scroll bar
        float frac = r.h / total;
        float bh = std::max(30.0f, r.h * frac);
        float by = r.y + (r.h - bh) * (infoScroll_ / infoMaxScroll_);
        g.fillRoundRect(r.x + r.w + 6, by, 4, bh, 2, th.border);
    }
}

void TreeVisualizer::drawLog(Gfx& g, const SDL_FRect& r) {
    const Theme& th = theme();
    struct Line { std::string text; int kind; bool current; };
    std::vector<Line> lines;
    const auto& frames = player_.frames();
    int cur = player_.current();
    int start = std::max(0, cur - 250);
    int lastWithText = -1;
    for (int i = cur; i >= 0; --i)
        if (!frames[i].log.empty() || !frames[i].header.empty()) { lastWithText = i; break; }
    for (int i = start; i <= cur; ++i) {
        const Frame& f = frames[i];
        bool isCur = (i == lastWithText);
        if (!f.header.empty())
            for (auto& l : g.wrap(f.header, 13, r.w - 12, true)) lines.push_back({l, LogHeader, false});
        if (!f.log.empty()) {
            for (auto& l : g.wrap(f.log, 13, r.w - 22, f.logKind == LogResult))
                lines.push_back({l, f.logKind, isCur});
        }
    }
    const float lh = 18;
    int visible = (int)(r.h / lh);
    int maxScroll = std::max(0, (int)lines.size() - visible);
    logScroll_ = std::min(logScroll_, maxScroll);
    int end = (int)lines.size() - logScroll_;
    int begin = std::max(0, end - visible);
    g.pushClip(r.x - 4, r.y, r.w + 8, r.h);
    float y = r.y;
    for (int i = begin; i < end; ++i) {
        const Line& L = lines[i];
        if (L.current) g.fillRect(r.x - 4, y - 1, r.w + 8, lh, withAlpha(th.current, 0.18f));
        if (L.kind == LogHeader) {
            g.text(L.text, r.x, y, 13, th.accent, Align::Left, true);
        } else {
            Color c = L.kind == LogResult ? th.visited : (L.current ? th.text : th.textDim);
            g.text(L.text, r.x + 10, y, 13, c, Align::Left, L.kind == LogResult);
        }
        y += lh;
    }
    g.popClip();
}
