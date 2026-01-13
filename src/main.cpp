#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <variant>
#include <random>
#include "../include/Order.hpp"
#include "../include/Orderbook.hpp"
#include "../include/Ringbuffer.hpp"

using namespace HFT_ENGINE;


using MarketCommand = std::variant<Order, uint64_t>; 

std::atomic<bool> marketOpen{true};


void marketProducer(RingBuffer<MarketCommand, 2048>& buffer) {
    std::cout << "[Producer] Starting Randomized Stress Test...\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> actionDist(0, 10); 
    
   
    uint32_t basePrice = 100;

    for (uint64_t i = 1; i <= 1000; ++i) {
      
        if (actionDist(gen) < 8) {
            Side side = (i % 3 == 0) ? Side::Sell : Side::Buy;
           
            buffer.push(Order{static_cast<uint32_t>(i), side, 10, basePrice});
        } 
        
        else if (i > 1) {
            uint64_t cancelTarget = (gen() % i) + 1;
            buffer.push(cancelTarget);
        }


        if (i % 5 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    }

   
    buffer.push(Order{9999, Side::Sell, 5000, basePrice});

    while(!buffer.isEmpty()) { 
        std::this_thread::yield(); 
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
        } else {
            // Buffer is empty, wait for producer
            std::this_thread::yield();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "\n[Consumer] Engine Finished.\n";
    std::cout << "Processed " << count << " ops in " << diff.count() << " us.\n";
    if (count > 0) {
        std::cout << "Avg Latency (including I/O): " << (diff.count() * 1000 / count) << " ns/op\n";
    }
}

int main() {
    Orderbook engine;
    RingBuffer<MarketCommand, 2048> pipe;

    // Launch threads
    std::thread t1(marketProducer, std::ref(pipe));
    std::thread t2(matchingConsumer, std::ref(engine), std::ref(pipe));

    t1.join();
    t2.join();

    engine.printStats();

    return 0;
}
