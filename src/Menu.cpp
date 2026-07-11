#include "Menu.hpp"
#include "Core.hpp"
#include "Macro.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/general.hpp>

#include <array>

namespace nova {

namespace {

enum Action {
    ActNone = 0,
    ActRestart,
    ActPractice,
    ActClearCheckpoints,
    ActSpeed075,
    ActSpeed100,
    ActSpeed150,
    ActRecord,
    ActPlayback,
    ActStop,
    ActClearMacro,
    ActSave,
    ActLoad,
    ActNextSlot,
    ActCopy,
    ActPaste,
    ActMacroFolder,
    ActCopyLevelID,
    ActCopyPosition,
    ActSaveFolder,
    ActSettings,
    ActProfileTraining,
    ActProfileMinimal,
    ActProfileMacro,
    ActProfileDefault
};

char const* tabName(MenuTab tab) {
    switch (tab) {
        case MenuTab::Home: return "HOME";
        case MenuTab::HUD: return "HUD";
        case MenuTab::Practice: return "PRACTICE";
        case MenuTab::Macro: return "MACRO BOT";
        case MenuTab::Input: return "INPUT";
        case MenuTab::Tools: return "TOOLS";
        case MenuTab::Profiles: return "PROFILES";
    }
    return "NOVA";
}

ButtonSprite* buttonSprite(std::string const& text, char const* texture, float width = 145.f) {
    return ButtonSprite::create(text.c_str(), static_cast<int>(width), 0, 0.42f, false, "bigFont.fnt", texture, 28.f);
}

void withPlayLayer(std::function<void(PlayLayer*)> const& fn) {
    if (auto play = currentPlayLayer()) fn(play);
    else notify("Open a level first", NotificationIcon::Warning);
}

} // namespace

NovaMenuPopup* NovaMenuPopup::create() {
    auto ret = new NovaMenuPopup();
    if (ret && ret->initAnchored(470.f, 285.f, "GJ_square01.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool NovaMenuPopup::setup() {
    setKeypadEnabled(true);

    auto title = CCLabelBMFont::create("NOVA MENU", "goldFont.fnt");
    title->setScale(0.75f);
    m_mainLayer->addChildAtPosition(title, Anchor::Top, {0.f, -18.f});

    auto tabs = CCMenu::create();
    tabs->setContentSize({100.f, 220.f});
    tabs->setPosition({0.f, 0.f});

    constexpr std::array<std::pair<MenuTab, char const*>, 7> items = {{
        {MenuTab::Home, "Home"}, {MenuTab::HUD, "HUD"},
        {MenuTab::Practice, "Practice"}, {MenuTab::Macro, "Macro"},
        {MenuTab::Input, "Input"}, {MenuTab::Tools, "Tools"},
        {MenuTab::Profiles, "Profiles"}
    }};

    for (size_t i = 0; i < items.size(); ++i) {
        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite(items[i].second, "GJ_button_04.png", 88.f),
            this,
            menu_selector(NovaMenuPopup::onTab)
        );
        button->setTag(static_cast<int>(items[i].first));
        button->setPosition({0.f, 88.f - static_cast<float>(i) * 29.f});
        tabs->addChild(button);
    }
    m_mainLayer->addChildAtPosition(tabs, Anchor::Left, {62.f, -5.f});

    m_contentRoot = CCNode::create();
    m_contentRoot->setContentSize({325.f, 220.f});
    m_mainLayer->addChildAtPosition(m_contentRoot, Anchor::Center, {63.f, -5.f});

    m_tabTitle = CCLabelBMFont::create("HOME", "bigFont.fnt");
    m_tabTitle->setScale(0.55f);
    m_tabTitle->setAnchorPoint({0.f, 0.5f});
    m_tabTitle->setPosition({0.f, 209.f});
    m_contentRoot->addChild(m_tabTitle);

    m_contentMenu = CCMenu::create();
    m_contentMenu->setContentSize({325.f, 185.f});
    m_contentMenu->setAnchorPoint({0.f, 0.f});
    m_contentMenu->setPosition({0.f, 15.f});
    m_contentRoot->addChild(m_contentMenu);

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(0.42f);
    m_status->setPosition({162.f, 3.f});
    m_contentRoot->addChild(m_status);

    rebuild();
    return true;
}

void NovaMenuPopup::onClose(CCObject*) {
    removeFromParentAndCleanup(true);
}

void NovaMenuPopup::keyBackClicked() {
    onClose(nullptr);
}

void NovaMenuPopup::onTab(CCObject* sender) {
    m_tab = static_cast<MenuTab>(sender->getTag());
    rebuild();
}

void NovaMenuPopup::onEntry(CCObject* sender) {
    auto index = sender->getTag();
    if (index < 0 || static_cast<size_t>(index) >= m_entries.size()) return;
    auto const entry = m_entries[static_cast<size_t>(index)];
    if (!entry.key.empty()) {
        auto value = Runtime::get().toggle(entry.key, entry.defaultValue);
        notify(fmt::format("{}: {}", entry.label, value ? "ON" : "OFF"), value ? NotificationIcon::Success : NotificationIcon::Info);
    }
    else {
        performAction(entry.action);
    }
    rebuild();
}

std::vector<MenuEntry> NovaMenuPopup::entriesForTab(MenuTab tab) const {
    switch (tab) {
        case MenuTab::Home: return {
            {"HUD Master", "hud-master", true}, {"Auto Practice", "auto-practice", false},
            {"Hide Game UI", "hide-game-ui", false}, {"Safe Indicator", "hud-safe-indicator", true},
            {"Restart", "", false, ActRestart}, {"Toggle Practice", "", false, ActPractice},
            {"Geode Settings", "", false, ActSettings}
        };
        case MenuTab::HUD: return {
            {"FPS", "hud-fps", true}, {"CPS", "hud-cps", true},
            {"Progress", "hud-progress", true}, {"Attempts", "hud-attempts", true},
            {"Deaths", "hud-deaths", true}, {"Session Time", "hud-session", true},
            {"Macro Status", "hud-macro", true}, {"Input State", "hud-input-state", true},
            {"Coordinates", "hud-coordinates", false}, {"Compact", "hud-compact", false},
            {"Background", "hud-background", true}, {"High Contrast", "hud-high-contrast", false}
        };
        case MenuTab::Practice: return {
            {"Auto Practice", "auto-practice", false}, {"Hide Game UI", "hide-game-ui", false},
            {"Restart", "", false, ActRestart}, {"Clear Checkpoints", "", false, ActClearCheckpoints},
            {"Speed 0.75x", "", false, ActSpeed075}, {"Speed 1.00x", "", false, ActSpeed100},
            {"Speed 1.50x", "", false, ActSpeed150}
        };
        case MenuTab::Macro: return {
            {"Start Recording", "", false, ActRecord}, {"Start Playback", "", false, ActPlayback},
            {"Stop", "", false, ActStop}, {"Clear Macro", "", false, ActClearMacro},
            {"Save Slot", "", false, ActSave}, {"Load Slot", "", false, ActLoad},
            {"Next Slot", "", false, ActNextSlot}, {"Copy Macro", "", false, ActCopy},
            {"Paste Macro", "", false, ActPaste}, {"Open Folder", "", false, ActMacroFolder},
            {"Loop", "macro-loop", false}, {"Auto-save", "macro-auto-save", true}
        };
        case MenuTab::Input: return {
            {"Record P1", "macro-record-p1", true}, {"Record P2", "macro-record-p2", true},
            {"Record Releases", "macro-record-releases", true}, {"Jump Only", "macro-jump-only", false},
            {"Block Manual", "macro-block-manual", true}, {"Auto Record", "macro-auto-record", false},
            {"Playback in CPS", "cps-count-playback", false}, {"All Buttons CPS", "cps-all-buttons", false}
        };
        case MenuTab::Tools: return {
            {"Copy Level ID", "", false, ActCopyLevelID}, {"Copy Position", "", false, ActCopyPosition},
            {"Open Save Folder", "", false, ActSaveFolder}, {"Open Macro Folder", "", false, ActMacroFolder},
            {"Reset Defaults", "", false, ActProfileDefault}
        };
        case MenuTab::Profiles: return {
            {"Training", "", false, ActProfileTraining}, {"Minimal HUD", "", false, ActProfileMinimal},
            {"Macro Lab", "", false, ActProfileMacro}, {"Default", "", false, ActProfileDefault}
        };
    }
    return {};
}

void NovaMenuPopup::rebuild() {
    m_contentMenu->removeAllChildrenWithCleanup(true);
    m_entries = entriesForTab(m_tab);
    m_tabTitle->setString(tabName(m_tab));

    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto const& entry = m_entries[i];
        auto enabled = !entry.key.empty() && Runtime::get().enabled(entry.key, entry.defaultValue);
        auto text = entry.key.empty() ? entry.label : fmt::format("{} {}", enabled ? "[ON]" : "[OFF]", entry.label);
        auto texture = entry.key.empty() ? "GJ_button_02.png" : (enabled ? "GJ_button_01.png" : "GJ_button_04.png");
        auto button = CCMenuItemSpriteExtra::create(buttonSprite(text, texture), this, menu_selector(NovaMenuPopup::onEntry));
        button->setTag(static_cast<int>(i));
        auto col = i % 2;
        auto row = i / 2;
        button->setPosition({col == 0 ? 78.f : 244.f, 168.f - static_cast<float>(row) * 29.f});
        m_contentMenu->addChild(button);
    }
    updateStatus();
}

void NovaMenuPopup::updateStatus() {
    auto& runtime = Runtime::get();
    auto& macro = MacroManager::get();
    m_status->setString(fmt::format("BOT {} | INPUTS {} | SLOT {} | SPEED {:.2f}x{}",
        macro.stateName(), macro.inputCount(), runtime.macroSlot(), runtime.timeScale(),
        runtime.shouldBlockProgress() ? " | PROGRESS OFF" : "").c_str());
}

void NovaMenuPopup::performAction(int action) {
    auto& runtime = Runtime::get();
    auto& macro = MacroManager::get();
    switch (action) {
        case ActRestart: withPlayLayer([](PlayLayer* p) { p->resetLevel(); }); break;
        case ActPractice: withPlayLayer([](PlayLayer* p) { p->togglePracticeMode(!p->m_isPracticeMode); }); break;
        case ActClearCheckpoints: withPlayLayer([](PlayLayer* p) { p->removeAllCheckpoints(); }); break;
        case ActSpeed075: runtime.setTimeScale(0.75f); break;
        case ActSpeed100: runtime.setTimeScale(1.f); break;
        case ActSpeed150: runtime.setTimeScale(1.5f); break;
        case ActRecord:
            if (auto p = currentPlayLayer()) { if (macro.startRecording(p)) p->resetLevel(); }
            else notify("Open a practice level first", NotificationIcon::Warning);
            break;
        case ActPlayback:
            if (auto p = currentPlayLayer()) { if (macro.startPlayback(p)) p->resetLevel(); }
            else notify("Open a practice level first", NotificationIcon::Warning);
            break;
        case ActStop: macro.stop(); runtime.setTimeScale(1.f); break;
        case ActClearMacro: macro.clear(); break;
        case ActSave: macro.saveSlot(runtime.macroSlot()); break;
        case ActLoad: macro.loadSlot(runtime.macroSlot()); break;
        case ActNextSlot: notify(fmt::format("Macro slot {}", runtime.nextMacroSlot())); break;
        case ActCopy: notify(macro.exportClipboard() ? "Macro copied" : "Nothing to copy"); break;
        case ActPaste: notify(macro.importClipboard() ? "Macro imported" : "Invalid clipboard macro"); break;
        case ActMacroFolder: macro.openMacroFolder(); break;
        case ActCopyLevelID: withPlayLayer([](PlayLayer* p) { clipboard::write(std::to_string(p->m_level->m_levelID.value())); }); break;
        case ActCopyPosition: withPlayLayer([](PlayLayer* p) { auto v = p->m_player1->getPosition(); clipboard::write(fmt::format("{:.2f}, {:.2f}", v.x, v.y)); }); break;
        case ActSaveFolder: file::openFolder(Mod::get()->getSaveDir()); break;
        case ActSettings: openSettingsPopup(Mod::get()); break;
        case ActProfileTraining: applyProfile(1); break;
        case ActProfileMinimal: applyProfile(2); break;
        case ActProfileMacro: applyProfile(3); break;
        case ActProfileDefault: applyProfile(0); break;
        default: break;
    }
}

void NovaMenuPopup::applyProfile(int profile) {
    resetFeatures();
    auto& r = Runtime::get();
    if (profile == 1) {
        r.setEnabled("auto-practice", true);
        r.setEnabled("hud-coordinates", true);
        r.setTimeScale(0.75f);
        notify("Training profile applied", NotificationIcon::Success);
    }
    else if (profile == 2) {
        r.setEnabled("hud-compact", true);
        r.setEnabled("hud-deaths", false);
        r.setEnabled("hud-session", false);
        notify("Minimal HUD applied", NotificationIcon::Success);
    }
    else if (profile == 3) {
        r.setEnabled("auto-practice", true);
        r.setEnabled("macro-auto-save", true);
        r.setEnabled("macro-block-manual", true);
        notify("Macro Lab applied", NotificationIcon::Success);
    }
    else notify("Defaults applied", NotificationIcon::Success);
}

void NovaMenuPopup::resetFeatures() {
    auto& r = Runtime::get();
    constexpr std::array<std::pair<char const*, bool>, 24> defaults = {{
        {"hud-master", true}, {"hud-fps", true}, {"hud-cps", true}, {"hud-progress", true},
        {"hud-attempts", true}, {"hud-deaths", true}, {"hud-session", true}, {"hud-macro", true},
        {"hud-input-state", true}, {"hud-coordinates", false}, {"hud-compact", false},
        {"hud-background", true}, {"hud-high-contrast", false}, {"hud-safe-indicator", true},
        {"auto-practice", false}, {"hide-game-ui", false}, {"macro-loop", false},
        {"macro-auto-save", true}, {"macro-record-p1", true}, {"macro-record-p2", true},
        {"macro-record-releases", true}, {"macro-jump-only", false},
        {"macro-block-manual", true}, {"macro-auto-record", false}
    }};
    for (auto const& [key, value] : defaults) r.setEnabled(key, value);
    r.setTimeScale(1.f);
    MacroManager::get().stop();
}

void openNovaMenu() {
    if (auto popup = NovaMenuPopup::create()) popup->show();
}

} // namespace nova
