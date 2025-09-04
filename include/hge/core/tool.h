//
// Created by henry on 10/13/24.
//

#ifndef HGEDITOR_TOOL_H
#define HGEDITOR_TOOL_H

#include <hagame/core/object.h>
#include <hagame/core/scene.h>
#include <hge/components/entityTree.h>

namespace hge {

    class Tool : public hg::Object {
    public:

        Tool(hg::Scene *scene) :
                m_scene(scene) {}

        void init() {
            onInit();
        }

        void update(hg::Vec2 mousePos, double dt) {
            m_mousePos = mousePos;
            if (m_open) {

                onUpdate(dt);
            }
        }

        void render(double dt) {
            if (m_open) {
                ImGui::Begin(getButtonLabel().c_str(), &m_open);
                renderUI(dt);
                ImGui::End();
                renderOverlay();
            }
        }

        void open() {
            m_open = true;
        }

        void close() {
            m_open = false;
        }

        virtual std::string getButtonLabel() = 0;

        virtual std::string getName() = 0;

    protected:
        OBJECT_NAME(Tool)

        hg::Scene *m_scene;

        bool m_open = false;

        hg::Vec2 m_mousePos;

        virtual void onInit() {}

        virtual void onUpdate(double dt) {};

        virtual void renderUI(double dt) {};

        virtual void renderOverlay() {};
    };

    template <typename T>
    concept IsTool = std::is_base_of<Tool, T>::value;
}

#endif //HGEDITOR_TOOL_H
