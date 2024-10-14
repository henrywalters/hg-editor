//
// Created by henry on 10/6/24.
//

#ifndef HGEDITOR_EDITOR_H
#define HGEDITOR_EDITOR_H

#include "hge.h"

namespace hge {
    class Editor {
    public:

        Editor(hg::Scene* scene, bool _connectEvents = true);

        void render();

        void setOutput(void* m_textureId, hg::Vec2i size);

        void connectEvents();

    private:

        bool m_hasTexture = false;
        hg::Vec2i m_textureSize;
        void* m_textureId;

        hg::Scene* m_scene;
        AssetBrowser m_assets;
        EntityTree m_entities;

    };
}

#endif //HGEDITOR_EDITOR_H
