//
// Created by henry on 8/24/26.
//

#ifndef HGEDITOR_DRAGTARGET_H
#define HGEDITOR_DRAGTARGET_H

#include <optional>
#include <string>
#include <hagame/utils/aliases.h>

#include "imgui.h"

namespace hge {
    struct ExtensionHandler {
        std::string extension;
        std::function<void(std::string)> handler;
    };

    void dragTarget(std::string label, ImTextureRef imageId, int height, std::vector<ExtensionHandler> handlers);
    void dragTarget(std::string label, int height, std::vector<ExtensionHandler> handlers);

};

#endif //HGEDITOR_DRAGTARGET_H
