//
// Created by henry on 10/20/24.
//

#ifndef HGEDITOR_FILEBROWSER_H
#define HGEDITOR_FILEBROWSER_H

#include <optional>
#include <filesystem>

#include <hagame/utils/pubsub.h>
#include <hagame/utils/config.h>
#include <hagame/utils/file.h>
#include <hagame/utils/watcher.h>

#include "imgui.h"
// #include "imfilebrowser.h"

namespace ImGui {
    class FileBrowser;
}

namespace hge {

    class FileBrowser {
    public:

        const std::string INI = "filebrowser.ini";

        FileBrowser();

        void loadFile(
            std::string title,
            std::function<void(std::filesystem::path)> onChoose,
            std::vector<std::string> filters = {}
        );

        void loadFiles(
            std::string title,
            std::function<void(std::vector<std::filesystem::path>)> onChoose,
            std::vector<std::string> filters = {}
        );

        void saveFile(std::string title, std::function<void(std::filesystem::path)> onChoose, std::vector<std::string> filters = {});

        void render();

    private:

        struct Browser {
            bool open = true;
            std::shared_ptr<ImGui::FileBrowser> browser;
            bool choosingMany = false;
            std::function<void(std::filesystem::path)> onChooseOne;
            std::function<void(std::vector<std::filesystem::path>)> onChooseMany;
        };

        Browser* newBrowser(std::string title, std::vector<std::string> filters, int flags = 0);

        hg::utils::Config m_config;

        std::unordered_map<std::string, Browser> m_files;

        std::optional<std::string> m_openFile = std::nullopt;

    };
}

#endif //HGEDITOR_FILEBROWSER_H
