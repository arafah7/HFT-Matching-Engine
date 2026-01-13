#pragma once
#include <vector>
#include "Order.hpp"

namespace HFT_ENGINE {

class OrderIndex {
public:
    enum class SlotState : uint8_t {
        EMPTY,      
        OCCUPIED,   
        DELETED     
    };

    struct Entry {
        uint32_t id = 0;
        Order* ptr = nullptr;
        SlotState state = SlotState::EMPTY;
    };

   
    OrderIndex(size_t capacity = 2048) : table(capacity), mask(capacity - 1) {}

    // O(1) Insertion
    void insert(uint32_t id, Order* ptr) {
        size_t index = id & mask;
       
        while (table[index].state == SlotState::OCCUPIED) {
            if (table[index].id == id) return; // Already exists
            index = (index + 1) & mask;
        }
        table[index] = {id, ptr, SlotState::OCCUPIED};
    }

    
    Order* get(uint32_t id) const {
        size_t index = id & mask;
        size_t startPos = index;
        
        while (table[index].state != SlotState::EMPTY) {
            if (table[index].state == SlotState::OCCUPIED && table[index].id == id) {
                return table[index].ptr;
            }
            index = (index + 1) & mask;
            if (index == startPos) break; 
        }
        return nullptr;
    }

   
    void remove(uint32_t id) {
        size_t index = id & mask;
        size_t startPos = index;

        while (table[index].state != SlotState::EMPTY) {
            if (table[index].state == SlotState::OCCUPIED && table[index].id == id) {
                table[index].state = SlotState::DELETED;
                table[index].ptr = nullptr;
                return;
            }
            index = (index + 1) & mask;
            if (index == startPos) break;
        }
    }

private:
    std::vector<Entry> table;
    size_t mask; 
};

}
