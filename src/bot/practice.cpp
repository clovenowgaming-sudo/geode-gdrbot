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
    marker.levelTime = pl->m_gameState.m_levelTime;

    if (pl->m_player1) marker.player1 = PlayerSnapshot::capture(pl->m_player1);
    if (pl->m_player2) marker.player2 = PlayerSnapshot::capture(pl->m_player2);

    bot.markers[checkpoint] = marker;
}

// Called AFTER PlayLayer::loadFromCheckpoint, so the game's own restore has
// already happened and nothing overwrites what is applied here.
void onLoadCheckpoint(PlayLayer* pl, CheckpointObject* checkpoint) {
    if (!pl || !checkpoint) return;

    auto& bot = Bot::get();

    // Tell the resetLevel hook a checkpoint was loaded, so it does not rewind
    // the macro back to frame 0.
    bot.restoredByCheckpoint = true;

    CheckpointMarker* marker = nullptr;
    if (bot.practiceFix()) {
        auto it = bot.markers.find(checkpoint);
        if (it != bot.markers.end()) marker = &it->second;
    }

    if (marker) {
        // keep the level clock identical to the moment the checkpoint was stored
        pl->m_gameState.m_levelTime = marker->levelTime;
    }

    unsigned int frame = marker ? marker->frame : bot.currentFrame();

    // Going back in time: drop everything recorded after the checkpoint, or
    // make playback continue from the first input after it.
    if (bot.state == BotState::Recording) {
        bot.trimRecordedInputs(frame);
    } else if (bot.state == BotState::Playing) {
        bot.syncToFrame(frame, pl);
    }

    if (!marker) return;

    // Restore the RNG so random triggers behave like they did while recording.
    if (marker->seed != 0) {
        pl->m_randomSeed = marker->seed;
    }

    // Applied after syncToFrame on purpose: re-pressing held buttons must not
    // change the restored velocity/position.
    if (pl->m_player1) marker->player1.apply(pl->m_player1);

    if (pl->m_player2 && pl->m_levelSettings && pl->m_levelSettings->m_twoPlayerMode) {
        marker->player2.apply(pl->m_player2);
    }

    bot.previousFrame = marker->previousFrame;
}

} // namespace practice
