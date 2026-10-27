// ============================================================================
// TreeVisualizer.h - shared base class for every tree screen.
//
// The base class owns everything that is the SAME for all trees:
//   * window layout (top bar, canvas, control bar)
//   * the input box, buttons, speed slider and keyboard shortcuts
//   * the AnimPlayer (queue of animation steps)
//
// A concrete tree (BSTVis, AVLVis, ...) only has to:
//   1. implement its data structure,
//   2. override onAction()  - run an operation and call step(...) as it goes,
//   3. override buildScene() - describe the current tree as nodes + edges.
// ============================================================================
#pragma once
#include <SDL.h>
#include <random>
#include <string>
#include <vector>
#include "Anim.h"
#include "Gfx.h"
#include "UI.h"

// One entry of the colour key drawn in the canvas corner.
struct LegendItem {
    Hl hl;
    std::string text;
};

// Action ids shared by all visualizers; trees add their own from ActCustom.
enum CommonAction {
    ActInsert = 1, ActDelete, ActSearch, ActClear,
    ActCustom = 100
};

class TreeVisualizer {
public:
    TreeVisualizer(std::string title, std::string subtitle);
    virtual ~TreeVisualizer() = default;

    // Must be called once after construction (virtual calls are not allowed
    // inside a constructor in C++).
    void init();

    virtual void update(float dt);
    virtual void draw(Gfx& g, int winW, int winH);
    virtual void handleEvent(const SDL_Event& e);

    const std::string& title() const { return title_; }

    // ---- testing helpers (used by --selftest and --shot) ----
    bool runScript(const std::string& script);   // e.g. "Insert:50|Insert:30|Search:30"
    void seek(int frame, float t) { player_.seek(frame, t); }
    int frameCount() const { return player_.count(); }
    void finishAnimations() { player_.skipToEnd(); }
    virtual std::string validate() const { return ""; }   // "" = invariants hold
    virtual std::string selfTest(std::mt19937& rng, int ops);

protected:
    // ---- implemented by each tree ----
    virtual void setup() = 0;                                     // buttons and the starting tree
    virtual void onAction(int id, const std::string& input) = 0;  // run an operation
    virtual void buildScene(Frame& f) = 0;                        // describe current tree

    // ---- controls ----
    void addButton(int id, const std::string& label, const std::string& tip = "", bool primary = false);
    void setPlaceholder(const std::string& p) { input_.placeholder = p; }
    // Allow "5 3 8 1" in the input box: Insert / Delete run once per value.
    void allowNumberLists() { numberLists_ = true; }
    std::vector<std::string> splitList(const std::string& input) const;
    bool hasButton(int id) const;

    // ---- recording animation steps ----
    int newId() { return nextId_++; }
    void setHl(int id, Hl h);
    Hl hl(int id) const;
    void clearHl();
    void beginOp(const std::string& header);
    void step(const std::string& msg);
    void done(const std::string& msg);  // final message, then colours return to normal
    void showNow();                     // replace history by the current state
    void setRecording(bool on) { recording_ = on; }   // off = build silently (samples)
    void toast(const std::string& msg); // short error / hint message

    // ---- helpers ----
    static bool parseInt(const std::string& s, int& out);
    static std::vector<int> parseInts(const std::string& s);
    int randInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng_); }

    std::string caption_;                 // shown at the top of the canvas
    std::vector<LegendItem> legend_;
    std::mt19937 rng_{12345};
    AnimPlayer player_;

private:
    void record(const std::string& msg, int kind);
    void layout(Gfx& g, int w, int h);
    void triggerAction(int id);
    void runAction(int id, const std::string& text);
    void drawTopBar(Gfx& g);
    void drawControls(Gfx& g);
    void drawLegend(Gfx& g);
    void setSpeed(float s);

    std::string title_, subtitle_;
    std::vector<Button> buttons_;
    Button btnBack_, btnPlay_, btnStep_;
    InputBox input_;
    Slider speed_;
    int enterAction_ = ActInsert;
    bool numberLists_ = false;
    std::string pendingHeader_;
    std::vector<Hl> hls_;
    int nextId_ = 1;
    bool recording_ = true;
    float time_ = 0, mouseX_ = 0, mouseY_ = 0;
    float legendWidth_ = 0;
    std::string toast_;
    float toastTime_ = 0;
    // layout rectangles (recomputed every frame)
    int winW_ = 1280, winH_ = 800;
    SDL_FRect topBar_{}, canvas_{}, controls_{};
};
