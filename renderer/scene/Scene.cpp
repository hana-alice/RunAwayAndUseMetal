#include "Scene.h"
#include "Common.h"

namespace raum::scene {

static uint64_t object_id{0};

Renderable::Renderable(): objectID(++object_id) {
}

float distance(const Vec3f& point, const Plane& plane) {
    // Ax + By + Cz + D = 0
    // D = -(Ax0 + By0 + Cz0)
    // plane.point = Point(x0, y0, z0)

    raum_check( realEqual(glm::length(plane.normal), 1.0f) , "");
    float D = -glm::dot(plane.normal, plane.point);
    return plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z + D;
}

bool frustumCulling(const scene::FrustumPlanes& frustum, const AABB& aabb) {
    bool outside = false;
    for (const auto& plane : frustum) {
        Vec3f mostPoint{};
        mostPoint.x = plane.normal.x > 0 ? aabb.maxBound.x : aabb.minBound.x;
        mostPoint.y = plane.normal.y > 0 ? aabb.maxBound.y : aabb.minBound.y;
        mostPoint.z = plane.normal.z > 0 ? aabb.maxBound.z : aabb.minBound.z;

        auto dist = distance(mostPoint, plane);
        if (distance(mostPoint, plane) < 0.0f) {
            outside = true;
            break;
        }
    }
    return !outside;
}


}