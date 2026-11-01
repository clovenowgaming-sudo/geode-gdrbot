#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "bot/bot.hpp"
#include "bot/practice.hpp"
#include "ui/bot_layer.hpp"

using namespace geode::prelude;

// --------------------------------------------------------------------- recording

// Every press ends up here: player controls, bot playback, touch and keyboard
// layers. Recording at this level keeps P1/P2 and platformer left/right input
// in one place.
class $modify(GDRBotBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& bot = Bot::get();

        if (bot.state == BotState::Recording && !bot.suppressRecord) {
            // handleButton's third argument is "is player 1"; GDR stores "player2".
            bot.recordInput(down, button, !isPlayer1);
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

// ---------------------------------------------------------------------- playback

class $modify(GDRBotPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        Bot::get().reset(this);
        return true;
    }

    void postUpdate(float dt) {
        auto& bot = Bot::get();

        if (bot.state == BotState::Playing) {
            bot.applyInputs(bot.currentFrame(), this);
        }

        PlayLayer::postUpdate(dt);
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto& bot = Bot::get();
        bot.previousFrame = 0;

        if (bot.state == BotState::Playing) {
            bot.actionIndex = 0;
            bot.respawnFrame = 0xFFFFFFFFu;
        }
    }

    void loadFromCheckpoint(CheckpointObject* checkpoint) {
        practice::onLoadCheckpoint(this, checkpoint);
        PlayLayer::loadFromCheckpoint(checkpoint);
    }

    void storeCheckpoint(CheckpointObject* checkpoint) {
        PlayLayer::storeCheckpoint(checkpoint);
        practice::onStoreCheckpoint(this, checkpoint);
    }

    void onQuit() {
        auto& bot = Bot::get();

        if (bot.state == BotState::Playing) {
            bot.stopPlaying(this);
        } else if (bot.state == BotState::Recording) {
            bot.stopRecording(this);
        }

        PlayLayer::onQuit();
    }
};

// ------------------------------------------------------------------------ UI hook

class $modify(GDRBotPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) {
            auto children = this->getChildren();
            if (children) {
                for (unsigned int i = 0; i < children->count(); i++) {
                    auto child = typeinfo_cast<cocos2d::CCMenu*>(children->objectAtIndex(i));
                    if (child && child->getChildrenCount() >= 3) {
                        menu = child;
                        break;
                    }
                }
            }
        }

        if (!menu) return;

        auto sprite = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_04.png", .8f);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(GDRBotPauseLayer::onBotMenu));

        button->setID("gdrbot-menu-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
    }

    void onBotMenu(CCObject*) {
        BotLayer::open(this);
    }
};
