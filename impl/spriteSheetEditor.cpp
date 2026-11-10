//
// Created by henry on 8/24/26.
//
#include <hagame/core/assets.h>
#include <hge/tools/spriteSheetEditor.h>

#include "hge/components/dragTarget.h"
#include "../../thirdparty/imgui/misc/cpp/imgui_stdlib.h"

using namespace hge;
using namespace hg;
using namespace hg::graphics;

SpriteSheetEditor::SpriteSheetEditor(Scene* scene):
    Tool(scene) {
    Events()->subscribe(EventTypes::OpenTool, [&](auto const& e) {
        auto payload = std::get<ToolEvent>(e.payload);
            if (getName() == payload.toolName) {
                open();
            }
    });
}

void SpriteSheetEditor::onOpen() {
    m_sheet = nullptr;
    m_selectedCell = std::nullopt;
}

void SpriteSheetEditor::renderUI(double dt) {

    float width = ImGui::GetContentRegionAvail().x;
    float height = ImGui::GetContentRegionAvail().y;
    float sidebarWidth = 300;
    float dragTargetHeight = 60;

    ImGuiStyle& style = ImGui::GetStyle();

    // One gap between the child row and dragTarget below it
    float panelHeight = height - dragTargetHeight - style.ItemSpacing.y;

    // One gap between Left and Right children (SameLine uses ItemSpacing.x by default)
    float leftWidth = width - sidebarWidth - style.ItemSpacing.x * 2;
    float imageWidth = leftWidth;

    if (m_sheet) {
        ImGui::BeginChild("Left", ImVec2(leftWidth, panelHeight), true, ImGuiWindowFlags_NoMove);
        auto texture = m_sheet->sprites->texture();
        float aspect = static_cast<float>(texture->image->size[0]) / texture->image->size[1];

        // Also clamp against the child's OWN padding, since Image draws inside it
        float availInsideChild = ImGui::GetContentRegionAvail().y;
        float imgH = std::min(imageWidth / aspect, availInsideChild);
        float imgW = imgH * aspect;

        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        ImGui::Image(texture->id, ImVec2(imgW, imgH));
        // Take the mouse so clicking cells doesn't drag the window
        ImGui::SetCursorScreenPos(imagePos);
        ImGui::InvisibleButton("##sheetCanvas", ImVec2(imgW, imgH), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool imageHovered = ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImU32 gridColor = IM_COL32(255, 0, 0, 150);

        int cols = m_sheet->atlas.size.x();
        int rows = m_sheet->atlas.size.y();

        float cellWidth = imgW / cols;
        float cellHeight = imgH / rows;

        auto& sprite = m_sprites[m_activeSprite];
        auto& spriteGroup = sprite.sprite.group();

        if (m_mode == Mode::Sprites) {
            for (const auto& idx : sprite.sprite.group()) {
                auto pos = idx.cast<float>().prod(Vec2(cellWidth, cellHeight));
                drawList->AddRectFilled(
                    ImVec2(imagePos.x + pos.x(), imagePos.y + pos.y()),
                    ImVec2(imagePos.x + pos.x() + cellWidth, imagePos.y + pos.y() + cellHeight),
                    ImColor(0, 0, 255, 50)
                    );
            }
        } else {
            drawCellColliders(imagePos, cellWidth, cellHeight);
        }

        ImVec2 mousePos = ImGui::GetMousePos();

        auto idx = Vec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y).div(Vec2(cellWidth, cellHeight)).cast<int>();
        bool inSheet = idx.x() >= 0 && idx.y() >= 0 && idx.x() < m_sheet->atlas.size.x() && idx.y() < m_sheet->atlas.size.y();

        if (m_mode == Mode::TileColliders) {
            if (inSheet && imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_selectedCell = idx;
                m_selectedShape = 0;
                m_dragPoint = std::nullopt;
            }
        } else if (inSheet) {
            auto getIdx = [&]() {
                for (int i = 0; i < spriteGroup.size(); i++) {
                    if (spriteGroup[i] == idx) {
                        return i;
                    }
                }
                return -1;
            };

            auto groupIdx = getIdx();

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (groupIdx == -1)
                    spriteGroup.push_back(idx);
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (groupIdx != -1) {
                    spriteGroup.erase(spriteGroup.begin() + groupIdx);
                }
            }
        }

        // Vertical lines
        for (int i = 0; i <= cols; i++) {
            float x = imagePos.x + (imgW * i) / cols;
            drawList->AddLine(ImVec2(x, imagePos.y), ImVec2(x, imagePos.y + imgH), gridColor);
        }

        // Horizontal lines
        for (int i = 0; i <= rows; i++) {
            float y = imagePos.y + (imgH * i) / rows;
            drawList->AddLine(ImVec2(imagePos.x, y), ImVec2(imagePos.x + imgW, y), gridColor);
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Right", ImVec2(sidebarWidth, panelHeight), true, ImGuiWindowFlags_NoMove);
        ImGui::DragInt2("Tile Count", m_sheet->atlas.size.vector.data(), 1, 1);
        if (m_sheet->atlas.size.x() == 0) {
            m_sheet->atlas.size[0] = 1;
        }
        if (m_sheet->atlas.size.y() == 0) {
            m_sheet->atlas.size[1] = 1;
        }

        if (ImGui::Button("Save")) {
            m_browser.saveFile(m_path, [&](std::filesystem::path const& path) {
                saveSprites();
                auto json = m_sheet->save();
                auto filePath = path.string() + (utils::s_endsWith(path, ".hgss") ? "" : ".hgss");
                utils::f_write(filePath, json.dump(4));
                loadSpriteSheet(m_path, filePath);
                m_sheet = std::shared_ptr<SpriteSheetV2>(getSpriteSheet(m_path), [&](SpriteSheetV2* sheet) {});
                loadSprites();
            }, {".hgss"});
        }

        std::optional<int> toRemove = std::nullopt;

        if (ImGui::RadioButton("Sprites", m_mode == Mode::Sprites)) {
            m_mode = Mode::Sprites;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Tile Colliders", m_mode == Mode::TileColliders)) {
            m_mode = Mode::TileColliders;
        }

        if (m_mode == Mode::TileColliders) {
            renderColliderEditor();
        } else {
            ImGui::SeparatorText("Sprite Groups");

            ImGui::InputText("##NewSpriteName", &m_spriteName);
            ImGui::SameLine();
            if (ImGui::Button("Add Sprite")) {
                m_sprites.push_back({
                    .sprite = AnimatedSprite({}),
                    .name = m_spriteName
                });
                m_sheet->sprites->addSprite(m_spriteName, {});
                m_spriteName = "";
            }

            for (const auto& s : m_sprites) {
                if (m_spriteName == s.name) {
                    ImGui::TextColored(ImVec4(255, 0, 0, 255), std::format("Sprite '{}' already exists", s.name).c_str());
                }
            }

            for (int i = 0; i < m_sprites.size(); i++) {
                if (ImGui::Selectable(m_sprites[i].name.c_str(), m_activeSprite == i)) {
                    m_activeSprite = i;
                }
            };

            ImGui::SeparatorText("Selected Sprite");

            if (sprite.name != "identity") {
                if (ImGui::Button("Remove")) {
                    toRemove = m_activeSprite;
                }
                if (ImGui::InputText("Name", &sprite.name)) {
                    for (int i = 0; i < m_sprites.size(); i++) {
                        if (i != m_activeSprite && m_sprites[i].name == sprite.name) {
                            ImGui::TextColored(ImVec4(255, 0, 0, 255), std::format("Sprite '{}' already exists", sprite.name).c_str());
                        }
                    }
                }
            }
            ImGui::DragInt("FPS", &sprite.sprite.fps, 1, 1);
            ImGui::Checkbox("Looping", &sprite.sprite.looping);
            ImGui::Checkbox("Animated", &sprite.sprite.animated);

            auto group = sprite.sprite.group();

            Rect rect;

            if (group.size() > 0) {
                rect = m_sheet->atlas.getRect(group[sprite.currentIndex], m_sheet->texture()->image->size);
            }

            if (ImGui::Button("Play")) {
                sprite.playing = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                sprite.playing = false;
            }

            if (sprite.playing && group.size() > 0) {
                double frameRate = 1.0 / (double) sprite.sprite.fps;
                sprite.elapsedTime += dt;
                int frame = sprite.elapsedTime / frameRate;
                sprite.currentIndex = frame % group.size();
            }

            ImGui::Image(
                m_sheet->texture()->id,
                ImVec2(64, 64),
                ImVec2(rect.min().x(), rect.min().y()),
                ImVec2(rect.min().x() + rect.size[0], rect.min().y() + rect.size[1])
            );
        }

        ImGui::EndChild();

        if (toRemove.has_value()) {
            m_sprites.erase(m_sprites.begin() + toRemove.value());
            m_activeSprite = 0;
        }
    }

    auto label = m_sheet ? std::format("Loaded: {}", m_path) : "Drag a Texture or SpriteSheet";
    dragTarget(label, dragTargetHeight, {
        {"png", [&](std::string const& path) {
            auto texturePath = utils::s_replace(path, ".png", "");
            auto ssPath = texturePath + ".hgss";
            m_path = texturePath;
            m_sheet = std::make_shared<SpriteSheetV2>(texturePath);
            m_selectedCell = std::nullopt;
            loadSprites();
        }},
        {"hgss", [&](std::string const& path) {
            auto ssPath = hg::utils::s_replace(path, ".hgss", "");
            m_sheet = std::shared_ptr<SpriteSheetV2>(getSpriteSheet(ssPath), [&](SpriteSheetV2* sheet) {});
            m_path = ssPath;
            m_selectedCell = std::nullopt;
            loadSprites();
        }}
    });

    m_browser.render();
}

