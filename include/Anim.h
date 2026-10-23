// ============================================================================
// Anim.h - easing functions, animation frames and the frame player.
//
// HOW THE ANIMATION SYSTEM WORKS (good thing to explain in the viva!)
//   1. When you press "Insert 17", the real algorithm runs immediately on the
//      real data structure.
//   2. At every interesting moment the algorithm calls step("17 > 10, go right").
//      step() takes a SNAPSHOT (a Frame) of the tree: every node's position,
//      label and colour, every edge, plus the log message.
//   3. Frames are appended to a queue inside AnimPlayer.
//   4. Every video frame, AnimPlayer moves from one snapshot to the next using
//      an ease-in-out curve. Nodes are matched by their unique id, so a node
//      that changes position GLIDES there, a new node GROWS in and a removed
//      node FADES out.
//   Because the steps are stored, pause / single-step / step-back / speed
//   control are trivial and always reliable.
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include "Theme.h"

// ---- easing -----------------------------------------------------------------
float easeInOutCubic(float t);
float easeOutBack(float t);     // small overshoot: nice "pop" for new nodes
float clamp01(float t);

// ---- frame content ----------------------------------------------------------
// A node in the picture. x/y are LAYOUT units (in-order column, depth); the
// scene renderer scales them to fit the window.
struct VNode {
    int id = 0;
    float x = 0, y = 0;
    std::string label;       // the key shown inside the node
    Hl hl = Hl::None;        // colour role in this step
};

// A parent -> child link.
struct VEdge {
    int from = 0, to = 0;
};

enum LogKind { LogStep = 0, LogHeader = 1, LogResult = 2 };

// One snapshot of the whole picture plus the explanation of this step.
struct Frame {
    std::vector<VNode> nodes;
    std::vector<VEdge> edges;
    std::string header;              // operation title, e.g. "INSERT 17" (first frame of an op)
    std::string log;                 // plain-English explanation of this step
    int logKind = LogStep;
    std::string caption;             // big line at the top of the canvas (traversal output)
};

// ---- global animation settings ------------------------------------------------
struct AnimSettings {
    static float speed;              // 0.25x .. 4x, shared by all visualizers
    static constexpr float kMinSpeed = 0.25f, kMaxSpeed = 4.0f;
    static constexpr float kTransition = 0.65f;   // seconds per transition at 1x
    static constexpr float kHold = 0.35f;         // pause between steps at 1x
};

// ---- the player -------------------------------------------------------------
class AnimPlayer {
public:
    void reset(const Frame& f);      // drop everything and show f
    void push(const Frame& f);       // queue a new step
    void update(float dt);

    void togglePause() { paused_ = !paused_; stepping_ = false; }
    void play() { paused_ = false; stepping_ = false; }
    bool paused() const { return paused_; }
    void stepForward();
    void stepBack();
    void skipToEnd();
    void seek(int index, float t);   // used by the screenshot/test mode

    const Frame& from() const { return from_; }
    const Frame& to() const { return frames_[cur_]; }
    float progress() const { return t_; }
    int current() const { return cur_; }
    int count() const { return (int)frames_.size(); }
    bool finished() const { return cur_ == (int)frames_.size() - 1 && t_ >= 1.0f; }
    const std::vector<Frame>& frames() const { return frames_; }

private:
    void advance();
    std::vector<Frame> frames_ = std::vector<Frame>(1);
    int cur_ = 0;
    Frame from_;
    float t_ = 1.0f;       // 0..1 progress of the transition from_ -> frames_[cur_]
    float hold_ = 0.0f;
    bool paused_ = false;
    bool stepping_ = false; // finishing one manual step while paused
};
