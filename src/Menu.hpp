#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <string>

using namespace geode::prelude;

namespace nova {

enum class MenuTab {
    Home,
    HUD,
    Practice,
    Macro,
    Input,
    Tools,
    Profiles
};

class NovaMenuPopup : public geode::Popup<> {
protected:
    bool setup() override;

public:
    static NovaMenuPopup* create();

private:
    void rebuild();
    void buildSidebar();
    void buildContent();
    void addTitle(std::string const& title, std::string const& subtitle = "");
    void addToggle(std::string const& title, std::string const& key, bool defaultValue = false);
    void addAction(std::string const& title, std::string const& action, int color = 1);
    void addInfo(std::string const& text);
    void addTab(std::string const& title, MenuTab tab);
    void applyProfile(std::string const& profile);
    void executeAction(std::string const& action);

    void onTab(CCObject* sender);
    void onToggle(CCObject* sender);
    void onAction(CCObject* sender);

    MenuTab m_tab = MenuTab::Home;
    CCNode* m_content = nullptr;
    CCMenu* m_sidebar = nullptr;
    float m_cursorY = 0.f;
};

void openNovaMenu();

} // namespace nova
