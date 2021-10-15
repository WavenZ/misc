#include <iostream>

#include "orderedlist.h"

struct Comparator {
    int operator()(const std::uint32_t& a, const std::uint32_t& b) const {
        if (a < b) return -1;
        return static_cast<int>(a != b);
    }
} compare;

int main(int argc, char* argv[]) {
    test::OrderedList<uint32_t, Comparator> list(compare);

    list.Insert(342);
    list.Insert(413);
    list.Insert(4552);
    list.Insert(65);
    list.Insert(512);
    list.Insert(1);
    list.Insert(31435);

    for (const auto& data: list) {
        std::cout << data << std::endl;
    }

    return 0; 
}