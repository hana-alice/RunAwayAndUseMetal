#include "KeyboardEvent.h"

namespace raum::framework {

extern bool getKeyPressedNative(Keyboard key);

bool keyPressed(Keyboard key) {
    return getKeyPressedNative(key);
}


}