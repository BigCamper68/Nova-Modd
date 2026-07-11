#include "Core.hpp"
#include "HUD.hpp"
#include "Macro.hpp"
#include "Menu.hpp"

#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace nova {

class $modify(NovaMainMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!Mod::get()->getSettingValue<bool>("show-main-menu-button")) return true;

        auto menu = getChildByID("bottom-menu");
        if (!menu) return true;

        auto icon = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!icon) return true;
        icon->setScale(0.72f);

        auto button = CCMenuItemSpriteExtra::create(
            icon,
            this,
            menu_selector(NovaMainMenuLayer::onNovaMenu)
        );
        button->setID("nova-menu-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onNovaMenu(CCObject*) {
        openNovaMenu();
    }
};

class $modify(NovaPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!Mod::get()->getSettingValue<bool>("show-pause-button")) return;

        auto menu = getChildByID("left-button-menu");
        if (!menu) return;

        auto icon = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!icon) return;
        icon->setScale(0.62f);

        auto circle = CircleButtonSprite::create(icon, CircleBaseColor::Blue);
        auto button = CCMenuItemSpriteExtra::create(
            circle,
            this,
            menu_selector(NovaPauseLayer::onNovaMenu)
        );
        button->setID("nova-pause-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
    }

    void onNovaMenu(CCObject*) {
        openNovaMenu();
    }
};

class $modify(NovaPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto& runtime = Runtime::get();
        runtime.onLevelStart(this);
        MacroManager::get().onAttemptReset();

        if (runtime.enabled("auto-practice", false) && !m_isPracticeMode) {
            togglePracticeMode(true);
        }

        if (auto hud = NovaHUD::create(this)) {
            addChild(hud, 9999);
        }

        if (m_uiLayer) {
            m_uiLayer->setVisible(!runtime.enabled("hide-game-ui", false));
        }
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        Runtime::get().onAttemptReset();
        MacroManager::get().onAttemptReset();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        Runtime::get().onDeath(getCurrentPercentInt());
        PlayLayer::destroyPlayer(player, object);

        if (
            MacroManager::get().state() == MacroState::Playback &&
            Runtime::get().enabled("macro-restart-on-death", true)
        ) {
            MacroManager::get().requestRestart();
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (m_uiLayer) {
            m_uiLayer->setVisible(!Runtime::get().enabled("hide-game-ui", false));
        }

        if (MacroManager::get().consumeLoopRequest()) {
            resetLevel();
        }
    }

    void levelComplete() {
        auto oldTestMode = m_isTestMode;
        if (Runtime::get().shouldBlockProgress()) {
            m_isTestMode = true;
        }
        PlayLayer::levelComplete();
        m_isTestMode = oldTestMode;
    }

    void showNewBest(bool newReward, int orbs, int diamonds, bool demonKey, bool noRetry, bool noTitle) {
        if (Runtime::get().shouldBlockProgress()) return;
        PlayLayer::showNewBest(newReward, orbs, diamonds, demonKey, noRetry, noTitle);
    }

    void onQuit() {
        MacroManager::get().stop();
        Runtime::get().setTimeScale(1.f);
        PlayLayer::onQuit();
        Runtime::get().clearProgressUnsafe();
    }
};

class $modify(NovaBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& macro = MacroManager::get();
        auto& runtime = Runtime::get();

        if (
            down &&
            !macro.isApplying() &&
            macro.state() == MacroState::Idle &&
            runtime.enabled("macro-auto-record", false)
        ) {
            macro.startRecording(this);
        }

        if (
            macro.state() == MacroState::Playback &&
            !macro.isApplying() &&
            runtime.enabled("macro-block-manual", true)
        ) {
            return;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        auto countClick = !macro.isApplying() || runtime.enabled("cps-count-playback", false);
        runtime.onInput(down, button, isPlayer1, countClick);
        macro.onInput(this, down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        MacroManager::get().tick(this, dt);
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(NovaGameLevel, GJGameLevel) {
    static void onModify(auto& self) {
        (void)self.setHookPriorityPost("GJGameLevel::savePercentage", Priority::First);
    }

    void savePercentage(int percent, bool isPractice, int clicks, int attempts, bool saveReplay) {
        if (Runtime::get().shouldBlockProgress()) return;
        GJGameLevel::savePercentage(percent, isPractice, clicks, attempts, saveReplay);
    }
};

class $modify(NovaKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        if (
            isKeyDown &&
            !isKeyRepeat &&
            key == enumKeyCodes::KEY_F2 &&
            Mod::get()->getSettingValue<bool>("enable-f2-keybind")
        ) {
            openNovaMenu();
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};

} // namespace nova
