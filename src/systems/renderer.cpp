//
// Created by henry on 10/13/24.
//
#include "renderer.h"
#include <hagame/graphics/shaders/text.h>
#include <hagame/graphics/components/spriteSheetAnimator.h>
#include <hagame/utils/profiler.h>
#include <hagame/graphics/shaders/texture.h>
#include <hagame/graphics/components/textRenderer.h>
#include <hagame/graphics/shaders/color.h>
#include <hagame/physics/physics2D.h>
#include <hagame/physics/rigidbody2D.h>
#include <hagame/core/game.h>
#include <hagame/common/components/tilemap.h>
#include "../scenes/runtime.h"

#include "../constants.h"
#include "../common/enums/debugLevel.h"
#include "imgui.h"

using namespace hg;
using namespace hg::graphics;
using namespace hg::utils;
using namespace hg::math::collisions;
using namespace hg::math::components;

HG_SYSTEM(Graphics, Renderer)

Renderer::Renderer(hg::graphics::Window *window):
        m_window(window),
        m_displayQuad((GAME_SIZE.cast<float>()), Vec2(0, 0), true),
        m_display(&m_displayQuad),
        m_animQuad(Vec2(1, 1)),
        m_anim(&m_animQuad)
{
    m_displayQuad.centered(false);
    m_display.update(&m_displayQuad);
}

void Renderer::onInit() {

    auto editor = static_cast<Runtime*>(scene)->editor();
    editor->orthographicCamera.size = GAME_SIZE.cast<float>();

    hg::setMissingTexture(hg::ASSET_DIR + "textures/missing.png");

    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);
    m_renderPasses.create(RenderMode::Geometry, GAME_SIZE, 7, true);
    m_renderPasses.create(RenderMode::Lighting, GAME_SIZE);
    m_renderPasses.create(RenderMode::Debug, GAME_SIZE);
    m_renderPasses.create(RenderMode::UI, GAME_SIZE);
    m_renderPasses.create(RenderMode::Combined, GAME_SIZE);

    m_renderPasses.validate();

    glGenTextures(1, &m_depthTextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthTextures);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, GAME_SIZE[0], GAME_SIZE[1], 5, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glGenFramebuffers(1, &m_depthFBO);

    auto colorShader = getShader(COLOR_SHADER.name);
    auto textShader = getShader(TEXT_SHADER.name);
    auto font = getFont("8bit");

    Debug::Initialize(colorShader, textShader, font);
}

void Renderer::onBeforeUpdate() {

    auto state = GameState::Get();

    auto editor = static_cast<Runtime*>(scene)->editor();

    m_batchRenderer.quads.clear();
    m_batchRenderer.sprites.clear();

    if (state->running) {
        scene->entities.forEach<PerspectiveCamera>([&](hg::graphics::PerspectiveCamera* cam, hg::Entity* entity) {
            view = cam->view();
            projection = cam->projection();
            camPos = entity->position();

            state->hasCamera = true;
        });

        scene->entities.forEach<OrthographicCamera>([&](hg::graphics::OrthographicCamera* cam, hg::Entity* entity) {
            state->mousePos = cam->getGamePos(state->rawMousePos);
            // state->mousePos[1] *= -1;
            view = cam->view();
            projection = cam->projection();
            camPos = entity->position();
            state->hasCamera = true;
        });
    } else {
        if (editor->cameraMode == hge::CameraMode::Orthographic) {
            view = editor->orthographicCamera.view();
            projection = editor->orthographicCamera.projection();
        } else {
            view = editor->perspectiveCamera.view();
            projection = editor->perspectiveCamera.projection();
        }
    }
}

