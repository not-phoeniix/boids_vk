#pragma once

#include <functional>
#include <cstdint>
#include <iostream>

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

    uint32_t get_block_start_addr(uint32_t address) {
        if (skipfield[address] == 0) return address;

        uint32_t value_left = address == 0 ? 0 : skipfield[address - 1];
        return value_left == 0 ? address : address - skipfield[address] + 1;
    }

    uint32_t get_block_size(uint32_t address) {
        return skipfield[get_block_start_addr(address)];
    }

    void RecalculateBlock(uint32_t address_start, uint32_t size) {
        for (uint32_t i = 1; i <= size; i++) {
            skipfield[address_start + i - 1] = i == 1 ? size : i;
        }
    }

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

    void print_skipfield() {
        for (size_t i = 0; i < size; i++) {
            std::cout << skipfield[i] << " ";
        }
        std::cout << "\n";
    }

    void print_data() {
        for (size_t i = 0; i < size; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "\n";
    }

    void remove_at(uint32_t address) {
        // don't remove invalid or already-removed addresses
        if (address >= size || skipfield[address] != 0) return;

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

        uint32_t block_start_addr = address;
        if (value_left != 0) {
            block_start_addr = address == 0 ? 0 : get_block_start_addr(address - 1);
        }

        uint32_t block_size = skipfield[block_start_addr] + value_right + 1;

        RecalculateBlock(block_start_addr, block_size);

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
        uint32_t value_right = address == static_cast<uint32_t>(size) - 1
                                   ? 0
                                   : skipfield[address + 1];

        uint32_t block_start = get_block_start_addr(address);
        uint32_t block_size = skipfield[block_start];
        uint32_t block_end = block_start + block_size - 1;

        // create left block
        if (value_left != 0) {
            uint32_t left_block_size = address - block_start - 1;
            RecalculateBlock(block_start, left_block_size);
        }

        // create right block
        if (value_right != 0) {
            uint32_t right_block_size = block_end - address;
            RecalculateBlock(address + 1, right_block_size);
        }

        // finally, set the skip value for current address
        //   as occupied (0) and save output address
        skipfield[address] = 0;
        data[address] = element;
        if (out_address != nullptr) {
            *out_address = address;
        }

        return true;
    }
};
