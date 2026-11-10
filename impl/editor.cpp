//
// Created by henry on 10/6/24.
//
#include <hge/editor.h>
#include <hagame/core/game.h>
#include <hagame/core/assets.h>
#include <hagame/math/functions.h>
#include <hagame/core/prefab.h>
#include <hagame/utils/string.h>
#include "hge/components/entityViewer.h"
#include <hge/core/extensions.h>
#include "imgui.h"
#include <nlohmann/json.hpp>
#include <array>
#include <hagame/graphics/components/quad.h>
#include <hagame/graphics/components/sprite.h>
#include <hagame/graphics/components/spriteSheetAnimator.h>
#include <hagame/math/components/rectCollider.h>

using namespace hg;
using namespace hge;

Editor::Editor(hg::Scene *scene, bool _connectEvents):
    m_scene(scene)
{
    if (_connectEvents) {
        connectEvents();
    }

    // Picking an entity from the tree jumps the camera to it; picking one in the viewport doesn't, as it's on screen
    m_entities.onTreeClick = [this](hg::Entity* entity) {
        focus(entity);
    };
}

void Editor::render(hg::input::Input *input, hg::Recti window, hg::Mat4 proj, hg::Mat4 view) {
    ImGuizmo::BeginFrame();

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
                m_entities.selected(nullptr);
                m_commandQueue.push_back(onNewScene);
                //onNewScene();
            }

            if (ImGui::MenuItem("Save As")) {
                m_commandQueue.push_back([&](){saveAs();});
                //saveAs();
            }

            if (!m_saveFile.empty() && ImGui::MenuItem("Save")) {
                m_commandQueue.push_back([&](){saveToDisc();});
            }

            if (ImGui::MenuItem("Load")) {
                m_commandQueue.push_back([&](){loadFromDisc();});
            }

            if (ImGui::MenuItem("Go to Main Menu")) {
                m_commandQueue.push_back([&]() {
                    m_scene->game()->scenes()->activate("main_menu");
                });
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            for (const auto& tool : m_tools) {
                if (ImGui::MenuItem(tool->getName().c_str())) {
                    Events()->emit(EventTypes::OpenTool, Event{ToolEvent{tool->getName()}});
                }
            }
            ImGui::EndMenu();
        }

        onMenuBar();

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Entity Tree");
    if (m_scene->entities.root) {
        auto ptr = m_scene->entities.root.get();
        m_entities.render(m_scene, ptr);
    }
    ImGui::End();

    ImGui::Begin("Assets");
    m_assets.render();
    ImGui::End();

    if (m_entities.selected()) {
        ImGui::Begin("Selected Entity");
        entityViewer(m_entities.selected());
        ImGui::End();
    }

    // Keep ImGui from dragging the Output window while the gizmo is being used
    ImGui::Begin("Output", nullptr, ImGuizmo::IsOver() || ImGuizmo::IsUsing() ? ImGuiWindowFlags_NoMove : ImGuiWindowFlags_None);

    if (ImGui::ImageButton("play", (ImTextureID)hg::getTexture("textures/ui/play")->id, ImVec2(15, 15))) {
        Events()->emit(EventTypes::StartGame, Event{RuntimeEvent{}});
        m_commandQueue.push_back(onPlay);
        //onPlay();
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("pause", (ImTextureID)hg::getTexture("textures/ui/pause")->id, ImVec2(15, 15))) {
        Events()->emit(EventTypes::PauseGame, Event{RuntimeEvent{}});
        m_commandQueue.push_back(onPause);
        //onPause();
    }

    ImGui::SameLine();

    if (ImGui::ImageButton("reset", (ImTextureID)hg::getTexture("textures/ui/reset")->id, ImVec2(15, 15))) {
        Events()->emit(EventTypes::RestartGame, Event{RuntimeEvent{}});
        m_entities.selected(nullptr);
        m_commandQueue.push_back(onReset);
        //onReset();
    }

    ImGui::SameLine();

    m_gizmo.renderModeSelector();

    hg::Vec2 size;
    ImTextureID texture;

    if (m_hasTexture) {
        size = m_textureSize.cast<float>();
        texture = reinterpret_cast<ImTextureID>(static_cast<void*>(m_textureId));
    } else {
        auto tex = hg::getTexture("textures/missing");
        texture = (ImTextureID)(size_t)tex->id;
        size = tex->image->size.cast<float>();
    }

    auto windowSize = ImGui::GetWindowSize();
    auto finalSize = hg::mapSizeToBounds(size.cast<float>(), hg::Vec2(windowSize[0], windowSize[1] - 65));

    ImGui::Image(texture, ImVec2(finalSize[0], finalSize[1]), ImVec2(0, 1), ImVec2(1, 0));

    auto minPoint = ImGui::GetItemRectMin();
    auto scale = size[0] / finalSize[0];

    auto rawMousePos = input->devices.keyboardMouse()->mousePosition();

    m_mousePos = hg::Vec2((rawMousePos[0] - minPoint.x) * scale, (rawMousePos[1] - minPoint.y) * scale);
    m_mousePos[1] = size[1] - m_mousePos[1];

    // Image pixels -> normalized device coords -> world, through the camera the scene was drawn with
    hg::Vec2 ndc = m_mousePos.div(size) * 2.0f - hg::Vec2(1.0f);
    hg::Mat4 inverse = (proj * view).inverted();
    hg::Vec4 world = inverse * hg::Vec4({ndc[0], ndc[1], 0, 1});
    hg::Vec2 worldMousePos = hg::Vec2(world[0], world[1]) / world[3];
    bool viewportHovered = ImGui::IsItemHovered();

    // Drop a .png from the asset browser onto the scene to make a Sprite entity where it lands. Only while editing,
    // as anything added during play is lost on reset
    if (enablePicking && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("png")) {
            std::string texturePath(static_cast<const char*>(payload->Data));
            m_commandQueue.push_back([this, texturePath, worldMousePos]() {
                m_entities.selected(addSprite(texturePath, worldMousePos));
            });
        }
        ImGui::EndDragDropTarget();
    }

    bool toolCapturesClicks = false;
    for (auto&& tool : m_tools) {
        if (tool->is_open() && tool->capturesViewportClicks()) {
            toolCapturesClicks = true;
        }
    }

    // Picking: a plain click on the scene image, not on another window and not on / dragging the gizmo
    if (enablePicking && !toolCapturesClicks && viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
        m_entities.selected(pickEntity(worldMousePos));
    }

    m_gizmo.render(cameraMode == CameraMode::Orthographic, hg::Recti(hg::Vec2i(minPoint.x, minPoint.y), finalSize.cast<int>()), proj, view);

    ImGui::End();

    ImGui::End();

    m_browser.render();

    while (m_commandQueue.size() > 0) {
        auto front = m_commandQueue.front();
        m_commandQueue.pop_front();
        front();
    }

    for (auto&& tool : m_tools) {
        tool->setMouse(worldMousePos, viewportHovered);
        if (window.contains(m_mousePos.cast<int>())) {
            tool->update(input, m_scene->game()->dt());
        }
        tool->render(m_scene->game()->dt());
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

    // New entities and prefab instances go in the middle of the view, rather than at the world origin, which may be
    // nowhere near what's being worked on. Their z, the draw layer, is left as it is. Children start on their parent
    events->subscribe(EventTypes::AddEntity, [&](auto e) {
        placeInView(m_scene->entities.add());
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
        placeInView(m_scene->entities.add(prefab));

//        auto config = utils::MultiConfig::Parse(payload.prefab);
//        std::cout << config.toString() << "\n";
//        m_scene->load(config);
    });
}

