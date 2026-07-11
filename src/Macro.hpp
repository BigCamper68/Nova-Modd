#pragma once

#include <Geode/Geode.hpp>

#include <filesystem>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace nova {

enum class MacroState {
    Idle,
    Recording,
    Playback
};

struct MacroInput {
    double time = 0.0;
    int button = 1;
    bool player1 = true;
    bool down = false;
};

class MacroManager final {
public:
    static MacroManager& get();

    MacroState state() const;
    std::string stateName() const;
    bool isApplying() const;
    size_t inputCount() const;
    size_t playbackIndex() const;
    std::string levelID() const;

    bool startRecording(GJBaseGameLayer* layer);
    bool startPlayback(GJBaseGameLayer* layer);
    void stop();
    void clear();
    void onAttemptReset();
    void onInput(GJBaseGameLayer* layer, bool down, int button, bool player1);
    void tick(GJBaseGameLayer* layer, float dt);

    bool saveSlot(int slot);
    bool loadSlot(int slot);
    bool exportClipboard() const;
    bool importClipboard();
    void openMacroFolder() const;

    void requestRestart();
    bool consumeLoopRequest();

private:
    MacroManager() = default;

    bool isPracticeContext(GJBaseGameLayer* layer) const;
    void releaseAll(GJBaseGameLayer* layer);
    std::filesystem::path macroFolder() const;
    std::filesystem::path slotPath(int slot) const;
    std::string serialize() const;
    bool deserialize(std::string const& text);

    MacroState m_state = MacroState::Idle;
    std::vector<MacroInput> m_inputs;
    size_t m_index = 0;
    bool m_applying = false;
    bool m_loopRequested = false;
    std::string m_levelID = "0";
};

} // namespace nova