void Renderer::onRender(double dt) {

    auto state = GameState::Get();
    auto editor = ((Runtime*)scene)->editor();
    if (mouseInViewport()) {
        if (state->running) {
            scene->entities.forEach<OrthographicZoom>([&](OrthographicZoom* zoom, auto entity) {
                zoom->update(m_window->input.devices.keyboardMouse()->axes[input::devices::MouseAxes::WheelY] * -1, state->mousePos, dt);
            });

            scene->entities.forEach<OrthographicPan>([&](OrthographicPan* pan, auto entity) {
                pan->update(m_window->input.devices.keyboardMouse()->buttons[input::devices::MouseButtons::Middle], state->rawMousePos);
            });
        } else {
            if (editor->cameraMode == hge::CameraMode::Orthographic) {
                editor->orthographicCamera.update(
                    m_window->input.devices.keyboardMouse()->axes[input::devices::MouseAxes::WheelY] * -1,
                    m_window->input.devices.keyboardMouse()->buttons[input::devices::MouseButtons::Middle],
                    state->rawMousePos,
                    dt);
            } else {

            }
        }
    }

    Profiler::Start("Renderer::onRender");

    m_window->color(Color::black());

    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);
    m_renderPasses.clear(RenderMode::Geometry, Color::black());
    m_renderPasses.clear(RenderMode::Lighting, Color::black());
    m_renderPasses.clear(RenderMode::Debug, Color::transparent());
    m_renderPasses.clear(RenderMode::UI, Color::transparent());
    m_renderPasses.clear(RenderMode::Combined, Color::black());

    m_renderPasses.bind(RenderMode::Geometry);
    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);

    Debug::ENABLED = true;
    
    auto shader = getShader(TEXT_SHADER.name);
    shader->use();

    shader->setMat4("projection", Mat4::Orthographic(0, GAME_SIZE[0], 0, GAME_SIZE[1], -100, 100));
    shader->setMat4("view", Mat4::Identity());

    shader = getShader(COLOR_SHADER.name);
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    m_window->setVSync(state->persistentSettings.vsync);

    prepareGeometry();

    Profiler::Start("Renderer::colorPass");
    colorPass(dt);
    Profiler::End("Renderer::colorPass");
    Profiler::Start("Renderer::lightPass");
    lightPass(dt);
    Profiler::End("Renderer::lightPass");
    Profiler::Start("Renderer::debugPass");
    debugPass(dt);
    Profiler::End("Renderer::debugPass");
    Profiler::End("Renderer::onRender");

    uiPass(dt);
    combinedPass(dt);
}

void Renderer::onUpdate(double dt) {

    auto state = GameState::Get();

    scene->entities.forEach<hg::graphics::components::SpriteSheetAnimator>([&](auto animator, auto entity) {
        animator->update(dt);
    });
}

