//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_ENTITYVIEWER_H
#define HGEDITOR_ENTITYVIEWER_H

#include <hagame/core/entity.h>

namespace hge {

    void transformViewer(hg::math::Transform& transform);
    void entityViewer(hg::Entity *entity);

}

#endif //HGEDITOR_ENTITYVIEWER_H
