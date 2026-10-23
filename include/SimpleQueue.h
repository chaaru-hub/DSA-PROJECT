// ============================================================================
// SimpleQueue.h - our own Queue (no std::queue needed), used by the
// level-order (breadth-first) traversal.
//
// Circular array: 'head' is the index of the front element; pushing writes
// at (head + count) % capacity; the array doubles when it is full.
// push and pop are O(1) (amortised).
// ============================================================================
#pragma once
#include <cassert>

template <class T>
class Queue {
public:
    Queue() : data_(new T[8]), cap_(8) {}
    ~Queue() { delete[] data_; }
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    bool empty() const { return count_ == 0; }
    int size() const { return count_; }

    void push(const T& v) {
        if (count_ == cap_) grow();
        data_[(head_ + count_) % cap_] = v;
        ++count_;
    }
    T pop() {
        assert(count_ > 0);
        T v = data_[head_];
        head_ = (head_ + 1) % cap_;
        --count_;
        return v;
    }
    const T& front() const { return data_[head_]; }
    const T& at(int i) const { return data_[(head_ + i) % cap_]; }   // i-th from front

private:
    void grow() {
        T* bigger = new T[cap_ * 2];
        for (int i = 0; i < count_; ++i) bigger[i] = data_[(head_ + i) % cap_];
        delete[] data_;
        data_ = bigger;
        head_ = 0;
        cap_ *= 2;
    }
    T* data_;
    int cap_;
    int head_ = 0, count_ = 0;
};
