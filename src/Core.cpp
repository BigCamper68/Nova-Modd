#include "Core.hpp"
#include "Macro.hpp"

#include <algorithm>
#include <cmath>

namespace nova {

Runtime& Runtime::get() {
    static Runtime instance;
    return instance;
}

bool Runtime::enabled(std::string const& key, bool defaultValue) const {
    return Mod::get()->getSavedValue<bool>(fmt::format("feature.{}", key), defaultValue);
}

void Runtime::setEnabled(std::string const& key, bool value) {
    Mod::get()->setSavedValue<bool>(fmt::format("feature.{}", key), value);
}

bool Runtime::toggle(std::string const& key, bool defaultValue) {
    auto value = !enabled(key, defaultValue);
    setEnabled(key, value);
    return value;
}

void Runtime::onLevelStart(PlayLayer*) {
    m_attempts = 1;
    m_deaths = 0;
    m_lastDeath = 0;
    m_sessionSeconds = 0.f;
    m_clicks.clear();
    m_player1Held = false;
    m_player2Held = false;
    m_macroSlot = std::clamp(Mod::get()->getSavedValue<int>("macro-slot", 1), 1, 5);
    m_attemptTainted = std::abs(m_timeScale - 1.f) > 0.001f;
}

void Runtime::onAttemptReset() {
    ++m_attempts;
    m_player1Held = false;
    m_player2Held = false;
    m_attemptTainted = MacroManager::get().state() != MacroState::Idle || std::abs(m_timeScale - 1.f) > 0.001f;
}

void Runtime::onDeath(int percent) {
    ++m_deaths;
    m_lastDeath = std::clamp(percent, 0, 100);
}

void Runtime::onInput(bool down, int button, bool player1, bool countClick) {
    if (button == 1) {
        if (player1) m_player1Held = down;
        else m_player2Held = down;
    }

    auto countAll = enabled("cps-all-buttons", false);
    if (countClick && down && (button == 1 || countAll)) {
        m_clicks.push_back(std::chrono::steady_clock::now());
        pruneClicks();
    }
}

void Runtime::update(float dt) {
    if (std::isfinite(dt) && dt > 0.f && dt < 2.f) {
        m_sessionSeconds += dt;
    }
    pruneClicks();
}

void Runtime::resetSession() {
    m_attempts = 1;
    m_deaths = 0;
    m_lastDeath = 0;
    m_sessionSeconds = 0.f;
    m_clicks.clear();
}

void Runtime::pruneClicks() const {
    auto const now = std::chrono::steady_clock::now();
    auto const window = std::chrono::milliseconds(
        enabled("cps-two-second-window", false) ? 2000 : 1000
    );

    while (!m_clicks.empty() && now - m_clicks.front() > window) {
        m_clicks.pop_front();
    }
}

int Runtime::cps() const {
    pruneClicks();
    if (enabled("cps-two-second-window", false)) {
        return static_cast<int>(std::lround(m_clicks.size() / 2.0));
    }
    return static_cast<int>(m_clicks.size());
}

int Runtime::attempts() const { return m_attempts; }
int Runtime::deaths() const { return m_deaths; }
int Runtime::lastDeathPercent() const { return m_lastDeath; }
float Runtime::sessionSeconds() const { return m_sessionSeconds; }
bool Runtime::player1Held() const { return m_player1Held; }
bool Runtime::player2Held() const { return m_player2Held; }

void Runtime::setTimeScale(float value) {
    m_timeScale = std::clamp(value, 0.25f, 2.f);
    if (std::abs(m_timeScale - 1.f) > 0.001f) {
        m_attemptTainted = true;
    }
    if (auto scheduler = CCDirector::get()->getScheduler()) {
        scheduler->setTimeScale(m_timeScale);
    }
}

float Runtime::timeScale() const {
    return m_timeScale;
}

void Runtime::markProgressUnsafe() {
    m_attemptTainted = true;
}

void Runtime::clearProgressUnsafe() {
    m_attemptTainted = false;
}

bool Runtime::shouldBlockProgress() const {
    return m_attemptTainted || MacroManager::get().state() != MacroState::Idle || std::abs(m_timeScale - 1.f) > 0.001f;
}

int Runtime::macroSlot() const {
    return m_macroSlot;
}

void Runtime::setMacroSlot(int slot) {
    m_macroSlot = std::clamp(slot, 1, 5);
    Mod::get()->setSavedValue<int>("macro-slot", m_macroSlot);
}

int Runtime::nextMacroSlot() {
    setMacroSlot(m_macroSlot % 5 + 1);
    return m_macroSlot;
}

void notify(std::string const& text, NotificationIcon icon) {
    Notification::create(text, icon, 0.35f)->show();
}

PlayLayer* currentPlayLayer() {
    return typeinfo_cast<PlayLayer*>(GJBaseGameLayer::get());
}

std::string formatDuration(float seconds) {
    auto total = std::max(0, static_cast<int>(seconds));
    auto mins = total / 60;
    auto secs = total % 60;
    return fmt::format("{:02}:{:02}", mins, secs);
}

} // namespace nova
