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
            ImGui::DragFloat(("##Position_" + std::to_string(i)).c_str(), &transform.position.vector[i]);
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
            ImGui::DragFloat(("##Scale_" + std::to_string(i)).c_str(), &transform.scale.vector[i]);
        }, i);
        ImGui::PopID();
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Rot");

    auto eulerAngles = transform.rotation.normalized().eulerAngles();

    for (int i = 0; i < 3; i++) {
        eulerAngles[i] *= hg::math::RAD2DEG;
    }

    ImGui::PushID(6);
    ImGui::TableSetColumnIndex(1);
    input([&]() {
        ImGui::DragFloat("##Roll", &eulerAngles[0], 0.1, -180, 180);
    }, 0);
    ImGui::PopID();

    ImGui::PushID(7);
    ImGui::TableSetColumnIndex(2);
    input([&]() {
        ImGui::DragFloat("##Pitch", &eulerAngles[1], 0.1,-89.9999, 89.9999);
    }, 1);
    ImGui::PopID();

    ImGui::PushID(8);
    ImGui::TableSetColumnIndex(3);
    input([&]() {
        ImGui::DragFloat("##Yaw", &eulerAngles[2], 0.1, -180, 180);
    }, 2);
    ImGui::PopID();

    for (int i = 0; i < 3; i++) {
        eulerAngles[i] *= hg::math::DEG2RAD;
    }

    transform.rotation = hg::math::Quaternion<float>(eulerAngles);

    ImGui::PopStyleVar();

    ImGui::EndTable();
}

void hge::entityViewer(hg::Entity* entity) {
    ImGui::InputText("Name", &entity->name);

    if (entity->name.empty()) {
        entity->name = "Entity<" + std::to_string(entity->id()) + ">";
    }

    ImGui::Text("ID: %lu", entity->id());

    transformViewer(entity->transform);

//    ImGui::DragFloat3("Position", entity->transform.position.vector);
//    ImGui::DragFloat3("Scale", entity->transform.scale.vector);
//


    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup(COMPONENT_EXPLORER.c_str());
    }

    auto addComponent = componentExplorer();
    if (addComponent.has_value()) {
        auto component = hg::ComponentFactory::Attach(entity, addComponent.value());
        Events()->emit(EventTypes::AddComponent, Event{ComponentEvent{entity, component->className(), ""}});
    }

    int index = 0;

    if (entity->components().size() > 0) {
        ImGui::SeparatorText("Components");
        for (const auto& component : entity->components()) {
            ImGui::Separator();
            ImGui::Text("%s", component->operator std::string().c_str());
            ImGui::SameLine();
            ImGui::PushID(index++);
            if (ImGui::Button("remove")) {
                hg::ComponentFactory::Get(component->className()).remove(entity);
                Events()->emit(EventTypes::RemoveComponent, Event{ComponentEvent{entity, component->className(), ""}});
            }
            ImGui::PopID();
            for (const auto& field : hg::ComponentFactory::GetFields(component->className())) {
                ImGui::PushID(index++);

                if (editComponentField(component, field)) {
                    Events()->emit(EventTypes::UpdateComponent, Event{ComponentEvent{entity, component->className(), field.field}});
                }

                if (field.field == "texture") {
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("png")) {
                            auto path = (char*) payload->Data;
                            field.setter(component, std::string(path));
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                ImGui::PopID();
            }

            component->uiUpdate();
        }
    }
}
