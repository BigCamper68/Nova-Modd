#include "HUD.hpp"
#include "Core.hpp"
#include "Macro.hpp"

#include <cmath>
#include <sstream>
#include <vector>

namespace nova {

NovaHUD* NovaHUD::create(PlayLayer* layer) {
    auto ret = new NovaHUD();
    if (ret && ret->init(layer)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool NovaHUD::init(PlayLayer* layer) {
    if (!CCNode::init() || !layer) return false;
    m_layer = layer;

    auto win = CCDirector::get()->getWinSize();
    setContentSize({150.f, 112.f});
    setAnchorPoint({0.f, 1.f});
    setPosition({8.f, win.height - 8.f});
    setZOrder(9999);
    setID("nova-hud"_spr);

    m_background = CCScale9Sprite::create("square02_001.png");
    m_background->setAnchorPoint({0.f, 1.f});
    m_background->setPosition({-4.f, 4.f});
    m_background->setContentSize({150.f, 112.f});
    m_background->setOpacity(95);
    addChild(m_background, -1);

    m_label = CCLabelBMFont::create("Nova Menu", "chatFont.fnt");
    m_label->setAnchorPoint({0.f, 1.f});
    m_label->setPosition({0.f, 0.f});
    m_label->setScale(0.55f);
    m_label->setAlignment(kCCTextAlignmentLeft);
    addChild(m_label);

    scheduleUpdate();
    return true;
}

void NovaHUD::update(float dt) {
    auto& runtime = Runtime::get();
    runtime.update(dt);

    auto visible = runtime.enabled("hud-master", true);
    setVisible(visible);
    if (!visible || !m_layer || !m_label) return;

    if (dt > 0.00001f && dt < 1.f) {
        auto instant = 1.f / dt;
        m_smoothedFPS = m_smoothedFPS * 0.9f + instant * 0.1f;
    }

    auto compact = runtime.enabled("hud-compact", false);
    std::vector<std::string> parts;

    if (runtime.enabled("hud-fps", true)) {
        parts.push_back(fmt::format("FPS {:.0f}", m_smoothedFPS));
    }
    if (runtime.enabled("hud-cps", true)) {
        parts.push_back(fmt::format("CPS {}", runtime.cps()));
    }
    if (runtime.enabled("hud-progress", true)) {
        parts.push_back(fmt::format("{}%", m_layer->getCurrentPercentInt()));
    }
    if (runtime.enabled("hud-attempts", true)) {
        parts.push_back(fmt::format("ATT {}", runtime.attempts()));
    }
    if (runtime.enabled("hud-deaths", true)) {
        parts.push_back(fmt::format("DEATHS {}", runtime.deaths()));
    }
    if (runtime.enabled("hud-session", true)) {
        parts.push_back(fmt::format("TIME {}", formatDuration(runtime.sessionSeconds())));
    }
    if (runtime.enabled("hud-last-death", false) && runtime.deaths() > 0) {
        parts.push_back(fmt::format("LAST {}%", runtime.lastDeathPercent()));
    }
    if (runtime.enabled("hud-coordinates", false) && m_layer->m_player1) {
        auto p = m_layer->m_player1->getPosition();
        parts.push_back(fmt::format("X {:.0f} Y {:.0f}", p.x, p.y));
    }
    if (runtime.enabled("hud-input-state", true)) {
        parts.push_back(fmt::format(
            "P1 {}  P2 {}",
            runtime.player1Held() ? "DOWN" : "UP",
            runtime.player2Held() ? "DOWN" : "UP"
        ));
    }
    if (runtime.enabled("hud-macro", true)) {
        auto& macro = MacroManager::get();
        parts.push_back(fmt::format(
            "BOT {} {}/{} S{}",
            macro.stateName(),
            macro.playbackIndex(),
            macro.inputCount(),
            runtime.macroSlot()
        ));
    }
    if (runtime.enabled("hud-speed", true) && std::abs(runtime.timeScale() - 1.f) > 0.001f) {
        parts.push_back(fmt::format("SPEED {:.2f}x", runtime.timeScale()));
    }
    if (runtime.shouldBlockProgress() && runtime.enabled("hud-safe-indicator", true)) {
        parts.push_back("SAFE MODE: PROGRESS OFF");
    }

    std::ostringstream text;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) text << (compact ? " | " : "\n");
        text << parts[i];
    }

    m_label->setString(text.str().c_str());
    m_label->setScale(compact ? 0.45f : 0.55f);
    m_label->setColor(runtime.shouldBlockProgress() ? ccColor3B{255, 190, 80} : ccColor3B{255, 255, 255});

    auto lineCount = compact ? 1.f : static_cast<float>(std::max<size_t>(1, parts.size()));
    auto width = compact ? 330.f : 150.f;
    auto height = compact ? 26.f : 12.f + lineCount * 13.f;
    m_background->setContentSize({width, height});
    m_background->setVisible(runtime.enabled("hud-background", true));
    m_background->setOpacity(runtime.enabled("hud-high-contrast", false) ? 165 : 95);
}

} // namespace nova
