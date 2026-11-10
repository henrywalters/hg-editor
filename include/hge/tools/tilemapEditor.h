//
// Created by henry on 7/24/26.
//

#ifndef HGEDITOR_TILEMAPEDITOR_H
#define HGEDITOR_TILEMAPEDITOR_H

#include <optional>

#include <hagame/common/components/tilemap.h>
#include <hagame/graphics/components/spriteSheetAnimator.h>
#include <hagame/graphics/spriteSheetV2.h>
#include <hagame/input/input.h>

#include "hge/core/tool.h"
#include "hagame/graphics/primitives/grid.h"

namespace hge {

    const hg::Vec2 DEFAULT_CELL_SIZE {50, 50};
    const hg::Vec2i DEFAULT_CELLS {100, 100};

    class TilemapEditor : public Tool {
    public:

        TilemapEditor(hg::Scene* scene);

        std::string getName() override { return "TilemapEditor"; }
        std::string getButtonLabel() override { return "Tilemap Editor"; }

        // Clicks paint tiles rather than select entities while a tilemap is being edited
        bool capturesViewportClicks() override { return tilemap() != nullptr; }

        // Whether tiles are shown by their normal maps (a texture's is the same path with _normal appended), so shapes
        // drawn only in the normals can be told apart. The game's renderer can read this to show normals in the scene too
        bool showNormals() const { return m_showNormals; }

    protected:

        void renderOverlay(hg::Mat4 projection, hg::Mat4 view) override;

        void renderUI(double dt) override;

        void onOpen() override;

        void onUpdate(hg::input::Input* input, double dt) override;

    private:

        bool m_showGrid = true;
        bool m_showNormals = false;

        hg::Vec2i m_cursorSize = hg::Vec2i(1, 1);
        hg::Vec2i m_lastTileIdx;
        hg::Vec2i m_tileIdx;

        hg::graphics::primitives::Quad m_cursor;
        hg::graphics::MeshInstance m_cursorMesh;

        hg::graphics::primitives::Grid m_grid;
        hg::graphics::MeshInstance m_gridMesh;
        hg::Vec2 m_gridTileSize;

        // The edited tilemap's entity. Looked up each frame, since reloading the scene (e.g. on reset) replaces it
        std::optional<hg::utils::uuid_t> m_tilemapEntity;

        int m_selectedTile = -1;

        // The sheet shown in the palette, by asset name, e.g. "textures/tiles"
        std::string m_sheet;
        std::string m_sheetError;
        // Whether tiles picked from the palette start out as colliders
        bool m_newTilesCollide = true;
        float m_paletteZoom = 2.0f;

        hg::components::Tilemap* tilemap();
        std::vector<hg::components::Tilemap*> findTilemaps();
        hg::Vec2 origin();
        void updateSize();
        void setTilemap(hg::components::Tilemap* tilemap);
        void setTileType(hg::components::TilemapLayer& layer, hg::utils::enum_t type);

        // Loads the sheet if it isn't yet, e.g. one made after the editor started. Null if it can't be
        hg::graphics::SpriteSheetV2* loadSheet(std::string const& name);
        // The sheet if it's loaded, otherwise null
        hg::graphics::SpriteSheetV2* getSheet(std::string const& name);

        // The layer drawing this cell of the sheet, or -1
        int findSheetLayer(std::string const& sheet, hg::Vec2i index);
        // Selects the layer for this cell of the sheet, adding one if there isn't one yet
        void selectSheetTile(std::string const& sheet, hg::Vec2i index);

        // The texture to show for a texture asset: its normal map while showing normals and it has one
        hg::graphics::Texture* displayTexture(std::string const& name);

        bool tileButton(const char* id, hg::components::Tile const& tile, float size);
        void renderPalette();
        void renderTileDetails(hg::components::Tile& tile);

        void cursor(std::function<void(hg::Vec2i)> fn);
    };
}

#endif //HGEDITOR_TILEMAPEDITOR_H
