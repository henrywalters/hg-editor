//
// Created by henry on 7/24/26.
//
#include "hge/tools/tilemapEditor.h"

#include <filesystem>
#include <format>
#include <hagame/core/assets.h>
#include <hagame/core/entity.h>
#include <hagame/graphics/shaders/color.h>
#include <hagame/math/functions.h>

#include "../../thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "hge/components/dragTarget.h"

using namespace hge;
using namespace hg;
using namespace hg::graphics;
using namespace hg::graphics::primitives;
using namespace hg::components;

TilemapEditor::TilemapEditor(hg::Scene* scene):
    m_grid(DEFAULT_CELL_SIZE, DEFAULT_CELLS, 2),
    m_gridMesh(&m_grid),
    m_cursor(DEFAULT_CELL_SIZE, Vec2::Zero(), false, false),
    m_cursorMesh(&m_cursor),
    Tool(scene) {
        Events()->subscribe(EventTypes::OpenTool, [&](const auto& e) {
            auto payload = std::get<ToolEvent>(e.payload);
            if (getName() == payload.toolName) {
                if (payload.component) {
                    setTilemap(static_cast<Tilemap*>(payload.component));
                }
                open();
            }
        });
        Events()->subscribe(EventTypes::SelectEntity, [&](const auto& e) {
            auto payload = std::get<EntityEvent>(e.payload);
            if (!payload.entity) return;
            auto tilemap = payload.entity->template getComponentInChildren<Tilemap>();
            if (tilemap) {
                setTilemap(tilemap);
                open();
            } else {
                close();
            }

        });
}

void TilemapEditor::renderOverlay(hg::Mat4 projection, hg::Mat4 view) {
    auto map = tilemap();
    if (!map || map->tileSize[0] <= 0 || map->tileSize[1] <= 0) return;

    if (map->tileSize != m_gridTileSize) {
        updateSize();
    }

    auto tilePos = origin() + map->getPos(m_tileIdx);

    auto shader = getShader(COLOR_SHADER.name);
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    shader->setMat4("model", Mat4::Translation((tilePos - map->tileSize.prod((DEFAULT_CELLS / 2).cast<float>())).resize<3>()));

    shader->setVec4("color", hg::graphics::Color(255, 0, 0, 100));

    if (m_showGrid)
        m_gridMesh.render();


    shader->setVec4("color", hg::graphics::Color(0, 0, 255, 100));

    shader->setMat4("model", Mat4::Translation((tilePos - (m_cursorSize / 2).cast<float>().prod(map->tileSize)).resize<3>()));

    m_cursorMesh.render();
}

void TilemapEditor::onUpdate(hg::input::Input* input, double dt) {

    auto map = tilemap();
    if (!map) return;

    if (ImGui::GetDragDropPayload() != nullptr) return;

    if (map->tileSize[0] <= 0 || map->tileSize[1] <= 0) return;

    m_tileIdx = map->getIndex(m_mousePos - origin());

    // Clicks on the tool's own window (e.g. the palette) shouldn't paint the tiles behind it
    if (!m_mouseOverViewport) {
        m_lastTileIdx = m_tileIdx;
        return;
    }

    bool hasSelection = m_selectedTile >= 0 && m_selectedTile < map->layers.size();

    if (hasSelection && input->devices.keyboardMouse()->buttons[hg::input::devices::MouseButtons::Left]) {
        auto& layer = map->layers[m_selectedTile];
        cursor([&](Vec2i idx) {
            // Painting over another tile replaces it
            if (!layer.map.has(idx)) {
                map->removeTile(idx);
                layer.map.add(idx);
            }
        });

    }

    if (input->devices.keyboardMouse()->buttons[hg::input::devices::MouseButtons::Right]) {
        cursor([&](Vec2i idx) {
            map->removeTile(idx);
        });
    }

    m_lastTileIdx = m_tileIdx;
}

