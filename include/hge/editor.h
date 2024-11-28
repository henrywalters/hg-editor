//
// Created by henry on 10/6/24.
//

#ifndef HGEDITOR_EDITOR_H
#define HGEDITOR_EDITOR_H

#include "hge.h"
#include "hge/components/entityViewer.h"
#include <hagame/core/game.h>
#include <hagame/utils/aliases.h>
#include <hge/components/fileBrowser.h>
#include <hagame/math/functions.h>
#include <hagame/core/assets.h>

namespace hge {

    const std::function<void()> _defaultFunc = [](){
        throw std::runtime_error("Unimplemented func");
    };

    class Editor {
    public:

        hg::VoidFunc onNewScene = _defaultFunc;
        hg::VoidFunc onPlay = _defaultFunc;
        hg::VoidFunc onPause = _defaultFunc;
        hg::VoidFunc onReset = _defaultFunc;

        explicit Editor(hg::Scene* scene, bool _connectEvents = true);

        void render(hg::Vec2 rawMousePos) {
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

            ImGui::Begin("Output");

            if (ImGui::ImageButton("play", (void*) hg::getTexture("ui/play")->id, ImVec2(15, 15))) {
                m_commandQueue.push_back(onPlay);
                //onPlay();
            }

            ImGui::SameLine();

            if (ImGui::ImageButton("pause", (void*) hg::getTexture("ui/pause")->id, ImVec2(15, 15))) {
                m_commandQueue.push_back(onPause);
                //onPause();
            }

            ImGui::SameLine();

            if (ImGui::ImageButton("reset", (void*) hg::getTexture("ui/reset")->id, ImVec2(15, 15))) {
                m_entities.selected(nullptr);
                m_commandQueue.push_back(onReset);
                //onReset();
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

            auto minPoint = ImGui::GetItemRectMin();
            auto scale = size[0] / finalSize[0];

            m_mousePos = hg::Vec2((rawMousePos[0] - minPoint.x) * scale, (rawMousePos[1] - minPoint.y) * scale);
            m_mousePos[1] = size[1] - m_mousePos[1];

            ImGui::End();

            ImGui::End();

            m_browser.render();

            while (m_commandQueue.size() > 0) {
                auto front = m_commandQueue.front();
                m_commandQueue.pop_front();
                front();
            }
        }

        void setOutput(void* m_textureId, hg::Vec2i size);

        void connectEvents();

        void saveAs();
        void saveToDisc();
        void loadFromDisc();
        void savePrefab(hg::Entity* entity);

        hg::Vec2 mousePos() const;

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
    };
}

#endif //HGEDITOR_EDITOR_H
