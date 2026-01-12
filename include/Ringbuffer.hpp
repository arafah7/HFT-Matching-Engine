#pragma once
#include<atomic>
#include<array>
#include<optional>

namespace HFT_ENGINE{
    template<typename T,size_t Size>
    class RingBuffer {
        public:
        RingBuffer() = default;

        // We MUST delete copy constructor because atomics cannot be copied
        RingBuffer(const RingBuffer&) = delete;
        RingBuffer& operator=(const RingBuffer&) = delete;
        // pushing and placing orders in the buffer 
        bool push(T&& item){
            size_t current_tail = tail.load(std::memory_order_relaxed);
            size_t next_tail = (current_tail + 1) % Size;
            if(next_tail == head.load(std::memory_order_acquire)){
                return false;
            }
            data[current_tail] = std::move(item);
            tail.store(next_tail,std::memory_order_release);
            return true;
        }

        // removing the orders from the buffer 
        std :: optional<T> pop(){
            size_t current_head = head.load(std :: memory_order_relaxed);
            if(current_head == tail.load(std :: memory_order_acquire)){
                return std :: nullopt;
            }
            T item = std :: move(data[current_head]);
            head.store((current_head+1) % Size, std :: memory_order_release);
            return item;
        }
        
        bool isEmpty() const {
    // If head and tail are at the same index, the buffer is empty.
    // We use acquire to ensure we see the latest updates from the other thread.
    return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
}
        // the 
        private:
        std :: array<T,Size> data;
        std :: atomic<size_t> head{0};
        std :: atomic<size_t> tail{0};

    };
}