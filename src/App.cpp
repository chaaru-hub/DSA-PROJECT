// ============================================================================
// App.cpp - SDL setup and the 60 FPS main loop
// ============================================================================
#include "App.h"
#include <algorithm>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

bool App::init() {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
#ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_version ver;
    SDL_GetVersion(&ver);
    if (ver.major == 2 && (ver.minor == 0 && ver.patch < 18)) {
        std::fprintf(stderr, "SDL 2.0.18 or newer is required (found %d.%d.%d)\n", ver.major, ver.minor, ver.patch);
        return false;
    }
    // Start at ~90% of the usable screen area.
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
        w_ = std::clamp((int)(usable.w * 0.9f), 1024, 1600);
        h_ = std::clamp((int)(usable.h * 0.9f), 680, 1000);
    }
    Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    window_ = SDL_CreateWindow("Binary Search Tree Visualizer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               w_, h_, flags);
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowMinimumSize(window_, 1000, 650);
    // Hardware accelerated + VSync = smooth, tear-free 60 FPS (double buffered).
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        vsync_ = false;
    }
    if (!renderer_) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer_, &info) == 0) vsync_ = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    if (!gfx_.init(renderer_)) {
        std::fprintf(stderr, "Font texture creation failed: %s\n", SDL_GetError());
        return false;
    }
    bst_.init();
    updatePixelScale();
    SDL_StartTextInput();
    return true;
}

void App::updatePixelScale() {
    SDL_GetWindowSize(window_, &w_, &h_);
    int pw = w_, ph = h_;
    SDL_GetRendererOutputSize(renderer_, &pw, &ph);
    float s = w_ > 0 ? (float)pw / w_ : 1.0f;
    // Draw in window coordinates; on Retina screens everything is scaled 2x
    // automatically, so lines and text stay sharp.
    SDL_RenderSetScale(renderer_, s, s);
    gfx_.setPixelScale(s);
}

void App::render() {
    const Theme& t = theme();
    SDL_SetRenderDrawColor(renderer_, t.bg.r, t.bg.g, t.bg.b, 255);
    SDL_RenderClear(renderer_);
    bst_.draw(gfx_, w_, h_);
    if (showFps_) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "%.0f FPS%s", fps_, vsync_ ? " (VSync)" : "");
        gfx_.fillRoundRect(w_ * 0.5f - 60, 4, 120, 22, 11, withAlpha(rgb(0, 0, 0), 0.6f));
        gfx_.text(buf, w_ * 0.5f, 7, 13, rgb(120, 255, 140), Align::Center, true);
    }
    gfx_.flush();
    SDL_RenderPresent(renderer_);
}

// --autotest: pushes real SDL keyboard / mouse events into the normal main
// loop on a timeline (typing, Enter, pause, step, speed, buttons, theme,
// resize), then quits and reports the average FPS.
void App::injectAutotestEvents(float elapsed) {
    auto key = [](SDL_Keycode k) {
        SDL_Event e{};
        e.type = SDL_KEYDOWN;
        e.key.keysym.sym = k;
        SDL_PushEvent(&e);
    };
    auto text = [](const char* s) {
        SDL_Event e{};
        e.type = SDL_TEXTINPUT;
        SDL_strlcpy(e.text.text, s, sizeof e.text.text);
        SDL_PushEvent(&e);
    };
    auto click = [](int x, int y) {
        SDL_Event e{};
        e.type = SDL_MOUSEBUTTONDOWN;
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = x;
        e.button.y = y;
        SDL_PushEvent(&e);
        e.type = SDL_MOUSEBUTTONUP;
        SDL_PushEvent(&e);
    };
    static const char* inputs[] = {"50 30 70 20 40", "35", "99", "45 10", "60", "5 1 8"};
    const int rounds = 6;
    const float slot = 2.0f;
    int round = (int)(elapsed / slot);
    float in = elapsed - round * slot;
    int phase = in < 0.3f ? 0 : in < 0.7f ? 1 : in < 1.1f ? 2 : in < 1.5f ? 3 : 4;
    int stepId = round * 10 + phase;
    if (stepId == autoStep_) return;
    autoStep_ = stepId;
    if (round >= rounds) {
        if (phase == 0) { toggleTheme(); SDL_SetWindowSize(window_, 1100, 700); }
        if (phase == 2) key(SDLK_F1);
        if (phase == 4) running_ = false;
        return;
    }
    switch (phase) {
    case 0: text(inputs[round]); key(SDLK_RETURN); break;                 // type + Enter (insert)
    case 1: key(SDLK_SPACE); key(SDLK_RIGHT); key(SDLK_RIGHT); break;      // pause, step
    case 2: key(SDLK_LEFT); key(SDLK_UP); key(SDLK_SPACE); break;          // step back, faster, resume
    case 3: click(300, h_ - 40); click(500, h_ - 40); break;               // bottom-bar buttons
    case 4: key(SDLK_DOWN); if (round == 2) { text("30"); key(SDLK_DELETE); } break;
    default: break;
    }
}

