//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_ENTITYVIEWER_H
#define HGEDITOR_ENTITYVIEWER_H

#include <hagame/core/entity.h>

#include "hge/components/entityViewer.h"
#include "hge/components/componentExplorer.h"
#include "hge/core/events.h"
#include <imgui.h>
#include <iostream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <hagame/utils/file.h>
#include <misc/cpp/imgui_stdlib.h>

namespace hge {

    // Which component types are open in the inspector, as the user last left them by clicking their headers. Saved
    // here, next to imgui.ini, so it lasts between sessions
    inline const std::string INSPECTOR_FILE = "inspector.json";

    inline std::unordered_map<std::string, bool>& inspectorExpanded() {
        static std::unordered_map<std::string, bool> expanded = [] {
            std::unordered_map<std::string, bool> loaded;
            if (hg::utils::f_exists(INSPECTOR_FILE)) {
                try {
                    loaded = nlohmann::json::parse(hg::utils::f_read(INSPECTOR_FILE)).get<std::unordered_map<std::string, bool>>();
                } catch (std::exception const& e) {
                    std::cout << "WARNING: couldn't read " << INSPECTOR_FILE << ", starting with every component closed: " << e.what() << "\n";
                }
            }
            return loaded;
        }();
        return expanded;
    }

    inline void saveInspectorExpanded() {
        hg::utils::f_write(INSPECTOR_FILE, nlohmann::json(inspectorExpanded()).dump(4));
    }

    void transformViewer(hg::math::Transform& transform);
    inline void entityViewer(hg::Entity *entity) {

        // ImGui::Text(entity->name.c_str());

        // Inactive entities (and everything under them) are skipped by rendering, physics, audio and events
        ImGui::Checkbox("##active", &entity->active);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Active");
        }
        ImGui::SameLine();
        ImGui::InputText("Name", &entity->name);

        if (entity->name.empty()) {
            entity->name = "Entity<" + std::to_string(entity->id()) + ">";
        }

        ImGui::Text("ID: %i", entity->id());

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

            // Each component folds away under its header. Whether it's open is remembered per component type, like
            // Unity's inspector: close components you don't touch once and they stay closed on every entity, between
            // sessions too. A type never opened or closed starts closed, so picking an entity shows what it has rather
            // than every field it has, unless it's the entity's only component
            auto& expanded = inspectorExpanded();
            bool lone = entity->components().size() == 1;

            for (const auto& component : entity->components()) {
                auto type = component->className();
                ImGui::PushID(index++);

                auto remembered = expanded.find(type);
                ImGui::SetNextItemOpen(remembered != expanded.end() ? remembered->second : lone, ImGuiCond_Always);
                bool open = ImGui::CollapsingHeader(type.c_str(), ImGuiTreeNodeFlags_AllowOverlap);
                // Only the user's own clicks are remembered, not a lone component opening by default
                if (ImGui::IsItemToggledOpen()) {
                    expanded[type] = open;
                    saveInspectorExpanded();
                }

                // Remove, on the header's line at its right
                ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize("remove").x - ImGui::GetStyle().FramePadding.x * 2);
                if (ImGui::SmallButton("remove")) {
                    hg::ComponentFactory::Get(type).remove(entity);
                    Events()->emit(EventTypes::RemoveComponent, Event{ComponentEvent{entity, type, ""}});
                    ImGui::PopID();
                    // The entity's component list just changed under this loop, so stop here; it's drawn again next frame
                    return;
                }

                if (open) {
                    ImGui::Indent();

                    if (type == "Tilemap") {
                        if (ImGui::Button("Open Tilemap Editor")) {
                            Events()->emit(EventTypes::OpenTool, Event{ToolEvent{"TilemapEditor", component}});
                        }
                    }

                    for (const auto& field : hg::ComponentFactory::GetFields(type)) {
                        ImGui::PushID(index++);

                        ImGui::SeparatorText(field.field.c_str());
                        try {
                            if (editComponentField(component, field)) {
                                Events()->emit(EventTypes::UpdateComponent, Event{ComponentEvent{entity, type, field.field}});
                            }
                        } catch (...) {
                            std::cout << "Missing field: " << field.field << "\n";
                        }

                        ImGui::PopID();
                    }

                    component->uiUpdate();

                    ImGui::Unindent();
                }

                ImGui::PopID();
            }
        }
    }


}

#endif //HGEDITOR_ENTITYVIEWER_H
