//
// Created by henry on 10/6/24.
//

#ifndef HGEDITOR_EDITOR_H
#define HGEDITOR_EDITOR_H

#include "hge.h"
#include <hagame/core/game.h>
#include <hagame/utils/aliases.h>
#include <hge/components/fileBrowser.h>

namespace hge {

    const std::function<void()> _defaultFunc = [](){
        throw std::runtime_error("Unimplemented func");
    };

    class Editor {
    public:

        hg::VoidFunc onNewScene = _defaultFunc;
        hg::VoidFunc onPlay = _defaultFunc;
        hg::VoidFunc onPause = _defaultFunc;
        hg::VoidFunc onReset = _defaultFunc;

        explicit Editor(hg::Scene* scene, bool _connectEvents = true);

        void update(double dt);
        void render();

        void setOutput(void* m_textureId, hg::Vec2i size);

        void connectEvents();

        void saveAs();
        void saveToDisc();
        void loadFromDisc();

    private:

        std::string m_saveFile;

        bool m_hasTexture = false;
        hg::Vec2i m_textureSize;
        void* m_textureId;

        hg::Scene* m_scene;
        FileBrowser m_browser;
        AssetBrowser m_assets;
        EntityTree m_entities;
    };
}

#endif //HGEDITOR_EDITOR_H
