//
// Created by henry on 10/3/24.
//
#include "runtime.h"
#include "hge/core/events.h"
#include "hge/components/entityViewer.h"
#include "imgui.h"

Runtime::Runtime(hg::graphics::Window *window):
    m_window(window),
    m_editor(this)
{}

void Runtime::onInit() {

}

void Runtime::onUpdate(double dt) {
    m_editor.render();
}
