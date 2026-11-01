#pragma once

#include <Geode/Geode.hpp>

#include "snapshot.hpp"

// Practice fix, in two halves:
//
//  * when GD stores a checkpoint we remember the frame and both player
//    snapshots (the vanilla checkpoint keeps much less state, which is why
//    replays desync after dying past a checkpoint);
//  * when GD loads a checkpoint we restore those snapshots and repair the input
//    stream so the recorded run continues where it left off.
namespace practice {

void onStoreCheckpoint(PlayLayer* pl, CheckpointObject* checkpoint);
void onLoadCheckpoint(PlayLayer* pl, CheckpointObject* checkpoint);

} // namespace practice
