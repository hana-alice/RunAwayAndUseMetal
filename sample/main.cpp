#include "ui.h"
int main(int argc, char** argv) {
    raum::sample::UI ui(argc, argv);
    ui.mainLoop();
    return 0;
}
