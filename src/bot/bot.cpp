#include "bot.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

using namespace geode::prelude;

Bot& Bot::get() {
    static Bot instance;
    return instance;
}

float Bot::tps() const {
    auto mod = Mod::get();
    if (!mod) return 240.f;
    int value = mod->getSettingValue<int>("macro-fps");
    if (value < 10) value = 240;
    return static_cast<float>(value);
}

unsigned int Bot::currentFrame() const {
    auto pl = PlayLayer::get();
    if (!pl) return 0;

    double time = pl->m_gameState.m_levelTime;
    double frame = time * static_cast<double>(tps());
    auto out = static_cast<long long>(frame) + 1;

    out -= static_cast<long long>(frameOffset());
    if (out < 0) out = 0;
    return static_cast<unsigned int>(out);
}

bool Bot::practiceFix() const {
    return Mod::get() ? Mod::get()->getSettingValue<bool>("practice-fix") : true;
}

bool Bot::autoStop() const {
    return Mod::get() ? Mod::get()->getSettingValue<bool>("auto-stop") : true;
}

bool Bot::autoRestart() const {
    return Mod::get() ? Mod::get()->getSettingValue<bool>("auto-restart") : true;
}

int Bot::frameOffset() const {
    return Mod::get() ? Mod::get()->getSettingValue<int>("frame-offset") : 0;
}

void Bot::setStatus(std::string const& text) {
    statusText = text;
}

// ------------------------------------------------------------------ recording

bool Bot::startRecording(PlayLayer* pl) {
    if (!pl) return false;

    replay = gdr::Replay{};
    replay.bot.name = "GDRBot";
    replay.bot.version = "1.0.0";
    if (auto mod = Mod::get()) replay.bot.version = std::string(mod->getVersion().toNonVString());
    replay.framerate = tps();
    replay.gameVersion = 2.208f;
    replay.author = "Player";
    if (auto gm = GameManager::get()) {
        replay.author = std::string(gm->m_playerName);
    }
    replay.level.id = pl->m_level ? pl->m_level->m_levelID.value() : 0;
    replay.level.name = botutil::levelName(pl);
    replay.seed = static_cast<int>(pl->m_randomSeed);
    replay.ldm = GameManager::get() && GameManager::get()->getGameVariable("0096");

    markers.clear();
    actionIndex = 0;
    respawnFrame = 0xFFFFFFFFu;
    previousFrame = 0;

    state = BotState::Recording;
    setStatus("Recording");
    return true;
}

void Bot::stopRecording(PlayLayer* pl) {
    if (state != BotState::Recording) return;

    unsigned int frame = currentFrame();
    replay.duration = static_cast<double>(frame) / static_cast<double>(replay.framerate);

    if (pl && pl->m_level) {
        replay.level.id = pl->m_level->m_levelID.value();
        replay.level.name = botutil::levelName(pl);
    }

    replay.seed = static_cast<int>(pl ? pl->m_randomSeed : 0);

    state = BotState::Idle;
    setStatus(fmt::format("Recorded {} inputs", replay.inputs.size()));
}

void Bot::recordInput(bool down, int button, bool player2) {
    if (state != BotState::Recording || suppressRecord) return;

    auto frame = currentFrame();
    replay.inputs.emplace_back(frame, button, player2, down);

    // keep duration in sync for tools that only read the tail of the run
    if (frame > 0) {
        replay.duration = static_cast<double>(frame) / static_cast<double>(replay.framerate);
    }
}

void Bot::trimRecordedInputs(unsigned int frame) {
    // Used when a checkpoint is loaded while recording: inputs recorded after
    // the checkpoint are invalid and must be dropped, otherwise the macro
    // contains actions that will never happen on a clean playback.
    auto it = std::find_if(replay.inputs.begin(), replay.inputs.end(),
        [frame](gdr::Input const& input) { return input.frame >= frame; });
    replay.inputs.erase(it, replay.inputs.end());

    auto fixes = std::find_if(replay.frameFixes.begin(), replay.frameFixes.end(),
        [frame](gdr::FrameFix const& fix) { return fix.frame >= frame; });
    replay.frameFixes.erase(fixes, replay.frameFixes.end());
}

// ------------------------------------------------------------------- playback

bool Bot::startPlaying(PlayLayer* pl) {
    if (!pl || !pendingReplay) return false;

    replay = *pendingReplay;
    pendingReplay.reset();

    if (replay.framerate < 10.f) replay.framerate = 240.f;

    std::stable_sort(replay.inputs.begin(), replay.inputs.end(),
        [](gdr::Input const& a, gdr::Input const& b) { return a.frame < b.frame; });

    actionIndex = 0;
    respawnFrame = 0xFFFFFFFFu;
    previousFrame = 0;

    if (pl->m_level && replay.level.id == 0) {
        replay.level.id = pl->m_level->m_levelID.value();
    }

    state = BotState::Playing;
    setStatus(fmt::format("Playing {} inputs", replay.inputs.size()));
    return true;
}

