//
// Created by henry on 10/6/24.
//
#include <hge/editor.h>
#include <hagame/core/game.h>
#include <hagame/core/assets.h>
#include <hagame/math/functions.h>
#include "hge/components/entityViewer.h"
#include "imgui.h"

using namespace hg;
using namespace hge;

Editor::Editor(hg::Scene *scene, bool _connectEvents):
    m_scene(scene)
{
    if (_connectEvents) {
        connectEvents();
    }
}

void Editor::render() {

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    const int PADDING = 15;

    // Set the window position and size to cover the entire screen
    ImGui::SetNextWindowPos(ImVec2(0, PADDING));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - PADDING));

    ImGui::Begin("DockSpace", nullptr, windowFlags);

    // Create a dockspace inside this window
    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
                onNewScene();
            }

            if (ImGui::MenuItem("Save As")) {
                saveAs();
            }

            if (!m_saveFile.empty() && ImGui::MenuItem("Save")) {
                saveToDisc();
            }

            if (ImGui::MenuItem("Load")) {
                loadFromDisc();
            }
            ImGui::EndMenu();
        }

//        if (ImGui::BeginMenu("Tools")) {
//            for (const auto& tool : m_tools) {
//                if (ImGui::MenuItem(tool->getName().c_str())) {
//                    tool->open();
//                }
//            }
//            ImGui::EndMenu();
//        }

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Entity Tree");
    m_entities.render(m_scene, m_scene->entities.root.get());
    ImGui::End();

    ImGui::Begin("Assets");
    m_assets.render();
    ImGui::End();

    if (m_entities.selected()) {
        ImGui::Begin("Selected Entity");
        entityViewer(m_entities.selected());
        ImGui::End();
    }

    ImGui::Begin("Output");

    if (ImGui::ImageButton("play", (void*) hg::getTexture("ui/play")->id, ImVec2(15, 15))) {
        onPlay();
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("pause", (void*) hg::getTexture("ui/pause")->id, ImVec2(15, 15))) {
        onPause();
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("reset", (void*) hg::getTexture("ui/reset")->id, ImVec2(15, 15))) {
        onReset();
    }

    hg::Vec2 size;
    ImTextureID texture;

    if (m_hasTexture) {
        size = m_textureSize.cast<float>();
        texture = m_textureId;
    } else {
        auto tex = hg::getTexture("missing");
        texture = (ImTextureID)(size_t)tex->id;
        size = tex->image->size.cast<float>();
    }

    auto windowSize = ImGui::GetWindowSize();
    auto finalSize = hg::mapSizeToBounds(size.cast<float>(), hg::Vec2(windowSize[0], windowSize[1] - 65));

    ImGui::Image(texture, ImVec2(finalSize[0], finalSize[1]), ImVec2(0, 1), ImVec2(1, 0));

    ImGui::End();

    ImGui::End();

    m_browser.render();
}

void Editor::setOutput(void* textureId, hg::Vec2i size) {
    m_textureId = textureId;
    m_textureSize = size;
    m_hasTexture = true;
}

void Editor::connectEvents() {
    auto events = Events();

    auto hasChild = [](hg::Entity* entity, hg::Entity* child) -> bool {

        if (!entity || !child) {
            return false;
        }

        std::deque<hg::Entity*> children;
        children.push_back(entity);
        while (children.size() > 0) {
            auto front = children.front();
            children.pop_front();

            if (front == child) {
                return true;
            }

            for (auto other : front->children()) {
                children.push_back(static_cast<hg::Entity*>(other));
            }
        }

        return false;
    };

    events->subscribe(EventTypes::AddEntity, [&](auto e) {
        m_scene->entities.add();
    });

    events->subscribe(EventTypes::RemoveEntity, [&](auto event) {
        auto e = std::get<EntityEvent>(event.payload);
        if (e.entity == m_entities.selected() || hasChild(e.entity, m_entities.selected())) {
            m_entities.selected(nullptr);
        }
        m_scene->entities.remove(e.entity);
    });

    Events()->subscribe(EventTypes::AddChild, [&](const auto& e) {
        m_scene->entities.add(std::get<EntityEvent>(e.payload).entity);
    });

    events->subscribe(EventTypes::AddChildTo, [&](const auto& e) {
        auto payload = std::get<EntityEvent>(e.payload);
        if (std::find(payload.entity->children().begin(), payload.entity->children().end(), payload.target) !=
            payload.entity->children().end()) {
            return; // Cant add a parent to its own child
        }
        payload.target->addChild(payload.entity);
    });

    events->subscribe(EventTypes::DuplicateEntity, [&](const auto& e) {
        auto payload = std::get<EntityEvent>(e.payload);
        auto entity = m_scene->entities.add((hg::Entity*) payload.entity->parent());
        entity->transform = payload.entity->transform;
        for (const auto& component : payload.entity->components()) {
            auto newComponent = ComponentFactory::Attach(entity, component->className());
            for (const ComponentFactory::ComponentField& field : ComponentFactory::GetFields(component->className())) {
                field.setter(newComponent, field.getter(component));
            }
        }
    });
}

void Editor::saveAs() {
    m_browser.saveFile([&](auto path){
        if (!hg::utils::s_endsWith(path, ".hgs")) {
            path += ".hgs";
        }
        m_saveFile = path;
        saveToDisc();
    }, {".hgs"});
}

void Editor::saveToDisc() {
    auto config = m_scene->save();
    utils::f_write(m_saveFile, config.toString());
    Events()->emit(EventTypes::SaveScene, Event{m_scene});
}

void Editor::loadFromDisc() {
    m_browser.loadFile([&](auto path) {
        m_saveFile = path;
        auto config = utils::MultiConfig::Parse(m_saveFile);
        m_scene->clear();
        m_scene->load(config);
        Events()->emit(EventTypes::LoadScene, Event{m_scene});
    }, {".hgs"});
}