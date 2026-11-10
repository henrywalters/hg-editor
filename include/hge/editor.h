//
// Created by henry on 10/6/24.
//

#ifndef HGEDITOR_EDITOR_H
#define HGEDITOR_EDITOR_H

#include "hge.h"
#include "hge/components/entityViewer.h"
#include "hge/core/tool.h"
#include <hagame/core/game.h>
#include <hagame/utils/aliases.h>
#include <hge/components/gizmo.h>
#include <hge/components/fileBrowser.h>
#include <hagame/math/functions.h>
#include <hagame/core/assets.h>

#include "ImGuizmo.h"
#include "core/camera.h"

namespace hge {

    const std::function<void()> _defaultFunc = [](){
        throw std::runtime_error("Unimplemented func");
    };

    class Editor {
    public:
        CameraMode cameraMode = CameraMode::Orthographic;
        OrthographicCamera orthographicCamera;
        PerspectiveCamera perspectiveCamera;

        hg::VoidFunc onNewScene = _defaultFunc;
        hg::VoidFunc onPlay = _defaultFunc;
        hg::VoidFunc onPause = _defaultFunc;
        hg::VoidFunc onReset = _defaultFunc;

        // Called inside the main menu bar after the editor's own menus, so the game can add menus of its own
        hg::VoidFunc onMenuBar = _defaultFunc;

        // Click in the viewport to select the entity under the mouse. Turn off while the game is running.
        bool enablePicking = true;

        // Texture pixels per world unit for a .png dropped on the viewport, so the new Sprite is its pixel-art size
        float spritePixelsPerMeter = 32;

        explicit Editor(hg::Scene* scene, bool _connectEvents = true);

        void render(hg::input::Input* input, hg::Recti window, hg::Mat4 proj, hg::Mat4 view);

        void setOutput(void* m_textureId, hg::Vec2i size);

        void connectEvents();

        void saveAs();
        void saveToDisc();
        void loadFromDisc();
        void savePrefab(hg::Entity* entity);

        hg::Vec2 mousePos() const;

        // Moves a new top-level entity to the middle of the view
        void placeInView(hg::Entity* entity);

        // Centres the orthographic camera on an entity: the middle of what it draws, or its position if it draws nothing
        void focus(hg::Entity* entity);

        // Makes a new top-level entity with a Sprite of the texture (an asset path like "textures/crate.png") at a world position
        hg::Entity* addSprite(const std::string& texturePath, hg::Vec2 worldPos);

        // The front-most entity drawn at a world position (prefab instances are picked as a whole), or null
        hg::Entity* pickEntity(hg::Vec2 worldPos);

        template <IsTool ToolType, typename... Args>
        std::shared_ptr<ToolType> registerTool(Args&&... args) {
            auto tool = std::make_shared<ToolType>(std::forward<Args>(args)...);
            m_tools.push_back(tool);
            return tool;
        }

        std::vector<std::shared_ptr<Tool>> m_tools;

    private:

        std::deque<std::function<void()>> m_commandQueue;

        std::string m_saveFile;

        bool m_hasTexture = false;
        hg::Vec2i m_textureSize;
        void* m_textureId;


        hg::Vec2 m_mousePos;

        hg::Scene* m_scene;
        FileBrowser m_browser;
        AssetBrowser m_assets;
        EntityTree m_entities;
        Gizmo m_gizmo;
    };
}

#endif //HGEDITOR_EDITOR_H