void TilemapEditor::renderUI(double dt) {

    auto map = tilemap();
    if (!map) return;

    ImGui::Checkbox("Show Grid", &m_showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Show Normals", &m_showNormals);

    if (ImGui::SliderInt2("Cursor Size (Odd only)", m_cursorSize.vector.data(), 1, 25)) {
        for (int i = 0; i < 2; i++) {
            if (m_cursorSize[i] % 2 == 0) m_cursorSize[i] += 1;
        }
        updateSize();
    }

    if (ImGui::InputFloat2("Tile Size", map->tileSize.vector.data())) {
        updateSize();
    }

    if (map->tileSize[0] <= 0 || map->tileSize[1] <= 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Set a tile size (in world units) to paint");
    }

    auto tilemaps = findTilemaps();
    if (tilemaps.size() > 1) {
        if (ImGui::BeginCombo("Current Tilemap", map->entity->name.c_str())) {

            for (const auto& other : tilemaps) {
                if (ImGui::Selectable(other->entity->name.c_str(), other == map)) {
                    setTilemap(other);
                }
            }

            ImGui::EndCombo();
        }
    }

    renderPalette();

    ImGui::SeparatorText("Tiles");

    if (ImGui::Button("Add Layer")) {
        map->layers.push_back(TilemapLayer{});
        m_selectedTile = map->layers.size() - 1;
    }

    std::optional<int> removeLayer = std::nullopt;

    int index = 0;

    float swatchSize = 40.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

    for (auto& layer : map->layers) {
        ImGui::PushID(index);

        bool selected = (m_selectedTile == index);

        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        }

        bool clicked = tileButton("##swatch", layer.tile, swatchSize);

        if (selected)
        {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        if (clicked)
            m_selectedTile = index;

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", layer.tile.name.c_str());

        if (ImGui::BeginPopupContextItem("##swatchContext"))
        {
            ImGui::Text("%s", layer.tile.name.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Delete"))
            {
                removeLayer = index;
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();

        float lastX2 = ImGui::GetItemRectMax().x;
        float nextX2 = lastX2 + spacing + swatchSize;
        bool isLast = (index == (int)map->layers.size() - 1);
        if (!isLast && nextX2 < windowVisibleX2)
            ImGui::SameLine();

        index++;
    }

    if (m_selectedTile >= 0 && m_selectedTile < map->layers.size()) {
        renderTileDetails(map->layers[m_selectedTile].tile);
    }

    if (removeLayer.has_value()) {
        map->layers.erase(map->layers.begin() + removeLayer.value());
        if (m_selectedTile >= map->layers.size()) {
            m_selectedTile--;
        }
    }

    ImGui::Separator();

    dragTarget("Drag asset to create tile", 60, {
        {"png", [&](std::string const& path) {
                SpriteTile tile {
                    .texture = hg::utils::s_replace(path, ".png", "")
                };
                map->layers.push_back(TilemapLayer{
                    .tile = Tile {
                        .name = hg::utils::s_replace(path, ".png", ""),
                        .collider = true,
                        .type = TileType::Sprite,
                        .details = tile,
                    }
                });
                m_selectedTile = map->layers.size() - 1;
        }}
    });
}

void TilemapEditor::renderPalette() {
    ImGui::SeparatorText("Tile Sheet");

    dragTarget("Drag a sprite sheet (.hgss) here", 40, {
        {"hgss", [&](std::string const& path) {
            auto name = hg::utils::s_replace(path, ".hgss", "");
            if (loadSheet(name)) {
                m_sheet = name;
            }
        }}
    });

    if (!m_sheetError.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_sheetError.c_str());
    }

    auto sheet = getSheet(m_sheet);
    if (!sheet || !sheet->texture()) {
        ImGui::TextDisabled("No sheet. Make one in the Sprite Sheet Editor by setting its grid to the tile size.");
        return;
    }

    auto texture = displayTexture(sheet->texturePath());
    auto imageSize = sheet->texture()->image->size;
    auto cells = sheet->atlas.size;

    ImGui::Text("%s (%d x %d tiles)", m_sheet.c_str(), cells[0], cells[1]);
    ImGui::Checkbox("New tiles are colliders", &m_newTilesCollide);
    ImGui::SliderFloat("Palette Zoom", &m_paletteZoom, 0.5f, 6.0f);

    // Highlight the cell the selected layer draws
    std::optional<Vec2i> selectedCell;
    auto map = tilemap();
    if (m_selectedTile >= 0 && m_selectedTile < map->layers.size()) {
        if (auto tile = std::get_if<SpriteSheetTile>(&map->layers[m_selectedTile].tile.details)) {
            if (tile->sheet == m_sheet) {
                selectedCell = tile->index;
            }
        }
    }

    auto cellSize = sheet->atlas.getCellSize(imageSize) * m_paletteZoom;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));

    for (int y = 0; y < cells[1]; y++) {
        for (int x = 0; x < cells[0]; x++) {
            Vec2i cell(x, y);
            auto rect = sheet->atlas.getRect(cell, imageSize);
            bool selected = selectedCell.has_value() && selectedCell.value() == cell;
            bool used = findSheetLayer(m_sheet, cell) >= 0;

            ImGui::PushID(y * cells[0] + x);

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            }

            // Cells already in the tilemap get a faint background
            auto background = used ? ImVec4(0.3f, 0.5f, 0.8f, 0.35f) : ImVec4(0, 0, 0, 0);

            if (ImGui::ImageButton("##cell", texture->id, ImVec2(cellSize[0], cellSize[1]),
                                   ImVec2(rect.pos[0], rect.pos[1]),
                                   ImVec2(rect.pos[0] + rect.size[0], rect.pos[1] + rect.size[1]),
                                   background)) {
                selectSheetTile(m_sheet, cell);
            }

            if (selected) {
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("[%d, %d]%s", x, y, used ? " (in tilemap)" : "");
            }

            ImGui::PopID();

            if (x < cells[0] - 1) {
                ImGui::SameLine();
            }
        }
    }

    ImGui::PopStyleVar(2);
}