void SpriteSheetEditor::saveSprites() {
    m_sheet->sprites->clear();
    for (auto& sprite : m_sprites) {
        m_sheet->sprites->addSprite(sprite.name, sprite.sprite.group());
        auto s = m_sheet->sprites->getSprite(sprite.name);
        s->looping = sprite.sprite.looping;
        s->animated = sprite.sprite.animated;
        s->fps = sprite.sprite.fps;
    }
}

void SpriteSheetEditor::loadSprites() {
    m_sprites.clear();
    m_sheet->sprites->forEach([&](std::string const& key, AnimatedSprite* sprite) {
        Sprite s{
            AnimatedSprite(sprite->group()),
            key,
        };
        s.sprite.fps = sprite->fps;
        s.sprite.looping = sprite->looping;
        s.sprite.animated = sprite->animated;
        m_sprites.push_back(s);
    });
    m_activeSprite = 0;
}

namespace {
    using Polygon = std::vector<Vec2>;
    using Shapes = std::vector<Polygon>;

    // In the cell's normalized space, y up
    const Polygon FULL_TILE = {Vec2(0, 0), Vec2(1, 0), Vec2(1, 1), Vec2(0, 1)};
    const Polygon BOTTOM_HALF = {Vec2(0, 0), Vec2(1, 0), Vec2(1, 0.5f), Vec2(0, 0.5f)};
    const Polygon TOP_HALF = {Vec2(0, 0.5f), Vec2(1, 0.5f), Vec2(1, 1), Vec2(0, 1)};
    const Polygon LEFT_HALF = {Vec2(0, 0), Vec2(0.5f, 0), Vec2(0.5f, 1), Vec2(0, 1)};
    const Polygon RIGHT_HALF = {Vec2(0.5f, 0), Vec2(1, 0), Vec2(1, 1), Vec2(0.5f, 1)};

