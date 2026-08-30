#pragma once

#include <memory>

namespace raum::sample {

class UI {
public:
    UI(int argc, char **argv);
    ~UI();

    UI(const UI &) = delete;
    UI &operator=(const UI &) = delete;

    void show();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace raum::sample
