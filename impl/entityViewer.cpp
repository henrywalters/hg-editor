//
// Created by henry on 10/3/24.
//
#include "hge/components/entityViewer.h"
#include "hge/components/componentExplorer.h"
#include "hge/core/events.h"

#include "imgui.h"

#include "../../thirdparty/imgui/misc/cpp/imgui_stdlib.h"

using namespace hge;
using namespace hg::math;

void hge::transformViewer(Transform &transform) {
    if (!ImGui::BeginTable("Transform", 4, ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX)) {
        return;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Pos");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    std::array<ImVec4, 3> bgColors {
        ImVec4(1.0, 0.0, 0.0, 1.0),
        ImVec4(0.0, 1.0, 0.0, 1.0),
        ImVec4(0.0, 0.0, 1.0, 1.0)
    };

    std::array<ImVec4, 3> fgColors {
        ImVec4(1, 1, 1, 1),
        ImVec4(0, 0, 0, 1),
        ImVec4(1, 1, 1, 1)
    };

    std::array<uint16_t, 5> colors {
        ImGuiCol_FrameBg,
        ImGuiCol_FrameBgHovered,
        ImGuiCol_FrameBgActive,
        ImGuiCol_SliderGrab,
        ImGuiCol_SliderGrabActive,
    };

    auto input = [&](std::function<void()> lambda, int index) {
        ImGui::PushStyleColor(ImGuiCol_Text, fgColors[index]);
        for (int i = 0; i < colors.size(); i++) {
            ImGui::PushStyleColor(colors[i], bgColors[index]);
        }
        lambda();
        ImGui::PopStyleColor(colors.size() + 1);
    };

    for (int i = 0; i < 3; i++) {
        ImGui::PushID(0 + i);
        ImGui::TableSetColumnIndex(i + 1);
        input([&]() {
            ImGui::DragFloat(("##Position_" + std::to_string(i)).c_str(), &transform.position.vector[i], 0.1);
        }, i);
        ImGui::PopID();
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Scale");

    for (int i = 0; i < 3; i++) {
        ImGui::PushID(1 + i);
        ImGui::TableSetColumnIndex(i + 1);
        input([&]() {
            ImGui::DragFloat(("##Scale_" + std::to_string(i)).c_str(), &transform.scale.vector[i], 0.1);
        }, i);
        ImGui::PopID();
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Rot: %s", transform.rotation.toString().c_str());

//    auto eulerAngles = transform.rotation.normalized().eulerAngles();
//
//    for (int i = 0; i < 3; i++) {
//        eulerAngles[i] *= hg::math::RAD2DEG;
//    }
//
//    ImGui::PushID(6);
//    ImGui::TableSetColumnIndex(1);
//    input([&]() {
//        ImGui::DragFloat("##Roll", &eulerAngles[0], 0.1, -180 + hg::math::EPSILON, 180 - hg::math::EPSILON);
//    }, 0);
//    ImGui::PopID();
//
//    ImGui::PushID(7);
//    ImGui::TableSetColumnIndex(2);
//    input([&]() {
//        ImGui::DragFloat("##Pitch", &eulerAngles[1], 0.1,-90 + hg::math::EPSILON, 90 - hg::math::EPSILON);
//    }, 1);
//    ImGui::PopID();
//
//    ImGui::PushID(8);
//    ImGui::TableSetColumnIndex(3);
//    input([&]() {
//        ImGui::DragFloat("##Yaw", &eulerAngles[2], 0.1, -180 + hg::math::EPSILON, 180 - hg::math::EPSILON);
//    }, 2);
//    ImGui::PopID();
//
//    for (int i = 0; i < 3; i++) {
//        eulerAngles[i] *= hg::math::DEG2RAD;
//    }
//
//    transform.rotation = hg::math::Quaternion<float>(eulerAngles);

    ImGui::PopStyleVar();

    ImGui::EndTable();
}

// void hge::entityViewer(hg::Entity* entity);