    struct ColliderPreset {
        const char* name;
        Shapes shapes;
    };

    const std::vector<ColliderPreset> COLLIDER_PRESETS = {
        {"Floor slope /", {{Vec2(0, 0), Vec2(1, 0), Vec2(1, 1)}}},
        {"Floor slope \\", {{Vec2(0, 0), Vec2(1, 0), Vec2(0, 1)}}},
        {"Ceiling slope /", {{Vec2(0, 0), Vec2(1, 1), Vec2(0, 1)}}},
        {"Ceiling slope \\", {{Vec2(1, 0), Vec2(1, 1), Vec2(0, 1)}}},
        {"Bottom half", {BOTTOM_HALF}},
        {"Top half", {TOP_HALF}},
        {"Left half", {LEFT_HALF}},
        {"Right half", {RIGHT_HALF}},
        // Two overlapping halves make an L, e.g. a floor running into a wall
        {"Floor + left wall", {BOTTOM_HALF, LEFT_HALF}},
        {"Floor + right wall", {BOTTOM_HALF, RIGHT_HALF}},
        {"Ceiling + left wall", {TOP_HALF, LEFT_HALF}},
        {"Ceiling + right wall", {TOP_HALF, RIGHT_HALF}},
    };

    // Box2D's B2_MAX_POLYGON_VERTICES
    const int MAX_COLLIDER_POINTS = 8;

