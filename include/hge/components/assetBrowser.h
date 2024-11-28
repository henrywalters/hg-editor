//
// Created by henry on 10/3/24.
//

#ifndef HGEDITOR_ASSETBROWSER_H
#define HGEDITOR_ASSETBROWSER_H

#include <hagame/graphics/texture.h>
#include <hagame/utils/file.h>

namespace hge {
    class AssetBrowser {
    public:

        AssetBrowser();

        void render();

    private:

        struct Command {
            std::string label;
            std::function<void(hg::utils::FileParts)> fn;
        };

        std::unordered_map<std::string, std::vector<Command>> m_assetMenus;

        hg::graphics::Texture* texture(hg::utils::FileParts fileParts);

        void contextMenu(hg::utils::FileParts fileParts);

        std::vector<std::string> m_path;

    };

}

#endif //HGEDITOR_ASSETBROWSER_H
