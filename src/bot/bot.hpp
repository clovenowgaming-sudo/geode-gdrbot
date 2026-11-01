#pragma once

#include <Geode/Geode.hpp>

#include "../gdr/gdr.hpp"
#include "snapshot.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

enum class BotState {
    Idle,
    Recording,
    Playing,
};

// Global bot state shared by the hooks and the UI. Single static instance,
// always accessed through Bot::get().
class Bot {
public:
    static Bot& get();

    BotState state = BotState::Idle;

    gdr::Replay replay;
    std::optional<gdr::Replay> pendingReplay;

    std::string macroName = "macro";
    std::filesystem::path macroPath;

    // playback
    std::size_t actionIndex = 0;
    unsigned int respawnFrame = 0xFFFFFFFFu;
    unsigned int previousFrame = 0;

    // recording
    bool suppressRecord = false;

    // practice fix
    std::unordered_map<CheckpointObject*, CheckpointMarker> markers;

    // misc
    bool frozen = false;
    std::string statusText = "Idle";

    float tps() const;
    unsigned int currentFrame() const;

    bool practiceFix() const;
    bool autoStop() const;
    bool autoRestart() const;
    int frameOffset() const;

    void setStatus(std::string const& text);

    // recording
    bool startRecording(PlayLayer* pl);
    void stopRecording(PlayLayer* pl);
    void recordInput(bool down, int button, bool player2);
    void trimRecordedInputs(unsigned int frame);

    // playback
    bool startPlaying(PlayLayer* pl);
    void stopPlaying(PlayLayer* pl);
    void applyInputs(unsigned int frame, GJBaseGameLayer* layer);
    void syncToFrame(unsigned int frame);

    // files
    std::filesystem::path macrosDir() const;
    bool save(std::string const& name, std::string* error = nullptr);
    bool load(std::filesystem::path const& path, std::string* error = nullptr);
    void reset(PlayLayer* pl);

    void setFrozen(bool value);
};

namespace botutil {
std::string sanitizeFileName(std::string const& name);
std::string levelName(PlayLayer* pl);
std::string formatTime(double seconds);
}