    bool isConvex(Polygon const& points) {
        if (points.size() < 3) {
            return false;
        }
        int sign = 0;
        for (int i = 0; i < points.size(); i++) {
            auto ab = points[(i + 1) % points.size()] - points[i];
            auto bc = points[(i + 2) % points.size()] - points[(i + 1) % points.size()];
            float cross = ab[0] * bc[1] - ab[1] * bc[0];
            if (std::abs(cross) < 1e-6f) {
                continue;
            }
            int turn = cross > 0 ? 1 : -1;
            if (sign != 0 && turn != sign) {
                return false;
            }
            sign = turn;
        }
        return sign != 0;
    }

    // Counter clockwise around the centroid, which keeps a convex shape's outline from crossing itself
    void sortAroundCentroid(Polygon& points) {
        if (points.size() < 3) {
            return;
        }
        Vec2 centroid = Vec2::Zero();
        for (const auto& point : points) {
            centroid += point;
        }
        centroid = centroid / (float) points.size();
        std::sort(points.begin(), points.end(), [&](Vec2 const& a, Vec2 const& b) {
            return std::atan2(a[1] - centroid[1], a[0] - centroid[0]) < std::atan2(b[1] - centroid[1], b[0] - centroid[0]);
        });
    }

    void drawShape(ImDrawList* drawList, std::vector<ImVec2> const& points, bool convex, ImU32 fill, ImU32 outline, float thickness) {
        if (convex) {
            drawList->AddConvexPolyFilled(points.data(), points.size(), fill);
        }
        if (points.size() >= 2) {
            drawList->AddPolyline(points.data(), points.size(), outline, ImDrawFlags_Closed, thickness);
        }
    }
}

void SpriteSheetEditor::drawCellColliders(ImVec2 imagePos, float cellWidth, float cellHeight) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto cells = m_sheet->atlas.size;

    for (int y = 0; y < cells[1]; y++) {
        for (int x = 0; x < cells[0]; x++) {
            Vec2i cell(x, y);
            ImVec2 min(imagePos.x + x * cellWidth, imagePos.y + y * cellHeight);
            ImVec2 max(min.x + cellWidth, min.y + cellHeight);

            auto colliders = m_sheet->getCellColliders(cell);
            if (colliders.has_value() && colliders->empty()) {
                // Doesn't collide
                drawList->AddLine(min, max, IM_COL32(255, 80, 80, 160));
                drawList->AddLine(ImVec2(max.x, min.y), ImVec2(min.x, max.y), IM_COL32(255, 80, 80, 160));
            } else if (colliders.has_value()) {
                for (const auto& shape : colliders.value()) {
                    std::vector<ImVec2> points;
                    for (const auto& point : shape) {
                        points.emplace_back(min.x + point[0] * cellWidth, min.y + (1 - point[1]) * cellHeight);
                    }
                    drawShape(drawList, points, isConvex(shape), IM_COL32(80, 255, 120, 60), IM_COL32(80, 255, 120, 220), 1.5f);
                }
            }

            if (m_selectedCell.has_value() && m_selectedCell.value() == cell) {
                drawList->AddRect(min, max, IM_COL32(255, 200, 50, 255), 0, 0, 2.0f);
            }
        }
    }
}