#ifdef __EMSCRIPTEN__
// In a web browser the page must return control after every frame, so the
// browser calls this function once per display refresh instead of a loop.
static void browserFrame(void* app) { static_cast<App*>(app)->frame(); }
#endif

void App::run(bool autotest) {
    freq_ = SDL_GetPerformanceFrequency();
    last_ = start_ = SDL_GetPerformanceCounter();
    autotest_ = autotest;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(browserFrame, this, 0, 1);
#else
    while (running_) frame();
#endif
    if (autotest) {
        double secs = (double)(SDL_GetPerformanceCounter() - start_) / freq_;
        std::printf("autotest finished: %ld frames in %.1f s = %.1f FPS average (vsync %s)\n", totalFrames_, secs,
                    totalFrames_ / secs, vsync_ ? "on" : "off");
    }
}

// One iteration of the main loop: events -> update -> render.
void App::frame() {
#ifdef __EMSCRIPTEN__
    // keep the canvas the same size as the page area that holds it (#stage)
    int bw = EM_ASM_INT({ var s = document.getElementById('stage'); return s ? s.clientWidth : window.innerWidth; });
    int bh = EM_ASM_INT({ var s = document.getElementById('stage'); return s ? s.clientHeight : window.innerHeight; });
    static int lastBw = 0, lastBh = 0;
    if (bw > 0 && bh > 0 && (bw != lastBw || bh != lastBh)) {
        lastBw = bw;
        lastBh = bh;
        SDL_SetWindowSize(window_, bw, bh);
        // On scaled displays (125%, Retina) SDL resizes only the canvas's pixel
        // buffer; set its on-screen size too so the picture fills the area and
        // mouse clicks land where they are drawn.
        EM_ASM({
            var c = Module['canvas'];
            c.style.width = $0 + 'px';
            c.style.height = $1 + 'px';
        }, bw, bh);
        updatePixelScale();
    }
#endif
    if (autotest_) injectAutotestEvents((float)(SDL_GetPerformanceCounter() - start_) / freq_);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running_ = false; break; }
        if (e.type == SDL_WINDOWEVENT &&
            (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e.window.event == SDL_WINDOWEVENT_RESIZED)) {
            updatePixelScale();
            continue;
        }
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_F2) { toggleTheme(); continue; }
            if (k == SDLK_F3) { showFps_ = !showFps_; continue; }
            if (k == SDLK_F11) {
                fullscreen_ = !fullscreen_;
                SDL_SetWindowFullscreen(window_, fullscreen_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                continue;
            }
            if (k == SDLK_q && (e.key.keysym.mod & KMOD_CTRL)) { running_ = false; break; }
        }
        bst_.handleEvent(e);
    }
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)(now - last_) / freq_;
    last_ = now;
    dt = std::min(dt, 0.05f);       // avoid huge jumps after a stall
    // frames-per-second counter (F3)
    ++fpsFrames_;
    fpsTimer_ += dt;
    if (fpsTimer_ >= 0.5f) {
        fps_ = fpsFrames_ / fpsTimer_;
        fpsFrames_ = 0;
        fpsTimer_ = 0;
    }
    bst_.update(dt);
    render();
    ++totalFrames_;
#ifndef __EMSCRIPTEN__
    if (!vsync_) {
        // no VSync available: cap at ~60 FPS ourselves
        Uint64 spent = SDL_GetPerformanceCounter() - now;
        double ms = 1000.0 * spent / freq_;
        if (ms < 16.0) SDL_Delay((Uint32)(16.0 - ms));
    }
#endif
}

bool App::screenshot(const std::string& file, const std::string& script, int frame, float t) {
    if (!script.empty() && !bst_.runScript(script)) {
        std::fprintf(stderr, "script failed: %s\n", script.c_str());
        return false;
    }
    if (frame < 0) bst_.finishAnimations();
    else bst_.seek(frame, t);
    bst_.update(0.016f);
    updatePixelScale();
    const Theme& th = theme();
    SDL_SetRenderDrawColor(renderer_, th.bg.r, th.bg.g, th.bg.b, 255);
    SDL_RenderClear(renderer_);
    bst_.draw(gfx_, w_, h_);
    gfx_.flush();
    int pw, ph;
    SDL_GetRendererOutputSize(renderer_, &pw, &ph);
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, pw, ph, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderSetScale(renderer_, 1, 1);
    SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch);
    bool ok = SDL_SaveBMP(s, file.c_str()) == 0;
    SDL_FreeSurface(s);
    return ok;
}

void App::shutdown() {
    gfx_.shutdown();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}
