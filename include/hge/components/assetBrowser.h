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

        void render();

    private:

        hg::graphics::Texture* texture(hg::utils::FileParts fileParts);

        std::vector<std::string> m_path;

    };

}

#endif //HGEDITOR_ASSETBROWSER_H
