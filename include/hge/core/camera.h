//
// Created by henry on 8/29/26.
//

#ifndef HGEDITOR_CAMERA_H
#define HGEDITOR_CAMERA_H

#include <hagame/graphics/camera.h>
#include <hagame/input/input.h>

namespace hge {

    enum class CameraMode {
        Perspective,
        Orthographic,
    };

    class EditorCamera {
    public:
        virtual hg::Mat4 view() const {
            return hg::Mat4::Identity();
        };

        virtual hg::Mat4 projection() const {
            return hg::Mat4::Identity();
        }
    };

    class OrthographicCamera : public EditorCamera {
    public:
        hg::Vec2 size;
        hg::Vec2 position;

        float zoomSpeed = 1.0f;
        float zNear = -1000;
        float zFar = 1000.0;
        float pixelsPerMeter = 1;
        float zoom = 1.0;

        hg::Mat4 projection() const override;
        hg::Mat4 view() const override;
        void update(int scroll, bool pan, hg::Vec2 mousePos, double dt);
    private:
        hg::Vec2 m_lastMousePos;
    };

    class PerspectiveCamera : public EditorCamera {
    public:
        hg::Vec3 position;
        hg::Vec3 up = hg::Vec3::Top();
        hg::Vec3 forward = hg::Vec3::Face();
        float fov = 75;
        float aspect = 1.0f;
        float zNear = 0.001f;
        float zFar = 1000.0f;

        hg::Mat4 view() const override;
        hg::Mat4 projection() const override;
        void update(bool rotate, hg::Vec2 mousePos, double dt);
    private:
        hg::Quat m_rotation = hg::Quat(0, hg::Vec3(0, 0, 1));
    };
}

#endif //HGEDITOR_CAMERA_H
