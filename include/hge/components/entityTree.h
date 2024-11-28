//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_ENTITYTREE_H
#define HGEDITOR_ENTITYTREE_H

#include <imgui.h>
#include "../core/events.h"
#include <hagame/core/scene.h>

namespace hge {

    class EntityTree {
    public:

        void render(hg::Scene *scene, hg::Entity *root) {
            if (ImGui::Button("Add Entity")) {
                Events()->emit(EventTypes::AddEntity, Event{EntityEvent{nullptr, nullptr}});
            }
            for (const auto &entity: root->children()) {
                renderTree(scene, static_cast<hg::Entity *>(entity), true);
            }
        }

        hg::Entity* selected() { return m_selected; }
        void selected(hg::Entity* entity) {
            m_selected = entity;
        }

    private:

        void renderTree(hg::Scene *scene, hg::Entity *entity, bool root = false) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

            if (entity->children().size() > 0) {
                flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            } else {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (m_selected && entity->id() == m_selected->id()) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::PushID(entity->id());
            bool open = ImGui::TreeNodeEx(entity->name.c_str(), flags);

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_selected = entity;
                Events()->emit(EventTypes::SelectEntity, Event{EntityEvent{entity, nullptr}});
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(std::to_string(entity->id()).c_str());
            }

            if (ImGui::BeginPopup(std::to_string(entity->id()).c_str())) {
                if (ImGui::Button("Add Child")) {
                    Events()->emit(EventTypes::AddChild, Event{EntityEvent{entity, nullptr}});
                }

                if (ImGui::Button("Duplicate")) {
                    Events()->emit(EventTypes::DuplicateEntity, Event{EntityEvent{entity, nullptr}});
                }

                if (ImGui::Button("Save Prefab")) {
                    Events()->emit(EventTypes::SavePrefab, Event{EntityEvent{entity, nullptr}});
                }

                if (ImGui::Button("Delete")) {
                    Events()->emit(EventTypes::RemoveEntity, Event{EntityEvent{entity, nullptr}});
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                auto id = entity->id();
                ImGui::SetDragDropPayload("ENTITY", (void *) &id, sizeof(hg::utils::uuid_t));
                ImGui::Text("%s", entity->name.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                    hg::utils::uuid_t entityId = *(const hg::utils::uuid_t *) payload->Data;
                    auto dragging = scene->entities.get(entityId);
                    Events()->emit(EventTypes::AddChildTo, Event{EntityEvent{dragging, entity}});
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();

            if (open) {
                for (const auto& child : entity->children()) {
                    renderTree(scene, (hg::Entity*)child, false);
                }
                ImGui::TreePop();
            }
        }

        bool m_dragging = false;
        hg::Entity *m_selected = nullptr;

        void savePrefab(hg::Entity* entity);

    };
}

#endif //HGEDITOR_ENTITYTREE_H