void TilemapEditor::renderTileDetails(Tile& tile) {
    ImGui::SeparatorText("Selected Tile");

    ImGui::InputText("Tile Name", &tile.name);

    ImGui::Checkbox("Collider?", &tile.collider);

    if (ImGui::BeginCombo("Tile Type", TileType::labelFor(tile.type).c_str())) {
        for (const auto& type : *TileType::values()) {
            if (ImGui::Selectable(type.label.c_str(), tile.type == type)) {
                auto& layer = tilemap()->layers[m_selectedTile];
                setTileType(layer, type); // operate on this layer, not a shared member
            }
        }
        ImGui::EndCombo();
    }

    std::visit(utils::overloaded{
        [&](ColorTile& details) {
            ImGui::ColorPicker4("Color", details.color.vector.data(),
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        },
        [&](SpriteTile& details) {
            auto texture = displayTexture(details.texture);
            ImGui::Image(texture->id, ImVec2(64, 64));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("png")) {
                    details.texture = hg::utils::s_replace(std::string(static_cast<char*>(payload->Data)), ".png", "");
                }
                ImGui::EndDragDropTarget();
            }
        },
        [&](SpriteSheetTile& details) {
            tileButton("##preview", tile, 64);
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("hgss")) {
                    auto name = hg::utils::s_replace(std::string(static_cast<char*>(payload->Data)), ".hgss", "");
                    if (loadSheet(name)) {
                        details.sheet = name;
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();
            ImGui::Text("%s", details.sheet.empty() ? "Drag a sprite sheet onto the preview" : details.sheet.c_str());

            if (ImGui::InputInt2("Cell", details.index.vector.data())) {
                if (auto sheet = getSheet(details.sheet)) {
                    for (int i = 0; i < 2; i++) {
                        details.index[i] = std::clamp(details.index[i], 0, sheet->atlas.size[i] - 1);
                    }
                }
            }
        }
    }, tile.details);
}

Texture* TilemapEditor::displayTexture(std::string const& name) {
    auto normal = name + "_normal";
    if (m_showNormals && assets::TEXTURES.has(normal)) {
        return getTexture(normal);
    }
    return getTexture(name);
}

bool TilemapEditor::tileButton(const char* id, Tile const& tile, float size) {
    return std::visit(utils::overloaded{
        [&](ColorTile const& details) -> bool {
            return ImGui::ColorButton(id, ImVec4(details.color[0], details.color[1], details.color[2], details.color[3]),
                                      ImGuiColorEditFlags_NoTooltip, ImVec2(size, size));
        },
        [&](SpriteTile const& details) -> bool {
            return ImGui::ImageButton(id, displayTexture(details.texture)->id, ImVec2(size, size));
        },
        [&](SpriteSheetTile const& details) -> bool {
            auto sheet = getSheet(details.sheet);
            if (!sheet || !sheet->texture()) {
                return ImGui::Button(id, ImVec2(size, size));
            }
            auto texture = displayTexture(sheet->texturePath());
            auto rect = sheet->atlas.getRect(details.index, sheet->texture()->image->size);
            return ImGui::ImageButton(id, texture->id, ImVec2(size, size),
                                      ImVec2(rect.pos[0], rect.pos[1]),
                                      ImVec2(rect.pos[0] + rect.size[0], rect.pos[1] + rect.size[1]));
        }
    }, tile.details);
}

SpriteSheetV2* TilemapEditor::loadSheet(std::string const& name) {
    m_sheetError = "";
    if (hasSpriteSheet(name)) {
        return getSpriteSheet(name);
    }
    auto path = ASSET_DIR + name + ".hgss";
    if (!std::filesystem::exists(path)) {
        m_sheetError = "Sprite sheet not found: " + path;
        return nullptr;
    }
    try {
        return loadSpriteSheet(name, path).get();
    } catch (std::exception const& e) {
        m_sheetError = std::format("Couldn't load {}: {}", path, e.what());
        return nullptr;
    }
}

