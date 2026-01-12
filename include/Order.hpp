#pragma once 
#include <cstdint>

enum class  Side{
    Buy,
    Sell,
};

struct Order{
    uint64_t id;
    Side side;
    uint32_t price;
    uint32_t quantity;
    
    // 1. Fixed: Changed 0.0 to 0 (matches uint32_t)
    Order() : id(0), side(Side::Buy), price(0), quantity(0) {} 

    // 2. Fixed: Changed int32_t to uint32_t to match the member 'price'
    Order(uint64_t i, Side s, uint32_t p, uint32_t q) 
        : id(i), side(s), price(p), quantity(q) {}
};