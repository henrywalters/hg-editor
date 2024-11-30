//
// Created by henry on 11/29/24.
//

#ifndef HGEDITOR_TEXTUREFIELD_H
#define HGEDITOR_TEXTUREFIELD_H

#include <imgui.h>
#include <hagame/graphics/texture.h>
#include <hagame/utils/file.h>
#include <hagame/core/assets.h>
#include <hagame/utils/string.h>

namespace hge {
    inline void textureField(std::string label, hg::graphics::TextureAsset& asset) {
        if (ImGui::BeginCombo(label.c_str(), asset.path().c_str())) {
            for (const auto& path : hg::utils::d_listFiles(hg::ASSET_DIR + "textures", true)) {
                auto trimmed = hg::utils::s_replace(hg::utils::s_replace(path, hg::ASSET_DIR + "textures/", ""), ".png", "");
                bool selected = trimmed == asset.path();
                if (ImGui::Selectable(trimmed.c_str(), selected)) {
                    asset.path(trimmed);
                    break;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Image(asset.texture()->id, ImVec2(64, 64));
    }
}

#endif //HGEDITOR_TEXTUREFIELD_H
