#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include "Order.hpp"
#include "Tradereport.hpp"
#include "RingBuffer.hpp"
#include "Orderindex.hpp"

namespace HFT_ENGINE {

    class Orderbook {
    private:
        // 1. THE PRICE LADDER: Sorted prices for matching
        // Key: Price, Value: List of orders (Time Priority)
        std::map<double, std::list<Order>> bids; 
        std::map<double, std::list<Order>> asks; 

        // 2. THE CANCEL MAP: Direct pointer to any order by ID
        struct OrderLocation {
            double price;
            Side side;
            std::list<Order>::iterator it;
        };
        std::unordered_map<uint64_t, OrderLocation> idLookup;

        // 3. THE AUDIT TRAIL: High-speed ring buffer for matches
        RingBuffer<Tradereport, 1024> tradeHistory;

    public:
        Orderbook() = default;

        // Item 41: Pass-by-value + std::move for zero-copy ingress
        void addOrder(Order order) {
            uint32_t id = order.id;
            double price = order.price;
            Side side = order.side;

           
            auto& sideMap = (side == Side::Buy) ? bids : asks;
            sideMap[price].push_back(std::move(order));

        
            auto it = std::prev(sideMap[price].end());
            idLookup[id] = {price, side, it};

            // Step C: Execute matching logic
            checkMatches();
        }

        void cancelOrder(uint32_t orderId) {
            auto it = idLookup.find(orderId);
            if (it == idLookup.end()) {
                std::cout << "[SYSTEM] Cancel failed: Order " << orderId << " not found.\n";
                return;
            }

            OrderLocation loc = it->second;
            auto& sideMap = (loc.side == Side::Buy) ? bids : asks;
            
            // O(1) removal from the linked list
            sideMap[loc.price].erase(loc.it);
            
            // Clean up the price level if no orders are left
            if (sideMap[loc.price].empty()) {
                sideMap.erase(loc.price);
            }

            // Remove from lookup map
            idLookup.erase(it);
            std::cout << "[CANCEL] Order " << orderId << " successfully removed.\n";
        }

        void checkMatches() {
            while (!bids.empty() && !asks.empty()) {
              
                auto bestBidIt = std::prev(bids.end()); 
                auto bestAskIt = asks.begin();

                if (bestBidIt->first >= bestAskIt->first) {
                    auto& bidList = bestBidIt->second;
                    auto& askList = bestAskIt->second;

                    while (!bidList.empty() && !askList.empty()) {
                        Order& bid = bidList.front();
                        Order& ask = askList.front();

                        uint32_t trade_qty = std::min(static_cast<uint32_t>(bid.quantity), 
                              static_cast<uint32_t>(ask.quantity));
                        double trade_price = ask.price; // Passive

                        // Record the trade in the Ring Buffer
                        tradeHistory.push(Tradereport{bid.id, ask.id, trade_qty, trade_price});

                        std::cout << "[MATCH] " << trade_qty << " units @ " << trade_price 
                                  << " (Bid:" << bid.id << " Ask:" << ask.id << ")\n";

                        bid.quantity -= trade_qty;
                        ask.quantity -= trade_qty;

                        // Cleanup fully filled orders
                        if (bid.quantity == 0) {
                            idLookup.erase(bid.id);
                            bidList.pop_front();
                        }
                        if (ask.quantity == 0) {
                            idLookup.erase(ask.id);
                            askList.pop_front();
                        }
                    }

                    // Remove empty price levels
                    if (bidList.empty()) bids.erase(bestBidIt);
                    if (askList.empty()) asks.erase(bestAskIt);
                } else {
                    break; // No overlap in prices
                }
            }
        }

        // Diagnostic helpers
        void printStats() const {
            std::cout << "--- Book Stats ---\n";
            std::cout << "Bid Levels: " << bids.size() << "\n";
            std::cout << "Ask Levels: " << asks.size() << "\n";
            std::cout << "Live Orders: " << idLookup.size() << "\n";
            std::cout << "------------------\n";
        }
    };
}