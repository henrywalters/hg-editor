//
// Created by henry on 10/3/24.
//
#include "runtime.h"
#include "hge/core/events.h"
#include "hge/widgets/enumField.h"
#include "hge/components/entityViewer.h"
#include "imgui.h"
#include "../common/gameState.h"
#include "../systems/renderer.h"

Runtime::Runtime(hg::graphics::Window *window):
    m_window(window),
    m_editor(this)
{}


void Runtime::onInit() {
    auto state = GameState::Get();

    addSystem<Renderer>(m_window);

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

    auto state = GameState::Get();

    auto renderer = getSystem<Renderer>();

    m_editor.render(
            m_window->input.devices.keyboardMouse()->mousePosition(),
            true,
            hg::Recti(hg::Vec2i::Zero(), renderer->m_window->size()),
            renderer->projection,
            renderer->view
    );

    state->rawMousePos = m_editor.mousePos();

    ImGui::Begin("Runtime");
    ImGui::Text("Elapsed Time: %f", state->elapsedTime);
    ImGui::Text("Raw Mouse Pos: %s", state->rawMousePos.toString().c_str());
    ImGui::Text("Mouse Pos: %s", state->mousePos.toString().c_str());

    // hge::enumField("Test", &testEnum, ENUMS(TestEnum));

    ImGui::End();

    auto texture = renderer->m_renderPasses.get(RenderMode::Combined)->textures[0].get();
    m_editor.setOutput((void*)texture->id, texture->size);

    if (state->running) {
        state->elapsedTime += dt;
    }

}
