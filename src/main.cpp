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

// Define a Variant to handle different command types in the same buffer
// This is cleaner and faster than a Union
using MarketCommand = std::variant<Order, uint64_t>; 

std::atomic<bool> marketOpen{true};

// --- THE PRODUCER: MARKET SIMULATOR ---
void marketProducer(RingBuffer<MarketCommand, 2048>& buffer) {
    std::cout << "[Producer] Starting Stress Test...\n";

    // 1. STRESS TEST: Add 1000 Buy Orders
    for (uint64_t i = 1; i <= 1000; ++i) {
        buffer.push(Order{i, Side::Buy, 100, 10});
    }

    // 2. CANCEL TEST: Cancel all even-numbered orders
    for (uint64_t i = 2; i <= 1000; i += 2) {
        buffer.push(i); // Push the ID to cancel
    }

    // 3. MATCH TEST: Add one massive Sell Order to clear the remaining 500 Bids
    buffer.push(Order{9999, Side::Sell, 100, 5000});

    // Let the consumer catch up
    while(!buffer.isEmpty()) { std::this_thread::yield(); }
    
    marketOpen = false; 
    std::cout << "[Producer] Stress test signals sent.\n";
}

// --- THE CONSUMER: MATCHING ENGINE ---
void matchingConsumer(Orderbook& engine, RingBuffer<MarketCommand, 2048>& buffer) {
    std::cout << "[Consumer] Engine processing started...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    size_t count = 0;

    while (marketOpen || !buffer.isEmpty()) {
        auto msg = buffer.pop();
        if (msg.has_value()) {
            count++;
            // Item 42: std::visit is the modern way to handle variants
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Order>) {
                    engine.addOrder(std::forward<T>(arg));
                } else if constexpr (std::is_same_v<T, uint64_t>) {
                    engine.cancelOrder(arg); // arg is the orderId
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