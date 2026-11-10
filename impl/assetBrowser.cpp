//
// Created by henry on 12/16/23.
//
#include <hge/components/assetBrowser.h>
#include <algorithm>
#include <cctype>
#include <hge/core/events.h>
#include <hagame/core/assets.h>
#include "imgui.h"
#include "hge/core/extensions.h"

using namespace hge;

AssetBrowser::AssetBrowser() {
    m_assetMenus.insert(std::make_pair("hg_pfb", std::vector<Command>({
        { "Add to Scene", [&](const hg::utils::FileParts& parts) {
            Events()->emit(EventTypes::AddPrefab, Event{PrefabEvent{parts.absolutePath(), nullptr}});
        }},
    })));
}

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

    // By name, ignoring case, rather than the order the file system happens to list them in
    auto byName = [](std::vector<std::string> paths) {
        auto lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        };
        std::sort(paths.begin(), paths.end(), [&](std::string const& a, std::string const& b) {
            return lower(a) < lower(b);
        });
        return paths;
    };

    for (const auto& file : byName(hg::utils::d_listFiles(path))) {
        ImGui::BeginChild(file.c_str(), size, true);
        auto fileParts = hg::utils::f_getParts(file);
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            auto events = Events();
            events->emit(EventTypes::SelectAsset, Event{fileParts});
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            auto assetPath = hg::utils::s_replace(file, hg::ASSET_DIR, "");
            ImGui::SetDragDropPayload(fileParts.extension.c_str(), (void*) assetPath.c_str(), assetPath.size() + 1);
            ImGui::Image((ImTextureID)texture(fileParts)->id, ImVec2(64, 64));
            ImGui::Text("%s", fileParts.fullName.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 64) * 0.5, 5));
        ImGui::Image((ImTextureID)texture(fileParts)->id, ImVec2(64, 64));
        ImGui::Text("%s", fileParts.fullName.c_str());

        contextMenu(fileParts);

        ImGui::EndChild();

        x += size.x + padding;
        if (x < ImGui::GetWindowSize().x - size.x) {
            ImGui::SameLine();
        } else {
            x = 0;
        }
    }

    for (const auto& dir : byName(hg::utils::d_listDirs(path))) {
        auto parts = hg::utils::f_getParts(dir);
        ImGui::BeginChild(dir.c_str(), size, true);
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - 64) * 0.5, 0));
        ImGui::Image(hg::getTexture("textures/ui/directory")->id, ImVec2(64, 64));
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
        return hg::getTexture("textures/ui/audio");
    } else if (assetType == "png") {
        auto textureName = hg::utils::s_replace(fileParts.path, hg::ASSET_DIR, "") + fileParts.name;
        return hg::getTexture(textureName);
    } else if (assetType == "ttf") {
        return hg::getTexture("textures/ui/font");
    } else if (assetType == "cpp") {
        return hg::getTexture("textures/ui/script");
    } else if (assetType == "frag") {
        return hg::getTexture("textures/ui/fragment_shader");
    } else if (assetType == "vert") {
        return hg::getTexture("textures/ui/vertex_shader");
    } else if (assetType == "hg") {
        return hg::getTexture("textures/ui/hg");
    } else if (assetType == "hg_pfb") {
        return hg::getTexture("textures/ui/prefab");
    }

    return hg::getTexture("textures/ui/unknown");
}

void AssetBrowser::contextMenu(hg::utils::FileParts fileParts) {
    if (m_assetMenus.find(fileParts.extension) == m_assetMenus.end()) {
        return;
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup(fileParts.name.c_str());
    }

    if (ImGui::BeginPopup(fileParts.name.c_str())) {

        for (const auto& cmd : m_assetMenus.at(fileParts.extension)) {
            if (ImGui::Button(cmd.label.c_str())) {
                cmd.fn(fileParts);
            }
        }

        ImGui::EndPopup();
    }
}