void Editor::placeInView(hg::Entity* entity) {
    entity->transform.position[0] = orthographicCamera.position[0];
    entity->transform.position[1] = orthographicCamera.position[1];
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
    utils::f_write(m_saveFile,  m_scene->save().dump(4));
    Events()->emit(EventTypes::SaveScene, Event{m_scene});
}

void Editor::loadFromDisc() {
    m_browser.loadFile("Scenes", [&](auto path) {
        m_saveFile = path;
        auto config = nlohmann::json::parse(utils::f_read(path));
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

        // Update in place: reloading the file would replace the Prefab and leave instances pointing at freed memory
        bool wasInstance = entity->prefab == prefab;
        prefab->updatePrefab(entity);
        prefab->save(path);

        if (!wasInstance) {
            // The entity already matches the prefab, so just subscribe it to future changes without rebuilding it
            entity->scene->entities.linkPrefab(entity, prefab, false);
        }

    }, {HG_PREFAB});
}

hg::Vec2 Editor::mousePos() const {
    return m_mousePos;
}

namespace {
    // The corners of a quad-like component in world space. Transforming corners forward (rather than inverting the
    // model matrix) handles rotation, scale and mirroring without any risk of a singular matrix.
    std::array<hg::Vec2, 4> worldCorners(hg::Entity* entity, hg::Vec2 size, hg::Vec2 offset, bool centered) {
        hg::Vec2 center = offset + (centered ? hg::Vec2::Zero() : size * 0.5f);
        hg::Vec2 half = size * 0.5f;
        hg::Mat4 model = entity->model();
        std::array<hg::Vec2, 4> corners;
        const hg::Vec2 signs[4] = {hg::Vec2(-1, -1), hg::Vec2(1, -1), hg::Vec2(1, 1), hg::Vec2(-1, 1)};
        for (int i = 0; i < 4; i++) {
            hg::Vec2 local = center + half.prod(signs[i]);
            hg::Vec4 world = model * hg::Vec4({local[0], local[1], 0, 1});
            corners[i] = hg::Vec2(world[0], world[1]);
        }
        return corners;
    }

    // Point inside a convex quad, whichever way round its corners wind
    bool contains(const std::array<hg::Vec2, 4>& quad, hg::Vec2 point) {
        bool anyPositive = false, anyNegative = false;
        for (int i = 0; i < 4; i++) {
            hg::Vec2 a = quad[i], b = quad[(i + 1) % 4];
            float cross = (b[0] - a[0]) * (point[1] - a[1]) - (b[1] - a[1]) * (point[0] - a[0]);
            anyPositive |= cross > 0;
            anyNegative |= cross < 0;
        }
        return !(anyPositive && anyNegative);
    }

