//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_ENTITYTREE_H
#define HGEDITOR_ENTITYTREE_H

#include <hagame/core/scene.h>

namespace hge {

    class EntityTree {
    public:

        void render(hg::Scene *scene, hg::Entity *root);

        hg::Entity* selected() { return m_selected; }
        void selected(hg::Entity* entity) {
            m_selected = entity;
        }

    private:

        void renderTree(hg::Scene *scene, hg::Entity *entity, bool root = false);

        bool m_dragging = false;
        hg::Entity *m_selected = nullptr;

    };
}

#endif //HGEDITOR_ENTITYTREE_H
