//
// Created by henry on 10/3/24.
//
#include "runtime.h"
#include "hge/core/events.h"
#include "hge/widgets/enumField.h"
#include "hge/components/entityViewer.h"
#include "imgui.h"
#include "../common/gameState.h"

Runtime::Runtime(hg::graphics::Window *window):
    m_window(window),
    m_editor(this)
{}


void Runtime::onInit() {
    auto state = GameState::Get();

    m_editor.onNewScene = [&, state](){
        game()->scenes()->active()->clear();
    };

    m_editor.onReset = [&, state](){
        state->elapsedTime = 0;
        state->running = false;
    };

    m_editor.onPause = [&, state]() {
        state->running = false;
    };

    m_editor.onPlay = [&, state]() {
        state->running = true;
    };
}

void Runtime::onUpdate(double dt) {
    m_editor.render(m_window->input.devices.keyboardMouse()->mousePosition());

    auto state = GameState::Get();

    ImGui::Begin("Runtime");
    ImGui::Text("Elapsed Time: %f", state->elapsedTime);

    // hge::enumField("Test", &testEnum, ENUMS(TestEnum));

    ImGui::End();

    if (state->running) {
        state->elapsedTime += dt;
    }

}
