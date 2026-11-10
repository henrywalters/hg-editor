//
// Created by henry on 10/13/24.
//

#ifndef HGEDITOR_TOOL_H
#define HGEDITOR_TOOL_H

#include <hagame/core/object.h>
#include <hagame/core/scene.h>
#include <hge/components/entityTree.h>
#include <hagame/input/input.h>

namespace hge {

    class Tool : public hg::Object {
    public:

        Tool(hg::Scene *scene) :
                m_scene(scene) {}

        void init() {
            onInit();
        }

        void update(hg::input::Input* input, double dt) {
            if (m_open) {
                onUpdate(input, dt);
            }
        }

        void render(double dt) {
            if (m_open) {
                ImGui::Begin(getButtonLabel().c_str(), &m_open);
                renderUI(dt);
                ImGui::End();
            }
        }

        bool is_open() {
            return m_open;
        }

        void open() {
            onOpen();
            m_open = true;
        }

        void close() {
            m_open = false;
        }

        virtual std::string getButtonLabel() = 0;

        virtual std::string getName() = 0;

        virtual void renderOverlay(hg::Mat4 projection, hg::Mat4 view) {};

        // Set by the editor each frame, before update
        void setMouse(hg::Vec2 worldPos, bool overViewport) {
            m_mousePos = worldPos;
            m_mouseOverViewport = overViewport;
        }

        // While true the editor doesn't pick entities from viewport clicks, so the tool can use them
        virtual bool capturesViewportClicks() { return false; }

    protected:
        OBJECT_NAME(Tool)

        hg::Scene *m_scene;

        bool m_open = false;

        // The mouse in world space, under the camera the viewport was drawn with
        hg::Vec2 m_mousePos;
        // Whether the mouse is over the scene image, rather than another window
        bool m_mouseOverViewport = false;

        virtual void onInit() {}

        virtual void onUpdate(hg::input::Input* input, double dt) {};

        virtual void renderUI(double dt) {};

        virtual void onOpen() {};
    };

    template <typename T>
    concept IsTool = std::is_base_of<Tool, T>::value;
}

#endif //HGEDITOR_TOOL_H
