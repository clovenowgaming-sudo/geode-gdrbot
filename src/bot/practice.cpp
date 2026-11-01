#include "practice.hpp"

#include "bot.hpp"

using namespace geode::prelude;

namespace practice {

void onStoreCheckpoint(PlayLayer* pl, CheckpointObject* checkpoint) {
    if (!pl || !checkpoint) return;

    auto& bot = Bot::get();
    if (!bot.practiceFix()) return;

    CheckpointMarker marker;
    marker.frame = bot.currentFrame();
    marker.previousFrame = bot.previousFrame;
    marker.seed = static_cast<unsigned int>(pl->m_randomSeed);

    if (pl->m_player1) marker.player1 = PlayerSnapshot::capture(pl->m_player1);
    if (pl->m_player2) marker.player2 = PlayerSnapshot::capture(pl->m_player2);

    bot.markers[checkpoint] = marker;
}

void onLoadCheckpoint(PlayLayer* pl, CheckpointObject* checkpoint) {
    if (!pl || !checkpoint) return;

    auto& bot = Bot::get();
    if (!bot.practiceFix()) return;

    auto it = bot.markers.find(checkpoint);
    if (it == bot.markers.end()) return;

    auto& marker = it->second;

    // Restore the RNG so random triggers behave like they did while recording.
    if (marker.seed != 0) {
        pl->m_randomSeed = marker.seed;
    }

    if (pl->m_player1) marker.player1.apply(pl->m_player1);

    if (pl->m_player2 && pl->m_levelSettings && pl->m_levelSettings->m_twoPlayerMode) {
        marker.player2.apply(pl->m_player2);
    }

    bot.previousFrame = marker.previousFrame;

    // A checkpoint load means the player went back in time: drop everything the
    // macro recorded after that point, and make playback skip the inputs that
    // already happened before the checkpoint.
    if (bot.state == BotState::Recording) {
        bot.trimRecordedInputs(marker.frame);
    } else if (bot.state == BotState::Playing) {
        bot.syncToFrame(marker.frame);
    }
}

} // namespace practice