SpriteSheetV2* TilemapEditor::getSheet(std::string const& name) {
    if (name.empty() || !hasSpriteSheet(name)) {
        return nullptr;
    }
    return getSpriteSheet(name);
}

int TilemapEditor::findSheetLayer(std::string const& sheet, Vec2i index) {
    auto map = tilemap();
    for (int i = 0; i < map->layers.size(); i++) {
        auto tile = std::get_if<SpriteSheetTile>(&map->layers[i].tile.details);
        if (tile && tile->sheet == sheet && tile->index == index) {
            return i;
        }
    }
    return -1;
}

void TilemapEditor::selectSheetTile(std::string const& sheet, Vec2i index) {
    auto map = tilemap();
    int layer = findSheetLayer(sheet, index);
    if (layer < 0) {
        auto sheetName = std::filesystem::path(sheet).filename().string();
        map->layers.push_back(TilemapLayer{
            .tile = Tile {
                .name = std::format("{} [{}, {}]", sheetName, index[0], index[1]),
                .collider = m_newTilesCollide,
                .type = TileType::SpriteSheet,
                .details = SpriteSheetTile {
                    .sheet = sheet,
                    .index = index,
                },
            }
        });
        layer = map->layers.size() - 1;
    }
    m_selectedTile = layer;
}

void TilemapEditor::onOpen() {
    if (!tilemap()) {
        auto tilemaps = findTilemaps();
        if (tilemaps.size() > 0) {
            setTilemap(tilemaps[0]);
        }
    }
}

std::vector<Tilemap*> TilemapEditor::findTilemaps() {
    std::vector<Tilemap*> tilemaps;
    // Inactive ones too, so a tilemap switched off can still be edited
    m_scene->entities.forEach([&](hg::Entity* entity) {
        for (const auto& component : entity->components()) {
            if (component->className() == "Tilemap") {
                tilemaps.push_back(static_cast<Tilemap*>(component));
            }
        }
    }, true);
    return tilemaps;
}

Tilemap* TilemapEditor::tilemap() {
    if (!m_tilemapEntity.has_value()) {
        return nullptr;
    }
    auto entity = m_scene->entities.get(m_tilemapEntity.value());
    return entity ? entity->getComponent<Tilemap>() : nullptr;
}

Vec2 TilemapEditor::origin() {
    return tilemap()->entity->position().resize<2>();
}

void TilemapEditor::updateSize() {
    auto map = tilemap();
    m_gridTileSize = map->tileSize;

    m_grid.cellSize(map->tileSize);
    // Lines a fraction of a tile wide, so the grid reads the same whatever units the scene uses
    m_grid.thickness(std::min(map->tileSize[0], map->tileSize[1]) * 0.04f);
    m_gridMesh.update(&m_grid);

    m_cursor.size(map->tileSize.prod(m_cursorSize.cast<float>()));
    m_cursorMesh.update(&m_cursor);
}

void TilemapEditor::setTilemap(Tilemap *tilemap) {
    m_tilemapEntity = tilemap->entity->id();

    updateSize();

    m_selectedTile = tilemap->layers.size() == 0 ? -1 : 0;

    // Show the sheet this tilemap already uses
    if (m_sheet.empty()) {
        for (const auto& layer : tilemap->layers) {
            if (auto tile = std::get_if<SpriteSheetTile>(&layer.tile.details)) {
                m_sheet = tile->sheet;
                loadSheet(m_sheet);
                break;
            }
        }
    }
}

void TilemapEditor::setTileType(TilemapLayer& layer, hg::utils::enum_t type) {
    layer.tile.type = type;
    if (type == TileType::Color) {
       layer.tile.details = ColorTile{};
    } else if (type == TileType::Sprite) {
        layer.tile.details = SpriteTile{};
    } else if (type == TileType::SpriteSheet) {
        layer.tile.details = SpriteSheetTile{};
    } else {
        throw std::runtime_error("Unhandled Tile Type in Tilemap Editor");
    }
}

void TilemapEditor::cursor(std::function<void(hg::Vec2i)> fn) {
    for (const auto& idx : bresenham(m_lastTileIdx, m_tileIdx)) {
        for (int h = idx.x() - m_cursorSize.x() / 2; h <= idx.x() + m_cursorSize.x() / 2; h++) {
            for (int v = idx.y() - m_cursorSize.y() / 2; v <= idx.y() + m_cursorSize.y() / 2; v++) {
                fn(hg::Vec2i(h, v));
            }
        }
    }
}