void SpriteSheetEditor::renderColliderEditor() {
    ImGui::SeparatorText("Tile Collider");
    ImGui::TextWrapped("Tiles without a collider here collide as full squares.");

    if (!m_selectedCell.has_value()) {
        ImGui::TextDisabled("Click a tile to edit its collider");
        return;
    }

    auto cell = m_selectedCell.value();

    auto colliders = m_sheet->getCellColliders(cell);
    bool full = !colliders.has_value();
    Shapes shapes = full ? Shapes{FULL_TILE} : colliders.value();
    bool changed = false;

    if (ImGui::Button("Full")) {
        m_sheet->clearCellColliders(cell);
        shapes = {FULL_TILE};
        full = true;
        m_selectedShape = 0;
        m_dragPoint = std::nullopt;
    }
    ImGui::SameLine();
    if (ImGui::Button("None")) {
        shapes = {};
        changed = true;
        m_dragPoint = std::nullopt;
    }

    ImGui::TextDisabled("A preset replaces the selected shape. Use Add Shape first to combine several");

    float presetWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2;
    for (int i = 0; i < COLLIDER_PRESETS.size(); i++) {
        if (i % 2 == 1) {
            ImGui::SameLine();
        }
        if (ImGui::Button(COLLIDER_PRESETS[i].name, ImVec2(presetWidth, 0))) {
            auto const& preset = COLLIDER_PRESETS[i].shapes;
            // Swap the preset in for the selected shape, keeping the others. A two shape preset (the Ls) takes its
            // place as both
            int at = std::min(m_selectedShape, (int) shapes.size());
            if (at < shapes.size()) {
                shapes.erase(shapes.begin() + at);
            }
            shapes.insert(shapes.begin() + at, preset.begin(), preset.end());
            m_selectedShape = at + preset.size() - 1;
            m_dragPoint = std::nullopt;
            changed = true;
        }
    }

    ImGui::SeparatorText("Shapes");

    if (shapes.empty()) {
        ImGui::TextDisabled("No collision");
    }

    for (int i = 0; i < shapes.size(); i++) {
        auto label = std::format("Shape {} ({} points){}", i + 1, shapes[i].size(), full ? ", full tile" : "");
        if (ImGui::Selectable(label.c_str(), m_selectedShape == i)) {
            m_selectedShape = i;
            m_dragPoint = std::nullopt;
        }
    }

    if (ImGui::Button("Add Shape")) {
        shapes.push_back({});
        m_selectedShape = shapes.size() - 1;
        m_dragPoint = std::nullopt;
        changed = true;
    }
    if (m_selectedShape >= 0 && m_selectedShape < shapes.size()) {
        ImGui::SameLine();
        if (ImGui::Button("Delete Shape")) {
            shapes.erase(shapes.begin() + m_selectedShape);
            m_dragPoint = std::nullopt;
            changed = true;
        }
    }

    m_selectedShape = std::clamp(m_selectedShape, 0, std::max(0, (int) shapes.size() - 1));
    bool hasSelection = m_selectedShape < shapes.size();

    ImGui::Checkbox("Snap to pixels", &m_snapToPixels);
    ImGui::TextDisabled("Click to add a point to the selected shape, drag to move one, right click to delete. Pick which shape to edit from the list.");

    auto texture = m_sheet->texture();
    auto imageSize = texture->image->size;
    auto rect = m_sheet->atlas.getRect(cell, imageSize);
    auto cellPixels = m_sheet->atlas.getCellSize(imageSize);

    // Points often sit on the tile's edges, so the canvas reaches past the image to keep their handles clickable.
    // A click that misses the canvas lands on the window's background, which moves a floating window
    const float margin = 10.0f;
    float width = std::min(ImGui::GetContentRegionAvail().x - margin * 2, 256.0f);
    float height = width * cellPixels[1] / cellPixels[0];
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 origin(canvasPos.x + margin, canvasPos.y + margin);
    ImGui::InvisibleButton("##colliderCanvas", ImVec2(width + margin * 2, height + margin * 2),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddImage(texture->id, origin, ImVec2(origin.x + width, origin.y + height),
                       ImVec2(rect.pos[0], rect.pos[1]),
                       ImVec2(rect.pos[0] + rect.size[0], rect.pos[1] + rect.size[1]));

    auto toScreen = [&](Vec2 point) {
        return ImVec2(origin.x + point[0] * width, origin.y + (1 - point[1]) * height);
    };

    auto mouse = ImGui::GetMousePos();
    Vec2 mousePoint((mouse.x - origin.x) / width, 1 - (mouse.y - origin.y) / height);
    for (int i = 0; i < 2; i++) {
        mousePoint[i] = std::clamp(mousePoint[i], 0.0f, 1.0f);
        if (m_snapToPixels) {
            mousePoint[i] = std::round(mousePoint[i] * cellPixels[i]) / cellPixels[i];
        }
    }

    auto pointUnderMouse = [&](int shape) -> std::optional<int> {
        for (int i = 0; i < shapes[shape].size(); i++) {
            auto screen = toScreen(shapes[shape][i]);
            float dx = screen.x - mouse.x;
            float dy = screen.y - mouse.y;
            if (dx * dx + dy * dy < 8 * 8) {
                return i;
            }
        }
        return std::nullopt;
    };

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Only the selected shape is edited. Shapes overlap and share corners (e.g. an L), so a click near or inside
        // another shape is still meant for this one; others are picked from the list
        std::optional<int> handle = hasSelection ? pointUnderMouse(m_selectedShape) : std::nullopt;

        if (handle.has_value()) {
            m_dragPoint = handle;
        } else {
            if (!hasSelection) {
                shapes.push_back({});
                m_selectedShape = shapes.size() - 1;
            }
            auto& points = shapes[m_selectedShape];
            if (points.size() < MAX_COLLIDER_POINTS) {
                points.push_back(mousePoint);
                sortAroundCentroid(points);
                changed = true;
            }
        }
        hasSelection = m_selectedShape < shapes.size();
    }

    if (m_dragPoint.has_value() && hasSelection && m_dragPoint.value() < shapes[m_selectedShape].size()) {
        auto& points = shapes[m_selectedShape];
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (points[m_dragPoint.value()] != mousePoint) {
                points[m_dragPoint.value()] = mousePoint;
                changed = true;
            }
        } else {
            // Sort once the drag ends, so the dragged point keeps its index while it moves
            m_dragPoint = std::nullopt;
            sortAroundCentroid(points);
            changed = true;
        }
    }

    if (hovered && hasSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (auto point = pointUnderMouse(m_selectedShape)) {
            auto& points = shapes[m_selectedShape];
            points.erase(points.begin() + point.value());
            // A shape with its last point gone goes with it
            if (points.empty()) {
                shapes.erase(shapes.begin() + m_selectedShape);
                m_selectedShape = std::max(0, m_selectedShape - 1);
            }
            m_dragPoint = std::nullopt;
            changed = true;
        }
    }

    if (changed) {
        m_sheet->setCellColliders(cell, shapes);
        full = false;
    }

    if (m_snapToPixels) {
        for (int x = 1; x < cellPixels[0]; x++) {
            float sx = origin.x + width * x / cellPixels[0];
            drawList->AddLine(ImVec2(sx, origin.y), ImVec2(sx, origin.y + height), IM_COL32(255, 255, 255, 20));
        }
        for (int y = 1; y < cellPixels[1]; y++) {
            float sy = origin.y + height * y / cellPixels[1];
            drawList->AddLine(ImVec2(origin.x, sy), ImVec2(origin.x + width, sy), IM_COL32(255, 255, 255, 20));
        }
    }

    // The other shapes underneath, then the selected one with its handles on top
    for (int i = 0; i < shapes.size(); i++) {
        if (i == m_selectedShape) {
            continue;
        }
        std::vector<ImVec2> screenPoints;
        for (const auto& point : shapes[i]) {
            screenPoints.push_back(toScreen(point));
        }
        drawShape(drawList, screenPoints, isConvex(shapes[i]), IM_COL32(80, 180, 255, 45), IM_COL32(80, 180, 255, 200), 1.5f);
    }

    if (m_selectedShape < shapes.size()) {
        auto const& points = shapes[m_selectedShape];
        std::vector<ImVec2> screenPoints;
        for (const auto& point : points) {
            screenPoints.push_back(toScreen(point));
        }
        drawShape(drawList, screenPoints, isConvex(points), IM_COL32(80, 255, 120, full ? 30 : 70), IM_COL32(80, 255, 120, 230), 2.0f);
        for (int i = 0; i < screenPoints.size(); i++) {
            bool dragging = m_dragPoint.has_value() && m_dragPoint.value() == i;
            drawList->AddCircleFilled(screenPoints[i], 4, dragging ? IM_COL32(255, 200, 50, 255) : IM_COL32(255, 255, 255, 230));
        }

        if (!points.empty() && points.size() < 3) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Shape %d needs at least 3 points", m_selectedShape + 1);
        } else if (points.size() >= 3 && !isConvex(points)) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Shape %d isn't convex: physics will use its convex hull. Split it into more shapes", m_selectedShape + 1);
        }
        if (points.size() >= MAX_COLLIDER_POINTS) {
            ImGui::TextDisabled("At most %d points per shape (Box2D's limit)", MAX_COLLIDER_POINTS);
        }
    }

    ImGui::TextDisabled("Save the sheet to keep colliders. Changes apply to the next Play.");
}
