#pragma once

#include <Geode/Geode.hpp>

#include <chrono>
#include <deque>
#include <string>

using namespace geode::prelude;

namespace nova {

class Runtime final {
public:
    static Runtime& get();

    bool enabled(std::string const& key, bool defaultValue = false) const;
    void setEnabled(std::string const& key, bool value);
    bool toggle(std::string const& key, bool defaultValue = false);

    void onLevelStart(PlayLayer* layer);
    void onAttemptReset();
    void onDeath(int percent);
    void onInput(bool down, int button, bool player1, bool countClick = true);
    void update(float dt);
    void resetSession();

    int cps() const;
    int attempts() const;
    int deaths() const;
    int lastDeathPercent() const;
    float sessionSeconds() const;
    bool player1Held() const;
    bool player2Held() const;

    void setTimeScale(float value);
    float timeScale() const;
    void markProgressUnsafe();
    void clearProgressUnsafe();
    bool shouldBlockProgress() const;

    int macroSlot() const;
    void setMacroSlot(int slot);
    int nextMacroSlot();

private:
    Runtime() = default;
    void pruneClicks() const;

    mutable std::deque<std::chrono::steady_clock::time_point> m_clicks;
    int m_attempts = 0;
    int m_deaths = 0;
    int m_lastDeath = 0;
    float m_sessionSeconds = 0.f;
    float m_timeScale = 1.f;
    bool m_player1Held = false;
    bool m_player2Held = false;
    int m_macroSlot = 1;
    bool m_attemptTainted = false;
};

void notify(std::string const& text, NotificationIcon icon = NotificationIcon::Info);
PlayLayer* currentPlayLayer();
std::string formatDuration(float seconds);

} // namespace nova
