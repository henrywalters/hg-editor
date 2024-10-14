//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_COMPONENTEXPLORER_H
#define HGEDITOR_COMPONENTEXPLORER_H

#include <hagame/core/component.h>
#include <optional>

namespace hge {

    const std::string COMPONENT_EXPLORER = "component_explorer";

    std::optional<std::string> componentExplorer();

    bool editComponentField(hg::Component *component, hg::ComponentFactory::ComponentField field);

}

#endif //HGEDITOR_COMPONENTEXPLORER_H
