//
// Created by henry on 11/28/24.
//
#include <imgui.h>
#include <ImGuizmo.h>
#include <hge/core/events.h>
#include <hge/components/gizmo.h>
#include <hge/widgets/enumField.h>
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
    ImGuizmo::BeginFrame();

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImGuizmo::SetDrawlist(drawList);
    ImGuizmo::SetOrthographic(true);
    //ImGuizmo::SetRect(0, 0, GAME_SIZE[0], GAME_SIZE[1])
    ImGuizmo::SetRect(window.pos[0], window.pos[1], window.size[0], window.size[1]);

    if (m_selectedEntity) {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        if (m_mode == GizmoMode::Scale) {
            op = ImGuizmo::SCALE;
        } else if (m_mode == GizmoMode::Rotate) {
            op = ImGuizmo::ROTATE;
        }

        auto transform = m_selectedEntity->model();

        if (ImGuizmo::Manipulate(view.transpose().mat, proj.transpose().mat, op, ImGuizmo::LOCAL, transform.transpose().mat)) {
            std::cout << "MANIPULATED\n";
        }

        if (ImGuizmo::IsUsing()) {
            std::cout << "UPDATED TRANSFORM\n";
        }
    }
}