void Renderer::prepareGeometry() {

    m_geometry.clear();

    scene->entities.forEach<Quad>([&](auto quad, Entity* entity) {
        auto pos = entity->position();
        m_batchRenderer.quads.batch(entity, quad);
    });

    scene->entities.forEach<hg::components::Tilemap>([&](hg::components::Tilemap* tilemap, Entity* entity) {
        for (const auto& layer : tilemap->layers) {
            std::visit(hg::utils::overloaded{
                [&](hg::components::ColorTile const& tile) {
                    layer.map.forEach([&](Vec2i idx) {
                        m_batchRenderer.quads.batch(
                            tilemap->tileSize,
                            tilemap->tileSize * 0.5,
                            tile.color,
                            Mat4::Translation(tilemap->getPos(idx).resize<3>() + entity->transform.position)
                        );
                    });
                },
                [&](hg::components::SpriteTile const& tile) {
                    layer.map.forEach([&](Vec2i idx) {
                        m_batchRenderer.sprites.batch(
                            tile.texture,
                            tilemap->tileSize,
                            tilemap->tileSize * 0.5, Color::white(),
                            Mat4::Translation(tilemap->getPos(idx).resize<3>() + entity->transform.position)
                        );
                    });
                },
                [&](hg::components::SpriteSheetTile const& tile) {
                    auto sheet = getSpriteSheet(tile.sheet);
                    layer.map.forEach([&](Vec2i idx) {
                        m_batchRenderer.sprites.batch(
                             sheet->texture()->image->path,
                             tilemap->tileSize,
                             tilemap->tileSize * 0.5,
                             sheet->atlas.getRect(tile.index, sheet->texture()->image->size),
                             Color::white(),
                             Mat4::Translation(tilemap->getPos(idx).resize<3>() + entity->transform.position)
                         );
                    });
                }
            }, layer.tile.details);
        }

    });

    scene->entities.forEach<Sprite>([&](auto sprite, Entity* entity) {
        //m_batchRenderer.sprites.batch(entity, sprite);
        if (sprite->batched) {
            m_batchRenderer.sprites.batch(
                    sprite->texture.path(),
                    sprite->size,
                    sprite->offset,
                    Rect(Vec2(0, 0), Vec2(1.0, 1.0)),
                    sprite->color,
                    entity->model()
            );
        } else {
            m_geometry.push_back(Renderable{[&, sprite, entity](auto shader) {
                if (!sprite->texture.asset()) return;
                auto position = entity->position();
                m_animQuad.setSizeAndOffset(sprite->size, sprite->offset);
                m_anim.update(&m_animQuad);
                //auto shader = getShader(TEXTURE_SHADER.name);
                sprite->texture.asset()->bind();
                shader->use();
                shader->setMat4("projection", projection);
                shader->setMat4("view", view);
                shader->setMat4("model", entity->model());
                m_anim.render();
            }});
        }
    });

    scene->entities.forEach<hg::graphics::components::SpriteSheetAnimator>([&](auto animator, auto entity) {
        m_geometry.push_back(Renderable{[&, animator, entity](auto shader) {
            auto sheet = animator->sprites.asset();
            if (!sheet) return;
            if (!sheet->sprites->hasSprite(animator->activeSprite)) return;
            sheet->texture()->bind();
            shader->use();
            auto sprite = sheet->sprites->getSprite(animator->activeSprite);
            auto rect = animator->drawRect();
            auto position = entity->position();
            m_animQuad.setSizeAndOffset(animator->size, animator->drawOffset());
            m_animQuad.texSize(rect.size);
            m_animQuad.texOffset(rect.pos);
            m_anim.update(&m_animQuad);
            shader->setMat4("projection", projection);
            shader->setMat4("view", view);
            shader->setMat4("model", entity->model());
            m_anim.render();
        }});
    });
    
    auto shader = getShader(BATCH_COLOR_SHADER.name);
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    m_batchRenderer.quads.render();

    shader = getShader(BATCH_TEXTURE_SHADER.name);
    shader->use();

    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

    for (int i = 0; i < maxTextureUnits; ++i) {
        std::string uniformName = "images[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(shader->id, uniformName.c_str()), i);
    }

    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    auto textures = m_batchRenderer.sprites.m_textures;

    m_batchRenderer.sprites.render();


}

void Renderer::colorPass(double dt) {

    m_renderPasses.bind(RenderMode::Geometry);

    // glEnable(GL_DEPTH_TEST);

    auto shader = getShader(TEXTURE_SHADER.name);
    shader->use();

    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    //shader->setMat4("projection", Mat4::Orthographic(-10, 10, -10, 10, 0.001, 100));
    //shader->setMat4("view", Mat4::LookAt(camera.transform.position, camera.transform.position + Vec3::Face(), Vec3::Top()));

//    scene->entities.forEach<DirectionalLight>([&](auto light, auto entity) {
//        shader->setMat4("projection", Mat4::Orthographic(-10, 10, -10, 10, 1.0, 10.0));
//        shader->setMat4("view", Mat4::LookAt(entity->position(), entity->position() + light->direction, Vec3::Face()));
//
//    });

    std::sort(m_geometry.begin(), m_geometry.end(), [&](const Renderable& a, const Renderable& b) {
        return (camPos - a.pos).magnitudeSq() < (camPos - b.pos).magnitudeSq();
    });

    for (const auto& geometry : m_geometry){
        geometry.render(shader);
    }

    m_renderPasses.render(RenderMode::Geometry, 7);
}

