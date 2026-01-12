#pragma once 
#include <cstdint>

namespace HFT_ENGINE{
   struct Tradereport {
    uint64_t bidId;   // Change from uint32 to uint64
    uint64_t askId;   // Change from uint32 to uint64
    uint32_t quantity;
    uint32_t price;

    Tradereport() : bidId(0), askId(0), quantity(0), price(0) {}
    Tradereport(uint64_t b, uint64_t a, uint32_t q, uint32_t p) 
        : bidId(b), askId(a), quantity(q), price(p) {}
};
}