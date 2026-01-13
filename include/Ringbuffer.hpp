#pragma once
#include <atomic>
#include <array>
#include <optional>

namespace HFT_ENGINE {
    template<typename T, size_t Size>
    class RingBuffer {
    public:
        RingBuffer() = default;
        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;

        bool push(T&& item) {
            size_t current_tail = tail.load(std::memory_order_relaxed);
            size_t next_tail = (current_tail + 1) % Size;
            if(next_tail == head.load(std::memory_order_acquire)) {
                return false; // buffer full
            }
            data[current_tail] = std::move(item);
            tail.store(next_tail, std::memory_order_release);
            return true;
        }

        std::optional<T> pop() {
            size_t current_head = head.load(std::memory_order_relaxed);
            if(current_head == tail.load(std::memory_order_acquire)) {
                return std::nullopt; // buffer empty
            }
            T item = std::move(data[current_head]);
            head.store((current_head + 1) % Size, std::memory_order_release);
            return item;
        }

        bool isEmpty() const noexcept {
    
    return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
}

    private:
        std::array<T, Size> data;
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
    };
}
