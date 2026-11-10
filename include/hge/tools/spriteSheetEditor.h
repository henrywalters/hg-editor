//
// Created by henry on 8/24/26.
//

#ifndef HGEDITOR_SPRITESHEETTOOL_H
#define HGEDITOR_SPRITESHEETTOOL_H

#include <optional>

#include "../components/fileBrowser.h"
#include <hagame/graphics/spriteSheetV2.h>

#include "hge/core/tool.h"

namespace hge {
    class SpriteSheetEditor : public hge::Tool {
    public:

        SpriteSheetEditor(hg::Scene* scene);

        std::string getName() override { return "SpriteSheetEditor"; }
        std::string getButtonLabel() override { return "SpriteSheet Editor"; }

    protected:

        void renderUI(double dt) override;

        void onOpen() override;

    private:

        struct Sprite {
            hg::graphics::AnimatedSprite sprite;
            std::string name;
            bool playing = true;
            double elapsedTime = 0;
            int currentIndex = 0;
        };

        FileBrowser m_browser;
        int m_activeSprite = 0;
        std::vector<Sprite> m_sprites;
        std::string m_path;
        std::string m_spriteName;
        std::shared_ptr<hg::graphics::SpriteSheetV2> m_sheet = nullptr;

        enum class Mode {
            Sprites,
            TileColliders,
        };

        Mode m_mode = Mode::Sprites;

        std::optional<hg::Vec2i> m_selectedCell;
        // Which of the selected cell's collider shapes is being edited, and the point of it being dragged
        int m_selectedShape = 0;
        std::optional<int> m_dragPoint;
        bool m_snapToPixels = true;

        void saveSprites();
        void loadSprites();

        // Outlines each cell's custom collider over the sheet image
        void drawCellColliders(ImVec2 imagePos, float cellWidth, float cellHeight);
        void renderColliderEditor();

    };
}

#endif //HGEDITOR_SPRITESHEETTOOL_H
