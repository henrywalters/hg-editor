//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_COMPONENTEXPLORER_H
#define HGEDITOR_COMPONENTEXPLORER_H

#include <hagame/core/component.h>
#include <functional>
#include <optional>
#include <vector>

namespace hge {

    const std::string COMPONENT_EXPLORER = "component_explorer";

    std::optional<std::string> componentExplorer();

    bool editComponentField(hg::Component *component, hg::ComponentFactory::ComponentField field);

    using StringOptions = std::function<std::vector<std::string>()>;

    // Shows a std::string field of a component as a dropdown of options instead of a text box. options is called each
    // time the field is drawn, so it can list things that change, e.g. whatever the game has registered
    // onChosen, if given, runs after a pick is stored in the field, so choosing one option can fill in other fields
    // too, e.g. a door's spawn setting where it leads
    using StringChosen = std::function<void(hg::Component*, std::string const&)>;
    void setStringFieldOptions(std::string const& component, std::string const& field, StringOptions options,
                               StringChosen onChosen = nullptr);

}

#endif //HGEDITOR_COMPONENTEXPLORER_H
