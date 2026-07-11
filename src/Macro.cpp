#include "Macro.hpp"
#include "Core.hpp"

#include <Geode/utils/general.hpp>
#include <Geode/utils/file.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace nova {

namespace {
constexpr size_t kMaxMacroInputs = 250000;
}

MacroManager& MacroManager::get() {
    static MacroManager instance;
    return instance;
}

MacroState MacroManager::state() const { return m_state; }
size_t MacroManager::inputCount() const { return m_inputs.size(); }
size_t MacroManager::playbackIndex() const { return m_index; }
std::string MacroManager::levelID() const { return m_levelID; }
bool MacroManager::isApplying() const { return m_applying; }

std::string MacroManager::stateName() const {
    switch (m_state) {
        case MacroState::Recording: return "RECORDING";
        case MacroState::Playback: return "PLAYBACK";
        default: return "IDLE";
    }
}

bool MacroManager::isPracticeContext(GJBaseGameLayer* layer) const {
    auto play = typeinfo_cast<PlayLayer*>(layer);
    return play && (play->m_isPracticeMode || play->m_isTestMode);
}

void MacroManager::releaseAll(GJBaseGameLayer* layer) {
    if (!layer) return;
    m_applying = true;
    for (int button = 1; button <= 5; ++button) {
        layer->handleButton(false, button, true);
        layer->handleButton(false, button, false);
    }
    m_applying = false;
}

bool MacroManager::startRecording(GJBaseGameLayer* layer) {
    if (!isPracticeContext(layer)) {
        notify("Macro recording requires Practice/Test Mode", NotificationIcon::Warning);
        return false;
    }

    m_inputs.clear();
    m_index = 0;
    m_loopRequested = false;
    m_state = MacroState::Recording;
    Runtime::get().markProgressUnsafe();
    if (layer && layer->m_level) {
        m_levelID = std::to_string(layer->m_level->m_levelID.value());
    }
    notify("Macro recording started", NotificationIcon::Success);
    return true;
}

bool MacroManager::startPlayback(GJBaseGameLayer* layer) {
    if (!isPracticeContext(layer)) {
        notify("Macro playback requires Practice/Test Mode", NotificationIcon::Warning);
        return false;
    }
    if (m_inputs.empty()) {
        notify("No macro is loaded", NotificationIcon::Error);
        return false;
    }
    if (
        Runtime::get().enabled("macro-strict-level", true) &&
        layer && layer->m_level &&
        m_levelID != "0" &&
        m_levelID != std::to_string(layer->m_level->m_levelID.value())
    ) {
        notify("Macro belongs to a different level", NotificationIcon::Error);
        return false;
    }

    m_index = 0;
    m_loopRequested = false;
    m_state = MacroState::Playback;
    Runtime::get().markProgressUnsafe();
    notify(fmt::format("Playback started: {} inputs", m_inputs.size()), NotificationIcon::Success);
    return true;
}

void MacroManager::stop() {
    auto oldState = m_state;
    if (oldState == MacroState::Playback) {
        releaseAll(GJBaseGameLayer::get());
    }
    m_state = MacroState::Idle;
    m_index = 0;
    m_loopRequested = false;

    if (oldState == MacroState::Recording && Runtime::get().enabled("macro-auto-save", true)) {
        saveSlot(Runtime::get().macroSlot());
    }

    if (oldState != MacroState::Idle) {
        notify("Macro stopped", NotificationIcon::Info);
    }
}

void MacroManager::clear() {
    m_state = MacroState::Idle;
    m_inputs.clear();
    m_index = 0;
    m_loopRequested = false;
    m_levelID = "0";
    notify("Macro cleared", NotificationIcon::Success);
}

void MacroManager::onAttemptReset() {
    m_index = 0;
    m_loopRequested = false;

    if (m_state == MacroState::Recording) {
        m_inputs.clear();
    }
}

void MacroManager::onInput(GJBaseGameLayer* layer, bool down, int button, bool player1) {
    if (m_state != MacroState::Recording || m_applying || !layer) return;
    if (m_inputs.size() >= kMaxMacroInputs) {
        stop();
        notify("Macro input limit reached", NotificationIcon::Error);
        return;
    }

    auto& runtime = Runtime::get();
    if (player1 && !runtime.enabled("macro-record-p1", true)) return;
    if (!player1 && !runtime.enabled("macro-record-p2", true)) return;
    if (!down && !runtime.enabled("macro-record-releases", true)) return;
    if (button != 1 && runtime.enabled("macro-jump-only", false)) return;

    MacroInput input;
    input.time = layer->m_gameState.m_levelTime;
    input.button = button;
    input.player1 = player1;
    input.down = down;
    m_inputs.push_back(input);
}

