//
// Created by Angel Dust on 24/05/2025
//
#pragma once

#include <stm32f4xx.h>
#include <stdint.h>
#include <stddef.h>
#include <algorithm>

template <typename T, size_t Capacity> class RingBuffer {
  public:
    RingBuffer() : head(0), tail(0), full(false) {
    }

    T *push(const T &item) {
        if (full) {
            return nullptr;
        }
        buffer[head] = item;
        T *allocated_item = &buffer[head];
        head = (head + 1) % Capacity;
        full = (head == tail);
        return allocated_item;
    }

    bool pop(T &item) {
        if (empty()) {
            return false;
        }
        item = buffer[tail];
        tail = (tail + 1) % Capacity;
        full = false;
        return true;
    }

    T &operator[](size_t index) {
        // Optional bounds check (or assert in embedded contexts)
        return buffer[(tail + index) % Capacity];
    }

    const T &operator[](size_t index) const {
        return buffer[(tail + index) % Capacity];
    }

    bool empty() const {
        return (!full && (head == tail));
    }

    bool isFull() const {
        return full;
    }

    void clear() {
        head = tail = 0;
        full = false;
    }

    void erase_last(size_t n) {
        size_t current_size = size();
        if (n >= current_size) {
            // If erasing all or more, just clear the buffer.
            clear();
            return;
        }

        // Move head backwards n times, wrapping properly
        head = (head + Capacity - n) % Capacity;
        full = false; // Erasing items means buffer can’t be full
    }

    size_t size() const {
        if (full) {
            return Capacity;
        }
        if (head >= tail) {
            return head - tail;
        }
        return Capacity + head - tail;
    }

    template <typename Predicate> T *find(Predicate pred) {
        size_t count = size();
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (tail + i) % Capacity;
            if (pred(buffer[idx])) {
                return &buffer[idx];
            }
        }
        return nullptr;
    }

    template <typename Compare> void sort(Compare comp) {
        T temp[Capacity];
        size_t count;

        // Copy the buffer to maximize thread safety
        uint32_t basepri = __get_BASEPRI();
        __set_BASEPRI(0x10);

        count = size();
        for (size_t i = 0; i < count; ++i) {
            temp[i] = buffer[(tail + i) % Capacity];
        }

        __set_BASEPRI(basepri);

        // Sorts securely
        std::sort(temp, temp + count, comp);

        // Copy back

        __set_BASEPRI(0x10);

        for (size_t i = 0; i < count; ++i) {
            buffer[i] = temp[i];
        }

        head = count % Capacity;
        tail = 0;
        full = (count == Capacity);

        __set_BASEPRI(basepri);
    }

  private:
    T buffer[Capacity];
    size_t head;
    size_t tail;
    bool full;
};
