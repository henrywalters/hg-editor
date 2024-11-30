//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_RUNTIME_H
#define HGEDITOR_RUNTIME_H

#include <hagame/core/scene.h>
#include <hagame/graphics/window.h>
#include <hge/editor.h>


ENUM(TestEnum)
ENUM_VALUE(TestEnum, Hello)
ENUM_VALUE(TestEnum, World)

class Runtime : public hg::Scene {
public:

    Runtime(hg::graphics::Window* window);

    void onInit() override;
    void onUpdate(double dt) override;

    hge::Editor* editor() { return &m_editor; }

private:

    hg::utils::enum_t testEnum;

    hge::Editor m_editor;

    hg::graphics::Window* m_window;

};

#endif //HGEDITOR_RUNTIME_H
