#pragma once

#include <functional>
#include <cstdint>

// implemented based on PLF library's conceptual overview:
//   https://www.plflib.org/colony.htm
//
// this guy's talk at cppcon also helped a lot for my understanding:
//   https://www.youtube.com/watch?v=wBER1R8YyGY

template <typename T>
class JumpSkipfield {
   private:
    size_t size;
    T* data;
    uint32_t* skipfield;
    uint32_t* free_address_stack;
    uint32_t stack_count;

   public:
    JumpSkipfield(size_t size) : size(size) {
        data = new T[size];
        skipfield = new uint32_t[size];
        free_address_stack = new uint32_t[size];
        stack_count = size;

        for (size_t i = 0; i < size; i++) {
            skipfield[i] = i == 0 ? size : i + 1;
            free_address_stack[i] = size - i - 1;
        }
    }

    ~JumpSkipfield() {
        delete[] data;
        delete[] skipfield;
        delete[] free_address_stack;
    }

    void iterate(std::function<void(const T* element, uint32_t address)> process_element) {
        // where every zero represents an element, every non-zero that
        //   follows a zero represents how many places we gotta
        //   skip to get to the next zero, and every other non-zero
        //   represents the distance to the zero before the start of
        //   that "block" of non-zeros
        //
        // 000100022000032300523450010

        uint32_t addr = skipfield[0];
        T* element_ptr = data + addr;
        uint32_t* end = skipfield + size;

        for (uint32_t* skip_ptr = skipfield + addr; skip_ptr != end;) {
            process_element(element_ptr, addr);

            skip_ptr++;

            element_ptr += 1 + *skip_ptr;
            addr += 1 + *skip_ptr;

            skip_ptr += *skip_ptr;
        }
    }

    void remove_at(uint32_t address) {
        // dont worry about removing already removed addresses
        if (skipfield[address] != 0) return;

        // we'll be merging existing blocks...
        // remove here
        //      \/
        // 0032304234
        // 0082345678

        // get adjacent values (use zero if we're on an edge)
        uint32_t value_left = address == 0 ? 0 : skipfield[address - 1];
        uint32_t value_right = address == static_cast<uint32_t>(size) - 1
                                   ? 0
                                   : skipfield[address + 1];

        // place new block size at start of block
        //   (will make a new block automatically)
        uint32_t block_size = (value_left + value_right + 1);
        skipfield[address - value_left] = block_size;

        // and then iterate across the block size updating distance
        //   to the start of the block from our block's left edge
        //
        // (just won't run for blocks of size 1)
        for (uint32_t i = 2; i <= block_size; i++) {
            skipfield[address - value_left + 1] = i;
        }

        // push newly freed address onto the stack
        free_address_stack[stack_count++] = address;
    }

    bool add(T element, uint32_t* out_address = nullptr) {
        // can't add to a full list
        if (stack_count == 0) return false;

        // we'll be taking blocks and splitting them in two...
        // insert here
        //      \/
        // 0062345600
        // 0032302200

        // pop an address from our stack
        uint32_t address = free_address_stack[--stack_count];

        // get adjacent values (use zero if we're on an edge)
        uint32_t value_left = address == 0 ? 0 : skipfield[address - 1];
        // uint32_t value_right = address == static_cast<uint32_t>(size) - 1
        //                            ? 0
        //                            : skipfield[address + 1];

        uint32_t prev_block_size = skipfield[address - value_left];

        // create left block
        uint32_t left_block_size = value_left;
        skipfield[address - value_left] = left_block_size;

        // create right block
        uint32_t right_block_size = prev_block_size - left_block_size - 1;
        for (uint32_t i = 1; i <= right_block_size; i++) {
            // iterate resizing distances starting at address plus one
            //   (doesn't run for zero-sized right blocks)
            skipfield[address + i] = i == 1 ? right_block_size : i;
        }

        // finally, set the skip value for current address
        //   as taken and save output address
        skipfield[address] = 0;
        data[address] = element;
        if (out_address != nullptr) {
            *out_address = address;
        }

        return true;
    }
};
