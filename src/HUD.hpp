#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace nova {

class NovaHUD final : public CCNode {
public:
    static NovaHUD* create(PlayLayer* layer);
    bool init(PlayLayer* layer);
    void update(float dt) override;

private:
    PlayLayer* m_layer = nullptr;
    CCLabelBMFont* m_label = nullptr;
    CCScale9Sprite* m_background = nullptr;
    float m_smoothedFPS = 60.f;
};

} // namespace nova
