#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace nova {

class NovaHUD : public CCNode {
public:
    static NovaHUD* create(PlayLayer* owner);

    bool init(PlayLayer* owner);
    void updateHUD(float dt);

private:
    PlayLayer* m_owner = nullptr;
    CCLabelBMFont* m_label = nullptr;
    CCScale9Sprite* m_background = nullptr;
    float m_accumulator = 0.f;
};

} // namespace nova