void MacroManager::tick(GJBaseGameLayer* layer, float dt) {
    if (m_state != MacroState::Playback || !layer || m_inputs.empty()) return;
    if (!isPracticeContext(layer)) {
        stop();
        return;
    }

    auto const gameTime = layer->m_gameState.m_levelTime + std::max(0.f, dt);
    while (m_index < m_inputs.size() && m_inputs[m_index].time <= gameTime) {
        auto const input = m_inputs[m_index++];
        m_applying = true;
        layer->handleButton(input.down, input.button, input.player1);
        m_applying = false;
    }

    if (m_index >= m_inputs.size()) {
        if (Runtime::get().enabled("macro-loop", false)) {
            m_loopRequested = true;
        }
        else {
            releaseAll(layer);
            m_state = MacroState::Idle;
            m_index = 0;
            notify("Macro playback finished", NotificationIcon::Success);
        }
    }
}

std::filesystem::path MacroManager::macroFolder() const {
    return Mod::get()->getSaveDir() / "macros";
}

std::filesystem::path MacroManager::slotPath(int slot) const {
    return macroFolder() / fmt::format("slot-{}.nova", std::clamp(slot, 1, 5));
}

std::string MacroManager::serialize() const {
    std::ostringstream out;
    out << "NOVA_MACRO_V1\n";
    out << m_levelID << '\n';
    out << m_inputs.size() << '\n';
    out << std::setprecision(17);
    for (auto const& input : m_inputs) {
        out << input.time << '|'
            << input.button << '|'
            << (input.player1 ? 1 : 0) << '|'
            << (input.down ? 1 : 0) << '\n';
    }
    return out.str();
}

bool MacroManager::deserialize(std::string const& text) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || line != "NOVA_MACRO_V1") return false;
    if (!std::getline(in, m_levelID) || m_levelID.size() > 64) return false;

    size_t expected = 0;
    if (!std::getline(in, line)) return false;
    try {
        expected = std::min(static_cast<size_t>(std::stoull(line)), kMaxMacroInputs);
    }
    catch (...) {
        return false;
    }

    std::vector<MacroInput> parsed;
    parsed.reserve(expected);
    while (parsed.size() < expected && std::getline(in, line)) {
        std::istringstream row(line);
        std::string part;
        MacroInput input;
        try {
            if (!std::getline(row, part, '|')) return false;
            input.time = std::stod(part);
            if (!std::getline(row, part, '|')) return false;
            input.button = std::stoi(part);
            if (!std::getline(row, part, '|')) return false;
            auto const player = std::stoi(part);
            if (!std::getline(row, part, '|')) return false;
            auto const down = std::stoi(part);
            if (!std::isfinite(input.time) || input.time < 0.0) return false;
            if (input.button < 1 || input.button > 5) return false;
            if ((player != 0 && player != 1) || (down != 0 && down != 1)) return false;
            input.player1 = player != 0;
            input.down = down != 0;
        }
        catch (...) {
            return false;
        }
        parsed.push_back(input);
    }

    if (parsed.size() != expected) return false;
    std::stable_sort(parsed.begin(), parsed.end(), [](auto const& a, auto const& b) {
        return a.time < b.time;
    });

    m_inputs = std::move(parsed);
    m_state = MacroState::Idle;
    m_index = 0;
    m_loopRequested = false;
    return true;
}

bool MacroManager::saveSlot(int slot) {
    if (m_inputs.empty()) {
        notify("Nothing to save", NotificationIcon::Warning);
        return false;
    }

    (void)file::createDirectoryAll(macroFolder());
    auto result = file::writeString(slotPath(slot), serialize());
    if (result.isErr()) {
        notify("Could not save macro slot", NotificationIcon::Error);
        return false;
    }
    notify(fmt::format("Saved macro to slot {}", slot), NotificationIcon::Success);
    return true;
}

bool MacroManager::loadSlot(int slot) {
    auto result = file::readString(slotPath(slot));
    if (result.isErr() || !deserialize(result.unwrap())) {
        notify(fmt::format("Slot {} is empty or invalid", slot), NotificationIcon::Error);
        return false;
    }
    notify(fmt::format("Loaded slot {}: {} inputs", slot, m_inputs.size()), NotificationIcon::Success);
    return true;
}

bool MacroManager::exportClipboard() const {
    if (m_inputs.empty()) return false;
    return clipboard::write(serialize());
}

bool MacroManager::importClipboard() {
    auto text = clipboard::read();
    if (text.empty()) return false;
    return deserialize(text);
}

void MacroManager::openMacroFolder() const {
    (void)file::createDirectoryAll(macroFolder());
    file::openFolder(macroFolder());
}

void MacroManager::requestRestart() {
    if (m_state == MacroState::Playback) {
        m_loopRequested = true;
    }
}

bool MacroManager::consumeLoopRequest() {
    auto requested = m_loopRequested;
    m_loopRequested = false;
    if (requested) m_index = 0;
    return requested;
}

} // namespace nova
