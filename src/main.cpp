#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <variant>
#include "../include/Order.hpp"
#include "../include/OrderBook.hpp"
#include "../include/Ringbuffer.hpp"
using namespace HFT_ENGINE;


using MarketCommand = std::variant<Order, uint64_t>; 

std::atomic<bool> marketOpen{true};


void marketProducer(RingBuffer<MarketCommand, 2048>& buffer) {
    std::cout << "[Producer] Starting Stress Test...\n";

  
    for (uint64_t i = 1; i <= 1000; ++i) {
        buffer.push(Order{i, Side::Buy, 100, 10});
    }

    // 2. CANCEL TEST: Cancel all even-numbered orders
    for (uint64_t i = 2; i <= 1000; i += 2) {
        buffer.push(i); // Push the ID to cancel
    }

  
    buffer.push(Order{9999, Side::Sell, 100, 5000});

    while(!buffer.isEmpty()) { std::this_thread::yield(); }
    
    marketOpen = false; 
    std::cout << "[Producer] Stress test signals sent.\n";
}


void matchingConsumer(Orderbook& engine, RingBuffer<MarketCommand, 2048>& buffer) {
    std::cout << "[Consumer] Engine processing started...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    size_t count = 0;

    while (marketOpen || !buffer.isEmpty()) {
        auto msg = buffer.pop();
        if (msg.has_value()) {
            count++;
          
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Order>) {
                    engine.addOrder(std::forward<T>(arg));
                } else if constexpr (std::is_same_v<T, uint64_t>) {
                    engine.cancelOrder(arg); 
                }
            }, msg.value());
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "[Consumer] Engine Finished.\n";
    std::cout << "Processed " << count << " ops in " << diff.count() << " us.\n";
    if (count > 0) std::cout << "Avg Latency: " << (diff.count() * 1000 / count) << " ns/op\n";
}

int main() {
    Orderbook engine;
    RingBuffer<MarketCommand, 2048> pipe;

    // Run threads
    std::thread t1(marketProducer, std::ref(pipe));
    std::thread t2(matchingConsumer, std::ref(engine), std::ref(pipe));

    t1.join();
    t2.join();

    std::cout << "\n--- Final Orderbook State ---\n";
    engine.printStats();

    return 0;
}