void Bot::stopPlaying(PlayLayer* pl) {
    if (state != BotState::Playing) return;

    if (pl) {
        if (pl->m_player1) pl->m_player1->releaseAllButtons();
        if (pl->m_player2) pl->m_player2->releaseAllButtons();
    }

    state = BotState::Idle;
    actionIndex = 0;
    setStatus("Idle");
}

static bool isIgnoredFrame(gdr::Replay const& replay, unsigned int frame) {
    for (auto const& fix : replay.frameFixes) {
        if (fix.frame == frame && fix.ignore) return true;
    }
    return false;
}

void Bot::applyInputs(unsigned int frame, GJBaseGameLayer* layer) {
    if (state != BotState::Playing || !layer) return;
    if (PlayLayer::get() && PlayLayer::get()->m_levelEndAnimationStarted) return;

    if (auto player1 = layer->m_player1; player1 && player1->m_isDead) {
        if (layer->m_player1) layer->m_player1->releaseAllButtons();
        if (layer->m_player2) layer->m_player2->releaseAllButtons();
        return;
    }

    while (actionIndex < replay.inputs.size() && frame >= replay.inputs[actionIndex].frame) {
        auto const& input = replay.inputs[actionIndex];

        if (input.frame != respawnFrame && !isIgnoredFrame(replay, input.frame)) {
            layer->handleButton(input.down, input.button, !input.player2);
        }

        actionIndex++;
    }

    if (actionIndex >= replay.inputs.size() && autoStop()) {
        stopPlaying(PlayLayer::get());
    }
}

void Bot::syncToFrame(unsigned int frame) {
    respawnFrame = frame;

    auto it = std::find_if(replay.inputs.begin(), replay.inputs.end(),
        [frame](gdr::Input const& input) { return input.frame >= frame; });
    actionIndex = static_cast<std::size_t>(std::distance(replay.inputs.begin(), it));
}

// ---------------------------------------------------------------------- files

std::filesystem::path Bot::macrosDir() const {
    auto mod = Mod::get();
    auto dir = mod ? std::filesystem::path(mod->getSaveDir()) / "macros"
                   : std::filesystem::path("macros");

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

bool Bot::save(std::string const& name, std::string* error) {
    if (replay.inputs.empty()) {
        if (error) *error = "Nothing recorded yet";
        return false;
    }

    auto clean = botutil::sanitizeFileName(name);
    if (clean.empty()) clean = "macro";

    auto path = macrosDir() / fmt::format("{}.gdr2", clean);
    auto data = replay.exportData(false);

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        if (error) *error = fmt::format("Cannot write {}", path.string());
        return false;
    }

    stream.write(reinterpret_cast<char const*>(data.data()), static_cast<std::streamsize>(data.size()));
    stream.close();

    macroPath = path;
    macroName = clean;
    setStatus(fmt::format("Saved {}", path.filename().string()));
    return true;
}

bool Bot::load(std::filesystem::path const& path, std::string* error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        if (error) *error = fmt::format("Cannot open {}", path.filename().string());
        return false;
    }

    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(stream)),
                                   std::istreambuf_iterator<char>());
    stream.close();

    if (data.empty()) {
        if (error) *error = "File is empty";
        return false;
    }

    auto imported = gdr::Replay::importData(data);
    if (imported.inputs.empty()) {
        if (error) *error = "No inputs found in file";
        return false;
    }

    pendingReplay = imported;
    macroPath = path;
    macroName = path.stem().string();

    setStatus(fmt::format("Loaded {} ({} inputs)", path.filename().string(), imported.inputs.size()));
    return true;
}

void Bot::reset(PlayLayer* pl) {
    if (pl) {
        if (pl->m_player1) pl->m_player1->releaseAllButtons();
        if (pl->m_player2) pl->m_player2->releaseAllButtons();
    }

    actionIndex = 0;
    respawnFrame = 0xFFFFFFFFu;
    previousFrame = 0;
    markers.clear();
}

// -------------------------------------------------------------- frame stepping

void Bot::setFrozen(bool value) {
    frozen = value;
    setStatus(value ? "Frozen (frame step)" : "Running");
}

// --------------------------------------------------------------- small helpers

namespace botutil {

std::string sanitizeFileName(std::string const& name) {
    std::string out;
    out.reserve(name.size());

    for (char c : name) {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '-' || c == '_' || c == ' ';
        if (ok) out.push_back(c);
    }

    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();

    if (out.size() > 64) out.resize(64);
    return out;
}

std::string levelName(PlayLayer* pl) {
    if (!pl || !pl->m_level) return "Unknown";
    std::string name = pl->m_level->m_levelName;
    return name.empty() ? "Unknown" : name;
}

std::string formatTime(double seconds) {
    int total = static_cast<int>(seconds);
    int minutes = total / 60;
    int rest = total % 60;
    return fmt::format("{:02d}:{:02d}", minutes, rest);
}

} // namespace botutil
