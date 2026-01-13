#pragma once
#include <map>
#include <list>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include "Order.hpp"
#include "Tradereport.hpp"
#include "RingBuffer.hpp"
#include "Orderindex.hpp"

namespace HFT_ENGINE {

    class Orderbook {
    private:
        std::map<double, std::list<Order>> bids; 
        std::map<double, std::list<Order>> asks; 

       
        OrderIndex idLookup{65536}; 
        RingBuffer<Tradereport, 2048> tradeHistory;

    public:
        Orderbook() = default;

        void addOrder(Order order) {
            uint32_t id = order.id;
            double price = order.price;
            auto& sideMap = (order.side == Side::Buy) ? bids : asks;

            sideMap[price].push_back(std::move(order));
            
            
            idLookup.insert(id, &sideMap[price].back());
               std::cout << "[ADD] Order " << order.id << " successfully added.\n";
            checkMatches();
        }

        void cancelOrder(uint64_t orderId) {
           
            Order* orderPtr = idLookup.get(static_cast<uint32_t>(orderId));
            
            if (!orderPtr) {
               
                return;
            }

            double price = orderPtr->price;
            auto& sideMap = (orderPtr->side == Side::Buy) ? bids : asks;

            auto listIt = sideMap.find(price);
            if (listIt != sideMap.end()) {
                listIt->second.remove_if([orderId](const Order& o) { 
                    return o.id == static_cast<uint32_t>(orderId); 
                });
                
                if (listIt->second.empty()) {
                    sideMap.erase(listIt);
                }
            }

            // 2. Remove from index and PRINT update
            idLookup.remove(static_cast<uint32_t>(orderId));
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
                        
                        double trade_price = ask.price; 

                        
                        std::cout << "[MATCH] " << trade_qty << " units @ " << trade_price 
                                  << " (Bid:" << bid.id << " Ask:" << ask.id << ")\n";

                        tradeHistory.push(Tradereport{bid.id, ask.id, trade_qty, trade_price});

                        bid.quantity -= trade_qty;
                        ask.quantity -= trade_qty;

                        if (bid.quantity == 0) {
                            idLookup.remove(bid.id);
                            bidList.pop_front();
                        }
                        if (ask.quantity == 0) {
                            idLookup.remove(ask.id);
                            askList.pop_front();
                        }
                    }

                    if (bidList.empty()) bids.erase(bestBidIt);
                    if (askList.empty()) asks.erase(bestAskIt);
                } else {
                    break; 
                }
            }
        }

        void printStats() const {
            std::cout << "\n--- Final Orderbook State ---" << std::endl;
            std::cout << "==============================" << std::endl;
            std::cout << " Bid Levels: " << bids.size() << std::endl;
            std::cout << " Ask Levels: " << asks.size() << std::endl;
            
            size_t liveOrders = 0;
            

            std::cout << " Live Orders: " << bids.size() + asks.size() << " (levels)" << std::endl;
            std::cout << "==============================" << std::endl;
        }
    };
}
