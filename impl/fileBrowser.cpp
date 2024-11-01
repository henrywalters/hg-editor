#include <hge/components/fileBrowser.h>
#include "../thirdparty/imgui-filebrowser/imfilebrowser.h"

using namespace hge;

void FileBrowser::loadFile(std::function<void(std::filesystem::path)> onChoose, std::vector<std::string> filters) {
    if (m_browsingFiles) {
        return;
    }

    m_files = std::make_shared<ImGui::FileBrowser>();
    m_files->SetTitle("Load file");
    m_files->SetTypeFilters(filters);

    if (m_config.has("meta", "pwd")) {
        m_files->SetPwd(m_config.getRaw("meta", "pwd"));
    }

    m_browsingFiles = true;
    m_onChooseOne = onChoose;
    m_choosingMany = false;

    m_files->Open();
}

void
FileBrowser::loadFiles(std::function<void(std::vector<std::filesystem::path>)> onChoose, std::vector<std::string> filters) {
    if (m_browsingFiles) {
        return;
    }

    m_files = std::make_shared<ImGui::FileBrowser>(ImGuiFileBrowserFlags_MultipleSelection);
    m_files->SetTitle("Load file");
    m_files->SetTypeFilters(filters);

    if (m_config.has("meta", "pwd")) {
        m_files->SetPwd(m_config.getRaw("meta", "pwd"));
    }

    m_browsingFiles = true;
    m_onChooseMany = onChoose;
    m_choosingMany = true;

    m_files->Open();
}

void FileBrowser::saveFile(std::function<void(std::filesystem::path)> onChoose, std::vector<std::string> filters) {
    if (m_browsingFiles) {
        return;
    }

    m_files = std::make_shared<ImGui::FileBrowser>(ImGuiFileBrowserFlags_EnterNewFilename);
    m_files->SetTitle("Save file");
    m_files->SetTypeFilters(filters);

    if (m_config.has("meta", "pwd")) {
        m_files->SetPwd(m_config.getRaw("meta", "pwd"));
    }

    m_browsingFiles = true;
    m_onChooseOne = onChoose;
    m_choosingMany = false;

    m_files->Open();
}

void FileBrowser::setPwd(std::string pwd) {
    m_files->SetPwd(pwd);
}

void FileBrowser::render() {
    if (m_browsingFiles) {
        m_files->Display();
        if (m_files->HasSelected()) {
            if (m_choosingMany) {
                m_onChooseMany(m_files->GetMultiSelected());
            } else {
                m_onChooseOne(m_files->GetSelected());
            }

            m_browsingFiles = false;
            m_files.reset();
        } else if (!m_files->IsOpened()) {
            m_browsingFiles = false;
        }
    }
}