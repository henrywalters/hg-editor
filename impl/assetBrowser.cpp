//
// Created by henry on 12/16/23.
//
#include <hge/components/assetBrowser.h>
#include <hge/core/events.h>
#include <hagame/core/assets.h>
#include "imgui.h"

using namespace hge;

void AssetBrowser::render() {
    std::string path = hg::ASSET_DIR;

    for (int i = 0; i < m_path.size(); i++) {
        path += m_path[i] + "/";
    }

    const ImVec2 size = ImVec2(96, 96);
    const float padding = 8;

    float x = 0;

    if (m_path.size() == 0) {
        ImGui::Text("Assets");
    } else {
        if (ImGui::Button("Assets")) {
            m_path.clear();
        }
        ImGui::SameLine();
        ImGui::Text("/");
        ImGui::SameLine();
    }

    for (int i = 0; i < m_path.size(); i++) {
        if (ImGui::Button(m_path[i].c_str())) {
            m_path.resize(i + 1);
        }
        if (i < m_path.size() - 1) {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
        }
    }

    for (const auto& file : hg::utils::d_listFiles(path)) {
        ImGui::BeginChild(file.c_str(), size, true);
        auto fileParts = hg::utils::f_getParts(file);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            auto events = Events();
            events->emit(EventTypes::SelectAsset, Event{fileParts});
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(fileParts.extension.c_str(), (void*) file.data(), sizeof(char) * file.size());
            ImGui::Image((void*)(size_t)texture(fileParts)->id, ImVec2(64, 64));
            ImGui::Text("%s", fileParts.fullName.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 64) * 0.5, 5));
        ImGui::Image((void*)(size_t)texture(fileParts)->id, ImVec2(64, 64));
        ImGui::Text("%s", fileParts.fullName.c_str());
        ImGui::EndChild();

        x += size.x + padding;
        if (x < ImGui::GetWindowSize().x - size.x) {
            ImGui::SameLine();
        } else {
            x = 0;
        }
    }

    for (const auto& dir : hg::utils::d_listDirs(path)) {
        auto parts = hg::utils::f_getParts(dir);
        ImGui::BeginChild(dir.c_str(), size, true);
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 64) * 0.5, 0));
        ImGui::Image((void*)(size_t)hg::getTexture("ui/directory")->id, ImVec2(64, 64));
        if (ImGui::IsItemClicked()) {
            m_path.push_back(parts.name);
        }
        ImGui::Text("%s", parts.name.c_str());
        ImGui::EndChild();
        x += size.x + padding;
        if (x < ImGui::GetWindowSize().x - size.x) {
            ImGui::SameLine();
        } else {
            x = 0;
        }
    }
}

hg::graphics::Texture* AssetBrowser::texture(hg::utils::FileParts fileParts) {

    std::string assetType = fileParts.extension;

    if (assetType == "wav") {
        return hg::getTexture("ui/audio");
    } else if (assetType == "png") {
        auto textureName = hg::utils::s_replace(fileParts.path, hg::ASSET_DIR + "textures/", "") + fileParts.name;
        return hg::getTexture(textureName);
    } else if (assetType == "ttf") {
        return hg::getTexture("ui/font");
    } else if (assetType == "cpp") {
        return hg::getTexture("ui/script");
    } else if (assetType == "frag") {
        return hg::getTexture("ui/fragment_shader");
    } else if (assetType == "vert") {
        return hg::getTexture("ui/vertex_shader");
    } else if (assetType == "hg") {
        return hg::getTexture("ui/hg");
    }

    return hg::getTexture("ui/unknown");
}
