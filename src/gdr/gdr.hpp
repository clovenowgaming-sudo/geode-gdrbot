#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Minimal, self-contained implementation of the GDReplayFormat (GDR) container
// used by the bot ecosystem. Files are msgpack-encoded JSON documents; a plain
// JSON document is accepted on import as well.
//
// Layout of the encoded object:
//   author, description, duration, gameVersion, version, framerate, seed, coins,
//   ldm, bot { name, version }, level { id, name }, inputs [ { frame, btn, 2p,
//   down, ... } ]
//
// The bot stores its own data inside the input objects under "gdrbot" and inside
// a top-level "gdrbot" object, so other tools can ignore it safely.

namespace gdr {

struct BotInfo {
    std::string name = "GDRBot";
    std::string version = "1.0.0";
};

struct LevelInfo {
    std::uint32_t id = 0;
    std::string name;
};

struct Input {
    std::uint32_t frame = 0;
    int button = 1;
    bool player2 = false;
    bool down = false;

    Input() = default;
    Input(std::uint32_t frame, int button, bool player2, bool down)
        : frame(frame), button(button), player2(player2), down(down) {}

    bool operator<(Input const& other) const {
        return frame < other.frame;
    }
};

struct FrameFix {
    std::uint32_t frame = 0;
    bool ignore = false;
};

struct Replay {
    std::string author;
    std::string description;

    double duration = 0.0;
    float gameVersion = 2.208f;
    float version = 1.0f;
    float framerate = 240.f;

    int seed = 0;
    int coins = 0;
    bool ldm = false;

    BotInfo bot;
    LevelInfo level;

    std::vector<Input> inputs;
    std::vector<FrameFix> frameFixes;

    std::uint32_t frameForTime(double time) const {
        return static_cast<std::uint32_t>(time * static_cast<double>(framerate));
    }

    // Encodes to msgpack (or pretty-ish JSON when asJson is true).
    std::vector<std::uint8_t> exportData(bool asJson = false) const;
    static Replay importData(std::vector<std::uint8_t> const& data);
};

} // namespace gdr