void Renderer::lightPass(double dt) {

    auto state = GameState::Get();

    m_renderPasses.bind(RenderMode::Lighting);

    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);

    auto shader = getShader("shaders/lighting");
    shader->use();

    shader->setFloat("useLighting", 0);
    shader->setMat4("view", Mat4::Identity());
    shader->setMat4("projection", Mat4::Orthographic(0, GAME_SIZE[0] , 0, GAME_SIZE[1] , -100, 100));
    shader->setMat4("model", Mat4::Identity());

    glActiveTexture(GL_TEXTURE0 + 0);
    m_renderPasses.get(RenderMode::Geometry)->textures[0]->bind();
    auto posId = glGetUniformLocation(shader->id, "positionTex");
    glUniform1i(posId, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    m_renderPasses.get(RenderMode::Geometry)->textures[1]->bind();
    glUniform1i(glGetUniformLocation(shader->id, "normalTex"), 1);

    glActiveTexture(GL_TEXTURE0 + 2);
    m_renderPasses.get(RenderMode::Geometry)->textures[2]->bind();
    posId = glGetUniformLocation(shader->id, "albedoTex");
    glUniform1i(glGetUniformLocation(shader->id, "albedoTex"), 2);

    glActiveTexture(GL_TEXTURE0 + 3);
    m_renderPasses.get(RenderMode::Geometry)->textures[3]->bind();
    posId = glGetUniformLocation(shader->id, "diffuseTex");
    glUniform1i(glGetUniformLocation(shader->id, "diffuseTex"), 3);

    glActiveTexture(GL_TEXTURE0 + 4);
    m_renderPasses.get(RenderMode::Geometry)->textures[4]->bind();
    posId = glGetUniformLocation(shader->id, "specularTex");
    glUniform1i(glGetUniformLocation(shader->id, "specularTex"), 4);

    glActiveTexture(GL_TEXTURE0 + 5);
    m_renderPasses.get(RenderMode::Geometry)->textures[5]->bind();
    posId = glGetUniformLocation(shader->id, "emissiveTex");
    glUniform1i(glGetUniformLocation(shader->id, "emissiveTex"), 5);

    glActiveTexture(GL_TEXTURE0 + 6);
    m_renderPasses.get(RenderMode::Geometry)->textures[6]->bind();
    posId = glGetUniformLocation(shader->id, "shininessTex");
    glUniform1i(glGetUniformLocation(shader->id, "shininessTex"), 6);

    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_depthTextures);
    posId = glGetUniformLocation(shader->id, "shadowMapArray");
    glUniform1i(posId, 7);

    int lightIndex = 0;

    shader->setVec3("viewPos", camPos);

    m_display.render();

    m_renderPasses.render(RenderMode::Lighting, 1);
}

void Renderer::debugPass(double dt) {
    m_renderPasses.bind(RenderMode::Debug);
    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);

    scene->entities.forEach<RectCollider>([&](RectCollider *coll, Entity *entity) {
        Debug::DrawRect(coll->getRect(), Color::blue(), 1.0 / 100);
    });

    scene->entities.forEach<CircleCollider>([&](CircleCollider *coll, Entity *entity) {
        Vec2 pos = entity->transform.position.resize<2>();
        Debug::DrawCircle(pos[0] + coll->pos[0], pos[1] + coll->pos[1], coll->radius, Color::blue(),
                          1.0 / 100);
    });

    auto editor = ((Runtime*)scene)->editor();

    for (const auto& tool : editor->m_tools) {
        if (tool->is_open()) {
            tool->renderOverlay(projection, view);
        }
    }

    auto shader = getShader(COLOR_SHADER.name);
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    Debug::Render();

    m_renderPasses.render(RenderMode::Debug, 1);
}

void Renderer::uiPass(double dt) {
    m_renderPasses.bind(RenderMode::UI);
    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);

    m_renderPasses.render(RenderMode::UI, 1);
}

