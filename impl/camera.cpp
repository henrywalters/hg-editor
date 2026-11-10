//
// Created by henry on 8/29/26.
//
#include <hge/core/camera.h>

using namespace hge;
using namespace hg;

hg::Mat4 OrthographicCamera::projection() const {
    hg::Vec2 visibleSize = size.cast<float>() / pixelsPerMeter * zoom;
    return Mat4::Orthographic(
                -visibleSize[0] * 0.5f, visibleSize[0] * 0.5f,
                -visibleSize[1] * 0.5f, visibleSize[1] * 0.5f,
                zNear, zFar
        );
}

hg::Mat4 OrthographicCamera::view() const {
    return Mat4::Translation(position.resize<3>() * -1);
}

void OrthographicCamera::update(int scroll, bool pan, hg::Vec2 mousePos, double dt) {
    if (scroll != 0.0f) {
        float factor = 1.0f + scroll * zoomSpeed * dt;
        zoom = zoom * factor;
    }

    if (pan) {
        auto delta = mousePos - m_lastMousePos;
        position -= (delta / pixelsPerMeter * zoom);
    }

    m_lastMousePos = mousePos;
}

hg::Mat4 PerspectiveCamera::projection() const {
    return Mat4::Perspective(fov * math::DEG2RAD, aspect, zNear, zFar);
}

hg::Mat4 PerspectiveCamera::view() const {
    return Mat4::LookAt(
        position,
        m_rotation.rotatePoint(forward) + position,
        up
    );
}

void PerspectiveCamera::update(bool rotate, hg::Vec2 mousePos, double dt) {

}
