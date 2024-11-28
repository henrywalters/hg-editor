#include <hge/components/fileBrowser.h>
#include "../thirdparty/imgui-filebrowser/imfilebrowser.h"

using namespace hge;

void FileBrowser::loadFile(std::string title, std::function<void(std::filesystem::path)> onChoose, std::vector<std::string> filters) {
    auto browser = newBrowser(title, filters);
    browser->onChooseOne = onChoose;
    browser->browser->Open();
}

void
FileBrowser::loadFiles(std::string title, std::function<void(std::vector<std::filesystem::path>)> onChoose, std::vector<std::string> filters) {
    auto browser = newBrowser(title, filters, ImGuiFileBrowserFlags_MultipleSelection);
    browser->onChooseMany = onChoose;
    browser->choosingMany = true;
    browser->browser->Open();
}

void FileBrowser::saveFile(std::string title, std::function<void(std::filesystem::path)> onChoose, std::vector<std::string> filters) {
    auto browser = newBrowser(title, filters, ImGuiFileBrowserFlags_EnterNewFilename);
    browser->onChooseOne = onChoose;
    browser->browser->Open();
}

void FileBrowser::render() {

    for (auto& [key, browser] : m_files) {
        if (!browser.open) {
            continue;
        }

        browser.browser->Display();
        if (browser.browser->HasSelected()) {
            if (browser.choosingMany) {
                m_config.setRaw("meta", key, hg::utils::f_getParts(browser.browser->GetMultiSelected()[0]).path);
                hg::utils::f_write(INI, m_config.toString());
                browser.onChooseMany(browser.browser->GetMultiSelected());
            } else {
                m_config.setRaw("meta", key, hg::utils::f_getParts(browser.browser->GetSelected()).path);
                hg::utils::f_write(INI, m_config.toString());
                browser.onChooseOne(browser.browser->GetSelected());
            }
            browser.open = false;
        } else if (!browser.browser->IsOpened()) {
            browser.open= false;
        }
    }
}

FileBrowser::FileBrowser() {
    if (hg::utils::f_exists(INI)) {
        m_config = hg::utils::Config::Parse(hg::utils::f_readLines(INI));
        if (!m_config.hasSection("meta")) {
            m_config.addSection("meta");
        }
    } else {
        m_config.addSection("meta");
        hg::utils::f_write(INI, m_config.toString());
    }
}

FileBrowser::Browser* FileBrowser::newBrowser(std::string title, std::vector<std::string> filters, int flags) {

    std::string pwd;

    if (m_files.find(title) == m_files.end()) {
        Browser browser;
        browser.browser = std::make_shared<ImGui::FileBrowser>(flags);
        if (m_config.has("meta", title)) {
            pwd = m_config.getRaw("meta", title);
        } else {
            pwd = "";
            m_config.setRaw("meta", title, "");
            hg::utils::f_write(INI, m_config.toString());
        }
        m_files.insert(std::make_pair(title, std::move(browser)));
    }

    m_files.at(title).open = true;
    m_files.at(title).browser->SetTitle(title);
    m_files.at(title).browser->SetTypeFilters(filters);
    m_files.at(title).browser->SetPwd(pwd);

    return &m_files.at(title);
}
