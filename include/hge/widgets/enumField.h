//
// Created by henry on 11/17/24.
//

#ifndef HGEDITOR_ENUMFIELD_H
#define HGEDITOR_ENUMFIELD_H

#include <imgui.h>
#include <hagame/utils/enum.h>

namespace hge {
    void enumField(std::string label, hg::utils::enum_t* val, std::vector<hg::utils::EnumElement>* enums) {
        if (ImGui::BeginCombo(label.c_str(), enums->at(*val).label.c_str())) {
            for (const auto& e : *enums) {
                bool selected = e.key == *val;
                if (ImGui::Selectable(e.label.c_str(), selected)) {
                    *val = e.key;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
}

#endif //HGEDITOR_ENUMFIELD_H
