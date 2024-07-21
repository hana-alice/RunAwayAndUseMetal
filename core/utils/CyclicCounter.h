//
// Created by zeqia on 2024/7/18.
//

#pragma once
#include <stdint.h>
#include <type_traits>

template <typename T>
    requires std::is_integral_v<T>
class CyclicCounter {
public:
    CyclicCounter() = delete;
    explicit CyclicCounter(T threshold):_threshold(threshold) {
    }
    ~CyclicCounter() = default;

    // CyclicCounter++
    T operator++() {
        _val = (_val + 1) % _threshold;
        return _val;
    }

    // ++CyclicCounter
    // https://en.cppreference.com/w/cpp/language/operator_incdec
    T operator++(int) {
        T temp = _val;
        _val = (_val + 1) % _threshold;
        return temp;
    }

    T currentValue() const { return _val; }

private:
    T _threshold{0};
    T _val{0};
};
