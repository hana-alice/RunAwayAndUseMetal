#pragma once
#include "World.h"

namespace raum::framework {

World::World() {
    _director = new Director();
}

void World::attachWindow(platform::WindowPtr window) {
    _director->attachWindow(window);
    _window = window;
}

void World::run() {
    _director->run();
}

} // namespace raum::framework