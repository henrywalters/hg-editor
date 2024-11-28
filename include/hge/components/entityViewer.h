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

namespace hge {

    void transformViewer(hg::math::Transform& transform);
    inline void entityViewer(hg::Entity *entity) {

        ImGui::Text(entity->name.c_str());

        //ImGui::InputText("Name", &entity->name);

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


}

#endif //HGEDITOR_ENTITYVIEWER_H
