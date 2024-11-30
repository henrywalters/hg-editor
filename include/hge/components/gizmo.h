//
// Created by henry on 11/28/24.
//

#ifndef HGEDITOR_GIZMO_H
#define HGEDITOR_GIZMO_H

#include <hagame/math/aliases.h>
#include <hagame/utils/enum.h>
#include <hagame/core/entity.h>

ENUM(GizmoMode)
ENUM_VALUE(GizmoMode, Translate)
ENUM_VALUE(GizmoMode, Scale)
ENUM_VALUE(GizmoMode, Rotate)

namespace hge {

    class Gizmo {
    public:

        Gizmo();

        void setMode(GizmoMode::type GizmoMode);

        void renderModeSelector();

        void render(bool orthographic, hg::Recti window, hg::Mat4 proj, hg::Mat4 view);

    private:

        hg::Entity *m_selectedEntity = nullptr;
        GizmoMode::type m_mode = GizmoMode::Translate;

    };
}
#endif //HGEDITOR_GIZMO_H
