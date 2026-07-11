#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

#include <functional>
#include <string>
#include <vector>

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

struct MenuEntry {
    std::string label;
    std::string key;
    bool defaultValue = false;
    int action = 0;
};

class NovaMenuPopup final : public geode::Popup {
public:
    static NovaMenuPopup* create();
    bool setup() override;

    // Compatibility wrapper for the older Geode Popup API used by Menu.cpp.
    bool initAnchored(float width, float height, char const* background = "GJ_square01.png") {
        return init(width, height, background);
    }

    void onClose(CCObject* sender) override;
    void keyBackClicked() override;
    void onTab(CCObject* sender);
    void onEntry(CCObject* sender);

private:
    void rebuild();
    void updateStatus();
    std::vector<MenuEntry> entriesForTab(MenuTab tab) const;
    void performAction(int action);
    void applyProfile(int profile);
    void resetFeatures();

    MenuTab m_tab = MenuTab::Home;
    CCNode* m_contentRoot = nullptr;
    CCMenu* m_contentMenu = nullptr;
    CCLabelBMFont* m_tabTitle = nullptr;
    CCLabelBMFont* m_status = nullptr;
    std::vector<MenuEntry> m_entries;
};

void openNovaMenu();

} // namespace nova
