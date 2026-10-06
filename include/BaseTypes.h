#pragma once

#include <functional>

struct Flag96 {
    uint32_t part1;
    uint32_t part2;
    uint32_t part3;
};

using guid_t = uint64_t;
using unk_t = uint32_t;
using HashKeyStri = uint32_t;

using DummyCallback = void (*)();
using FunctionCallback = std::function<void()>;

template <typename T>
struct SlotLink {
    T** link_slot;
    T* next;
};

template <typename T>
struct SlotList {
    T* head;
    T** tail;
};
