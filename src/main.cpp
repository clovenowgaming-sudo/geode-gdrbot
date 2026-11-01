#include <Geode/Geode.hpp>

#include "bot/bot.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("GDRBot loaded - GDReplayFormat (.gdr2) recorder/player");
}