void Renderer::combinedPass(double dt) {

    auto state = GameState::Get();

    m_renderPasses.bind(RenderMode::Combined);

    glDisable(GL_CULL_FACE);

    // glDisable(GL_DEPTH_TEST);
    // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, GAME_SIZE[0], GAME_SIZE[1]);

    auto shader = getShader("shaders/combined");
    shader->use();
//
//    shader->setInt("colorTex", 0);
//    shader->setInt("lightTex", 1);
//    shader->setInt("debugTex", 2);
//    shader->setInt("uiTex", 3);
    shader->setFloat("useLighting", 1);
    shader->setMat4("view", Mat4::Identity());
    shader->setMat4("projection", Mat4::Orthographic(0, GAME_SIZE[0], 0, GAME_SIZE[1], -100, 100));
    shader->setMat4("model", Mat4::Identity());

    ImGui::Begin("Render Passes");

    if (!state->hasCamera) {
        ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "Missing Camera Entity!");
    }

    auto enums = *ENUMS(RenderMode);

    glActiveTexture(GL_TEXTURE0 + 0);
    m_renderPasses.get(RenderMode::Geometry)->textures[0]->bind();
    auto posId = glGetUniformLocation(shader->id, "positionTex");
    glUniform1i(posId, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    m_renderPasses.get(RenderMode::Geometry)->textures[1]->bind();
    glUniform1i(glGetUniformLocation(shader->id, "normalTex"), 1);

    glActiveTexture(GL_TEXTURE0 + 2);
    m_renderPasses.get(RenderMode::Geometry)->textures[2]->bind();
    posId = glGetUniformLocation(shader->id, "albedoTex");
    glUniform1i(glGetUniformLocation(shader->id, "albedoTex"), 2);

    glActiveTexture(GL_TEXTURE0 + 4);
    m_renderPasses.get(RenderMode::Lighting)->textures[0]->bind();
    glUniform1i(glGetUniformLocation(shader->id, "lightTex"), 4);

    glActiveTexture(GL_TEXTURE0 + 5);
    m_renderPasses.get(RenderMode::Debug)->textures[0]->bind();
    glUniform1i(glGetUniformLocation(shader->id, "debugTex"), 5);

    glActiveTexture(GL_TEXTURE0 + 6);
    m_renderPasses.get(RenderMode::UI)->textures[0]->bind();
    glUniform1i(glGetUniformLocation(shader->id, "uiTex"), 6);

    m_display.render();

    m_renderPasses.render(RenderMode::Combined, 1);

    // glActiveTexture(GL_TEXTURE0 + 0);

    m_renderPasses.unbind(RenderMode::Combined);
    glViewport(0, 0, m_window->size()[0], m_window->size()[1]);

    static enum_t renderModeDisplay = RenderMode::Geometry;
    static int textureDisplay = 0;

    if (m_renderPasses.get(renderModeDisplay)->textures.size() > 1) {
        ImGui::SliderInt("Texture ID", &textureDisplay, 0, m_renderPasses.get(renderModeDisplay)->textures.size() - 1);
    }

    ImGui::Image((ImTextureID) m_renderPasses.get(renderModeDisplay)->textures[textureDisplay]->id, ImVec2(GAME_SIZE[0] / 4, GAME_SIZE[1] / 4), ImVec2(0, 1), ImVec2(1, 0));

    for (const auto& e : enums) {
        if (ImGui::Selectable(e.label.c_str(), e.key == renderModeDisplay)) {
            renderModeDisplay = e.key;
            textureDisplay = 0;
        }
    }



    ImGui::End();

}

void Renderer::renderTile(hg::Vec2i index, hg::Vec3 position) {
    auto sheet = getSpriteSheet("tilesheet");
    auto texture = getTexture("textures/tilesheet");
    Rect rect = sheet->atlas.getRect(index, texture->image->size);
    Mat4 model = Mat4::Translation(position);
    m_batchRenderer.sprites.batch("tilesheet", Vec2(1.), Vec2::Identity(), rect, Color::white(), model);
}

bool Renderer::mouseInViewport() const {
    auto pos = GameState::Get()->rawMousePos;
    return pos[0] >= 0 && pos[1] >= 0 && pos[0] < GAME_SIZE[0] && pos[1] < GAME_SIZE[1];
}