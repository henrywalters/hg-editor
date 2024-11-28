//
// Created by henry on 10/6/24.
//
#include <hge/editor.h>
#include <hagame/core/game.h>
#include <hagame/core/assets.h>
#include <hagame/math/functions.h>
#include <hagame/core/prefab.h>
#include "hge/components/entityViewer.h"
#include <hge/core/extensions.h>
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
        m_scene->entities.duplicate(static_cast<Entity*>(payload.entity));
//        auto entity = m_scene->entities.add((hg::Entity*) payload.entity->parent());
//        entity->transform = payload.entity->transform;
//        for (const auto& component : payload.entity->components()) {
//            auto newComponent = ComponentFactory::Attach(entity, component->className());
//            for (const ComponentFactory::ComponentField& field : ComponentFactory::GetFields(component->className())) {
//                field.setter(newComponent, field.getter(component));
//            }
//        }
    });

    events->subscribe(EventTypes::SavePrefab, [&](const auto& e) {
        auto payload = std::get<EntityEvent>(e.payload);
        savePrefab(payload.entity);
    });

    events->subscribe(EventTypes::AddPrefab, [&](const auto& e) {
        auto payload = std::get<PrefabEvent>(e.payload);
        auto parts = hg::utils::f_getParts(payload.prefab);
        Prefab* prefab;
        if (hg::hasPrefab(parts.name)) {
            prefab = hg::getPrefab(parts.name);
        } else {
            prefab = hg::loadPrefab(parts.name, parts.absolutePath()).get();
        }
        m_scene->entities.add(prefab);

//        auto config = utils::MultiConfig::Parse(payload.prefab);
//        std::cout << config.toString() << "\n";
//        m_scene->load(config);
    });
}

void Editor::saveAs() {
    m_browser.saveFile("Scenes", [&](auto path){
        if (!hg::utils::s_endsWith(path, HG_SCENE)) {
            path += HG_SCENE;
        }
        std::cout << "SAVED TO DISC\n";
        std::cout << path << "\n";
        m_saveFile = path;
        saveToDisc();
    }, {HG_SCENE});
}

void Editor::saveToDisc() {
    auto config = m_scene->save();
    utils::f_write(m_saveFile, config.toString());
    Events()->emit(EventTypes::SaveScene, Event{m_scene});
}

void Editor::loadFromDisc() {
    m_browser.loadFile("Scenes", [&](auto path) {
        m_saveFile = path;
        auto config = utils::MultiConfig::Parse(m_saveFile);
        m_scene->clear();
        m_scene->load(config);
        m_entities.selected(nullptr);
        Events()->emit(EventTypes::LoadScene, Event{m_scene});
    }, {HG_SCENE});
}

void Editor::savePrefab(hg::Entity *entity) {
    m_browser.saveFile("Prefabs", [&, entity](auto path) {

        if (!hg::utils::s_endsWith(path, HG_PREFAB)) {
            path += HG_PREFAB;
        }

        auto parts = hg::utils::f_getParts(path);

        if (!hg::hasPrefab(parts.name)) {
            if (hg::utils::f_exists(path)) {
                hg::loadPrefab(parts.name, path);
            } else {
                hg::createPrefab(parts.name);
            }
        }

        auto prefab = hg::getPrefab(parts.name);

        prefab->updatePrefab(entity);
        prefab->save(path);

    }, {HG_PREFAB});
}

hg::Vec2 Editor::mousePos() const {
    return m_mousePos;
}
