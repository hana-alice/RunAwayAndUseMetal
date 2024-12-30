#include "Camera.h"

namespace raum::scene {

Camera::Camera(const raum::scene::PerspectiveFrustum &frustum): _eye(frustum) {
}

Camera::Camera(const OrthoFrustum& frustum):_eye(frustum) {

}


}