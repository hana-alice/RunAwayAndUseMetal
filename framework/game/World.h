#pragma once
#include <functional>
#include "Director.h"
#include "window.h"
namespace raum::framework {

class World {
public:
    World();
    ~World();

    void attachWindow(platform::WindowPtr window);

    void run();

    Director& director() const { return *_director; }

private:
    platform::WindowPtr _window;
    Director* _director{nullptr};
};

} // namespace raum::framework