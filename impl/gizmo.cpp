//
// Created by henry on 11/28/24.
//
#include <imgui.h>
#include <ImGuizmo.h>
#include <hge/core/events.h>
#include <hge/components/gizmo.h>
#include <hge/widgets/enumField.h>
#include <cmath>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using namespace hge;

Gizmo::Gizmo() {
    hge::Events()->subscribe(hge::EventTypes::SelectEntity, [&](const auto& ev) {
        auto e = std::get<hge::EntityEvent>(ev.payload);
        m_selectedEntity = e.entity;
    });
}

void Gizmo::setMode(GizmoMode::type mode) {
    m_mode = mode;
}

void Gizmo::renderModeSelector() {
    hge::enumField("Gizmo Mode", &m_mode, ENUMS(GizmoMode));
}

void Gizmo::render(bool orthographic, hg::Recti window, hg::Mat4 proj, hg::Mat4 view) {
    // Must be called inside the window that shows the scene. ImGuizmo only accepts input when the window
    // that owns its draw list is the hovered window, so the foreground draw list never received clicks.
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetOrthographic(orthographic);
    ImGuizmo::SetRect(window.pos[0], window.pos[1], window.size[0], window.size[1]);

    if (!m_selectedEntity) {
        return;
    }

    // In orthographic mode the camera looks down Z, so only offer the axes that make sense in 2D
    ImGuizmo::OPERATION op = orthographic ? (ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y) : ImGuizmo::TRANSLATE;
    if (m_mode == GizmoMode::Scale) {
        op = orthographic ? (ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y) : ImGuizmo::SCALE;
    } else if (m_mode == GizmoMode::Rotate) {
        op = orthographic ? ImGuizmo::ROTATE_Z : ImGuizmo::ROTATE;
    }

    // hg matrices are row-major, ImGuizmo wants column-major
    hg::Mat4 viewT = view.transpose();
    hg::Mat4 projT = proj.transpose();
    hg::Mat4 modelT = m_selectedEntity->model().transpose();

    if (!ImGuizmo::Manipulate(viewT.mat, projT.mat, op, ImGuizmo::LOCAL, modelT.mat)) {
        return;
    }

    hg::Mat4 world = modelT.transpose();

    // model() = T * R * S, so the columns of the upper 3x3 are the rotated axes scaled by the world scale
    hg::Vec3 worldPos(world.get(0, 3), world.get(1, 3), world.get(2, 3));
    hg::Vec3 worldScale;
    float r[3][3];
    for (int col = 0; col < 3; col++) {
        hg::Vec3 axis(world.get(0, col), world.get(1, col), world.get(2, col));
        worldScale[col] = axis.magnitude();
        for (int row = 0; row < 3; row++) {
            r[row][col] = worldScale[col] != 0 ? axis[row] / worldScale[col] : 0;
        }
    }

    // Rotation matrix -> quaternion (w, x, y, z), matching hg::Mat4::Rotation
    hg::Quat worldRot;
    float trace = r[0][0] + r[1][1] + r[2][2];
    if (trace > 0) {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        worldRot = hg::Quat(0.25f / s, (r[2][1] - r[1][2]) * s, (r[0][2] - r[2][0]) * s, (r[1][0] - r[0][1]) * s);
    } else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
        float s = 2.0f * std::sqrt(1.0f + r[0][0] - r[1][1] - r[2][2]);
        worldRot = hg::Quat((r[2][1] - r[1][2]) / s, 0.25f * s, (r[0][1] + r[1][0]) / s, (r[0][2] + r[2][0]) / s);
    } else if (r[1][1] > r[2][2]) {
        float s = 2.0f * std::sqrt(1.0f + r[1][1] - r[0][0] - r[2][2]);
        worldRot = hg::Quat((r[0][2] - r[2][0]) / s, (r[0][1] + r[1][0]) / s, 0.25f * s, (r[1][2] + r[2][1]) / s);
    } else {
        float s = 2.0f * std::sqrt(1.0f + r[2][2] - r[0][0] - r[1][1]);
        worldRot = hg::Quat((r[1][0] - r[0][1]) / s, (r[0][2] + r[2][0]) / s, (r[1][2] + r[2][1]) / s, 0.25f * s);
    }
    worldRot = worldRot.normalized();

    // The gizmo edits the world matrix, but the entity stores its transform relative to its parent
    hg::Vec3 parentPos = hg::Vec3::Zero();
    hg::Vec3 parentScale = hg::Vec3::Identity();
    hg::Quat parentRot(1, 0, 0, 0);

    if (auto parent = m_selectedEntity->parent()) {
        parentPos = parent->position();
        parentScale = parent->scale();
        parentRot = parent->rotation().normalized();
    }

    auto safeDiv = [](hg::Vec3 a, hg::Vec3 b) {
        return hg::Vec3(
            b[0] != 0 ? a[0] / b[0] : a[0],
            b[1] != 0 ? a[1] / b[1] : a[1],
            b[2] != 0 ? a[2] / b[2] : a[2]
        );
    };

    // Only write the part being edited so float error doesn't creep into the others
    auto& t = m_selectedEntity->transform;
    if (m_mode == GizmoMode::Translate) {
        t.position = safeDiv(parentRot.conjugate().rotatePoint(worldPos - parentPos), parentScale);
    } else if (m_mode == GizmoMode::Rotate) {
        t.rotation = (parentRot.conjugate() * worldRot).normalized();
    } else if (m_mode == GizmoMode::Scale) {
        t.scale = safeDiv(worldScale, parentScale);
    }
}