    float area(const std::array<hg::Vec2, 4>& quad) {
        float sum = 0;
        for (int i = 0; i < 4; i++) {
            sum += quad[i][0] * quad[(i + 1) % 4][1] - quad[(i + 1) % 4][0] * quad[i][1];
        }
        return std::abs(sum) * 0.5f;
    }
}

hg::Entity* Editor::pickEntity(hg::Vec2 worldPos) {
    using namespace hg::graphics;

    hg::Entity* best = nullptr;
    float bestZ = 0;
    float bestArea = 0;

    auto consider = [&](hg::Entity* entity, const std::array<hg::Vec2, 4>& quad) {
        if (!contains(quad, worldPos)) {
            return;
        }
        // Front-most first (higher z is drawn in front), then the smallest, so small things on top of big
        // backgrounds stay clickable
        float z = entity->renderPosition()[2];
        float a = area(quad);
        if (!best || z > bestZ || (z == bestZ && a < bestArea)) {
            best = entity;
            bestZ = z;
            bestArea = a;
        }
    };

    m_scene->entities.forEach([&](hg::Entity* entity) {
        if (entity->name == "root") {
            return;
        }

        bool drawn = false;
        if (auto quad = entity->getComponent<Quad>()) {
            consider(entity, worldCorners(entity, quad->size, quad->offset, quad->centered));
            drawn = true;
        }
        if (auto sprite = entity->getComponent<Sprite>()) {
            consider(entity, worldCorners(entity, sprite->size, sprite->offset, sprite->centered));
            drawn = true;
        }
        if (auto animator = entity->getComponent<components::SpriteSheetAnimator>()) {
            consider(entity, worldCorners(entity, animator->size, animator->drawOffset(), animator->centered));
            drawn = true;
        }
        // Invisible things (triggers, walls) can still be picked by their collider
        if (!drawn) {
            if (auto collider = entity->getComponent<hg::math::components::RectCollider>()) {
                auto rect = collider->getRect();
                consider(entity, {rect.pos, rect.pos + hg::Vec2(rect.size[0], 0), rect.pos + rect.size, rect.pos + hg::Vec2(0, rect.size[1])});
            }
        }
    });

    if (!best) {
        return nullptr;
    }

    // Clicking part of a prefab instance selects the instance, which is what you usually want to move
    for (hg::GameObject* node = best; node; node = node->parent()) {
        auto nodeEntity = static_cast<hg::Entity*>(node);
        if (nodeEntity->prefab) {
            return nodeEntity;
        }
    }

    return best;
}

void Editor::focus(hg::Entity* entity) {
    using namespace hg::graphics;

    if (!entity) {
        return;
    }

    // The middle of everything it draws, so a sprite drawn from its corner still ends up centred
    bool hasBounds = false;
    hg::Vec2 min, max;
    auto include = [&](const std::array<hg::Vec2, 4>& quad) {
        for (const auto& corner : quad) {
            if (!hasBounds) {
                min = max = corner;
                hasBounds = true;
            } else {
                min = hg::Vec2(std::min(min[0], corner[0]), std::min(min[1], corner[1]));
                max = hg::Vec2(std::max(max[0], corner[0]), std::max(max[1], corner[1]));
            }
        }
    };

    if (auto quad = entity->getComponent<Quad>()) {
        include(worldCorners(entity, quad->size, quad->offset, quad->centered));
    }
    if (auto sprite = entity->getComponent<Sprite>()) {
        include(worldCorners(entity, sprite->size, sprite->offset, sprite->centered));
    }
    if (auto animator = entity->getComponent<components::SpriteSheetAnimator>()) {
        include(worldCorners(entity, animator->size, animator->drawOffset(), animator->centered));
    }

    hg::Vec2 target;
    if (hasBounds) {
        target = (min + max) * 0.5f;
    } else {
        auto position = entity->position();
        target = hg::Vec2(position[0], position[1]);
    }

    orthographicCamera.position[0] = target[0];
    orthographicCamera.position[1] = target[1];
}

hg::Entity* Editor::addSprite(const std::string& texturePath, hg::Vec2 worldPos) {
    auto parts = hg::utils::f_getParts(texturePath);
    // Textures are named by their asset path without the extension, e.g. "textures/crate"
    auto textureName = hg::utils::s_replace(texturePath, "." + parts.extension, "");

    auto entity = m_scene->entities.add();
    entity->name = parts.name;
    entity->transform.position[0] = worldPos[0];
    entity->transform.position[1] = worldPos[1];

    auto sprite = entity->addComponent<hg::graphics::Sprite>();
    sprite->texture.path(textureName);
    if (auto texture = sprite->texture.asset(); texture && texture->image) {
        sprite->size = texture->image->size.cast<float>() / spritePixelsPerMeter;
    }

    return entity;
}
