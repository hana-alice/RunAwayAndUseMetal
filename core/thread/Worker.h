#pragma once
#include <thread>
namespace raum {

class Worker {
public:
    Worker() {
        _thread = std::jthread{};
    }
    Worker(const Worker&) = delete;
    Worker(Worker&&) = delete;
    Worker& operator=(const Worker&) = delete;

    ~Worker() {
        _thread.request_stop();
        _thread.join();

    }



    auto handle() {
        return _thread.get_id();
    }

private:
    std::jthread _thread;
};

} // namespace raum