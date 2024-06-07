#include "Camera.h"

namespace raum::scene {

Camera::Camera(const raum::scene::Frustum &frustum, raum::scene::Projection type): _eye(frustum, type) {
}

}