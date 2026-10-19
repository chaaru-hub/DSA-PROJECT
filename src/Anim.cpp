// ============================================================================
// Anim.cpp - easing curves and the AnimPlayer (queue of animation steps)
// ============================================================================
#include "Anim.h"
#include <algorithm>
#include <cmath>

float AnimSettings::speed = 1.0f;

float clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }

// Smooth start and smooth stop: slow -> fast -> slow.
float easeInOutCubic(float t) {
    t = clamp01(t);
    return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3.0f) / 2;
}

// Goes slightly past 1 and settles back - makes new nodes "pop" in.
float easeOutBack(float t) {
    t = clamp01(t);
    const float c1 = 1.70158f, c3 = c1 + 1;
    return 1 + c3 * std::pow(t - 1, 3.0f) + c1 * std::pow(t - 1, 2.0f);
}

void AnimPlayer::reset(const Frame& f) {
    frames_.assign(1, f);
    from_ = f;
    cur_ = 0;
    t_ = 1.0f;
    hold_ = 0;
    stepping_ = false;
}

void AnimPlayer::push(const Frame& f) {
    frames_.push_back(f);
    // Keep memory bounded during very long sessions: forget old history.
    const int kMax = 4000;
    if ((int)frames_.size() > kMax && cur_ > 1000) {
        frames_.erase(frames_.begin(), frames_.begin() + 1000);
        cur_ -= 1000;
    }
}

void AnimPlayer::advance() {
    from_ = frames_[cur_];
    ++cur_;
    t_ = 0;
    hold_ = 0;
}

void AnimPlayer::update(float dt) {
    float sp = AnimSettings::speed;
    if (paused_ && !stepping_) return;
    if (t_ < 1.0f) {
        // a transition is running
        t_ += dt * sp / AnimSettings::kTransition;
        if (t_ >= 1.0f) {
            t_ = 1.0f;
            hold_ = 0;
            stepping_ = false;
        }
        return;
    }
    stepping_ = false;
    if (paused_) return;
    // transition finished: wait a little, then go to the next queued step
    if (cur_ < (int)frames_.size() - 1) {
        hold_ += dt * sp;
        // when many steps are queued, shorten the pause so we catch up
        float hold = AnimSettings::kHold;
        if ((int)frames_.size() - 1 - cur_ > 40) hold *= 0.3f;
        if (hold_ >= hold) advance();
    }
}

void AnimPlayer::stepForward() {
    paused_ = true;
    if (t_ < 1.0f) { stepping_ = true; return; }      // finish current transition
    if (cur_ < (int)frames_.size() - 1) {
        advance();
        stepping_ = true;
    }
}

void AnimPlayer::stepBack() {
    paused_ = true;
    if (cur_ == 0) return;
    from_ = frames_[cur_];       // animate backwards from where we are
    --cur_;
    t_ = 0;
    stepping_ = true;
}

void AnimPlayer::skipToEnd() {
    cur_ = (int)frames_.size() - 1;
    from_ = frames_[cur_];
    t_ = 1.0f;
    stepping_ = false;
}

void AnimPlayer::seek(int index, float t) {
    index = std::clamp(index, 0, (int)frames_.size() - 1);
    from_ = frames_[index > 0 ? index - 1 : 0];
    cur_ = index;
    t_ = clamp01(t);
    paused_ = true;
}
