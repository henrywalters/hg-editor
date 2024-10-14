//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_RUNTIME_H
#define HGEDITOR_RUNTIME_H

#include <hagame/core/scene.h>
#include <hagame/graphics/window.h>
#include <hge/editor.h>

class Runtime : public hg::Scene {
public:

    Runtime(hg::graphics::Window* window);

    void onInit() override;
    void onUpdate(double dt) override;

private:

    hge::Editor m_editor;

    hg::graphics::Window* m_window;

};

#endif //HGEDITOR_RUNTIME_H
